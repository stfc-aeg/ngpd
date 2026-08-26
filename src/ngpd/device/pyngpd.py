"""PyNGPD Class Module.

Contains the Main PyNGPD Class for library access, and dataclasses to replicate the various C structs from the library.
"""

import logging
from dataclasses import dataclass
from enum import IntEnum, IntFlag, auto
from typing import Literal

import numpy as np

from ngpd import cffi_lib as lib
from ngpd.cffi_lib import ffi
from ngpd.util import NgpdLibException

ANALOG_MAX_GAIN = 35
ANALOG_MAX_OFFSET = 65535

BSUB_MIN_FIXED = -65536
BSUB_MAX_FIXED = 65535
BSUB_MAX_ERROR = 65535

MIN_DIV_CONT = 64
NUM_DIV_CONT = 8

MEASURE_THRES_SIZE = lib.NGPD_MEASURE_TAIL_THRES_SIZE
MEASURE_MAX_DELAY = lib.NGPD_MEASURE_SUM_DELAY_MAX
MEASURE_MAX_SUM_NUM = lib.NGPD_MEASURE_SUM_NUM_MAX
MEASURE_MAX_HEIGHT = lib.NGPD_MEASURE_HEIGHT_MAX
MEASURE_MAX_FALL_TIME = lib.NGPD_MEASURE_FALL_TIME_MAX
MEASURE_MAX_TAIL_COUNT = lib.NGPD_MEASURE_TAIL_COUNT_MAX

TRIG_MIN_SEP = lib.NGPD_DIFF_TRIG_MIN_SEP
TRIG_MAX_SEP = lib.NGPD_DIFF_TRIG_MAX_SEP
TRIG_MIN_DATA_DELAY = lib.NGPD_DIFF_TRIG_MIN_DATA_DELAY
TRIG_MAX_DATA_DELAY = lib.NGPD_DIFF_TRIG_MAX_DATA_DELAY
TRIG_MIN_TRIG_DELAY = lib.NGPD_DIFF_TRIG_MIN_TRIG_DELAY
TRIG_MAX_TRIG_DELAY = lib.NGPD_DIFF_TRIG_MAX_TRIG_DELAY
TRIG_MIN_DELAY_AB = lib.NGPD_DIFF_TRIG_MIN_DELAY_AB
TRIG_MAX_DELAY_AB = lib.NGPD_DIFF_TRIG_MAX_DELAY_AB
TRIG_MIN_TRIG_STRETCH = lib.NGPD_DIFF_TRIG_MIN_TRIG_STRETCH
TRIG_MAX_TRIG_STRETCH = lib.NGPD_DIFF_TRIG_MAX_TRIG_STRETCH
TRIG_MIN_THRES = lib.NGPD_DIFF_TRIG_MIN_THRES
TRIG_MAX_THRES = lib.NGPD_DIFF_TRIG_MAX_THRES

VOLTAGE_SIGNAL_NAMES = [
    [
        "AVDD3V3_INT_3V9",
        "AVDD1V_INT_1V3",
        "AVDD1V8_INT_2V4",
        "AVDD2V5_INT_3V4",
        "DVDD1V_INT_1V3",
        "VDD3V3_CLK_INT_3V9",
        "VDD1V2_DIG",
        "VDD3V3_DAE",
        "VDD1V8_SPI",
        "AVDD3V3_0",
        "AVDD3V3_1",
        "AVDD1V_0",
        "AVDD1V_1",
        "VIN_12VA",
        None,
        None,
    ],
    [
        "AVDD2V5_0",
        "AVDD2V5_1",
        "DVDD1V_0",
        "DVDD1V_1",
        "AVDD1V8_PLL_0",
        "AVDD1V8_PLL_1",
        "AVDD1V8_0",
        "AVDD1V8_1",
        "VIN_12VB",
        "VDD3V3_CLK",
        "VDD5V_I2C",
        "VMON_INT_OR_2V",
        "VMON_VTT",
        "VMON_PSINT_OR_1V2",
        "VMON_0V9",
        "VMON_1V2_OR_BAT",
    ],
]

ITFG_State = Literal["error, paused, counting, not_counting, idle"]

