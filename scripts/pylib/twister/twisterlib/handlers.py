#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2022 Intel Corporation
# Copyright 2022 NXP
# Copyright (c) 2024 Arm Limited (or its affiliates). All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import contextlib
import logging
import math
import os
import re
import select
import shlex
import signal
import subprocess
import sys
import threading
import time
from contextlib import contextmanager
from pathlib import Path
from queue import Empty, Queue

import psutil
from twisterlib.environment import ZEPHYR_BASE, strip_ansi_sequences
from twisterlib.error import TwisterException
from twisterlib.platform import Platform
from twisterlib.statuses import TwisterStatus

sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/build_helpers"))
from domains import Domains

try:
    import serial
except ImportError:
    print("Install pyserial python module with pip to use --device-testing option.")

try:
    import pty
except ImportError as capture_error:
    if os.name == "nt":  # "nt" means that program is running on Windows OS
        pass  # "--device-serial-pty" option is not supported on Windows OS
    else:
        raise capture_error

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)


def terminate_process(proc):
    """
    encapsulate terminate functionality so we do it consistently where ever
    we might want to terminate the proc.  We need try_kill_process_by_pid
    because of both how newer ninja (1.6.0 or greater) and .NET / renode
    work.  Newer ninja's don't seem to pass SIGTERM down to the children
    so we need to use try_kill_process_by_pid.
    """

    for child in psutil.Process(proc.pid).children(recursive=True):
        with contextlib.suppress(ProcessLookupError, psutil.NoSuchProcess):
            os.kill(child.pid, signal.SIGTERM)
    proc.terminate()
    # sleep for a while before attempting to kill
    time.sleep(0.5)
    proc.kill()


class Handler:
    def __init__(self, instance, type_str: str, options: argparse.Namespace,
                 generator_cmd: str | None = None, suite_name_check: bool = True):
        """Constructor

        """
        self.options = options

        self.run = False
        self.type_str = type_str

        self.pid_fn = None
        self.call_make_run = True

        self.name = instance.name
        self.instance = instance
        self.sourcedir = instance.testsuite.source_dir
        self.build_dir = instance.build_dir
        self.log = os.path.join(self.build_dir, "handler.log")
        self.returncode = 0
        self.generator_cmd = generator_cmd
        self.suite_name_check = suite_name_check
        self.ready = False

        self.args = []
        self.terminated = False

    def get_test_timeout(self):
        return math.ceil(self.instance.testsuite.timeout *
                         self.instance.platform.timeout_multiplier *
                         self.options.timeout_multiplier)

    def terminate(self, proc):
        terminate_process(proc)
        self.terminated = True

    def _verify_ztest_suite_name(self, harness_status, detected_suite_names, handler_time):
        """
        If test suite names was found in test's C source code, then verify if
        detected suite names from output correspond to expected suite names
        (and not in reverse).
        """
        expected_suite_names = self.instance.testsuite.ztest_suite_names
        logger.debug(f"Expected suite names:{expected_suite_names}")
        logger.debug(f"Detected suite names:{detected_suite_names}")
        if not expected_suite_names or \
                harness_status != TwisterStatus.PASS:
            return
        if not detected_suite_names:
            self._missing_suite_name(expected_suite_names, handler_time)
            return
        # compare the expect and detect from end one by one without order
        _d_suite = detected_suite_names[-len(expected_suite_names):]
        if (
            set(_d_suite) != set(expected_suite_names)
            and not set(_d_suite).issubset(set(expected_suite_names))
        ):
            self._missing_suite_name(expected_suite_names, handler_time)

    def _missing_suite_name(self, expected_suite_names, handler_time):
        """
        Change result of performed test if problem with missing or unpropper
        suite name was occurred.
        """
        self.instance.status = TwisterStatus.FAIL
        self.instance.execution_time = handler_time
        for tc in self.instance.testcases:
            tc.status = TwisterStatus.FAIL
        self.instance.reason = "Testsuite mismatch"
        logger.debug(
            "Test suite names were not printed or some of them in output"
            f" do not correspond with expected: {str(expected_suite_names)}",
        )

    def _final_handle_actions(self, harness, handler_time):

        # only for Ztest tests:
        harness_class_name = type(harness).__name__
        if self.suite_name_check and harness_class_name == "Test":
            self._verify_ztest_suite_name(
                harness.status,
                harness.detected_suite_names,
                handler_time
            )
            if self.instance.status == TwisterStatus.FAIL:
                return
            if not harness.matched_run_id and harness.run_id_exists:
                self.instance.status = TwisterStatus.FAIL
                self.instance.execution_time = handler_time
                self.instance.reason = "RunID mismatch"
                for tc in self.instance.testcases:
                    tc.status = TwisterStatus.FAIL

        self.instance.record(harness.recording)

    def get_default_domain_build_dir(self):
        if self.instance.sysbuild:
            # Load domain yaml to get default domain build directory
            # Note: for targets using QEMU, we assume that the target will
            # have added any additional images to the run target manually
            domain_path = os.path.join(self.build_dir, "domains.yaml")
            domains = Domains.from_file(domain_path)
            logger.debug(f"Loaded sysbuild domain data from {domain_path}")
            build_dir = domains.get_default_domain().build_dir
        else:
            build_dir = self.build_dir
        return build_dir


