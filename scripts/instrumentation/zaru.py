#!/usr/bin/env python3
#
# Copyright 2023 Linaro
# Copyright 2025 Antmicro
# Copyright 2025 Analog Devices
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import sys

try:
    import bt2
except ImportError:
    sys.exit(
        "Python3 babeltrace2 module is missing. Please install it.\n"
        "On Ubuntu, install with 'apt-get install python3-bt2\n"
        "For other systems, please consult the Installation page of the\n"
        "Babeltrace 2 Python bindings project:\n"
        "https://babeltrace.org/docs/v2.0/python/bt2/installation.html"
    )

import json
import os
import pathlib
import re
import shutil
import subprocess
import tempfile

import serial
from colorama import Fore, Style
from west.app.main import WestApp
from west.configuration import Configuration, config
from west.util import west_topdir

STATUS_REPLY_PATTERN = r"(0|1)\s(0|1)"


LISTSETS_REPLY_PATTERN = r"(trigger|stopper): (0x[0-9A-Fa-f]+)"


PATTERN = r"([0-9A-Fa-f]{8}).*(text).*([0-9A-Fa-f]{8})\s(.*)"


ELF = "zephyr/zephyr.elf"

CTF_METADATA = "ctf_metadata"


OBJDUMP_CMD = ["objdump", "-t"]


CPPFILT_CMD = ["c++filt"]


SERIAL = "/dev/ttyACM0"


NUM_PING_ATTEMPTS = 20


PERFETTO_FILENAME = "./perfetto.json"


def get_symbols_from_elf(elf_file, verbose=False):
    """Get symbols from ELF.

    Get symbols from a given 'elf_file' file and return them as a dict() indexed
    by the symbol addresses.
    """

    assert elf_file.exists(), f"File '{elf_file}' does not exist!"

    cmd = OBJDUMP_CMD + [elf_file]

    try:
        output = subprocess.check_output(cmd, text=True)
    except subprocess.CalledProcessError:
        cmd = " ".join(cmd)
        print(f"'{cmd}' failed execution. Check if it is properly installed.")
        sys.exit(2)
    except FileNotFoundError:
        print(f"Could not find executable '{OBJDUMP_CMD[0]}'. Please install it.")
        sys.exit(3)

    if verbose:
        print(f"Reading symbols from '{pathlib.Path(elf_file).resolve()}'.")

    lines = output.split("\n")
    r = re.compile(PATTERN)
    # dict: {addr: symbol}
    addr_to_symbol = {
        r.match(line).group(1): r.match(line).group(4)
        for line in lines
        if r.match(line) is not None
    }

    # '0' symbol is special and is mapped to address 0. It's used to disable
    # trigger and stopper.
    addr_to_symbol["0"] = "0"

    return addr_to_symbol


def generate_reverse_symbol_lookup(addr_to_symbol):
    """Generate a reverse symbol lookup dict.

    Given a dict of ELF symbols indexed by the symbol address, return the same
    dict but indexed by the symbols intead of by the symbol address, allowing
    a reverse lookup, i.e. look up for a symbol address given a symbol.
    """

    symbol_to_addr = {symbol: addr for addr, symbol in addr_to_symbol.items()}

    return symbol_to_addr


def get_zephyr_base(die_if_not_set=False):
    zephyr_base = os.environ.get("ZEPHYR_BASE", None)
    if zephyr_base is None and die_if_not_set:
        sys.exit("'ZEPHYR_BASE' env variable is not set!")

    return pathlib.Path(zephyr_base)


def get_zephyr_build_dir(args, die_if_not_set=False):
    zephyr_base = get_zephyr_base(die_if_not_set=True)
    sys.path.insert(0, str(zephyr_base / "scripts/west_commands"))
    try:
        from build_helpers import is_zephyr_build
        from run_common import get_build_dir
    finally:
        sys.path.pop(0)

    # WestApp class only populates the config attributes if run() method is
    # called. Since run() is used effectively to run commands -- which is not
    # the purpose here, a new derivated class which populates the config
    # attributes when initialized is defined below. Without config being
    # populated method get_build_dir() won't correctly find/guess the Zephyr
    # build dir and will simply return a default at best.
    class WestAppNoRun(WestApp):
        def __init__(self):
            super().__init__()

            self.config = Configuration(topdir=west_topdir())
            self.config._copy_to_configparser(config)

    # Init config attributes, so get_build_dir() works fine.
    # pylint: disable=unused-variable
    west_app = WestAppNoRun()  # noqa: F841

    # Although get_build_dir() checks args to see if build_dir is provided,
    # returning it if provided, it neither checks if the build_dir exists nor
    # checks if it is a valid Zephyr build dir, hence the checks below.
    if args.build_dir:
        if not os.path.exists(args.build_dir):
            sys.exit(f"Build dir {args.build_dir} doesn't exist!")
        elif not is_zephyr_build(args.build_dir):
            sys.exit(f"{args.build_dir} is not a Zephyr build dir!")
        else:
            return pathlib.Path(args.build_dir)

    # If build_dir is not given by the user, try to guess it.
    build_dir = get_build_dir(args, die_if_none=False)

    if build_dir is None and die_if_not_set:
        sys.exit("Could not determine build dir. Please provide one via '--build-dir'.")

    return pathlib.Path(build_dir)


