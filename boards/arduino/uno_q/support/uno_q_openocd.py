#!/usr/bin/env python3
#
# Copyright (c) 2026 Huang Jian <huangjian921@outlook.com>
#
# SPDX-License-Identifier: Apache-2.0

"""OpenOCD-compatible ADB bridge for Arduino UNO Q.

The UNO Q exposes its STM32U585 debug access through OpenOCD (``arduino-debug``)
running on the QRB2210. This script is selected as the board-local OPENOCD
executable and translates the OpenOCD runner's flash/debug invocations into:

  adb forward tcp:3333 tcp:3333
  adb shell arduino-debug
  gdb ... target extended-remote :3333 ...

Custom GDB port: pass ``-c "gdb_port <port>"`` (which the openocd runner does
when ``--gdb-port=<port>`` is given); the script will forward that port over
ADB instead of the default 3333.

RTT is not supported through this bridge.  The ADB-forwarded connection only
exposes the GDB port; the OpenOCD RTT/telnet ports are not forwarded.
"""

import argparse
import os
import shlex
import signal
import socket
import subprocess
import sys
import time
from contextlib import ExitStack
from pathlib import Path

DEFAULT_GDB_PORT = "3333"


def parse_args(argv):
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument("--version", action="store_true")
    parser.add_argument("--log_output")
    parser.add_argument("-c", dest="commands", action="append", default=[])
    parser.add_argument("-f", dest="configs", action="append", default=[])
    parser.add_argument("-s", dest="search", action="append", default=[])
    args, _unknown = parser.parse_known_args(argv)
    args.commands = [command.strip() for command in args.commands]
    return args


def find_command(commands, prefix):
    for command in commands:
        if command.startswith(prefix):
            return command
    return None


def find_gdb_port(commands):
    command = find_command(commands, "gdb_port ")
    if command is None:
        return DEFAULT_GDB_PORT
    return shlex.split(command)[1]


def find_load_image(commands):
    for command in commands:
        parts = shlex.split(command)
        if len(parts) >= 2 and parts[0] == "load_image":
            return parts[1]
    return None


def monitor_commands(commands):
    monitors = []
    reset_after_load = False
    for command in commands:
        if command in {"init", "targets", "halt", "shutdown"}:
            continue
        if command == "gdb_report_data_abort enable":
            continue
        if command.startswith(("tcl_port ", "telnet_port ", "gdb_port ")):
            continue
        if command.startswith(("load_image ", "verify_image ")):
            continue
        if command == "reset init":
            monitors.append("reset halt")
            continue
        if command.startswith("resume "):
            reset_after_load = True
            continue
        monitors.append(command)

    if "reset halt" not in monitors:
        monitors.append("reset halt")
    monitors.append("load")

    if any(command.startswith("verify_image ") for command in commands):
        monitors.append("compare-sections")

    if reset_after_load and "reset run" not in monitors:
        monitors.append("reset run")

    return monitors


def gdb_program():
    gdb = os.environ.get("GDB")
    if gdb:
        return gdb

    toolchain = os.environ.get("GNUARMEMB_TOOLCHAIN_PATH")
    if toolchain:
        suffixes = ["arm-none-eabi-gdb.exe", "arm-none-eabi-gdb-py.exe"]
        if os.name != "nt":
            suffixes = ["arm-none-eabi-gdb", "arm-none-eabi-gdb-py"]
        for suffix in suffixes:
            candidate = Path(toolchain) / "bin" / suffix
            if candidate.exists():
                return str(candidate)

    return "gdb"


def run_or_print(cmd, dry_run=False, check=True, **kwargs):
    printable = " ".join(shlex.quote(str(part)) for part in cmd)
    if dry_run:
        print(printable)
        return None
    return subprocess.run(cmd, check=check, **kwargs)


def popen_or_print(cmd, dry_run=False, **kwargs):
    printable = " ".join(shlex.quote(str(part)) for part in cmd)
    if dry_run:
        print(printable)
        return None
    return subprocess.Popen(cmd, **kwargs)


def open_log(args, stack):
    if not args.log_output:
        return None

    log_path = Path(args.log_output)
    log_path.parent.mkdir(parents=True, exist_ok=True)
    return stack.enter_context(log_path.open("ab"))