class BinaryHandler(Handler):
    def __init__(
        self,
        instance,
        type_str: str,
        options: argparse.Namespace,
        generator_cmd: str | None = None,
        suite_name_check: bool = True
    ):
        """Constructor

        @param instance Test Instance
        """
        super().__init__(instance, type_str, options, generator_cmd, suite_name_check)

        self.seed = None
        self.extra_test_args = None
        self.line = b""
        self.binary: str | None = None

    def try_kill_process_by_pid(self):
        if self.pid_fn:
            with open(self.pid_fn) as pid_file:
                pid = int(pid_file.read())
            os.unlink(self.pid_fn)
            self.pid_fn = None  # clear so we don't try to kill the binary twice
            with contextlib.suppress(ProcessLookupError, psutil.NoSuchProcess):
                os.kill(pid, signal.SIGKILL)

    def _output_reader(self, proc):
        self.line = proc.stdout.readline()

    def _output_handler(self, proc, harness):
        suffix = '\\r\\n'

        with open(self.log, "w") as log_out_fp:
            timeout_extended = False
            timeout_time = time.time() + self.get_test_timeout()
            while True:
                this_timeout = timeout_time - time.time()
                if this_timeout < 0:
                    break
                reader_t = threading.Thread(target=self._output_reader, args=(proc,), daemon=True)
                reader_t.start()
                reader_t.join(this_timeout)
                if not reader_t.is_alive() and self.line != b"":
                    line_decoded = self.line.decode('utf-8', "replace")
                    stripped_line = line_decoded.rstrip()
                    if stripped_line.endswith(suffix):
                        stripped_line = stripped_line[:-len(suffix)].rstrip()
                    logger.debug(f"OUTPUT: {stripped_line}")
                    log_out_fp.write(strip_ansi_sequences(line_decoded))
                    log_out_fp.flush()
                    harness.handle(stripped_line)
                    if (
                        harness.status != TwisterStatus.NONE
                        and not timeout_extended
                        or harness.capture_coverage
                    ):
                        timeout_extended = True
                        if harness.capture_coverage:
                            timeout_time = time.time() + 30
                        else:
                            timeout_time = time.time() + 2
                else:
                    reader_t.join(0)
                    break
            try:
                # POSIX arch based ztests end on their own,
                # so let's give it up to 100ms to do so
                proc.wait(0.1)
            except subprocess.TimeoutExpired:
                self.terminate(proc)

    def _create_command(self, robot_test):

        if robot_test:
            keywords = os.path.join(self.options.coverage_basedir, 'tests/robot/common.robot')
            elf = os.path.join(self.build_dir, "zephyr/zephyr.elf")
            command = [self.generator_cmd]
            resc = ""
            uart = ""
            # os.path.join cannot be used on a Mock object, so we are
            # explicitly checking the type
            if isinstance(self.instance.platform, Platform):
                for board_dir in self.options.board_root:
                    path = os.path.join(Path(board_dir).parent, self.instance.platform.resc)
                    if os.path.exists(path):
                        resc = path
                        break
                uart = self.instance.platform.uart
                command = ["renode-test",
                            "--variable", "KEYWORDS:" + keywords,
                            "--variable", "ELF:@" + elf,
                            "--variable", "RESC:@" + resc,
                            "--variable", "UART:" + uart]
        elif self.call_make_run:
            if self.options.sim_name:
                target = f"run_{self.options.sim_name}"
            else:
                target = "run"

            command = [self.generator_cmd, "-C", self.get_default_domain_build_dir(), target]
        elif self.instance.testsuite.type == "unit":
            assert self.binary, "Missing binary in unit testsuite."
            command = [self.binary]
        else:
            binary = os.path.join(self.get_default_domain_build_dir(), "zephyr", "zephyr.exe")
            command = [binary]

        if self.options.enable_valgrind:
            command = ["valgrind", "--error-exitcode=2",
                       "--leak-check=full",
                       "--suppressions=" + ZEPHYR_BASE + "/scripts/valgrind.supp",
                       "--log-file=" + self.build_dir + "/valgrind.log",
                       "--track-origins=yes",
                       ] + command

        # Only valid for native_sim
        if self.seed is not None:
            command.append(f"--seed={self.seed}")
        if self.extra_test_args is not None:
            command.extend(self.extra_test_args)

        return command

    def _create_env(self):
        env = os.environ.copy()
        if self.options.enable_asan:
            env["ASAN_OPTIONS"] = "log_path=stdout:" + \
                                  env.get("ASAN_OPTIONS", "")
            if not self.options.enable_lsan:
                env["ASAN_OPTIONS"] += "detect_leaks=0"

        if self.options.enable_ubsan:
            env["UBSAN_OPTIONS"] = "log_path=stdout:halt_on_error=1:" + \
                                  env.get("UBSAN_OPTIONS", "")

        return env

    def _update_instance_info(self, harness, handler_time):
        self.instance.execution_time = handler_time
        if not self.terminated and self.returncode != 0:
            self.instance.status = TwisterStatus.FAIL
            if self.options.enable_valgrind and self.returncode == 2:
                self.instance.reason = "Valgrind error"
            else:
                # When a process is killed, the default handler returns 128 + SIGTERM
                # so in that case the return code itself is not meaningful
                self.instance.reason = f"Failed (rc={self.returncode})"
            self.instance.add_missing_case_status(TwisterStatus.BLOCK)
        elif harness.status != TwisterStatus.NONE:
            self.instance.status = harness.status
            if harness.status == TwisterStatus.FAIL:
                self.instance.reason = f"Failed harness:'{harness.reason}'"
            self.instance.add_missing_case_status(TwisterStatus.BLOCK)
        else:
            self.instance.status = TwisterStatus.FAIL
            self.instance.reason = "Timeout"
            self.instance.add_missing_case_status(TwisterStatus.BLOCK, "Timeout")

    def handle(self, harness):
        robot_test = getattr(harness, "is_robot_test", False)

        command = self._create_command(robot_test)

        logger.debug("Spawning process: " +
                     " ".join(shlex.quote(word) for word in command) + os.linesep +
                     "in directory: " + self.build_dir)

        start_time = time.time()

        env = self._create_env()

        if robot_test:
            harness.run_robot_test(command, self)
            return

        stderr_log = f"{self.instance.build_dir}/handler_stderr.log"
        with open(stderr_log, "w+") as stderr_log_fp, subprocess.Popen(
            command, stdout=subprocess.PIPE, stderr=stderr_log_fp, cwd=self.build_dir, env=env
        ) as proc:
            logger.debug(f"Spawning BinaryHandler Thread for {self.name}")
            t = threading.Thread(target=self._output_handler, args=(proc, harness,), daemon=True)
            t.start()
            t.join()
            if t.is_alive():
                self.terminate(proc)
                t.join()
            proc.wait()
            self.returncode = proc.returncode
            if proc.returncode != 0:
                self.instance.status = TwisterStatus.ERROR
                self.instance.reason = f"BinaryHandler returned {proc.returncode}"
            self.try_kill_process_by_pid()

        handler_time = time.time() - start_time

        # FIXME: This is needed when killing the simulator, the console is
        # garbled and needs to be reset. Did not find a better way to do that.
        if sys.stdout.isatty():
            subprocess.call(["stty", "sane"], stdin=sys.stdout)

        self._update_instance_info(harness, handler_time)

        self._final_handle_actions(harness, handler_time)


