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

    def __repr__(self) -> str:
        type = ffi.typeof(self.c_object)
        return_string = self.struct_name + "\n"
        if type.kind == "pointer":
            type = type.item
        for field, fieldtype in type.fields:
            return_string += "{}: {} ({}), ".format(field, getattr(self.c_object, field), fieldtype.type.cname)
        return return_string

class NGPD():

    def __init__(self, **kwargs) -> None:
       self.error_message = ""

    def setup(self, **kwargs):
        
        adq_num = kwargs.get("adq_num", 1)
        debug = kwargs.get("debug", False)
        value = lib.ngpd_config_adq14(adq_num, debug)

        if value < 0:
            # an error has occured
            self.get_error_message()
        return value

    def get_error_message(self):
        self.error_message = ffi.string(lib.ngpd_get_error_message())

# val = lib.ngpd_dummy(123)
# print(val)

api = NGPD()
api.setup(adq_num=0, debug=True)
print(api.error_message)

# val = lib.ngpd_config_adq14(0, 1)
# print(val)

# val = lib.ngpd_get_num_chan(0)
# print(val)


# trigger = DiffTrigger(4000, 6, 25, 14, 0, 1, 0, 91)
trigger = Struct("NGPDDiffTrigger", thres=4000, sep=6, data_delay=25, trig_delay=14,
                                    width_a=1, width_b=91)
print(trigger)
# print(trigger.trigger)
val = lib.ngpd_write_diff_trigger(0, 0, trigger.c_object)

measure = Struct(struct_name="NGPDMeasure", tail_sum_delay=20, tail_sum_num=55)
print(measure)
print(measure.c_object)

