# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

"""Hardware test for the NXP single-wire UART test."""

from __future__ import annotations

import os
import selectors
import shutil
import socket
import subprocess
import time
from pathlib import Path

import pytest
from elftools.elf.elffile import ELFFile
from twister_harness import DeviceAdapter

RESULT_SYMBOL = "single_wire_test_result"
RESULT_WORDS = 6
TEST_PASS = 2


def _find_free_port() -> str:
    """Reserve a local TCP port for the LinkServer GDB stub."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return str(sock.getsockname()[1])


def _wait_for_gdb_server(server: subprocess.Popen[str], port: str) -> None:
    """Wait until LinkServer reports its GDB port is listening."""
    deadline = time.monotonic() + 30.0
    output = []
    selector = selectors.DefaultSelector()
    if server.stdout:
        selector.register(server.stdout, selectors.EVENT_READ)
    try:
        while time.monotonic() < deadline:
            if server.poll() is not None:
                if server.stdout:
                    output.extend(server.stdout.read().splitlines(True))
                raise AssertionError(
                    f"LinkServer exited with {server.returncode}:\n{''.join(output)}"
                )
            if server.stdout:
                for key, _ in selector.select(timeout=0.1):
                    line = key.fileobj.readline()
                    if line:
                        output.append(line)
                        if f"port {port}" in line and "listening" in line:
                            return
            else:
                time.sleep(0.1)
    finally:
        selector.close()
    raise AssertionError(f"LinkServer did not open port {port}:\n{''.join(output)}")


def _debugserver_command(device, west: str, port: str) -> list[str]:
    """Build a debugger-server command from Twister's DUT context."""
    if not device.runner:
        raise AssertionError("Twister did not provide a debugger runner")
    if not device.id:
        raise AssertionError("Twister did not provide a debugger device ID")

    command = [
        west,
        "debugserver",
        "--no-rebuild",
        "--build-dir",
        str(device.build_dir),
        "--runner",
        device.runner,
    ]
    command.extend(device.base_params or [])

    # Match the device-selection conventions used by Twister's hardware
    # adapter.  The remaining options are passed to the selected runner.
    if device.runner == "linkserver":
        command.append(f"--probe={device.id}")
    elif device.runner in ("jlink", "mplab_ipe", "pyocd", "spsdk") or "nrf" in device.runner:
        command.extend(["--dev-id", device.id])
    else:
        command.extend(device.runner_params or [])

    command.extend(["--", "--gdb-port", port])
    if device.runner not in ("jlink", "mplab_ipe", "pyocd", "spsdk") and "nrf" not in device.runner:
        command.extend(device.runner_params or [])
    return command


def _find_symbol(elf_path: Path, name: str) -> int:
    with elf_path.open("rb") as stream:
        elf = ELFFile(stream)
        for section_name in (".symtab", ".dynsym"):
            section = elf.get_section_by_name(section_name)
            if section is None:
                continue
            for symbol in section.iter_symbols():
                if symbol.name == name:
                    return symbol.entry.st_value
    raise AssertionError(f"ELF symbol {name!r} was not found")


def _find_reset_vector(elf_path: Path) -> tuple[int, int]:
    """Return the initial stack pointer and reset handler from the ELF."""
    with elf_path.open("rb") as stream:
        elf = ELFFile(stream)
        vector_address = None

        for section_name in (".symtab", ".dynsym"):
            section = elf.get_section_by_name(section_name)
            if section is None:
                continue
            for symbol in section.iter_symbols():
                if symbol.name == "_vector_table":
                    vector_address = symbol.entry.st_value
                    break
            if vector_address is not None:
                break

        if vector_address is None:
            raise AssertionError("ELF vector table symbol was not found")

        for segment in elf.iter_segments():
            if segment["p_type"] != "PT_LOAD":
                continue
            segment_start = segment["p_vaddr"]
            segment_end = segment_start + segment["p_filesz"]
            if segment_start <= vector_address and vector_address + 8 <= segment_end:
                offset = vector_address - segment_start
                vector = segment.data()[offset : offset + 8]
                stack_pointer = int.from_bytes(vector[0:4], "little")
                reset_handler = int.from_bytes(vector[4:8], "little")
                if stack_pointer and reset_handler:
                    return stack_pointer, reset_handler

        raise AssertionError("ELF reset vector was not found")


def _parse_words(output: str, address: int) -> list[int]:
    """Return the last result structure printed by GDB."""
    values = []
    address_text = f"0x{address:x}"
    for line in output.splitlines():
        if address_text not in line and ":" not in line:
            continue
        values.extend(
            int(word, 16) for word in line.split(":", 1)[-1].split() if word.startswith("0x")
        )

    if len(values) < RESULT_WORDS:
        raise AssertionError(f"Could not read test result from debugger:\n{output}")
    return values[-RESULT_WORDS:]


