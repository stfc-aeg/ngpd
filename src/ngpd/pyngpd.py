from ngpd import cffi_lib as lib
from ngpd.cffi_lib import ffi
from dataclasses import dataclass
from enum import IntEnum
import logging
import numpy as np

from odin_control.adapters.base_controller import BaseError


class NGPDLibError(Exception):
    """Exception class for NGPD CFFI Lib Errors"""


@dataclass
class PyNGPDbassub:
    use_fixed: int = 0
    fixed: int = 0
    error_limit: int = 0
    div_cont: int = 0


@dataclass
class PyNGPDDiffTrigger:
    thres: int = 1
    sep: int = 2
    data_delay: int = 0
    trig_delay: int = 0
    delay_a: int = 0
    width_a: int = 1
    delay_b: int = 0
    width_b: int = 1


@dataclass
class PyNGPDFilter:
    filt_type: int = 0
    iarg1: int = 1
    iarg2: int = 2
    darg: float = 1.0


@dataclass
class PyNGPDChanCont:
    use_pb_start: int = 0
    data_src: int = 0
    inv_data: int = 0


class DummyLevel(IntEnum):
    """Defines level of Dummy parts to the system, IE how much to simulate"""
    NONE = 0
    FPGA = 1
    ADC = 2,
    PREAMP = 4,
    ALL = 0xFF


