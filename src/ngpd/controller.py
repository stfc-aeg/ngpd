import logging
from odin_control.adapters.parameter_tree import ParameterTree, ParameterTreeError
from odin_control.adapters.base_controller import BaseController, BaseError


class NgpdError(BaseError):
    """Simple exception class to wrap lower-level exceptions."""


class NgpdController(BaseController):
    """Controller class for Ngpd."""

    def __init__(self, options):
        self.options = options
        self.example_param = "Example"
        self.param_tree = ParameterTree({
            "example_param": (
                lambda: self.example_param,
                lambda v: setattr(self, 'example_param', v),
                {"description": "Example Parameter for Template"})
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
        except ParameterTreeError as error:
            logging.error(error)
            raise NgpdError(error)

    def set(self, path, data):
        try:
            self.param_tree.set(path, data)
        except ParameterTreeError as error:
            logging.error(error)
            raise NgpdError(error)