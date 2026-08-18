from ngpd.pyngpd import PyNgpd, DummyLevel
from ngpd.pyngpd import PyNGPDFilter, PyNGPDDiffTrigger, PyNGPDbassub, PyNGPDTailMeasure
from ngpd.util import UsesNgpdLibrary, NgpdLibException
from ipaddress import ip_address
from typing import Literal
from math import log2
from functools import partial
import logging


class NgpdDevice:
    """
    NGPD Handling class, providing a bridge between Adapter/Controller
    and the interface class of PyNgpd
    """

    def __init__(self, options: dict[str, str]):
        self.ip = ip_address(options.get("ip_addr", "192.168.0.1"))
        self.num_cards = int(options.get("num_cards", 1))
        self.dummy_level = DummyLevel[options.get("dummy_level", "none").upper()]

        self.ngpd: PyNgpd = None

        self.adc_temps = [0] * 4
        self.preamp_temps = [0] * 2

        self._analog_gain = [0] * 8
        self._analog_offset = [0] * 8

        self.channels: list[NgpdChannel] = []
        for i in range(8):
            # 8 is assumed number of channels. Can redo when configured?
            self.channels.append(NgpdChannel(i))

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
            }
        }

    def configure(self):
        # we have to subtract 1 from the Ip Addr due to logic in William's code,
        # which adds the one back on when initialising
        logging.debug("Configuring Ngpd Device")
        self.ngpd = PyNgpd(str(self.ip - 1), self.num_cards, self.dummy_level)
        self.ngpd.setup_run_mode(self.dummy_level & DummyLevel.ADC)

        # re-create channel list with actual number of channels
        # self.channels = [NgpdChannel(i) for i in range(self.ngpd.num_chan)]
        for channel in self.channels:
            channel.configure(self.ngpd)

    @property
    def gain(self):
        if self.ngpd:
            self._analog_gain = self.ngpd.read_dga_gain(0, self.ngpd.num_chan)
        return self._analog_gain

    @UsesNgpdLibrary
    def set_gain(self, values: list[int]):
        """Set the Gain for every channel"""
        self.ngpd.write_dga_gain(0, values)

    @property
    def offset(self):
        if self.ngpd:
            self._analog_offset = self.ngpd.read_preamp_offset(0, self.ngpd.num_chan)
        return self._analog_offset

    @UsesNgpdLibrary
    def set_offset(self, values: list[int]):
        self.ngpd.write_preamp_offset(0, values)

    @UsesNgpdLibrary()
    def read_adc_temps(self):
        self.adc_temps = self.ngpd.read_adc_temp()
        return self.adc_temps

    @UsesNgpdLibrary()
    def read_preamp_temps(self):
        self.preamp_temps = self.ngpd.read_preamp_temp()
        return self.preamp_temps


class NgpdChannel:
    """Class to hold the read/write info for each individual channel"""

    MAX_GAIN = 35
    MAX_OFFSET = 65535

    MIN_BSUB_FIXED = -65536
    MAX_BSUB_FIXED = 65535

    MAX_ERROR_LIMIT = 65535

    MIN_DIV_CONT = 64
    NUM_DIV_CONT = 8

    def __init__(self, chan_num: int):

        self.ngpd: PyNgpd = None
        self.chan_num = chan_num
        self._gain = 0
        self._offset = 0

        self._filter = PyNGPDFilter()
        self._trigger = PyNGPDDiffTrigger()
        self._base_sub: PyNGPDbassub = None
        self._tail_measure = PyNGPDTailMeasure()

        self.tree = {
            "gain": (lambda: self.gain, self.set_gain,
                     {"description": "Analog Input Gain",
                      "min": 0, "max": self.MAX_GAIN}),
            "offset": (lambda: self.offset, self.set_offset,
                       {"description": "Analog Input Offset",
                        "min": 0, "max": self.MAX_OFFSET}),
            "baseline_subtract": {
                "use_fixed": (
                    lambda: self.base_sub.use_fixed,
                    partial(self.set_base_sub, "use_fixed"),
                    {"description": "Toggle to control if the base subtration uses a fixed value"}
                ),
                "fixed": (
                    lambda: self.base_sub.fixed,
                    partial(self.set_base_sub, "fixed"),
                    {"description": "Fixed Value for base subtraction, if in use",
                     "min": self.MIN_BSUB_FIXED, "max": self.MAX_BSUB_FIXED}
                ),
                "error_limit": (
                    lambda: self.base_sub.error_limit,
                    partial(self.set_base_sub, "error_limit"),
                    {"description": "Baseline Subtraction Error Limit",
                     "min": 0, "max": self.MAX_ERROR_LIMIT}
                ),
                "div_cont": (
                    lambda: self.MIN_DIV_CONT * (2 ** self.base_sub.div_cont),
                    partial(self.set_base_sub, "div_cont"),
                    {"allowed_values": [self.MIN_DIV_CONT * (2 ** i)
                                        for i in range(self.NUM_DIV_CONT)]}
                )
            }
        }

    def configure(self, ngpd: PyNgpd):
        self.ngpd = ngpd

    @property
    def gain(self):
        if self.ngpd:
            self._gain = self.ngpd.read_dga_gain(self.chan_num, 1)[0]
        return self._gain

    @UsesNgpdLibrary()
    def set_gain(self, value: int):
        if not (self.MAX_GAIN >= value >= 0):
            raise NgpdLibException(
                f"Gain Value for channel {self.chan_num} out of range 0 to {self.MAX_GAIN}"
            )
        self.ngpd.write_dga_gain(self.chan_num, [value])

    @property
    def offset(self):
        if self.ngpd:
            self._offset = self.ngpd.read_preamp_offset(self.chan_num, 1)[0]
        return self._offset

    @UsesNgpdLibrary()
    def set_offset(self, value: int):
        if not (self.MAX_OFFSET >= value >= 0):
            raise NgpdLibException(
                f"Offset Value for channel {self.chan_num} out of range 0 to {self.MAX_OFFSET}"
            )
        self.ngpd.write_preamp_offset(self.chan_num, [value])

    @property
    def base_sub(self):
        if self.ngpd and self._base_sub is None:
            logging.debug(f"Reading Base Sub Struct for channel {self.chan_num}")
            sub = self.ngpd.read_basesub(self.chan_num)
            if sub is not None:
                self._base_sub = sub
        if self._base_sub is None:
            return PyNGPDbassub()
        return self._base_sub

    @UsesNgpdLibrary()
    def set_base_sub(self, setting: Literal["use_fixed", "fixed", "error_limit", "div_cont"],
                     value: int | bool):

        if setting == "use_fixed":
            self.base_sub.use_fixed = value
        elif setting == "error_limit":
            self.base_sub.error_limit = value
        elif setting == "fixed":
            self.base_sub.fixed = value
        elif setting == "div_cont":
            logging.debug(f"DIV CONT: {value}")
            # invert the maths turning the div_count index (0-7) into the real number
            self.base_sub.div_cont = int(log2(value / self.MIN_DIV_CONT))
        self.ngpd.write_basesub(self.chan_num, self.base_sub)