def get_elf_file(args, verbose, exit_on_failure=True):
    build_dir = get_zephyr_build_dir(args, die_if_not_set=True)
    elf_file = build_dir / ELF

    if not os.path.exists(elf_file):
        if exit_on_failure:
            sys.exit(f"Could not find ELF file '{elf_file}'!")
        else:
            elf_file = None

    if verbose:
        print(f"Using ELF file: {elf_file}")

    return elf_file


def get_ctf_metadata_file(args, verbose, exit_on_failure=True):
    """Get CTF metadata.

    Get CTF Common Trace Format) metadata full path.
    """
    build_dir = get_zephyr_build_dir(args, die_if_not_set=True)
    ctf_metadata_file = build_dir / CTF_METADATA

    if not os.path.exists(ctf_metadata_file):
        if exit_on_failure:
            sys.exit(f"Could not find ctf_metadata file '{ctf_metadata_file}'!")
        else:
            ctf_metadata_file = None

    if verbose:
        print(f"Using ctf_metadata file: {ctf_metadata_file}")

    return ctf_metadata_file


def ping(port):
    """Ping target.

    Ping target connected to port 'port'. If target responds (by sending back a
    'pong' string, then this function returns 'True', otherwise it returns
    false.
    """

    # Clean up possible app. output before getting command response.
    _ = port.readlines()

    port.write(b'ping\r')

    # Kludge for annoying incompatibility change in the 'serial' module.
    if float(serial.__version__) < 3.5:
        r = port.read_until(terminator=b'pong\n')
    else:
        r = port.read_until(expected=b'pong\n')

    return b'pong' in r


def connect_to_target(port, verbose=False):
    """Connect to target.

    Connect to target using serial device 'port'. A ping command is sent
    to the target to check if it's alive and responding. If target responds
    'True' is return, otherwise 'False' is returned.
    """

    try:
        sport = serial.serial_for_url(port, baudrate=115200, timeout=0.2, write_timeout=1)
        if verbose:
            print(f"Using '{port}' to connect.")

        if ping(sport):
            if verbose:
                print("Connected to target.")
            return sport
        else:
            print(
                "No response from target. Is the correct FW flashed? Also, check if port is "
                "open by another program."
            )
            sys.exit(1)
    except serial.SerialException as e:
        # Annoying that error formats for serial and socket are different.
        if e.args[0] == 2:
            # Serial error
            print(e.args[1].capitalize())
        else:
            # Socket error
            print(e)
        sys.exit(1)


def reboot_target(port, verbose=False):
    """Reboot target.

    Reboot target connected to the port 'port'. After the reboot the target is
    checked to see if it's still alive using the 'ping' command. If it's ok
    after the reboot 'True' is returned, otherwise 'False' is returned.
    """

    port.write(b'reboot\r')

    if verbose:
        print("Rebooting target...", end="", flush=True)

    num_attempts = 0
    while num_attempts < NUM_PING_ATTEMPTS:
        if ping(port):
            break
        num_attempts = num_attempts + 1
        if verbose:
            print(".", end="", flush=True)

    if num_attempts < NUM_PING_ATTEMPTS:
        if verbose:
            print(" done!")

        return True

    else:
        if verbose:
            # Newline
            print("")

        return False


def get_target_status(port, verbose=False):
    """Get trace and profile status.

    Returns trace and profile status in a dict() from the target connected via
    port 'port'. If it's not possible to obtain the status 'None' is returned
    instead.
    """

    # Clean up possible app. output before getting command response.
    _ = port.readlines()

    port.write(b'status\r')

    inb = port.readline()

    regex = re.compile(STATUS_REPLY_PATTERN)
    r = regex.match(inb.decode("ascii"))

    if r is None:
        return r

    trace_enabled = r.group(1) == "1"
    profile_enabled = r.group(2) == "1"

    return {"trace": trace_enabled, "profile": profile_enabled}


def get_trigger_stopper_addr(port):
    """Get trigger and stopper addresses set in the target.

    This function get the trigger and stopper address from the target connected
    via port 'port' and returns a dict() with keys 'trigger' and 'stopper'. When
    the address is not set (mechanism is disabled) 'None' is returned instead of
    the addresses.
    """

    port.write(b'listsets\r')

    try:
        trigger_reply = port.readline()
        stopper_reply = port.readline()
    except serial.serialutil.SerialException:
        print(
            "Device reports readiness to read but returned no data (device disconnected or "
            "multiple access on port?)"
        )
        sys.exit(1)

    # If trigger or stopper is not set (disabled), then only a message saying it
    # is displayed by the target, hence LISTSETS_REPLY_PATTERN won't match and
    # trigger_addr and/or stopper_addr will then be 'None'.
    r = re.compile(LISTSETS_REPLY_PATTERN)
    trigger_addr = r.match(trigger_reply.decode("ascii"))
    stopper_addr = r.match(stopper_reply.decode("ascii"))

    # Format address so it's possible to use it as keys for the symbol lookup.
    # If trigger_addr or stopper_addr is 'None', then skip format.

    if trigger_addr:
        addr = int(trigger_addr.group(2), base=16)
        trigger_addr = f'{addr:08x}'

    if stopper_addr:
        addr = int(stopper_addr.group(2), base=16)
        stopper_addr = f'{addr:08x}'

    return {"trigger": trigger_addr, "stopper": stopper_addr}


