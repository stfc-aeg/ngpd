# from  pyngpd import ffi, lib
# path= lib.ngpd_config_ngzmp(1, ffi.NULL, -1, ffi.NULL, -1, 2, 0, 1, 255)

from pyngpd_cffi import ffi, lib
import logging

class Struct():
    """
    Python class to allow the pythonic creation of C Structs for use in the NGPD library
    """
    def __init__(self, struct_name, **kwargs):
        
        self.struct_name = struct_name
        self.options = {}
        for arg in kwargs:
            self.options[arg] = kwargs[arg]

        self.c_object = ffi.new(struct_name + " *", self.options)

    def set(self, name, value):
        # self.c_object.attr = value
        print("Setting {} as {}".format(name, value))
        setattr(self.c_object, name, value)

    def __repr__(self) -> str:
        return self.get_struct_contents_string(self.c_object)
    

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


class NGPD():

    def __init__(self, **kwargs) -> None:
       self.error_message = ""
       self.error_flag = False

    def setup(self, **kwargs):
        
        adq_num = kwargs.get("adq_num", 1)
        debug = kwargs.get("debug", False)
        if lib.ngpd_config_adq14(adq_num, debug) < 0: self.get_error_message()

        #TODO: get values for these steps from kwargs
        self.generate_filter(tsamples=1.5)
        
        diff_trigger = Struct("NGPDDiffTrigger", thres=4000, sep=6, data_delay=25, trig_delay=14,
                                    width_a=1, width_b=91)
        self.setup_diff_trigger(0, 0, diff_trigger)
        self.setup_base_subtraction(0, 0, type="div-1k")
        self.setup_measure(0, 0, 35, 100, 0.1)

        self.setup_neutron_discrimination(0, 0, 485*64, 775*64, "ignore_fall", tail_thres_m=1.6, tail_thres_c=90*90)

        self.set_adc_input_range(0, 0, 1, 114)
        self.set_adc_offset(0, 0, 1, 32000)


        self.set_scope_options(0, 0, set_streams=1)
        self.set_scope_streams(0, 0, 0, 0, source_type="inp")


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

    def setup_base_subtraction(self, path, chan, type="div-1k", **kwargs):
        base_sub = Struct("NGPDBaseSubtract")

        if "error_limit" in kwargs:
            base_sub.set("error_limit", kwargs['error_limit'])

        if type == "fixed":
            base_sub.set("use_fixed", 1)
            base_sub.set("fixed", kwargs['fixed'])
        else:
            types = ["div-64" ,"div-128","div-256","div-512","div-1k","div-2k","div-4k","div-8k"]
            base_sub.set("div_cont", types.index(type))
        #TODO: sanity check that the type given is one of the options
        val = lib.ngpd_write_baseline_subtract(path, chan, base_sub.c_object)

        if val < 0:
            self.get_error_message()

    def setup_measure(self, path, chan, tail_delay, tail_sum, fall_frac, *args):
        measure = Struct("NGPDMeasure")

        if lib.ngpd_read_measure(path, chan, measure.c_object) < 0:
            self.get_error_message()
        
        measure.set("tail_sum_delay", tail_delay)
        measure.set("tail_sum_num", tail_sum)
        measure.set("fall_time_frac", fall_frac)
        measure.set("enable_tail_subtract", "tail-subtract" in args)
        measure.set("tail_subtract_test", "tail-sub-test" in args)
        measure.set("tail_subtract_test_neutron", "test-neutron" in args)
        measure.set("min_tail_count", 1)

        if lib.ngpd_write_measure(path, chan, measure.c_object) < 0:
            self.get_error_message()

    def setup_neutron_discrimination(self, path, chan, min_height, max_height, *args, **kwargs):
        measure = Struct("NGPDMeasure")

        if lib.ngpd_read_measure(path, chan, measure.c_object) < 0:
            self.get_error_message()

        min_fall_time = kwargs.get("min-fall", 0)
        max_fall_time = kwargs.get("max-fall", 0)
        min_tail_count = kwargs.get("min_tail_count", 0) if "adaptive" in args else 0

        measure.set("adaptive_tail_sum", "adaptive" in args)
        measure.set("ignore_fall_time","ignore_fall" in args)
        measure.set("ignore_tail_sum", "ignore_tail_sum" in args)
        measure.set("min_height", min_height)
        measure.set("max_height", max_height)
        measure.set("min_fall_time", min_fall_time)
        measure.set("max_fall_time", max_fall_time)
        measure.set("min_tail_count", min_tail_count)

        if "tail_thres_c" in kwargs:
            measure.set("tail_thres_c", [kwargs.get("tail_thres_c")] * 256)

        if "tail_thres_m" in kwargs:
            # multiply and round to an int, as in the original scripts
            m = int(kwargs['tail_thres_m']*0x800000)
            measure.set("tail_thres_m", [m] * 256)

        if lib.ngpd_write_measure(path, chan, measure.c_object) < 0:
            self.get_error_message()

    def setup_histogram(self):
        pass  # not done yet, probs not important for initial setup testing

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

        # return self.resize_read_handle(path)
    
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
        print(module)
        module.c_object = lib.ngpd_scope_get_mod(path)

        print(module)


    def read_scope_data(self, path, sx, sy, st, dx, dy, dt):
        
        module = Struct("NGPDScopeModule")
        module.c_object = lib.ngpd_scope_get_mod(path)
        data = []
        # data_point = lib.ngpd_scope_mod_get_ptr(module.c_object, 0, 0)
        # inc = lib.ngpd_scope_mod_get_inc(module.c_object, 0)

        for t in range(st, dt):

            for y in range(sy, dy):
                inc = lib.ngpd_scope_mod_get_inc(module.c_object, y)
                ptr = lib.ngpd_scope_mod_get_ptr(module.c_object, t, y)

                ptr += inc*sx

                for i in range(0, dx):
                    data.append(ptr[0])
                    ptr += inc
                return data

    def start_scope(self, path, col_time, *args, **kwargs):
        
        itfg_setup = Struct("NGPDITFGSetup")
        scope_status = ffi.new("uint32_t *")

        itfg_setup.set("col_time", col_time)
        itfg_setup.set("trig_mode", 1 if "ext-trig-cycles" in args else 0)

        itfg_setup.set("cycles", kwargs.get("num_cycles", 1))

        card = kwargs.get("card", -1)

        save_flags = lib.ngpd_get_run_flags(path)
        run_flags = save_flags
        print("Save Flags: {0:b}".format(save_flags))

        # BITWISE operators for run flags
        if "update_settings" in args:
            run_flags |= lib.NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS
            run_flags &= ~lib.NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD
        else:
            run_flags |= lib.NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD
            run_flags &= ~lib.NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS

        if "read" in args:
            run_flags |= lib.NGPD_RUN_FLAGS_SCOPEMODE

        print("Run Flags:  {0:b}".format(run_flags))

        lib.ngpd_set_run_flags(path, run_flags)

        if lib.ngpd_dma_system_start(path, card, 0, 0, itfg_setup.c_object) < 0:
            self.get_error_message()
            return -1

        if "read" in args or "wait_scope" in args:
            val = lib.ngpd_dma_wait_scope(path, card, scope_status)
            if val < 0:
                self.get_error_message()
                return -1
            if scope_status[0] != 0:
                print("Error with Scope Status: {:b}".format(scope_status[0]))
                return -1
        
        if "read" in args:
            if lib.ngpd_dma_read_scope(path, card, 0, 0, 0) < 0:
                self.get_error_message()
        
        lib.ngpd_set_run_flags(path, save_flags)



    def get_error_message(self):
        self.error_message = ffi.string(lib.ngpd_get_error_message())
        self.error_flag = True
        print(self.error_message)
    
    def reset_error(self):
        self.error_flag = False
        self.error_message = ""

# val = lib.ngpd_dummy(123)
# print(val)
# print(lib.ngpd_get_measure_tail_thres())
api = NGPD()
api.setup(adq_num=1, debug=True)
#dx = 535822336
print(api.read_scope_data(0, 0, 0, 0, 100, 1, 1))
api.start_scope(0, -1.0, "read")
print(api.read_scope_data(0, 0, 0, 0, 100, 1, 1))

print(api.error_message)


