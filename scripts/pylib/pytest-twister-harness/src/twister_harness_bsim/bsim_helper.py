# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0


import logging
import shlex
import subprocess

from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.device.utils import terminate_process
from twister_harness.fixtures import get_ready_shells
from twisterlib.harness import Shell

logger = logging.getLogger(__name__)


class BsimHelperException(Exception):
    """Custom exception for BsimHelper errors."""


class BsimHelper:
    """Helper class to manage the BSIM process for tests."""

    bs_phy: str = "./bs_2G4_phy_v1"
    device_handbrake: str = "./bs_device_handbrake"

    def __init__(
        self,
        bsim_out_path: str,
        simulation_id: str,
        unlaunched_duts: list[DeviceAdapter],
        verbosity_level: str,
    ) -> None:
        self.bsim_out_path: str = bsim_out_path
        self.simulation_id = simulation_id
        self.duts: list[DeviceAdapter] = unlaunched_duts
        self.verbosity_level = verbosity_level
        self._bsim_process: subprocess.Popen | None = None
        self._handbrake_process: subprocess.Popen | None = None

    def execute_bs_command(self, program: str, args: str) -> subprocess.Popen:
        """Execute a command from the bsim_out_path/bin directory and return the process handle."""
        cmd = [
            program,
            f"-v={self.verbosity_level}",
            f"-s={self.simulation_id}",
            *shlex.split(args),
        ]
        logger.debug("Running command: %s", shlex.join(cmd))
        try:
            process = subprocess.Popen(
                cmd,
                stderr=subprocess.PIPE,
                stdout=subprocess.PIPE,
                cwd=f"{self.bsim_out_path}/bin",
            )
        except Exception as exc:
            msg = f'Failed to start command {shlex.join(cmd)}: {exc}'
            logger.exception(msg)
            raise BsimHelperException(msg) from exc
        return process

    def start_bsim(self, args: str) -> None:
        """Start the bsim physical layer process with the given parameters."""
        self._bsim_process = self.execute_bs_command(self.bs_phy, args)

    def start_device_handbrake(self, args: str) -> None:
        """Start the device handbrake process with the given parameters."""
        self._handbrake_process = self.execute_bs_command(self.device_handbrake, args)

    def wait_for_bsim(self, timeout: float) -> None:
        """Wait for the bsim process to finish and check its exit status."""
        if self._bsim_process is None:
            return

        try:
            stdout, stderr = self._bsim_process.communicate(timeout=timeout)
            if stdout:
                logger.debug(stdout.decode(errors="ignore"))
            if self._bsim_process.returncode != 0:
                msg = f'bsim process failed: \n{stderr.decode(errors="ignore")}'
                logger.error(msg)
                raise BsimHelperException(msg)
        except subprocess.TimeoutExpired as exc:
            self.stop_bsim()
            msg = f'Timeout occurred ({timeout}s) during execution of bsim process.'
            logger.error(msg)
            raise BsimHelperException(msg) from exc

    def terminate_bs_process(self, process: subprocess.Popen) -> None:
        """Terminate process if it is still running."""
        if process is None:
            return
        return_code: int | None = process.poll()
        if return_code is None:
            terminate_process(process)
        else:
            logger.debug('bsim process already finished with return code %s', return_code)

    def stop_bsim(self) -> None:
        """Terminate the bsim processes"""
        self.terminate_bs_process(self._bsim_process)
        self._bsim_process = None

    def stop_device_handbrake(self) -> None:
        """Terminate the device handbrake process."""
        self.terminate_bs_process(self._handbrake_process)
        self._handbrake_process = None

    def close(self) -> None:
        """Stop all running processes."""
        self.stop_bsim()
        self.stop_device_handbrake()

    def run_dut(self, dut_index: int, args: str) -> DeviceAdapter:
        """Run the specified DUT with the given extra arguments."""
        if dut_index >= len(self.duts):
            raise BsimHelperException(
                f"Invalid DUT index: {dut_index}. Only {len(self.duts)} DUT(s) available."
            )
        dut: DeviceAdapter = self.duts[dut_index]
        # Must clear the command first to avoid interference with other tests
        dut.command = []
        user_extra_args = dut.device_config.extra_test_args or ""
        dut.device_config.extra_test_args = (
            f"-v={self.verbosity_level} -s={self.simulation_id} {args} {user_extra_args}"
        )
        dut.launch()
        # Restore the original extra_test_args to avoid affecting other tests
        dut.device_config.extra_test_args = user_extra_args
        return dut

    def flush_duts_output(self, print_output: bool = True) -> None:
        """Flush the output buffers of all DUTs to ensure all logs are captured."""
        for dut in self.duts:
            dut.readlines(print_output=print_output)

    def get_ready_shells(self) -> list[Shell]:
        """Get the shell interfaces for all DUTs."""
        return get_ready_shells(self.duts)