def set_trigger_addr(port, addr, verbose=False):
    """Set the function address to start tracing.

    This function sets the 'addr' as the address of the function which, when
    is entered, will start the tracing in the target connected bia port 'port.
    Once it's set in the target, this function reads the address back to confirm
    it is set correctly. If it's correct, 'True' is returned, otherwise 'False'
    is returned. If address '0' is given it disables the trigger.
    """

    port.write(b'trigger ' + b'0x' + bytes(addr, "ascii") + b'\r')

    # Check if trigger was set correctly.
    addr_set = get_trigger_stopper_addr(port)["trigger"]

    # Special case, when disabling trigger: if given 'addr' is 0, then it's
    # expected that 'addr_set' is 'None', because trigger is disabled.
    if addr == "0":
        return addr_set is None

    return addr_set == addr


def set_stopper_addr(port, addr):
    """Set the function address to stop tracing.

    This function sets the 'addr' as the address of the function which, when it
    exits, will stop the tracing in the target connected via port 'port'. Once
    it's set in the target, this function reads the address back to confirm it
    is set correctly. If it's correct, 'True' is returned, otherwise 'False' is
    returned. If address '0' is given it disables the stopper.
    """

    port.write(b'stopper ' + b'0x' + bytes(addr, "ascii") + b'\r')

    # Check if stopper was set correctly.
    addr_set = get_trigger_stopper_addr(port)["stopper"]

    # Special case, when disabling stopper: if given 'addr' is 0, then it's
    # expected that 'addr_set' is 'None', because stopper is disabled.
    if addr == "0":
        return addr_set is None

    return addr_set == addr


def get_stream(port):
    """Get a binary stream from target.

    This function gets a binary stream extracted from target connected via port
    'port' and returns the stream as 'bytes' object.
    """

    ll = b""
    while True:
        si = port.read(1)
        ll = ll + si
        if b"-*-#" in ll:  # Initiator
            ll = b""  # zero input buffer
            while True:
                si = port.read(1)
                ll = ll + si
                if b"-*-!" in ll:  # Terminator
                    ll = ll[:-4]  # trim terminator
                    return ll


