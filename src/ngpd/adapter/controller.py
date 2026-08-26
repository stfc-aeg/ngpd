"""Odin Control Controller Class Module."""

import logging
from ipaddress import ip_address
from functools import partial
from dataclasses import asdict
from odin_control.adapters.parameter_tree import ParameterTree, ParameterTreeError
from odin_control.adapters.base_controller import BaseController, BaseError
from ngpd.device.pyngpd import DummyLevel
from ngpd.util import NgpdLibException
from ngpd.device import NgpdDevice


class NgpdError(BaseError):
    """Simple exception class to wrap lower-level exceptions."""


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
                    "description": ("IP Address of the NGPD Board. "
                                    "In a multi-board system this is the first board")
                }
            ),
            "num_cards": (
                lambda: self.device.num_cards,
                partial(setattr, self.device, "num_cards"),
                {"description": "Number of NGPD cards in the whole system",
                 "min": 1, "max": 8}
            ),
            "dummy_system": (
                lambda: self.device.dummy_level.name.lower(),
                self.set_dummy_level,
                {"description": "Set what parts of the system are simulated by the software",
                 "allowed_values": [level.name.lower() for level in DummyLevel]}
            ),
            "connect": (
                lambda: self.device.ngpd is not None, lambda _: self.device.configure(),
                {"description": "Configure the NGPD card(s) and initializes the connection(s)"}
            )
        }

        monitoring_tree = {
            "adc": {
                "temperature": (
                    lambda: self.device.adc_temp, None,
                    {"description": "Temperature reading from the ADC board",
                     "units": "°C"}
                ),
                "voltages": (
                    lambda: self.device.adc_voltages, None,
                    {"description": "ADC Voltage Signals"}
                ),
                "trip_temp": (
                    lambda: self.device.adc_tcrit,
                    lambda v: self.device.set_adc_tcrit(v),
                    {"description": "Maximum allowed temperature before tripping protection"}
                )
            },
            "preamp": {
                "temperature": (
                    lambda: self.device.preamp_temp, None,
                    {"description": "Temperature reading from the Pre-Amp board",
                     "units": "°C"}
                ),
                "trip_temp": (
                    lambda: self.device.preamp_tcrit,
                    lambda v: self.device.set_preamp_tcrit(v),
                    {"description": "Maximum allowed temperature before tripping protection"}
                )
            },
            "fpga": {
                "temperature": (
                    lambda: max(
                        self.device.system_monitor.AMS_PSTempLPD,
                        self.device.system_monitor.AMS_PSTempFPD,
                        self.device.system_monitor.XADC_Temp) * 0.001,
                    None,
                    {"description": "Temperature reading from the FPGA",
                     "units": "°C"}
                ),
                "voltages": (
                    lambda: {key: val * 0.0001 for key, val in asdict(self.device.system_monitor).items()
                             if key not in ["AMS_PSTempLPD", "AMS_PSTempFPD", "XADC_Temp"]},
                    None,
                    {"description": "Dictionary of voltages on the FPGA"}
                )
            }
        }

        acquisition_tree = {
            "num_cycles": (
                lambda: self.device.acquisition.num_cyles,
                lambda v: setattr(self.device.acquisition, "num_cycles", v),
                {"description": "Number of Cycles for the Acquisition."}
            ),
            "frame_length": (
                lambda: self.device.acquisition.frame_length,
                lambda v: setattr(self.device.acquisition, "frame_length", v),
                {"description": "Length, in seconds, of each Acquisition Cycle"}
            ),
            "scope_setup": (
                lambda: self.device.acquisition.setup_scope,
                lambda v: setattr(self.device.acquisition, "setup_scope", v),
                {"description": "Enable to setup Scope Mode when starting the run."}
            ),
            "scope_run": (
                lambda: self.device.acquisition.run_scope,
                lambda v: setattr(self.device.acquisition, "run_scope", v),
                {"description": "Run the NGPD system in Scope Mode"}
            ),
            "run": (
                None,
                self.device.set_run,
                {"description": "Start or stop the acquisition"}
            ),
            "state": {
                "total": (
                    lambda: self.device.acquisition.total,
                    None,
                    {"description": "Total Frames requested for this acquisition"}
                ),
                "current": (
                    lambda: self.device.acquisition.done,
                    None,
                    {"description": "Number of frames currently processed by this acquisition"}
                ),
                "status": (
                    lambda: self.device.acquisition.acq_state,
                    None,
                    {"description": "ITFG Status"}
                )
            },
            "data": (
                self.device.dataHandler.get_data,
                self.device.dataHandler.refresh_data,
                {"description": "Histogram Data from channel 0, encoded as a Base64 string"}
            ),
            "data_shape": (
                lambda: self.device.dataHandler.data_shape,
                None,
                {"description": "Shape of Histogram Data"}
            )
        }

        self.param_tree = ParameterTree({
            "device": device_tree,
            "monitor": monitoring_tree,
            # "channel_config": channel_tree,
            "config": self.device.tree,
            "acq": acquisition_tree
        })

    def initialize(self, adapters):
        self.adapters = adapters
        logging.debug(f"Adapters initialized: {list(adapters.keys())}")
        # Add to param tree if needed post-initialization

    def cleanup(self):
        logging.info("Cleaning up NgpdController")

    def get(self, path, with_metadata=False):
        try:
            return self.param_tree.get(path, with_metadata)
        except (ParameterTreeError, NgpdLibException) as error:
            logging.error(error)
            raise NgpdError(error)

    def set(self, path, data):
        try:
            self.param_tree.set(path, data)
        except (ParameterTreeError, NgpdLibException) as error:
            logging.error(error)
            raise NgpdError(error)

    def set_ip(self, ip: str):
        self.device.ip = ip_address(ip)

    def set_dummy_level(self, dummy: str):
        self.device.dummy_level = DummyLevel[dummy.upper()]
