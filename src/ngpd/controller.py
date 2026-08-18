import logging
from ipaddress import ip_address
from functools import partial
from odin_control.adapters.parameter_tree import ParameterTree, ParameterTreeError
from odin_control.adapters.base_controller import BaseController, BaseError
from ngpd.pyngpd import PyNgpd, DummyLevel
from ngpd.util import UsesNgpdLibrary, NgpdLibException
from ngpd.device import NgpdDevice


class NgpdError(BaseError):
    """Simple exception class to wrap lower-level exceptions."""


class NgpdController(BaseController):
    """Controller class for Ngpd."""

    def __init__(self, options: dict[str, str]):
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
                "temps": (self.read_adc_temps, None)
            },
            "preamp": {
                "temps": (self.read_preamp_temps, None)
            }
        }

        self.param_tree = ParameterTree({
            "device": device_tree,
            "monitor": monitoring_tree,
            # "channel_config": channel_tree,
            "config": self.device.tree
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

    def read_adc_temps(self):
        try:
            return self.device.read_adc_temps()
        except NgpdLibException:
            return self.device.adc_temps

    def read_preamp_temps(self):
        try:
            return self.device.read_preamp_temps()
        except NgpdLibException:
            return self.device.preamp_temps
