"""Exception Module."""
from odin_control.adapters.base_controller import BaseError

class NgpdError(BaseError):
    """Simple exception class to wrap lower-level exceptions."""