class SimulationHandler(BinaryHandler):
    def __init__(
        self,
        instance,
        type_str: str,
        options: argparse.Namespace,
        generator_cmd: str | None = None,
        suite_name_check: bool = True,
    ):
        """Constructor

        @param instance Test Instance
        """
        super().__init__(instance, type_str, options, generator_cmd, suite_name_check)

        if type_str == 'renode':
            self.pid_fn = os.path.join(instance.build_dir, "renode.pid")
        elif type_str == 'native':
            self.call_make_run = False
            self.ready = True


class DeviceHandler(Handler):
    def get_test_timeout(self):
        timeout = super().get_test_timeout()
        if self.options.enable_coverage:
            # wait more for gcov data to be dumped on console
            timeout += 120
        return timeout

    def monitor_serial(self, ser, halt_event, harness):
        if self.options.enable_coverage:
            # Set capture_coverage to True to indicate that right after
            # test results we should get coverage data, otherwise we exit
            # from the test.
            harness.capture_coverage = True

        # Wait for serial connection
        while not ser.isOpen():
            time.sleep(0.1)

        # Clear serial leftover.
        ser.reset_input_buffer()

        with open(self.log, "wb") as log_out_fp:
            while ser.isOpen():
                if halt_event.is_set():
                    logger.debug('halted')
                    ser.close()
                    break

                try:
                    if not ser.in_waiting:
                        # no incoming bytes are waiting to be read from
                        # the serial input buffer, let other threads run
                        time.sleep(0.001)
                        continue
                # maybe the serial port is still in reset
                # check status may cause error
                # wait for more time
                except OSError:
                    time.sleep(0.001)
                    continue
                except TypeError:
                    # This exception happens if the serial port was closed and
                    # its file descriptor cleared in between of ser.isOpen()
                    # and ser.in_waiting checks.
                    logger.debug("Serial port is already closed, stop reading.")
                    break

                serial_line = None
                # SerialException may happen during the serial device power off/on process.
                with contextlib.suppress(TypeError, serial.SerialException):
                    serial_line = ser.readline()

                # Just because ser_fileno has data doesn't mean an entire line
                # is available yet.
                if serial_line:
                    # can be more lines in serial_line so split them before handling
                    for sl in serial_line.decode('utf-8', 'ignore').splitlines(keepends=True):
                        log_out_fp.write(strip_ansi_sequences(sl).encode('utf-8'))
                        log_out_fp.flush()
                        if sl := sl.strip():
                            logger.debug(f"DEVICE: {sl}")
                            harness.handle(sl)

                if harness.status != TwisterStatus.NONE and not harness.capture_coverage:
                    ser.close()
                    break

    @staticmethod
    @contextmanager
    def acquire_dut_locks(duts):
        try:
            for d in duts:
                d.lock.acquire()
            yield
        finally:
            for d in duts:
                d.lock.release()

    def device_is_available(self, instance):
        device = instance.platform.name
        fixture = instance.testsuite.harness_config.get("fixture")
        duts_found = []

        for d in self.duts:
            if fixture and fixture not in map(lambda f: f.split(sep=':')[0], d.fixtures):
                continue
            if d.platform != device or (d.serial is None and d.serial_pty is None):
                continue
            duts_found.append(d)

        if not duts_found:
            raise TwisterException(f"No device to serve as {device} platform.")

        # Select an available DUT with less failures
        for d in sorted(duts_found, key=lambda _dut: _dut.failures):
            # get all DUTs with the same id
            duts_shared_hw = [_d for _d in self.duts if _d.id == d.id]
            with self.acquire_dut_locks(duts_shared_hw):
                avail = False
                if d.available:
                    for _d in duts_shared_hw:
                        _d.available = 0
                    d.counter_increment()
                    avail = True
                    logger.debug(f"Retain DUT:{d.platform}, Id:{d.id}, "
                                f"counter:{d.counter}, failures:{d.failures}")
            if avail:
                return d

        return None

    def make_dut_available(self, dut):
        if self.instance.status in [TwisterStatus.ERROR, TwisterStatus.FAIL]:
            dut.failures_increment()
        logger.debug(f"Release DUT:{dut.platform}, Id:{dut.id}, "
                     f"counter:{dut.counter}, failures:{dut.failures}")
        # get all DUTs with the same id
        duts_shared_hw = [_d for _d in self.duts if _d.id == dut.id]
        with self.acquire_dut_locks(duts_shared_hw):
            for _d in duts_shared_hw:
                _d.available = 1

    @staticmethod
    def run_custom_script(script, timeout):
        with subprocess.Popen(script, stderr=subprocess.PIPE, stdout=subprocess.PIPE) as proc:
            try:
                stdout, stderr = proc.communicate(timeout=timeout)
                logger.debug(stdout.decode())
                if proc.returncode != 0:
                    logger.error(f"Custom script failure: {stderr.decode(errors='ignore')}")

            except subprocess.TimeoutExpired:
                proc.kill()
                proc.communicate()
                logger.error(f"{script} timed out")

    def _create_command(self, runner, hardware):
        if (self.options.west_flash is not None) or runner:
            command = ["west", "flash", "--skip-rebuild", "-d", self.build_dir]
            command_extra_args = []

            # There are three ways this option is used.
            # 1) bare: --west-flash
            #    This results in options.west_flash == []
            # 2) with a value: --west-flash="--board-id=42"
            #    This results in options.west_flash == "--board-id=42"
            # 3) Multiple values: --west-flash="--board-id=42,--erase"
            #    This results in options.west_flash == "--board-id=42 --erase"
            if self.options.west_flash and self.options.west_flash != []:
                command_extra_args.extend(self.options.west_flash.split(','))

            if runner:
                command.append("--runner")
                command.append(runner)

                board_id = hardware.probe_id or hardware.id
                product = hardware.product
                serial_port = hardware.serial
                if board_id is not None:
                    if runner in ("pyocd", "nrfjprog", "nrfutil"):
                        command_extra_args.append("--dev-id")
                        command_extra_args.append(board_id)
                    elif runner == "esp32":
                        command_extra_args.append("--esp-device")
                        command_extra_args.append(serial_port)
                    elif (
                        runner == "openocd"
                        and product == "STM32 STLink"
                        or runner == "openocd"
                        and product == "STLINK-V3"
                    ):
                        command_extra_args.append("--cmd-pre-init")
                        command_extra_args.append(f"hla_serial {board_id}")
                    elif runner == "openocd" and product == "EDBG CMSIS-DAP":
                        command_extra_args.append("--cmd-pre-init")
                        command_extra_args.append(f"cmsis_dap_serial {board_id}")
                    elif runner == "openocd" and product == "LPC-LINK2 CMSIS-DAP":
                        command_extra_args.append("--cmd-pre-init")
                        command_extra_args.append(f"adapter serial {board_id}")
                    elif runner == "jlink":
                        command.append("--dev-id")
                        command.append(board_id)
                    elif runner == "linkserver":
                        # for linkserver
                        # --probe=#<number> select by probe index
                        # --probe=<serial number> select by probe serial number
                        command.append(f"--probe={board_id}")
                    elif runner == "stm32cubeprogrammer":
                        command.append(f"--tool-opt=sn={board_id}")

                    # Receive parameters from runner_params field.
                    if hardware.runner_params:
                        for param in hardware.runner_params:
                            command.append(param)

            if command_extra_args:
                command.append('--')
                command.extend(command_extra_args)
        else:
            command = [self.generator_cmd, "-C", self.build_dir, "flash"]

        return command

    def _update_instance_info(self, harness, handler_time, flash_error):
        self.instance.execution_time = handler_time
        if harness.status != TwisterStatus.NONE:
            self.instance.status = harness.status
            if harness.status == TwisterStatus.FAIL:
                self.instance.reason = f"Failed harness:'{harness.reason}'"
            self.instance.add_missing_case_status(TwisterStatus.BLOCK, harness.status)
        elif not flash_error:
            self.instance.status = TwisterStatus.FAIL
            self.instance.reason = "Timeout"

        if self.instance.status in [TwisterStatus.ERROR, TwisterStatus.FAIL]:
            self.instance.add_missing_case_status(TwisterStatus.BLOCK, self.instance.reason)

    def _terminate_pty(self, ser_pty, ser_pty_process):
        logger.debug(f"Terminating serial-pty:'{ser_pty}'")
        terminate_process(ser_pty_process)
        try:
            (stdout, stderr) = ser_pty_process.communicate(timeout=self.get_test_timeout())
            logger.debug(f"Terminated serial-pty:'{ser_pty}', stdout:'{stdout}', stderr:'{stderr}'")
        except subprocess.TimeoutExpired:
            logger.debug(f"Terminated serial-pty:'{ser_pty}'")
    #

    def _create_serial_connection(self, dut, serial_device, hardware_baud,
                                  flash_timeout, serial_pty, ser_pty_process):
        try:
            ser = serial.Serial(
                serial_device,
                baudrate=hardware_baud,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS,
                # the worst case of no serial input
                timeout=max(flash_timeout, self.get_test_timeout())
            )
        except serial.SerialException as e:
            self._handle_serial_exception(e, dut, serial_pty, ser_pty_process)
            raise

        return ser


    def _handle_serial_exception(self, exception, dut, serial_pty, ser_pty_process):
        self.instance.status = TwisterStatus.FAIL
        self.instance.reason = "Serial Device Error"
        logger.error(f"Serial device error: {exception!s}")

        self.instance.add_missing_case_status(TwisterStatus.BLOCK, "Serial Device Error")
        if serial_pty and ser_pty_process:
            self._terminate_pty(serial_pty, ser_pty_process)

        self.make_dut_available(dut)


    def get_hardware(self):
        hardware = None
        try:
            hardware = self.device_is_available(self.instance)
            in_waiting = 0
            while not hardware:
                time.sleep(1)
                in_waiting += 1
                if in_waiting%60 == 0:
                    logger.debug(f"Waiting for a DUT to run {self.instance.name}")
                hardware = self.device_is_available(self.instance)
        except TwisterException as error:
            self.instance.status = TwisterStatus.FAIL
            self.instance.reason = str(error)
            logger.error(self.instance.reason)
        return hardware

    def _get_serial_device(self, serial_pty, hardware_serial):
        ser_pty_process = None
        if serial_pty:
            master, slave = pty.openpty()
            try:
                ser_pty_process = subprocess.Popen(
                    re.split('[, ]', serial_pty),
                    stdout=master,
                    stdin=master,
                    stderr=master
                )
            except subprocess.CalledProcessError as error:
                logger.error(
                    f"Failed to run subprocess {serial_pty}, error {error.output}"
                )
                return

            serial_device = os.ttyname(slave)
        else:
            serial_device = hardware_serial

        return serial_device, ser_pty_process

    def handle(self, harness):
        runner = None
        hardware = self.get_hardware()
        if hardware:
            self.instance.dut = hardware.id
        else:
            return

        runner = hardware.runner or self.options.west_runner
        serial_pty = hardware.serial_pty

        serial_device, ser_pty_process = self._get_serial_device(serial_pty, hardware.serial)

        logger.debug(f"Using serial device {serial_device} @ {hardware.baud} baud")

        command = self._create_command(runner, hardware)

        pre_script = hardware.pre_script
        post_flash_script = hardware.post_flash_script
        post_script = hardware.post_script
        script_param = hardware.script_param

        if pre_script:
            timeout = 30
            if script_param:
                timeout = script_param.get("pre_script_timeout", timeout)
            self.run_custom_script(pre_script, timeout)

        flash_timeout = hardware.flash_timeout
        if hardware.flash_with_test:
            flash_timeout += self.get_test_timeout()

        serial_port = None
        if hardware.flash_before is False:
            serial_port = serial_device

        try:
            ser = self._create_serial_connection(
                hardware,
                serial_port,
                hardware.baud,
                flash_timeout,
                serial_pty,
                ser_pty_process
            )
        except serial.SerialException:
            return

        halt_monitor_evt = threading.Event()

        t = threading.Thread(target=self.monitor_serial, daemon=True,
                             args=(ser, halt_monitor_evt, harness))
        start_time = time.time()
        t.start()

        d_log = f"{self.instance.build_dir}/device.log"
        logger.debug(f'Flash command: {command}', )
        flash_error = False
        try:
            stdout = stderr = None
            with subprocess.Popen(command, stderr=subprocess.PIPE, stdout=subprocess.PIPE) as proc:
                try:
                    (stdout, stderr) = proc.communicate(timeout=flash_timeout)
                    # ignore unencodable unicode chars
                    logger.debug(stdout.decode(errors="ignore"))

                    if proc.returncode != 0:
                        self.instance.status = TwisterStatus.ERROR
                        self.instance.reason = "Device issue (Flash error?)"
                        flash_error = True
                        with open(d_log, "w") as dlog_fp:
                            dlog_fp.write(stderr.decode())
                        halt_monitor_evt.set()
                except subprocess.TimeoutExpired:
                    logger.warning("Flash operation timed out.")
                    self.terminate(proc)
                    (stdout, stderr) = proc.communicate()
                    self.instance.status = TwisterStatus.ERROR
                    self.instance.reason = "Device issue (Timeout)"
                    flash_error = True

            with open(d_log, "w") as dlog_fp:
                dlog_fp.write(stderr.decode())

        except subprocess.CalledProcessError:
            halt_monitor_evt.set()
            self.instance.status = TwisterStatus.ERROR
            self.instance.reason = "Device issue (Flash error)"
            flash_error = True

        if post_flash_script:
            timeout = 30
            if script_param:
                timeout = script_param.get("post_flash_timeout", timeout)
            self.run_custom_script(post_flash_script, timeout)

        # Connect to device after flashing it
        if hardware.flash_before:
            try:
                logger.debug(f"Attach serial device {serial_device} @ {hardware.baud} baud")
                ser.port = serial_device
                ser.open()
            except serial.SerialException as e:
                self._handle_serial_exception(e, hardware, serial_pty, ser_pty_process)
                return

        if not flash_error:
            # Always wait at most the test timeout here after flashing.
            t.join(self.get_test_timeout())
        else:
            # When the flash error is due exceptions,
            # twister tell the monitor serial thread
            # to close the serial. But it is necessary
            # for this thread being run first and close
            # have the change to close the serial.
            t.join(0.1)

        if t.is_alive():
            logger.debug(
                f"Timed out while monitoring serial output on {self.instance.platform.name}"
            )

        if ser.isOpen():
            ser.close()

        if serial_pty:
            self._terminate_pty(serial_pty, ser_pty_process)

        handler_time = time.time() - start_time

        self._update_instance_info(harness, handler_time, flash_error)

        self._final_handle_actions(harness, handler_time)

        if post_script:
            timeout = 30
            if script_param:
                timeout = script_param.get("post_script_timeout", timeout)
            self.run_custom_script(post_script, timeout)

        self.make_dut_available(hardware)