def stop_remote_openocd(adb, dry_run=False):
    if dry_run:
        print(f"{adb} shell pidof openocd")
        print(f"{adb} shell kill <openocd-pid> ...")
        return

    result = subprocess.run(
        [adb, "shell", "pidof", "openocd"],
        check=False,
        capture_output=True,
        text=True,
    )
    for pid in result.stdout.split():
        subprocess.run([adb, "shell", "kill", pid], check=False)


def stop_process(proc):
    if proc is None:
        return

    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()


def wait_for_port(port, timeout=10):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.settimeout(0.5)
            if sock.connect_ex(("127.0.0.1", int(port))) == 0:
                return
        time.sleep(0.2)
    raise TimeoutError(f"Timed out waiting for OpenOCD GDB port {port}")


def flash(args):
    image_file = find_load_image(args.commands)
    if image_file is None:
        raise SystemExit("UNO Q ADB OpenOCD bridge supports ELF load_image flashing only")

    gdb_port = find_gdb_port(args.commands)
    dry_run = os.environ.get("UNO_Q_OPENOCD_DRY_RUN") == "1"
    adb = os.environ.get("ADB", "adb")

    gdb_cmd = [
        gdb_program(),
        "--batch",
        "-ex",
        "set confirm off",
        "-ex",
        "set pagination off",
        "-ex",
        "set remotetimeout 30",
        "-ex",
        f"target extended-remote :{gdb_port}",
    ]
    for command in monitor_commands(args.commands):
        if command in {"load", "compare-sections"}:
            gdb_cmd.extend(["-ex", command])
        else:
            gdb_cmd.extend(["-ex", f"monitor {command}"])
    gdb_cmd.extend(["-ex", "detach", "-ex", "quit", image_file])

    server_proc = None
    with ExitStack() as stack:
        log_file = open_log(args, stack)
        output = log_file if log_file is not None else subprocess.DEVNULL
        try:
            stop_remote_openocd(adb, dry_run)
            run_or_print(
                [adb, "forward", f"tcp:{gdb_port}", f"tcp:{gdb_port}"],
                dry_run,
                stdout=subprocess.DEVNULL,
            )
            server_proc = popen_or_print(
                [adb, "shell", "arduino-debug"],
                dry_run,
                stdout=output,
                stderr=output,
            )
            if not dry_run:
                wait_for_port(gdb_port)
            run_or_print(gdb_cmd, dry_run)
        finally:
            stop_process(server_proc)
            stop_remote_openocd(adb, dry_run)
            run_or_print(
                [adb, "forward", "--remove", f"tcp:{gdb_port}"],
                dry_run,
                stdout=subprocess.DEVNULL,
            )


def debug_server(args):
    gdb_port = find_gdb_port(args.commands)
    dry_run = os.environ.get("UNO_Q_OPENOCD_DRY_RUN") == "1"
    adb = os.environ.get("ADB", "adb")

    stop_remote_openocd(adb, dry_run)
    run_or_print(
        [adb, "forward", f"tcp:{gdb_port}", f"tcp:{gdb_port}"],
        dry_run,
        stdout=subprocess.DEVNULL,
    )
    proc = popen_or_print([adb, "shell", "arduino-debug"], dry_run)
    if proc is None:
        return

    def _stop(_signum, _frame):
        stop_process(proc)
        run_or_print(
            [adb, "forward", "--remove", f"tcp:{gdb_port}"],
            dry_run,
            stdout=subprocess.DEVNULL,
        )
        raise SystemExit(0)

    signal.signal(signal.SIGINT, _stop)
    signal.signal(signal.SIGTERM, _stop)
    try:
        proc.wait()
    finally:
        run_or_print(
            [adb, "forward", "--remove", f"tcp:{gdb_port}"],
            dry_run,
            stdout=subprocess.DEVNULL,
        )


def main(argv):
    args = parse_args(argv)
    if args.version:
        adb = os.environ.get("ADB", "adb")
        result = subprocess.run(
            [adb, "shell", "arduino-debug", "--version"],
            check=False,
            stderr=subprocess.STDOUT,
            stdout=subprocess.PIPE,
            text=True,
        )
        print(result.stdout, end="")
        return result.returncode

    if find_load_image(args.commands) is not None:
        flash(args)
    else:
        debug_server(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
