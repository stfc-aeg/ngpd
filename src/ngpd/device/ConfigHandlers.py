"""Config Handler Classes."""

from math import log2

from ngpd.device.channel import NgpdChannel
from ngpd.device.pyngpd import (
    BSUB_MAX_ERROR,
    BSUB_MAX_FIXED,
    BSUB_MIN_FIXED,
    MIN_DIV_CONT,
    NUM_DIV_CONT,
    TRIG_MAX_DATA_DELAY,
    TRIG_MAX_DELAY_AB,
    TRIG_MAX_SEP,
    TRIG_MAX_THRES,
    TRIG_MAX_TRIG_DELAY,
    TRIG_MAX_TRIG_STRETCH,
    TRIG_MIN_DATA_DELAY,
    TRIG_MIN_DELAY_AB,
    TRIG_MIN_SEP,
    TRIG_MIN_THRES,
    TRIG_MIN_TRIG_DELAY,
    TRIG_MIN_TRIG_STRETCH,
    MEASURE_MAX_HEIGHT,
    MEASURE_MAX_FALL_TIME,
    MEASURE_MAX_TAIL_COUNT,
    MEASURE_MAX_DELAY,
    MEASURE_MAX_SUM_NUM,
    FilterType,
)

DIV_CONT_ALLOWED = [MIN_DIV_CONT * (2**i) for i in range(NUM_DIV_CONT)]


class BaseSubConfig:
    """Base Subtraction Config Handler Class."""

    def __init__(self, channels: list[NgpdChannel]):
        """Init the Base Sub Config Handler."""
        self.channels = channels
        self.tree = {
            f"channel_{i}": {
                "use_fixed": (
                    lambda i=i: self.channels[i].base_sub.use_fixed,
                    lambda v, i=i: self.channels[i].set_base_sub("use_fixed", v),
                    {"description": "Toggle the use of a fixed Base Subtraction Value"},
                ),
                "fixed": (
                    lambda i=i: self.channels[i].base_sub.fixed,
                    lambda v, i=i: self.channels[i].set_base_sub("fixed", v),
                    {
                        "description": "Fixed Base Subtraction Value",
                        "min": BSUB_MIN_FIXED,
                        "max": BSUB_MAX_FIXED,
                    },
                ),
                "error_limit": (
                    lambda i=i: self.channels[i].base_sub.error_limit,
                    lambda v, i=i: self.channels[i].set_base_sub("error_limit", v),
                    {
                        "description": "Base Subtraction Error Limit",
                        "min": 0,
                        "max": BSUB_MAX_ERROR,
                    },
                ),
                "div_cont": (
                    lambda i=i: (
                        MIN_DIV_CONT * (2 ** self.channels[i].base_sub.div_cont)
                    ),
                    lambda v, i=i: self.channels[i].set_base_sub(
                        "div_cont", int(log2(v / MIN_DIV_CONT))
                    ),
                    {
                        "description": "Division Count for Base Subtraction",
                        "allowed_values": DIV_CONT_ALLOWED,
                    },
                ),
            }
            for i in range(len(self.channels))
        }


class FilterConfig:
    """Filter Config Handler."""

    def __init__(self, channels: list[NgpdChannel]):
        """Init the Config Module."""
        self.channels = channels
        self.tree = {
            f"channel_{i}": {
                "type": (
                    lambda i=i: self.channels[i].filter.filt_type.name.lower(),
                    lambda v, i=i: self.channels[i].set_filter(
                        "type", FilterType[v.upper()]
                    ),
                    {
                        "description": "Type of Filter",
                        "allowed_values": [
                            e.name.lower()
                            for e in FilterType
                            if e.name not in ["UNKNOWN", "CUSTOM"]
                        ],
                    },
                ),
                "arg_1": (
                    lambda i=i: self.channels[i].filter.iarg1,
                    lambda v, i=i: self.channels[i].set_filter("arg1", v),
                    {
                        "description": (
                            "First integer filter argument. "
                            "Exact use is dependant on type of filter"
                        )
                    },
                ),
                "arg_2": (
                    lambda i=i: self.channels[i].filter.iarg2,
                    lambda v, i=i: self.channels[i].set_filter("arg2", v),
                    {
                        "description": (
                            "Second integer filter argument. "
                            "Exact use is dependant on type of filter"
                        )
                    },
                ),
                "arg_float": (
                    lambda i=i: self.channels[i].filter.darg,
                    lambda v, i=i: self.channels[i].set_filter("darg", v),
                    {
                        "description": (
                            "Float filter argument. "
                            "Exact use is dependant on type of filter"
                        )
                    },
                ),
            }
            for i in range(len(self.channels))
        }


