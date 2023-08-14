from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError
from pyngpd_cffi import ffi, lib

from functools import partial

import numpy as np

import threading
import logging

class NGPD():

    def __init__(self, **kwargs) -> None:
        self.error_message = ""
        self.error_flag = False

        self.setup_flag = False

        self.adq_num = NGPDDefaults.adq_num
        self.debug = False

        self.path = 0
        self.channel = 0

        self.filter_type = "exp"

        self.div_cont_options = ["div-64", "div-128", "div-256", "div-512",
                                 "div-1k", "div-2k", "div-4k", "div-8k"]
        self.scope_src_options = ["test-pat", "inp", "filter", "dtrig-diff", "dtrig-out", "bsub-out", "bsub-int", "meas-data"] #TODO: more options are available
        self.filter_type_options = ["exp", "ave", "gaussian"]
        
        self.revision = 0
        self.mod = None

        self.hist_mods = {}

        # one second of data contains 500 million points
        self.data = np.zeros(500000000, dtype=np.uint16)  # ha ha this is SO MUCH DATA

    def setup(self, adq_num, debug):
        
        if lib.ngpd_config_adq14(adq_num, debug) < 0:
            self.get_error_message()
            self.setup_flag = False
        else:
            self.setup_flag = True
            self.mod = ScopeMod(self.path)
            print(self.mod.mod)
            print(self.mod.get_num_t())

        


    def generate_filter(self, path=0, chan=0, type="exp", **kwargs):
        val = -1
        if type == "exp":
            val = lib.ngpd_filter_load_exp(path, chan, kwargs['tsamples'])
        elif type == "ave":
            val = lib.npgd_filter_load_rect(path, chan, kwargs['num_ave'])
        elif type == "gaussian":
            val = lib.ngpd_filter_load_gaus(path, chan, kwargs['sigma'])
        elif type == "trapezoid":
            val = lib.ngpd_filter_load_trapezoid(path, chan, kwargs['trap_top'], kwargs['trap_bot'])
        else:
            pass # type not specified, woops
            
        if val < 0:
            self.get_error_message()

    def setup_diff_trigger(self, path, chan, trigger):
        
        num_chan = lib.ngpd_get_num_chan(path)
        #TODO: sanity check num_chan
        #TODO: check chan is not more than num_chan

        val = lib.ngpd_write_diff_trigger(path, chan, trigger.c_object)

        if val < 0:
            self.get_error_message()

    def setup_base_subtraction(self, path, chan, base_sub):
        val = lib.ngpd_write_baseline_subtract(path, chan, base_sub.c_object)

        if val < 0:
            self.get_error_message()

    def setup_measure(self, path, chan, measure):
        if lib.ngpd_write_measure(path, chan, measure.c_object) < 0:
            self.get_error_message()

    def setup_neutron_discrimination(self, path, chan, measure):
        # we've combined the measures from both setups into one, so this should work fine?
        self.setup_measure(path, chan, measure)  

    def setup_histogram(self, enable_dict, path=0, num_bins_height=1024, num_bins_tailsum=1024, num_bins_falltime=256, num_bins_tailratio=1024,
                        max_ratio=0.5, ):
        measure = Struct("NGPDMeasure")
        enable = 0
        suffix = "ngp" if enable_dict['separate_ngp'] else "all"
        if enable_dict["height"]:            enable = enable | lib.NGPDHistEnbHeight
        if enable_dict["tail_sum"]:          enable = enable | lib.NGPDHistEnbTailSum
        if enable_dict["fall_time"]:         enable = enable | lib.NGPDHistEnbFallTime
        if enable_dict["tail_ratio"]:        enable = enable | lib.NGPDHistEnbTailRatio
        if enable_dict["height_tail_sum"]:   enable = enable | lib.NGPDHistEnbHgtTailSum
        if enable_dict["height_fall_time"]:  enable = enable | lib.NGPDHistEnbHgtFallTime
        if enable_dict["height_tail_ratio"]: enable = enable | lib.NGPDHistEnbHgtTailRatio
        if enable_dict["separate_ngp"]:      enable = enable | lib.NGPDHistSeparateNeutrons
        if enable_dict["discard_pileup"]:    enable = enable | lib.NGPDHistDiscardPileup

        if lib.ngpd_read_measure(path, 0, measure.c_object) < 0:
            self.get_error_message()

        max_height = 65535
        max_fall_time = 255

        max_tail_sum = int(65536/5*measure.get("tail_sum_num"))
        ratio_scale = num_bins_tailratio/(max_ratio*measure.get("tail_sum_num"))

        rc = lib.ngpd_hist_setup(path, max_height, max_tail_sum, max_fall_time, ratio_scale, num_bins_height, num_bins_tailsum, num_bins_falltime, num_bins_tailratio, enable)

        if rc < 0:
            self.get_error_message()
            return

        if enable_dict["height"]:
            mod_name = "ngpd1_hist_height{}_{}".format(num_bins_height, suffix)
            self.hist_mods["height"] = ImageMod(mod_name)
        if enable_dict["tail_sum"]:
            mod_name = "ngpd1_hist_tail_sum{}_{}".format(num_bins_tailsum, suffix)
            self.hist_mods["tail_sum"] = ImageMod(mod_name)
        if enable_dict["fall_time"]:
            mod_name = "ngpd1_hist_fall_time{}_{}".format(num_bins_falltime, suffix)
            self.hist_mods["fall_time"] = ImageMod(mod_name)
        if enable_dict["tail_ratio"]:
            mod_name = "ngpd1_hist_tail_ratio{}_{}".format(num_bins_tailratio, suffix)
            self.hist_mods["tail_ratio"] = ImageMod(mod_name)
        if enable_dict["height_tail_sum"]:
            mod_name = "ngpd1_hist_heightXtailsum{}x{}_{}".format(num_bins_height, num_bins_tailsum, suffix)
            self.hist_mods["height_tail_sum"] = ImageMod(mod_name)
        if enable_dict["height_fall_time"]:
            mod_name = "ngpd1_hist_heightXfalltime{}x{}_{}".format(num_bins_height, num_bins_falltime, suffix)
            self.hist_mods["height_fall_time"] = ImageMod(mod_name)
        if enable_dict["height_tail_ratio"]:
            mod_name = "ngpd1_hist_heightXtailratio{}x{}_{}".format(num_bins_height, num_bins_tailratio, suffix)
            self.hist_mods["height_tail_ratio"] = ImageMod(mod_name)


    def set_adc_range_and_offset(self, path, channels, adc_range, offset):
        if type(channels) is not list:
            channels = [channels]
        logging.debug("Range Desired: %f", adc_range)
        for channel in channels:
            range_actual = lib.ngpd_adc_set_range(path, channel, adc_range)
            if range_actual < 0: self.get_error_message()
            else:
                logging.debug("Range Actual: %f", range_actual)
            if lib.ngpd_adc_set_offset(path, channel, offset) < 0: self.get_error_message()

    def set_adc_input_range(self, path, first_chan, num_chan, adc_range):
        #TODO: sanity check range, ensure its not more than number of channels, etc
        for chan in range(first_chan, first_chan+num_chan):
            if lib.ngpd_adc_set_range(path, chan, adc_range) < 0:
                self.get_error_message()

    def set_adc_offset(self, path, first_chan, num_chan, offset):

        for chan in range(first_chan, first_chan+num_chan):
            if lib.ngpd_adc_set_offset(path, chan, offset) < 0:
                self.get_error_message()

    def set_scope_options(self, path, card, **kwargs):

        options = Struct("NGPDScopeOptions")
        options.set("flags", 0)
        options.set("nstreams", kwargs.get("set_streams", 0))
        if lib.ngpd_set_scope_options(path, card, options.c_object) < 0:
            self.get_error_message()

        return self.resize_read_handle(path)
    
    def set_scope_streams(self, path, card, stream, channel, source_type="inp"):

        generation = 0
        alt = 0
        # module = Struct("NGPDScopeModule")
        src_list = ["test-pat", "inp", "filter", "dtrig-diff", "dtrig-out", "bsub-out", "bsub-int", "meas-data"] #TODO: more options are available
        if source_type in src_list:
            src = src_list.index(source_type)
        if lib.ngpd_scope_setup_stream(path, card, stream, channel, src, alt) < 0:
            self.get_error_message()
        
    def resize_read_handle(self, path):

        module = Struct("NGPDScopeModule")
        # print(module)
        module.c_object = lib.ngpd_scope_get_mod(path)

        # print(module)

    def set_dae_pulse(self, path, channel, stretch):
        if lib.ngpd_write_dae_pulse(path, channel, stretch) < 0:
            self.get_error_message()
            return -1

    def read_scope_data(self, path, sx, sy, st, dx, dy, dt):
        i = 0
        for t in range(st, dt):

            for y in range(sy, dy):
                inc = lib.ngpd_scope_mod_get_inc(self.mod.get_object(), y)
                ptr = lib.ngpd_scope_mod_get_ptr(self.mod.get_object(), t, y)

                ptr += inc * sx

                logging.debug("Increment: %d", inc)
                # logging.debug("Pointer: %x", ptr)
                self.data = np.frombuffer(ffi.buffer(ptr, size=(dx*2)), dtype=np.uint16)


                # for i in range(0, dx):
                #     # get from buffer?
                #     self.data[i] = ptr[0]
                #     ptr += inc
                #     i = i + 1
        return

    def start_scope(self, path, itfg, update_settings, read):
        
        # itfg_setup = Struct("NGPDITFGSetup")
        scope_status = ffi.new("uint32_t *")

        # itfg_setup.set("col_time", col_time)
        # itfg_setup.set("trig_mode", 1 if "ext-trig-cycles" in args else 0)

        # itfg_setup.set("cycles", kwargs.get("num_cycles", 1))

        card = -1

        save_flags = lib.ngpd_get_run_flags(path)
        run_flags = save_flags
        # print("Save Flags: {0:b}".format(save_flags))

        # BITWISE operators for run flags
        if update_settings:
            run_flags |= lib.NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS
            run_flags &= ~lib.NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD
        else:
            run_flags |= lib.NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD
            run_flags &= ~lib.NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS

        if read:
            run_flags |= lib.NGPD_RUN_FLAGS_SCOPEMODE

        # print("Run Flags:  {0:b}".format(run_flags))

        lib.ngpd_set_run_flags(path, run_flags)

        if lib.ngpd_dma_system_start(path, card, 0, 0, itfg.c_object) < 0:
            self.get_error_message()
            return -1

        if read:
            val = lib.ngpd_dma_wait_scope(path, card, scope_status)
            if val < 0:
                self.get_error_message()
            if scope_status[0] != 0:
                self.get_error_message()

            if lib.ngpd_dma_read_scope(path, card, 0, 0, 0) < 0:
                self.get_error_message()
        
        lib.ngpd_set_run_flags(path, save_flags)

        # self.read_scope_data()



    def get_error_message(self):
        self.error_message = str(ffi.string(lib.ngpd_get_error_message()))
        self.error_flag = True
        raise NGPDException(self.error_message)
        # print(self.error_message)
    
    def reset_error(self):
        self.error_flag = False
        self.error_message = ""

    def get_revision(self):
        return self.revision
    
    def api_get_revision(self, _):
        self.revision = lib.ADQAPI_GetRevision()

    # def get_overflow(self):
    #     return lib.py_GetStreamOverflow()


