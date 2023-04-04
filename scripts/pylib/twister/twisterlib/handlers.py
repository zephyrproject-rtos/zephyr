#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2022 Intel Corporation
# Copyright 2022 NXP
# SPDX-License-Identifier: Apache-2.0

import csv
import logging
import math
import os
import psutil
import re
import select
import shlex
import signal
import subprocess
import sys
import threading
import time

from queue import Queue, Empty
from twisterlib.environment import ZEPHYR_BASE
from twisterlib.error import TwisterException
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

SUPPORTED_SIMS = ["mdb-nsim", "nsim", "renode", "qemu", "tsim", "armfvp", "xt-sim", "native", "custom"]
SUPPORTED_SIMS_IN_PYTEST = ['native', 'qemu']


def terminate_process(proc):
    """
    encapsulate terminate functionality so we do it consistently where ever
    we might want to terminate the proc.  We need try_kill_process_by_pid
    because of both how newer ninja (1.6.0 or greater) and .NET / renode
    work.  Newer ninja's don't seem to pass SIGTERM down to the children
    so we need to use try_kill_process_by_pid.
    """

    for child in psutil.Process(proc.pid).children(recursive=True):
        try:
            os.kill(child.pid, signal.SIGTERM)
        except (ProcessLookupError, psutil.NoSuchProcess):
            pass
    proc.terminate()
    # sleep for a while before attempting to kill
    time.sleep(0.5)
    proc.kill()


class Handler:
    def __init__(self, instance, type_str="build"):
        """Constructor

        """
        self.options = None

        self.state = "waiting"
        self.run = False
        self.type_str = type_str

        self.binary = None
        self.pid_fn = None
        self.call_make_run = True

        self.name = instance.name
        self.instance = instance
        self.sourcedir = instance.testsuite.source_dir
        self.build_dir = instance.build_dir
        self.log = os.path.join(self.build_dir, "handler.log")
        self.returncode = 0
        self.generator = None
        self.generator_cmd = None
        self.suite_name_check = True
        self.ready = False

        self.args = []
        self.terminated = False

    def get_test_timeout(self):
        return math.ceil(self.instance.testsuite.timeout *
                         self.instance.platform.timeout_multiplier *
                         self.options.timeout_multiplier)

    def record(self, harness):
        if harness.recording:
            if self.instance.recording is None:
                self.instance.recording = harness.recording.copy()
            else:
                self.instance.recording.extend(harness.recording)

            filename = os.path.join(self.build_dir, "recording.csv")
            with open(filename, "at") as csvfile:
                cw = csv.DictWriter(csvfile,
                                    fieldnames = harness.recording[0].keys(),
                                    lineterminator = os.linesep,
                                    quoting = csv.QUOTE_NONNUMERIC)
                cw.writeheader()
                cw.writerows(harness.recording)

    def terminate(self, proc):
        terminate_process(proc)
        self.terminated = True

    def _verify_ztest_suite_name(self, harness_state, detected_suite_names, handler_time):
        """
        If test suite names was found in test's C source code, then verify if
        detected suite names from output correspond to expected suite names
        (and not in reverse).
        """
        expected_suite_names = self.instance.testsuite.ztest_suite_names
        logger.debug(f"Expected suite names:{expected_suite_names}")
        logger.debug(f"Detected suite names:{detected_suite_names}")
        if not expected_suite_names or \
                not harness_state == "passed":
            return
        if not detected_suite_names:
            self._missing_suite_name(expected_suite_names, handler_time)
        for detected_suite_name in detected_suite_names:
            if detected_suite_name not in expected_suite_names:
                self._missing_suite_name(expected_suite_names, handler_time)
                break

    def _missing_suite_name(self, expected_suite_names, handler_time):
        """
        Change result of performed test if problem with missing or unpropper
        suite name was occurred.
        """
        self.instance.status = "failed"
        self.instance.execution_time = handler_time
        for tc in self.instance.testcases:
            tc.status = "failed"
        self.instance.reason = f"Testsuite mismatch"
        logger.debug("Test suite names were not printed or some of them in " \
                     "output do not correspond with expected: %s",
                     str(expected_suite_names))

    def _final_handle_actions(self, harness, handler_time):

        # only for Ztest tests:
        harness_class_name = type(harness).__name__
        if self.suite_name_check and harness_class_name == "Test":
            self._verify_ztest_suite_name(harness.state, harness.detected_suite_names, handler_time)
            if self.instance.status == 'failed':
                return
            if not harness.matched_run_id and harness.run_id_exists:
                self.instance.status = "failed"
                self.instance.execution_time = handler_time
                self.instance.reason = "RunID mismatch"
                for tc in self.instance.testcases:
                    tc.status = "failed"

        self.record(harness)


