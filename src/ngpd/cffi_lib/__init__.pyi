"""
NGPD C library
Python Interface built using CFFI
"""

NGPDTrigSW: int
NGPDDataSrcPlayback: int
NGPDDataSrcADCFWD: int

NGPD_RUN_FLAGS_SCOPEMODE: int
NGPD_RUN_FLAGS_PLAYBACK: int
NGPD_RUN_FLAGS_PLAYBACK_CONTINUOUS: int

NGZMPClkSrcFPGA: int
NGZMPClkSrcLMK61E2: int


def ngpd_config_ngzmp(ncards: int, baseIPaddress: str,
                      basePort: int, baseMACaddress: str, num_chan: int,
                      debug: int, card_index: int, do_init: int, dummy_system: int) -> int:
    """Configure connection to NGPD Card"""


def ngpd_get_error_message() -> str:
    """Get latest error message"""


def ngpd_i2c_write_preamp_offset(path: int, chan: int, num: int, data: memoryview) -> int:
    """Write values into the ADC Preamp"""


def ngpd_i2c_read_preamp_offset(path: int, chan: int, num: int, data: memoryview) -> int:
    """"""


def ngzmp_spi_write_dga():
    """"""


def ngzmp_spi_read_dga():
    """"""


def ngpd_read_baseline_subtract():
    """"""


def ngpd_write_baseline_subtract():
    """"""


def ngpd_read_measure():
    """"""


def ngpd_write_measure():
    """"""


def ngpd_write_diff_trigger():
    """"""


def ngpd_read_diff_trigger():
    """"""


def ngpg_filter_load_rect():
    """"""


def ngpd_filter_load_exp():
    """"""


def ngpd_filter_load_gaus():
    """"""


def ngpd_filter_load_trapezoid():
    """"""


def ngpd_get_filter_type():
    """"""


def ngpd_save_settings():
    """"""


def ngpd_restore_settings():
    """"""


def ngpd_write_chan_cont():
    """"""


def ngpd_read_chan_cont():
    """"""


def ngzmp_hist_write_chan_config():
    """"""


def ngzmp_hist_read_chan_config():
    """"""


def ngzmp_adc_setup_adc():
    """"""


def ngzmp_adc_read_status():
    """"""