def get_and_print_trace(args, port, elf, demangle, annotate_ret=False, verbose=False):
    """Get traces from target and print them.

    This function uses 'port' to get the binary stream from target and 'elf'
    file to resolve the symbols and then prints the traces in it. The binary
    stream is interpreted according to the CTF (Common Trace Format) file
    'metadata', using library babeltrece2.
    """

    port.write(b"dump_trace\r")

    # babeltrace2 and CTF 1.8 specification is a tad odd in the sense that
    # TraceCollectionMessageIterator() only allows a directory to be specified, which must contain
    # a 'data' (binary) file and 'metadata' file written in the TSDL, hence it's not possible to
    # specify an alternative path for 'data' or 'metadata' files. Thus here a temporary dir. is
    # created and a copy of the 'metadata' is copied to it together with the binary stream extracted
    # from the target, which is saved as 'data' file. The tempory dir. is then passed the babeltrace
    # methods.
    with tempfile.TemporaryDirectory() as tmpdir:
        if verbose:
            print("Temporary dir:", tmpdir)

        # Copy binary stream to temporary dir.
        data_file = tmpdir + "/data"
        with open(data_file, "wb") as fd:
            ll = get_stream(port)
            fd.write(ll)

        # Copy CTF metadata to temporary dir.
        metadata_file = get_ctf_metadata_file(args, args.verbose)
        shutil.copy(metadata_file, tmpdir + "/metadata")

        msg_it = bt2.TraceCollectionMessageIterator(tmpdir)

        symbols = get_symbols_from_elf(elf, verbose)

        # Generator for getting an event with symbols resolved.
        # The event returned is a dict().
        def get_trace_event_generator():
            for msg in msg_it:
                if isinstance(msg, bt2._EventMessageConst):
                    event = msg.event

                    # Entry / exit events (w/ or wo/ context) are converted to
                    # 'entry' and 'exit' types just to simplify the matching
                    # code using them -- match against a shorter string.  It can
                    # be enhanced if necessary. Currently just handle entry /
                    # exit with context and sched switch in/out events.
                    if "entry" in event.name:
                        event_type = "entry"
                    elif "exit" in event.name:
                        event_type = "exit"
                    elif "switched_in" in event.name:
                        event_type = "switched_in"
                    elif "switched_out" in event.name:
                        event_type = "switched_out"
                    else:
                        continue

                    # Resolve callee symbol.
                    callee = event.payload_field.get("callee").real
                    callee = f'{callee:08x}'  # Format before lookup
                    callee_symbol = symbols.get(callee)

                    if callee_symbol is None:
                        print(
                            Fore.RED + f"Symbol address {callee} could not be resolved! "
                            "Are you sure FW flashed matches provided zephyr.elf in build dir?\n"
                            "Tracing will be aborted because it's unreliable when symbols can't be "
                            "properly resolved."
                        )

                        sys.exit(1)

                    thread_id = event.payload_field.get("thread_id")
                    # When tracing non-application code usually there isn't
                    # a thread ID associated to the context, so in this case
                    # change thread ID to "none-thread".
                    if thread_id == 0:
                        thread_id = "none-thread"
                    else:
                        thread_id = hex(thread_id)

                    cpu = event.payload_field.get("cpu")
                    mode = event.payload_field.get("mode")
                    timestamp = event.payload_field.get("timestamp").real
                    thread_name = event.payload_field.get("thread_name")

                    e = dict()
                    e["type"] = str(event_type)
                    e["func"] = str(callee_symbol)
                    e["thread_id"] = str(thread_id)
                    e["cpu"] = str(cpu)
                    e["mode"] = str(mode)
                    e["timestamp"] = str(timestamp)
                    e["thread_name"] = str(thread_name)

                    yield e  # event

        ge = get_trace_event_generator()

        line_buffer_first_half = []  # thread_id, CPU, mode, and timestamp (ts)
        line_buffer_second_half = []  # functions
        per_thread_func_nest_depth = {}
        func_nest_depth_step = 2
        current_event = next(ge, None)
        while current_event:
            cur_type, cur_func, cur_thread_id, cur_cpu, cur_mode, cur_ts, cur_tn = (
                current_event.values()
            )

            if demangle:
                cmd = CPPFILT_CMD + [cur_func]

                try:
                    func_demangled = subprocess.check_output(cmd, text=True).strip()
                except subprocess.CalledProcessError:
                    cmd = " ".join(cmd)
                    print(f"'{cmd}' failed execution. Check if it is properly installed.")
                    sys.exit(2)
                except FileNotFoundError:
                    print(f"Could not find executable '{CPPFILT_CMD[0]}'. Please install it.")
                    sys.exit(3)

                cur_func = func_demangled

            line = (
                cur_tn.rjust(20)
                + cur_thread_id.rjust(14)
                + " "
                + cur_cpu.rjust(3)
                + ") "
                + cur_mode.rjust(4)
                + " | "
                + cur_ts.rjust(12)
                + " ns |"
            )
            line_buffer_first_half.append(line)

            # Keep track of nesting depth per thread, since thread stacks
            # are independent.
            if cur_tn not in per_thread_func_nest_depth:
                # Init function nest depth for each new found thread. It's used
                # for function indentation, which is different for each thread.
                per_thread_func_nest_depth[cur_tn] = 0

            next_event = next(ge, None)
            if next_event:
                next_type, next_func, *_ = next_event.values()

                if cur_type == "entry" and next_type == "exit" and cur_func == next_func:
                    # Same function, so use compact print.
                    line = cur_func + "();"
                    thread = cur_tn
                    per_thread_line = {
                        "thread": thread,
                        "line": line.rjust(per_thread_func_nest_depth[cur_tn] + len(line)),
                    }
                    line_buffer_second_half.append(per_thread_line)

                    # Fetch next event.
                    current_event = next(ge, None)

                    continue

            if cur_type == "entry":
                line = cur_func + "() {"
                per_thread_line = {
                    "thread": cur_tn,
                    "line": line.rjust(per_thread_func_nest_depth[cur_tn] + len(line)),
                }
                line_buffer_second_half.append(per_thread_line)

                # Advance nesting.
                per_thread_func_nest_depth[cur_tn] = (
                    per_thread_func_nest_depth[cur_tn] + func_nest_depth_step
                )

            elif cur_type == "exit":
                # Retreat nesting.
                per_thread_func_nest_depth[cur_tn] = (
                    per_thread_func_nest_depth[cur_tn] - func_nest_depth_step
                )

                if per_thread_func_nest_depth[cur_tn] >= 0:
                    line = "};"
                    if annotate_ret:
                        line = (
                            line
                            + Fore.WHITE
                            + Style.DIM
                            + f"   /* {cur_func} */"
                            + Fore.RESET
                            + Style.RESET_ALL
                        )
                    per_thread_line = {
                        "thread": cur_tn,
                        "line": line.rjust(per_thread_func_nest_depth[cur_tn] + len(line)),
                    }

                else:
                    # Found an orphaned exit, so adjust indentation for all the previous lines to
                    # the correct nesting depth, i.e. justify lines at right by
                    # 'func_nest_depth_step'. Only the lines of current thread(cur_tn) are
                    # justified, since each thread has a different stack, the nesting depth is kept
                    # per thread.
                    line_buffer_second_half = [
                        {
                            "thread": lines["thread"],
                            "line": lines["line"].rjust(func_nest_depth_step + len(lines["line"])),
                        }
                        if lines["thread"] == cur_tn
                        else lines
                        for lines in line_buffer_second_half
                    ]

                    # Then inform the orphaned exit.
                    per_thread_line = {
                        "thread": cur_tn,
                        "line": Fore.RED + f"}}; --> orphaned '{cur_func}'" + Fore.RESET,
                    }

                    # Reset function nest depth after justification of all previous lines.
                    per_thread_func_nest_depth[cur_tn] = 0

                # Save new line to the buffer.
                line_buffer_second_half.append(per_thread_line)

            elif cur_type == "switched_in":
                # Switch events are not adjusted for indentation, hence the
                # 'thread' key is not 'cur_tn' but an unlikely name of thread
                # (00000), so list comprehension above that justifies the
                # previous lines never matches this thread name.
                line = (
                    Fore.LIGHTYELLOW_EX
                    + f"/* <-- Scheduler switched IN thread '{cur_tn}' */"
                    + Fore.RESET
                    + Style.RESET_ALL
                )
                line_buffer_second_half.append({"thread": "00000", "line": line})
            else:  # switched_out
                line = (
                    Fore.LIGHTYELLOW_EX
                    + f"/* --> Scheduler switched OUT from thread '{cur_tn}' */"
                    + Fore.RESET
                    + Style.RESET_ALL
                )
                line_buffer_second_half.append({"thread": "00000", "line": line})

            current_event = next_event

        # Join both halves into a single line (trace) and print it. Two halves
        # are used because the second half needs to be adjusted for the orphaned
        # function exit cases, whilst the first associated half is not adjusted.

        # Remove "thread" key/value from the second half keeping just the lines
        # before joining it with first half.
        line_buffer_second_half = [lines["line"] for lines in line_buffer_second_half]
        traces = [
            " ".join(halves)
            for halves in tuple(zip(line_buffer_first_half, line_buffer_second_half, strict=False))
        ]

        # Print header
        if traces:
            print(
                "Thread Name".rjust(20),
                "Thread ID".rjust(14),
                "",
                "CPU".rjust(3),
                "",
                "Mode".rjust(4),
                "",
                "Timestamp".rjust(12),
                "",
                "Function(s)".rjust(19),
            )
            print("    ".ljust(100, "-"))

        # Print traces
        for trace in traces:
            print(trace)

        return len(traces)