class BinaryHandler(Handler):
    def __init__(self, instance, type_str):
        """Constructor

        @param instance Test Instance
        """
        super().__init__(instance, type_str)

        self.call_west_flash = False
        self.seed = None
        self.extra_test_args = None
        self.line = b""

    def try_kill_process_by_pid(self):
        if self.pid_fn:
            pid = int(open(self.pid_fn).read())
            os.unlink(self.pid_fn)
            self.pid_fn = None  # clear so we don't try to kill the binary twice
            try:
                os.kill(pid, signal.SIGKILL)
            except (ProcessLookupError, psutil.NoSuchProcess):
                pass

    def _output_reader(self, proc):
        self.line = proc.stdout.readline()

    def _output_handler(self, proc, harness):
        suffix = '\\r\\n'

        with open(self.log, "wt") as log_out_fp:
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
                    if line_decoded.endswith(suffix):
                        stripped_line = line_decoded[:-len(suffix)].rstrip()
                    else:
                        stripped_line = line_decoded.rstrip()
                    logger.debug("OUTPUT: %s", stripped_line)
                    log_out_fp.write(line_decoded)
                    log_out_fp.flush()
                    harness.handle(stripped_line)
                    if harness.state:
                        if not timeout_extended or harness.capture_coverage:
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
            command = [self.generator_cmd, "run_renode_test"]
        elif self.call_make_run:
            command = [self.generator_cmd, "run"]
        elif self.call_west_flash:
            command = ["west", "flash", "--skip-rebuild", "-d", self.build_dir]
        else:
            command = [self.binary]

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

    def _update_instance_info(self, harness_state, handler_time):
        self.instance.execution_time = handler_time
        if not self.terminated and self.returncode != 0:
            self.instance.status = "failed"
            if self.options.enable_valgrind and self.returncode == 2:
                self.instance.reason = "Valgrind error"
            else:
                # When a process is killed, the default handler returns 128 + SIGTERM
                # so in that case the return code itself is not meaningful
                self.instance.reason = "Failed"
        elif harness_state:
            self.instance.status = harness_state
            if harness_state == "failed":
                self.instance.reason = "Failed"
        else:
            self.instance.status = "failed"
            self.instance.reason = "Timeout"
            self.instance.add_missing_case_status("blocked", "Timeout")

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

        with subprocess.Popen(command, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, cwd=self.build_dir, env=env) as proc:
            logger.debug("Spawning BinaryHandler Thread for %s" % self.name)
            t = threading.Thread(target=self._output_handler, args=(proc, harness,), daemon=True)
            t.start()
            t.join()
            if t.is_alive():
                self.terminate(proc)
                t.join()
            proc.wait()
            self.returncode = proc.returncode
            self.try_kill_process_by_pid()

        handler_time = time.time() - start_time

        # FIXME: This is needed when killing the simulator, the console is
        # garbled and needs to be reset. Did not find a better way to do that.
        if sys.stdout.isatty():
            subprocess.call(["stty", "sane"], stdin=sys.stdout)

        self._update_instance_info(harness.state, handler_time)

        self._final_handle_actions(harness, handler_time)


class SimulationHandler(BinaryHandler):
    def __init__(self, instance, type_str):
        """Constructor

        @param instance Test Instance
        """
        super().__init__(instance, type_str)

        if type_str == 'renode':
            self.pid_fn = os.path.join(instance.build_dir, "renode.pid")
        elif type_str == 'native':
            self.call_make_run = False
            self.binary = os.path.join(instance.build_dir, "zephyr", "zephyr.exe")
            self.ready = True