class NGPDException(Exception):
    pass

class Struct():
    """
    Python class to allow the pythonic creation of C Structs for use in the NGPD library
    """
    def __init__(self, struct_name, **kwargs):
        
        self.struct_name = struct_name
        options = {}
        for arg in kwargs:
            options[arg] = kwargs[arg]

        self.c_object = ffi.new(struct_name + " *", options)

    def put(self, value):
        # print(value)
        for key in value:
            self.set(key, value[key])

    def set(self, name, value):
        # print("Setting {} as {}".format(name, value))
        setattr(self.c_object, name, value)

    def get(self, name):
        return getattr(self.c_object, name)

    def get_array(self, name):
        array = getattr(self.c_object, name)
        return list(array)

    def get_array_shared_value(self, struct, name):
        array = getattr(struct, name)
        if len(set(array)) > 1:
            return None
        else:
            return array[0]

    def set_array(self, name, value):
        array = getattr(self.c_object, name)
        if len(value) == len(array):
            setattr(self.c_object, name, value)

    def set_all_array(self, name, value):
        array = getattr(self.c_object, name)
        setattr(self.c_object, name, [value] * len(array))

    def __repr__(self) -> str:
        return self.get_struct_contents_string(self.c_object)

    def get_tree(self):
        return self._get_values_dict(self.c_object)
    

    def _get_values_dict(self, struct):
        return_val = {}
        type = ffi.typeof(struct)
        if type.kind == "pointer":
            type = type.item
        for field, fieldtype in type.fields:
            if fieldtype.type.kind == "struct":
                return_val[field] = self._get_values_dict(getattr(struct, field))
            elif fieldtype.type.kind == "array":
                return_val[field] = {
                    "all": (partial(self.get_array_shared_value, struct, field), partial(self.set_all_array, field)),
                    # "individual": (partial(self.get_array, field), partial(self.set_array, field))
                }
            else:
                return_val[field] = (partial(getattr, struct, field), partial(self.set, field))
        return ParameterTree(return_val)

    def get_struct_contents_string(self, struct):
        type = ffi.typeof(struct)
        return_string = self.struct_name + "\n"
        if type.kind == "pointer":
            type = type.item
        for field, fieldtype in type.fields:
            if fieldtype.type.kind == "struct":
                return_string += self.get_struct_contents_string(getattr(struct, field))
            else:
                return_string += "{}: {} ({}), ".format(field, getattr(struct, field), fieldtype.type.cname)
        return return_string