class DiscriminationConfig:
    """Discrimination Config Handler."""

    def __init__(self, channels: list[NgpdChannel]):
        """Init the Config Handler."""
        self.channels = channels
        self.tree = {
            f"channel_{i}": {
                "height_min": (
                    lambda i=i: self.channels[i].tail_measure.min_height,
                    lambda v, i=i: self.channels[i].set_tail_measure("min_height", v),
                    {
                        "description": "Minimum Discrimination Height",
                        "min": 0,
                        "max": MEASURE_MAX_HEIGHT,
                    },
                ),
                "height_max": (
                    lambda i=i: self.channels[i].tail_measure.max_height,
                    lambda v, i=i: self.channels[i].set_tail_measure("max_height", v),
                    {
                        "description": "Maximum Discrimination Height",
                        "min": 0,
                        "max": MEASURE_MAX_HEIGHT,
                    },
                ),
                "adaptive": (
                    lambda i=i: self.channels[i].tail_measure.adaptive_tail_sum,
                    lambda v, i=i: self.channels[i].set_tail_measure(
                        "adaptive_tail_sum", v
                    ),
                    {"description": "Toggle the Adaptive Tail Sum"},
                ),
                "enable_tail_sum": (
                    lambda i=i: not self.channels[i].tail_measure.ignore_tail_sum,
                    lambda v, i=i: self.channels[i].set_tail_measure(
                        "ignore_tail_sum", not v
                    ),
                    {
                        "description": "Toggle the use of the Tail sum in the discrimination"
                    },
                ),
                "enable_fall_time": (
                    lambda i=i: not self.channels[i].tail_measure.ignore_fall_time,
                    lambda v, i=i: self.channels[i].set_tail_measure(
                        "ignore_fall_time", not v
                    ),
                    {
                        "description": "Toggle the use of the Fall Time in the discrimination"
                    },
                ),
                "min_fall": (
                    lambda i=i: self.channels[i].tail_measure.min_fall,
                    lambda v, i=i: self.channels[i].set_tail_measure("min_fall", v),
                    {
                        "description": "Minimum Fall Time",
                        "min": 0,
                        "max": MEASURE_MAX_FALL_TIME,
                    },
                ),
                "max_fall": (
                    lambda i=i: self.channels[i].tail_measure.max_fall,
                    lambda v, i=i: self.channels[i].set_tail_measure("max_fall", v),
                    {
                        "description": "Maximum Fall Time",
                        "min": 0,
                        "max": MEASURE_MAX_FALL_TIME,
                    },
                ),
                "min_count": (
                    lambda i=i: self.channels[i].tail_measure.min_count,
                    lambda v, i=i: self.channels[i].set_tail_measure("min_count", v),
                    {
                        "description": "Minimum Pulse Count",
                        "min": 0,
                        "max": MEASURE_MAX_TAIL_COUNT,
                    },
                ),
                "threshold_c": (
                    lambda i=i: self.channels[i].tail_measure.tail_thres_c,
                    lambda v, i=i: self.channels[i].set_tail_measure("tail_thres_c", v),
                    {
                        "description": "C parameter for threshold calculation",
                        "min": -0x4000000,
                        "max": 0x4000000 - 1,
                    },  # TODO: set up as consts in PyNgpd
                ),
                "threshold_m": (
                    lambda i=i: self.channels[i].tail_measure.tail_thres_m,
                    lambda v, i=i: self.channels[i].set_tail_measure("tail_thres_m", v),
                    {
                        "description": "M parameter for threshold calculation",
                        "min": 0,
                    },
                ),
            }
            for i in range(len(self.channels))
        }


