from ngpd.pyngpd import PyNgpd, DummyLevel, FilterType
from ngpd.pyngpd import PyNGPDFilter, PyNGPDDiffTrigger, PyNGPDbassub, PyNGPDTailMeasure
from ngpd.pyngpd import ANALOG_MAX_GAIN, ANALOG_MAX_OFFSET
from ngpd.pyngpd import BSUB_MAX_ERROR, BSUB_MAX_FIXED, BSUB_MIN_FIXED
from ngpd.pyngpd import MIN_DIV_CONT, NUM_DIV_CONT
from ngpd.pyngpd import (MEASURE_MAX_DELAY, MEASURE_MAX_FALL_TIME, MEASURE_MAX_HEIGHT,
                         MEASURE_MAX_SUM_NUM, MEASURE_MAX_TAIL_COUNT)
from ngpd.pyngpd import (TRIG_MIN_SEP, TRIG_MAX_SEP, TRIG_MAX_DATA_DELAY, TRIG_MAX_DELAY_AB,
                         TRIG_MAX_THRES, TRIG_MAX_TRIG_DELAY, TRIG_MAX_TRIG_STRETCH,
                         TRIG_MIN_DATA_DELAY, TRIG_MIN_DELAY_AB, TRIG_MIN_THRES,
                         TRIG_MIN_TRIG_DELAY, TRIG_MIN_TRIG_STRETCH)
from ngpd.util import UsesNgpdLibrary, NgpdLibException
from ipaddress import ip_address
from typing import Literal
from math import log2
import logging

TailMeasureSetting = Literal["tail_sum_delay", "tail_sum_sample", "fall_time_frac",
                             "enable_tail_subtract", "enable_subtract_test",
                             "enable_subtract_neutron",
                             "ignore_tail_sum", "ignore_fall_time", "adaptive_tail_sum",
                             "min_height", "max_height", "min_fall", "max_fall", "min_count",
                             "tail_thres_c", "tail_thres_m"]

FilterSetting = Literal["type", "arg1", "arg2", "darg"]

TriggerSetting = Literal["thres", "sep", "data_delay", "trig_delay",
                         "delay_a", "delay_b", "width_a", "width_b"]

BaseSubSetting = Literal["use_fixed", "fixed", "error_limit", "div_cont"]