class PyNgpd:
    """Python class handing all NGPD config and Access"""

    def __init__(self, ipaddr: str, num_cards=1, dummy=DummyLevel.NONE, first_card=0):
        self.num_cards = num_cards
        self.first_crad = first_card
        self.dummy = dummy
        c_ip_addr = ffi.new("char[]", ipaddr.encode())

        self.path = lib.ngpd_config_ngzmp(num_cards, c_ip_addr, -1, ffi.NULL, -1, 0,
                                          first_card, 0, dummy)

        if self.path < 0:
            raise NGPDLibError("Cannot open path to NGPD Board")

        self.num_chan = lib.ngpd_get_num_chan(self.path)
        self.itfg_setup = ffi.new("NGPDITFGSetup *")
        self.itfg_setup.col_time = 1
        self.itfg_setup.trig_mode = lib.NGPDTrigSW
        self.itfg_setup.cycles = 1
        logging.debug("Completed NGPD Init")

    def get_error_message(self) -> str:
        return ffi.string(lib.ngpd_get_error_message())

    def write_preamp_offset(self, chan, values):
        num = len(values)
        ptr = ffi.from_buffer("uint16_t[]", values)
        ret_code = lib.ngpd_i2c_write_preamp_offset(self.path, chan, num, ptr)
        return ret_code

    def read_preamp_offset(self, chan, num):
        buff = np.zeros((num,), np.uint16)
        ptr = ffi.from_buffer("uint16_t[]", buff)
        ret_code = lib.ngpd_i2c_read_preamp_offset(self.path, chan, num, ptr)
        if ret_code < 0:
            logging.error(self.get_error_message())

        return buff

    def write_dga_gain(self, chan, values):
        num = len(values)
        ptr = ffi.from_buffer("uint16_t[]", values)
        rc = lib.ngzmp_spi_write_dga(self.path, chan, num, ptr)
        return rc

    def read_dga_gain(self, chan, num):
        buff = np.zeros((num, ), np.uint16)
        # ptr = ffi.cast("uint16_t *", ffi.from_buffer(buff))
        ptr = ffi.from_buffer("uint16_t[]", buff)
        rc = lib.ngzmp_spi_read_dga(self.path, chan, num, ptr)
        if rc < 0:
            logging.error(self.get_error_message())
        return buff

    def read_basesub(self, chan) -> PyNGPDbassub:
        c_bsub = ffi.new("NGPDBaseSubtract *")
        rc = lib.ngpd_read_baseline_subtract(self.path, chan, c_bsub)
        if rc < 0:
            return None
        bsub = PyNGPDbassub()

        bsub.use_fixed = c_bsub.use_fixed
        bsub.fixed = c_bsub.fixed
        bsub.error_limit = c_bsub.error_limit
        bsub.div_cont = c_bsub.div_cont
        return bsub

    def write_basesub(self, chan, bsub: PyNGPDbassub):
        c_bsub = ffi.new("NGPDBaseSubtract *")
        c_bsub.use_fixed = bsub.use_fixed
        c_bsub.fixed = bsub.fixed
        c_bsub.error_limit = bsub.error_limit
        c_bsub.div_cont = bsub.div_cont
        rc = lib.ngpd_write_baseline_subtract(self.path, chan, c_bsub)
        return rc

    def read_tail_measure(self, chan):
        measure = ffi.new("NGPDMeasure *")
        rc = lib.ngpd_read_measure(self.path, chan, measure)
        if rc < 0:
            return None
        return measure

    def writemeasure(self, chan, measure):
        rc = lib.ngpd_write_measure(self.path, chan, measure)
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

    def get_filter_type(self, chan) -> PyNGPDFilter:
        c_filter = ffi.new("NGPDFilter *")

        rc = lib.ngpd_get_filter_type(self.path, chan, c_filter)
        if rc < 0:
            logging.error(self.get_error_message())
            return None
        py_filt = PyNGPDFilter()

        py_filt.filt_type = c_filter.type
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

    def write_chan_cont(self, chan, chan_cont):
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

    def write_hist_conf(self, chan, hist_conf):
        rc = lib.ngzmp_hist_write_chan_config(self.path, chan, hist_conf)
        if rc < 0:
            logging.error(self.get_error_message())
        # self.hist_conf = hist_conf
        return rc

    def read_hist_conf(self, chan):
        hist_conf = ffi.new("NGZMPHistConf *")
        rc = lib.ngzmp_hist_read_chan_config(self.path, chan, hist_conf)
        if rc < 0:
            logging.error(self.get_error_message())
            return None

        return hist_conf

    def setup_adc(self):
        rc = lib.ngzmp_adc_setup_adc(self.path, -1, lib.NGZMPADC_Default)
        if rc < 0:
            logging.error(self.get_error_message())
        lib.ngzmp_adc_read_status(self.path, 0)

    def setup_hist(self, chan, nbits_height, nbits_tail_sum,
                   shift_height=-1, shift_tail_sum=-1, separate_ngp=-1):
        new_hist_conf = ffi.new("NGZMPHistConf *")

        new_hist_conf.enb_hgt_sum = 1
        new_hist_conf.enb_hgt_fall = 0
        new_hist_conf.discard_pu = 0
        new_hist_conf.nbits_height = nbits_height

        new_hist_conf.nbits_fall_time = 0
        new_hist_conf.shift_fall_time = 0
        new_hist_conf.nbits_tail_sum = nbits_tail_sum

        if shift_height == -1:
            new_hist_conf.shift_height = 16-nbits_height
        else:
            new_hist_conf.shift_height = shift_height
        if shift_tail_sum == -1:
            new_hist_conf.shift_tail_sum = 19-nbits_tail_sum
        else:
            new_hist_conf.shift_tail_sum = shift_tail_sum
        if separate_ngp == -1:
            new_hist_conf.separate_ngp = 0
        else:
            new_hist_conf.separate_ngp = separate_ngp

        logging.debug("setup_hist: shift_height, shift_tail_sum",
                      new_hist_conf.shift_height, new_hist_conf.shift_tail_sum)
        return self.write_hist_conf(chan, new_hist_conf)

    def setup_run_mode(self, playback_mode):
        chan_cont = PyNGPDChanCont()
        if playback_mode:
            chan_cont.data_src = lib.NGPDDataSrcPlayback
            run_flags = (lib.NGPD_RUN_FLAGS_SCOPEMODE |
                         lib.NGPD_RUN_FLAGS_PLAYBACK |
                         lib.NGPD_RUN_FLAGS_PLAYBACK_CONTINUOUS)
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
        num_words = 1 << (hist_conf.nbits_height+hist_conf.nbits_tail_sum)
        # num words is measured in AXI bus 256 bit words, rather than histogram uint32_t
        num_words = int(num_words/8)
        if hist_conf.separate_ngp:
            num_words = num_words*3
        lib.ngzmp_hist_clear_all_start(self.path, -1, 0, num_words)
        lib.ngzmp_hist_clear_wait(self.path, -1)

    def start(self, col_time, ncycles=1, setup_scope=True, run_scope=False):
        self.itfg_setup.col_time = col_time
        self.itfg_setup.trig_mode = lib.NGPDTrigSW
        self.itfg_setup.cycles = ncycles

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

        rc = lib.ngpd_dma_system_start(self.path, -1, 0, 0, self.itfg_setup)
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

    def setup_clock(self, clk_src):
        if self.dummy & lib.NGPDDummy_ADC:
            clk_src = lib.NGZMPClkSrcFPGA

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
                if itfg_status.flags != prev_status or itfg_status.cycles != prev_cycles or (itfg_status.timer_us != 0 and abs(itfg_status.timer_us - prev_timer) > 200000):
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
                        f"Timer={itfg_status.timer_us/1000000.0:.1f} s, Cycles={itfg_status.cycles}, Status=0x{itfg_status.flags:08X}, State={state_string}")
            prev_status = itfg_status.flags
            prev_cycles = itfg_status.cycles
            if abs(itfg_status.timer_us - prev_timer) > 200000:
                prev_timer = itfg_status.timer_us
            if (itfg_status.flags & lib.NGPD_SCOPE_STATUS_ITFG_RUNNING) == 0:
                finished = True
            if not ignore_pause and (itfg_status.flags & lib.NGPD_SCOPE_STATUS_ITFG_WAITING):
                finished = True

    def poll_itfg(self, quiet=False):
        card = 0
        done = 0.0
        total = self.itfg_setup.cycles*self.itfg_setup.col_time
        paused = False
        itfg_status = ffi.new("NGPDITFGStatus *")
        rc = lib.ngpd_read_itfg_status(self.path, card, itfg_status)
        if rc < 0:
            logging.error(self.get_error_message())
            return ("ERROR", -1, -1)

        if itfg_status.flags & lib.NGPD_SCOPE_STATUS_ITFG_RUNNING:
            if itfg_status.flags & lib.NGPD_SCOPE_STATUS_ITFG_WAITING:
                state_string = "Paused"
            elif itfg_status.flags & lib.NGPD_SCOPE_STATUS_ITFG_COUNTING:
                state_string = "Counting"
            else:
                state_string = "Not Counting"
        else:
            state_string = "Idle"
        if not quiet:
            logging.debug(
                f"Timer={itfg_status.timer_us} s, Cycles={itfg_status.cycles}, Status=0x{itfg_status.flags:08X}, State={state_string}")
            # logging.debug (f"Timer={itfg_status.timer_us/1000000.0:.1f} s, Cycles={itfg_status.cycles}, Status=0x{itfg_status.flags:08X}, State={state_string}")
        done = self.itfg_setup.col_time-itfg_status.timer_us/1000000.0 + \
            (self.itfg_setup.cycles-itfg_status.cycles)*self.itfg_setup.col_time
        return (state_string, done, total)

    def read_histogram(self, chan):
        hist_conf = self.read_hist_conf(chan)
        nbins_hgt = 1 << hist_conf.nbits_height
        nbins_tail_sum = 1 << hist_conf.nbits_tail_sum
        nbins_ngp = 1
        if hist_conf.separate_ngp:
            nbins_ngp = 3

        buff = np.zeros((nbins_ngp, nbins_tail_sum, nbins_hgt), np.uint32)
        ptr = ffi.from_buffer("uint32_t[]", buff)

        rc = lib.ngzmp_hist_read_chan(self.path, chan, 0, nbins_hgt*nbins_tail_sum*nbins_ngp, ptr)
        if rc < 0:
            logging.error(self.get_error_message())
        return buff

    def read_scope(self, card, stream, t, dt):
        num_dig = ffi.new("int *")
        bit_posn = ffi.new("int *")
        dig_stream = ffi.new("int *")

        stream_type = lib.ngpd_scope_stream_details_path(
            self.path, card, stream, num_dig, bit_posn, dig_stream, ffi.NULL, ffi.NULL, ffi.NULL, ffi.NULL, ffi.NULL)

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

    def read_adc_temp(self):
        temps = np.zeros((self.nCards*lib.NGZMP_I2C_NUM_ADT7410_ADC, ), np.float64)
        max_temp = -200
        one_card = ffi.new("float [4]")
        status = ffi.new("int [4]")
        for card in range(self.nCards):
            logging.debug("Reading temperature from card ", card)
            rc = lib.ngpd_i2c_read_adc_temp(self.path, card, one_card, status)
            logging.debug(" ... rc=", rc)
            if rc < 0:
                logging.error(self.get_error_message())
                return None
            for i in range(lib.NGZMP_I2C_NUM_ADT7410_ADC):
                temps[lib.NGZMP_I2C_NUM_ADT7410_ADC*card+i] = one_card[i]
        return temps

    def cal_offsets(self, first, last, target, num_pass, adjust_only, fname):
        if fname != "":
            fname_bytes = str.encode(fname)
            lib.ngpd_cal_offsets(self.path,  ffi.NULL, ffi.NULL, first, last,
                                 target, num_pass, adjust_only, fname_bytes)
        else:
            lib.ngpd_cal_offsets(self.path,  ffi.NULL, ffi.NULL, first, last,
                                 target, num_pass, adjust_only, ffi.NULL)
