"""Acquisition Class module."""

import base64
import logging
from ngpd.device.pyngpd import PyNgpd, ITFGStatus, ScopeStatus
from ngpd.util import UsesNgpdLibrary
from tornado.ioloop import PeriodicCallback
from typing import Literal
import numpy as np
from numpy import ndarray


Acq_State = Literal["error", "paused", "counting", "running", "idle"]


class NgpdAcquisition:
    """Acquisition handling class.
    
    Handles the running and stopping of an acquisition, as well as scope mode enabling.
    """

    def __init__(self):
        """Init the class."""
        self.num_cyles = 1
        self.frame_length: float = 1.0
        self.setup_scope = False
        self.run_scope = False
        self.ngpd: PyNgpd = None

        self._state: ITFGStatus = None

        self.acq_state: Acq_State = "idle"
        self._total = -1
        self.done = -1

        self.monitorCallback = PeriodicCallback(
            self.acqMonitoring, 100
        )

    def configure(self, ngpd: PyNgpd):
        """Configure the Acquisiton Handler by giving it access to the ngpd object."""
        self.ngpd = ngpd

    @property
    def status(self):
        """Status of the Internal Time Frame Generator."""
        if self.ngpd:
            logging.debug("POLLING ITFG")
            self._state = self.ngpd.poll_itfg()
        return ITFGStatus() if self._state is None else self._state

    @property
    def total(self):
        """Total requested Acquisition time or frames."""
        self._total = self.num_cyles * self.frame_length
        return self._total

    @UsesNgpdLibrary
    def start(self):
        """Start the Acquisition."""
        logging.debug("Starting Acquisition")
        self.ngpd.clear_hist()
        self.ngpd.start(self.frame_length, self.num_cyles, self.setup_scope, self.run_scope)
        self.monitorCallback.start()

    @UsesNgpdLibrary
    def stop(self):
        """Stop the Acquisition."""
        logging.debug("Stopping Acquisition")
        self.ngpd.stop()
        self.monitorCallback.stop()

    @UsesNgpdLibrary
    def acqMonitoring(self):
        """Monitor the active acquisition."""
        col_time = self.frame_length
        cycles = self.num_cyles

        status = self.ngpd.poll_itfg()

        state: Acq_State = "error" if status.flags == ScopeStatus.ERROR else "idle"

        if status.flags & ScopeStatus.ITFG_RUNNING:
            if status.flags & ScopeStatus.ITFG_WAITING:
                state = "paused"
            elif status.flags & ScopeStatus.ITFG_COUNTING:
                state = "counting"
            else:
                state = "running"

        done = col_time - (status.timer_us/1000000.0) + (cycles - status.cycles) * col_time

        self.acq_state = state
        self.done = done

        if state == "error" or state == "idle":
            logging.debug("Acquisition Complete")
            self.monitorCallback.stop()


class NgpdData:
    """Class for the handling and rendering of data from a completed acquisition."""

    def __init__(self):
        """Create the Data handing class."""
        self.ngpd: PyNgpd = None
        self.data_shape = (3, 1024, 1024)
        self.data = np.zeros(self.data_shape, np.uint32)

    def configure(self, ngpd: PyNgpd):
        """Configure the data handler to use the ngpd class object."""
        self.ngpd = ngpd

    @UsesNgpdLibrary
    def refresh_data(self):
        """Read the histogram data from the specified channel."""
        # data  = [self.ngpd.read_histogram(i) for i in range(8)]

        # logging.debug(f"Data Shape: {data[0].shape}")
        self.data = self.ngpd.read_histogram(0)
        logging.debug(f"Data Shape: {self.data.shape}")
        self.data_shape = self.data.shape

    def get_data(self):
        """Get Data as Base64 encoded string, for better HTTP transfer."""
        encoded = base64.b64encode(np.ascontiguousarray(self.data))
        return encoded.decode("utf-8")