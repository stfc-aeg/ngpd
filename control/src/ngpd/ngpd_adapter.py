from odin.adapters.adapter import (ApiAdapter, ApiAdapterRequest, ApiAdapterResponse,
                                    request_types, response_types)

from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError
from odin.util import decode_request_body

from pyngpd_cffi import ffi, lib

from functools import partial

from ngpd.ngpd import NGPD, Struct, NGPDDefaults, NGPDException

import logging

import h5py
import numpy as np

# from ngpd.ngpd_adapter import NGPDDefaults

class ngpdAdapter(ApiAdapter): 

    def __init__(self, **kwargs):
        super(ngpdAdapter, self).__init__(**kwargs)

        self.ngpd = NGPD()

        self.adq_num = self.options.get("adq_num", NGPDDefaults.adq_num)
        self.setup_debug  = self.options.get("debug", NGPDDefaults.debug)

        self.adc = {**NGPDDefaults.adc}

        self.path = self.options.get("path", NGPDDefaults.path)
        self.channel = self.options.get("channel", NGPDDefaults.channel)

        self.scope_src = 1
        self.scope_update_flags = False
        self.scope_read_flag = True


        self.filter = {}
        self.filter["type"] = self.options.get("filter_type", NGPDDefaults.filter_type)
        self.filter['type_options'] = self.ngpd.filter_type_options
        self.filter['tsamples'] = self.options.get("filter_tsamples", NGPDDefaults.t_samples)
        self.filter['num_ave'] = self.options.get("filter_num_ave", NGPDDefaults.num_ave)
        self.filter['sigma'] = self.options.get("filter_sigma", NGPDDefaults.sigma)
        self.filter['trap_top'] = self.options.get("filter_trap_top", NGPDDefaults.trap_top)
        self.filter['trap_bot'] = self.options.get("filter_trap_bot", NGPDDefaults.trap_bot)

        self.trigger = Struct("NGPDDiffTrigger", **NGPDDefaults.trigger)
        self.measure = Struct("NGPDMeasure", **NGPDDefaults.measure)
        lib.ngpd_read_measure(self.path, self.channel, self.measure.c_object)
        self.itfg = Struct("NGPDITFGSetup", **NGPDDefaults.itfg)
        self.base_sub = Struct("NGPDBaseSubtract", **NGPDDefaults.base_sub)
        self.scope_options = Struct("NGPDScopeOptions", **NGPDDefaults.scope_options)

        self.data = []




        self.param_tree = ParameterTree({
            "error_message": (lambda: self.ngpd.error_message, None),
            "setup":{
                "path": (lambda: self.path, self.set_path),
                "channel": (lambda: self.channel, self.set_channel),
                "adq_num": (self.adq_num, None),
                "debug": (self.setup_debug, None),
                "setup_adq": (None, self._config_adq),
                "is_setup": (lambda: self.ngpd.setup_flag, None)
            },
            "filter": 
            {
                **self.filter,
               "setup_filter": (None, self._generate_filter),
            },
            "trigger": 
            {
                "settings": self.trigger.get_tree(),
                "setup_trigger": (None, self._setup_diff_trigger),
            },
            "base_sub": 
            {
                "settings": self.base_sub.get_tree(),
                "div_cont_options": self.ngpd.div_cont_options,
                "setup_base_sub": (None, self._setup_base_sub)
            },
            "measure": 
            {
                "settings": self.measure.get_tree(),
                "setup_measure": (None, self._setup_measure)
            },
            "adc": {
                **self.adc,
                "setup_adc": (None, self._setup_adc)
            },
            "scope_options": 
            {
                "settings": self.scope_options.get_tree(),
                "itfg": self.itfg.get_tree(),
                "scope_src": (self.scope_src, self.set_scope_src),
                "setup_scope_options": (None, self._setup_scope_options),
                "setup_scope_streams": (None, self._setup_scope_streams),
                "start_scope": (None, self._start_scope),
                "read": self.scope_read_flag,
                "update_settings": self.scope_update_flags
            },

            "div_cont_options": (self.ngpd.div_cont_options, None),
            "scope_src_options": (self.ngpd.scope_src_options, None),

            "revision": (self.ngpd.get_revision, self.ngpd.api_get_revision),
            # "stream_overflow": (self.ngpd.get_overflow, None)
            "data": {
                "raw_data": (self._get_raw_data, None),
                "num_points": (len(self.data), None),
                "refresh_data": (None, self._set_raw_data),
                "save_data": (None, self._save_raw_data)
            }

        })

    @response_types('application/json', default='application/json')
    def get(self, path, request):
        try:
            response = self.param_tree.get(path)
            content_type = 'application/json'
            status = 200
        except ParameterTreeError as param_error:
            response = {"response: NGPD GET Error: {}".format(param_error)}
            content_type='application/json'
            status = 400
        
        return ApiAdapterResponse(response, content_type=content_type, status_code=status)

    @response_types('application/json', default='application/json')
    def put(self, path, request):
        try:
            data = decode_request_body(request)
            self.param_tree.set(path, data)

            response = self.param_tree.get(path)
            content_type = 'application/json'
            status = 200

        except ParameterTreeError as param_error:
            response = {'response': 'NGPD PUT error: {}'.format(param_error)}
            content_type = 'application/json'
            status = 400

        except NGPDException as ngpd_error:
            response = {'response': 'NGPD API Error: {}'.format(ngpd_error)}
            content_type = 'application/json'
            status = 400

        return ApiAdapterResponse(response, content_type=content_type, status_code=status)


    def set_path(self, value):
        self.path = value

    def set_channel(self, value):
        # check channel is valid value
        self.channel = value

    def set_scope_src(self, value):
        self.scope_src = value

    def _config_adq(self, _):
        self.ngpd.setup(self.adq_num, self.setup_debug)

    def _generate_filter(self, _):
        self.ngpd.generate_filter(self.path, self.channel, self.filter["type"],
                                  tsamples=self.filter["tsamples"],
                                  num_ave=self.filter['num_ave'],
                                  sigma=self.filter['sigma'],
                                  trap_top=self.filter['trap_top'],
                                  trap_bot=self.filter['trap_bot'])

    def _setup_diff_trigger(self, _):
        # trigger = Struct("NGPDDiffTrigger", **self.trigger)
        self.ngpd.setup_diff_trigger(self.path, self.channel, self.trigger)

    def _setup_base_sub(self, _):
        self.ngpd.setup_base_subtraction(self.path, self.channel, self.base_sub)

    def _setup_measure(self, _):
        self.ngpd.setup_measure(self.path, self.channel, self.measure)
    
    def _setup_adc(self, _):
        self.ngpd.set_adc_range_and_offset(self.path, self.adc["channels"],
                                           self.adc["range"], self.adc['offset'])

    def _setup_scope_options(self, _):
        self.ngpd.set_scope_options(path=self.path, card=0, options=self.scope_options)

    def _setup_scope_streams(self, _):
        self.ngpd.set_scope_streams(path=self.path, card=0, stream=0, channel=self.channel, source_type=self.ngpd.scope_src_options[self.scope_src])

    def _start_scope(self, _):
        self.ngpd.start_scope(self.path, self.itfg, self.scope_update_flags, self.scope_read_flag)

    def _get_raw_data(self):
        return self.data[0:100]
    
    def _set_raw_data(self, points):
        logging.debug("Num Points: %d", points )
        self.data = self.ngpd.read_scope_data(self.path, 0, 0, 0, points, 1, 1)

    def _save_raw_data(self, filename):
        logging.debug("Saving Data to File: %s", filename)

        if not filename.endswith(".h5"):
            filename.append(".h5")
        with h5py.File(filename, "w") as f:
            dset = f.create_dataset("Data", (len(self.data),), data=self.data)
        # try:

        #     with open(filename, "xb") as f:
        #         [f.write(x.to_bytes(2, 'big')) for x in self.data]
        # except FileExistsError as err:
        #     logging.error("File %s already exists!", filename)
        #     return False
        # except (ValueError, OverflowError) as err:
        #     logging.error("Error writing to file: %s", err)
        #     os.remove(filename)