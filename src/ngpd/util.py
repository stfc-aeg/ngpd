"""NGPD Utility Classes and Functions."""
from functools import wraps


class NgpdLibException(Exception):
    """Basic Exception Class for handling errors from the CFFI Library."""


def UsesNgpdLibrary(func):
    """Check that the NGPD class object exists before attempting to run a function that makes use of it."""
    @wraps(func)
    def _wrapper(self, *args, **kwargs):
        if self.ngpd is None:
            raise NgpdLibException(f"{func.__name__}: NGPD Not Configured")
        return func(self, *args, **kwargs)
    return _wrapper
