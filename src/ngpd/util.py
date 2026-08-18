import logging
from functools import wraps


class NgpdLibException(Exception):
    """"""


def UsesNgpdLibrary(level=logging.DEBUG):
    def decorator(func):
        @wraps(func)
        def _wrapper(self, *args, **kwargs):
            if self.ngpd is None:
                raise NgpdLibException(f"{func.__name__}: NGPD Not Configured")
            return func(self, *args, **kwargs)
        return _wrapper
    return decorator