class DummyLevel(IntFlag):
    """Defines level of Dummy parts to the system, IE how much to simulate."""

    NONE = 0
    """No Simulated parts"""
    FPGA = 1
    """Simulate FPGA"""
    ADC = 2
    """Simulate ADC Read/Writes"""
    PREAMP = 4
    """Simulate Preamps"""
    DEV = 6
    """ADC + PREAMP"""
    ALL = 0xFF
    """Simulate Everything"""


class ScopeStatus(IntFlag):
    """Enum for Scope Status."""

    RUNNING = auto()
    OUT_RUNNING = auto()
    OUT_PAUSED = auto()
    ITFG_RUNNING = auto()
    ITFG_WAITING = auto()
    ITFG_COUNTING = auto()

    ERROR = 0



class FilterType(IntEnum):
    """Enum for Filter Type, replcating C Library's Enum."""

    UNKNOWN = 0
    RECTANGLE = auto()
    GAUSSIAN = auto()
    EXPONENTIAL = auto()
    TRAPEZOIDAL = auto()
    CUSTOM = auto()


@dataclass
class PyNGPDbassub:
    """Base Subtraction Struct."""

    use_fixed: bool = False
    fixed: int = -1
    error_limit: int = -1
    div_cont: int = 0


@dataclass
class PyNGPDDiffTrigger:
    """Differential Trigger Struct."""

    thres: int = -1
    sep: int = -1
    data_delay: int = -1
    trig_delay: int = -1
    delay_a: int = -1
    width_a: int = -1
    delay_b: int = -1
    width_b: int = -1


@dataclass
class PyNGPDFilter:
    """Filter Struct."""

    filt_type: FilterType = FilterType.UNKNOWN
    iarg1: int = -1
    iarg2: int = -1
    darg: float = -1.0


@dataclass
class PyNGPDChanCont:
    """Channel Config Struct."""

    use_pb_start: int = -1
    data_src: int = -1
    inv_data: int = -1


@dataclass
class PyNGPDTailMeasure:
    """Tail Measurement Struct."""

    tail_sum_delay: int = -1
    tail_sum_sample: int = -1
    fall_time_frac: float = -1.0
    enable_tail_subtract: bool = False
    enable_subtract_test: bool = False
    enable_subtract_neutron: bool = False
    min_height: int = -1
    max_height: int = -1
    adaptive_tail_sum: bool = False
    min_fall: int = -1
    max_fall: int = -1
    min_count: int = -1
    ignore_fall_time: bool = False
    ignore_tail_sum: bool = False
    tail_thres_c: int = -1
    tail_thres_m: float = -1.0


@dataclass
class SystemMonitor:
    """FPGA System Information Dataclass."""

    AMS_PSTempLPD: int = -1
    AMS_PSTempFPD: int = -1
    AMS_PSVccIntLP: int = -1
    AMS_PSVccIntFP: int = -1
    AMS_PSVccAux: int = -1
    AMS_PSVccDDR: int = -1
    AMS_PSVccIO0: int = -1
    AMS_PSVccIO1: int = -1
    AMS_PSVccIO2: int = -1
    AMS_PSVccIO3: int = -1
    AMS_PSAVccMGTR: int = -1
    AMS_PSAVttMGTR: int = -1
    AMS_PSVccAMS: int = -1
    AMS_PSVccPLL0: int = -1
    AMS_PSVccBatt: int = -1
    AMS_PLVccInt: int = -1
    AMS_PLVccBRAM: int = -1
    AMS_PLVccAux: int = -1
    AMS_PSVccDDRPLL: int = -1
    AMS_PSVccIntFPDDR: int = -1
    XADC_VccInt: int = -1
    XADC_VccAux: int = -1
    XADC_VccBRam: int = -1
    XADC_VccPSInt: int = -1
    XADC_VccPSAux: int = -1
    XADC_VccoDdr: int = -1
    XADC_Temp: int = -1


@dataclass
class HistogramConfig:
    """Histogram Configuration Struct."""

    separate_ngp: bool = False
    enb_hgt_sum: bool = False
    enb_hgt_fall: bool = False
    discard_pu: bool = False
    nbits_height: int = 0
    shift_height: int = 0
    nbits_fall_time: int = 0
    shift_fall_time: int = 0
    nbits_tail_sum: int = 0
    shift_tail_sum: int = 0


@dataclass
class ITFGStatus:
    """ITFG Status Struct."""

    flags: ScopeStatus = ScopeStatus.ERROR
    cycles: int = -1
    timer_us: int = -1