class DeviceHandler(Handler):

    def __init__(self, instance, type_str):
        """Constructor

        @param instance Test Instance
        """
        super().__init__(instance, type_str)

    def get_test_timeout(self):
        timeout = super().get_test_timeout()
        if self.options.enable_coverage:
            # wait more for gcov data to be dumped on console
            timeout += 120
        return timeout

    def monitor_serial(self, ser, halt_event, harness):
        log_out_fp = open(self.log, "wb")

        if self.options.enable_coverage:
            # Set capture_coverage to True to indicate that right after
            # test results we should get coverage data, otherwise we exit
            # from the test.
            harness.capture_coverage = True

        # Clear serial leftover.
        ser.reset_input_buffer()

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
            try:
                serial_line = ser.readline()
            except TypeError:
                pass
            # ignore SerialException which may happen during the serial device
            # power off/on process.
            except serial.SerialException:
                pass

            # Just because ser_fileno has data doesn't mean an entire line
            # is available yet.
            if serial_line:
                sl = serial_line.decode('utf-8', 'ignore').lstrip()
                logger.debug("DEVICE: {0}".format(sl.rstrip()))

                log_out_fp.write(sl.encode('utf-8'))
                log_out_fp.flush()
                harness.handle(sl.rstrip())

            if harness.state:
                if not harness.capture_coverage:
                    ser.close()
                    break

        log_out_fp.close()

    def device_is_available(self, instance):
        device = instance.platform.name
        fixture = instance.testsuite.harness_config.get("fixture")
        dut_found = False

        for d in self.duts:
            if fixture and fixture not in d.fixtures:
                continue
            if d.platform != device or (d.serial is None and d.serial_pty is None):
                continue
            dut_found = True
            d.lock.acquire()
            avail = False
            if d.available:
                d.available = 0
                d.counter += 1
                avail = True
            d.lock.release()
            if avail:
                return d

        if not dut_found:
            raise TwisterException(f"No device to serve as {device} platform.")

        return None

    def make_device_available(self, serial):
        for d in self.duts:
            if serial in [d.serial_pty, d.serial]:
                d.available = 1

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
                logger.error("{} timed out".format(script))

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
                if board_id is not None:
                    if runner in ("pyocd", "nrfjprog"):
                        command_extra_args.append("--dev-id")
                        command_extra_args.append(board_id)
                    elif runner == "openocd" and product == "STM32 STLink":
                        command_extra_args.append("--cmd-pre-init")
                        command_extra_args.append("hla_serial %s" % board_id)
                    elif runner == "openocd" and product == "STLINK-V3":
                        command_extra_args.append("--cmd-pre-init")
                        command_extra_args.append("hla_serial %s" % board_id)
                    elif runner == "openocd" and product == "EDBG CMSIS-DAP":
                        command_extra_args.append("--cmd-pre-init")
                        command_extra_args.append("cmsis_dap_serial %s" % board_id)
                    elif runner == "jlink":
                        command.append("--tool-opt=-SelectEmuBySN  %s" % board_id)
                    elif runner == "stm32cubeprogrammer":
                        command.append("--tool-opt=sn=%s" % board_id)

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

    def _update_instance_info(self, harness_state, handler_time, flash_error):
        self.instance.execution_time = handler_time
        if harness_state:
            self.instance.status = harness_state
            if harness_state == "failed":
                self.instance.reason = "Failed"
            self.instance.add_missing_case_status("blocked", harness_state)
        elif not flash_error:
            self.instance.status = "failed"
            self.instance.reason = "Timeout"

        if self.instance.status in ["error", "failed"]:
            self.instance.add_missing_case_status("blocked", self.instance.reason)

    def _create_serial_connection(self, serial_device, hardware_baud,
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
            self.instance.status = "failed"
            self.instance.reason = "Serial Device Error"
            logger.error("Serial device error: %s" % (str(e)))

            self.instance.add_missing_case_status("blocked", "Serial Device Error")
            if serial_pty and ser_pty_process:
                ser_pty_process.terminate()
                outs, errs = ser_pty_process.communicate()
                logger.debug("Process {} terminated outs: {} errs {}".format(serial_pty, outs, errs))

            if serial_pty:
                self.make_device_available(serial_pty)
            else:
                self.make_device_available(serial_device)
            raise

        return ser

    def get_hardware(self):
        hardware = None
        try:
            hardware = self.device_is_available(self.instance)
            while not hardware:
                time.sleep(1)
                hardware = self.device_is_available(self.instance)
        except TwisterException as error:
            self.instance.status = "failed"
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
                    "Failed to run subprocess {}, error {}".format(
                        serial_pty,
                        error.output
                    )
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
        if not hardware:
            return

        runner = hardware.runner or self.options.west_runner
        serial_pty = hardware.serial_pty

        serial_device, ser_pty_process = self._get_serial_device(serial_pty, hardware.serial)

        logger.debug(f"Using serial device {serial_device} @ {hardware.baud} baud")

        command = self._create_command(runner, hardware)

        pre_script = hardware.pre_script
        post_flash_script = hardware.post_flash_script
        post_script = hardware.post_script

        if pre_script:
            self.run_custom_script(pre_script, 30)

        flash_timeout = hardware.flash_timeout
        if hardware.flash_with_test:
            flash_timeout += self.get_test_timeout()

        try:
            ser = self._create_serial_connection(
                serial_device,
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

        d_log = "{}/device.log".format(self.instance.build_dir)
        logger.debug('Flash command: %s', command)
        flash_error = False
        try:
            stdout = stderr = None
            with subprocess.Popen(command, stderr=subprocess.PIPE, stdout=subprocess.PIPE) as proc:
                try:
                    (stdout, stderr) = proc.communicate(timeout=flash_timeout)
                    # ignore unencodable unicode chars
                    logger.debug(stdout.decode(errors="ignore"))

                    if proc.returncode != 0:
                        self.instance.status = "error"
                        self.instance.reason = "Device issue (Flash error?)"
                        flash_error = True
                        with open(d_log, "w") as dlog_fp:
                            dlog_fp.write(stderr.decode())
                        halt_monitor_evt.set()
                except subprocess.TimeoutExpired:
                    logger.warning("Flash operation timed out.")
                    self.terminate(proc)
                    (stdout, stderr) = proc.communicate()
                    self.instance.status = "error"
                    self.instance.reason = "Device issue (Timeout)"
                    flash_error = True

            with open(d_log, "w") as dlog_fp:
                dlog_fp.write(stderr.decode())

        except subprocess.CalledProcessError:
            halt_monitor_evt.set()
            self.instance.status = "error"
            self.instance.reason = "Device issue (Flash error)"
            flash_error = True

        if post_flash_script:
            self.run_custom_script(post_flash_script, 30)

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
            logger.debug("Timed out while monitoring serial output on {}".format(self.instance.platform.name))

        if ser.isOpen():
            ser.close()

        if serial_pty:
            ser_pty_process.terminate()
            outs, errs = ser_pty_process.communicate()
            logger.debug("Process {} terminated outs: {} errs {}".format(serial_pty, outs, errs))

        handler_time = time.time() - start_time

        self._update_instance_info(harness.state, handler_time, flash_error)

        self._final_handle_actions(harness, handler_time)

        if post_script:
            self.run_custom_script(post_script, 30)

        if serial_pty:
            self.make_device_available(serial_pty)
        else:
            self.make_device_available(serial_device)


class QEMUHandler(Handler):
    """Spawns a thread to monitor QEMU output from pipes

    We pass QEMU_PIPE to 'make run' and monitor the pipes for output.
    We need to do this as once qemu starts, it runs forever until killed.
    Test cases emit special messages to the console as they run, we check
    for these to collect whether the test passed or failed.
    """

    def __init__(self, instance, type_str):
        """Constructor

        @param instance Test instance
        """

        super().__init__(instance, type_str)
        self.fifo_fn = os.path.join(instance.build_dir, "qemu-fifo")

        self.pid_fn = os.path.join(instance.build_dir, "qemu.pid")

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
    def _thread_open_files(fifo_in, fifo_out, logfile):
        # These in/out nodes are named from QEMU's perspective, not ours
        if os.path.exists(fifo_in):
            os.unlink(fifo_in)
        os.mkfifo(fifo_in)
        if os.path.exists(fifo_out):
            os.unlink(fifo_out)
        os.mkfifo(fifo_out)

        # We don't do anything with out_fp but we need to open it for
        # writing so that QEMU doesn't block, due to the way pipes work
        out_fp = open(fifo_in, "wb")
        # Disable internal buffering, we don't
        # want read() or poll() to ever block if there is data in there
        in_fp = open(fifo_out, "rb", buffering=0)
        log_out_fp = open(logfile, "wt")

        return out_fp, in_fp, log_out_fp

    @staticmethod
    def _thread_close_files(fifo_in, fifo_out, pid, out_fp, in_fp, log_out_fp):
        log_out_fp.close()
        out_fp.close()
        in_fp.close()
        if pid:
            try:
                if pid:
                    os.kill(pid, signal.SIGTERM)
            except (ProcessLookupError, psutil.NoSuchProcess):
                # Oh well, as long as it's dead! User probably sent Ctrl-C
                pass

        os.unlink(fifo_in)
        os.unlink(fifo_out)

    @staticmethod
    def _thread_update_instance_info(handler, handler_time, out_state):
        handler.instance.execution_time = handler_time
        if out_state == "timeout":
            handler.instance.status = "failed"
            handler.instance.reason = "Timeout"
        elif out_state == "failed":
            handler.instance.status = "failed"
            handler.instance.reason = "Failed"
        elif out_state in ['unexpected eof', 'unexpected byte']:
            handler.instance.status = "failed"
            handler.instance.reason = out_state
        else:
            handler.instance.status = out_state
            handler.instance.reason = "Unknown"

    @staticmethod
    def _thread(handler, timeout, outdir, logfile, fifo_fn, pid_fn, results,
                harness, ignore_unexpected_eof=False):
        fifo_in, fifo_out = QEMUHandler._thread_get_fifo_names(fifo_fn)

        out_fp, in_fp, log_out_fp = QEMUHandler._thread_open_files(
            fifo_in,
            fifo_out,
            logfile
        )

        start_time = time.time()
        timeout_time = start_time + timeout
        p = select.poll()
        p.register(in_fp, select.POLLIN)
        out_state = None

        line = ""
        timeout_extended = False

        pid = 0
        if os.path.exists(pid_fn):
            pid = int(open(pid_fn).read())

        while True:
            this_timeout = int((timeout_time - time.time()) * 1000)
            if this_timeout < 0 or not p.poll(this_timeout):
                try:
                    if pid and this_timeout > 0:
                        # there's possibility we polled nothing because
                        # of not enough CPU time scheduled by host for
                        # QEMU process during p.poll(this_timeout)
                        cpu_time = QEMUHandler._get_cpu_time(pid)
                        if cpu_time < timeout and not out_state:
                            timeout_time = time.time() + (timeout - cpu_time)
                            continue
                except psutil.NoSuchProcess:
                    pass
                except ProcessLookupError:
                    out_state = "failed"
                    break

                if not out_state:
                    out_state = "timeout"
                break

            if pid == 0 and os.path.exists(pid_fn):
                pid = int(open(pid_fn).read())

            try:
                c = in_fp.read(1).decode("utf-8")
            except UnicodeDecodeError:
                # Test is writing something weird, fail
                out_state = "unexpected byte"
                break

            if c == "":
                # EOF, this shouldn't happen unless QEMU crashes
                if not ignore_unexpected_eof:
                    out_state = "unexpected eof"
                break
            line = line + c
            if c != "\n":
                continue

            # line contains a full line of data output from QEMU
            log_out_fp.write(line)
            log_out_fp.flush()
            line = line.rstrip()
            logger.debug(f"QEMU ({pid}): {line}")

            harness.handle(line)
            if harness.state:
                # if we have registered a fail make sure the state is not
                # overridden by a false success message coming from the
                # testsuite
                if out_state not in ['failed', 'unexpected eof', 'unexpected byte']:
                    out_state = harness.state

                # if we get some state, that means test is doing well, we reset
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
        logger.debug(f"QEMU ({pid}) complete ({out_state}) after {handler_time} seconds")

        QEMUHandler._thread_update_instance_info(handler, handler_time, out_state)

        QEMUHandler._thread_close_files(fifo_in, fifo_out, pid, out_fp, in_fp, log_out_fp)

    def _get_sysbuild_build_dir(self):
        if self.instance.testsuite.sysbuild:
            # Load domain yaml to get default domain build directory
            # Note: for targets using QEMU, we assume that the target will
            # have added any additional images to the run target manually
            domain_path = os.path.join(self.build_dir, "domains.yaml")
            domains = Domains.from_file(domain_path)
            logger.debug("Loaded sysbuild domain data from %s" % domain_path)
            build_dir = domains.get_default_domain().build_dir
        else:
            build_dir = self.build_dir

        return build_dir

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

    def _update_instance_info(self, harness_state, is_timeout):
        if (self.returncode != 0 and not self.ignore_qemu_crash) or not harness_state:
            self.instance.status = "failed"
            if is_timeout:
                self.instance.reason = "Timeout"
            else:
                if not self.instance.reason:
                    self.instance.reason = "Exited with {}".format(self.returncode)
            self.instance.add_missing_case_status("blocked")

    def handle(self, harness):
        self.results = {}
        self.run = True

        sysbuild_build_dir = self._get_sysbuild_build_dir()

        command = self._create_command(sysbuild_build_dir)

        self._set_qemu_filenames(sysbuild_build_dir)

        self.thread = threading.Thread(name=self.name, target=QEMUHandler._thread,
                                       args=(self, self.get_test_timeout(), self.build_dir,
                                             self.log_fn, self.fifo_fn,
                                             self.pid_fn, self.results, harness,
                                             self.ignore_unexpected_eof))

        self.thread.daemon = True
        logger.debug("Spawning QEMUHandler Thread for %s" % self.name)
        self.thread.start()
        thread_max_time = time.time() + self.get_test_timeout()
        if sys.stdout.isatty():
            subprocess.call(["stty", "sane"], stdin=sys.stdout)

        logger.debug("Running %s (%s)" % (self.name, self.type_str))

        is_timeout = False
        qemu_pid = None

        with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=self.build_dir) as proc:
            logger.debug("Spawning QEMUHandler Thread for %s" % self.name)

            try:
                proc.wait(self.get_test_timeout())
            except subprocess.TimeoutExpired:
                # sometimes QEMU can't handle SIGTERM signal correctly
                # in that case kill -9 QEMU process directly and leave
                # twister to judge testing result by console output

                is_timeout = True
                self.terminate(proc)
                if harness.state == "passed":
                    self.returncode = 0
                else:
                    self.returncode = proc.returncode
            else:
                if os.path.exists(self.pid_fn):
                    qemu_pid = int(open(self.pid_fn).read())
                logger.debug(f"No timeout, return code from QEMU ({qemu_pid}): {proc.returncode}")
                self.returncode = proc.returncode
            # Need to wait for harness to finish processing
            # output from QEMU. Otherwise it might miss some
            # messages.
            self.thread.join(max(thread_max_time - time.time(), 0))
            if self.thread.is_alive():
                logger.debug("Timed out while monitoring QEMU output")

            if os.path.exists(self.pid_fn):
                qemu_pid = int(open(self.pid_fn).read())
                os.unlink(self.pid_fn)

        logger.debug(f"return code from QEMU ({qemu_pid}): {self.returncode}")

        self._update_instance_info(harness.state, is_timeout)

        self._final_handle_actions(harness, 0)

    def get_fifo(self):
        return self.fifo_fn


class QEMUWinHandler(Handler):
    """Spawns a thread to monitor QEMU output on Windows OS

    We redirect subprocess output to pipe and monitor the pipes for output.
    We need to do this as once qemu starts, it runs forever until killed.
    Test cases emit special messages to the console as they run, we check
    for these to collect whether the test passed or failed.
    The pipe includes also messages from ninja command which is used for
    running QEMU.
    """

    def __init__(self, instance, type_str):
        """Constructor

        @param instance Test instance
        """

        super().__init__(instance, type_str)
        self.pid_fn = os.path.join(instance.build_dir, "qemu.pid")
        self.pid = 0
        self.thread = None
        self.stop_thread = False
        self.results = {}

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
        return open(logfile, "wt")

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

    @staticmethod
    def _monitor_update_instance_info(handler, handler_time, out_state):
        handler.instance.execution_time = handler_time
        if out_state == "timeout":
            handler.instance.status = "failed"
            handler.instance.reason = "Timeout"
        elif out_state == "failed":
            handler.instance.status = "failed"
            handler.instance.reason = "Failed"
        elif out_state in ['unexpected eof', 'unexpected byte']:
            handler.instance.status = "failed"
            handler.instance.reason = out_state
        else:
            handler.instance.status = out_state
            handler.instance.reason = "Unknown"

    def _get_sysbuild_build_dir(self):
        if self.instance.testsuite.sysbuild:
            # Load domain yaml to get default domain build directory
            # Note: for targets using QEMU, we assume that the target will
            # have added any additional images to the run target manually
            domain_path = os.path.join(self.build_dir, "domains.yaml")
            domains = Domains.from_file(domain_path)
            logger.debug("Loaded sysbuild domain data from %s" % domain_path)
            build_dir = domains.get_default_domain().build_dir
        else:
            build_dir = self.build_dir

        return build_dir

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

    def _update_instance_info(self, harness_state, is_timeout):
        if (self.returncode != 0 and not self.ignore_qemu_crash) or not harness_state:
            self.instance.status = "failed"
            if is_timeout:
                self.instance.reason = "Timeout"
            else:
                if not self.instance.reason:
                    self.instance.reason = "Exited with {}".format(self.returncode)
            self.instance.add_missing_case_status("blocked")

    def _enqueue_char(self, stdout, queue):
        while not self.stop_thread:
            try:
                c = stdout.read(1)
            except ValueError:
                # Reading on closed file exception can occur when subprocess is killed.
                # Can be ignored.
                pass
            else:
                queue.put(c)

    def _monitor_output(self, queue, timeout, logfile, pid_fn, harness, ignore_unexpected_eof=False):
        start_time = time.time()
        timeout_time = start_time + timeout
        out_state = None
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
                        if cpu_time < timeout and not out_state:
                            timeout_time = time.time() + (timeout - cpu_time)
                            continue
                except psutil.NoSuchProcess:
                    pass
                except ProcessLookupError:
                    out_state = "failed"
                    break

                if not out_state:
                    out_state = "timeout"
                break

            if self.pid == 0 and os.path.exists(pid_fn):
                try:
                    self.pid = int(open(pid_fn).read())
                except ValueError:
                    # pid file probably not contains pid yet, continue
                    pass

            try:
                c = queue.get_nowait()
            except Empty:
                continue
            try:
                c = c.decode("utf-8")
            except UnicodeDecodeError:
                # Test is writing something weird, fail
                out_state = "unexpected byte"
                break

            if c == "":
                # EOF, this shouldn't happen unless QEMU crashes
                if not ignore_unexpected_eof:
                    out_state = "unexpected eof"
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
            if harness.state:
                # if we have registered a fail make sure the state is not
                # overridden by a false success message coming from the
                # testsuite
                if out_state not in ['failed', 'unexpected eof', 'unexpected byte']:
                    out_state = harness.state

                # if we get some state, that means test is doing well, we reset
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
        logger.debug(f"QEMU ({self.pid}) complete ({out_state}) after {handler_time} seconds")
        self._monitor_update_instance_info(self, handler_time, out_state)
        self._close_log_file(log_out_fp)
        self._stop_qemu_process(self.pid)

    def handle(self, harness):
        self.results = {}
        self.run = True

        sysbuild_build_dir = self._get_sysbuild_build_dir()
        command = self._create_command(sysbuild_build_dir)
        self._set_qemu_filenames(sysbuild_build_dir)

        logger.debug("Running %s (%s)" % (self.name, self.type_str))
        is_timeout = False
        self.stop_thread = False
        queue = Queue()

        with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=self.build_dir) as proc:
            logger.debug("Spawning QEMUHandler Thread for %s" % self.name)

            self.thread = threading.Thread(target=self._enqueue_char, args=(proc.stdout, queue))
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

            if harness.state == "passed":
                self.returncode = 0
            else:
                self.returncode = proc.returncode

            if os.path.exists(self.pid_fn):
                os.unlink(self.pid_fn)

        logger.debug(f"return code from QEMU ({self.pid}): {self.returncode}")

        self._update_instance_info(harness.state, is_timeout)

        self._final_handle_actions(harness, 0)
