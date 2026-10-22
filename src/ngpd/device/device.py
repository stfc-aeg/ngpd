"""NGPD Device Class Module File."""

import logging
from ipaddress import ip_address
from math import log2
from os import listdir, path
from typing import Literal

from ngpd.adapter.exception import NgpdError
from ngpd.device.acquisition import NgpdAcquisition, NgpdData
from ngpd.device.ConfigHandlers import (
    BaseSubConfig,
    TailMeasureConfig,
    TriggerConfig,
    DiscriminationConfig,
    FilterConfig,
)
from ngpd.device.channel import NgpdChannel

from ngpd.device.pyngpd import (
    ANALOG_MAX_GAIN,
    ANALOG_MAX_OFFSET,
    DummyLevel,
    HistogramConfig,
    PyNgpd,
    SystemMonitor,
)

from ngpd.util import UsesNgpdLibrary

HistogramSetting = Literal[
    "separate_ngp", "nbits_height", "shift_height", "nbits_tail_sum", "shift_tail_sum"
]


class NgpdDevice:
    """NGPD Handling class, providing a bridge between Adapter/Controller and the interface class of PyNgpd."""

    def __init__(self, options: dict[str, str]):
        """Initialise the NGPD Device class."""
        self.ip = ip_address(options.get("ip_addr", "192.168.0.1"))
        self.num_cards = int(options.get("num_cards", 1))
        self.dummy_level = DummyLevel[options.get("dummy_level", "none").upper()]

        self.playback_file_dir = options.get("playback_dir", "web/config/files")
        self.config_file_dir = options.get("config_dir", "web/config/files")
        self.save_dir = options.get("save_dir", "saved_hist")
        self.selected_playback = ""
        self.selected_config_file = ""
        self.last_saved_filename = ""
        self.enable_playback = bool(self.dummy_level & DummyLevel.ADC)
        self.allowed_playback = [""]
        if path.exists(self.playback_file_dir) and path.isdir(self.playback_file_dir):
            self.allowed_playback.extend(
                [
                    f
                    for f in listdir(self.playback_file_dir)
                    if path.isfile(path.join(self.playback_file_dir, f))
                    and f.endswith(".d16")
                ]
            )
        else:
            logging.warning(
                (
                    f"Path {self.playback_file_dir} either does not exist, "
                    "is not a directory, or has invalid permissions for access"
                )
            )

        self.ngpd: PyNgpd = None

        self._adc_temp = -1.0
        self._preamp_temp = -1.0

        self._adc_voltages = {"none": -1}

        # Overtemp trip values
        self._adc_tcrit = -1
        self._preamp_tcrit = -1

        self._system_monitor = SystemMonitor()
        self._hist_config: HistogramConfig = None

        self.channels = [NgpdChannel(i) for i in range(8)]

        self.acquisition = NgpdAcquisition()
        self.dataHandler = NgpdData()

        self.configHandlers = {
            "base_sub": BaseSubConfig(self.channels),
            "filter": FilterConfig(self.channels),
            "discrimination": DiscriminationConfig(self.channels),
            "trigger": TriggerConfig(self.channels),
            "tail_measure": TailMeasureConfig(self.channels),
        }

        self.tree = {
            # lambdas in dict comprehension have scoping issues, so the index
            # is getting set as a default argument (eg: lambda i=i) to bind it per channel.
            # Without this, all of the channels would affect only the last (channel_7)
            # this is also why were using the list index rather than iterating over the list itself
            "analog": {
                f"channel_{i}": {
                    "gain": (
                        lambda i=i: self.channels[i].analog_gain,
                        lambda v, i=i: self.channels[i].set_analog_gain(v),
                        {
                            "description": "ADC Analog Gain",
                            "min": 0,
                            "max": ANALOG_MAX_GAIN,
                        },
                    ),
                    "offset": (
                        lambda i=i: self.channels[i].analog_offset,
                        lambda v, i=i: self.channels[i].set_analog_offset(v),
                        {
                            "description": "Preamp Analog Offset",
                            "min": 0,
                            "max": ANALOG_MAX_OFFSET,
                        },
                    ),
                }
                for i in range(len(self.channels))
            },
            "base_sub": self.configHandlers["base_sub"].tree,
            "filter": self.configHandlers["filter"].tree,
            "discrimination": self.configHandlers["discrimination"].tree,
            "tail_measure": self.configHandlers["tail_measure"].tree,
            "trigger": self.configHandlers["trigger"].tree,
            "playback": {
                "enabled": (
                    lambda: self.enable_playback,
                    self.set_playback_enable,
                    {"description": "Enable Playback Mode"},
                ),
                "file_name": (
                    lambda: self.selected_playback,
                    self.set_playback_file,
                    {
                        "description": "Name of the playback file",
                        "allowed_values": self.allowed_playback,
                    },
                ),
            },
            "histogram": {
                "num_bins_height": (
                    lambda: 2**self.hist_config.nbits_height,
                    lambda v: self.set_histogram_config("nbits_height", int(log2(v))),
                    {
                        "description": "Number of Bins to use for Pulse Height",
                        "allowed_values": [2**i for i in range(12)],
                    },
                ),
                "num_bins_tailsum": (
                    lambda: 2**self.hist_config.nbits_tail_sum,
                    lambda v: self.set_histogram_config("nbits_tail_sum", int(log2(v))),
                    {
                        "description": "Number of Bins to use for Tail Sum",
                        "allowed_values": [2**i for i in range(12)],
                    },
                ),
                "shift_height": (
                    lambda: self.hist_config.shift_height,
                    lambda v: self.set_histogram_config("shift_height", v),
                    {"description": "Value to Bitwise Right Shift the Height by"},
                ),
                "shift_tailsum": (
                    lambda: self.hist_config.shift_tail_sum,
                    lambda v: self.set_histogram_config("shift_tail_sum", v),
                    {"description": "Value to Bitwise Right Shift the tailsum by"},
                ),
                "separate_ngp": (
                    lambda: self.hist_config.separate_ngp,
                    lambda v: self.set_histogram_config("separate_ngp", v),
                    {
                        "description": "Enable separate histograms for Neutron, Gamma, and Pileup"
                    },
                ),
            },
            "settings_file": {
                "save": (
                    lambda: self.selected_config_file,
                    lambda file: self.save_config(file),
                    {"description": "Save the current system config to a HDF5 file"},
                ),
                "load": (
                    lambda: self.selected_config_file,
                    lambda file: self.load_config(file),
                    {"description": "Load the config from the selected HDF5 file"},
                ),
                "available": (
                    lambda: [
                        filename
                        for filename in listdir(self.config_file_dir)
                        if path.isfile(path.join(self.config_file_dir, filename))
                        and (filename.endswith(".h5") or filename.endswith(".hdf5"))
                    ],
                    None,
                    {
                        "description": "List of currently available config files saved locally"
                    },
                ),
            },
        }

    def configure(self):
        """Configure the device class and it's connection to the NGPD system."""
        # we have to subtract 1 from the Ip Addr due to logic in William's code,
        # which adds the one back on when initialising
        logging.debug("Configuring Ngpd Device")
        self.ngpd = PyNgpd(str(self.ip - 1), self.num_cards, self.dummy_level)
        self.ngpd.setup_run_mode(self.dummy_level & DummyLevel.ADC)
        for channel in self.channels:
            channel.configure(self.ngpd)

        self.acquisition.configure(self.ngpd)
        self.dataHandler.configure(self.ngpd)

    @property
    def adc_temp(self):
        """Temperature of the ADCs on the PCB."""
        if self.ngpd:
            self._adc_temp = self.ngpd.read_adc_temp()
        return self._adc_temp

    @property
    def preamp_temp(self):
        """Temperature of the Pre-amps on the PCB."""
        if self.ngpd:
            self._preamp_temp = self.ngpd.read_preamp_temp()
        return self._preamp_temp

    @property
    def adc_tcrit(self):
        """The trigger temperature at which the ADCs will shut down to prevent damage."""
        if self.ngpd:
            tcrit = self.ngpd.read_adc_tcrit()
            if not tcrit < 0:
                # read_adc_trcrit returns -1 if we're simulating ADCs (due to Dummy Level)
                self._adc_tcrit = tcrit
        return self._adc_tcrit

    @property
    def preamp_tcrit(self):
        """The trigger temperature at which the Pre-Amps will shut down to prevent damage."""
        if self.ngpd:
            tcrit = self.ngpd.read_preamp_tcrit()
            if not tcrit < 0:
                self._preamp_tcrit = tcrit
        return self._preamp_tcrit

    @property
    def adc_voltages(self):
        """Voltage signals for the ADCs."""
        if self.ngpd:
            self._adc_voltages = self.ngpd.read_adc_voltages()
        return self._adc_voltages

    @property
    def system_monitor(self):
        """FPGA Monitoring dataclass."""
        if self.ngpd:
            self._system_monitor = self.ngpd.read_fpga_data()
        return self._system_monitor

    @property
    def hist_config(self):
        """Histogram Configuration Dataclass."""
        if self.ngpd and self._hist_config is None:
            logging.debug("READING HISTOGRAM CONFIG")
            self._hist_config = self.ngpd.read_hist_conf(0)
        return HistogramConfig() if self._hist_config is None else self._hist_config

    @UsesNgpdLibrary
    def set_adc_tcrit(self, value: int):
        """Set the ADC shutdown Trigger Temperature."""
        self._adc_tcrit = value
        self.ngpd.write_adc_tcrit(value)

    @UsesNgpdLibrary
    def set_preamp_tcrit(self, value: int):
        """Set the Pre-Amp shutdown Trigger Temperature."""
        self._preamp_tcrit = value
        self.ngpd.write_preamp_tcrit(value)

    @UsesNgpdLibrary
    def set_playback_enable(self, value: bool):
        """Set the NGPD to enable or disable Playback Mode."""
        self.enable_playback = value
        self.ngpd.setup_run_mode(self.enable_playback)

    @UsesNgpdLibrary
    def set_playback_file(self, fname: str):
        """Load the specified playback file into the NGPD system."""
        self.selected_playback = fname
        self.ngpd.load_playback(-1, path.join(self.playback_file_dir, fname))

    @UsesNgpdLibrary
    def set_histogram_config(self, setting: HistogramSetting, value: bool | int):
        """Set the specific Histogram Config setting in the NGPD System."""
        if hasattr(self.hist_config, setting):
            setattr(self.hist_config, setting, value)

        self.ngpd.setup_hist(-1, self.hist_config)

    def set_run(self, run: bool):
        """Start or stop the acquisition."""
        if run:
            self.acquisition.start()
        else:
            self.acquisition.stop()

    @UsesNgpdLibrary
    def save_config(self, filename: str):
        """Save the Config settings to a HDF5 file."""
        try:
            base, ext = path.splitext(path.basename(filename))
            if ext not in [".h5", ".hdf5"]:
                ext = ".hdf5"
            filename = f"{base}{ext}"

            fullpath = path.join(self.config_file_dir, filename)
            self.selected_config_file = filename
            if path.isdir(self.config_file_dir):
                if path.isfile(fullpath):
                    logging.warning(f"Overwriting Config File {filename}")
                self.ngpd.save_settings(fullpath)
            else:
                raise NgpdError(
                    f"Config Directory {self.config_file_dir} cannot be opened or found. Check Path."
                )
        except OSError as err:
            raise NgpdError(err)

    @UsesNgpdLibrary
    def load_config(self, filename: str):
        """Load the config settings from a HDF5 file."""
        try:
            base, ext = path.splitext(path.basename(filename))
            if ext not in [".h5", ".hdf5"]:
                ext = ".hdf5"
            filename = f"{base}{ext}"
            logging.debug(f"Loading config from file {filename}")

            if path.isdir(self.config_file_dir) and path.isfile(
                path.join(self.config_file_dir, filename)
            ):
                self.selected_config_file = filename
                self.ngpd.restore_settings(path.join(self.config_file_dir, filename))
                for channel in self.channels:
                    # reset channel reads to make sure displayed vals on GUI match changes made by config file
                    channel._filter = None
                    channel._trigger = None
                    channel._base_sub = None
                    channel._tail_measure = None
            else:
                raise NgpdError(
                    f"{filename} cannot be opened or found. Config Not Loaded."
                )
        except OSError as err:
            raise NgpdError(err)

    def upload_config(self, filename: str, data: bytes):
        """Create a new config file from one uploaded via a POST request."""
        try:
            base, ext = path.splitext(path.basename(filename))
            if ext not in [".h5", ".hdf5"]:
                ext = ".hdf5"
            filename = f"{base}{ext}"
            logging.debug(f"Uploaded File Name: {filename}")

            increment = 0
            while path.isfile(path.join(self.config_file_dir, filename)):
                logging.debug(f"{filename} already exists")
                # if file of that name already exists in the directory, add an incrememnting number to avoid overwriting the existing config
                increment += 1
                filename = f"{base} ({increment}){ext}"

            with open(path.join(self.config_file_dir, filename), "xb") as file:
                file.write(data)
            self.selected_config_file = filename
            return filename
        except FileExistsError as err:
            logging.error(f"File Exists! {err}")
            raise NgpdError(f"Config File already exists, can't save new one: {err}")
        except OSError as err:
            logging.error(f"OS error uploading file: {err}")
            raise NgpdError(f"OS Error when trying to upload new config: {err}")

    def download_config(self, filename: str = None):
        """Provide the requested Config File as a Bytes object."""
        filename = filename or self.selected_config_file
        filename = self.validate_filename(filename)
        return self.dataHandler.download_data(path.join(self.config_file_dir, filename))


    def save_data(self, filename: str):
        """Save the Histogram data to a HDF5 file."""
        filename = self.validate_filename(filename)

        self.dataHandler.save_data(path.join(self.save_dir, filename))
        self.last_saved_filename = filename  # done after the save_data call so if that fails for any reason, we dont change the variable

    def get_saved_data(self, filename: str = None):
        """Return the saved HDF5 file as a bytes object."""
        filename = filename or self.last_saved_filename
        filename = self.validate_filename(filename)

        fullpath = path.join(self.save_dir, filename)
        return self.dataHandler.download_data(fullpath)

    def validate_filename(self, filename: str):
        """Ensure a provided filename has a HDF5 extension and strip any additional path parts to ensure it's not accesing invalid file locations."""
        base, ext = path.splitext(path.basename(filename))
        if ext not in [".h5", ".hdf5"]:
            ext = ".hdf5"
        return f"{base}{ext}"