@dataclass
class ITFGSetup:
    """ITFG Setup Struct."""

    col_time: float
    trig_mode: int
    cycles: int


class PyNgpd:
    """Python class handing all NGPD config and Access."""

    def __init__(self, ipaddr: str, num_cards=1, dummy=DummyLevel.NONE, first_card=0):
        """Create a Python class to interface with the NGPD system via the CFFI Library.

        :param ipaddr: The IP Address of the NGPD System. In a multi-card system, this should be the address of the first card.
        :param num_cards: The Number of cards in the system.
        :param nummy: The Dummy Level for the system. Defines which parts, if any, the library should "simulate".
        :param first_card: The index of the first card in the NGPD System.
        """
        self.num_cards = num_cards
        self.first_crad = first_card
        self.dummy = dummy
        c_ip_addr = ffi.new("char[]", ipaddr.encode())

        self.path = lib.ngpd_config_ngzmp(
            num_cards, c_ip_addr, -1, ffi.NULL, -1, 0, first_card, 0, dummy
        )

        if self.path < 0:
            raise NgpdLibException("Cannot open path to NGPD Board")

        self.num_chan = lib.ngpd_get_num_chan(self.path)
        self.itfg_setup = ITFGSetup(1, lib.NGPDTrigSW, 1)

        logging.debug("Completed NGPD Init")

    def get_error_message(self) -> str:
        """Get the Error message from the cffi Library."""
        return str(ffi.string(lib.ngpd_get_error_message()))

    def write_preamp_offset(self, chan: int, values: list[int]):
        """Write the offsets to the various channel's Preamps.

        :param chan: First channel in the list to write the offset for.
        :param values: Array of Offset values to write.
        """
        num = len(values)
        ptr = ffi.new("uint16_t[]", values)
        ret_code = lib.ngpd_i2c_write_preamp_offset(self.path, chan, num, ptr)
        return ret_code

    def read_preamp_offset(self, chan: int = 0, num: int = 8):
        """Read the list of Preamp Offsets."""
        buff = np.zeros((num,), np.uint16)
        ptr = ffi.from_buffer("uint16_t[]", buff)
        ret_code = lib.ngpd_i2c_read_preamp_offset(self.path, chan, num, ptr)
        if ret_code < 0:
            logging.error(self.get_error_message())

        return buff.tolist()

    def write_dga_gain(self, chan: int, values: list[int]):
        num = len(values)
        ptr = ffi.new("uint16_t[]", values)
        rc = lib.ngzmp_spi_write_dga(self.path, chan, num, ptr)
        if rc < 0:
            logging.error(self.get_error_message())
        return rc

    def read_dga_gain(self, chan: int = 0, num: int = 8):
        """Read the Analog Gain for each selected channel.

        :param chan: the first channel in the returned list
        :param num: the number of channels to return
        """
        buff = np.zeros((num,), np.uint16)
        # ptr = ffi.cast("uint16_t *", ffi.from_buffer(buff))
        ptr = ffi.from_buffer("uint16_t[]", buff)
        rc = lib.ngzmp_spi_read_dga(self.path, chan, num, ptr)
        if rc < 0:
            logging.error(self.get_error_message())
        return buff.tolist()

    def read_basesub(self, chan):
        c_bsub = ffi.new("NGPDBaseSubtract *")
        rc = lib.ngpd_read_baseline_subtract(self.path, chan, c_bsub)
        if rc < 0:
            return None
        bsub = PyNGPDbassub()

        bsub.use_fixed = bool(c_bsub.use_fixed)
        bsub.fixed = c_bsub.fixed
        bsub.error_limit = c_bsub.error_limit
        bsub.div_cont = c_bsub.div_cont
        return bsub

    def write_basesub(self, chan: int, bsub: PyNGPDbassub):
        c_bsub = ffi.new("NGPDBaseSubtract *")
        c_bsub.use_fixed = 1 if bsub.use_fixed else 0
        c_bsub.fixed = bsub.fixed
        c_bsub.error_limit = bsub.error_limit
        c_bsub.div_cont = bsub.div_cont
        rc = lib.ngpd_write_baseline_subtract(self.path, chan, c_bsub)
        return rc

    def read_tail_measure(self, chan: int):
        c_measure = ffi.new("NGPDMeasure *")
        rc = lib.ngpd_read_measure(self.path, chan, c_measure)
        if rc < 0:
            return None

        measure = PyNGPDTailMeasure(
            c_measure.tail_sum_delay,
            c_measure.tail_sum_num,
            c_measure.fall_time_frac,
            bool(c_measure.enable_tail_subtract),
            bool(c_measure.tail_subtract_test),
            bool(c_measure.tail_subtract_test_neutron),
            c_measure.min_height,
            c_measure.max_height,
            bool(c_measure.adaptive_tail_sum),
            c_measure.min_fall_time,
            c_measure.max_fall_time,
            c_measure.min_tail_count,
            bool(c_measure.ignore_fall_time),
            bool(c_measure.ignore_tail_sum),
            c_measure.tail_thres_c[0],
            c_measure.tail_thres_m[0] / 0x800000,
        )

        return measure

    def write_tail_measure(self, chan: int, measure: PyNGPDTailMeasure) -> int:
        c_measure = ffi.new("NGPDMeasure *")
        c_measure.tail_sum_delay = measure.tail_sum_delay
        c_measure.tail_sum_num = measure.tail_sum_sample
        c_measure.fall_time_frac = measure.fall_time_frac
        c_measure.enable_tail_subtract = int(measure.enable_tail_subtract)
        c_measure.tail_subtract_test = int(measure.enable_subtract_test)
        c_measure.tail_subtract_test_neutron = int(measure.enable_subtract_neutron)
        c_measure.min_height = measure.min_height
        c_measure.max_height = measure.max_height
        c_measure.adaptive_tail_sum = int(measure.adaptive_tail_sum)
        c_measure.min_fall_time = measure.min_fall
        c_measure.max_fall_time = measure.max_fall
        c_measure.min_tail_count = measure.min_count
        c_measure.ignore_fall_time = int(measure.ignore_fall_time)
        c_measure.ignore_tail_sum = int(measure.ignore_tail_sum)

        for i in range(lib.NGPD_MEASURE_TAIL_THRES_SIZE):
            c_measure.tail_thres_c[i] = measure.tail_thres_c
            c_measure.tail_thres_m[i] = int(measure.tail_thres_m * 0x800000)

        rc = lib.ngpd_write_measure(self.path, chan, c_measure)
        return rc

    def write_diff_trigger(self, chan, trig: PyNGPDDiffTrigger):
        c_trig = ffi.new("NGPDDiffTrigger *")
        c_trig.thres = trig.thres
        c_trig.sep = trig.sep
        c_trig.data_delay = trig.data_delay
        c_trig.trig_delay = trig.trig_delay
        c_trig.delay_a = trig.delay_a
        c_trig.width_a = trig.width_a
        c_trig.delay_b = trig.delay_b
        c_trig.width_b = trig.width_b
        rc = lib.ngpd_write_diff_trigger(self.path, chan, c_trig)
        return rc

    def read_diff_trigger(self, chan) -> PyNGPDDiffTrigger:
        c_trig = ffi.new("NGPDDiffTrigger *")
        rc = lib.ngpd_read_diff_trigger(self.path, chan, c_trig)
        if rc < 0:
            return None

        trig = PyNGPDDiffTrigger()
        trig.thres = c_trig.thres
        trig.sep = c_trig.sep
        trig.data_delay = c_trig.data_delay
        trig.trig_delay = c_trig.trig_delay
        trig.delay_a = c_trig.delay_a
        trig.width_a = c_trig.width_a
        trig.delay_b = c_trig.delay_b
        trig.width_b = c_trig.width_b
        return trig

    def filter_load_rect(self, chan, num_ave):
        rc = lib.ngpd_filter_load_rect(self.path, chan, num_ave)
        if rc < 0:
            logging.error(self.get_error_message())
        return rc

    def filter_load_exp(self, chan, Tsamples):
        rc = lib.ngpd_filter_load_exp(self.path, chan, Tsamples)
        if rc < 0:
            logging.error(self.get_error_message())
        return rc

    def filter_load_gaus(self, chan, sigma):
        rc = lib.ngpd_filter_load_gaus(self.path, chan, sigma)
        if rc < 0:
            logging.error(self.get_error_message())
        return rc

    def filter_load_trapezoid(self, chan, wid_top, wid_bot):
        rc = lib.ngpd_filter_load_trapezoid(self.path, chan, wid_top, wid_bot)
        if rc < 0:
            logging.error(self.get_error_message())
        return rc

    def get_filter_type(self, chan: int) -> PyNGPDFilter | None:
        c_filter = ffi.new("NGPDFilter *")

        rc = lib.ngpd_get_filter_type(self.path, chan, c_filter)
        if rc < 0:
            logging.error(self.get_error_message())
            return None
        py_filt = PyNGPDFilter()

        py_filt.filt_type = FilterType(c_filter.type)
        py_filt.iarg1 = c_filter.iarg1
        py_filt.iarg2 = c_filter.iarg2
        py_filt.darg = c_filter.darg

        return py_filt

    def save_settings(self, fname):
        fname_bytes = str.encode(fname)
        rc = lib.ngpd_save_settings(self.path, fname_bytes)
        if rc < 0:
            logging.error(self.get_error_message())
        return rc

    def restore_settings(self, fname):
        fname_bytes = str.encode(fname)
        rc = lib.ngpd_restore_settings(self.path, fname_bytes)
        if rc < 0:
            logging.error(self.get_error_message())
        return rc

    def write_chan_cont(self, chan, chan_cont: PyNGPDChanCont):
        c_chan_cont = ffi.new("NGPDChanCont *")
        c_chan_cont.use_pb_start = chan_cont.use_pb_start
        c_chan_cont.data_src = chan_cont.data_src
        c_chan_cont.inv_data = chan_cont.inv_data

        rc = lib.ngpd_write_chan_cont(self.path, chan, c_chan_cont)
        if rc < 0:
            logging.error(self.get_error_message())
        return rc

    def read_chan_cont(self, chan):
        c_chan_cont = ffi.new("NGPDChanCont *")
        rc = lib.ngpd_read_chan_cont(self.path, chan, c_chan_cont)
        if rc < 0:
            logging.error(self.get_error_message())
            return None
        chan_cont = PyNGPDChanCont()
        chan_cont.use_pb_start = c_chan_cont.use_pb_start
        chan_cont.data_src = c_chan_cont.data_src
        chan_cont.inv_data = c_chan_cont.inv_data
        return chan_cont

    def write_hist_conf(self, chan: int, histogram_config: HistogramConfig):
        c_hist_conf = ffi.new("NGZMPHistConf *")

        c_hist_conf.separate_ngp = int(histogram_config.separate_ngp)
        c_hist_conf.enb_hgt_sum = int(histogram_config.enb_hgt_sum)
        c_hist_conf.enb_hgt_fall = int(histogram_config.enb_hgt_fall)
        c_hist_conf.discard_pu = int(histogram_config.discard_pu)
        c_hist_conf.nbits_height = histogram_config.nbits_height
        c_hist_conf.shift_height = histogram_config.shift_height
        c_hist_conf.nbits_fall_time = histogram_config.nbits_fall_time
        c_hist_conf.shift_fall_time = histogram_config.shift_fall_time
        c_hist_conf.nbits_tail_sum = histogram_config.nbits_tail_sum
        c_hist_conf.shift_tail_sum = histogram_config.shift_tail_sum
        rc = lib.ngzmp_hist_write_chan_config(self.path, chan, c_hist_conf)
        if rc < 0:
            logging.error(self.get_error_message())
        # self.hist_conf = hist_conf
        return rc

    def read_hist_conf(self, chan: int):
        hist_conf = ffi.new("NGZMPHistConf *")
        rc = lib.ngzmp_hist_read_chan_config(self.path, chan, hist_conf)
        if rc < 0:
            logging.error(self.get_error_message())
            return None

        hist_config = HistogramConfig(
            bool(hist_conf.separate_ngp),
            bool(hist_conf.enb_hgt_sum),
            bool(hist_conf.enb_hgt_fall),
            bool(hist_conf.discard_pu),
            hist_conf.nbits_height,
            hist_conf.shift_height,
            hist_conf.nbits_fall_time,
            hist_conf.shift_fall_time,
            hist_conf.nbits_tail_sum,
            hist_conf.shift_tail_sum,
        )
        return hist_config

    def setup_adc(self):
        rc = lib.ngzmp_adc_setup_adc(self.path, -1, lib.NGZMPADC_Default)
        if rc < 0:
            logging.error(self.get_error_message())
        lib.ngzmp_adc_read_status(self.path, 0)

    def setup_hist(self, chan: int, hist_conf: HistogramConfig):
        """
        Setup the Histogram Config for the specified channel, overwriting some
        of the values with specific setup
        """

        hist_conf.enb_hgt_sum = 1
        hist_conf.enb_hgt_fall = 0
        hist_conf.discard_pu = 0

        hist_conf.nbits_fall_time = 0
        hist_conf.shift_fall_time = 0

        return self.write_hist_conf(chan, hist_conf)

    def setup_run_mode(self, playback_mode: bool):
        chan_cont = PyNGPDChanCont()
        if playback_mode:
            chan_cont.data_src = lib.NGPDDataSrcPlayback
            run_flags = (
                lib.NGPD_RUN_FLAGS_SCOPEMODE
                | lib.NGPD_RUN_FLAGS_PLAYBACK
                | lib.NGPD_RUN_FLAGS_PLAYBACK_CONTINUOUS
            )
            self.setup_clock(lib.NGZMPClkSrcFPGA)
        else:
            chan_cont.data_src = lib.NGPDDataSrcADCFWD
            chan_cont.inv_data = 1
            run_flags = lib.NGPD_RUN_FLAGS_SCOPEMODE
            self.setup_clock(lib.NGZMPClkSrcLMK61E2)
            self.setup_adc()

        self.write_chan_cont(-1, chan_cont)
        lib.ngpd_set_run_flags(self.path, run_flags)

    def load_playback(self, card, fname):
        fname_bytes = str.encode(fname)
        rc = lib.ngpd_playback_load(self.path, card, fname_bytes, 0)
        if rc < 0:
            logging.error(self.get_error_message())
        return rc

    def clear_hist(self):
        hist_conf = self.read_hist_conf(0)
        num_words = 1 << (hist_conf.nbits_height + hist_conf.nbits_tail_sum)
        # num words is measured in AXI bus 256 bit words, rather than histogram uint32_t
        num_words = int(num_words / 8)
        if hist_conf.separate_ngp:
            num_words = num_words * 3
        lib.ngzmp_hist_clear_all_start(self.path, -1, 0, num_words)
        lib.ngzmp_hist_clear_wait(self.path, -1)

    def start(self, col_time, ncycles=1, setup_scope=True, run_scope=False):
        """Start a Run."""
        self.itfg_setup.col_time = col_time
        self.itfg_setup.trig_mode = lib.NGPDTrigSW
        self.itfg_setup.cycles = ncycles

        c_itfg_setup = ffi.new("NGPDITFGSetup *")
        c_itfg_setup.col_time = self.itfg_setup.col_time
        c_itfg_setup.trig_mode = self.itfg_setup.trig_mode
        c_itfg_setup.cycles = self.itfg_setup.cycles

        save_flags = lib.ngpd_get_run_flags(self.path)
        run_flags = save_flags
        if setup_scope:
            run_flags |= lib.NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS
            run_flags &= ~lib.NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD
        else:
            run_flags |= lib.NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD
            run_flags &= ~lib.NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS
        if run_scope:
            run_flags |= lib.NGPD_RUN_FLAGS_SCOPEMODE
        else:
            run_flags &= ~lib.NGPD_RUN_FLAGS_SCOPEMODE

        lib.ngpd_set_run_flags(self.path, run_flags)

        rc = lib.ngzmp_hist_enable(self.path, -1, lib.NGZMPHistEnbRun)

        rc = lib.ngpd_dma_system_start(self.path, -1, 0, 0, c_itfg_setup)
        lib.ngpd_set_run_flags(self.path, save_flags)
        if rc < 0:
            logging.error(self.get_error_message())
        return rc

    def stop(self):
        rc = lib.ngzmp_system_stop(self.path, -1)
        if rc < 0:
            logging.error(self.get_error_message())
        return rc

    def wait_scope(self):
        rc = lib.ngpd_dma_wait_scope(self.path, -1, ffi.NULL)
        if rc < 0:
            logging.error(self.get_error_message())
        return rc

    def setup_clock(self, clk_src: int):
        cset = ffi.new("NGPDClockSetup *")
        cset.sysref = lib.NGZMPSysRefBurstSPI
        cset.monitor_op = lib.NGZMPClockMonDefault
        cset.src = clk_src
        rc = lib.ngpd_clock_setup(self.path, -1, cset)
        if rc < 0:
            logging.error(self.get_error_message())
        return rc

    def clock_check_locked(self, card):
        rc = lib.ngpd_clock_check_locked(self.path, card)
        if rc < 0:
            logging.error(self.get_error_message())
        return rc

    def wait_itfg(self, quiet=False, ignore_pause=False):
        card = 0
        finished = False
        itfg_status = ffi.new("NGPDITFGStatus *")
        prev_status = -1
        prev_cycles = -1
        prev_timer = -1
        while not finished:
            rc = lib.ngpd_read_itfg_status(self.path, card, itfg_status)
            if rc < 0:
                logging.error(self.get_error_message())
                finished = True
            if not quiet:
                if (
                    itfg_status.flags != prev_status
                    or itfg_status.cycles != prev_cycles
                    or (
                        itfg_status.timer_us != 0
                        and abs(itfg_status.timer_us - prev_timer) > 200000
                    )
                ):
                    if itfg_status.flags & lib.NGPD_SCOPE_STATUS_ITFG_RUNNING:
                        if itfg_status.flags & lib.NGPD_SCOPE_STATUS_ITFG_WAITING:
                            state_string = "Paused"
                        elif itfg_status.flags & lib.NGPD_SCOPE_STATUS_ITFG_COUNTING:
                            state_string = "Counting"
                        else:
                            state_string = "Not Counting"
                    else:
                        state_string = "Idle"
                    logging.debug(
                        f"Timer={itfg_status.timer_us / 1000000.0:.1f} s, Cycles={itfg_status.cycles}, Status=0x{itfg_status.flags:08X}, State={state_string}"
                    )
            prev_status = itfg_status.flags
            prev_cycles = itfg_status.cycles
            if abs(itfg_status.timer_us - prev_timer) > 200000:
                prev_timer = itfg_status.timer_us
            if (itfg_status.flags & lib.NGPD_SCOPE_STATUS_ITFG_RUNNING) == 0:
                finished = True
            if not ignore_pause and (
                itfg_status.flags & lib.NGPD_SCOPE_STATUS_ITFG_WAITING
            ):
                finished = True

    def poll_itfg(self, card=0):
        """Poll the NGPD ITFG for current acquisition state."""
        itfg_status = ffi.new("NGPDITFGStatus *")
        rc = lib.ngpd_read_itfg_status(self.path, card, itfg_status)

        if rc < 0:
            logging.error(self.get_error_message())
            return None
        status = ITFGStatus(
            ScopeStatus(itfg_status.flags),
            itfg_status.cycles,
            itfg_status.timer_us
        )
        
        return status

    def read_histogram(self, chan=0):
        """Read the histogram Data of the specified channel."""
        hist_conf = self.read_hist_conf(chan)
        nbins_hgt = 1 << hist_conf.nbits_height
        nbins_tail_sum = 1 << hist_conf.nbits_tail_sum
        nbins_ngp = 1
        if hist_conf.separate_ngp:
            nbins_ngp = 3

        buff = np.zeros((nbins_ngp, nbins_tail_sum, nbins_hgt), np.uint32)
        ptr = ffi.from_buffer("uint32_t[]", buff)

        rc = lib.ngzmp_hist_read_chan(
            self.path, chan, 0, nbins_hgt * nbins_tail_sum * nbins_ngp, ptr
        )
        if rc < 0:
            logging.error(self.get_error_message())
        return buff

    def read_scope(self, card, stream, t, dt):
        num_dig = ffi.new("int *")
        bit_posn = ffi.new("int *")
        dig_stream = ffi.new("int *")

        stream_type = lib.ngpd_scope_stream_details_path(
            self.path,
            card,
            stream,
            num_dig,
            bit_posn,
            dig_stream,
            ffi.NULL,
            ffi.NULL,
            ffi.NULL,
            ffi.NULL,
            ffi.NULL,
        )

        if stream_type == lib.NGPDScopeStream_Signed:
            buff = np.zeros((dt,), np.int16)
            ptr = ffi.from_buffer("int16_t[]", buff)
        else:
            buff = np.zeros((dt,), np.uint16)
            ptr = ffi.from_buffer("uint16_t[]", buff)

        rc = lib.ngpd_scope_read_to_buff(self.path, card, stream, t, dt, ptr)
        if rc < 0:
            logging.error(self.get_error_message())
        return buff

    def read_adc_temp(self, card=0) -> float:
        """Read the temperatures of the ADCs on the specified card."""
        temps = ffi.new("float[]", lib.NGZMP_I2C_NUM_ADT7410_ADC)
        rc = lib.ngpd_i2c_read_adc_temp(self.path, card, temps, ffi.NULL)
        if rc < 0:
            raise NgpdLibException(self.get_error_message())
        return max(temps)

    def read_preamp_temp(self, card=0) -> float:
        """Read the temperatures of the preamps on the specified card."""
        temps = ffi.new("float[]", lib.NGZMP_I2C_NUM_ADT7410_PREAMP)
        rc = lib.ngpd_i2c_read_preamp_temp(self.path, card, temps, ffi.NULL)
        if rc < 0:
            raise NgpdLibException(self.get_error_message())
        return max(temps)

    def read_adc_voltages(self, card=0) -> dict[str, float]:
        dict = {}
        for chip in range(lib.NGZMP_UCD90160_NUM_CHIPS):
            chip_read = ffi.new("int32_t[]", lib.NGZMP_UCD90160_NUM_RAILS)
            rc = lib.ngzmp_i2c_read_ucd90160_vout(
                self.path, card, chip, 0, lib.NGZMP_UCD90160_NUM_RAILS, chip_read
            )
            if rc < 0:
                raise NgpdLibException(self.get_error_message())
            for i, val in enumerate(chip_read):
                label = VOLTAGE_SIGNAL_NAMES[chip][i]
                if label:
                    dict[label] = (
                        val * 0.0001
                    )  # display as volts rather than millivolts
        return dict

    def read_fpga_data(self, card=0) -> SystemMonitor:
        data = ffi.new("int32_t[]", lib.NGZMP_ParamMax)
        rc = lib.ngzmp_read_xadc(self.path, card, 0, lib.NGZMP_ParamMax, data)

        if rc < 0:
            raise NgpdLibException(self.get_error_message())
        monitor = SystemMonitor(*data)
        return monitor

    def read_adc_tcrit(self, card=0) -> int:
        if self.dummy & DummyLevel.ADC:
            # we're simulating the ADCs so can't actually read the value
            return -1
        data = ffi.new("uint8_t[]", 2)
        rc = lib.ngzmp_i2c_read_reg_addr(
            self.path,
            card,
            lib.NGZMP_I2C_BUS_PMBUS,
            0x48,  # HARDCODING FOR NOW CAUSE #DEFINE FUNC NOT WORK WITH CFFi
            lib.ADT7410_TCRIT_MSB,
            2,
            data,
        )
        if rc < 0:
            raise NgpdLibException(self.get_error_message())
        tcrit = (data[0] << 8) | data[1]
        return int(tcrit / 128)

    def read_preamp_tcrit(self, card=0) -> int:
        if self.dummy & DummyLevel.PREAMP:
            # we're simulating the preamps so can't actually read the value.
            return -1
        data = ffi.new("uint8_t[]", 2)
        rc = lib.ngzmp_i2c_read_reg_addr(
            self.path,
            card,
            lib.NGZMP_I2C_BUS_PREAMP,
            0x48,
            lib.ADT7410_TCRIT_MSB,
            2,
            data,
        )
        if rc < 0:
            raise NgpdLibException(self.get_error_message())
        tcrit = (data[0] << 8) | [data[1]]
        return int(tcrit / 128)

    def write_adc_tcrit(self, value: int, card=0) -> int:

        rc = lib.ngpd_i2c_write_adc_tcrit(self.path, card, -1, value)
        if rc < 0:
            raise NgpdLibException(self.get_error_message())
        return rc

    def write_preamp_tcrit(self, value: int, card=0) -> int:
        rc = lib.ngpd_i2c_write_preamp_tcrit(self.path, card, -1, value)
        if rc < 0:
            raise NgpdLibException(self.get_error_message())
        return rc

    def cal_offsets(self, first, last, target, num_pass, adjust_only, fname):
        if fname != "":
            fname_bytes = str.encode(fname)
            lib.ngpd_cal_offsets(
                self.path,
                ffi.NULL,
                ffi.NULL,
                first,
                last,
                target,
                num_pass,
                adjust_only,
                fname_bytes,
            )
        else:
            lib.ngpd_cal_offsets(
                self.path,
                ffi.NULL,
                ffi.NULL,
                first,
                last,
                target,
                num_pass,
                adjust_only,
                ffi.NULL,
            )
