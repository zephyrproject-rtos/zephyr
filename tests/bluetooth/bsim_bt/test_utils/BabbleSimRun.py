import logging
import os
import sys
import subprocess
import multiprocessing as mp
import abc
from test_utils.BabbleSimError import BabbleSimError

LOGGER_NAME = f"bsim_plugin.{__name__.split('.')[-1]}"
logger = logging.getLogger(LOGGER_NAME)

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
if not ZEPHYR_BASE:
    sys.exit("$ZEPHYR_BASE environment variable undefined")

BSIM_OUT_PATH = os.getenv("BSIM_OUT_PATH")
if not BSIM_OUT_PATH:
    sys.exit("$BSIM_OUT_PATH environment variable undefined")

BSIM_BIN_DIR_NAME = "bin"
BSIM_BIN_DIR_PATH = os.path.join(BSIM_OUT_PATH, BSIM_BIN_DIR_NAME)

RUN_SIM_TIMEOUT_DURATION = 300.0  # [s]


class BabbleSimRun:
    def __init__(self, test_out_path, sim_id, general_exe_name, devices_config,
                 phy_config):
        general_exe_path = os.path.join(BSIM_BIN_DIR_PATH, general_exe_name)
        self.devices = []
        for device_no, device_config in enumerate(devices_config):
            device = Device(sim_id, general_exe_path, device_no, device_config,
                            test_out_path)
            self.devices.append(device)
        self.phy = Phy(sim_id, len(self.devices), phy_config, test_out_path)

    def run(self):
        self._log_run_bsim_cmd()

        if mp.get_start_method(allow_none=True) != "spawn":
            mp.set_start_method("spawn", force=True)

        number_processes = len(self.devices) + 1  # devices + phy
        pool = mp.Pool(processes=number_processes)

        for device in self.devices:
            device.run_process_result = \
                pool.apply_async(run_process, device.run_process_args)

        self.phy.run_process_result = \
            pool.apply_async(run_process, self.phy.run_process_args)

        # TODO: add timeout here
        pool.close()
        pool.join()

        self._log_run_bsim_info()

        self._verify_run_bsim_results()

    def _log_run_bsim_cmd(self):
        general_log_msg = "Run BabbleSim simulation with commands:"
        for device in self.devices:
            device_cmd_log = " ".join(device.process_cmd)
            general_log_msg += f"\n{device_cmd_log} &"

        phy_cmd_log = " ".join(self.phy.process_cmd)
        general_log_msg += f"\n{phy_cmd_log}"

        logger.info(general_log_msg)

    def _log_run_bsim_info(self):
        general_log_msg = f"Logs saved at:"

        for device in self.devices:
            general_log_msg += f"\n{device.log_file_base_path}_out.log"
            err_file_path = f"{device.log_file_base_path}_err.log"
            if os.path.exists(err_file_path):
                general_log_msg += f"\n{err_file_path}"

        general_log_msg += f"\n{self.phy.log_file_base_path}_out.log"
        err_file_path = f"{self.phy.log_file_base_path}_err.log"
        if os.path.exists(err_file_path):
            general_log_msg += f"\n{err_file_path}"

        logger.info(general_log_msg)

    def _verify_run_bsim_results(self):
        failure_occur_flag = False
        failures_msg = []

        for device in self.devices:
            if device.run_process_result.get() != 0:
                failure_msg = f"Failure during simulate {device.name} device"
                failures_msg += [failure_msg]
                logger.error(failure_msg)
                failure_occur_flag = True

        if self.phy.run_process_result.get() != 0:
            failure_msg = \
                f"Failure during simulate {self.phy.name} physical layer"
            failures_msg += [failure_msg]
            logger.error(failure_msg)
            failure_occur_flag = True

        if failure_occur_flag:
            failures_msg = "\n".join(failures_msg)
            raise BabbleSimError(failures_msg)