class NgpdDevice:
    """
    NGPD Handling class, providing a bridge between Adapter/Controller
    and the interface class of PyNgpd
    """

    DIV_CONT_ALLOWED = [MIN_DIV_CONT * (2 ** i) for i in range(NUM_DIV_CONT)]

    def __init__(self, options: dict[str, str]):
        self.ip = ip_address(options.get("ip_addr", "192.168.0.1"))
        self.num_cards = int(options.get("num_cards", 1))
        self.dummy_level = DummyLevel[options.get("dummy_level", "none").upper()]

        self.ngpd: PyNgpd = None

        self._adc_temps = [0] * 4
        self._preamp_temps = [0] * 2

        self._analog_gain = [0] * 8
        self._analog_offset = [0] * 8

        self.channels = [NgpdChannel(i) for i in range(8)]

        self.tree = {
            "analog": {
                f"channel_{i}": {
                    "gain": (
                        lambda i=i: self.channels[i].analog_gain,
                        lambda v, i=i: self.channels[i].set_analog_gain(v),
                        {"description": "ADC Analog Gain",
                         "min": 0, "max": ANALOG_MAX_GAIN}
                    ),
                    "offset": (
                        lambda i=i: self.channels[i].analog_offset,
                        lambda v, i=i: self.channels[i].set_analog_offset(v),
                        {"description": "Preamp Analog Offset",
                         "min": 0, "max": ANALOG_MAX_OFFSET}
                    )
                }
                for i in range(len(self.channels))
            },
            # lambdas in dict comprehension have scoping issues, so the index
            # is getting set as a default argument (eg: lambda i=i) to bind it per channel.
            # Without this, all of the channels would affect only the last (channel_7)
            # this is also why were using the list index rather than iterating over the list itself
            "base_sub": {
                f"channel_{i}": {
                    "use_fixed": (
                        lambda i=i: self.channels[i].base_sub.use_fixed,
                        lambda v, i=i: self.channels[i].set_base_sub("use_fixed", v),
                        {"description": "Toggle the use of a fixed Base Subtraction Value"}
                    ),
                    "fixed": (
                        lambda i=i: self.channels[i].base_sub.fixed,
                        lambda v, i=i: self.channels[i].set_base_sub("fixed", v),
                        {"description": "Fixed Base Subtraction Value",
                         "min": BSUB_MIN_FIXED, "max": BSUB_MAX_FIXED}
                    ),
                    "error_limit": (
                        lambda i=i: self.channels[i].base_sub.error_limit,
                        lambda v, i=i: self.channels[i].set_base_sub("error_limit", v),
                        {"description": "Base Subtraction Error Limit",
                         "min": 0, "max": BSUB_MAX_ERROR}
                    ),
                    "div_cont": (
                        lambda i=i: MIN_DIV_CONT * (2 ** self.channels[i].base_sub.div_cont),
                        lambda v, i=i: self.channels[i].set_base_sub(
                            "div_cont", int(log2(v / MIN_DIV_CONT))),
                        {"description": "Division Count for Base Subtraction",
                         "allowed_values": self.DIV_CONT_ALLOWED}
                    )
                }
                for i in range(len(self.channels))
            },
            "filter": {
                # TODO: Instead of using the Filter Struct iargs etc,
                # setup the actual params for each filter?
                f"channel_{i}": {
                    "type": (
                        lambda i=i: self.channels[i].filter.filt_type.name.lower(),
                        lambda v, i=i: self.channels[i].set_filter("type", FilterType[v.upper()]),
                        {"description": "Type of Filter",
                         "allowed_values": [e.name.lower() for e in FilterType
                                            if e.name not in ["UNKNOWN", "CUSTOM"]]}
                    ),
                    "arg_1": (
                        lambda i=i: self.channels[i].filter.iarg1,
                        lambda v, i=i: self.channels[i].set_filter("arg1", v),
                        {"description": ("First integer filter argument. "
                                         "Exact use is dependant on type of filter")}
                    ),
                    "arg_2": (
                        lambda i=i: self.channels[i].filter.iarg2,
                        lambda v, i=i: self.channels[i].set_filter("arg2", v),
                        {"description": ("Second integer filter argument. "
                                         "Exact use is dependant on type of filter")}
                    ),
                    "arg_float": (
                        lambda i=i: self.channels[i].filter.darg,
                        lambda v, i=i: self.channels[i].set_filter("darg", v),
                        {"description": ("Float filter argument. "
                                         "Exact use is dependant on type of filter")}
                    )
                }
                for i in range(len(self.channels))
            },
            "discrimination": {
                f"channel_{i}": {
                    "height_min": (
                        lambda i=i: self.channels[i].tail_measure.min_height,
                        lambda v, i=i: self.channels[i].set_tail_measure("min_height", v),
                        {"description": "Minimum Discrimination Height",
                         "min": 0, "max": MEASURE_MAX_HEIGHT}
                    ),
                    "height_max": (
                        lambda i=i: self.channels[i].tail_measure.max_height,
                        lambda v, i=i: self.channels[i].set_tail_measure("max_height", v),
                        {"description": "Maximum Discrimination Height",
                         "min": 0, "max": MEASURE_MAX_HEIGHT}
                    ),
                    "adaptive": (
                        lambda i=i: self.channels[i].tail_measure.adaptive_tail_sum,
                        lambda v, i=i: self.channels[i].set_tail_measure("adaptive_tail_sum", v),
                        {"description": "Toggle the Adaptive Tail Sum"}
                    ),
                    "enable_tail_sum": (
                        lambda i=i: not self.channels[i].tail_measure.ignore_tail_sum,
                        lambda v, i=i: self.channels[i].set_tail_measure("ignore_tail_sum", not v),
                        {"description": "Toggle the use of the Tail sum in the discrimination"}
                    ),
                    "enable_fall_time": (
                        lambda i=i: self.channels[i].tail_measure.ignore_fall_time,
                        lambda v, i=i: self.channels[i].set_tail_measure("ignore_fall_time", not v),
                        {"description": "Toggle the use of the Fall Time in the discrimination"}
                    ),
                    "min_fall": (
                        lambda i=i: self.channels[i].tail_measure.min_fall,
                        lambda v, i=i: self.channels[i].set_tail_measure("min_fall", v),
                        {"description": "Minimum Fall Time",
                         "min": 0, "max": MEASURE_MAX_FALL_TIME}
                    ),
                    "max_fall": (
                        lambda i=i: self.channels[i].tail_measure.max_fall,
                        lambda v, i=i: self.channels[i].set_tail_measure("max_fall", v),
                        {"description": "Maximum Fall Time",
                         "min": 0, "max": MEASURE_MAX_FALL_TIME}
                    ),
                    "min_count": (
                        lambda i=i: self.channels[i].tail_measure.min_count,
                        lambda v, i=i: self.channels[i].set_tail_measure("min_count", v),
                        {"description": "Minimum Fall Time",
                         "min": 0, "max": MEASURE_MAX_TAIL_COUNT}
                    ),
                    "threshold_c": (
                        lambda i=i: self.channels[i].tail_measure.tail_thres_c,
                        lambda v, i=i: self.channels[i].set_tail_measure("tail_thres_c", v),
                        {"description": "C parameter for threshold calculation",
                         "min": -0x4000000, "max": 0x4000000-1}  # TODO: set up as consts in PyNgpd
                    ),
                    "threshold_m": (
                        lambda i=i: self.channels[i].tail_measure.tail_thres_m,
                        lambda v, i=i: self.channels[i].set_tail_measure("tail_thres_m", v),
                        {"description": "M parameter for threshold calculation",
                         "min": 0}
                    )
                }
                for i in range(len(self.channels))
            },
            "tail_measure": {
                f"channel_{i}": {
                    "delay": (
                        lambda i=i: self.channels[i].tail_measure.tail_sum_delay,
                        lambda v, i=i: self.channels[i].set_tail_measure("tail_sum_delay", v),
                        {"description": "Tail Sum Delay",
                         "min": 0, "max": MEASURE_MAX_DELAY}
                    ),
                    "num_sample": (
                        lambda i=i: self.channels[i].tail_measure.tail_sum_sample,
                        lambda v, i=i: self.channels[i].set_tail_measure("tail_sum_sample", v),
                        {"description": "Number of samples for Tail Sum measurement",
                         "min": 1, "max": MEASURE_MAX_SUM_NUM}
                    ),
                    "fall_time_frac": (
                        lambda i=i: self.channels[i].tail_measure.fall_time_frac,
                        lambda v, i=i: self.channels[i].set_tail_measure("fall_time_frac", v),
                        {"description": "Ratio for fall time calculation",
                         "min": 0.0, "max": 1.0}
                    ),
                    "enable_tail_subtract": (
                        lambda i=i: self.channels[i].tail_measure.enable_tail_subtract,
                        lambda v, i=i: self.channels[i].set_tail_measure("enable_tail_subtract", v),
                        {"description": "Toggle Tail Subtraction"}
                    ),
                    "enable_subtract_test": (
                        lambda i=i: self.channels[i].tail_measure.enable_subtract_test,
                        lambda v, i=i: self.channels[i].set_tail_measure("enable_subtract_test", v),
                        {"description": "Toggle Tail Subtraction Test"}
                    ),
                    "enable_subtract_neutron": (
                        lambda i=i: self.channels[i].tail_measure.enable_subtract_neutron,
                        lambda v, i=i: self.channels[i].set_tail_measure("enable_subtract_neutron", v),
                        {"description": "Toggle Test Neutron Subtraction"}
                    )
                }
                for i in range(len(self.channels))
            },
            "trigger": {
                f"channel_{i}": {
                    "threshold": (
                        lambda i=i: self.channels[i].trigger.thres,
                        lambda v, i=i: self.channels[i].set_trigger("thres", v),
                        {"description": "Trigger Threshold",
                         "min": TRIG_MIN_THRES, "max": TRIG_MAX_THRES}
                    ),
                    "separation": (
                        lambda i=i: self.channels[i].trigger.sep,
                        lambda v, i=i: self.channels[i].set_trigger("sep", v),
                        {"description": "Trigger Separation",
                         "min": TRIG_MIN_SEP, "max": TRIG_MAX_SEP}
                    ),
                    "data_delay": (
                        lambda i=i: self.channels[i].trigger.data_delay,
                        lambda v, i=i: self.channels[i].set_trigger("data_delay", v),
                        {"description": "Trigger Data Delay",
                         "min": TRIG_MIN_DATA_DELAY, "max": TRIG_MAX_DATA_DELAY}
                    ),
                    "trig_delay": (
                        lambda i=i: self.channels[i].trigger.trig_delay,
                        lambda v, i=i: self.channels[i].set_trigger("trig_delay", v),
                        {"description": "Trigger Delay",
                         "min": TRIG_MIN_TRIG_DELAY, "max": TRIG_MAX_TRIG_DELAY}
                    ),
                    "delay_a": (
                        lambda i=i: self.channels[i].trigger.delay_a,
                        lambda v, i=i: self.channels[i].set_trigger("delay_a", v),
                        {"description": "Trigger Delay A Parameter",
                         "min": TRIG_MIN_DELAY_AB, "max": TRIG_MAX_DELAY_AB}
                    ),
                    "delay_b": (
                        lambda i=i: self.channels[i].trigger.delay_b,
                        lambda v, i=i: self.channels[i].set_trigger("delay_b", v),
                        {"description": "Trigger Delay B Parameter",
                         "min": TRIG_MIN_DELAY_AB, "max": TRIG_MAX_DELAY_AB}
                    ),
                    "width_a": (
                        lambda i=i: self.channels[i].trigger.width_a,
                        lambda v, i=i: self.channels[i].set_trigger("width_a", v),
                        {"description": "Trigger Width A Parameter",
                         "min": TRIG_MIN_TRIG_STRETCH, "max": TRIG_MAX_TRIG_STRETCH}
                    ),
                    "width_b": (
                        lambda i=i: self.channels[i].trigger.width_b,
                        lambda v, i=i: self.channels[i].set_trigger("width_b", v),
                        {"description": "Trigger Width B Parameter",
                         "min": TRIG_MIN_TRIG_STRETCH, "max": TRIG_MAX_TRIG_STRETCH}
                    )
                }
                for i in range(len(self.channels))
            }
        }

    def configure(self):
        # we have to subtract 1 from the Ip Addr due to logic in William's code,
        # which adds the one back on when initialising
        logging.debug("Configuring Ngpd Device")
        self.ngpd = PyNgpd(str(self.ip - 1), self.num_cards, self.dummy_level)
        self.ngpd.setup_run_mode(self.dummy_level & DummyLevel.ADC)
        for channel in self.channels:
            channel.configure(self.ngpd)

    @property
    def adc_temps(self):
        if self.ngpd:
            self._adc_temps = self.ngpd.read_adc_temp()
        return self._adc_temps

    @property
    def preamp_temps(self):
        if self.ngpd:
            self._preamp_temps = self.ngpd.read_preamp_temp()
        return self._preamp_temps


