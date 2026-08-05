from odin_control.adapters.adapter import ApiAdapter
from .controller import NgpdController, NgpdError
from ._version import __version__


class NgpdAdapter(ApiAdapter):
    """Ngpd Adapter class inheriting base adapter functionality."""

    controller_cls = NgpdController
    error_cls = NgpdError
    version = __version__