class ScopeMod():

    def __init__(self, path):
        self.mod = Struct("NGPDScopeModule")
        self.mod.c_object = lib.ngpd_scope_get_mod(path)
        self.tree = self.mod.get_tree()
    
    def get_num_t(self):
        return self.tree.get("head/num_t")['num_t']
    
    def get_object(self):
        return self.mod.c_object

class ImageMod():

    def __init__(self, name):
        byte_name = ffi.new("char[]", name.encode())
        point_name = ffi.new("char **", byte_name)

        self.mod = ffi.new("void **")
        self.mod_head = ffi.new("void **")

        type_lang = ffi.new("uint16_t *")
        attr_rev = ffi.new("uint16_t *")
        rc = lib._os_link(point_name, self.mod_head, self.mod, type_lang, attr_rev)
        if rc < 0:
            logging.error("Linking to module %s failed", name)
            return
        self.num_x = lib.id_get_num_x(self.mod[0])
        self.num_y = lib.id_get_num_y(self.mod[0])
        self.num_t = lib.id_get_num_t(self.mod[0])
        self.dtype = lib.id_get_data_type(self.mod[0])

        if self.dtype == lib.DATA_SHORT:
            self.dtype = np.int16
            self.dtype_text = "int16_t"
        elif self.dtype == lib.DATA_FLOAT:
            self.dtype = np.float32
            self.dtype_text = "float"
        elif self.dtype == lib.DATA_DOUBLE:
            self.dtype = np.float64
            self.dtype_text = "double"
        else:  # self.dtype == lib.DATA_LONG:
            self.dtype = np.int32
            self.dtype_text = "int32_t"

        logging.debug("Linked to Module %s", name)
        logging.debug("Data Size: (%d, %d, %d)", self.num_x, self.num_y, self.num_t)

        self.data = np.zeros((self.num_t, self.num_y, self.num_x), dtype=self.dtype)

        # ptr = lib.id_get_ptr(self.mod[0], 0, 0, 0)
    
    def update_data(self):
        ptr = lib.id_get_ptr(self.mod[0], 0, 0, 0)
        self.data = np.frombuffer(ffi.buffer(ptr, self.num_x*self.num_y*self.num_t*ffi.sizeof(self.dtype_text)), dtype=self.dtype)
        self.data.shape = (self.num_x, self.num_y, self.num_t)


