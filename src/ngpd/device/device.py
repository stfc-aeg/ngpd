"""NGPD Device Class Module File."""

import logging
from ipaddress import ip_address
from math import log2
from os import listdir, path
from typing import Literal
from ngpd.device.acquisition import NgpdAcquisition, NgpdData
from ngpd.device.channel import NgpdChannel
from ngpd.device.pyngpd import (
    ANALOG_MAX_GAIN,
    ANALOG_MAX_OFFSET,
    BSUB_MAX_ERROR,
    BSUB_MAX_FIXED,
    BSUB_MIN_FIXED,
    MEASURE_MAX_DELAY,
    MEASURE_MAX_FALL_TIME,
    MEASURE_MAX_HEIGHT,
    MEASURE_MAX_SUM_NUM,
    MEASURE_MAX_TAIL_COUNT,
    MIN_DIV_CONT,
    NUM_DIV_CONT,
    TRIG_MAX_DATA_DELAY,
    TRIG_MAX_DELAY_AB,
    TRIG_MAX_SEP,
    TRIG_MAX_THRES,
    TRIG_MAX_TRIG_DELAY,
    TRIG_MAX_TRIG_STRETCH,
    TRIG_MIN_DATA_DELAY,
    TRIG_MIN_DELAY_AB,
    TRIG_MIN_SEP,
    TRIG_MIN_THRES,
    TRIG_MIN_TRIG_DELAY,
    TRIG_MIN_TRIG_STRETCH,
    DummyLevel,
    FilterType,
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

    DIV_CONT_ALLOWED = [MIN_DIV_CONT * (2**i) for i in range(NUM_DIV_CONT)]

    def __init__(self, options: dict[str, str]):
        """Initialise the NGPD Device class."""
        self.ip = ip_address(options.get("ip_addr", "192.168.0.1"))
        self.num_cards = int(options.get("num_cards", 1))
        self.dummy_level = DummyLevel[options.get("dummy_level", "none").upper()]

        self.playback_file_dir = options.get("playback_dir", "web/config/files")
        self.selected_playback = ""
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

        self._adc_temp = -1
        self._preamp_temp = -1

        self._adc_voltages = {"none": -1}

        # Overtemp trip values
        self._adc_tcrit = -1
        self._preamp_tcrit = -1

        self._system_monitor = SystemMonitor()
        self._hist_config: HistogramConfig = None

        self.channels = [NgpdChannel(i) for i in range(8)]

        self.acquisition = NgpdAcquisition()
        self.dataHandler = NgpdData()

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
            "base_sub": {
                f"channel_{i}": {
                    "use_fixed": (
                        lambda i=i: self.channels[i].base_sub.use_fixed,
                        lambda v, i=i: self.channels[i].set_base_sub("use_fixed", v),
                        {
                            "description": "Toggle the use of a fixed Base Subtraction Value"
                        },
                    ),
                    "fixed": (
                        lambda i=i: self.channels[i].base_sub.fixed,
                        lambda v, i=i: self.channels[i].set_base_sub("fixed", v),
                        {
                            "description": "Fixed Base Subtraction Value",
                            "min": BSUB_MIN_FIXED,
                            "max": BSUB_MAX_FIXED,
                        },
                    ),
                    "error_limit": (
                        lambda i=i: self.channels[i].base_sub.error_limit,
                        lambda v, i=i: self.channels[i].set_base_sub("error_limit", v),
                        {
                            "description": "Base Subtraction Error Limit",
                            "min": 0,
                            "max": BSUB_MAX_ERROR,
                        },
                    ),
                    "div_cont": (
                        lambda i=i: (
                            MIN_DIV_CONT * (2 ** self.channels[i].base_sub.div_cont)
                        ),
                        lambda v, i=i: self.channels[i].set_base_sub(
                            "div_cont", int(log2(v / MIN_DIV_CONT))
                        ),
                        {
                            "description": "Division Count for Base Subtraction",
                            "allowed_values": self.DIV_CONT_ALLOWED,
                        },
                    ),
                }
                for i in range(len(self.channels))
            },
            "filter": {
                # TODO: Instead of using the Filter Struct iargs etc,
                # setup the actual params for each filter?
                f"channel_{i}": {
                    "type": (
                        lambda i=i: self.channels[i].filter.filt_type.name.lower(),
                        lambda v, i=i: self.channels[i].set_filter(
                            "type", FilterType[v.upper()]
                        ),
                        {
                            "description": "Type of Filter",
                            "allowed_values": [
                                e.name.lower()
                                for e in FilterType
                                if e.name not in ["UNKNOWN", "CUSTOM"]
                            ],
                        },
                    ),
                    "arg_1": (
                        lambda i=i: self.channels[i].filter.iarg1,
                        lambda v, i=i: self.channels[i].set_filter("arg1", v),
                        {
                            "description": (
                                "First integer filter argument. "
                                "Exact use is dependant on type of filter"
                            )
                        },
                    ),
                    "arg_2": (
                        lambda i=i: self.channels[i].filter.iarg2,
                        lambda v, i=i: self.channels[i].set_filter("arg2", v),
                        {
                            "description": (
                                "Second integer filter argument. "
                                "Exact use is dependant on type of filter"
                            )
                        },
                    ),
                    "arg_float": (
                        lambda i=i: self.channels[i].filter.darg,
                        lambda v, i=i: self.channels[i].set_filter("darg", v),
                        {
                            "description": (
                                "Float filter argument. "
                                "Exact use is dependant on type of filter"
                            )
                        },
                    ),
                }
                for i in range(len(self.channels))
            },
            "discrimination": {
                f"channel_{i}": {
                    "height_min": (
                        lambda i=i: self.channels[i].tail_measure.min_height,
                        lambda v, i=i: self.channels[i].set_tail_measure(
                            "min_height", v
                        ),
                        {
                            "description": "Minimum Discrimination Height",
                            "min": 0,
                            "max": MEASURE_MAX_HEIGHT,
                        },
                    ),
                    "height_max": (
                        lambda i=i: self.channels[i].tail_measure.max_height,
                        lambda v, i=i: self.channels[i].set_tail_measure(
                            "max_height", v
                        ),
                        {
                            "description": "Maximum Discrimination Height",
                            "min": 0,
                            "max": MEASURE_MAX_HEIGHT,
                        },
                    ),
                    "adaptive": (
                        lambda i=i: self.channels[i].tail_measure.adaptive_tail_sum,
                        lambda v, i=i: self.channels[i].set_tail_measure(
                            "adaptive_tail_sum", v
                        ),
                        {"description": "Toggle the Adaptive Tail Sum"},
                    ),
                    "enable_tail_sum": (
                        lambda i=i: not self.channels[i].tail_measure.ignore_tail_sum,
                        lambda v, i=i: self.channels[i].set_tail_measure(
                            "ignore_tail_sum", not v
                        ),
                        {
                            "description": "Toggle the use of the Tail sum in the discrimination"
                        },
                    ),
                    "enable_fall_time": (
                        lambda i=i: not self.channels[i].tail_measure.ignore_fall_time,
                        lambda v, i=i: self.channels[i].set_tail_measure(
                            "ignore_fall_time", not v
                        ),
                        {
                            "description": "Toggle the use of the Fall Time in the discrimination"
                        },
                    ),
                    "min_fall": (
                        lambda i=i: self.channels[i].tail_measure.min_fall,
                        lambda v, i=i: self.channels[i].set_tail_measure("min_fall", v),
                        {
                            "description": "Minimum Fall Time",
                            "min": 0,
                            "max": MEASURE_MAX_FALL_TIME,
                        },
                    ),
                    "max_fall": (
                        lambda i=i: self.channels[i].tail_measure.max_fall,
                        lambda v, i=i: self.channels[i].set_tail_measure("max_fall", v),
                        {
                            "description": "Maximum Fall Time",
                            "min": 0,
                            "max": MEASURE_MAX_FALL_TIME,
                        },
                    ),
                    "min_count": (
                        lambda i=i: self.channels[i].tail_measure.min_count,
                        lambda v, i=i: self.channels[i].set_tail_measure(
                            "min_count", v
                        ),
                        {
                            "description": "Minimum Fall Time",
                            "min": 0,
                            "max": MEASURE_MAX_TAIL_COUNT,
                        },
                    ),
                    "threshold_c": (
                        lambda i=i: self.channels[i].tail_measure.tail_thres_c,
                        lambda v, i=i: self.channels[i].set_tail_measure(
                            "tail_thres_c", v
                        ),
                        {
                            "description": "C parameter for threshold calculation",
                            "min": -0x4000000,
                            "max": 0x4000000 - 1,
                        },  # TODO: set up as consts in PyNgpd
                    ),
                    "threshold_m": (
                        lambda i=i: self.channels[i].tail_measure.tail_thres_m,
                        lambda v, i=i: self.channels[i].set_tail_measure(
                            "tail_thres_m", v
                        ),
                        {
                            "description": "M parameter for threshold calculation",
                            "min": 0,
                        },
                    ),
                }
                for i in range(len(self.channels))
            },
            "tail_measure": {
                f"channel_{i}": {
                    "delay": (
                        lambda i=i: self.channels[i].tail_measure.tail_sum_delay,
                        lambda v, i=i: self.channels[i].set_tail_measure(
                            "tail_sum_delay", v
                        ),
                        {
                            "description": "Tail Sum Delay",
                            "min": 0,
                            "max": MEASURE_MAX_DELAY,
                        },
                    ),
                    "num_sample": (
                        lambda i=i: self.channels[i].tail_measure.tail_sum_sample,
                        lambda v, i=i: self.channels[i].set_tail_measure(
                            "tail_sum_sample", v
                        ),
                        {
                            "description": "Number of samples for Tail Sum measurement",
                            "min": 1,
                            "max": MEASURE_MAX_SUM_NUM,
                        },
                    ),
                    "fall_time_frac": (
                        lambda i=i: self.channels[i].tail_measure.fall_time_frac,
                        lambda v, i=i: self.channels[i].set_tail_measure(
                            "fall_time_frac", v
                        ),
                        {
                            "description": "Ratio for fall time calculation",
                            "min": 0.0,
                            "max": 1.0,
                        },
                    ),
                    "enable_tail_subtract": (
                        lambda i=i: self.channels[i].tail_measure.enable_tail_subtract,
                        lambda v, i=i: self.channels[i].set_tail_measure(
                            "enable_tail_subtract", v
                        ),
                        {"description": "Toggle Tail Subtraction"},
                    ),
                    "enable_subtract_test": (
                        lambda i=i: self.channels[i].tail_measure.enable_subtract_test,
                        lambda v, i=i: self.channels[i].set_tail_measure(
                            "enable_subtract_test", v
                        ),
                        {"description": "Toggle Tail Subtraction Test"},
                    ),
                    "enable_subtract_neutron": (
                        lambda i=i: (
                            self.channels[i].tail_measure.enable_subtract_neutron
                        ),
                        lambda v, i=i: self.channels[i].set_tail_measure(
                            "enable_subtract_neutron", v
                        ),
                        {"description": "Toggle Test Neutron Subtraction"},
                    ),
                }
                for i in range(len(self.channels))
            },
            "trigger": {
                f"channel_{i}": {
                    "threshold": (
                        lambda i=i: self.channels[i].trigger.thres,
                        lambda v, i=i: self.channels[i].set_trigger("thres", v),
                        {
                            "description": "Trigger Threshold",
                            "min": TRIG_MIN_THRES,
                            "max": TRIG_MAX_THRES,
                        },
                    ),
                    "separation": (
                        lambda i=i: self.channels[i].trigger.sep,
                        lambda v, i=i: self.channels[i].set_trigger("sep", v),
                        {
                            "description": "Trigger Separation",
                            "min": TRIG_MIN_SEP,
                            "max": TRIG_MAX_SEP,
                        },
                    ),
                    "data_delay": (
                        lambda i=i: self.channels[i].trigger.data_delay,
                        lambda v, i=i: self.channels[i].set_trigger("data_delay", v),
                        {
                            "description": "Trigger Data Delay",
                            "min": TRIG_MIN_DATA_DELAY,
                            "max": TRIG_MAX_DATA_DELAY,
                        },
                    ),
                    "trig_delay": (
                        lambda i=i: self.channels[i].trigger.trig_delay,
                        lambda v, i=i: self.channels[i].set_trigger("trig_delay", v),
                        {
                            "description": "Trigger Delay",
                            "min": TRIG_MIN_TRIG_DELAY,
                            "max": TRIG_MAX_TRIG_DELAY,
                        },
                    ),
                    "delay_a": (
                        lambda i=i: self.channels[i].trigger.delay_a,
                        lambda v, i=i: self.channels[i].set_trigger("delay_a", v),
                        {
                            "description": "Trigger Delay A Parameter",
                            "min": TRIG_MIN_DELAY_AB,
                            "max": TRIG_MAX_DELAY_AB,
                        },
                    ),
                    "delay_b": (
                        lambda i=i: self.channels[i].trigger.delay_b,
                        lambda v, i=i: self.channels[i].set_trigger("delay_b", v),
                        {
                            "description": "Trigger Delay B Parameter",
                            "min": TRIG_MIN_DELAY_AB,
                            "max": TRIG_MAX_DELAY_AB,
                        },
                    ),
                    "width_a": (
                        lambda i=i: self.channels[i].trigger.width_a,
                        lambda v, i=i: self.channels[i].set_trigger("width_a", v),
                        {
                            "description": "Trigger Width A Parameter",
                            "min": TRIG_MIN_TRIG_STRETCH,
                            "max": TRIG_MAX_TRIG_STRETCH,
                        },
                    ),
                    "width_b": (
                        lambda i=i: self.channels[i].trigger.width_b,
                        lambda v, i=i: self.channels[i].set_trigger("width_b", v),
                        {
                            "description": "Trigger Width B Parameter",
                            "min": TRIG_MIN_TRIG_STRETCH,
                            "max": TRIG_MAX_TRIG_STRETCH,
                        },
                    ),
                }
                for i in range(len(self.channels))
            },
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