def _run_debugger(
    gdb: str,
    elf: Path,
    address: int,
    port: str,
    stack_pointer: int,
    reset_handler: int,
) -> list[int]:
    """Attach, halt the running target, and read target RAM."""
    commands = [
        "set pagination off",
        "set confirm off",
        "set remotetimeout 5",
        "set mem inaccessible-by-default off",
        f"target remote :{port}",
        f"set $sp = 0x{stack_pointer:x}",
        f"set $pc = 0x{reset_handler & ~1:x}",
        "watch -l single_wire_test_result.state",
        "condition 1 single_wire_test_result.state == 2",
        "continue",
        f"x/{RESULT_WORDS}wx 0x{address:x}",
        "detach",
        "quit",
    ]
    result = subprocess.run(
        [gdb, "-q", str(elf), *sum((["-ex", command] for command in commands), [])],
        check=True,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=10.0,
    )
    return _parse_words(result.stdout, address)


def _find_zephyr_gdb(build_dir: Path) -> str | None:
    """Find the ARM GDB shipped with the Twister-selected Zephyr SDK."""
    # Use the SDK selected by the current Twister process.  This allows the
    # test to run on a machine whose SDK path differs from the build cache.
    sdk_dirs = []
    sdk_dir = os.environ.get("ZEPHYR_SDK_INSTALL_DIR")
    if sdk_dir:
        sdk_dirs.append(Path(sdk_dir))

    cache = build_dir / "CMakeCache.txt"
    if cache.is_file():
        for line in cache.read_text(encoding="utf-8").splitlines():
            if line.startswith("ZEPHYR_SDK_INSTALL_DIR:PATH="):
                cached_sdk_dir = line.split("=", 1)[1]
                if cached_sdk_dir and not cached_sdk_dir.endswith("-NOTFOUND"):
                    sdk_dirs.append(Path(cached_sdk_dir))
                break

    # The build may have been produced on a different machine.  In that
    # case, the CMake cache contains a stale SDK path, so also inspect the
    # runner's normal SDK install locations.
    home = Path.home()
    for parent in (home, Path("/home"), Path("/opt"), Path("/usr/local")):
        if parent.is_dir():
            sdk_dirs.extend(parent.glob("zephyr-sdk-*"))
    if Path("/home").is_dir():
        sdk_dirs.extend(Path("/home").glob("*/zephyr-sdk-*"))

    seen = set()
    for sdk_dir in sdk_dirs:
        sdk_dir = sdk_dir.resolve()
        if sdk_dir in seen:
            continue
        seen.add(sdk_dir)
        for relative_path in (
            Path("gnu") / "arm-zephyr-eabi" / "bin" / "arm-zephyr-eabi-gdb",
            Path("arm-zephyr-eabi") / "bin" / "arm-zephyr-eabi-gdb",
        ):
            candidate = sdk_dir / relative_path
            if candidate.is_file() and os.access(candidate, os.X_OK):
                return str(candidate)

    return shutil.which("arm-zephyr-eabi-gdb")


def test_single_wire_uart(dut: DeviceAdapter) -> None:
    """Verify both directions of the single-wire UART connection."""
    build_dir = Path(dut.device_config.build_dir)
    elf = build_dir / "zephyr" / "zephyr.elf"
    gdb = _find_zephyr_gdb(build_dir) or shutil.which("gdb-multiarch")
    if gdb is None:
        pytest.fail("A Zephyr SDK GDB client is required")
    if not elf.is_file():
        pytest.fail(f"Firmware ELF was not found: {elf}")

    address = _find_symbol(elf, RESULT_SYMBOL)
    stack_pointer, reset_handler = _find_reset_vector(elf)
    device = dut.device_config
    gdb_port = _find_free_port()
    try:
        debugserver_command = _debugserver_command(device, dut.west, gdb_port)
    except AssertionError as error:
        pytest.fail(str(error))

    server = subprocess.Popen(
        debugserver_command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    try:
        _wait_for_gdb_server(server, gdb_port)
        try:
            result = _run_debugger(gdb, elf, address, gdb_port, stack_pointer, reset_handler)
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
            output = server.stdout.read() if server.stdout else ""
            pytest.fail(f"Debugger failed: {error}\nDebug server output:\n{output}")

        assert result[0] == TEST_PASS, (
            f"single-wire UART test failed: state={result[0]}, last_error={result[5]}"
        )
        assert result[1] > 0, "UART1 did not transmit"
        assert result[2] > 0, "UART2 did not transmit"
        assert result[3] > 0, "UART1 did not receive"
        assert result[4] > 0, "UART2 did not receive"
        assert result[5] == 0, f"single-wire UART error: {result[5]}"
    finally:
        if server.poll() is None:
            server.terminate()
        server.wait(timeout=5)