class NgpdChannel:

    def __init__(self, chan_num: int):
        self.ngpd: PyNgpd = None
        self.chan = chan_num

        self._analog_gain = -1
        self._analog_offset = -1

        self._filter: PyNGPDFilter = None
        self._trigger: PyNGPDDiffTrigger = None
        self._base_sub: PyNGPDbassub = None
        self._tail_measure: PyNGPDTailMeasure = None

    def configure(self, ngpd: PyNgpd):
        self.ngpd = ngpd

    @property
    def analog_gain(self):
        if self.ngpd and self._analog_gain < 0:
            self._analog_gain = self.ngpd.read_dga_gain()[self.chan]
        return self._analog_gain

    @property
    def analog_offset(self):
        if self.ngpd and self._analog_offset < 0:
            self._analog_offset = self.ngpd.read_preamp_offset()[self.chan]
        return self._analog_offset

    @UsesNgpdLibrary
    def set_analog_gain(self, value: int):
        vals = self.ngpd.read_dga_gain()
        vals[self.chan] = value
        self.ngpd.write_dga_gain(0, vals)

    @UsesNgpdLibrary
    def set_analog_offset(self, value: int):
        vals = self.ngpd.read_preamp_offset()
        vals[self.chan] = value
        self.ngpd.write_preamp_offset(0, vals)

    @property
    def filter(self):
        if self.ngpd and self._filter is None:
            logging.debug(f"Reading Filter Struct for channel {self.chan}")
            self._filter = self.ngpd.get_filter_type(self.chan)
        return PyNGPDFilter() if self._filter is None else self._filter

    @property
    def trigger(self):
        if self.ngpd and self._trigger is None:
            logging.debug(f"Reading Trigger Struct for channel {self.chan}")
            self._trigger = self.ngpd.read_diff_trigger(self.chan)
        return PyNGPDDiffTrigger() if self._trigger is None else self._trigger

    @property
    def base_sub(self):
        if self.ngpd and self._base_sub is None:
            logging.debug(f"Reading Base Sub Struct for channel {self.chan}")
            self._base_sub = self.ngpd.read_basesub(self.chan)
        return PyNGPDbassub() if self._base_sub is None else self._base_sub

    @property
    def tail_measure(self):
        if self.ngpd and self._tail_measure is None:
            logging.debug(f"Reading Tail Measurement Struct for channel {self.chan}")
            self._tail_measure = self.ngpd.read_tail_measure(self.chan)
        return PyNGPDTailMeasure() if self._tail_measure is None else self._tail_measure

    @UsesNgpdLibrary
    def set_filter(self, setting: FilterSetting, value: int | float | FilterType):

        if setting == "type":
            self.filter.filt_type = value
        elif setting == "arg1":
            self.filter.iarg1 = value
        elif setting == "arg2":
            self.filter.iarg2 = value
        elif setting == "darg":
            self.filter.darg = value
        else:
            raise NgpdLibException(f"Invalid Filter Setting {setting}")

        type = self.filter.filt_type
        if type == FilterType.RECTANGLE:
            # TODO: check filter width in appropriate range
            self.ngpd.filter_load_rect(self.chan, self.filter.iarg1)
        elif type == FilterType.GAUSSIAN:
            self.ngpd.filter_load_gaus(self.chan, self.filter.darg)
        elif type == FilterType.EXPONENTIAL:
            self.ngpd.filter_load_exp(self.chan, self.filter.darg)
        elif type == FilterType.TRAPEZOIDAL:
            self.ngpd.filter_load_trapezoid(self.chan, self.filter.iarg1, self.filter.iarg2)
        else:
            raise NgpdLibException(f"Invalid Filter Type {type}")

    @UsesNgpdLibrary
    def set_trigger(self, setting: TriggerSetting, value: int):
        if hasattr(self.trigger, setting):
            setattr(self.trigger, setting, value)
        else:
            raise NgpdLibException(f"Invalid Filter Setting {setting}")
        self.ngpd.write_diff_trigger(self.chan, self.trigger)

    @UsesNgpdLibrary
    def set_base_sub(self, setting: BaseSubSetting, value: int | bool):
        if hasattr(self.base_sub, setting):
            setattr(self.base_sub, setting, value)
        else:
            raise NgpdLibException(f"Invalid Base Sub Setting {setting}")
        self.ngpd.write_basesub(self.chan, self.base_sub)

    @UsesNgpdLibrary
    def set_tail_measure(self, setting: TailMeasureSetting, value: int | float):
        if hasattr(self.tail_measure, setting):
            setattr(self.tail_measure, setting, value)
        else:
            raise NgpdLibException(f"Invalid Tail Measure Setting {setting}")

        self.ngpd.write_tail_measure(self.chan, self.tail_measure)