def export_to_perfetto(args, port, elf, output_filename, demangle, verbose=False):
    """Get traces from target and save them in Tracer Event Format for Perfetto ingestion.

    This function uses 'port' to get the binary stream from target and 'elf'
    file to resolve the symbols and then prints the traces in it. The binary
    stream is interpreted according to the CTF (Common Trace Format) file
    'metadata', using library babeltrece2. Data is then saved into a JSON file,
    in Trace Event Format (https://tinyurl.com/45bb69s9).
    """

    port.write(b"dump_trace\r")

    # babeltrace2 and CTF 1.8 specification is a tad odd in the sense that
    # TraceCollectionMessageIterator() only allows a directory to be specified, which must contain a
    # 'data' (binary) file and 'metadata' file written in the TSDL, hence it's not possible to
    # specify an alternative path for 'data' or 'metadata' files. Thus here a temporary dir is
    # created and a copy of the 'metadata' is copied to it together with the binary stream extracted
    # from the target, which is saved as 'data' file. The tempory dir is then passed to the
    # babeltrace methods.
    with tempfile.TemporaryDirectory() as tmpdir:
        if verbose:
            print("Temporary dir:", tmpdir)

        # Copy binary stream to temporary dir.
        data_file = tmpdir + "/data"
        with open(data_file, "wb") as fd:
            ll = get_stream(port)
            fd.write(ll)

        # Copy CTF metadata to temporary dir.
        metadata_file = get_ctf_metadata_file(args, args.verbose)
        shutil.copy(metadata_file, tmpdir + "/metadata")

        msg_it = bt2.TraceCollectionMessageIterator(tmpdir)

        symbols = get_symbols_from_elf(elf, verbose)

        # Generator for getting an event with symbols resolved.
        # The event returned is a dict().
        def get_trace_event_generator():
            for msg in msg_it:
                if isinstance(msg, bt2._EventMessageConst):
                    event = msg.event

                    # Entry / exit events (w/ or wo/ context) are converted to
                    # 'entry' and 'exit' types just to simplify the matching
                    # code using them -- match against a shorter string.  It can
                    # be enhanced if necessary. Currently just handle entry /
                    # exit with context and sched switch in/out events.
                    if "entry" in event.name:
                        event_type = "entry"
                    elif "exit" in event.name:
                        event_type = "exit"
                    elif "switched_in" in event.name:
                        event_type = "switched_in"
                    elif "switched_out" in event.name:
                        event_type = "switched_out"
                    else:
                        continue

                    # Resolve callee symbol.
                    callee = event.payload_field.get("callee").real
                    callee = f'{callee:08x}'  # Format before lookup
                    callee_symbol = symbols.get(callee)

                    assert callee_symbol is not None, (
                        f"Symbol address {callee} could not be resolved! Are you sure "
                        "FW flashed matches provided ELF file in build dir?"
                    )

                    thread_id = event.payload_field.get("thread_id")
                    # When tracing non-application code usually there isn't
                    # a thread ID associated to the context, so in this case
                    # change thread ID to "none-thread".
                    if thread_id == 0:
                        thread_id = "none-thread"
                    else:
                        thread_id = hex(thread_id)

                    cpu = event.payload_field.get("cpu")
                    mode = event.payload_field.get("mode")
                    timestamp = event.payload_field.get("timestamp").real
                    thread_name = event.payload_field.get("thread_name")

                    e = dict()
                    e["type"] = str(event_type)
                    e["func"] = str(callee_symbol)
                    e["thread_id"] = str(thread_id)
                    e["cpu"] = str(cpu)
                    e["mode"] = str(mode)
                    e["timestamp"] = str(timestamp)
                    e["thread_name"] = str(thread_name)

                    yield e  # event

        ge = get_trace_event_generator()

        trace_events = []
        system_trace_events = []
        named_thread_list = []
        for trace in ge:
            event_type, func, tid, cpu, _, ts, tn = trace.values()

            # Use 4 LSB in tid (address) as the final thread ID just to ease
            # displaying it in Perfetto. Hardly there will be a collision.
            tid = int(tid, 0)
            tid = tid & 0xFFFF

            # Set phase type according with the Event
            if event_type == "entry":
                ph = "B"  # Event Begin
            elif event_type == "exit":
                ph = "E"  # Event End
            elif event_type == "switched_out":
                # Mimic a ftrace sched_switch event out of a pair of switched
                # out / switched in events in Zephyr.

                # Trace Event Format time unit is different from ftrace time
                # unit in sched switch event, so 1 time unit in Trace Event
                # Format is actually 1/10^6 time units in ftrace event. Since,
                # to correctly display events in Perfetto, it's necessary to
                # show the same time scale for ftrace events and other events,
                # convert the ftrace time accordingly.
                ftrace_ts = int(ts) / (10**6)
                sched_ts = f"{ftrace_ts:.6f}"

                sched_cpu = f"{int(cpu):03d}"

                # See ftrace https://github.com/google/perfetto/blob/master/docs/data-sources/cpu-scheduling.md
                # doc for details on the thread states.
                system_trace_switched_out_event = (
                    f"{tn}-{tid} ({tid}) [{sched_cpu}] .... {sched_ts}: sched_switch: "
                    f"prev_comm={tn} prev_pid={tid} prev_prio=0 prev_state=T ==> "
                )

                next_trace = next(ge, None)
                if not next_trace:
                    print(
                        Fore.RED + "Warning: A switched in event is expected after a switched out! "
                        "Skipping sched switch event." + Fore.RESET
                    )
                    continue

                next_event_type, _, next_tid, _, _, next_ts, next_tn = next_trace.values()
                assert next_event_type == "switched_in", (
                    "A switched in event is expected after a switched out!"
                )

                # See comment above on 'tid'.
                next_tid = int(next_tid, 0)
                next_tid = next_tid & 0xFFFF

                ftrace_ts = int(next_ts) / (10**6)

                system_trace_switched_in_event = (
                    f"next_comm={next_tn} next_pid={next_tid} next_prio=0\n"
                )

                system_trace_switch_event = (
                    system_trace_switched_out_event + system_trace_switched_in_event
                )

                if verbose:
                    # Remove \n at the end of line just for sake of showing it
                    # better in log.
                    print(Fore.YELLOW + system_trace_switch_event.strip() + Fore.RESET)

                system_trace_events.append(system_trace_switch_event)

            else:  # Ignore other type of traces.
                continue

            # Check if it's necessary to name a thread/process in the Event
            # Trace Format. Once a new thread is found it's named and included
            # to the list of named threads, threads need to be named only once.
            # N.B.: Zephyr has no notion of process as it's found in Linux, so
            # consider pid == tid.
            if tn not in named_thread_list:
                named_thread_list.append(tn)
                if verbose:
                    print(f"Found thread '{tn}'.")

                trace_event = {
                    "args": {"name": tn},
                    "cat": "__metadata",
                    "name": "thread_name",
                    "ph": "M",
                    "pid": tid,
                    "tid": tid,
                    "ts": 0,
                }
                trace_events.append(trace_event)

            if demangle:
                cmd = CPPFILT_CMD + [func]

                try:
                    func_demangled = subprocess.check_output(cmd, text=True).strip()
                except subprocess.CalledProcessError:
                    cmd = " ".join(cmd)
                    print(f"'{cmd}' failed execution. Check if it is properly installed.")
                    sys.exit(2)
                except FileNotFoundError:
                    print(f"Could not find executable '{CPPFILT_CMD[0]}'. Please install it.")
                    sys.exit(3)

                func = func_demangled

            trace_event = {"ts": ts, "pid": tid, "tid": tid, "ph": ph, "name": func}
            trace_events.append(trace_event)

        if verbose:
            print(
                f"Found {len(trace_events)} event(s), {len(named_thread_list)} thread(s), "
                f"{len(system_trace_events)} context switch(es)."
            )

        # Write Event Trace Format data to JSON file.

        # 'systemTraceEvents' must be prefixed with '# tracer: nop\n' string.
        system_trace_events = " ".join(system_trace_events)
        system_trace_events_w_prefix = "# tracer: nop\n " + system_trace_events

        output = {"traceEvents": trace_events, "systemTraceEvents": system_trace_events_w_prefix}
        with open(output_filename, "w") as fd:
            json.dump(output, fd)

        if verbose:
            st = os.stat(output_filename)
            print(f"{st.st_size} byte(s) written to '{output_filename}'.")

        return len(trace_events) + len(system_trace_events)


