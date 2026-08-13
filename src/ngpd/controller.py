import logging
from ipaddress import ip_address
from functools import partial
from odin_control.adapters.parameter_tree import ParameterTree, ParameterTreeError
from odin_control.adapters.base_controller import BaseController, BaseError
from ngpd.pyngpd import PyNgpd, DummyLevel, NGPDLibError


class NgpdError(BaseError):
    """Simple exception class to wrap lower-level exceptions."""


class NgpdController(BaseController):
    """Controller class for Ngpd."""

    def __init__(self, options: dict[str, str]):
        self.options = options

        self.ngpd: PyNgpd = None

        self.board_ip = ip_address(self.options.get("ip_addr", "192.168.0.1"))
        self.num_cards = int(self.options.get("num_cards", "1"))
        self.dummy_level = DummyLevel[self.options.get("dummy_level", "none").upper()]

        device_tree = {
            "base_ip": (
                lambda: str(self.board_ip),
                self.set_base_ip,
                {
                    "description": ("IP Address of the NGPD Board. "
                                    "In a multi-board system this is the first board")
                }
            ),
            "num_cards": (
                lambda: self.num_cards,
                partial(setattr, self, "num_cards"),
                {"description": "Number of NGPD cards in the whole system",
                 "min": 1, "max": 8}
            ),
            "dummy_system": (
                lambda: self.dummy_level.name.lower(),
                self.set_dummy_level,
                {"description": "Set what parts of the system are simulated by the software",
                 "allowed_values": [level.name.lower() for level in DummyLevel]}
            ),
            "connect": (
                lambda: self.ngpd is not None, lambda _: self.config_ngpd(),
                {"description": "Configure the NGPD card(s). "}
            )
        }

        self.param_tree = ParameterTree({
            "device": device_tree

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
        except (ParameterTreeError, NGPDLibError) as error:
            logging.error(error)
            raise NgpdError(error)

    def set(self, path, data):
        try:
            self.param_tree.set(path, data)
        except (ParameterTreeError, NGPDLibError) as error:
            logging.error(error)
            raise NgpdError(error)

    def config_ngpd(self):
        # we have to subtract 1 from the IP Addr due to strange logic in William's
        # code, which adds one back to the ip when initialising
        self.ngpd = PyNgpd(str(self.board_ip - 1), self.num_cards, self.dummy_level)

    def set_base_ip(self, ip: str):
        self.board_ip = ip_address(ip)

    def set_dummy_level(self, dummy: str):
        self.dummy_level = DummyLevel[dummy.upper()]

