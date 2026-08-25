"""NGPD Channel Class Module File."""
import logging
from typing import Literal

from ngpd.device.pyngpd import (
    FilterType,
    PyNgpd,
    PyNGPDbassub,
    PyNGPDDiffTrigger,
    PyNGPDFilter,
    PyNGPDTailMeasure,
)
from ngpd.util import NgpdLibException, UsesNgpdLibrary

FilterSetting = Literal["type", "arg1", "arg2", "darg"]
TriggerSetting = Literal["thres", "sep", "data_delay", "trig_delay",
                         "delay_a", "delay_b", "width_a", "width_b"]
BaseSubSetting = Literal["use_fixed", "fixed", "error_limit", "div_cont"]
HistogramSetting = Literal["separate_ngp",
                           "nbits_height", "shift_height",
                           "nbits_tail_sum", "shift_tail_sum"]
TailMeasureSetting = Literal["tail_sum_delay", "tail_sum_sample", "fall_time_frac",
                             "enable_tail_subtract", "enable_subtract_test",
                             "enable_subtract_neutron",
                             "ignore_tail_sum", "ignore_fall_time", "adaptive_tail_sum",
                             "min_height", "max_height", "min_fall", "max_fall", "min_count",
                             "tail_thres_c", "tail_thres_m"]


class NgpdChannel:
    """Ngpd Channel class for channel specific config."""

    def __init__(self, chan_num: int):
        """Init Method for NPGD Channel Class."""
        self.ngpd: PyNgpd = None
        self.chan = chan_num

        self._analog_gain = -1
        self._analog_offset = -1

        self._filter: PyNGPDFilter = None
        self._trigger: PyNGPDDiffTrigger = None
        self._base_sub: PyNGPDbassub = None
        self._tail_measure: PyNGPDTailMeasure = None

    def configure(self, ngpd: PyNgpd):
        """Configure the Channel by giving it access to the PyNGPD Class."""
        self.ngpd = ngpd

    @property
    def analog_gain(self):
        """Analog ADC Gain Value for this channel."""
        if self.ngpd:
            vals = self.ngpd.read_dga_gain()
            self._analog_gain = vals[self.chan]
        return self._analog_gain

    @property
    def analog_offset(self):
        """Analog Preamp Offset Value for this channel."""
        if self.ngpd and self._analog_offset < 0:
            self._analog_offset = self.ngpd.read_preamp_offset()[self.chan]
        return self._analog_offset

    @UsesNgpdLibrary
    def set_analog_gain(self, value: int):
        """Set the ADC Gain."""
        vals = self.ngpd.read_dga_gain()
        logging.debug(f"ANALOG GAIN VALS: {vals}")
        vals[self.chan] = value
        self._analog_gain = value
        self.ngpd.write_dga_gain(0, vals)

    @UsesNgpdLibrary
    def set_analog_offset(self, value: int):
        """Set the Preamp Offset."""
        vals = self.ngpd.read_preamp_offset()
        vals[self.chan] = value
        self._analog_offset = value
        self.ngpd.write_preamp_offset(0, vals)

    @property
    def filter(self):
        """NGPD Filter Dataclass for this channel, containing information about the Filter Type and it's Parameters."""
        if self.ngpd and self._filter is None:
            logging.debug(f"Reading Filter Struct for channel {self.chan}")
            self._filter = self.ngpd.get_filter_type(self.chan)
        return PyNGPDFilter() if self._filter is None else self._filter

    @property
    def trigger(self):
        """NGPD Trigger Dataclass for this channel, containing information about the various Threshold settings."""
        if self.ngpd and self._trigger is None:
            logging.debug(f"Reading Trigger Struct for channel {self.chan}")
            self._trigger = self.ngpd.read_diff_trigger(self.chan)
        return PyNGPDDiffTrigger() if self._trigger is None else self._trigger

    @property
    def base_sub(self):
        """NGPD Baseline Subtraction Dataclass for this channel."""
        if self.ngpd and self._base_sub is None:
            logging.debug(f"Reading Base Sub Struct for channel {self.chan}")
            self._base_sub = self.ngpd.read_basesub(self.chan)
        return PyNGPDbassub() if self._base_sub is None else self._base_sub

    @property
    def tail_measure(self):
        """NGPD Tail Measurement Dataclass for this channel."""
        if self.ngpd and self._tail_measure is None:
            logging.debug(f"Reading Tail Measurement Struct for channel {self.chan}")
            self._tail_measure = self.ngpd.read_tail_measure(self.chan)
        return PyNGPDTailMeasure() if self._tail_measure is None else self._tail_measure

    @UsesNgpdLibrary
    def set_filter(self, setting: FilterSetting, value: int | float | FilterType):
        """Set the channel to use a specific filter with specific arguments."""
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
        """Set one of the Trigger settings and apply for this channel."""
        if hasattr(self.trigger, setting):
            setattr(self.trigger, setting, value)
        else:
            raise NgpdLibException(f"Invalid Filter Setting {setting}")
        self.ngpd.write_diff_trigger(self.chan, self.trigger)

    @UsesNgpdLibrary
    def set_base_sub(self, setting: BaseSubSetting, value: int | bool):
        """Set one of the Baseline Subtraction settings for this channel."""
        if hasattr(self.base_sub, setting):
            setattr(self.base_sub, setting, value)
        else:
            raise NgpdLibException(f"Invalid Base Sub Setting {setting}")
        self.ngpd.write_basesub(self.chan, self.base_sub)

    @UsesNgpdLibrary
    def set_tail_measure(self, setting: TailMeasureSetting, value: int | float):
        """Set one of the Tail Measurement settings for this channel."""
        if hasattr(self.tail_measure, setting):
            setattr(self.tail_measure, setting, value)
        else:
            raise NgpdLibException(f"Invalid Tail Measure Setting {setting}")

        self.ngpd.write_tail_measure(self.chan, self.tail_measure)