class TailMeasureConfig:
    """Tail Measurement Config Handler."""

    def __init__(self, channels: list[NgpdChannel]):
        """Init the Config Handler."""
        self.channels = channels
        self.tree = {
            f"channel_{i}": {
                "delay": (
                    lambda i=i: self.channels[i].tail_measure.tail_sum_delay,
                    lambda v, i=i: self.channels[i].set_tail_measure(
                        "tail_sum_delay", v
                    ),
                    {
                        "description": "Tail Sum Delay",
                        "min": 0,
                        "max": MEASURE_MAX_DELAY,
                    },
                ),
                "num_sample": (
                    lambda i=i: self.channels[i].tail_measure.tail_sum_sample,
                    lambda v, i=i: self.channels[i].set_tail_measure(
                        "tail_sum_sample", v
                    ),
                    {
                        "description": "Number of samples for Tail Sum measurement",
                        "min": 1,
                        "max": MEASURE_MAX_SUM_NUM,
                    },
                ),
                "fall_time_frac": (
                    lambda i=i: self.channels[i].tail_measure.fall_time_frac,
                    lambda v, i=i: self.channels[i].set_tail_measure(
                        "fall_time_frac", v
                    ),
                    {
                        "description": "Ratio for fall time calculation",
                        "min": 0.0,
                        "max": 1.0,
                    },
                ),
                "enable_tail_subtract": (
                    lambda i=i: self.channels[i].tail_measure.enable_tail_subtract,
                    lambda v, i=i: self.channels[i].set_tail_measure(
                        "enable_tail_subtract", v
                    ),
                    {"description": "Toggle Tail Subtraction"},
                ),
                "enable_subtract_test": (
                    lambda i=i: self.channels[i].tail_measure.enable_subtract_test,
                    lambda v, i=i: self.channels[i].set_tail_measure(
                        "enable_subtract_test", v
                    ),
                    {"description": "Toggle Tail Subtraction Test"},
                ),
                "enable_subtract_neutron": (
                    lambda i=i: self.channels[i].tail_measure.enable_subtract_neutron,
                    lambda v, i=i: self.channels[i].set_tail_measure(
                        "enable_subtract_neutron", v
                    ),
                    {"description": "Toggle Test Neutron Subtraction"},
                ),
            }
            for i in range(len(self.channels))
        }


class TriggerConfig:
    """Trigger Config Handler."""

    def __init__(self, channels: list[NgpdChannel]):
        """Init the Config Handler."""
        self.channels = channels
        self.tree = {
            f"channel_{i}": {
                "threshold": (
                    lambda i=i: self.channels[i].trigger.thres,
                    lambda v, i=i: self.channels[i].set_trigger("thres", v),
                    {
                        "description": "Trigger Threshold",
                        "min": TRIG_MIN_THRES,
                        "max": TRIG_MAX_THRES,
                    },
                ),
                "separation": (
                    lambda i=i: self.channels[i].trigger.sep,
                    lambda v, i=i: self.channels[i].set_trigger("sep", v),
                    {
                        "description": "Trigger Separation",
                        "min": TRIG_MIN_SEP,
                        "max": TRIG_MAX_SEP,
                    },
                ),
                "data_delay": (
                    lambda i=i: self.channels[i].trigger.data_delay,
                    lambda v, i=i: self.channels[i].set_trigger("data_delay", v),
                    {
                        "description": "Trigger Data Delay",
                        "min": TRIG_MIN_DATA_DELAY,
                        "max": TRIG_MAX_DATA_DELAY,
                    },
                ),
                "trig_delay": (
                    lambda i=i: self.channels[i].trigger.trig_delay,
                    lambda v, i=i: self.channels[i].set_trigger("trig_delay", v),
                    {
                        "description": "Trigger Delay",
                        "min": TRIG_MIN_TRIG_DELAY,
                        "max": TRIG_MAX_TRIG_DELAY,
                    },
                ),
                "delay_a": (
                    lambda i=i: self.channels[i].trigger.delay_a,
                    lambda v, i=i: self.channels[i].set_trigger("delay_a", v),
                    {
                        "description": "Trigger Delay A Parameter",
                        "min": TRIG_MIN_DELAY_AB,
                        "max": TRIG_MAX_DELAY_AB,
                    },
                ),
                "delay_b": (
                    lambda i=i: self.channels[i].trigger.delay_b,
                    lambda v, i=i: self.channels[i].set_trigger("delay_b", v),
                    {
                        "description": "Trigger Delay B Parameter",
                        "min": TRIG_MIN_DELAY_AB,
                        "max": TRIG_MAX_DELAY_AB,
                    },
                ),
                "width_a": (
                    lambda i=i: self.channels[i].trigger.width_a,
                    lambda v, i=i: self.channels[i].set_trigger("width_a", v),
                    {
                        "description": "Trigger Width A Parameter",
                        "min": TRIG_MIN_TRIG_STRETCH,
                        "max": TRIG_MAX_TRIG_STRETCH,
                    },
                ),
                "width_b": (
                    lambda i=i: self.channels[i].trigger.width_b,
                    lambda v, i=i: self.channels[i].set_trigger("width_b", v),
                    {
                        "description": "Trigger Width B Parameter",
                        "min": TRIG_MIN_TRIG_STRETCH,
                        "max": TRIG_MAX_TRIG_STRETCH,
                    },
                ),
            }
            for i in range(len(self.channels))
        }