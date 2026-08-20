"""
NGPD C library
Python Interface built using CFFI
"""
from typing import Final

NGPDTrigSW: Final[int]
NGPDDataSrcPlayback: Final[int]
NGPDDataSrcADCFWD: Final[int]

NGPD_RUN_FLAGS_SCOPEMODE: Final[int]
NGPD_RUN_FLAGS_PLAYBACK: Final[int]
NGPD_RUN_FLAGS_PLAYBACK_CONTINUOUS: Final[int]

NGZMPClkSrcFPGA: Final[int]
NGZMPClkSrcLMK61E2: Final[int]

NGZMP_I2C_NUM_ADT7410_ADC: Final[int]
"""Number of ADT7410 temperature monitor chips on PCB."""

NGZMP_I2C_NUM_ADT7410_PREAMP: Final[int]
"""Number of ADT7410 temperature monitor chips on PCB"""

NGPD_MEASURE_TAIL_THRES_SIZE: Final[int]

NGPD_MEASURE_SUM_DELAY_MAX: Final[int]
NGPD_MEASURE_SUM_NUM_MAX: Final[int]
NGPD_MEASURE_HEIGHT_MAX: Final[int]
NGPD_MEASURE_FALL_TIME_MAX: Final[int]
NGPD_MEASURE_TAIL_COUNT_MAX: Final[int]

NGPD_DIFF_TRIG_MIN_SEP: Final[int]
NGPD_DIFF_TRIG_MAX_SEP: Final[int]
NGPD_DIFF_TRIG_MIN_DATA_DELAY: Final[int]
NGPD_DIFF_TRIG_MAX_DATA_DELAY: Final[int]
NGPD_DIFF_TRIG_MIN_TRIG_DELAY: Final[int]
NGPD_DIFF_TRIG_MAX_TRIG_DELAY: Final[int]
NGPD_DIFF_TRIG_MIN_DELAY_AB: Final[int]
NGPD_DIFF_TRIG_MAX_DELAY_AB: Final[int]
NGPD_DIFF_TRIG_MIN_TRIG_STRETCH: Final[int]
NGPD_DIFF_TRIG_MAX_TRIG_STRETCH: Final[int]
NGPD_DIFF_TRIG_MIN_THRES: Final[int]
NGPD_DIFF_TRIG_MAX_THRES: Final[int]


def ngpd_config_ngzmp(ncards: int, baseIPaddress: str,
                      basePort: int, baseMACaddress: str, num_chan: int,
                      debug: int, card_index: int, do_init: int, dummy_system: int) -> int:
    """Configure connection to NGPD Card"""


def ngpd_get_num_chan(path: int) -> int:
    """returns the total number of channels for this configured system"""


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


def ngpd_read_measure(path: int, chan: int, measure):
    """"""


def ngpd_write_measure(path: int, chan: int, measure):
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


def ngpd_i2c_read_adc_temp(path: int, card: int, temp: list[float], status: list[int]):
    """Read the temperatures from the available sensors on the ADC chips"""


def ngpd_i2c_read_preamp_temp(path: int, card: int, temp: list[float], status: list[int]):
    """Read the temperatures from the available sensors on the preamps"""