def get_and_print_profile(args, port, elf, n, verbose=False):
    """Get profile info from target and print it.

    This function uses 'port' to get the binary stream from target and 'elf'
    file to resolve the symbols and then prints the profile info in it. The
    binary stream is interpreted according to the CTF (Common Trace Format) file
    'metadata', using library babeltrece2.
    """

    port.write(b'dump_profile\r')

    # See comment above in get_and_print_trace() about CTF and babeltrace2
    # caveats.
    with tempfile.TemporaryDirectory() as tmpdir:
        if verbose:
            print("Temporary dir:", tmpdir)

        # Copy binary stream to temporary dir.
        data_file = tmpdir + "/data"
        with open(data_file, "wb") as fd:
            ll = get_stream(port)
            fd.write(ll)

        # Copy CTF metadata to temporary dir.
        metadata_file = get_ctf_metadata_file(args, args.verbose)
        shutil.copy(metadata_file, tmpdir + "/metadata")

        msg_it = bt2.TraceCollectionMessageIterator(tmpdir)

        symbols = get_symbols_from_elf(elf, verbose)

        profiles = []
        acc_delta_t = 0
        for msg in msg_it:
            if isinstance(msg, bt2._EventMessageConst):
                event = msg.event

                # Profile events have always ID = 2. This value is defined first
                # by enum instr_event_types, in instrumentation.h, then it is
                # defined accordingly in ctf/metadata.
                if event.id == 2:
                    callee = event.payload_field.get("callee").real
                    delta_t = event.payload_field.get("delta_t").real

                    profiles.append((callee, delta_t))
                    acc_delta_t = acc_delta_t + delta_t

        # Sort by delta_t
        profiles.sort(key=lambda t: t[1], reverse=True)

        if n > 0:
            N = n
        else:
            N = len(profiles)

        for i, (callee, delta_t) in enumerate(profiles):
            if i == N:
                break
            callee = f'{callee:08x}'
            callee_symbol = symbols.get(callee)

            if callee_symbol is None:
                print(
                    Fore.RED + f"Can't resolve address {callee}, skipping it..."
                    "(Are you sure the zephyr.elf is the FW flashed to the target?)"
                )
                continue

            percent_delta_t = (delta_t / acc_delta_t) * 100
            # Threshold set to greater than 22 % to show in red color.
            if percent_delta_t > 22:
                color = Fore.LIGHTGREEN_EX
            else:
                color = Fore.GREEN

            print(
                color + (f'{percent_delta_t:.2f}' + "%").rjust(6),
                callee,
                callee_symbol.ljust(20),
                Fore.WHITE,
            )

        return len(profiles)


