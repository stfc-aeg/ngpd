from odin.adapters.adapter import (ApiAdapter, ApiAdapterRequest, ApiAdapterResponse,
                                    request_types, response_types)
from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError
from odin.util import decode_request_body

from pyngpd_cffi import ffi, lib

class ngpdAdapter(ApiAdapter): 

    def __init__(self, **kwargs):
        super(ngpdAdapter, self).__init__(**kwargs)

        self.param_tree = ParameterTree({
            
        })