class QEMUHandler(Handler):
    """Spawns a thread to monitor QEMU output from pipes

    We pass QEMU_PIPE to 'make run' and monitor the pipes for output.
    We need to do this as once qemu starts, it runs forever until killed.
    Test cases emit special messages to the console as they run, we check
    for these to collect whether the test passed or failed.
    """

    def __init__(
        self,
        instance,
        type_str: str,
        options: argparse.Namespace,
        generator_cmd: str | None = None,
        suite_name_check: bool = True,
    ):
        """Constructor

        @param instance Test instance
        """

        super().__init__(instance, type_str, options, generator_cmd, suite_name_check)
        self.fifo_fn = os.path.join(instance.build_dir, "qemu-fifo")

        self.pid_fn = os.path.join(instance.build_dir, "qemu.pid")

        self.stdout_fn = os.path.join(instance.build_dir, "qemu.stdout")

        self.stderr_fn = os.path.join(instance.build_dir, "qemu.stderr")

        if instance.testsuite.ignore_qemu_crash:
            self.ignore_qemu_crash = True
            self.ignore_unexpected_eof = True
        else:
            self.ignore_qemu_crash = False
            self.ignore_unexpected_eof = False

    @staticmethod
    def _get_cpu_time(pid):
        """get process CPU time.

        The guest virtual time in QEMU icount mode isn't host time and
        it's maintained by counting guest instructions, so we use QEMU
        process execution time to mostly simulate the time of guest OS.
        """
        proc = psutil.Process(pid)
        cpu_time = proc.cpu_times()
        return cpu_time.user + cpu_time.system

    @staticmethod
    def _thread_get_fifo_names(fifo_fn):
        fifo_in = fifo_fn + ".in"
        fifo_out = fifo_fn + ".out"

        return fifo_in, fifo_out

    @staticmethod
    def _thread_update_instance_info(handler, handler_time, status, reason):
        handler.instance.execution_time = handler_time
        handler.instance.status = status
        if reason:
            handler.instance.reason = reason
        else:
            handler.instance.reason = "Unknown"

    @staticmethod
    def _thread(handler, timeout, outdir, logfile, fifo_fn, pid_fn,
                harness, ignore_unexpected_eof=False):
        fifo_in, fifo_out = QEMUHandler._thread_get_fifo_names(fifo_fn)

        # These in/out nodes are named from QEMU's perspective, not ours
        if os.path.exists(fifo_in):
            os.unlink(fifo_in)
        os.mkfifo(fifo_in)
        if os.path.exists(fifo_out):
            os.unlink(fifo_out)
        os.mkfifo(fifo_out)

        with (
            # We don't do anything with out_fp but we need to open it for
            # writing so that QEMU doesn't block, due to the way pipes work
            open(fifo_in, "wb") as _,
            # Disable internal buffering, we don't
            # want read() or poll() to ever block if there is data in there
            open(fifo_out, "rb", buffering=0) as in_fp,
            open(logfile, "w") as log_out_fp
        ):
            start_time = time.time()
            timeout_time = start_time + timeout
            p = select.poll()
            p.register(in_fp, select.POLLIN)
            _status = TwisterStatus.NONE
            _reason = None

            line = ""
            timeout_extended = False

            pid = 0
            if os.path.exists(pid_fn):
                with open(pid_fn) as pid_file:
                    pid = int(pid_file.read())

            while True:
                this_timeout = int((timeout_time - time.time()) * 1000)
                if timeout_extended:
                    # Quit early after timeout extension if no more data is being received
                    this_timeout = min(this_timeout, 1000)
                if this_timeout < 0 or not p.poll(this_timeout):
                    try:
                        if pid and this_timeout > 0:
                            # there's possibility we polled nothing because
                            # of not enough CPU time scheduled by host for
                            # QEMU process during p.poll(this_timeout)
                            cpu_time = QEMUHandler._get_cpu_time(pid)
                            if cpu_time < timeout and _status == TwisterStatus.NONE:
                                timeout_time = time.time() + (timeout - cpu_time)
                                continue
                    except psutil.NoSuchProcess:
                        pass
                    except ProcessLookupError:
                        _status = TwisterStatus.FAIL
                        _reason = "Execution error"
                        break

                    if _status == TwisterStatus.NONE:
                        _status = TwisterStatus.FAIL
                        _reason = "timeout"
                    break

                if pid == 0 and os.path.exists(pid_fn):
                    with open(pid_fn) as pid_file:
                        pid = int(pid_file.read())

                try:
                    c = in_fp.read(1).decode("utf-8")
                except UnicodeDecodeError:
                    # Test is writing something weird, fail
                    _status = TwisterStatus.FAIL
                    _reason = "unexpected byte"
                    break

                if c == "":
                    # EOF, this shouldn't happen unless QEMU crashes
                    if not ignore_unexpected_eof:
                        _status = TwisterStatus.FAIL
                        _reason = "unexpected eof"
                    break
                line = line + c
                if c != "\n":
                    continue

                # line contains a full line of data output from QEMU
                log_out_fp.write(strip_ansi_sequences(line))
                log_out_fp.flush()
                line = line.rstrip()
                logger.debug(f"QEMU ({pid}): {line}")

                harness.handle(line)
                if harness.status != TwisterStatus.NONE:
                    # if we have registered a fail make sure the status is not
                    # overridden by a false success message coming from the
                    # testsuite
                    if _status != TwisterStatus.FAIL:
                        _status = harness.status
                        _reason = harness.reason

                    # if we get some status, that means test is doing well, we reset
                    # the timeout and wait for 2 more seconds to catch anything
                    # printed late. We wait much longer if code
                    # coverage is enabled since dumping this information can
                    # take some time.
                    if not timeout_extended or harness.capture_coverage:
                        timeout_extended = True
                        if harness.capture_coverage:
                            timeout_time = time.time() + 30
                        else:
                            timeout_time = time.time() + 2
                line = ""

            handler_time = time.time() - start_time
            logger.debug(
                f"QEMU ({pid}) complete with {_status} ({_reason}) after {handler_time} seconds"
            )

            QEMUHandler._thread_update_instance_info(handler, handler_time, _status, _reason)

        if pid:
            # Oh well, as long as it's dead! User probably sent Ctrl-C
            with contextlib.suppress(ProcessLookupError, psutil.NoSuchProcess):
                if pid:
                    os.kill(pid, signal.SIGTERM)

        os.unlink(fifo_in)
        os.unlink(fifo_out)

    def _set_qemu_filenames(self, sysbuild_build_dir):
        # We pass this to QEMU which looks for fifos with .in and .out suffixes.
        # QEMU fifo will use main build dir
        self.fifo_fn = os.path.join(self.instance.build_dir, "qemu-fifo")
        # PID file will be created in the main sysbuild app's build dir
        self.pid_fn = os.path.join(sysbuild_build_dir, "qemu.pid")

        if os.path.exists(self.pid_fn):
            os.unlink(self.pid_fn)

        self.log_fn = self.log

    def _create_command(self, sysbuild_build_dir):
        command = [self.generator_cmd]
        command += ["-C", sysbuild_build_dir, "run"]

        return command

    def _update_instance_info(self, harness, is_timeout):
        if (self.returncode != 0 and not self.ignore_qemu_crash) or \
            harness.status == TwisterStatus.NONE:
            self.instance.status = TwisterStatus.FAIL
            if is_timeout:
                self.instance.reason = "Timeout"
            else:
                if not self.instance.reason:
                    self.instance.reason = f"Exited with {self.returncode}"
            self.instance.add_missing_case_status(TwisterStatus.BLOCK)

    def handle(self, harness):
        self.run = True

        domain_build_dir = self.get_default_domain_build_dir()

        command = self._create_command(domain_build_dir)

        self._set_qemu_filenames(domain_build_dir)

        self.thread = threading.Thread(name=self.name, target=QEMUHandler._thread,
                                       args=(self, self.get_test_timeout(), self.build_dir,
                                             self.log_fn, self.fifo_fn,
                                             self.pid_fn, harness,
                                             self.ignore_unexpected_eof))

        self.thread.daemon = True
        logger.debug(f"Spawning QEMUHandler Thread for {self.name}")
        self.thread.start()
        thread_max_time = time.time() + self.get_test_timeout()
        if sys.stdout.isatty():
            subprocess.call(["stty", "sane"], stdin=sys.stdout)

        logger.debug(f"Running {self.name} ({self.type_str})")

        is_timeout = False
        qemu_pid = None

        with open(self.stdout_fn, "w") as stdout_fp, \
            open(self.stderr_fn, "w") as stderr_fp, subprocess.Popen(
            command,
            stdout=stdout_fp,
            stderr=stderr_fp,
            cwd=self.build_dir
        ) as proc:
            logger.debug(f"Spawning QEMUHandler Thread for {self.name}")

            try:
                proc.wait(self.get_test_timeout())
            except subprocess.TimeoutExpired:
                # sometimes QEMU can't handle SIGTERM signal correctly
                # in that case kill -9 QEMU process directly and leave
                # twister to judge testing result by console output

                is_timeout = True
                self.terminate(proc)
                if harness.status == TwisterStatus.PASS:
                    self.returncode = 0
                else:
                    self.returncode = proc.returncode
            else:
                if os.path.exists(self.pid_fn):
                    with open(self.pid_fn) as pid_file:
                        qemu_pid = int(pid_file.read())
                logger.debug(f"No timeout, return code from QEMU ({qemu_pid}): {proc.returncode}")
                self.returncode = proc.returncode
            # Need to wait for harness to finish processing
            # output from QEMU. Otherwise it might miss some
            # messages.
            self.thread.join(max(thread_max_time - time.time(), 0))
            if self.thread.is_alive():
                logger.debug("Timed out while monitoring QEMU output")

            if os.path.exists(self.pid_fn):
                with open(self.pid_fn) as pid_file:
                    qemu_pid = int(pid_file.read())
                os.unlink(self.pid_fn)

        logger.debug(f"return code from QEMU ({qemu_pid}): {self.returncode}")

        self._update_instance_info(harness, is_timeout)

        self._final_handle_actions(harness, 0)

    def get_fifo(self):
        return self.fifo_fn