class BabbleSimObject(abc.ABC):
    """
    Abstract class to store information about BabbleSim "objects" like devices
    (nodes/applications) or physical layer.
    """
    def __init__(self, sim_id, name, exe_path, extra_run_args, test_out_path):
        self.sim_id = sim_id
        self.name = name
        self.exe_path = exe_path
        self.extra_run_args = extra_run_args
        self.log_file_base_path = os.path.join(test_out_path, name)
        self.process_cmd = self._get_cmd()
        self.run_process_args = self._prepare_run_process_args()
        self.run_process_result = None

    @abc.abstractmethod
    def _get_cmd(self):
        pass

    def _prepare_run_process_args(self):
        run_process_args = [
            self.process_cmd,
            self.log_file_base_path
        ]
        return run_process_args


class Device(BabbleSimObject):
    """
    Device represents single node/application in whole simulation like "central"
    or "peripheral" device.
    """
    def __init__(self, sim_id, exe_path, device_no, device_config,
                 test_out_path):
        name = device_config["testid"]
        extra_run_args = device_config.get("extra_run_args", [])
        self.device_no = device_no

        super().__init__(sim_id, name, exe_path, extra_run_args, test_out_path)

    def _get_cmd(self):
        run_app_args = [
            f'-s={self.sim_id}',
            f'-d={self.device_no}',
            f'-testid={self.name}'
        ]
        run_app_args += self.extra_run_args
        run_app_cmd = [self.exe_path] + run_app_args
        return run_app_cmd


class Phy(BabbleSimObject):
    """
    Phy represents BabbleSim's wireless transport physical layer like
    "bs_2G4_phy_v1".
    """
    def __init__(self, sim_id, number_devices, phy_config, test_out_path):
        name = phy_config["name"]
        exe_phy_path = os.path.join(BSIM_BIN_DIR_PATH, name)
        extra_run_args = phy_config.get("extra_run_args", [])
        self.sim_length = phy_config["sim_length"]
        self.number_devices = number_devices

        super().__init__(sim_id, name, exe_phy_path, extra_run_args,
                         test_out_path)

    def _get_cmd(self):
        run_phy_args = [
            f'-s={self.sim_id}',
            f'-D={self.number_devices}',
            f'-sim_length={self.sim_length}'
        ]
        run_phy_args += self.extra_run_args
        run_phy_cmd = [self.exe_path] + run_phy_args
        return run_phy_cmd


def run_process(process_cmd, log_file_base_path):
    """
    Functions used during spawn several processes necessary when BabbleSim
    simulation is launched.
    """
    ps_logger = ProcessLogger(log_file_base_path, debug_enable=False)

    result = subprocess.run(
        process_cmd,
        cwd=BSIM_BIN_DIR_PATH,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=RUN_SIM_TIMEOUT_DURATION
    )

    ps_logger.save_out_log(result.stdout)
    if result.stderr:
        ps_logger.save_err_log(result.stderr)

    return result.returncode


class ProcessLogger:
    """
    Logger class designed to use particularly in run_process function, due to
    the problem with use "classic" logger instance when this function is run
    in separate process.
    """
    def __init__(self, log_file_base_path, debug_enable=False):
        """
        :param debug_enable: used only during plugin debugging
        """
        self.out_file_path = f"{log_file_base_path}_out.log"

        self.err_file_path = f"{log_file_base_path}_err.log"

        self.debug_file_path = f"{log_file_base_path}_debug.log"
        self.debug_enable = debug_enable
        process_name = os.path.basename(log_file_base_path)
        self.save_debug_log(f"Debug logger for process: {process_name}, "
                            f"with PID: {os.getpid()}\n\n")

    def save_out_log(self, data):
        self._save_log(self.out_file_path, data)

    def save_err_log(self, data):
        self._save_log(self.err_file_path, data)

    @staticmethod
    def _save_log(log_file_path, data):
        with open(log_file_path, "wb") as file:
            file.write(data)

    def save_debug_log(self, log):
        """
        Used only during plugin debugging.
        """
        if self.debug_enable:
            with open(self.debug_file_path, "a") as file:
                file.write(log)
