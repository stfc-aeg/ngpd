"""Odin Control Controller Class Module."""

import logging
from dataclasses import asdict
from functools import partial
from ipaddress import ip_address

from odin_control.adapters.base_controller import BaseController
from odin_control.adapters.parameter_tree import ParameterTree

from ngpd.adapter.exception import NgpdError
from ngpd.device import NgpdDevice
from ngpd.device.pyngpd import DummyLevel


class NgpdController(BaseController):
    """Controller class for Ngpd."""

    def __init__(self, options: dict[str, str]):
        """Initialise the controller, creating the Param Tree."""
        self.options = options

        self.device = NgpdDevice(self.options)

        device_tree = {
            "base_ip": (
                lambda: str(self.device.ip),
                self.set_ip,
                {
                    "description": (
                        "IP Address of the NGPD Board. "
                        "In a multi-board system this is the first board"
                    )
                },
            ),
            "num_cards": (
                lambda: self.device.num_cards,
                partial(setattr, self.device, "num_cards"),
                {
                    "description": "Number of NGPD cards in the whole system",
                    "min": 1,
                    "max": 8,
                },
            ),
            "dummy_system": (
                lambda: self.device.dummy_level.name.lower(),
                self.set_dummy_level,
                {
                    "description": "Set what parts of the system are simulated by the software",
                    "allowed_values": [level.name.lower() for level in DummyLevel],
                },
            ),
            "connect": (
                lambda: self.device.ngpd is not None,
                lambda _: self.device.configure(),
                {
                    "description": "Configure the NGPD card(s) and initializes the connection(s)"
                },
            ),
        }

        monitoring_tree = {
            "adc": {
                "temperature": (
                    lambda: self.device.adc_temp,
                    None,
                    {
                        "description": "Temperature reading from the ADC board",
                        "units": "°C",
                    },
                ),
                "voltages": (
                    lambda: self.device.adc_voltages,
                    None,
                    {"description": "ADC Voltage Signals"},
                ),
                "trip_temp": (
                    lambda: self.device.adc_tcrit,
                    lambda v: self.device.set_adc_tcrit(v),
                    {
                        "description": "Maximum allowed temperature before tripping protection"
                    },
                ),
            },
            "preamp": {
                "temperature": (
                    lambda: self.device.preamp_temp,
                    None,
                    {
                        "description": "Temperature reading from the Pre-Amp board",
                        "units": "°C",
                    },
                ),
                "trip_temp": (
                    lambda: self.device.preamp_tcrit,
                    lambda v: self.device.set_preamp_tcrit(v),
                    {
                        "description": "Maximum allowed temperature before tripping protection"
                    },
                ),
            },
            "fpga": {
                "temperature": (
                    lambda: (
                        max(
                            self.device.system_monitor.AMS_PSTempLPD,
                            self.device.system_monitor.AMS_PSTempFPD,
                            self.device.system_monitor.XADC_Temp,
                        )
                        * 0.001
                    ),
                    None,
                    {"description": "Temperature reading from the FPGA", "units": "°C"},
                ),
                "voltages": (
                    lambda: {
                        key: val * 0.0001
                        for key, val in asdict(self.device.system_monitor).items()
                        if key not in ["AMS_PSTempLPD", "AMS_PSTempFPD", "XADC_Temp"]
                    },
                    None,
                    {"description": "Dictionary of voltages on the FPGA"},
                ),
            },
        }

        acquisition_tree = {
            **self.device.acquisition.tree,
            "download": (
                "Directly Access this Param to download the last saved data file.",
                None,
                {"description": "The latest Saved HDF5 File"},
            ),
            "graph": {
                "hist": (
                    lambda: "Directly access this Param to get the actual data",
                    None,
                    {
                        "description": "Histogram Data from selected channel, encoded as a Base64 string"
                    },
                ),
                "tailsum": (
                    lambda: "Directly access this Param to get the actual data",
                    None,
                    {
                        "description": "Tailsum Scatterplot data encoded as Base64 string"
                    },
                ),
                "pulse_height": (
                    lambda: "Directly access this Param to get the actual data",
                    None,
                    {
                        "description": "Pulseheight Scatterplot data encoded as Base64 string"
                    },
                ),
                "refresh_data": (
                    None,
                    lambda _: self.device.dataHandler.refresh_data(),
                    {
                        "description": "Tell the Data Handler to re-read the data from the NGPD"
                    },
                ),
                "data_shape": (
                    lambda: self.device.dataHandler.data_shape,
                    None,
                    {"description": "Shape of Histogram Data"},
                ),
                "channel": (
                    lambda: self.device.dataHandler.chan,
                    self.device.dataHandler.set_channel,
                    {"description": "Select channel to graph data", "min": 0, "max": 7},
                ),
                "signal": (
                    lambda: self.device.dataHandler.select_display,
                    lambda v: setattr(self.device.dataHandler, "select_display", v),
                    {
                        "description": "Select which signal to histogram",
                        "allowed_values": ["neutron", "gamma", "pileup", "all"],
                    },
                ),
                "save": (
                    lambda: self.device.last_saved_filename,
                    lambda v: self.device.save_data(v),
                    {
                        "description": "Save data to the specified filename. Getting this param returns the file as a bytes object, for downloading."
                    },
                ),
                "download": (
                    "Directly GET this param to download the last saved histogram file",
                    None,
                    {"description": "Param to download the latest saved HDF5 file."},
                ),
            },
        }

        self.param_tree = ParameterTree(
            {
                "device": device_tree,
                "monitor": monitoring_tree,
                "config": self.device.tree,
                "acq": acquisition_tree,
            }
        )

    def initialize(self, adapters):
        """Run when all adapters in the system have loaded.

        Allows for inter adapter communication, or other setup that needs to wait for all adapters to load.
        """
        self.adapters = adapters
        logging.debug(f"Adapters initialized: {list(adapters.keys())}")
        if self.options.get("auto_connect"):
            self.device.configure()
        # Add to param tree if needed post-initialization

    def cleanup(self):
        """Clean up controller resources and state."""
        logging.info("Cleaning up NgpdController")

    def get(self, path, with_metadata=False):
        """Get data from the controller at the specified path."""
        try:
            # using Python 3.10 match case
            match path.strip("/"):
                case "acq/graph/hist":
                    return {"value": self.device.dataHandler.get_data()}
                case "acq/graph/tailsum":
                    return {"value": self.device.dataHandler.get_tailsum()}
                case "acq/graph/pulse_height":
                    return {"value": self.device.dataHandler.get_pulse_height()}
                case "acq/graph/download":
                    logging.warning("Returning File Data")
                    return self.device.get_saved_data()
                case _:
                    # default behaviour
                    return self.param_tree.get(path, with_metadata)
        except Exception as error:
            logging.error(error)
            raise NgpdError(error)

    def set(self, path, data):
        """Set data in the controller at the specified path."""
        try:
            self.param_tree.set(path, data)
        except Exception as error:
            logging.error(error)
            raise NgpdError(error)

    def set_ip(self, ip: str):
        """Set the IP Address of the first board."""
        self.device.ip = ip_address(ip)

    def set_dummy_level(self, dummy: str):
        """Set level of dummy system, which defines how much to mock instead of accessing actual hardware."""
        self.device.dummy_level = DummyLevel[dummy.upper()]