def reboot(args):
    sport = connect_to_target(args.serial, args.verbose)
    if not reboot_target(sport, args.verbose):
        print("Reboot failed! Check target.")
        sys.exit(2)


def status(args):
    sport = connect_to_target(args.serial, args.verbose)

    status = get_target_status(sport, args.verbose)

    if not status:
        print("Failed while trying to get status! Check target.")
        sys.exit(2)

    trace_status = "supported" if status["trace"] else "not supported"
    profile_status = "supported" if status["profile"] else "not supported"

    print(f'Trace {trace_status}.')
    print(f'Profile {profile_status}.')


def trace(args):
    sport = connect_to_target(args.serial, args.verbose)

    status = get_target_status(sport, args.verbose)
    if not status['trace']:
        print(Fore.YELLOW + "Trace is not supported. Please enable it via 'menuconfig'.")
        sys.exit(1)

    if args.list:
        elf_file = get_elf_file(args, args.verbose)
        symbols = get_symbols_from_elf(elf_file, args.verbose)
        addr = get_trigger_stopper_addr(sport)

        if addr["trigger"]:
            func_name = symbols.get(addr["trigger"], None)
            if func_name:
                print(f"Trigger is set to {func_name}() entry.")
            else:
                print(
                    f"Trigger is set to unresolved address {addr['trigger']} entry. "
                    f"Is it a stale address that needs to be reset?"
                )
        else:
            print("Trigger is not set.")

        if addr["stopper"]:
            func_name = symbols.get(addr["stopper"], None)
            if func_name:
                print(f"Stopper is set to {func_name}() exit.")
            else:
                print(
                    f"Stopper is set to unresolved address {addr['trigger']} exit. "
                    f"Is it a stale address that needs to be reset?"
                )
        else:
            print("Stopper is not set.")

        sys.exit(0)

    if args.couple:
        args.trigger = args.couple
        args.stopper = args.couple

    if args.trigger or args.stopper:
        elf_file = get_elf_file(args, args.verbose)
        addr_to_symbol = get_symbols_from_elf(elf_file, args.verbose)
        symbol_to_addr = generate_reverse_symbol_lookup(addr_to_symbol)

        if args.trigger:
            if args.trigger not in symbol_to_addr:
                print(f"Could not find symbol '{args.trigger}' in ELF file '{elf_file.resolve()}'.")
                sys.exit(2)
            else:
                address = symbol_to_addr[args.trigger]
                if not set_trigger_addr(sport, address):
                    print("Failed to set new trigger address! Check target.")
                    sys.exit(2)
                else:
                    if args.trigger == "0":
                        print("Trigger disabled.")
                    else:
                        print(
                            f"New trigger set at entry of '{args.trigger}' function, "
                            f"at address 0x{address}."
                        )

        if args.stopper:
            if args.stopper not in symbol_to_addr:
                print(f"Could not find symbol '{args.stopper}' in ELF file '{elf_file.resolve()}'.")
                sys.exit(2)
            else:
                address = symbol_to_addr[args.stopper]
                if not set_stopper_addr(sport, address):
                    print("Failed to set new stopper address! Check target.")
                    sys.exit(2)
                else:
                    if args.stopper == "0":  # "0" is a special name/address
                        print("Stopper disabled.")
                    else:
                        print(
                            f"New stopper set at exit of '{args.stopper}' function, "
                            f"at address 0x{address}."
                        )

        sys.exit(0)

    if args.reboot and not reboot_target(sport, args.verbose):
        print("Failed to reboot target before tracing! Check target.")
        sys.exit(1)

    elf_file = get_elf_file(args, args.verbose)
    if args.perfetto:
        num_traces = export_to_perfetto(
            args, sport, elf_file, args.output, args.demangle, args.verbose
        )
    else:
        num_traces = get_and_print_trace(
            args, sport, elf_file, args.demangle, args.annotation, args.verbose
        )

    if num_traces == 0:
        print_message_on_empty_buffer("trace")


