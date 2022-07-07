#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
import os
import contextlib
import string
import mmap
import math
import sys
import re
import subprocess
import select
import shutil
import shlex
import signal
import hashlib
import threading
from datetime import datetime
from collections import OrderedDict
import queue
import time
import csv
import glob
import random
import xml.etree.ElementTree as ET
import logging
from pathlib import Path
from distutils.spawn import find_executable
import colorama
from colorama import Fore
import pickle
import platform
import yaml
import json
from multiprocessing import Lock, Process, Value
from typing import List

try:
    # Use the C LibYAML parser if available, rather than the Python parser.
    # It's much faster.
    from yaml import CSafeLoader as SafeLoader
    from yaml import CDumper as Dumper
except ImportError:
    from yaml import SafeLoader, Dumper

try:
    import serial
except ImportError:
    print("Install pyserial python module with pip to use --device-testing option.")

try:
    from tabulate import tabulate
except ImportError:
    print("Install tabulate python module with pip to use --device-testing option.")

try:
    import psutil
except ImportError:
    print("Install psutil python module with pip to run in Qemu.")

try:
    import pty
except ImportError as capture_error:
    if os.name == "nt":  # "nt" means that program is running on Windows OS
        pass  # "--device-serial-pty" option is not supported on Windows OS
    else:
        raise capture_error

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
if not ZEPHYR_BASE:
    sys.exit("$ZEPHYR_BASE environment variable undefined")

# This is needed to load edt.pickle files.
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "dts",
                                "python-devicetree", "src"))
from devicetree import edtlib  # pylint: disable=unused-import

# Use this for internal comparisons; that's what canonicalization is
# for. Don't use it when invoking other components of the build system
# to avoid confusing and hard to trace inconsistencies in error messages
# and logs, generated Makefiles, etc. compared to when users invoke these
# components directly.
# Note "normalization" is different from canonicalization, see os.path.
canonical_zephyr_base = os.path.realpath(ZEPHYR_BASE)

sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/"))

import scl
import expr_parser

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)


class ExecutionCounter(object):
    def __init__(self, total=0):
        self._done = Value('i', 0)
        self._passed = Value('i', 0)
        self._skipped_configs = Value('i', 0)
        self._skipped_runtime = Value('i', 0)
        self._skipped_filter = Value('i', 0)
        self._skipped_cases = Value('i', 0)
        self._error = Value('i', 0)
        self._failed = Value('i', 0)
        self._total = Value('i', total)
        self._cases = Value('i', 0)


        self.lock = Lock()


    def summary(self):
        logger.debug("--------------------------------")
        logger.debug(f"Total Test suites: {self.total}")
        logger.debug(f"Total Test cases: {self.cases}")
        logger.debug(f"Skipped test cases: {self.skipped_cases}")
        logger.debug(f"Completed Testsuites: {self.done}")
        logger.debug(f"Passing Testsuites: {self.passed}")
        logger.debug(f"Failing Testsuites: {self.failed}")
        logger.debug(f"Skipped Testsuites: {self.skipped_configs}")
        logger.debug(f"Skipped Testsuites (runtime): {self.skipped_runtime}")
        logger.debug(f"Skipped Testsuites (filter): {self.skipped_filter}")
        logger.debug(f"Errors: {self.error}")
        logger.debug("--------------------------------")

    @property
    def cases(self):
        with self._cases.get_lock():
            return self._cases.value

    @cases.setter
    def cases(self, value):
        with self._cases.get_lock():
            self._cases.value = value

    @property
    def skipped_cases(self):
        with self._skipped_cases.get_lock():
            return self._skipped_cases.value

    @skipped_cases.setter
    def skipped_cases(self, value):
        with self._skipped_cases.get_lock():
            self._skipped_cases.value = value

    @property
    def error(self):
        with self._error.get_lock():
            return self._error.value

    @error.setter
    def error(self, value):
        with self._error.get_lock():
            self._error.value = value

    @property
    def done(self):
        with self._done.get_lock():
            return self._done.value

    @done.setter
    def done(self, value):
        with self._done.get_lock():
            self._done.value = value

    @property
    def passed(self):
        with self._passed.get_lock():
            return self._passed.value

    @passed.setter
    def passed(self, value):
        with self._passed.get_lock():
            self._passed.value = value

    @property
    def skipped_configs(self):
        with self._skipped_configs.get_lock():
            return self._skipped_configs.value

    @skipped_configs.setter
    def skipped_configs(self, value):
        with self._skipped_configs.get_lock():
            self._skipped_configs.value = value

    @property
    def skipped_filter(self):
        with self._skipped_filter.get_lock():
            return self._skipped_filter.value

    @skipped_filter.setter
    def skipped_filter(self, value):
        with self._skipped_filter.get_lock():
            self._skipped_filter.value = value

    @property
    def skipped_runtime(self):
        with self._skipped_runtime.get_lock():
            return self._skipped_runtime.value

    @skipped_runtime.setter
    def skipped_runtime(self, value):
        with self._skipped_runtime.get_lock():
            self._skipped_runtime.value = value

    @property
    def failed(self):
        with self._failed.get_lock():
            return self._failed.value

    @failed.setter
    def failed(self, value):
        with self._failed.get_lock():
            self._failed.value = value

    @property
    def total(self):
        with self._total.get_lock():
            return self._total.value

class CMakeCacheEntry:
    '''Represents a CMake cache entry.

    This class understands the type system in a CMakeCache.txt, and
    converts the following cache types to Python types:

    Cache Type    Python type
    ----------    -------------------------------------------
    FILEPATH      str
    PATH          str
    STRING        str OR list of str (if ';' is in the value)
    BOOL          bool
    INTERNAL      str OR list of str (if ';' is in the value)
    ----------    -------------------------------------------
    '''

    # Regular expression for a cache entry.
    #
    # CMake variable names can include escape characters, allowing a
    # wider set of names than is easy to match with a regular
    # expression. To be permissive here, use a non-greedy match up to
    # the first colon (':'). This breaks if the variable name has a
    # colon inside, but it's good enough.
    CACHE_ENTRY = re.compile(
        r'''(?P<name>.*?)                               # name
         :(?P<type>FILEPATH|PATH|STRING|BOOL|INTERNAL)  # type
         =(?P<value>.*)                                 # value
        ''', re.X)

    @classmethod
    def _to_bool(cls, val):
        # Convert a CMake BOOL string into a Python bool.
        #
        #   "True if the constant is 1, ON, YES, TRUE, Y, or a
        #   non-zero number. False if the constant is 0, OFF, NO,
        #   FALSE, N, IGNORE, NOTFOUND, the empty string, or ends in
        #   the suffix -NOTFOUND. Named boolean constants are
        #   case-insensitive. If the argument is not one of these
        #   constants, it is treated as a variable."
        #
        # https://cmake.org/cmake/help/v3.0/command/if.html
        val = val.upper()
        if val in ('ON', 'YES', 'TRUE', 'Y'):
            return 1
        elif val in ('OFF', 'NO', 'FALSE', 'N', 'IGNORE', 'NOTFOUND', ''):
            return 0
        elif val.endswith('-NOTFOUND'):
            return 0
        else:
            try:
                v = int(val)
                return v != 0
            except ValueError as exc:
                raise ValueError('invalid bool {}'.format(val)) from exc

    @classmethod
    def from_line(cls, line, line_no):
        # Comments can only occur at the beginning of a line.
        # (The value of an entry could contain a comment character).
        if line.startswith('//') or line.startswith('#'):
            return None

        # Whitespace-only lines do not contain cache entries.
        if not line.strip():
            return None

        m = cls.CACHE_ENTRY.match(line)
        if not m:
            return None

        name, type_, value = (m.group(g) for g in ('name', 'type', 'value'))
        if type_ == 'BOOL':
            try:
                value = cls._to_bool(value)
            except ValueError as exc:
                args = exc.args + ('on line {}: {}'.format(line_no, line),)
                raise ValueError(args) from exc
        elif type_ in ['STRING', 'INTERNAL']:
            # If the value is a CMake list (i.e. is a string which
            # contains a ';'), convert to a Python list.
            if ';' in value:
                value = value.split(';')

        return CMakeCacheEntry(name, value)

    def __init__(self, name, value):
        self.name = name
        self.value = value

    def __str__(self):
        fmt = 'CMakeCacheEntry(name={}, value={})'
        return fmt.format(self.name, self.value)


class CMakeCache:
    '''Parses and represents a CMake cache file.'''

    @staticmethod
    def from_file(cache_file):
        return CMakeCache(cache_file)

    def __init__(self, cache_file):
        self.cache_file = cache_file
        self.load(cache_file)

    def load(self, cache_file):
        entries = []
        with open(cache_file, 'r') as cache:
            for line_no, line in enumerate(cache):
                entry = CMakeCacheEntry.from_line(line, line_no)
                if entry:
                    entries.append(entry)
        self._entries = OrderedDict((e.name, e) for e in entries)

    def get(self, name, default=None):
        entry = self._entries.get(name)
        if entry is not None:
            return entry.value
        else:
            return default

    def get_list(self, name, default=None):
        if default is None:
            default = []
        entry = self._entries.get(name)
        if entry is not None:
            value = entry.value
            if isinstance(value, list):
                return value
            elif isinstance(value, str):
                return [value] if value else []
            else:
                msg = 'invalid value {} type {}'
                raise RuntimeError(msg.format(value, type(value)))
        else:
            return default

    def __contains__(self, name):
        return name in self._entries

    def __getitem__(self, name):
        return self._entries[name].value

    def __setitem__(self, name, entry):
        if not isinstance(entry, CMakeCacheEntry):
            msg = 'improper type {} for value {}, expecting CMakeCacheEntry'
            raise TypeError(msg.format(type(entry), entry))
        self._entries[name] = entry

    def __delitem__(self, name):
        del self._entries[name]

    def __iter__(self):
        return iter(self._entries.values())


class TwisterException(Exception):
    pass


class TwisterRuntimeError(TwisterException):
    pass


class ConfigurationError(TwisterException):
    def __init__(self, cfile, message):
        TwisterException.__init__(self, cfile + ": " + message)


class BuildError(TwisterException):
    pass


class ExecutionError(TwisterException):
    pass


class HarnessImporter:

    def __init__(self, name):
        sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))
        module = __import__("harness")
        if name:
            my_class = getattr(module, name)
        else:
            my_class = getattr(module, "Test")

        self.instance = my_class()


