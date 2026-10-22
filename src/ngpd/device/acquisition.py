"""Acquisition Class module."""

import base64
import logging
import h5py
from numpy.typing import NDArray
from ngpd.device.pyngpd import PyNgpd, ITFGStatus, ScopeStatus
from ngpd.util import UsesNgpdLibrary
from tornado.ioloop import PeriodicCallback
from typing import Literal
import numpy as np
from enum import IntEnum


Acq_State = Literal["error", "paused", "counting", "running", "idle"]


class HistDisplay(IntEnum):
    """Simple Enum for selecting hist channel."""

    neutron = 0
    gamma = 1
    pileup = 2
    all = 3


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

        self.monitorCallback = PeriodicCallback(self.acqMonitoring, 100)

        self.tree = {
            "num_cycles": (
                lambda: self.num_cyles,
                lambda v: setattr(self, "num_cycles", v),
                {"description": "Number of Cycles for the Acquisition.", "min": 1},
            ),
            "frame_length": (
                lambda: self.frame_length,
                lambda v: setattr(self, "frame_length", v),
                {"description": "Seconds per each Acquisition Cycle", "min": 1},
            ),
            "scope_setup": (
                lambda: self.setup_scope,
                lambda v: setattr(self, "setup_scope", v),
                {"description": "Enable to setup Scope Mode when starting the run."},
            ),
            "scope_run": (
                lambda: self.run_scope,
                lambda v: setattr(self, "run_scope", v),
                {"description": "Run the NGPD system in Scope Mode"},
            ),
            "run": (
                None,
                self.set_run,
                {"description": "Start or stop the acquisition"},
            ),
            "state": {
                "total": (
                    lambda: self.total,
                    None,
                    {"description": "Total Frames requested for this acquisition"},
                ),
                "current": (
                    lambda: self.done,
                    None,
                    {
                        "description": "Number of frames currently processed by this acquisition"
                    },
                ),
                "status": (
                    lambda: self.acq_state,
                    None,
                    {
                        "description": "ITFG Status",
                        "allowed_values": [
                            "error",
                            "paused",
                            "counting",
                            "running",
                            "idle",
                        ],
                    },
                ),
            },
        }

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
        self.ngpd.start(
            self.frame_length, self.num_cyles, self.setup_scope, self.run_scope
        )
        self.monitorCallback.start()

    @UsesNgpdLibrary
    def stop(self):
        """Stop the Acquisition."""
        logging.debug("Stopping Acquisition")
        self.ngpd.stop()
        self.monitorCallback.stop()

    def set_run(self, run: bool):
        """Start or stop the acquisition."""
        if run:
            self.start()
        else:
            self.stop()

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

        done = (
            col_time
            - (status.timer_us / 1000000.0)
            + (cycles - status.cycles) * col_time
        )

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

        tail_data: NDArray = self.data.sum(2, dtype=np.uint32)
        self.tail_sum_data = np.append(
            tail_data, [tail_data.sum(0, dtype=np.uint32)], 0
        )
        pulse_data: NDArray = self.data.sum(1, dtype=np.uint32)
        self.pulse_height_data = np.append(
            pulse_data, [pulse_data.sum(0, dtype=np.uint32)], 0
        )

        self.chan = 0
        self.select_display: Literal["neutron", "gamma", "pileup", "all"] = "all"

    def configure(self, ngpd: PyNgpd):
        """Configure the data handler to use the ngpd class object."""
        self.ngpd = ngpd

    @UsesNgpdLibrary
    def refresh_data(self):
        """Read the histogram data from the specified channel."""
        self.data = self.ngpd.read_histogram(self.chan)
        logging.debug(f"Data Shape: {self.data.shape}")
        self.data_shape = self.data.shape

        # creating datasets for line graphs by summing bins together
        # also adding an "exta" dataset thats the total for each (assuming its 3 datasets for separate NGP)
        pulse_data: NDArray = self.data.sum(1, dtype=np.uint32)
        if pulse_data.shape[0] == 3:
            pulse_data: NDArray = np.append(
                pulse_data, [pulse_data.sum(0, dtype=np.uint32)], 0
            )
        logging.debug(f"Pulse Data Shape: {pulse_data.shape}")

        tail_data: NDArray = self.data.sum(2, dtype=np.uint32)
        if tail_data.shape[0] == 3:
            tail_data = np.append(tail_data, [tail_data.sum(0, dtype=np.uint32)], 0)
        logging.debug(f"Tail Data Shape: {tail_data.shape}")

        self.tail_sum_data = tail_data
        self.pulse_height_data = pulse_data

    def get_data(self):
        """Get Data as Base64 encoded string, for better HTTP transfer."""
        if self.select_display == "all":
            data: NDArray = self.data.sum(0, dtype=np.uint32)
        else:
            data: NDArray = self.data[HistDisplay[self.select_display]]
        logging.debug("Encoding Data")
        logging.debug(f"Max Data: {np.max(data)}")
        logging.debug(f"Data Shape: {data.shape}")
        logging.debug(f"Data Type: {data.dtype}")
        encoded = base64.b64encode(np.ascontiguousarray(data))
        return encoded.decode("utf-8")

    def get_pulse_height(self):
        """Get Pulse height data as Base64 string."""
        encoded = base64.b64encode(np.ascontiguousarray(self.pulse_height_data))
        return encoded.decode("utf-8")

    def get_tailsum(self):
        """Get Tail Sum data as Base64 string."""
        encoded = base64.b64encode(np.ascontiguousarray(self.tail_sum_data))
        return encoded.decode("utf-8")

    def set_channel(self, chan: int):
        """Set the channel to read data from, and refresh data."""
        self.chan = chan
        self.refresh_data()

    @UsesNgpdLibrary
    def save_data(self, filename: str):
        """Save the Histogram of each channel to a HDF5 file."""
        with h5py.File(filename, "w") as file:
            for chan in range(8):
                data = self.ngpd.read_histogram(chan)
                if data.shape[0] == 3:
                    # add the All Summed histogram to the dataset, if they've been separated
                    data = np.append(data, [data.sum(0)], 0)
                file.create_dataset(f"channel {chan}", data=data)

    def download_data(self, filename: str) -> bytes:
        """Provide the saved HDF5 histogram as a bytes object, for downloading."""
        filedata: bytes = None
        with open(filename, "rb") as f:
            filedata = f.read()
        return filedata