class NGPDDefaults():
    # defaults based on the setup scripts from da.server
    adq_num = 1
    debug = True
    
    path = 0
    channel = 0
    
    filter_type = "exp"
    t_samples = 1.5
    num_ave = 0
    sigma = 1
    trap_top = 0
    trap_bot = 0
    
    trigger = {
        "thres": 4000,
        "sep": 6,
        "data_delay": 25,
        "trig_delay": 14,
        "width_a": 1,
        "width_b": 91,
        "delay_a": 0,
        "delay_b": 0
    }
    
    base_sub = {
        "use_fixed": False,
        "fixed": 0,
        "error_limit": 0,
        "div_cont": 4 # div-1k
    }
    base_sub_type = "div-1k"

    measure = {
        "tail_sum_delay": 35,
        "tail_sum_num": 100,
        "fall_time_frac": 0.1,
        "min_height": 475*64,
        "max_height": 710*64,
        "tail_thres_m": [int(1.4*0x800000)] * 256,  # multiplication as required in d.a scripts
        "tail_thres_c": [60*90] * 256,
        "ignore_fall_time": True
    } 
    adc = {
        "range": 114,
        "offset": 32000,
        "channels": [0]
    }
    itfg = {
        "col_time": -1,
        "trig_mode": 0,
        "cycles": 1
    }

    scope_options = {
        "flags": 0,
        "nstreams": 1
    }