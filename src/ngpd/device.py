from ngpd.pyngpd import PyNgpd, DummyLevel
from ngpd.pyngpd import PyNGPDFilter, PyNGPDDiffTrigger, PyNGPDbassub, PyNGPDTailMeasure
from ngpd.util import UsesNgpdLibrary, NgpdLibException
from ipaddress import ip_address
from typing import Literal
from math import log2
import logging


class NgpdDevice:
    """
    NGPD Handling class, providing a bridge between Adapter/Controller
    and the interface class of PyNgpd
    """

    MAX_ANALOG_GAIN = 35
    MAX_ANALOG_OFFSET = 65535

    MIN_BSUB_FIXED = -65536
    MAX_BSUB_FIXED = 65535

    MAX_BSUB_ERROR = 65535

    MIN_DIV_CONT = 64
    NUM_DIV_CONT = 8
    DIV_CONT_ALLOWED = [64 * (2 ** x) for x in range(8)]

    def __init__(self, options: dict[str, str]):
        self.ip = ip_address(options.get("ip_addr", "192.168.0.1"))
        self.num_cards = int(options.get("num_cards", 1))
        self.dummy_level = DummyLevel[options.get("dummy_level", "none").upper()]

        self.ngpd: PyNgpd = None

        self._adc_temps = [0] * 4
        self._preamp_temps = [0] * 2

        self._analog_gain = [0] * 8
        self._analog_offset = [0] * 8

        self._base_sub: list[PyNGPDbassub] = [None] * 8
        self._filter: list[PyNGPDFilter] = [None] * 8
        self._tail_measure: list[PyNGPDTailMeasure] = [None] * 8

        self.tree = {
            "analog": {
                "gain": (
                    lambda: self.gain,
                    self.set_gain,
                    {"description": "Analog Gain for each channel"}
                ),
                "offset": (
                    lambda: self.offset,
                    self.set_offset,
                    {"description": "Analog Offset for each channel"}
                )
            },
            "base_sub": {
                "use_fixed": (
                    lambda: [sub.use_fixed for sub in self.base_sub],
                    lambda v: self.set_base_sub("use_fixed", v),
                    {"description": "Toggle the use of a fixed Baseline value for each channel."}
                ),
                "fixed": (
                    lambda: [sub.fixed for sub in self.base_sub],
                    lambda v: self.set_base_sub("fixed", v),
                    {"description": "Fixed values for each channel's Baseline Subtraction, if used"}
                ),
                "error_limit": (
                    lambda: [sub.error_limit for sub in self.base_sub],
                    lambda v: self.set_base_sub("error_limit", v),
                    {"description": "Baseline Subtraction Error Limit for each channel"}
                ),
                "div_cont": (
                    lambda: [64 * (2 ** sub.div_cont) for sub in self.base_sub],
                    lambda v: self.set_base_sub("div_cont", v),
                    {"description": "Division Count for each channel's Baseline Subtraction"}
                )
            },
            "filter": {
                "type": (
                    lambda: [filt.filt_type.name.lower() for filt in self.filter],
                    None,
                    {"description": "Type of filter used for each channel"}
                ),
                "arg_1": (
                    lambda: [filt.iarg1 for filt in self.filter],
                    None,
                    {"description": "First Argument for each channel filter. "
                                    "Exact use depends of filter type"}
                ),
                "arg_2": (
                    lambda: [filt.iarg2 for filt in self.filter],
                    None,
                    {"description": "Second Argument for each channel filter. "
                                    "Exact use depends of filter type"}
                ),
                "arg_3": (
                    lambda: [filt.darg for filt in self.filter],
                    None,
                    {"description": "Float Argument for each channel filter. "
                                    "Exact use depends of filter type"}
                )
            },
            "discrimination": {
                "height_min": (
                    lambda: [measure.min_height for measure in self.tail_measure],
                    None,
                    {"description": "Minimum Discrimination Height"}
                ),
                "height_max": (
                    lambda: [measure.max_height for measure in self.tail_measure],
                    None,
                    {"description": "Maximum Discrimination Height"}
                ),
                "adaptive": (
                    lambda: [measure.adaptive_tail_sum for measure in self.tail_measure],
                    None,
                    {"description": "Toggle Adaptive Tail Sum"}
                ),
                "enable_tail_sum": (
                    lambda: [not measure.ignore_tail_sum for measure in self.tail_measure],
                    None,
                    {"description": "Use Tail Sum measurement in Discrimination"}
                ),
                "enable_fall_time": (
                    lambda: [not measure.ignore_fall_time for measure in self.tail_measure],
                    None,
                    {"description": "Use Fall Time measurement in Discrimination"}
                ),
                "min_fall": (
                    lambda: [measure.min_fall for measure in self.tail_measure],
                    None,
                    {"description": "Minimum Fall Time"}
                ),
                "max_fall": (
                    lambda: [measure.max_fall for measure in self.tail_measure],
                    None,
                    {"description": "Maximum Fall Time"}
                ),
                "min_count": (
                    lambda: [measure.min_count for measure in self.tail_measure],
                    None,
                    {"description": "Minimum Tail Count"}
                ),
                "thres_c": (
                    lambda: [measure.tail_thres_c for measure in self.tail_measure],
                    None,
                    {"description": "Tail Threshold C"}
                ),
                "thres_m": (
                    lambda: [measure.tail_thres_m for measure in self.tail_measure],
                    None,
                    {"description": "Tail Threshold M"}
                )
            }
        }

    def configure(self):
        # we have to subtract 1 from the Ip Addr due to logic in William's code,
        # which adds the one back on when initialising
        logging.debug("Configuring Ngpd Device")
        self.ngpd = PyNgpd(str(self.ip - 1), self.num_cards, self.dummy_level)
        self.ngpd.setup_run_mode(self.dummy_level & DummyLevel.ADC)

    @property
    def gain(self):
        if self.ngpd:
            self._analog_gain = self.ngpd.read_dga_gain(0, self.ngpd.num_chan)
        return self._analog_gain

    @UsesNgpdLibrary
    def set_gain(self, values: list[int]):
        """Set the Gain for every channel"""
        for i, val in enumerate(values):
            if not (0 <= val <= self.MAX_ANALOG_GAIN):
                raise NgpdLibException(
                    f"Invalid value for channel {i} gain. "
                    f"{val} not between 0 and {self.MAX_ANALOG_GAIN}"
                )
        self.ngpd.write_dga_gain(0, values)

    @property
    def offset(self):
        if self.ngpd:
            self._analog_offset = self.ngpd.read_preamp_offset(0, self.ngpd.num_chan)
        return self._analog_offset

    @UsesNgpdLibrary
    def set_offset(self, values: list[int]):
        for i, val in enumerate(values):
            if not (0 <= val <= self.MAX_ANALOG_OFFSET):
                raise NgpdLibException(
                    f"Invalid value for channel {i} offset. ",
                    f"{val} not between 0 and {self.MAX_ANALOG_GAIN}"
                )
        self.ngpd.write_preamp_offset(0, values)

    @property
    def base_sub(self):
        if self.ngpd:
            for index, base_sub in enumerate(self._base_sub):
                if base_sub is None:
                    logging.debug(f"Reading Base Sub Struct for channel {index}")
                    self._base_sub[index] = self.ngpd.read_basesub(index)
        return [PyNGPDbassub() if sub is None else sub
                for sub in self._base_sub]

    @UsesNgpdLibrary
    def set_base_sub(self, setting: Literal["use_fixed", "fixed", "error_limit", "div_cont"],
                     values: list[int] | list[bool]):

        subs = self.base_sub
        if len(subs) != len(values):
            raise NgpdLibException(
                f"Incorrect number of values for Base Sub. Expected {len(subs)} got {len(values)}"
            )
        if setting == "use_fixed":
            for i, value in enumerate(values):
                subs[i].use_fixed = value
        elif setting == "fixed":
            for i, value in enumerate(values):
                if not (self.MIN_BSUB_FIXED <= value <= self.MAX_BSUB_FIXED):
                    raise NgpdLibException(
                        f"Invalid value for bsub fixed on channel {i}. "
                        f"{value} is not between {self.MIN_BSUB_FIXED} and {self.MAX_BSUB_FIXED}"
                    )
                subs[i].fixed = value
        elif setting == "error_limit":
            for i, value in enumerate(values):
                if not (0 <= value <= self.MAX_BSUB_FIXED):
                    raise NgpdLibException(
                        f"Invalid value for bsub error limit on channel {i}. "
                        f"{value} is not between 0 and {self.MAX_BSUB_ERROR}"
                    )
                subs[i].error_limit = value
        elif setting == "div_cont":
            for i, value in enumerate(values):
                if value not in self.DIV_CONT_ALLOWED:
                    raise NgpdLibException(
                        f"Invalid value {value} for bsub Div Count. ",
                        f"Must be one of {self.DIV_CONT_ALLOWED}"
                    )
                subs[i] = int(log2(value / self.MIN_DIV_CONT))

        for i, bsub in enumerate(subs):
            self.ngpd.write_basesub(i, bsub)

    @property
    def filter(self):
        if self.ngpd:
            for index, filt in enumerate(self._filter):
                if filt is None:
                    logging.debug(f"Reading Filter Struct for channel {index}")
                    self._filter[index] = self.ngpd.get_filter_type(index)
        return [PyNGPDFilter() if filt is None else filt
                for filt in self._filter]

    @property
    def tail_measure(self):
        if self.ngpd:
            for index, measure in enumerate(self._tail_measure):
                if measure is None:
                    logging.debug(f"Reading Tail Measure Struct for channel {index}")
                    self._tail_measure[index] = self.ngpd.read_tail_measure(index)
        return [PyNGPDTailMeasure() if measure is None else measure
                for measure in self._tail_measure]

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