class QEMUWinHandler(Handler):
    """Spawns a thread to monitor QEMU output from pipes on Windows OS

     QEMU creates single duplex pipe at //.pipe/path, where path is fifo_fn.
     We need to do this as once qemu starts, it runs forever until killed.
     Test cases emit special messages to the console as they run, we check
     for these to collect whether the test passed or failed.
     """

    def __init__(
        self,
        instance,
        type_str: str,
        options: argparse.Namespace,
        generator_cmd: str | None = None,
        suite_name_check: bool = True,
    ):
        """Constructor

        @param instance Test instance
        """

        super().__init__(instance, type_str, options, generator_cmd, suite_name_check)
        self.pid_fn = os.path.join(instance.build_dir, "qemu.pid")
        self.fifo_fn = os.path.join(instance.build_dir, "qemu-fifo")
        self.pipe_handle = None
        self.pid = 0
        self.thread = None
        self.stop_thread = False

        if instance.testsuite.ignore_qemu_crash:
            self.ignore_qemu_crash = True
            self.ignore_unexpected_eof = True
        else:
            self.ignore_qemu_crash = False
            self.ignore_unexpected_eof = False

    @staticmethod
    def _get_cpu_time(pid):
        """get process CPU time.

        The guest virtual time in QEMU icount mode isn't host time and
        it's maintained by counting guest instructions, so we use QEMU
        process execution time to mostly simulate the time of guest OS.
        """
        proc = psutil.Process(pid)
        cpu_time = proc.cpu_times()
        return cpu_time.user + cpu_time.system

    @staticmethod
    def _open_log_file(logfile):
        return open(logfile, "w")

    @staticmethod
    def _close_log_file(log_file):
        log_file.close()

    @staticmethod
    def _stop_qemu_process(pid):
        if pid:
            try:
                if pid:
                    os.kill(pid, signal.SIGTERM)
            except (ProcessLookupError, psutil.NoSuchProcess):
                # Oh well, as long as it's dead! User probably sent Ctrl-C
                pass
            except OSError:
                pass

    @staticmethod
    def _monitor_update_instance_info(handler, handler_time, status, reason):
        handler.instance.execution_time = handler_time
        handler.instance.status = status
        if reason:
            handler.instance.reason = reason
        else:
            handler.instance.reason = "Unknown"

    def _set_qemu_filenames(self, sysbuild_build_dir):
        # PID file will be created in the main sysbuild app's build dir
        self.pid_fn = os.path.join(sysbuild_build_dir, "qemu.pid")

        if os.path.exists(self.pid_fn):
            os.unlink(self.pid_fn)

        self.log_fn = self.log

    def _create_command(self, sysbuild_build_dir):
        command = [self.generator_cmd]
        command += ["-C", sysbuild_build_dir, "run"]

        return command

    def _update_instance_info(self, harness, is_timeout):
        if (self.returncode != 0 and not self.ignore_qemu_crash) or \
            harness.status == TwisterStatus.NONE:
            self.instance.status = TwisterStatus.FAIL
            if is_timeout:
                self.instance.reason = "Timeout"
            else:
                if not self.instance.reason:
                    self.instance.reason = f"Exited with {self.returncode}"
            self.instance.add_missing_case_status(TwisterStatus.BLOCK)

    def _enqueue_char(self, queue):
        while not self.stop_thread:
            if not self.pipe_handle:
                try:
                    self.pipe_handle = os.open(r"\\.\pipe\\" + self.fifo_fn, os.O_RDONLY)
                except FileNotFoundError as e:
                    if e.args[0] == 2:
                        # Pipe is not opened yet, try again after a delay.
                        time.sleep(1)
                continue

            c = ""
            try:
                c = os.read(self.pipe_handle, 1)
            finally:
                queue.put(c)

    def _monitor_output(
        self,
        queue,
        timeout,
        logfile,
        pid_fn,
        harness,
        ignore_unexpected_eof=False
    ):
        start_time = time.time()
        timeout_time = start_time + timeout
        _status = TwisterStatus.NONE
        _reason = None
        line = ""
        timeout_extended = False
        self.pid = 0

        log_out_fp = self._open_log_file(logfile)

        while True:
            this_timeout = int((timeout_time - time.time()) * 1000)
            if this_timeout < 0:
                try:
                    if self.pid and this_timeout > 0:
                        # there's possibility we polled nothing because
                        # of not enough CPU time scheduled by host for
                        # QEMU process during p.poll(this_timeout)
                        cpu_time = self._get_cpu_time(self.pid)
                        if cpu_time < timeout and _status == TwisterStatus.NONE:
                            timeout_time = time.time() + (timeout - cpu_time)
                            continue
                except psutil.NoSuchProcess:
                    pass
                except ProcessLookupError:
                    _status = TwisterStatus.FAIL
                    _reason = "Execution error"
                    break

                if _status == TwisterStatus.NONE:
                    _status = TwisterStatus.FAIL
                    _reason = "timeout"
                break

            if self.pid == 0 and os.path.exists(pid_fn):
                # pid file probably not contains pid yet, continue
                with (
                    contextlib.suppress(ValueError),
                    open(pid_fn) as pid_file
                ):
                    self.pid = int(pid_file.read())

            try:
                c = queue.get_nowait()
            except Empty:
                continue
            try:
                c = c.decode("utf-8")
            except UnicodeDecodeError:
                # Test is writing something weird, fail
                _status = TwisterStatus.FAIL
                _reason = "unexpected byte"
                break

            if c == "":
                # EOF, this shouldn't happen unless QEMU crashes
                if not ignore_unexpected_eof:
                    _status = TwisterStatus.FAIL
                    _reason = "unexpected eof"
                break
            line = line + c
            if c != "\n":
                continue

            # line contains a full line of data output from QEMU
            log_out_fp.write(line)
            log_out_fp.flush()
            line = line.rstrip()
            logger.debug(f"QEMU ({self.pid}): {line}")

            harness.handle(line)
            if harness.status != TwisterStatus.NONE:
                # if we have registered a fail make sure the status is not
                # overridden by a false success message coming from the
                # testsuite
                if _status != TwisterStatus.FAIL:
                    _status = harness.status
                    _reason = harness.reason

                # if we get some status, that means test is doing well, we reset
                # the timeout and wait for 2 more seconds to catch anything
                # printed late. We wait much longer if code
                # coverage is enabled since dumping this information can
                # take some time.
                if not timeout_extended or harness.capture_coverage:
                    timeout_extended = True
                    if harness.capture_coverage:
                        timeout_time = time.time() + 30
                    else:
                        timeout_time = time.time() + 2
            line = ""

        self.stop_thread = True

        handler_time = time.time() - start_time
        logger.debug(
            f"QEMU ({self.pid}) complete with {_status} ({_reason}) after {handler_time} seconds"
        )
        self._monitor_update_instance_info(self, handler_time, _status, _reason)
        self._close_log_file(log_out_fp)
        self._stop_qemu_process(self.pid)

    def handle(self, harness):
        self.run = True

        domain_build_dir = self.get_default_domain_build_dir()
        command = self._create_command(domain_build_dir)
        self._set_qemu_filenames(domain_build_dir)

        logger.debug(f"Running {self.name} ({self.type_str})")
        is_timeout = False
        self.stop_thread = False
        queue = Queue()

        with subprocess.Popen(command, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT,
                              cwd=self.build_dir) as proc:
            logger.debug(f"Spawning QEMUHandler Thread for {self.name}")

            self.thread = threading.Thread(target=self._enqueue_char, args=(queue,))
            self.thread.daemon = True
            self.thread.start()

            thread_max_time = time.time() + self.get_test_timeout()

            self._monitor_output(queue, self.get_test_timeout(), self.log_fn, self.pid_fn, harness,
                                 self.ignore_unexpected_eof)

            if (thread_max_time - time.time()) < 0:
                logger.debug("Timed out while monitoring QEMU output")
                proc.terminate()
                # sleep for a while before attempting to kill
                time.sleep(0.5)
                proc.kill()

            if harness.status == TwisterStatus.PASS:
                self.returncode = 0
            else:
                self.returncode = proc.returncode

            if os.path.exists(self.pid_fn):
                os.unlink(self.pid_fn)

        logger.debug(f"return code from QEMU ({self.pid}): {self.returncode}")

        os.close(self.pipe_handle)
        self.pipe_handle = None

        self._update_instance_info(harness, is_timeout)

        self._final_handle_actions(harness, 0)

    def get_fifo(self):
        return self.fifo_fn