def profile(args):
    sport = connect_to_target(args.serial, args.verbose)

    status = get_target_status(sport, args.verbose)
    if not status['profile']:
        print(Fore.YELLOW + "Profile is not supported. Please enable it via 'menuconfig'.")
        sys.exit(1)

    if args.reboot and not reboot_target(sport, args.verbose):
        print("Failed to reboot target before tracing! Check target.")
        sys.exit(1)

    elf_file = get_elf_file(args, args.verbose)
    num_profiles = get_and_print_profile(args, sport, elf_file, args.n, args.verbose)
    if num_profiles == 0:
        print_message_on_empty_buffer("profile")


def print_message_on_empty_buffer(command):
    print(Fore.YELLOW)

    print(
        f"{command.title()} buffer is empty. Please check the trigger "
        f"function set using `zaru trace --list`.\nAn empty {command} buffer usually "
        "means the trigger function was not called during execution,\n"
        "which might happen because the function was really not executed or "
        "is set to a function\nthat is executed too early in the boot, "
        "like z_early_memset(), thus when instrumentation\nis still disabled."
    )
    print(Fore.WHITE)


if __name__ == "__main__":
    # pylint: disable=argument-parser-with-abbreviations
    parser = argparse.ArgumentParser(
        description="Zephyr compiler-managed system profiling and tracing tool."
    )
    parser.add_argument(
        "--serial",
        default=SERIAL,
        help=f"defaults to '{SERIAL}'. It can be also a TCP/IP port, e.g 'socket://localhost:4321'.",
    )
    parser.add_argument(
        "--build-dir",
        help=f"build dir where flashed {ELF} is. If not given zaru will try to guess it.",
    )

    subparsers = parser.add_subparsers(required=True)

    reboot_parser = subparsers.add_parser("reboot", help="reboot target.")
    reboot_parser.add_argument('--verbose', '-v', action='store_true', help="verbose mode.")
    reboot_parser.set_defaults(func=reboot)

    status_parser = subparsers.add_parser("status", help="get instrumentation status from target.")
    status_parser.add_argument('--verbose', '-v', action='store_true', help="verbose mode.")
    status_parser.set_defaults(func=status)

    trace_parser = subparsers.add_parser("trace", help="get traces from target.")
    trace_parser.add_argument('--verbose', '-v', action='store_true', help="verbose mode.")
    trace_parser.add_argument(
        '--reboot', '-r', action='store_true', help="reboot target before getting traces."
    )
    trace_parser.add_argument(
        '--list',
        '-l',
        action='store_true',
        help="list trigger and stopper functions set in the target.",
    )
    trace_parser.add_argument(
        '--trigger',
        '-t',
        metavar="FUNC_NAME",
        type=str,
        help="set FUNC_NAME as the trigger function. When called it turns on instrumentation.",
    )
    trace_parser.add_argument(
        '--stopper',
        '-s',
        metavar="FUNC_NAME",
        type=str,
        help="set FUNC_NAME as the stopper function. When returned it turns off instrumentation.",
    )
    trace_parser.add_argument(
        '--couple',
        '-c',
        metavar="FUNC_NAME",
        type=str,
        help="set both trigger and stopper to FUNC_NAME.",
    )
    trace_parser.add_argument(
        '--perfetto',
        '-p',
        action='store_true',
        help="export traces to Perfetto TraceViewer UI (Event Trace Format).",
    )
    trace_parser.add_argument(
        '--output',
        '-o',
        type=str,
        default=PERFETTO_FILENAME,
        help=f"output filename. Default to '{PERFETTO_FILENAME}'.",
    )
    trace_parser.add_argument('--demangle', '-d', action='store_true', help="demangle C++ symbols.")
    trace_parser.add_argument(
        '--annotation',
        '-a',
        action='store_false',
        default=True,
        help="disable function name annotation in function returns.",
    )
    trace_parser.set_defaults(func=trace)

    profile_parser = subparsers.add_parser("profile", help="get profile info from target.")
    profile_parser.add_argument('--verbose', '-v', action='store_true', help="verbose mode.")
    profile_parser.add_argument(
        '--reboot', '-r', action='store_true', help="reboot target before profiling."
    )
    profile_parser.add_argument(
        '-n', nargs='?', type=int, default=100, help="show first N most expensive functions."
    )
    profile_parser.set_defaults(func=profile)

    args = parser.parse_args()
    args.func(args)