class Handler:
    def __init__(self, instance, type_str="build"):
        """Constructor

        """
        self.state = "waiting"
        self.run = False
        self.type_str = type_str

        self.binary = None
        self.pid_fn = None
        self.call_make_run = False

        self.name = instance.name
        self.instance = instance
        self.timeout = math.ceil(instance.testsuite.timeout * instance.platform.timeout_multiplier)
        self.sourcedir = instance.testsuite.source_dir
        self.build_dir = instance.build_dir
        self.log = os.path.join(self.build_dir, "handler.log")
        self.returncode = 0
        self.generator = None
        self.generator_cmd = None
        self.suite_name_check = True

        self.args = []
        self.terminated = False

    def record(self, harness):
        if harness.recording:
            filename = os.path.join(self.build_dir, "recording.csv")
            with open(filename, "at") as csvfile:
                cw = csv.writer(csvfile, harness.fieldnames, lineterminator=os.linesep)
                cw.writerow(harness.fieldnames)
                for instance in harness.recording:
                    cw.writerow(instance)

    def terminate(self, proc):
        # encapsulate terminate functionality so we do it consistently where ever
        # we might want to terminate the proc.  We need try_kill_process_by_pid
        # because of both how newer ninja (1.6.0 or greater) and .NET / renode
        # work.  Newer ninja's don't seem to pass SIGTERM down to the children
        # so we need to use try_kill_process_by_pid.
        for child in psutil.Process(proc.pid).children(recursive=True):
            try:
                os.kill(child.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
        proc.terminate()
        # sleep for a while before attempting to kill
        time.sleep(0.5)
        proc.kill()
        self.terminated = True

    def _verify_ztest_suite_name(self, harness_state, detected_suite_names, handler_time):
        """
        If test suite names was found in test's C source code, then verify if
        detected suite names from output correspond to expected suite names
        (and not in reverse).
        """
        expected_suite_names = self.instance.testsuite.ztest_suite_names
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

        # Tool options
        self.valgrind = False
        self.lsan = False
        self.asan = False
        self.ubsan = False
        self.coverage = False
        self.seed = None

    def try_kill_process_by_pid(self):
        if self.pid_fn:
            pid = int(open(self.pid_fn).read())
            os.unlink(self.pid_fn)
            self.pid_fn = None  # clear so we don't try to kill the binary twice
            try:
                os.kill(pid, signal.SIGTERM)
            except ProcessLookupError:
                pass

    def _output_reader(self, proc):
        self.line = proc.stdout.readline()

    def _output_handler(self, proc, harness):
        if harness.is_pytest:
            harness.handle(None)
            return

        log_out_fp = open(self.log, "wt")
        timeout_extended = False
        timeout_time = time.time() + self.timeout
        while True:
            this_timeout = timeout_time - time.time()
            if this_timeout < 0:
                break
            reader_t = threading.Thread(target=self._output_reader, args=(proc,), daemon=True)
            reader_t.start()
            reader_t.join(this_timeout)
            if not reader_t.is_alive():
                line = self.line
                logger.debug("OUTPUT: {0}".format(line.decode('utf-8').rstrip()))
                log_out_fp.write(line.decode('utf-8'))
                log_out_fp.flush()
                harness.handle(line.decode('utf-8').rstrip())
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

        log_out_fp.close()

    def handle(self):

        harness_name = self.instance.testsuite.harness.capitalize()
        harness_import = HarnessImporter(harness_name)
        harness = harness_import.instance
        harness.configure(self.instance)

        if self.call_make_run:
            command = [self.generator_cmd, "run"]
        elif self.call_west_flash:
            command = ["west", "flash", "--skip-rebuild", "-d", self.build_dir]
        else:
            command = [self.binary]

        run_valgrind = False
        if self.valgrind:
            command = ["valgrind", "--error-exitcode=2",
                       "--leak-check=full",
                       "--suppressions=" + ZEPHYR_BASE + "/scripts/valgrind.supp",
                       "--log-file=" + self.build_dir + "/valgrind.log",
                       "--track-origins=yes",
                       ] + command
            run_valgrind = True

        # Only valid for native_posix
        if self.seed is not None:
            command = command + ["--seed="+str(self.seed)]

        logger.debug("Spawning process: " +
                     " ".join(shlex.quote(word) for word in command) + os.linesep +
                     "in directory: " + self.build_dir)

        start_time = time.time()

        env = os.environ.copy()
        if self.asan:
            env["ASAN_OPTIONS"] = "log_path=stdout:" + \
                                  env.get("ASAN_OPTIONS", "")
            if not self.lsan:
                env["ASAN_OPTIONS"] += "detect_leaks=0"

        if self.ubsan:
            env["UBSAN_OPTIONS"] = "log_path=stdout:halt_on_error=1:" + \
                                  env.get("UBSAN_OPTIONS", "")

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

        if self.coverage:
            subprocess.call(["GCOV_PREFIX=" + self.build_dir,
                             "gcov", self.sourcedir, "-b", "-s", self.build_dir], shell=True)

        # FIXME: This is needed when killing the simulator, the console is
        # garbled and needs to be reset. Did not find a better way to do that.
        if sys.stdout.isatty():
            subprocess.call(["stty", "sane"])

        if harness.is_pytest:
            harness.pytest_run(self.log)

        self.instance.execution_time = handler_time
        if not self.terminated and self.returncode != 0:
            self.instance.status = "failed"
            if run_valgrind and self.returncode == 2:
                self.instance.reason = "Valgrind error"
            else:
                # When a process is killed, the default handler returns 128 + SIGTERM
                # so in that case the return code itself is not meaningful
                self.instance.reason = "Failed"
        elif harness.state:
            self.instance.status = harness.state
            if harness.state == "failed":
                self.instance.reason = "Failed"
        else:
            self.instance.status = "failed"
            self.instance.reason = "Timeout"
            self.instance.add_missing_case_status("blocked", "Timeout")

        self._final_handle_actions(harness, handler_time)


class DeviceHandler(Handler):

    def __init__(self, instance, type_str):
        """Constructor

        @param instance Test Instance
        """
        super().__init__(instance, type_str)

        self.testplan = None

    def monitor_serial(self, ser, halt_fileno, harness):
        if harness.is_pytest:
            harness.handle(None)
            return

        log_out_fp = open(self.log, "wt")

        ser_fileno = ser.fileno()
        readlist = [halt_fileno, ser_fileno]

        if self.coverage:
            # Set capture_coverage to True to indicate that right after
            # test results we should get coverage data, otherwise we exit
            # from the test.
            harness.capture_coverage = True

        ser.flush()

        while ser.isOpen():
            readable, _, _ = select.select(readlist, [], [], self.timeout)

            if halt_fileno in readable:
                logger.debug('halted')
                ser.close()
                break
            if ser_fileno not in readable:
                continue  # Timeout.

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

                log_out_fp.write(sl)
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
        for d in self.testplan.duts:
            if fixture and fixture not in d.fixtures:
                continue
            if d.platform != device or (d.serial is None and d.serial_pty is None):
                continue
            d.lock.acquire()
            avail = False
            if d.available:
                d.available = 0
                d.counter += 1
                avail = True
            d.lock.release()
            if avail:
                return d

        return None

    def make_device_available(self, serial):
        for d in self.testplan.duts:
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

    def handle(self):
        runner = None

        hardware = self.device_is_available(self.instance)
        while not hardware:
            logger.debug("Waiting for device {} to become available".format(self.instance.platform.name))
            time.sleep(1)
            hardware = self.device_is_available(self.instance)

        runner = hardware.runner or self.testplan.west_runner
        serial_pty = hardware.serial_pty

        ser_pty_process = None
        if serial_pty:
            master, slave = pty.openpty()
            try:
                ser_pty_process = subprocess.Popen(re.split(',| ', serial_pty), stdout=master, stdin=master, stderr=master)
            except subprocess.CalledProcessError as error:
                logger.error("Failed to run subprocess {}, error {}".format(serial_pty, error.output))
                return

            serial_device = os.ttyname(slave)
        else:
            serial_device = hardware.serial

        logger.debug(f"Using serial device {serial_device} @ {hardware.baud} baud")

        if (self.testplan.west_flash is not None) or runner:
            command = ["west", "flash", "--skip-rebuild", "-d", self.build_dir]
            command_extra_args = []

            # There are three ways this option is used.
            # 1) bare: --west-flash
            #    This results in options.west_flash == []
            # 2) with a value: --west-flash="--board-id=42"
            #    This results in options.west_flash == "--board-id=42"
            # 3) Multiple values: --west-flash="--board-id=42,--erase"
            #    This results in options.west_flash == "--board-id=42 --erase"
            if self.testplan.west_flash and self.testplan.west_flash != []:
                command_extra_args.extend(self.testplan.west_flash.split(','))

            if runner:
                command.append("--runner")
                command.append(runner)

                board_id = hardware.probe_id or hardware.id
                product = hardware.product
                if board_id is not None:
                    if runner == "pyocd":
                        command_extra_args.append("--board-id")
                        command_extra_args.append(board_id)
                    elif runner == "nrfjprog":
                        command_extra_args.append("--dev-id")
                        command_extra_args.append(board_id)
                    elif runner == "openocd" and product == "STM32 STLink":
                        command_extra_args.append("--cmd-pre-init")
                        command_extra_args.append("hla_serial %s" % (board_id))
                    elif runner == "openocd" and product == "STLINK-V3":
                        command_extra_args.append("--cmd-pre-init")
                        command_extra_args.append("hla_serial %s" % (board_id))
                    elif runner == "openocd" and product == "EDBG CMSIS-DAP":
                        command_extra_args.append("--cmd-pre-init")
                        command_extra_args.append("cmsis_dap_serial %s" % (board_id))
                    elif runner == "jlink":
                        command.append("--tool-opt=-SelectEmuBySN  %s" % (board_id))
                    elif runner == "stm32cubeprogrammer":
                        command.append("--tool-opt=sn=%s" % (board_id))
                    elif runner == "intel_adsp":
                        command.append("--pty")

                    # Receive parameters from an runner_params field
                    # of the specified hardware map file.
                    for d in self.testplan.duts:
                        if (d.platform == self.instance.platform.name) and d.runner_params:
                            for param in d.runner_params:
                                command.append(param)

            if command_extra_args != []:
                command.append('--')
                command.extend(command_extra_args)
        else:
            command = [self.generator_cmd, "-C", self.build_dir, "flash"]

        pre_script = hardware.pre_script
        post_flash_script = hardware.post_flash_script
        post_script = hardware.post_script

        if pre_script:
            self.run_custom_script(pre_script, 30)

        try:
            ser = serial.Serial(
                serial_device,
                baudrate=hardware.baud,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS,
                timeout=self.timeout
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
            return

        ser.flush()

        # turns out the ser.flush() is not enough to clear serial leftover from last case
        # explicitly readline() can do it reliably
        old_timeout = ser.timeout
        ser.timeout = 1 # wait for 1s if no serial output
        leftover_lines = ser.readlines(1000) # or read 1000 lines at most
        for line in leftover_lines:
            logger.debug(f"leftover log of previous test: {line}")
        ser.timeout = old_timeout

        harness_name = self.instance.testsuite.harness.capitalize()
        harness_import = HarnessImporter(harness_name)
        harness = harness_import.instance
        harness.configure(self.instance)
        read_pipe, write_pipe = os.pipe()
        start_time = time.time()

        t = threading.Thread(target=self.monitor_serial, daemon=True,
                             args=(ser, read_pipe, harness))
        t.start()

        d_log = "{}/device.log".format(self.instance.build_dir)
        logger.debug('Flash command: %s', command)
        try:
            stdout = stderr = None
            with subprocess.Popen(command, stderr=subprocess.PIPE, stdout=subprocess.PIPE) as proc:
                try:
                    (stdout, stderr) = proc.communicate(timeout=60)
                    # ignore unencodable unicode chars
                    logger.debug(stdout.decode(errors = "ignore"))

                    if proc.returncode != 0:
                        self.instance.status = "error"
                        self.instance.reason = "Device issue (Flash error?)"
                        with open(d_log, "w") as dlog_fp:
                            dlog_fp.write(stderr.decode())
                        os.write(write_pipe, b'x')  # halt the thread
                except subprocess.TimeoutExpired:
                    logger.warning("Flash operation timed out.")
                    proc.kill()
                    (stdout, stderr) = proc.communicate()
                    self.instance.status = "error"
                    self.instance.reason = "Device issue (Timeout)"

            with open(d_log, "w") as dlog_fp:
                dlog_fp.write(stderr.decode())

        except subprocess.CalledProcessError:
            os.write(write_pipe, b'x')  # halt the thread

        if post_flash_script:
            self.run_custom_script(post_flash_script, 30)

        t.join(self.timeout)
        if t.is_alive():
            logger.debug("Timed out while monitoring serial output on {}".format(self.instance.platform.name))

        if ser.isOpen():
            ser.close()

        if serial_pty:
            ser_pty_process.terminate()
            outs, errs = ser_pty_process.communicate()
            logger.debug("Process {} terminated outs: {} errs {}".format(serial_pty, outs, errs))

        os.close(write_pipe)
        os.close(read_pipe)

        handler_time = time.time() - start_time

        if harness.is_pytest:
            harness.pytest_run(self.log)

        self.instance.execution_time = handler_time
        if harness.state:
            self.instance.status = harness.state
            if harness.state == "failed":
                self.instance.reason = "Failed"
        else:
            self.instance.status = "error"
            self.instance.reason = "No Console Output(Timeout)"

        if self.instance.status == "error":
            self.instance.add_missing_case_status("blocked", self.instance.reason)

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

        if "ignore_qemu_crash" in instance.testsuite.tags:
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
    def _thread(handler, timeout, outdir, logfile, fifo_fn, pid_fn, results, harness,
                ignore_unexpected_eof=False):
        fifo_in = fifo_fn + ".in"
        fifo_out = fifo_fn + ".out"

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
                        #there's possibility we polled nothing because
                        #of not enough CPU time scheduled by host for
                        #QEMU process during p.poll(this_timeout)
                        cpu_time = QEMUHandler._get_cpu_time(pid)
                        if cpu_time < timeout and not out_state:
                            timeout_time = time.time() + (timeout - cpu_time)
                            continue
                except ProcessLookupError:
                    out_state = "failed"
                    break

                if not out_state:
                    out_state = "timeout"
                break

            if pid == 0 and os.path.exists(pid_fn):
                pid = int(open(pid_fn).read())

            if harness.is_pytest:
                harness.handle(None)
                out_state = harness.state
                break

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
            line = line.strip()
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

        if harness.is_pytest:
            harness.pytest_run(logfile)
            out_state = harness.state

        handler_time = time.time() - start_time
        logger.debug(f"QEMU ({pid}) complete ({out_state}) after {handler_time} seconds")

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

        log_out_fp.close()
        out_fp.close()
        in_fp.close()
        if pid:
            try:
                if pid:
                    os.kill(pid, signal.SIGTERM)
            except ProcessLookupError:
                # Oh well, as long as it's dead! User probably sent Ctrl-C
                pass

        os.unlink(fifo_in)
        os.unlink(fifo_out)

    def handle(self):
        self.results = {}
        self.run = True

        # We pass this to QEMU which looks for fifos with .in and .out
        # suffixes.

        self.fifo_fn = os.path.join(self.instance.build_dir, "qemu-fifo")
        self.pid_fn = os.path.join(self.instance.build_dir, "qemu.pid")

        if os.path.exists(self.pid_fn):
            os.unlink(self.pid_fn)

        self.log_fn = self.log

        harness_import = HarnessImporter(self.instance.testsuite.harness.capitalize())
        harness = harness_import.instance
        harness.configure(self.instance)

        self.thread = threading.Thread(name=self.name, target=QEMUHandler._thread,
                                       args=(self, self.timeout, self.build_dir,
                                             self.log_fn, self.fifo_fn,
                                             self.pid_fn, self.results, harness,
                                             self.ignore_unexpected_eof))

        self.thread.daemon = True
        logger.debug("Spawning QEMUHandler Thread for %s" % self.name)
        self.thread.start()
        if sys.stdout.isatty():
            subprocess.call(["stty", "sane"])

        logger.debug("Running %s (%s)" % (self.name, self.type_str))
        command = [self.generator_cmd]
        command += ["-C", self.build_dir, "run"]

        is_timeout = False
        qemu_pid = None

        with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=self.build_dir) as proc:
            logger.debug("Spawning QEMUHandler Thread for %s" % self.name)

            try:
                proc.wait(self.timeout)
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
            # error messages.
            self.thread.join(0)
            if self.thread.is_alive():
                logger.debug("Timed out while monitoring QEMU output")

            if os.path.exists(self.pid_fn):
                qemu_pid = int(open(self.pid_fn).read())
                os.unlink(self.pid_fn)

        logger.debug(f"return code from QEMU ({qemu_pid}): {self.returncode}")

        if (self.returncode != 0 and not self.ignore_qemu_crash) or not harness.state:
            self.instance.status = "failed"
            if is_timeout:
                self.instance.reason = "Timeout"
            else:
                self.instance.reason = "Exited with {}".format(self.returncode)
            self.instance.add_missing_case_status("blocked")

        self._final_handle_actions(harness, 0)

    def get_fifo(self):
        return self.fifo_fn


class SizeCalculator:
    alloc_sections = [
        "bss",
        "noinit",
        "app_bss",
        "app_noinit",
        "ccm_bss",
        "ccm_noinit"
    ]

    rw_sections = [
        "datas",
        "initlevel",
        "exceptions",
        "initshell",
        "_static_thread_data_area",
        "k_timer_area",
        "k_mem_slab_area",
        "k_mem_pool_area",
        "sw_isr_table",
        "k_sem_area",
        "k_mutex_area",
        "app_shmem_regions",
        "_k_fifo_area",
        "_k_lifo_area",
        "k_stack_area",
        "k_msgq_area",
        "k_mbox_area",
        "k_pipe_area",
        "net_if_area",
        "net_if_dev_area",
        "net_l2_area",
        "net_l2_data",
        "k_queue_area",
        "_net_buf_pool_area",
        "app_datas",
        "kobject_data",
        "mmu_tables",
        "app_pad",
        "priv_stacks",
        "ccm_data",
        "usb_descriptor",
        "usb_data", "usb_bos_desc",
        "uart_mux",
        'log_backends_sections',
        'log_dynamic_sections',
        'log_const_sections',
        "app_smem",
        'shell_root_cmds_sections',
        'log_const_sections',
        "font_entry_sections",
        "priv_stacks_noinit",
        "_GCOV_BSS_SECTION_NAME",
        "gcov",
        "nocache",
        "devices",
        "k_heap_area",
    ]

    # These get copied into RAM only on non-XIP
    ro_sections = [
        "rom_start",
        "text",
        "ctors",
        "init_array",
        "reset",
        "z_object_assignment_area",
        "rodata",
        "net_l2",
        "vector",
        "sw_isr_table",
        "settings_handler_static_area",
        "bt_l2cap_fixed_chan_area",
        "bt_l2cap_br_fixed_chan_area",
        "bt_gatt_service_static_area",
        "vectors",
        "net_socket_register_area",
        "net_ppp_proto",
        "shell_area",
        "tracing_backend_area",
        "ppp_protocol_handler_area",
    ]

    def __init__(self, filename, extra_sections):
        """Constructor

        @param filename Path to the output binary
            The <filename> is parsed by objdump to determine section sizes
        """
        # Make sure this is an ELF binary
        with open(filename, "rb") as f:
            magic = f.read(4)

        try:
            if magic != b'\x7fELF':
                raise TwisterRuntimeError("%s is not an ELF binary" % filename)
        except Exception as e:
            print(str(e))
            sys.exit(2)

        # Search for CONFIG_XIP in the ELF's list of symbols using NM and AWK.
        # GREP can not be used as it returns an error if the symbol is not
        # found.
        is_xip_command = "nm " + filename + \
                         " | awk '/CONFIG_XIP/ { print $3 }'"
        is_xip_output = subprocess.check_output(
            is_xip_command, shell=True, stderr=subprocess.STDOUT).decode(
            "utf-8").strip()
        try:
            if is_xip_output.endswith("no symbols"):
                raise TwisterRuntimeError("%s has no symbol information" % filename)
        except Exception as e:
            print(str(e))
            sys.exit(2)

        self.is_xip = (len(is_xip_output) != 0)

        self.filename = filename
        self.sections = []
        self.rom_size = 0
        self.ram_size = 0
        self.extra_sections = extra_sections

        self._calculate_sizes()

    def get_ram_size(self):
        """Get the amount of RAM the application will use up on the device

        @return amount of RAM, in bytes
        """
        return self.ram_size

    def get_rom_size(self):
        """Get the size of the data that this application uses on device's flash

        @return amount of ROM, in bytes
        """
        return self.rom_size

    def unrecognized_sections(self):
        """Get a list of sections inside the binary that weren't recognized

        @return list of unrecognized section names
        """
        slist = []
        for v in self.sections:
            if not v["recognized"]:
                slist.append(v["name"])
        return slist

    def _calculate_sizes(self):
        """ Calculate RAM and ROM usage by section """
        objdump_command = "objdump -h " + self.filename
        objdump_output = subprocess.check_output(
            objdump_command, shell=True).decode("utf-8").splitlines()

        for line in objdump_output:
            words = line.split()

            if not words:  # Skip lines that are too short
                continue

            index = words[0]
            if not index[0].isdigit():  # Skip lines that do not start
                continue  # with a digit

            name = words[1]  # Skip lines with section names
            if name[0] == '.':  # starting with '.'
                continue

            # TODO this doesn't actually reflect the size in flash or RAM as
            # it doesn't include linker-imposed padding between sections.
            # It is close though.
            size = int(words[2], 16)
            if size == 0:
                continue

            load_addr = int(words[4], 16)
            virt_addr = int(words[3], 16)

            # Add section to memory use totals (for both non-XIP and XIP scenarios)
            # Unrecognized section names are not included in the calculations.
            recognized = True
            if name in SizeCalculator.alloc_sections:
                self.ram_size += size
                stype = "alloc"
            elif name in SizeCalculator.rw_sections:
                self.ram_size += size
                self.rom_size += size
                stype = "rw"
            elif name in SizeCalculator.ro_sections:
                self.rom_size += size
                if not self.is_xip:
                    self.ram_size += size
                stype = "ro"
            else:
                stype = "unknown"
                if name not in self.extra_sections:
                    recognized = False

            self.sections.append({"name": name, "load_addr": load_addr,
                                  "size": size, "virt_addr": virt_addr,
                                  "type": stype, "recognized": recognized})



class TwisterConfigParser:
    """Class to read test case files with semantic checking
    """

    def __init__(self, filename, schema):
        """Instantiate a new TwisterConfigParser object

        @param filename Source .yaml file to read
        """
        self.data = {}
        self.schema = schema
        self.filename = filename
        self.tests = {}
        self.common = {}

    def load(self):
        self.data = scl.yaml_load_verify(self.filename, self.schema)

        if 'tests' in self.data:
            self.tests = self.data['tests']
        if 'common' in self.data:
            self.common = self.data['common']

    def _cast_value(self, value, typestr):
        if isinstance(value, str):
            v = value.strip()
        if typestr == "str":
            return v

        elif typestr == "float":
            return float(value)

        elif typestr == "int":
            return int(value)

        elif typestr == "bool":
            return value

        elif typestr.startswith("list") and isinstance(value, list):
            return value
        elif typestr.startswith("list") and isinstance(value, str):
            vs = v.split()
            if len(typestr) > 4 and typestr[4] == ":":
                return [self._cast_value(vsi, typestr[5:]) for vsi in vs]
            else:
                return vs

        elif typestr.startswith("set"):
            vs = v.split()
            if len(typestr) > 3 and typestr[3] == ":":
                return {self._cast_value(vsi, typestr[4:]) for vsi in vs}
            else:
                return set(vs)

        elif typestr.startswith("map"):
            return value
        else:
            raise ConfigurationError(
                self.filename, "unknown type '%s'" % value)

    def get_test(self, name, valid_keys):
        """Get a dictionary representing the keys/values within a test

        @param name The test in the .yaml file to retrieve data from
        @param valid_keys A dictionary representing the intended semantics
            for this test. Each key in this dictionary is a key that could
            be specified, if a key is given in the .yaml file which isn't in
            here, it will generate an error. Each value in this dictionary
            is another dictionary containing metadata:

                "default" - Default value if not given
                "type" - Data type to convert the text value to. Simple types
                    supported are "str", "float", "int", "bool" which will get
                    converted to respective Python data types. "set" and "list"
                    may also be specified which will split the value by
                    whitespace (but keep the elements as strings). finally,
                    "list:<type>" and "set:<type>" may be given which will
                    perform a type conversion after splitting the value up.
                "required" - If true, raise an error if not defined. If false
                    and "default" isn't specified, a type conversion will be
                    done on an empty string
        @return A dictionary containing the test key-value pairs with
            type conversion and default values filled in per valid_keys
        """

        d = {}
        for k, v in self.common.items():
            d[k] = v

        for k, v in self.tests[name].items():
            if k in d:
                if isinstance(d[k], str):
                    # By default, we just concatenate string values of keys
                    # which appear both in "common" and per-test sections,
                    # but some keys are handled in adhoc way based on their
                    # semantics.
                    if k == "filter":
                        d[k] = "(%s) and (%s)" % (d[k], v)
                    else:
                        d[k] += " " + v
            else:
                d[k] = v

        for k, kinfo in valid_keys.items():
            if k not in d:
                if "required" in kinfo:
                    required = kinfo["required"]
                else:
                    required = False

                if required:
                    raise ConfigurationError(
                        self.filename,
                        "missing required value for '%s' in test '%s'" %
                        (k, name))
                else:
                    if "default" in kinfo:
                        default = kinfo["default"]
                    else:
                        default = self._cast_value("", kinfo["type"])
                    d[k] = default
            else:
                try:
                    d[k] = self._cast_value(d[k], kinfo["type"])
                except ValueError:
                    raise ConfigurationError(
                        self.filename, "bad %s value '%s' for key '%s' in name '%s'" %
                                       (kinfo["type"], d[k], k, name))

        return d


class Platform:
    """Class representing metadata for a particular platform

    Maps directly to BOARD when building"""

    platform_schema = scl.yaml_load(os.path.join(ZEPHYR_BASE,
                                                 "scripts", "schemas", "twister", "platform-schema.yaml"))

    def __init__(self):
        """Constructor.

        """

        self.name = ""
        self.twister = True
        # if no RAM size is specified by the board, take a default of 128K
        self.ram = 128

        self.timeout_multiplier = 1.0
        self.ignore_tags = []
        self.only_tags = []
        self.default = False
        # if no flash size is specified by the board, take a default of 512K
        self.flash = 512
        self.supported = set()

        self.arch = ""
        self.type = "na"
        self.simulation = "na"
        self.supported_toolchains = []
        self.env = []
        self.env_satisfied = True
        self.filter_data = dict()

    def load(self, platform_file):
        scp = TwisterConfigParser(platform_file, self.platform_schema)
        scp.load()
        data = scp.data

        self.name = data['identifier']
        self.twister = data.get("twister", True)
        # if no RAM size is specified by the board, take a default of 128K
        self.ram = data.get("ram", 128)
        testing = data.get("testing", {})
        self.timeout_multiplier = testing.get("timeout_multiplier", 1.0)
        self.ignore_tags = testing.get("ignore_tags", [])
        self.only_tags = testing.get("only_tags", [])
        self.default = testing.get("default", False)
        # if no flash size is specified by the board, take a default of 512K
        self.flash = data.get("flash", 512)
        self.supported = set()
        for supp_feature in data.get("supported", []):
            for item in supp_feature.split(":"):
                self.supported.add(item)

        self.arch = data['arch']
        self.type = data.get('type', "na")
        self.simulation = data.get('simulation', "na")
        self.supported_toolchains = data.get("toolchain", [])
        self.env = data.get("env", [])
        self.env_satisfied = True
        for env in self.env:
            if not os.environ.get(env, None):
                self.env_satisfied = False

    def __repr__(self):
        return "<%s on %s>" % (self.name, self.arch)


class DisablePyTestCollectionMixin(object):
    __test__ = False


class ScanPathResult:
    """Result of the TestSuite.scan_path function call.

    Attributes:
        matches                          A list of test cases
        warnings                         A string containing one or more
                                         warnings to display
        has_registered_test_suites       Whether or not the path contained any
                                         calls to the ztest_register_test_suite
                                         macro.
        has_run_registered_test_suites   Whether or not the path contained at
                                         least one call to
                                         ztest_run_registered_test_suites.
        has_test_main                    Whether or not the path contains a
                                         definition of test_main(void)
        ztest_suite_names                Names of found ztest suites
    """
    def __init__(self,
                 matches: List[str] = None,
                 warnings: str = None,
                 has_registered_test_suites: bool = False,
                 has_run_registered_test_suites: bool = False,
                 has_test_main: bool = False,
                 ztest_suite_names: List[str] = []):
        self.matches = matches
        self.warnings = warnings
        self.has_registered_test_suites = has_registered_test_suites
        self.has_run_registered_test_suites = has_run_registered_test_suites
        self.has_test_main = has_test_main
        self.ztest_suite_names = ztest_suite_names

    def __eq__(self, other):
        if not isinstance(other, ScanPathResult):
            return False
        return (sorted(self.matches) == sorted(other.matches) and
                self.warnings == other.warnings and
                (self.has_registered_test_suites ==
                 other.has_registered_test_suites) and
                (self.has_run_registered_test_suites ==
                 other.has_run_registered_test_suites) and
                self.has_test_main == other.has_test_main and
                (sorted(self.ztest_suite_names) ==
                 sorted(other.ztest_suite_names)))

class TestCase(DisablePyTestCollectionMixin):

    def __init__(self, name=None, testsuite=None):
        self.duration = 0
        self.name = name
        self.status = None
        self.reason = None
        self.testsuite = testsuite
        self.output = ""
        self.freeform = False

    def __lt__(self, other):
        return self.name < other.name

    def __repr__(self):
        return "<TestCase %s with %s>" % (self.name, self.status)

    def __str__(self):
        return self.name

class TestSuite(DisablePyTestCollectionMixin):
    """Class representing a test application
    """

    def __init__(self, testsuite_root, workdir, name):
        """TestSuite constructor.

        This gets called by TestPlan as it finds and reads test yaml files.
        Multiple TestSuite instances may be generated from a single testcase.yaml,
        each one corresponds to an entry within that file.

        We need to have a unique name for every single test case. Since
        a testcase.yaml can define multiple tests, the canonical name for
        the test case is <workdir>/<name>.

        @param testsuite_root os.path.abspath() of one of the --testcase-root
        @param workdir Sub-directory of testsuite_root where the
            .yaml test configuration file was found
        @param name Name of this test case, corresponding to the entry name
            in the test case configuration file. For many test cases that just
            define one test, can be anything and is usually "test". This is
            really only used to distinguish between different cases when
            the testcase.yaml defines multiple tests
        """


        self.source_dir = ""
        self.yamlfile = ""
        self.testcases = []
        self.name = self.get_unique(testsuite_root, workdir, name)
        self.id = name

        self.type = None
        self.tags = set()
        self.extra_args = None
        self.extra_configs = None
        self.arch_allow = None
        self.arch_exclude = None
        self.skip = False
        self.platform_exclude = None
        self.platform_allow = None
        self.platform_type = []
        self.toolchain_exclude = None
        self.toolchain_allow = None
        self.ts_filter = None
        self.timeout = 60
        self.harness = ""
        self.harness_config = {}
        self.build_only = True
        self.build_on_all = False
        self.slow = False
        self.min_ram = -1
        self.depends_on = None
        self.min_flash = -1
        self.extra_sections = None
        self.integration_platforms = []
        self.ztest_suite_names = []


    def add_testcase(self, name, freeform=False):
        tc = TestCase(name=name, testsuite=self)
        tc.freeform = freeform
        self.testcases.append(tc)

    @staticmethod
    def get_unique(testsuite_root, workdir, name):

        canonical_testsuite_root = os.path.realpath(testsuite_root)
        if Path(canonical_zephyr_base) in Path(canonical_testsuite_root).parents:
            # This is in ZEPHYR_BASE, so include path in name for uniqueness
            # FIXME: We should not depend on path of test for unique names.
            relative_tc_root = os.path.relpath(canonical_testsuite_root,
                                               start=canonical_zephyr_base)
        else:
            relative_tc_root = ""

        # workdir can be "."
        unique = os.path.normpath(os.path.join(relative_tc_root, workdir, name))
        check = name.split(".")
        if len(check) < 2:
            raise TwisterException(f"""bad test name '{name}' in {testsuite_root}/{workdir}. \
Tests should reference the category and subsystem with a dot as a separator.
                    """
                    )
        return unique

    def scan_file(self, inf_name):
        regular_suite_regex = re.compile(
            # do not match until end-of-line, otherwise we won't allow
            # stc_regex below to catch the ones that are declared in the same
            # line--as we only search starting the end of this match
            br"^\s*ztest_test_suite\(\s*(?P<suite_name>[a-zA-Z0-9_]+)\s*,",
            re.MULTILINE)
        registered_suite_regex = re.compile(
            br"^\s*ztest_register_test_suite"
            br"\(\s*(?P<suite_name>[a-zA-Z0-9_]+)\s*,",
            re.MULTILINE)
        new_suite_regex = re.compile(
            br"^\s*ZTEST_SUITE\(\s*(?P<suite_name>[a-zA-Z0-9_]+)\s*,",
            re.MULTILINE)
        # Checks if the file contains a definition of "void test_main(void)"
        # Since ztest provides a plain test_main implementation it is OK to:
        # 1. register test suites and not call the run function iff the test
        #    doesn't have a custom test_main.
        # 2. register test suites and a custom test_main definition iff the test
        #    also calls ztest_run_registered_test_suites.
        test_main_regex = re.compile(
            br"^\s*void\s+test_main\(void\)",
            re.MULTILINE)
        registered_suite_run_regex = re.compile(
            br"^\s*ztest_run_registered_test_suites\("
            br"(\*+|&)?(?P<state_identifier>[a-zA-Z0-9_]+)\)",
            re.MULTILINE)

        warnings = None
        has_registered_test_suites = False
        has_run_registered_test_suites = False
        has_test_main = False

        with open(inf_name) as inf:
            if os.name == 'nt':
                mmap_args = {'fileno': inf.fileno(), 'length': 0, 'access': mmap.ACCESS_READ}
            else:
                mmap_args = {'fileno': inf.fileno(), 'length': 0, 'flags': mmap.MAP_PRIVATE, 'prot': mmap.PROT_READ,
                             'offset': 0}

            with contextlib.closing(mmap.mmap(**mmap_args)) as main_c:
                regular_suite_regex_matches = \
                    [m for m in regular_suite_regex.finditer(main_c)]
                registered_suite_regex_matches = \
                    [m for m in registered_suite_regex.finditer(main_c)]
                new_suite_regex_matches = \
                    [m for m in new_suite_regex.finditer(main_c)]

                if registered_suite_regex_matches:
                    has_registered_test_suites = True
                if registered_suite_run_regex.search(main_c):
                    has_run_registered_test_suites = True
                if test_main_regex.search(main_c):
                    has_test_main = True

                if regular_suite_regex_matches:
                    ztest_suite_names = \
                        self._extract_ztest_suite_names(regular_suite_regex_matches)
                    testcase_names, warnings = \
                        self._find_regular_ztest_testcases(main_c, regular_suite_regex_matches, has_registered_test_suites)
                elif registered_suite_regex_matches:
                    ztest_suite_names = \
                        self._extract_ztest_suite_names(registered_suite_regex_matches)
                    testcase_names, warnings = \
                        self._find_regular_ztest_testcases(main_c, registered_suite_regex_matches, has_registered_test_suites)
                elif new_suite_regex_matches:
                    ztest_suite_names = \
                        self._extract_ztest_suite_names(new_suite_regex_matches)
                    testcase_names, warnings = \
                        self._find_new_ztest_testcases(main_c)
                else:
                    # can't find ztest_test_suite, maybe a client, because
                    # it includes ztest.h
                    ztest_suite_names = []
                    testcase_names, warnings = None, None

                return ScanPathResult(
                    matches=testcase_names,
                    warnings=warnings,
                    has_registered_test_suites=has_registered_test_suites,
                    has_run_registered_test_suites=has_run_registered_test_suites,
                    has_test_main=has_test_main,
                    ztest_suite_names=ztest_suite_names)

    @staticmethod
    def _extract_ztest_suite_names(suite_regex_matches):
        ztest_suite_names = \
            [m.group("suite_name") for m in suite_regex_matches]
        ztest_suite_names = \
            [name.decode("UTF-8") for name in ztest_suite_names]
        return ztest_suite_names

    def _find_regular_ztest_testcases(self, search_area, suite_regex_matches, is_registered_test_suite):
        """
        Find regular ztest testcases like "ztest_unit_test" or similar. Return
        testcases' names and eventually found warnings.
        """
        testcase_regex = re.compile(
            br"""^\s*  # empty space at the beginning is ok
            # catch the case where it is declared in the same sentence, e.g:
            #
            # ztest_test_suite(mutex_complex, ztest_user_unit_test(TESTNAME));
            # ztest_register_test_suite(n, p, ztest_user_unit_test(TESTNAME),
            (?:ztest_
              (?:test_suite\(|register_test_suite\([a-zA-Z0-9_]+\s*,\s*)
              [a-zA-Z0-9_]+\s*,\s*
            )?
            # Catch ztest[_user]_unit_test-[_setup_teardown](TESTNAME)
            ztest_(?:1cpu_)?(?:user_)?unit_test(?:_setup_teardown)?
            # Consume the argument that becomes the extra testcase
            \(\s*(?P<testcase_name>[a-zA-Z0-9_]+)
            # _setup_teardown() variant has two extra arguments that we ignore
            (?:\s*,\s*[a-zA-Z0-9_]+\s*,\s*[a-zA-Z0-9_]+)?
            \s*\)""",
            # We don't check how it finishes; we don't care
            re.MULTILINE | re.VERBOSE)
        achtung_regex = re.compile(
            br"(#ifdef|#endif)",
            re.MULTILINE)

        search_start, search_end = \
            self._get_search_area_boundary(search_area, suite_regex_matches, is_registered_test_suite)
        limited_search_area = search_area[search_start:search_end]
        testcase_names, warnings = \
            self._find_ztest_testcases(limited_search_area, testcase_regex)

        achtung_matches = re.findall(achtung_regex, limited_search_area)
        if achtung_matches and warnings is None:
            achtung = ", ".join(sorted({match.decode() for match in achtung_matches},reverse = True))
            warnings = f"found invalid {achtung} in ztest_test_suite()"

        return testcase_names, warnings

    @staticmethod
    def _get_search_area_boundary(search_area, suite_regex_matches, is_registered_test_suite):
        """
        Get search area boundary based on "ztest_test_suite(...)",
        "ztest_register_test_suite(...)" or "ztest_run_test_suite(...)"
        functions occurrence.
        """
        suite_run_regex = re.compile(
            br"^\s*ztest_run_test_suite\((?P<suite_name>[a-zA-Z0-9_]+)\)",
            re.MULTILINE)

        search_start = suite_regex_matches[0].end()

        suite_run_match = suite_run_regex.search(search_area)
        if suite_run_match:
            search_end = suite_run_match.start()
        elif not suite_run_match and not is_registered_test_suite:
            raise ValueError("can't find ztest_run_test_suite")
        else:
            search_end = re.compile(br"\);", re.MULTILINE) \
                .search(search_area, search_start) \
                .end()

        return search_start, search_end

    def _find_new_ztest_testcases(self, search_area):
        """
        Find regular ztest testcases like "ZTEST" or "ZTEST_F". Return
        testcases' names and eventually found warnings.
        """
        testcase_regex = re.compile(
            br"^\s*(?:ZTEST|ZTEST_F)\(\s*(?P<suite_name>[a-zA-Z0-9_]+)\s*,"
            br"\s*(?P<testcase_name>[a-zA-Z0-9_]+)\s*",
            re.MULTILINE)

        return self._find_ztest_testcases(search_area, testcase_regex)

    @staticmethod
    def _find_ztest_testcases(search_area, testcase_regex):
        """
        Parse search area and try to find testcases defined in testcase_regex
        argument. Return testcase names and eventually found warnings.
        """
        testcase_regex_matches = \
            [m for m in testcase_regex.finditer(search_area)]
        testcase_names = \
            [m.group("testcase_name") for m in testcase_regex_matches]
        testcase_names = [name.decode("UTF-8") for name in testcase_names]
        warnings = None
        for testcase_name in testcase_names:
            if not testcase_name.startswith("test_"):
                warnings = "Found a test that does not start with test_"
        testcase_names = \
            [tc_name.replace("test_", "", 1) for tc_name in testcase_names]

        return testcase_names, warnings

    def scan_path(self, path):
        subcases = []
        has_registered_test_suites = False
        has_run_registered_test_suites = False
        has_test_main = False
        ztest_suite_names = []

        src_dir_path = self._find_src_dir_path(path)
        for filename in glob.glob(os.path.join(src_dir_path, "*.c*")):
            try:
                result: ScanPathResult = self.scan_file(filename)
                if result.warnings:
                    logger.error("%s: %s" % (filename, result.warnings))
                    raise TwisterRuntimeError(
                        "%s: %s" % (filename, result.warnings))
                if result.matches:
                    subcases += result.matches
                if result.has_registered_test_suites:
                    has_registered_test_suites = True
                if result.has_run_registered_test_suites:
                    has_run_registered_test_suites = True
                if result.has_test_main:
                    has_test_main = True
                if result.ztest_suite_names:
                    ztest_suite_names += result.ztest_suite_names

            except ValueError as e:
                logger.error("%s: can't find: %s" % (filename, e))

        for filename in glob.glob(os.path.join(path, "*.c")):
            try:
                result: ScanPathResult = self.scan_file(filename)
                if result.warnings:
                    logger.error("%s: %s" % (filename, result.warnings))
                if result.matches:
                    subcases += result.matches
                if result.ztest_suite_names:
                    ztest_suite_names += result.ztest_suite_names
            except ValueError as e:
                logger.error("%s: can't find: %s" % (filename, e))

        if (has_registered_test_suites and has_test_main and
                not has_run_registered_test_suites):
            warning = \
                "Found call to 'ztest_register_test_suite()' but no "\
                "call to 'ztest_run_registered_test_suites()'"
            logger.error(warning)
            raise TwisterRuntimeError(warning)

        return subcases, ztest_suite_names

    def parse_subcases(self, test_path):
        subcases, ztest_suite_names = self.scan_path(test_path)
        # if testcases are provided as part of the yaml, skip this step.
        if not self.testcases:
            # only add each testcase once
            for sub in set(subcases):
                name = "{}.{}".format(self.id, sub)
                self.add_testcase(name)

            if not subcases:
                self.add_testcase(self.id, freeform=True)

        self.ztest_suite_names = ztest_suite_names

    @staticmethod
    def _find_src_dir_path(test_dir_path):
        """
        Try to find src directory with test source code. Sometimes due to the
        optimization reasons it is placed in upper directory.
        """
        src_dir_name = "src"
        src_dir_path = os.path.join(test_dir_path, src_dir_name)
        if os.path.isdir(src_dir_path):
            return src_dir_path
        src_dir_path = os.path.join(test_dir_path, "..", src_dir_name)
        if os.path.isdir(src_dir_path):
            return src_dir_path
        return ""

    def __str__(self):
        return self.name


class TestInstance(DisablePyTestCollectionMixin):
    """Class representing the execution of a particular TestSuite on a platform

    @param test The TestSuite object we want to build/execute
    @param platform Platform object that we want to build and run against
    @param base_outdir Base directory for all test results. The actual
        out directory used is <outdir>/<platform>/<test case name>
    """

    def __init__(self, testsuite, platform, outdir):

        self.testsuite = testsuite
        self.platform = platform

        self.status = None
        self.filters = []
        self.reason = "Unknown"
        self.filter_type = None
        self.metrics = dict()
        self.handler = None
        self.outdir = outdir
        self.execution_time = 0

        self.name = os.path.join(platform.name, testsuite.name)
        self.run_id = self._get_run_id()
        self.build_dir = os.path.join(outdir, platform.name, testsuite.name)

        self.run = False
        self.testcases = []
        self.init_cases()

    # Fix an issue with copying objects from testsuite, need better solution.
    def init_cases(self):
        for c in self.testsuite.testcases:
            self.add_testcase(c.name, freeform=c.freeform)

    def _get_run_id(self):
        """ generate run id from instance unique identifier and a random
        number"""

        hash_object = hashlib.md5(self.name.encode())
        random_str = f"{random.getrandbits(64)}".encode()
        hash_object.update(random_str)
        return hash_object.hexdigest()

    def add_filter(self, reason, filter_type):
        self.filters.append({'type': filter_type, 'reason': reason })
        self.status = "filtered"
        self.reason = reason
        self.filter_type = filter_type


    def add_missing_case_status(self, status, reason=None):
        for case in self.testcases:
            if not case.status:
                case.status = status
                if reason:
                    case.reason = reason
                else:
                    case.reason = self.reason

    def __getstate__(self):
        d = self.__dict__.copy()
        return d

    def __setstate__(self, d):
        self.__dict__.update(d)

    def __lt__(self, other):
        return self.name < other.name

    def set_case_status_by_name(self, name, status, reason=None):
        tc = self.get_case_or_create(name)
        tc.status = status
        if reason:
            tc.reason = reason
        return tc

    def add_testcase(self, name, freeform=False):
        tc = TestCase(name=name)
        tc.freeform = freeform
        self.testcases.append(tc)
        return tc

    def get_case_by_name(self, name):
        for c in self.testcases:
            if c.name == name:
                return c
        return None

    def get_case_or_create(self, name):
        for c in self.testcases:
            if c.name == name:
                return c

        logger.debug(f"Could not find a matching testcase for {name}")
        tc = TestCase(name=name)
        self.testcases.append(tc)
        return tc

    @staticmethod
    def testsuite_runnable(testsuite, fixtures):
        can_run = False
        # console harness allows us to run the test and capture data.
        if testsuite.harness in [ 'console', 'ztest', 'pytest']:
            can_run = True
            # if we have a fixture that is also being supplied on the
            # command-line, then we need to run the test, not just build it.
            fixture = testsuite.harness_config.get('fixture')
            if fixture:
                can_run = (fixture in fixtures)

        elif testsuite.harness:
            can_run = False
        else:
            can_run = True

        return can_run


    # Global testsuite parameters
    def check_runnable(self, enable_slow=False, filter='buildable', fixtures=[]):

        # right now we only support building on windows. running is still work
        # in progress.
        if os.name == 'nt':
            return False

        # we asked for build-only on the command line
        if self.testsuite.build_only:
            return False

        # Do not run slow tests:
        skip_slow = self.testsuite.slow and not enable_slow
        if skip_slow:
            return False

        target_ready = bool(self.testsuite.type == "unit" or \
                        self.platform.type == "native" or \
                        self.platform.simulation in ["mdb-nsim", "nsim", "renode", "qemu", "tsim", "armfvp", "xt-sim"] or \
                        filter == 'runnable')

        if self.platform.simulation == "nsim":
            if not find_executable("nsimdrv"):
                target_ready = False

        if self.platform.simulation == "mdb-nsim":
            if not find_executable("mdb"):
                target_ready = False

        if self.platform.simulation == "renode":
            if not find_executable("renode"):
                target_ready = False

        if self.platform.simulation == "tsim":
            if not find_executable("tsim-leon3"):
                target_ready = False

        testsuite_runnable = self.testsuite_runnable(self.testsuite, fixtures)

        return testsuite_runnable and target_ready

    def create_overlay(self, platform, enable_asan=False, enable_ubsan=False, enable_coverage=False, coverage_platform=[]):
        # Create this in a "twister/" subdirectory otherwise this
        # will pass this overlay to kconfig.py *twice* and kconfig.cmake
        # will silently give that second time precedence over any
        # --extra-args=CONFIG_*
        subdir = os.path.join(self.build_dir, "twister")

        content = ""

        if self.testsuite.extra_configs:
            content = "\n".join(self.testsuite.extra_configs)

        if enable_coverage:
            if platform.name in coverage_platform:
                content = content + "\nCONFIG_COVERAGE=y"
                content = content + "\nCONFIG_COVERAGE_DUMP=y"

        if enable_asan:
            if platform.type == "native":
                content = content + "\nCONFIG_ASAN=y"

        if enable_ubsan:
            if platform.type == "native":
                content = content + "\nCONFIG_UBSAN=y"

        if content:
            os.makedirs(subdir, exist_ok=True)
            file = os.path.join(subdir, "testsuite_extra.conf")
            with open(file, "w") as f:
                f.write(content)

        return content

    def calculate_sizes(self):
        """Get the RAM/ROM sizes of a test case.

        This can only be run after the instance has been executed by
        MakeGenerator, otherwise there won't be any binaries to measure.

        @return A SizeCalculator object
        """
        fns = glob.glob(os.path.join(self.build_dir, "zephyr", "*.elf"))
        fns.extend(glob.glob(os.path.join(self.build_dir, "zephyr", "*.exe")))
        fns = [x for x in fns if '_pre' not in x]
        if len(fns) != 1:
            raise BuildError("Missing/multiple output ELF binary")

        return SizeCalculator(fns[0], self.testsuite.extra_sections)

    def __repr__(self):
        return "<TestSuite %s on %s>" % (self.testsuite.name, self.platform.name)

class CMake():
    config_re = re.compile('(CONFIG_[A-Za-z0-9_]+)[=]\"?([^\"]*)\"?$')
    dt_re = re.compile('([A-Za-z0-9_]+)[=]\"?([^\"]*)\"?$')

    def __init__(self, testsuite, platform, source_dir, build_dir):

        self.cwd = None
        self.capture_output = True

        self.defconfig = {}
        self.cmake_cache = {}

        self.instance = None
        self.testsuite = testsuite
        self.platform = platform
        self.source_dir = source_dir
        self.build_dir = build_dir
        self.log = "build.log"
        self.generator = None
        self.generator_cmd = None

        self.default_encoding = sys.getdefaultencoding()

    def parse_generated(self):
        self.defconfig = {}
        return {}

    def run_build(self, args=[]):

        logger.debug("Building %s for %s" % (self.source_dir, self.platform.name))

        cmake_args = []
        cmake_args.extend(args)
        cmake = shutil.which('cmake')
        cmd = [cmake] + cmake_args
        kwargs = dict()

        if self.capture_output:
            kwargs['stdout'] = subprocess.PIPE
            # CMake sends the output of message() to stderr unless it's STATUS
            kwargs['stderr'] = subprocess.STDOUT

        if self.cwd:
            kwargs['cwd'] = self.cwd

        p = subprocess.Popen(cmd, **kwargs)
        out, _ = p.communicate()

        results = {}
        if p.returncode == 0:
            msg = "Finished building %s for %s" % (self.source_dir, self.platform.name)

            self.instance.status = "passed"
            if not self.instance.run:
                self.instance.add_missing_case_status("skipped", "Test was built only")
            results = {'msg': msg, "returncode": p.returncode, "instance": self.instance}

            if out:
                log_msg = out.decode(self.default_encoding)
                with open(os.path.join(self.build_dir, self.log), "a", encoding=self.default_encoding) as log:
                    log.write(log_msg)

            else:
                return None
        else:
            # A real error occurred, raise an exception
            log_msg = ""
            if out:
                log_msg = out.decode(self.default_encoding)
                with open(os.path.join(self.build_dir, self.log), "a", encoding=self.default_encoding) as log:
                    log.write(log_msg)

            if log_msg:
                overflow_found = re.findall("region `(FLASH|ROM|RAM|ICCM|DCCM|SRAM)' overflowed by", log_msg)
                if overflow_found and not self.overflow_as_errors:
                    logger.debug("Test skipped due to {} Overflow".format(overflow_found[0]))
                    self.instance.status = "skipped"
                    self.instance.reason = "{} overflow".format(overflow_found[0])
                else:
                    self.instance.status = "error"
                    self.instance.reason = "Build failure"

            results = {
                "returncode": p.returncode,
                "instance": self.instance,
            }

        return results

    def run_cmake(self, args=[]):

        if self.warnings_as_errors:
            ldflags = "-Wl,--fatal-warnings"
            cflags = "-Werror"
            aflags = "-Werror -Wa,--fatal-warnings"
            gen_defines_args = "--edtlib-Werror"
        else:
            ldflags = cflags = aflags = ""
            gen_defines_args = ""

        logger.debug("Running cmake on %s for %s" % (self.source_dir, self.platform.name))
        cmake_args = [
            f'-B{self.build_dir}',
            f'-S{self.source_dir}',
            f'-DTC_RUNID={self.instance.run_id}',
            f'-DEXTRA_CFLAGS={cflags}',
            f'-DEXTRA_AFLAGS={aflags}',
            f'-DEXTRA_LDFLAGS={ldflags}',
            f'-DEXTRA_GEN_DEFINES_ARGS={gen_defines_args}',
            f'-G{self.generator}'
        ]

        args = ["-D{}".format(a.replace('"', '')) for a in args]
        cmake_args.extend(args)

        cmake_opts = ['-DBOARD={}'.format(self.platform.name)]
        cmake_args.extend(cmake_opts)


        logger.debug("Calling cmake with arguments: {}".format(cmake_args))
        cmake = shutil.which('cmake')
        cmd = [cmake] + cmake_args
        kwargs = dict()

        if self.capture_output:
            kwargs['stdout'] = subprocess.PIPE
            # CMake sends the output of message() to stderr unless it's STATUS
            kwargs['stderr'] = subprocess.STDOUT

        if self.cwd:
            kwargs['cwd'] = self.cwd

        p = subprocess.Popen(cmd, **kwargs)
        out, _ = p.communicate()

        if p.returncode == 0:
            filter_results = self.parse_generated()
            msg = "Finished building %s for %s" % (self.source_dir, self.platform.name)
            logger.debug(msg)
            results = {'msg': msg, 'filter': filter_results}

        else:
            self.instance.status = "error"
            self.instance.reason = "Cmake build failure"

            for tc in self.instance.testcases:
                tc.status = self.instance.status

            logger.error("Cmake build failure: %s for %s" % (self.source_dir, self.platform.name))
            results = {"returncode": p.returncode}

        if out:
            with open(os.path.join(self.build_dir, self.log), "a", encoding=self.default_encoding) as log:
                log_msg = out.decode(self.default_encoding)
                log.write(log_msg)

        return results

    @staticmethod
    def run_cmake_script(args=[]):

        logger.debug("Running cmake script %s" % (args[0]))

        cmake_args = ["-D{}".format(a.replace('"', '')) for a in args[1:]]
        cmake_args.extend(['-P', args[0]])

        logger.debug("Calling cmake with arguments: {}".format(cmake_args))
        cmake = shutil.which('cmake')
        if not cmake:
            msg = "Unable to find `cmake` in path"
            logger.error(msg)
            raise Exception(msg)
        cmd = [cmake] + cmake_args

        kwargs = dict()
        kwargs['stdout'] = subprocess.PIPE
        # CMake sends the output of message() to stderr unless it's STATUS
        kwargs['stderr'] = subprocess.STDOUT

        p = subprocess.Popen(cmd, **kwargs)
        out, _ = p.communicate()

        # It might happen that the environment adds ANSI escape codes like \x1b[0m,
        # for instance if twister is executed from inside a makefile. In such a
        # scenario it is then necessary to remove them, as otherwise the JSON decoding
        # will fail.
        ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
        out = ansi_escape.sub('', out.decode())

        if p.returncode == 0:
            msg = "Finished running  %s" % (args[0])
            logger.debug(msg)
            results = {"returncode": p.returncode, "msg": msg, "stdout": out}

        else:
            logger.error("Cmake script failure: %s" % (args[0]))
            results = {"returncode": p.returncode, "returnmsg": out}

        return results


class FilterBuilder(CMake):

    def __init__(self, testsuite, platform, source_dir, build_dir):
        super().__init__(testsuite, platform, source_dir, build_dir)

        self.log = "config-twister.log"

    def parse_generated(self):

        if self.platform.name == "unit_testing":
            return {}

        cmake_cache_path = os.path.join(self.build_dir, "CMakeCache.txt")
        defconfig_path = os.path.join(self.build_dir, "zephyr", ".config")

        with open(defconfig_path, "r") as fp:
            defconfig = {}
            for line in fp.readlines():
                m = self.config_re.match(line)
                if not m:
                    if line.strip() and not line.startswith("#"):
                        sys.stderr.write("Unrecognized line %s\n" % line)
                    continue
                defconfig[m.group(1)] = m.group(2).strip()

        self.defconfig = defconfig

        cmake_conf = {}
        try:
            cache = CMakeCache.from_file(cmake_cache_path)
        except FileNotFoundError:
            cache = {}

        for k in iter(cache):
            cmake_conf[k.name] = k.value

        self.cmake_cache = cmake_conf

        filter_data = {
            "ARCH": self.platform.arch,
            "PLATFORM": self.platform.name
        }
        filter_data.update(os.environ)
        filter_data.update(self.defconfig)
        filter_data.update(self.cmake_cache)

        edt_pickle = os.path.join(self.build_dir, "zephyr", "edt.pickle")
        if self.testsuite and self.testsuite.ts_filter:
            try:
                if os.path.exists(edt_pickle):
                    with open(edt_pickle, 'rb') as f:
                        edt = pickle.load(f)
                else:
                    edt = None
                res = expr_parser.parse(self.testsuite.ts_filter, filter_data, edt)

            except (ValueError, SyntaxError) as se:
                sys.stderr.write(
                    "Failed processing %s\n" % self.testsuite.yamlfile)
                raise se

            if not res:
                return {os.path.join(self.platform.name, self.testsuite.name): True}
            else:
                return {os.path.join(self.platform.name, self.testsuite.name): False}
        else:
            self.platform.filter_data = filter_data
            return filter_data


class ProjectBuilder(FilterBuilder):

    def __init__(self, tplan, instance, **kwargs):
        super().__init__(instance.testsuite, instance.platform, instance.testsuite.source_dir, instance.build_dir)

        self.log = "build.log"
        self.instance = instance
        self.testplan = tplan
        self.filtered_tests = 0

        self.lsan = kwargs.get('lsan', False)
        self.asan = kwargs.get('asan', False)
        self.ubsan = kwargs.get('ubsan', False)
        self.valgrind = kwargs.get('valgrind', False)
        self.extra_args = kwargs.get('extra_args', [])
        self.device_testing = kwargs.get('device_testing', False)
        self.cmake_only = kwargs.get('cmake_only', False)
        self.cleanup = kwargs.get('cleanup', False)
        self.coverage = kwargs.get('coverage', False)
        self.inline_logs = kwargs.get('inline_logs', False)
        self.generator = kwargs.get('generator', None)
        self.generator_cmd = kwargs.get('generator_cmd', None)
        self.verbose = kwargs.get('verbose', None)
        self.warnings_as_errors = kwargs.get('warnings_as_errors', True)
        self.overflow_as_errors = kwargs.get('overflow_as_errors', False)
        self.suite_name_check = kwargs.get('suite_name_check', True)
        self.seed = kwargs.get('seed', 0)

    @staticmethod
    def log_info(filename, inline_logs):
        filename = os.path.abspath(os.path.realpath(filename))
        if inline_logs:
            logger.info("{:-^100}".format(filename))

            try:
                with open(filename) as fp:
                    data = fp.read()
            except Exception as e:
                data = "Unable to read log data (%s)\n" % (str(e))

            logger.error(data)

            logger.info("{:-^100}".format(filename))
        else:
            logger.error("see: " + Fore.YELLOW + filename + Fore.RESET)

    def log_info_file(self, inline_logs):
        build_dir = self.instance.build_dir
        h_log = "{}/handler.log".format(build_dir)
        b_log = "{}/build.log".format(build_dir)
        v_log = "{}/valgrind.log".format(build_dir)
        d_log = "{}/device.log".format(build_dir)

        if os.path.exists(v_log) and "Valgrind" in self.instance.reason:
            self.log_info("{}".format(v_log), inline_logs)
        elif os.path.exists(h_log) and os.path.getsize(h_log) > 0:
            self.log_info("{}".format(h_log), inline_logs)
        elif os.path.exists(d_log) and os.path.getsize(d_log) > 0:
            self.log_info("{}".format(d_log), inline_logs)
        else:
            self.log_info("{}".format(b_log), inline_logs)

    def setup_handler(self):

        instance = self.instance
        args = []

        # FIXME: Needs simplification
        if instance.platform.simulation == "qemu":
            instance.handler = QEMUHandler(instance, "qemu")
            args.append("QEMU_PIPE=%s" % instance.handler.get_fifo())
            instance.handler.call_make_run = True
        elif instance.testsuite.type == "unit":
            instance.handler = BinaryHandler(instance, "unit")
            instance.handler.binary = os.path.join(instance.build_dir, "testbinary")
            if self.coverage:
                args.append("COVERAGE=1")
        elif instance.platform.type == "native":
            handler = BinaryHandler(instance, "native")

            handler.asan = self.asan
            handler.valgrind = self.valgrind
            handler.lsan = self.lsan
            handler.ubsan = self.ubsan
            handler.coverage = self.coverage

            handler.binary = os.path.join(instance.build_dir, "zephyr", "zephyr.exe")
            instance.handler = handler
        elif instance.platform.simulation == "renode":
            if find_executable("renode"):
                instance.handler = BinaryHandler(instance, "renode")
                instance.handler.pid_fn = os.path.join(instance.build_dir, "renode.pid")
                instance.handler.call_make_run = True
        elif instance.platform.simulation == "tsim":
            instance.handler = BinaryHandler(instance, "tsim")
            instance.handler.call_make_run = True
        elif self.device_testing:
            instance.handler = DeviceHandler(instance, "device")
            instance.handler.coverage = self.coverage
        elif instance.platform.simulation == "nsim":
            if find_executable("nsimdrv"):
                instance.handler = BinaryHandler(instance, "nsim")
                instance.handler.call_make_run = True
        elif instance.platform.simulation == "mdb-nsim":
            if find_executable("mdb"):
                instance.handler = BinaryHandler(instance, "nsim")
                instance.handler.call_make_run = True
        elif instance.platform.simulation == "armfvp":
            instance.handler = BinaryHandler(instance, "armfvp")
            instance.handler.call_make_run = True
        elif instance.platform.simulation == "xt-sim":
            instance.handler = BinaryHandler(instance, "xt-sim")
            instance.handler.call_make_run = True

        if instance.handler:
            instance.handler.args = args
            instance.handler.generator_cmd = self.generator_cmd
            instance.handler.generator = self.generator
            instance.handler.suite_name_check = self.suite_name_check

    def process(self, pipeline, done, message, lock, results):
        op = message.get('op')

        if not self.instance.handler:
            self.setup_handler()

        # The build process, call cmake and build with configured generator
        if op == "cmake":
            res = self.cmake()
            if self.instance.status in ["failed", "error"]:
                pipeline.put({"op": "report", "test": self.instance})
            elif self.cmake_only:
                if self.instance.status is None:
                    self.instance.status = "passed"
                pipeline.put({"op": "report", "test": self.instance})
            else:
                # Here we check the runtime filter results coming from running cmake
                if self.instance.name in res['filter'] and res['filter'][self.instance.name]:
                    logger.debug("filtering %s" % self.instance.name)
                    self.instance.status = "filtered"
                    self.instance.reason = "runtime filter"
                    results.skipped_runtime += 1
                    self.instance.add_missing_case_status("skipped")
                    pipeline.put({"op": "report", "test": self.instance})
                else:
                    pipeline.put({"op": "build", "test": self.instance})

        elif op == "build":
            logger.debug("build test: %s" % self.instance.name)
            res = self.build()
            if not res:
                self.instance.status = "error"
                self.instance.reason = "Build Failure"
                pipeline.put({"op": "report", "test": self.instance})
            else:
                # Count skipped cases during build, for example
                # due to ram/rom overflow.
                if  self.instance.status == "skipped":
                    results.skipped_runtime += 1
                    self.instance.add_missing_case_status("skipped", self.instance.reason)

                if res.get('returncode', 1) > 0:
                    self.instance.add_missing_case_status("blocked", self.instance.reason)
                    pipeline.put({"op": "report", "test": self.instance})
                else:
                    pipeline.put({"op": "gather_metrics", "test": self.instance})

        elif op == "gather_metrics":
            self.gather_metrics(self.instance)
            if self.instance.run and self.instance.handler:
                pipeline.put({"op": "run", "test": self.instance})
            else:
                pipeline.put({"op": "report", "test": self.instance})

        # Run the generated binary using one of the supported handlers
        elif op == "run":
            logger.debug("run test: %s" % self.instance.name)
            self.run()
            logger.debug(f"run status: {self.instance.name} {self.instance.status}")

            # to make it work with pickle
            self.instance.handler.thread = None
            self.instance.handler.testplan = None
            pipeline.put({
                "op": "report",
                "test": self.instance,
                "status": self.instance.status,
                "reason": self.instance.reason
                }
            )

        # Report results and output progress to screen
        elif op == "report":
            with lock:
                done.put(self.instance)
                self.report_out(results)

            if self.cleanup and not self.coverage and self.instance.status == "passed":
                pipeline.put({
                    "op": "cleanup",
                    "test": self.instance
                })

        elif op == "cleanup":
            if self.device_testing:
                self.cleanup_device_testing_artifacts()
            else:
                self.cleanup_artifacts()

    def cleanup_artifacts(self, additional_keep=[]):
        logger.debug("Cleaning up {}".format(self.instance.build_dir))
        allow = [
            'zephyr/.config',
            'handler.log',
            'build.log',
            'device.log',
            'recording.csv',
            # below ones are needed to make --test-only work as well
            'Makefile',
            'CMakeCache.txt',
            'build.ninja',
            'CMakeFiles/rules.ninja'
            ]

        allow += additional_keep

        allow = [os.path.join(self.instance.build_dir, file) for file in allow]

        for dirpath, dirnames, filenames in os.walk(self.instance.build_dir, topdown=False):
            for name in filenames:
                path = os.path.join(dirpath, name)
                if path not in allow:
                    os.remove(path)
            # Remove empty directories and symbolic links to directories
            for dir in dirnames:
                path = os.path.join(dirpath, dir)
                if os.path.islink(path):
                    os.remove(path)
                elif not os.listdir(path):
                    os.rmdir(path)

    def cleanup_device_testing_artifacts(self):
        logger.debug("Cleaning up for Device Testing {}".format(self.instance.build_dir))

        sanitizelist = [
            'CMakeCache.txt',
            'zephyr/runners.yaml',
        ]
        keep = [
            'zephyr/zephyr.hex',
            'zephyr/zephyr.bin',
            'zephyr/zephyr.elf',
            ]

        keep += sanitizelist

        self.cleanup_artifacts(keep)

        # sanitize paths so files are relocatable
        for file in sanitizelist:
            file = os.path.join(self.instance.build_dir, file)

            with open(file, "rt") as fin:
                data = fin.read()
                data = data.replace(canonical_zephyr_base+"/", "")

            with open(file, "wt") as fin:
                fin.write(data)

    def report_out(self, results):
        total_to_do = results.total
        total_tests_width = len(str(total_to_do))
        results.done += 1
        instance = self.instance

        if instance.status in ["error", "failed"]:
            if instance.status == "error":
                results.error += 1
            else:
                results.failed += 1
            if self.verbose:
                status = Fore.RED + "FAILED " + Fore.RESET + instance.reason
            else:
                print("")
                logger.error(
                    "{:<25} {:<50} {}FAILED{}: {}".format(
                        instance.platform.name,
                        instance.testsuite.name,
                        Fore.RED,
                        Fore.RESET,
                        instance.reason))
            if not self.verbose:
                self.log_info_file(self.inline_logs)
        elif instance.status in ["skipped", "filtered"]:
            status = Fore.YELLOW + "SKIPPED" + Fore.RESET
            results.skipped_configs += 1
            results.skipped_cases += len(instance.testsuite.testcases)
        elif instance.status == "passed":
            status = Fore.GREEN + "PASSED" + Fore.RESET
            results.passed += 1
            for case in instance.testcases:
                if case.status == 'skipped':
                    results.skipped_cases += 1
        else:
            logger.debug(f"Unknown status = {instance.status}")
            status = Fore.YELLOW + "UNKNOWN" + Fore.RESET

        if self.verbose:
            if self.cmake_only:
                more_info = "cmake"
            elif instance.status in ["skipped", "filtered"]:
                more_info = instance.reason
            else:
                if instance.handler and instance.run:
                    more_info = instance.handler.type_str
                    htime = instance.execution_time
                    if htime:
                        more_info += " {:.3f}s".format(htime)
                else:
                    more_info = "build"

                if ( instance.status in ["error", "failed", "timeout", "flash_error"]
                     and hasattr(self.instance.handler, 'seed')
                     and self.instance.handler.seed is not None ):
                    more_info += "/seed: " + str(self.seed)

            logger.info("{:>{}}/{} {:<25} {:<50} {} ({})".format(
                results.done + results.skipped_filter, total_tests_width, total_to_do , instance.platform.name,
                instance.testsuite.name, status, more_info))

            if instance.status in ["error", "failed", "timeout"]:
                self.log_info_file(self.inline_logs)
        else:
            completed_perc = 0
            if total_to_do > 0:
                completed_perc = int((float(results.done + results.skipped_filter) / total_to_do) * 100)

            sys.stdout.write("\rINFO    - Total complete: %s%4d/%4d%s  %2d%%  skipped: %s%4d%s, failed: %s%4d%s" % (
                Fore.GREEN,
                results.done + results.skipped_filter,
                total_to_do,
                Fore.RESET,
                completed_perc,
                Fore.YELLOW if results.skipped_configs > 0 else Fore.RESET,
                results.skipped_filter + results.skipped_runtime,
                Fore.RESET,
                Fore.RED if results.failed > 0 else Fore.RESET,
                results.failed,
                Fore.RESET
            )
                             )
        sys.stdout.flush()

    def cmake(self):

        instance = self.instance
        args = self.testsuite.extra_args[:]
        args += self.extra_args

        if instance.handler:
            args += instance.handler.args

        # merge overlay files into one variable
        def extract_overlays(args):
            re_overlay = re.compile('OVERLAY_CONFIG=(.*)')
            other_args = []
            overlays = []
            for arg in args:
                match = re_overlay.search(arg)
                if match:
                    overlays.append(match.group(1).strip('\'"'))
                else:
                    other_args.append(arg)

            args[:] = other_args
            return overlays

        overlays = extract_overlays(args)

        if os.path.exists(os.path.join(instance.build_dir,
                                       "twister", "testsuite_extra.conf")):
            overlays.append(os.path.join(instance.build_dir,
                                         "twister", "testsuite_extra.conf"))

        if overlays:
            args.append("OVERLAY_CONFIG=\"%s\"" % (" ".join(overlays)))

        res = self.run_cmake(args)
        return res

    def build(self):
        res = self.run_build(['--build', self.build_dir])
        return res

    def run(self):

        instance = self.instance

        if instance.handler:
            if instance.handler.type_str == "device":
                instance.handler.testplan = self.testplan

            if(self.seed is not None and instance.platform.name.startswith("native_posix")):
                self.parse_generated()
                if('CONFIG_FAKE_ENTROPY_NATIVE_POSIX' in self.defconfig and
                    self.defconfig['CONFIG_FAKE_ENTROPY_NATIVE_POSIX'] == 'y'):
                    instance.handler.seed = self.seed

            instance.handler.handle()

        sys.stdout.flush()

    def gather_metrics(self, instance):
        if self.testplan.enable_size_report and not self.testplan.cmake_only:
            self.calc_one_elf_size(instance)
        else:
            instance.metrics["ram_size"] = 0
            instance.metrics["rom_size"] = 0
            instance.metrics["unrecognized"] = []

    @staticmethod
    def calc_one_elf_size(instance):
        if instance.status not in ["error", "failed", "skipped"]:
            if instance.platform.type != "native":
                size_calc = instance.calculate_sizes()
                instance.metrics["ram_size"] = size_calc.get_ram_size()
                instance.metrics["rom_size"] = size_calc.get_rom_size()
                instance.metrics["unrecognized"] = size_calc.unrecognized_sections()
            else:
                instance.metrics["ram_size"] = 0
                instance.metrics["rom_size"] = 0
                instance.metrics["unrecognized"] = []

            instance.metrics["handler_time"] = instance.execution_time

class Filters:
    # filters provided on command line by the user/tester
    CMD_LINE = 'command line filter'
    # filters in the testsuite yaml definition
    TESTSUITE = 'testsuite filter'
    # filters realted to platform definition
    PLATFORM = 'Platform related filter'
    # in case a testcase was quarantined.
    QUARENTINE = 'Quarantine filter'


class TestPlan(DisablePyTestCollectionMixin):
    config_re = re.compile('(CONFIG_[A-Za-z0-9_]+)[=]\"?([^\"]*)\"?$')
    dt_re = re.compile('([A-Za-z0-9_]+)[=]\"?([^\"]*)\"?$')

    ts_schema = scl.yaml_load(
        os.path.join(ZEPHYR_BASE,
                     "scripts", "schemas", "twister", "testsuite-schema.yaml"))
    quarantine_schema = scl.yaml_load(
        os.path.join(ZEPHYR_BASE,
                     "scripts", "schemas", "twister", "quarantine-schema.yaml"))

    testsuite_valid_keys = {"tags": {"type": "set", "required": False},
                       "type": {"type": "str", "default": "integration"},
                       "extra_args": {"type": "list"},
                       "extra_configs": {"type": "list"},
                       "build_only": {"type": "bool", "default": False},
                       "build_on_all": {"type": "bool", "default": False},
                       "skip": {"type": "bool", "default": False},
                       "slow": {"type": "bool", "default": False},
                       "timeout": {"type": "int", "default": 60},
                       "min_ram": {"type": "int", "default": 8},
                       "modules": {"type": "list", "default": []},
                       "depends_on": {"type": "set"},
                       "min_flash": {"type": "int", "default": 32},
                       "arch_allow": {"type": "set"},
                       "arch_exclude": {"type": "set"},
                       "extra_sections": {"type": "list", "default": []},
                       "integration_platforms": {"type": "list", "default": []},
                       "testcases": {"type": "list", "default": []},
                       "platform_type": {"type": "list", "default": []},
                       "platform_exclude": {"type": "set"},
                       "platform_allow": {"type": "set"},
                       "toolchain_exclude": {"type": "set"},
                       "toolchain_allow": {"type": "set"},
                       "filter": {"type": "str"},
                       "harness": {"type": "str"},
                       "harness_config": {"type": "map", "default": {}},
                       "seed": {"type": "int", "default": 0}
                       }

    SAMPLE_FILENAME = 'sample.yaml'
    TESTSUITE_FILENAME = 'testcase.yaml'

    def __init__(self, board_root_list=[], testsuite_roots=[], outdir=None):

        self.roots = testsuite_roots
        if not isinstance(board_root_list, list):
            self.board_roots = [board_root_list]
        else:
            self.board_roots = board_root_list

        # Test Plan Options
        self.coverage_platform = []
        self.build_only = False
        self.cmake_only = False
        self.cleanup = False
        self.enable_slow = False
        self.device_testing = False
        self.fixtures = []
        self.enable_coverage = False
        self.enable_ubsan = False
        self.enable_lsan = False
        self.enable_asan = False
        self.detailed_skipped_report = False
        self.enable_valgrind = False
        self.extra_args = []
        self.inline_logs = False
        self.enable_sizes_report = False
        self.west_flash = None
        self.west_runner = None
        self.generator = None
        self.generator_cmd = None
        self.warnings_as_errors = True
        self.overflow_as_errors = False
        self.quarantine_verify = False
        self.retry_build_errors = False
        self.suite_name_check = True
        self.seed = 0

        # Keep track of which test cases we've filtered out and why
        self.testsuites = {}
        self.quarantine = {}
        self.platforms = []
        self.platform_names = []
        self.selected_platforms = []
        self.filtered_platforms = []
        self.default_platforms = []
        self.outdir = os.path.abspath(outdir)
        self.load_errors = 0
        self.instances = dict()

        self.total_platforms = 0
        self.start_time = 0
        self.duration = 0
        self.warnings = 0

        # hardcoded for now
        self.duts = []

        # run integration tests only
        self.integration = False

        # used during creating shorter build paths
        self.link_dir_counter = 0

        self.pipeline = None
        self.version = "NA"

        self.modules = []

        self.timestamp = datetime.now().isoformat()

    def check_zephyr_version(self):
        try:
            subproc = subprocess.run(["git", "describe", "--abbrev=12", "--always"],
                                     stdout=subprocess.PIPE,
                                     universal_newlines=True,
                                     cwd=ZEPHYR_BASE)
            if subproc.returncode == 0:
                self.version = subproc.stdout.strip()
                logger.info(f"Zephyr version: {self.version}")
        except OSError:
            logger.info("Cannot read zephyr version.")

    def get_platform_instances(self, platform):
        filtered_dict = {k:v for k,v in self.instances.items() if k.startswith(platform + os.sep)}
        return filtered_dict

    def config(self):
        logger.info("coverage platform: {}".format(self.coverage_platform))

    # Debug Functions
    @staticmethod
    def info(what):
        sys.stdout.write(what + "\n")
        sys.stdout.flush()

    def update_counting(self, results=None):
        for instance in self.instances.values():
            results.cases += len(instance.testsuite.testcases)
            if instance.status == 'filtered':
                results.skipped_filter += 1
                results.skipped_configs += 1
            elif instance.status == 'passed':
                results.passed += 1
                results.done += 1
            elif instance.status == 'error':
                results.error += 1
                results.done += 1

    def compare_metrics(self, filename):
        # name, datatype, lower results better
        interesting_metrics = [("ram_size", int, True),
                               ("rom_size", int, True)]

        if not os.path.exists(filename):
            logger.error("Cannot compare metrics, %s not found" % filename)
            return []

        results = []
        saved_metrics = {}
        with open(filename) as fp:
            jt = json.load(fp)
            for ts in jt.get("testsuites", []):
                d = {}
                for m, _, _ in interesting_metrics:
                    d[m] = ts.get(m, 0)
                ts_name = ts.get('name')
                ts_platform = ts.get('platform')
                saved_metrics[(ts_name, ts_platform)] = d

        for instance in self.instances.values():
            mkey = (instance.testsuite.name, instance.platform.name)
            if mkey not in saved_metrics:
                continue
            sm = saved_metrics[mkey]
            for metric, mtype, lower_better in interesting_metrics:
                if metric not in instance.metrics:
                    continue
                if sm[metric] == "":
                    continue
                delta = instance.metrics.get(metric, 0) - mtype(sm[metric])
                if delta == 0:
                    continue
                results.append((instance, metric, instance.metrics.get(metric, 0), delta,
                                lower_better))
        return results

    def footprint_reports(self, report, show_footprint, all_deltas,
                          footprint_threshold, last_metrics):
        if not report:
            return

        logger.debug("running footprint_reports")
        deltas = self.compare_metrics(report)
        warnings = 0
        if deltas and show_footprint:
            for i, metric, value, delta, lower_better in deltas:
                if not all_deltas and ((delta < 0 and lower_better) or
                                       (delta > 0 and not lower_better)):
                    continue

                percentage = 0
                if value > delta:
                    percentage = (float(delta) / float(value - delta))

                if not all_deltas and (percentage < (footprint_threshold / 100.0)):
                    continue

                logger.info("{:<25} {:<60} {}{}{}: {} {:<+4}, is now {:6} {:+.2%}".format(
                    i.platform.name, i.testsuite.name, Fore.YELLOW,
                    "INFO" if all_deltas else "WARNING", Fore.RESET,
                    metric, delta, value, percentage))
                warnings += 1

        if warnings:
            logger.warning("Deltas based on metrics from last %s" %
                           ("release" if not last_metrics else "run"))

    def summary(self, results, unrecognized_sections):
        failed = 0
        run = 0
        for instance in self.instances.values():
            if instance.status == "failed":
                failed += 1
            elif instance.metrics.get("unrecognized") and not unrecognized_sections:
                logger.error("%sFAILED%s: %s has unrecognized binary sections: %s" %
                             (Fore.RED, Fore.RESET, instance.name,
                              str(instance.metrics.get("unrecognized", []))))
                failed += 1

            # FIXME: need a better way to identify executed tests
            handler_time = instance.metrics.get('handler_time', 0)
            if float(handler_time) > 0:
                run += 1

        if results.total and results.total != results.skipped_configs:
            pass_rate = (float(results.passed) / float(results.total - results.skipped_configs))
        else:
            pass_rate = 0

        logger.info(
            "{}{} of {}{} test configurations passed ({:.2%}), {}{}{} failed, {} skipped with {}{}{} warnings in {:.2f} seconds".format(
                Fore.RED if failed else Fore.GREEN,
                results.passed,
                results.total,
                Fore.RESET,
                pass_rate,
                Fore.RED if results.failed else Fore.RESET,
                results.failed + results.error,
                Fore.RESET,
                results.skipped_configs,
                Fore.YELLOW if self.warnings else Fore.RESET,
                self.warnings,
                Fore.RESET,
                self.duration))

        self.total_platforms = len(self.platforms)
        # if we are only building, do not report about tests being executed.
        if self.platforms and not self.build_only:
            logger.info("In total {} test cases were executed, {} skipped on {} out of total {} platforms ({:02.2f}%)".format(
                results.cases - results.skipped_cases,
                results.skipped_cases,
                len(self.filtered_platforms),
                self.total_platforms,
                (100 * len(self.filtered_platforms) / len(self.platforms))
            ))

        built_only = results.total - run - results.skipped_configs
        logger.info(f"{Fore.GREEN}{run}{Fore.RESET} test configurations executed on platforms, \
{Fore.RED}{built_only}{Fore.RESET} test configurations were only built.")

    def save_reports(self, name, suffix, report_dir, no_update, platform_reports):
        if not self.instances:
            return

        logger.info("Saving reports...")
        if name:
            report_name = name
        else:
            report_name = "twister"

        if report_dir:
            os.makedirs(report_dir, exist_ok=True)
            filename = os.path.join(report_dir, report_name)
            outdir = report_dir
        else:
            filename = os.path.join(self.outdir, report_name)
            outdir = self.outdir

        if suffix:
            filename = "{}_{}".format(filename, suffix)

        if not no_update:
            json_file = filename + ".json"
            self.json_report(json_file, version=self.version)
            self.xunit_report(json_file, filename + ".xml", full_report=False)
            self.xunit_report(json_file, filename + "_report.xml", full_report=True)
            self.xunit_report_suites(json_file, filename + "_suite_report.xml")

            if platform_reports:
                self.target_report(json_file, outdir, suffix)


    def target_report(self, json_file, outdir, suffix):
        platforms = {inst.platform.name for _, inst in self.instances.items()}
        for platform in platforms:
            if suffix:
                filename = os.path.join(outdir,"{}_{}.xml".format(platform, suffix))
            else:
                filename = os.path.join(outdir,"{}.xml".format(platform))
            self.xunit_report(json_file, filename, platform, full_report=True)


    def add_configurations(self):

        for board_root in self.board_roots:
            board_root = os.path.abspath(board_root)

            logger.debug("Reading platform configuration files under %s..." %
                         board_root)

            for file in glob.glob(os.path.join(board_root, "*", "*", "*.yaml")):
                try:
                    platform = Platform()
                    platform.load(file)
                    if platform.name in [p.name for p in self.platforms]:
                        logger.error(f"Duplicate platform {platform.name} in {file}")
                        raise Exception(f"Duplicate platform identifier {platform.name} found")
                    if platform.twister:
                        self.platforms.append(platform)
                        if platform.default:
                            self.default_platforms.append(platform.name)

                except RuntimeError as e:
                    logger.error("E: %s: can't load: %s" % (file, e))
                    self.load_errors += 1

        self.platform_names = [p.name for p in self.platforms]

    def get_all_tests(self):
        testcases = []
        for _, ts in self.testsuites.items():
            for case in ts.testcases:
                testcases.append(case)

        return testcases

    @staticmethod
    def get_toolchain():
        toolchain_script = Path(ZEPHYR_BASE) / Path('cmake/modules/verify-toolchain.cmake')
        result = CMake.run_cmake_script([toolchain_script, "FORMAT=json"])

        try:
            if result['returncode']:
                raise TwisterRuntimeError(f"E: {result['returnmsg']}")
        except Exception as e:
            print(str(e))
            sys.exit(2)
        toolchain = json.loads(result['stdout'])['ZEPHYR_TOOLCHAIN_VARIANT']
        logger.info(f"Using '{toolchain}' toolchain.")

        return toolchain

    def add_testsuites(self, testsuite_filter=[]):
        for root in self.roots:
            root = os.path.abspath(root)

            logger.debug("Reading test case configuration files under %s..." % root)

            for dirpath, _, filenames in os.walk(root, topdown=True):
                if self.SAMPLE_FILENAME in filenames:
                    filename = self.SAMPLE_FILENAME
                elif self.TESTSUITE_FILENAME in filenames:
                    filename = self.TESTSUITE_FILENAME
                else:
                    continue

                logger.debug("Found possible test case in " + dirpath)

                ts_path = os.path.join(dirpath, filename)

                try:
                    parsed_data = TwisterConfigParser(ts_path, self.ts_schema)
                    parsed_data.load()

                    ts_path = os.path.dirname(ts_path)
                    workdir = os.path.relpath(ts_path, root)

                    for name in parsed_data.tests.keys():
                        ts = TestSuite(root, workdir, name)

                        ts_dict = parsed_data.get_test(name, self.testsuite_valid_keys)

                        ts.source_dir = ts_path
                        ts.yamlfile = ts_path

                        ts.type = ts_dict["type"]
                        ts.tags = ts_dict["tags"]
                        ts.extra_args = ts_dict["extra_args"]
                        ts.extra_configs = ts_dict["extra_configs"]
                        ts.arch_allow = ts_dict["arch_allow"]
                        ts.arch_exclude = ts_dict["arch_exclude"]
                        ts.skip = ts_dict["skip"]
                        ts.platform_exclude = ts_dict["platform_exclude"]
                        ts.platform_allow = ts_dict["platform_allow"]
                        ts.platform_type = ts_dict["platform_type"]
                        ts.toolchain_exclude = ts_dict["toolchain_exclude"]
                        ts.toolchain_allow = ts_dict["toolchain_allow"]
                        ts.ts_filter = ts_dict["filter"]
                        ts.timeout = ts_dict["timeout"]
                        ts.harness = ts_dict["harness"]
                        ts.harness_config = ts_dict["harness_config"]
                        if ts.harness == 'console' and not ts.harness_config:
                            raise Exception('Harness config error: console harness defined without a configuration.')
                        ts.build_only = ts_dict["build_only"]
                        ts.build_on_all = ts_dict["build_on_all"]
                        ts.slow = ts_dict["slow"]
                        ts.min_ram = ts_dict["min_ram"]
                        ts.modules = ts_dict["modules"]
                        ts.depends_on = ts_dict["depends_on"]
                        ts.min_flash = ts_dict["min_flash"]
                        ts.extra_sections = ts_dict["extra_sections"]
                        ts.integration_platforms = ts_dict["integration_platforms"]
                        ts.seed = ts_dict["seed"]

                        testcases = ts_dict.get("testcases", [])
                        if testcases:
                            for tc in testcases:
                                ts.add_testcase(name=f"{name}.{tc}")
                        else:
                            ts.parse_subcases(ts_path)

                        if testsuite_filter:
                            if ts.name and ts.name in testsuite_filter:
                                self.testsuites[ts.name] = ts
                        else:
                            self.testsuites[ts.name] = ts

                except Exception as e:
                    logger.error("%s: can't load (skipping): %s" % (ts_path, e))
                    self.load_errors += 1
        return len(self.testsuites)

    def get_platform(self, name):
        selected_platform = None
        for platform in self.platforms:
            if platform.name == name:
                selected_platform = platform
                break
        return selected_platform

    def load_quarantine(self, file):
        """
        Loads quarantine list from the given yaml file. Creates a dictionary
        of all tests configurations (platform + scenario: comment) that shall be
        skipped due to quarantine
        """

        # Load yaml into quarantine_yaml
        quarantine_yaml = scl.yaml_load_verify(file, self.quarantine_schema)

        # Create quarantine_list with a product of the listed
        # platforms and scenarios for each entry in quarantine yaml
        quarantine_list = []
        for quar_dict in quarantine_yaml:
            if quar_dict['platforms'][0] == "all":
                plat = self.platform_names
            else:
                plat = quar_dict['platforms']
            comment = quar_dict.get('comment', "NA")
            quarantine_list.append([{".".join([p, s]): comment}
                                   for p in plat for s in quar_dict['scenarios']])

        # Flatten the quarantine_list
        quarantine_list = [it for sublist in quarantine_list for it in sublist]
        # Change quarantine_list into a dictionary
        for d in quarantine_list:
            self.quarantine.update(d)

    def load_from_file(self, file, filter_platform=[]):
        with open(file, "r") as json_test_plan:
            jtp = json.load(json_test_plan)
            instance_list = []
            for ts in jtp.get("testsuites", []):
                logger.debug(f"loading {ts['name']}...")
                testsuite = ts["name"]

                platform = self.get_platform(ts["platform"])
                if filter_platform and platform.name not in filter_platform:
                    continue
                instance = TestInstance(self.testsuites[testsuite], platform, self.outdir)
                if ts.get("run_id"):
                    instance.run_id = ts.get("run_id")

                if self.device_testing:
                    tfilter = 'runnable'
                else:
                    tfilter = 'buildable'
                instance.run = instance.check_runnable(
                    self.enable_slow,
                    tfilter,
                    self.fixtures
                )

                instance.metrics['handler_time'] = ts.get('execution_time', 0)
                instance.metrics['ram_size'] = ts.get("ram_size", 0)
                instance.metrics['rom_size']  = ts.get("rom_size",0)

                status = ts.get('status', None)
                reason = ts.get("reason", "Unknown")
                if status in ["error", "failed"]:
                    instance.status = None
                    instance.reason = None
                # test marked as passed (built only) but can run when
                # --test-only is used. Reset status to capture new results.
                elif status == 'passed' and instance.run and self.test_only:
                    instance.status = None
                    instance.reason = None
                else:
                    instance.status = status
                    instance.reason = reason

                for tc in ts.get('testcases', []):
                    identifier = tc['identifier']
                    tc_status = tc.get('status', None)
                    tc_reason = None
                    # we set reason only if status is valid, it might have been
                    # reset above...
                    if instance.status:
                        tc_reason = tc.get('reason')
                    if tc_status:
                        case = instance.set_case_status_by_name(identifier, tc_status, tc_reason)
                        case.duration = tc.get('execution_time', 0)
                        if tc.get('log'):
                            case.output = tc.get('log')


                instance.create_overlay(platform, self.enable_asan, self.enable_ubsan, self.enable_coverage, self.coverage_platform)
                instance_list.append(instance)
            self.add_instances(instance_list)

    def apply_filters(self, **kwargs):

        toolchain = self.get_toolchain()

        platform_filter = kwargs.get('platform')
        exclude_platform = kwargs.get('exclude_platform', [])
        testsuite_filter = kwargs.get('run_individual_tests', [])
        arch_filter = kwargs.get('arch')
        tag_filter = kwargs.get('tag')
        exclude_tag = kwargs.get('exclude_tag')
        all_filter = kwargs.get('all')
        runnable = kwargs.get('runnable')
        force_toolchain = kwargs.get('force_toolchain')
        force_platform = kwargs.get('force_platform')
        emu_filter = kwargs.get('emulation_only')

        logger.debug("platform filter: " + str(platform_filter))
        logger.debug("    arch_filter: " + str(arch_filter))
        logger.debug("     tag_filter: " + str(tag_filter))
        logger.debug("    exclude_tag: " + str(exclude_tag))

        default_platforms = False
        emulation_platforms = False


        if all_filter:
            logger.info("Selecting all possible platforms per test case")
            # When --all used, any --platform arguments ignored
            platform_filter = []
        elif not platform_filter and not emu_filter:
            logger.info("Selecting default platforms per test case")
            default_platforms = True
        elif emu_filter:
            logger.info("Selecting emulation platforms per test case")
            emulation_platforms = True

        if platform_filter:
            self.verify_platforms_existence(platform_filter, f"platform_filter")
            platforms = list(filter(lambda p: p.name in platform_filter, self.platforms))
        elif emu_filter:
            platforms = list(filter(lambda p: p.simulation != 'na', self.platforms))
        elif arch_filter:
            platforms = list(filter(lambda p: p.arch in arch_filter, self.platforms))
        elif default_platforms:
            platforms = list(filter(lambda p: p.default, self.platforms))
        else:
            platforms = self.platforms

        logger.info("Building initial testsuite list...")

        for ts_name, ts in self.testsuites.items():

            if ts.build_on_all and not platform_filter:
                platform_scope = self.platforms
            elif ts.integration_platforms and self.integration:
                self.verify_platforms_existence(
                    ts.integration_platforms, f"{ts_name} - integration_platforms")
                platform_scope = list(filter(lambda item: item.name in ts.integration_platforms, \
                                         self.platforms))
            else:
                platform_scope = platforms

            integration = self.integration and ts.integration_platforms

            # If there isn't any overlap between the platform_allow list and the platform_scope
            # we set the scope to the platform_allow list
            if ts.platform_allow and not platform_filter and not integration:
                self.verify_platforms_existence(
                    ts.platform_allow, f"{ts_name} - platform_allow")
                a = set(platform_scope)
                b = set(filter(lambda item: item.name in ts.platform_allow, self.platforms))
                c = a.intersection(b)
                if not c:
                    platform_scope = list(filter(lambda item: item.name in ts.platform_allow, \
                                             self.platforms))

            # list of instances per testsuite, aka configurations.
            instance_list = []
            for plat in platform_scope:
                instance = TestInstance(ts, plat, self.outdir)
                if runnable:
                    tfilter = 'runnable'
                else:
                    tfilter = 'buildable'

                instance.run = instance.check_runnable(
                    self.enable_slow,
                    tfilter,
                    self.fixtures
                )
                if runnable and self.duts:
                    for h in self.duts:
                        if h.platform == plat.name:
                            if ts.harness_config.get('fixture') in h.fixtures:
                                instance.run = True

                if not force_platform and plat.name in exclude_platform:
                    instance.add_filter("Platform is excluded on command line.", Filters.CMD_LINE)

                if (plat.arch == "unit") != (ts.type == "unit"):
                    # Discard silently
                    continue

                if ts.modules and self.modules:
                    if not set(ts.modules).issubset(set(self.modules)):
                        instance.add_filter(f"one or more required modules not available: {','.join(ts.modules)}", Filters.TESTSUITE)

                if runnable and not instance.run:
                    instance.add_filter("Not runnable on device", Filters.PLATFORM)

                if self.integration and ts.integration_platforms and plat.name not in ts.integration_platforms:
                    instance.add_filter("Not part of integration platforms", Filters.TESTSUITE)

                if ts.skip:
                    instance.add_filter("Skip filter", Filters.TESTSUITE)

                if tag_filter and not ts.tags.intersection(tag_filter):
                    instance.add_filter("Command line testsuite tag filter", Filters.CMD_LINE)

                if exclude_tag and ts.tags.intersection(exclude_tag):
                    instance.add_filter("Command line testsuite exclude filter", Filters.CMD_LINE)

                if testsuite_filter and ts_name not in testsuite_filter:
                    instance.add_filter("TestSuite name filter", Filters.CMD_LINE)

                if arch_filter and plat.arch not in arch_filter:
                    instance.add_filter("Command line testsuite arch filter", Filters.CMD_LINE)

                if not force_platform:

                    if ts.arch_allow and plat.arch not in ts.arch_allow:
                        instance.add_filter("Not in test case arch allow list", Filters.TESTSUITE)

                    if ts.arch_exclude and plat.arch in ts.arch_exclude:
                        instance.add_filter("In test case arch exclude", Filters.TESTSUITE)

                    if ts.platform_exclude and plat.name in ts.platform_exclude:
                        instance.add_filter("In test case platform exclude", Filters.TESTSUITE)

                if ts.toolchain_exclude and toolchain in ts.toolchain_exclude:
                    instance.add_filter("In test case toolchain exclude", Filters.TESTSUITE)

                if platform_filter and plat.name not in platform_filter:
                    instance.add_filter("Command line platform filter", Filters.CMD_LINE)

                if ts.platform_allow and plat.name not in ts.platform_allow:
                    instance.add_filter("Not in testsuite platform allow list", Filters.TESTSUITE)

                if ts.platform_type and plat.type not in ts.platform_type:
                    instance.add_filter("Not in testsuite platform type list", Filters.TESTSUITE)

                if ts.toolchain_allow and toolchain not in ts.toolchain_allow:
                    instance.add_filter("Not in testsuite toolchain allow list", Filters.TESTSUITE)

                if not plat.env_satisfied:
                    instance.add_filter("Environment ({}) not satisfied".format(", ".join(plat.env)), Filters.PLATFORM)

                if not force_toolchain \
                        and toolchain and (toolchain not in plat.supported_toolchains) \
                        and "host" not in plat.supported_toolchains \
                        and ts.type != 'unit':
                    instance.add_filter("Not supported by the toolchain", Filters.PLATFORM)

                if plat.ram < ts.min_ram:
                    instance.add_filter("Not enough RAM", Filters.PLATFORM)

                if ts.depends_on:
                    dep_intersection = ts.depends_on.intersection(set(plat.supported))
                    if dep_intersection != set(ts.depends_on):
                        instance.add_filter("No hardware support", Filters.PLATFORM)

                if plat.flash < ts.min_flash:
                    instance.add_filter("Not enough FLASH", Filters.PLATFORM)

                if set(plat.ignore_tags) & ts.tags:
                    instance.add_filter("Excluded tags per platform (exclude_tags)", Filters.PLATFORM)

                if plat.only_tags and not set(plat.only_tags) & ts.tags:
                    instance.add_filter("Excluded tags per platform (only_tags)", Filters.PLATFORM)

                test_configuration = ".".join([instance.platform.name,
                                               instance.testsuite.id])
                # skip quarantined tests
                if test_configuration in self.quarantine and not self.quarantine_verify:
                    instance.add_filter(f"Quarantine: {self.quarantine[test_configuration]}", Filters.QUARENTINE)
                # run only quarantined test to verify their statuses (skip everything else)
                if self.quarantine_verify and test_configuration not in self.quarantine:
                    instance.add_filter("Not under quarantine", Filters.QUARENTINE)

                # if nothing stopped us until now, it means this configuration
                # needs to be added.
                instance_list.append(instance)

            # no configurations, so jump to next testsuite
            if not instance_list:
                continue

            # if twister was launched with no platform options at all, we
            # take all default platforms
            if default_platforms and not ts.build_on_all and not integration:
                if ts.platform_allow:
                    a = set(self.default_platforms)
                    b = set(ts.platform_allow)
                    c = a.intersection(b)
                    if c:
                        aa = list(filter(lambda ts: ts.platform.name in c, instance_list))
                        self.add_instances(aa)
                    else:
                        self.add_instances(instance_list)
                else:
                    instances = list(filter(lambda ts: ts.platform.default, instance_list))
                    self.add_instances(instances)
            elif integration:
                instances = list(filter(lambda item:  item.platform.name in ts.integration_platforms, instance_list))
                self.add_instances(instances)

            elif emulation_platforms:
                self.add_instances(instance_list)
                for instance in list(filter(lambda inst: not inst.platform.simulation != 'na', instance_list)):
                    instance.add_filter("Not an emulated platform", Filters.PLATFORM)
            else:
                self.add_instances(instance_list)

        for _, case in self.instances.items():
            case.create_overlay(case.platform, self.enable_asan, self.enable_ubsan, self.enable_coverage, self.coverage_platform)

        self.selected_platforms = set(p.platform.name for p in self.instances.values())

        filtered_instances = list(filter(lambda item:  item.status == "filtered", self.instances.values()))
        for filtered_instance in filtered_instances:
            # If integration mode is on all skips on integration_platforms are treated as errors.
            if self.integration and filtered_instance.platform.name in filtered_instance.testsuite.integration_platforms \
                and "Quarantine" not in filtered_instance.reason:
                # Do not treat this as error if filter type is command line
                filters = {t['type'] for t in filtered_instance.filters}
                if Filters.CMD_LINE in filters:
                    continue
                filtered_instance.status = "error"
                filtered_instance.reason += " but is one of the integration platforms"
                self.instances[filtered_instance.name] = filtered_instance

            filtered_instance.add_missing_case_status(filtered_instance.status)

        self.filtered_platforms = set(p.platform.name for p in self.instances.values()
                                      if p.status != "skipped" )

    def add_instances(self, instance_list):
        for instance in instance_list:
            self.instances[instance.name] = instance

    def add_tasks_to_queue(self, pipeline, build_only=False, test_only=False, retry_build_errors=False):
        for instance in self.instances.values():
            if build_only:
                instance.run = False

            no_retry_statuses = ['passed', 'skipped', 'filtered']
            if not retry_build_errors:
                no_retry_statuses.append("error")

            if instance.status not in no_retry_statuses:
                logger.debug(f"adding {instance.name}")
                instance.status = None
                if test_only and instance.run:
                    pipeline.put({"op": "run", "test": instance})
                else:
                    pipeline.put({"op": "cmake", "test": instance})

    def pipeline_mgr(self, pipeline, done_queue, lock, results):
        while True:
            try:
                task = pipeline.get_nowait()
            except queue.Empty:
                break
            else:
                test = task['test']
                pb = ProjectBuilder(self,
                                    test,
                                    lsan=self.enable_lsan,
                                    asan=self.enable_asan,
                                    ubsan=self.enable_ubsan,
                                    coverage=self.enable_coverage,
                                    extra_args=self.extra_args,
                                    device_testing=self.device_testing,
                                    cmake_only=self.cmake_only,
                                    cleanup=self.cleanup,
                                    valgrind=self.enable_valgrind,
                                    inline_logs=self.inline_logs,
                                    generator=self.generator,
                                    generator_cmd=self.generator_cmd,
                                    verbose=self.verbose,
                                    warnings_as_errors=self.warnings_as_errors,
                                    overflow_as_errors=self.overflow_as_errors,
                                    suite_name_check=self.suite_name_check,
                                    seed=self.seed
                                    )
                pb.process(pipeline, done_queue, task, lock, results)

        return True

    def execute(self, pipeline, done, results):
        lock = Lock()
        logger.info("Adding tasks to the queue...")
        self.add_tasks_to_queue(pipeline, self.build_only, self.test_only,
                                retry_build_errors=self.retry_build_errors)
        logger.info("Added initial list of jobs to queue")

        processes = []
        for job in range(self.jobs):
            logger.debug(f"Launch process {job}")
            p = Process(target=self.pipeline_mgr, args=(pipeline, done, lock, results, ))
            processes.append(p)
            p.start()

        try:
            for p in processes:
                p.join()
        except KeyboardInterrupt:
            logger.info("Execution interrupted")
            for p in processes:
                p.terminate()

        return results

    @staticmethod
    def process_log(log_file):
        filtered_string = ""
        if os.path.exists(log_file):
            with open(log_file, "rb") as f:
                log = f.read().decode("utf-8")
                filtered_string = ''.join(filter(lambda x: x in string.printable, log))

        return filtered_string


    @staticmethod
    def xunit_testcase(eleTestsuite, name, classname, status, ts_status, reason, duration, runnable, stats, log, build_only_as_skip):
        fails, passes, errors, skips = stats

        if status in ['skipped', 'filtered']:
            duration = 0

        eleTestcase = ET.SubElement(
            eleTestsuite, "testcase",
            classname=classname,
            name=f"{name}",
            time=f"{duration}")

        if status in ['skipped', 'filtered']:
            skips += 1
            # temporarily add build_only_as_skip to restore existing CI report behaviour
            if ts_status == "passed" and not runnable:
                tc_type = "build"
            else:
                tc_type = status
            ET.SubElement(eleTestcase, 'skipped', type=f"{tc_type}", message=f"{reason}")
        elif status in ["failed", "blocked"]:
            fails += 1
            el = ET.SubElement(eleTestcase, 'failure', type="failure", message=f"{reason}")
            if log:
                el.text = log
        elif status == "error":
            errors += 1
            el = ET.SubElement(eleTestcase, 'error', type="failure", message=f"{reason}")
            if log:
                el.text = log
        elif status == 'passed':
            if not runnable and build_only_as_skip:
                ET.SubElement(eleTestcase, 'skipped', type="build", message="built only")
                skips += 1
            else:
                passes += 1
        else:
            if not status:
                logger.debug(f"{name}: No status")
                ET.SubElement(eleTestcase, 'skipped', type=f"untested", message="No results captured, testsuite misconfiguration?")
            else:
                logger.error(f"{name}: Unknown status '{status}'")

        return (fails, passes, errors, skips)

    # Generate a report with all testsuites instead of doing this per platform
    def xunit_report_suites(self, json_file, filename):

        json_data = {}
        with open(json_file, "r") as json_results:
            json_data = json.load(json_results)


        env = json_data.get('environment', {})
        version = env.get('zephyr_version', None)

        eleTestsuites = ET.Element('testsuites')
        all_suites = json_data.get("testsuites", [])

        suites_to_report = all_suites
            # do not create entry if everything is filtered out
        if not self.detailed_skipped_report:
            suites_to_report = list(filter(lambda d: d.get('status') != "filtered", all_suites))

        for suite in suites_to_report:
            duration = 0
            eleTestsuite = ET.SubElement(eleTestsuites, 'testsuite',
                                            name=suite.get("name"), time="0",
                                            timestamp = self.timestamp,
                                            tests="0",
                                            failures="0",
                                            errors="0", skipped="0")
            eleTSPropetries = ET.SubElement(eleTestsuite, 'properties')
            # Multiple 'property' can be added to 'properties'
            # differing by name and value
            ET.SubElement(eleTSPropetries, 'property', name="version", value=version)
            ET.SubElement(eleTSPropetries, 'property', name="platform", value=suite.get("platform"))
            ET.SubElement(eleTSPropetries, 'property', name="architecture", value=suite.get("arch"))

            total = 0
            fails = passes = errors = skips = 0
            handler_time = suite.get('execution_time', 0)
            runnable = suite.get('runnable', 0)
            duration += float(handler_time)
            ts_status = suite.get('status')
            for tc in suite.get("testcases", []):
                status = tc.get('status')
                reason = tc.get('reason', suite.get('reason', 'Unknown'))
                log = tc.get("log", suite.get("log"))

                tc_duration = tc.get('execution_time', handler_time)
                name = tc.get("identifier")
                classname = ".".join(name.split(".")[:2])
                fails, passes, errors, skips = self.xunit_testcase(eleTestsuite,
                    name, classname, status, ts_status, reason, tc_duration, runnable,
                    (fails, passes, errors, skips), log, True)

            total = (errors + passes + fails + skips)

            eleTestsuite.attrib['time'] = f"{duration}"
            eleTestsuite.attrib['failures'] = f"{fails}"
            eleTestsuite.attrib['errors'] = f"{errors}"
            eleTestsuite.attrib['skipped'] = f"{skips}"
            eleTestsuite.attrib['tests'] = f"{total}"

        result = ET.tostring(eleTestsuites)
        with open(filename, 'wb') as report:
            report.write(result)

    def xunit_report(self, json_file, filename, selected_platform=None, full_report=False):
        if selected_platform:
            selected = [selected_platform]
            logger.info(f"Writing target report for {selected_platform}...")
        else:
            logger.info(f"Writing xunit report {filename}...")
            selected = self.selected_platforms

        json_data = {}
        with open(json_file, "r") as json_results:
            json_data = json.load(json_results)


        env = json_data.get('environment', {})
        version = env.get('zephyr_version', None)

        eleTestsuites = ET.Element('testsuites')
        all_suites = json_data.get("testsuites", [])

        for platform in selected:
            suites = list(filter(lambda d: d['platform'] == platform, all_suites))
            # do not create entry if everything is filtered out
            if not self.detailed_skipped_report:
                non_filtered = list(filter(lambda d: d.get('status') != "filtered", suites))
                if not non_filtered:
                    continue

            duration = 0
            eleTestsuite = ET.SubElement(eleTestsuites, 'testsuite',
                                            name=platform,
                                            timestamp = self.timestamp,
                                            time="0",
                                            tests="0",
                                            failures="0",
                                            errors="0", skipped="0")
            eleTSPropetries = ET.SubElement(eleTestsuite, 'properties')
            # Multiple 'property' can be added to 'properties'
            # differing by name and value
            ET.SubElement(eleTSPropetries, 'property', name="version", value=version)

            total = 0
            fails = passes = errors = skips = 0
            for ts in suites:
                handler_time = ts.get('execution_time', 0)
                runnable = ts.get('runnable', 0)
                duration += float(handler_time)

                ts_status = ts.get('status')
                # Do not report filtered testcases
                if ts_status == 'filtered' and not self.detailed_skipped_report:
                    continue
                if full_report:
                    for tc in ts.get("testcases", []):
                        status = tc.get('status')
                        reason = tc.get('reason', ts.get('reason', 'Unknown'))
                        log = tc.get("log", ts.get("log"))

                        tc_duration = tc.get('execution_time', handler_time)
                        name = tc.get("identifier")
                        classname = ".".join(name.split(".")[:2])
                        fails, passes, errors, skips = self.xunit_testcase(eleTestsuite,
                            name, classname, status, ts_status, reason, tc_duration, runnable,
                            (fails, passes, errors, skips), log, True)
                else:
                    reason = ts.get('reason', 'Unknown')
                    name = ts.get("name")
                    classname = f"{platform}:{name}"
                    log = ts.get("log")
                    fails, passes, errors, skips = self.xunit_testcase(eleTestsuite,
                        name, classname, ts_status, ts_status, reason, duration, runnable,
                        (fails, passes, errors, skips), log, False)

            total = (errors + passes + fails + skips)

            eleTestsuite.attrib['time'] = f"{duration}"
            eleTestsuite.attrib['failures'] = f"{fails}"
            eleTestsuite.attrib['errors'] = f"{errors}"
            eleTestsuite.attrib['skipped'] = f"{skips}"
            eleTestsuite.attrib['tests'] = f"{total}"

        result = ET.tostring(eleTestsuites)
        with open(filename, 'wb') as report:
            report.write(result)

    def json_report(self, filename, version="NA"):
        logger.info(f"Writing JSON report {filename}")
        report = {}
        report["environment"] = {"os": os.name,
                                 "zephyr_version": version,
                                 "toolchain": self.get_toolchain()
                                 }
        suites = []

        for instance in self.instances.values():
            suite = {}
            handler_log = os.path.join(instance.build_dir, "handler.log")
            build_log = os.path.join(instance.build_dir, "build.log")
            device_log = os.path.join(instance.build_dir, "device.log")

            handler_time = instance.metrics.get('handler_time', 0)
            ram_size = instance.metrics.get ("ram_size", 0)
            rom_size  = instance.metrics.get("rom_size",0)
            suite = {
                "name": instance.testsuite.name,
                "arch": instance.platform.arch,
                "platform": instance.platform.name,
            }
            if instance.run_id:
                suite['run_id'] = instance.run_id

            suite["runnable"] = False
            if instance.status != 'filtered':
                suite["runnable"] = instance.run

            if ram_size:
                suite["ram_size"] = ram_size
            if rom_size:
                suite["rom_size"] = rom_size

            if instance.status in ["error", "failed"]:
                suite['status'] = instance.status
                suite["reason"] = instance.reason
                # FIXME
                if os.path.exists(handler_log):
                    suite["log"] = self.process_log(handler_log)
                elif os.path.exists(device_log):
                    suite["log"] = self.process_log(device_log)
                else:
                    suite["log"] = self.process_log(build_log)
            elif instance.status == 'filtered':
                suite["status"] = "filtered"
                suite["reason"] = instance.reason
            elif instance.status == 'passed':
                suite["status"] = "passed"
            elif instance.status == 'skipped':
                suite["status"] = "skipped"
                suite["reason"] = instance.reason

            if instance.status is not None:
                suite["execution_time"] =  f"{float(handler_time):.2f}"

            testcases = []

            if len(instance.testcases) == 1:
                single_case_duration = f"{float(handler_time):.2f}"
            else:
                single_case_duration = 0

            for case in instance.testcases:
                # freeform was set when no sub testcases were parsed, however,
                # if we discover those at runtime, the fallback testcase wont be
                # needed anymore and can be removed from the output, it does
                # not have a status and would otherwise be reported as skipped.
                if case.freeform and case.status is None and len(instance.testcases) > 1:
                    continue
                testcase = {}
                testcase['identifier'] = case.name
                if instance.status:
                    if single_case_duration:
                        testcase['execution_time'] = single_case_duration
                    else:
                        testcase['execution_time'] = f"{float(case.duration):.2f}"

                if case.output != "":
                    testcase['log'] = case.output

                if case.status == "skipped":
                    if instance.status == "filtered":
                        testcase["status"] = "filtered"
                    else:
                        testcase["status"] = "skipped"
                        testcase["reason"] = case.reason or instance.reason
                else:
                    testcase["status"] = case.status
                    if case.reason:
                        testcase["reason"] = case.reason

                testcases.append(testcase)

            suite['testcases'] = testcases
            suites.append(suite)

        report["testsuites"] = suites
        with open(filename, "wt") as json_file:
            json.dump(report, json_file, indent=4, separators=(',',':'))

    def get_testsuite(self, identifier):
        results = []
        for _, ts in self.testsuites.items():
            for case in ts.testcases:
                if case == identifier:
                    results.append(ts)
        return results

    def verify_platforms_existence(self, platform_names_to_verify, log_info=""):
        """
        Verify if platform name (passed by --platform option, or in yaml file
        as platform_allow or integration_platforms options) is correct. If not -
        log and raise error.
        """
        for platform in platform_names_to_verify:
            if platform in self.platform_names:
                break
            else:
                logger.error(f"{log_info} - unrecognized platform - {platform}")
                sys.exit(2)

    def create_build_dir_links(self):
        """
        Iterate through all no-skipped instances in suite and create links
        for each one build directories. Those links will be passed in the next
        steps to the CMake command.
        """

        links_dir_name = "twister_links"  # folder for all links
        links_dir_path = os.path.join(self.outdir, links_dir_name)
        if not os.path.exists(links_dir_path):
            os.mkdir(links_dir_path)

        for instance in self.instances.values():
            if instance.status != "skipped":
                self._create_build_dir_link(links_dir_path, instance)

    def _create_build_dir_link(self, links_dir_path, instance):
        """
        Create build directory with original "long" path. Next take shorter
        path and link them with original path - create link. At the end
        replace build_dir to created link. This link will be passed to CMake
        command. This action helps to limit path length which can be
        significant during building by CMake on Windows OS.
        """

        os.makedirs(instance.build_dir, exist_ok=True)

        link_name = f"test_{self.link_dir_counter}"
        link_path = os.path.join(links_dir_path, link_name)

        if os.name == "nt":  # if OS is Windows
            command = ["mklink", "/J", f"{link_path}", f"{instance.build_dir}"]
            subprocess.call(command, shell=True)
        else:  # for Linux and MAC OS
            os.symlink(instance.build_dir, link_path)

        # Here original build directory is replaced with symbolic link. It will
        # be passed to CMake command
        instance.build_dir = link_path

        self.link_dir_counter += 1


class CoverageTool:
    """ Base class for every supported coverage tool
    """

    def __init__(self):
        self.gcov_tool = None
        self.base_dir = None

    @staticmethod
    def factory(tool):
        if tool == 'lcov':
            t =  Lcov()
        elif tool == 'gcovr':
            t =  Gcovr()
        else:
            logger.error("Unsupported coverage tool specified: {}".format(tool))
            return None

        logger.debug(f"Select {tool} as the coverage tool...")
        return t

    @staticmethod
    def retrieve_gcov_data(input_file):
        logger.debug("Working on %s" % input_file)
        extracted_coverage_info = {}
        capture_data = False
        capture_complete = False
        with open(input_file, 'r') as fp:
            for line in fp.readlines():
                if re.search("GCOV_COVERAGE_DUMP_START", line):
                    capture_data = True
                    continue
                if re.search("GCOV_COVERAGE_DUMP_END", line):
                    capture_complete = True
                    break
                # Loop until the coverage data is found.
                if not capture_data:
                    continue
                if line.startswith("*"):
                    sp = line.split("<")
                    if len(sp) > 1:
                        # Remove the leading delimiter "*"
                        file_name = sp[0][1:]
                        # Remove the trailing new line char
                        hex_dump = sp[1][:-1]
                    else:
                        continue
                else:
                    continue
                extracted_coverage_info.update({file_name: hex_dump})
        if not capture_data:
            capture_complete = True
        return {'complete': capture_complete, 'data': extracted_coverage_info}

    @staticmethod
    def create_gcda_files(extracted_coverage_info):
        logger.debug("Generating gcda files")
        for filename, hexdump_val in extracted_coverage_info.items():
            # if kobject_hash is given for coverage gcovr fails
            # hence skipping it problem only in gcovr v4.1
            if "kobject_hash" in filename:
                filename = (filename[:-4]) + "gcno"
                try:
                    os.remove(filename)
                except Exception:
                    pass
                continue

            with open(filename, 'wb') as fp:
                fp.write(bytes.fromhex(hexdump_val))

    def generate(self, outdir):
        for filename in glob.glob("%s/**/handler.log" % outdir, recursive=True):
            gcov_data = self.__class__.retrieve_gcov_data(filename)
            capture_complete = gcov_data['complete']
            extracted_coverage_info = gcov_data['data']
            if capture_complete:
                self.__class__.create_gcda_files(extracted_coverage_info)
                logger.debug("Gcov data captured: {}".format(filename))
            else:
                logger.error("Gcov data capture incomplete: {}".format(filename))

        with open(os.path.join(outdir, "coverage.log"), "a") as coveragelog:
            ret = self._generate(outdir, coveragelog)
            if ret == 0:
                logger.info("HTML report generated: {}".format(
                    os.path.join(outdir, "coverage", "index.html")))


class Lcov(CoverageTool):

    def __init__(self):
        super().__init__()
        self.ignores = []

    def add_ignore_file(self, pattern):
        self.ignores.append('*' + pattern + '*')

    def add_ignore_directory(self, pattern):
        self.ignores.append('*/' + pattern + '/*')

    def _generate(self, outdir, coveragelog):
        coveragefile = os.path.join(outdir, "coverage.info")
        ztestfile = os.path.join(outdir, "ztest.info")
        cmd = ["lcov", "--gcov-tool", self.gcov_tool,
                         "--capture", "--directory", outdir,
                         "--rc", "lcov_branch_coverage=1",
                         "--output-file", coveragefile]
        cmd_str = " ".join(cmd)
        logger.debug(f"Running {cmd_str}...")
        subprocess.call(cmd, stdout=coveragelog)
        # We want to remove tests/* and tests/ztest/test/* but save tests/ztest
        subprocess.call(["lcov", "--gcov-tool", self.gcov_tool, "--extract",
                         coveragefile,
                         os.path.join(self.base_dir, "tests", "ztest", "*"),
                         "--output-file", ztestfile,
                         "--rc", "lcov_branch_coverage=1"], stdout=coveragelog)

        if os.path.exists(ztestfile) and os.path.getsize(ztestfile) > 0:
            subprocess.call(["lcov", "--gcov-tool", self.gcov_tool, "--remove",
                             ztestfile,
                             os.path.join(self.base_dir, "tests/ztest/test/*"),
                             "--output-file", ztestfile,
                             "--rc", "lcov_branch_coverage=1"],
                            stdout=coveragelog)
            files = [coveragefile, ztestfile]
        else:
            files = [coveragefile]

        for i in self.ignores:
            subprocess.call(
                ["lcov", "--gcov-tool", self.gcov_tool, "--remove",
                 coveragefile, i, "--output-file",
                 coveragefile, "--rc", "lcov_branch_coverage=1"],
                stdout=coveragelog)

        # The --ignore-errors source option is added to avoid it exiting due to
        # samples/application_development/external_lib/
        return subprocess.call(["genhtml", "--legend", "--branch-coverage",
                                "--ignore-errors", "source",
                                "-output-directory",
                                os.path.join(outdir, "coverage")] + files,
                               stdout=coveragelog)


class Gcovr(CoverageTool):

    def __init__(self):
        super().__init__()
        self.ignores = []

    def add_ignore_file(self, pattern):
        self.ignores.append('.*' + pattern + '.*')

    def add_ignore_directory(self, pattern):
        self.ignores.append(".*/" + pattern + '/.*')

    @staticmethod
    def _interleave_list(prefix, list):
        tuple_list = [(prefix, item) for item in list]
        return [item for sublist in tuple_list for item in sublist]

    def _generate(self, outdir, coveragelog):
        coveragefile = os.path.join(outdir, "coverage.json")
        ztestfile = os.path.join(outdir, "ztest.json")

        excludes = Gcovr._interleave_list("-e", self.ignores)

        # We want to remove tests/* and tests/ztest/test/* but save tests/ztest
        cmd = ["gcovr", "-r", self.base_dir, "--gcov-executable",
               self.gcov_tool, "-e", "tests/*"] + excludes + ["--json", "-o",
               coveragefile, outdir]
        cmd_str = " ".join(cmd)
        logger.debug(f"Running {cmd_str}...")
        subprocess.call(cmd, stdout=coveragelog)

        subprocess.call(["gcovr", "-r", self.base_dir, "--gcov-executable",
                         self.gcov_tool, "-f", "tests/ztest", "-e",
                         "tests/ztest/test/*", "--json", "-o", ztestfile,
                         outdir], stdout=coveragelog)

        if os.path.exists(ztestfile) and os.path.getsize(ztestfile) > 0:
            files = [coveragefile, ztestfile]
        else:
            files = [coveragefile]

        subdir = os.path.join(outdir, "coverage")
        os.makedirs(subdir, exist_ok=True)

        tracefiles = self._interleave_list("--add-tracefile", files)

        return subprocess.call(["gcovr", "-r", self.base_dir, "--html",
                                "--html-details"] + tracefiles +
                               ["-o", os.path.join(subdir, "index.html")],
                               stdout=coveragelog)


class DUT(object):
    def __init__(self,
                 id=None,
                 serial=None,
                 serial_baud=None,
                 platform=None,
                 product=None,
                 serial_pty=None,
                 connected=False,
                 runner_params=None,
                 pre_script=None,
                 post_script=None,
                 post_flash_script=None,
                 runner=None):

        self.serial = serial
        self.baud = serial_baud or 115200
        self.platform = platform
        self.serial_pty = serial_pty
        self._counter = Value("i", 0)
        self._available = Value("i", 1)
        self.connected = connected
        self.pre_script = pre_script
        self.id = id
        self.product = product
        self.runner = runner
        self.runner_params = runner_params
        self.fixtures = []
        self.post_flash_script = post_flash_script
        self.post_script = post_script
        self.pre_script = pre_script
        self.probe_id = None
        self.notes = None
        self.lock = Lock()
        self.match = False


    @property
    def available(self):
        with self._available.get_lock():
            return self._available.value

    @available.setter
    def available(self, value):
        with self._available.get_lock():
            self._available.value = value

    @property
    def counter(self):
        with self._counter.get_lock():
            return self._counter.value

    @counter.setter
    def counter(self, value):
        with self._counter.get_lock():
            self._counter.value = value

    def to_dict(self):
        d = {}
        exclude = ['_available', '_counter', 'match']
        v = vars(self)
        for k in v.keys():
            if k not in exclude and v[k]:
                d[k] = v[k]
        return d


    def __repr__(self):
        return f"<{self.platform} ({self.product}) on {self.serial}>"

class HardwareMap:
    schema_path = os.path.join(ZEPHYR_BASE, "scripts", "schemas", "twister", "hwmap-schema.yaml")

    manufacturer = [
        'ARM',
        'SEGGER',
        'MBED',
        'STMicroelectronics',
        'Atmel Corp.',
        'Texas Instruments',
        'Silicon Labs',
        'NXP Semiconductors',
        'Microchip Technology Inc.',
        'FTDI',
        'Digilent'
    ]

    runner_mapping = {
        'pyocd': [
            'DAPLink CMSIS-DAP',
            'MBED CMSIS-DAP'
        ],
        'jlink': [
            'J-Link',
            'J-Link OB'
        ],
        'openocd': [
            'STM32 STLink', '^XDS110.*', 'STLINK-V3'
        ],
        'dediprog': [
            'TTL232R-3V3',
            'MCP2200 USB Serial Port Emulator'
        ]
    }

    def __init__(self):
        self.detected = []
        self.duts = []

    def add_device(self, serial, platform, pre_script, is_pty, baud=None):
        device = DUT(platform=platform, connected=True, pre_script=pre_script, serial_baud=baud)

        if is_pty:
            device.serial_pty = serial
        else:
            device.serial = serial

        self.duts.append(device)

    def load(self, map_file):
        hwm_schema = scl.yaml_load(self.schema_path)
        duts = scl.yaml_load_verify(map_file, hwm_schema)
        for dut in duts:
            pre_script = dut.get('pre_script')
            post_script = dut.get('post_script')
            post_flash_script = dut.get('post_flash_script')
            platform  = dut.get('platform')
            id = dut.get('id')
            runner = dut.get('runner')
            runner_params = dut.get('runner_params')
            serial_pty = dut.get('serial_pty')
            serial = dut.get('serial')
            baud = dut.get('baud', None)
            product = dut.get('product')
            fixtures = dut.get('fixtures', [])
            connected= dut.get('connected') and ((serial or serial_pty) is not None)
            new_dut = DUT(platform=platform,
                          product=product,
                          runner=runner,
                          runner_params=runner_params,
                          id=id,
                          serial_pty=serial_pty,
                          serial=serial,
                          serial_baud=baud,
                          connected=connected,
                          pre_script=pre_script,
                          post_script=post_script,
                          post_flash_script=post_flash_script)
            new_dut.fixtures = fixtures
            new_dut.counter = 0
            self.duts.append(new_dut)

    def scan(self, persistent=False):
        from serial.tools import list_ports

        if persistent and platform.system() == 'Linux':
            # On Linux, /dev/serial/by-id provides symlinks to
            # '/dev/ttyACMx' nodes using names which are unique as
            # long as manufacturers fill out USB metadata nicely.
            #
            # This creates a map from '/dev/ttyACMx' device nodes
            # to '/dev/serial/by-id/usb-...' symlinks. The symlinks
            # go into the hardware map because they stay the same
            # even when the user unplugs / replugs the device.
            #
            # Some inexpensive USB/serial adapters don't result
            # in unique names here, though, so use of this feature
            # requires explicitly setting persistent=True.
            by_id = Path('/dev/serial/by-id')
            def readlink(link):
                return str((by_id / link).resolve())

            persistent_map = {readlink(link): str(link)
                              for link in by_id.iterdir()}
        else:
            persistent_map = {}

        serial_devices = list_ports.comports()
        logger.info("Scanning connected hardware...")
        for d in serial_devices:
            if d.manufacturer in self.manufacturer:

                # TI XDS110 can have multiple serial devices for a single board
                # assume endpoint 0 is the serial, skip all others
                if d.manufacturer == 'Texas Instruments' and not d.location.endswith('0'):
                    continue
                s_dev = DUT(platform="unknown",
                                        id=d.serial_number,
                                        serial=persistent_map.get(d.device, d.device),
                                        product=d.product,
                                        runner='unknown',
                                        connected=True)

                for runner, _ in self.runner_mapping.items():
                    products = self.runner_mapping.get(runner)
                    if d.product in products:
                        s_dev.runner = runner
                        continue
                    # Try regex matching
                    for p in products:
                        if re.match(p, d.product):
                            s_dev.runner = runner

                s_dev.connected = True
                s_dev.lock = None
                self.detected.append(s_dev)
            else:
                logger.warning("Unsupported device (%s): %s" % (d.manufacturer, d))

    def save(self, hwm_file):
        # use existing map
        self.detected.sort(key=lambda x: x.serial or '')
        if os.path.exists(hwm_file):
            with open(hwm_file, 'r') as yaml_file:
                hwm = yaml.load(yaml_file, Loader=SafeLoader)
                if hwm:
                    hwm.sort(key=lambda x: x.get('id', ''))

                    # disconnect everything
                    for h in hwm:
                        h['connected'] = False
                        h['serial'] = None

                    for _detected in self.detected:
                        for h in hwm:
                            if _detected.id == h['id'] and _detected.product == h['product'] and not _detected.match:
                                h['connected'] = True
                                h['serial'] = _detected.serial
                                _detected.match = True

                new_duts = list(filter(lambda d: not d.match, self.detected))
                new = []
                for d in new_duts:
                    new.append(d.to_dict())

                if hwm:
                    hwm = hwm + new
                else:
                    hwm = new

            with open(hwm_file, 'w') as yaml_file:
                yaml.dump(hwm, yaml_file, Dumper=Dumper, default_flow_style=False)

            self.load(hwm_file)
            logger.info("Registered devices:")
            self.dump()

        else:
            # create new file
            dl = []
            for _connected in self.detected:
                platform  = _connected.platform
                id = _connected.id
                runner = _connected.runner
                serial = _connected.serial
                product = _connected.product
                d = {
                    'platform': platform,
                    'id': id,
                    'runner': runner,
                    'serial': serial,
                    'product': product,
                    'connected': _connected.connected
                }
                dl.append(d)
            with open(hwm_file, 'w') as yaml_file:
                yaml.dump(dl, yaml_file, Dumper=Dumper, default_flow_style=False)
            logger.info("Detected devices:")
            self.dump(detected=True)

    def dump(self, filtered=[], header=[], connected_only=False, detected=False):
        print("")
        table = []
        if detected:
            to_show = self.detected
        else:
            to_show = self.duts

        if not header:
            header = ["Platform", "ID", "Serial device"]
        for p in to_show:
            platform = p.platform
            connected = p.connected
            if filtered and platform not in filtered:
                continue

            if not connected_only or connected:
                table.append([platform, p.id, p.serial])

        print(tabulate(table, headers=header, tablefmt="github"))

def init(colorama_strip):
    colorama.init(strip=colorama_strip)
