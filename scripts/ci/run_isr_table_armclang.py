#!/usr/bin/env python3
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Build and run real Cortex-M ISR tests with an activated Arm Compiler 6."""

import argparse
import hashlib
import json
import os
import re
import selectors
import shlex
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
RUNTIME_APP = "tests/arch/common/isr_table_runtime"
BOARD = "qemu_cortex_m3"
CASES = (
    ("defaults", RUNTIME_APP, ()),
    ("array.static", RUNTIME_APP, ("CONFIG_GEN_SW_ISR_TABLE_ARRAY=y",)),
    ("array.lto", RUNTIME_APP, ("CONFIG_GEN_SW_ISR_TABLE_ARRAY=y", "CONFIG_LTO=y")),
    (
        "array.dynamic",
        RUNTIME_APP,
        ("CONFIG_GEN_SW_ISR_TABLE_ARRAY=y", "CONFIG_DYNAMIC_INTERRUPTS=y"),
    ),
    (
        "array.shared",
        RUNTIME_APP,
        ("CONFIG_GEN_SW_ISR_TABLE_ARRAY=y", "CONFIG_SHARED_INTERRUPTS=y"),
    ),
    ("switch.static", RUNTIME_APP, ("CONFIG_GEN_SW_ISR_TABLE_SWITCH=y",)),
    (
        "switch.shared",
        RUNTIME_APP,
        ("CONFIG_GEN_SW_ISR_TABLE_SWITCH=y", "CONFIG_SHARED_INTERRUPTS=y"),
    ),
    (
        "nvic",
        "tests/arch/common/gen_isr_table",
        ("CONFIG_NULL_POINTER_EXCEPTION_DETECTION_NONE=y",),
    ),
)
RUNTIME_TIMEOUT_SECONDS = 20
SUCCESS = b"PROJECT EXECUTION SUCCESSFUL"
SUMMARY = re.compile(r"pass = (\d+), fail = (\d+), skip = (\d+), total = (\d+)")


def command_log(command, log, env):
    with log.open("w") as stream:
        result = subprocess.run(command, env=env, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError(f"Command failed ({result.returncode}); see {log}")


def run_qemu(command, log, env):
    data = bytearray()
    deadline = time.monotonic() + RUNTIME_TIMEOUT_SECONDS
    process = subprocess.Popen(command, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    terminated_by_runner = False
    try:
        with selectors.DefaultSelector() as ready:
            ready.register(process.stdout, selectors.EVENT_READ)
            while time.monotonic() < deadline and SUCCESS not in data:
                if not ready.select(timeout=1):
                    if process.poll() is not None:
                        break
                    continue
                block = os.read(process.stdout.fileno(), 65536)
                if not block:
                    break
                data.extend(block)
    finally:
        if process.poll() is None and SUCCESS in data:
            try:
                process.wait(timeout=0.1)
            except subprocess.TimeoutExpired:
                terminated_by_runner = True
        if process.poll() is None:
            process.terminate()
        try:
            tail, _ = process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            tail, _ = process.communicate()
        data.extend(tail)
        log.write_bytes(data)
    summaries = SUMMARY.findall(data.decode(errors="replace"))
    if (
        SUCCESS not in data
        or b"PROJECT EXECUTION FAILED" in data
        or (
            process.returncode != 0
            and not (terminated_by_runner and process.returncode == -signal.SIGTERM)
        )
        or len(summaries) != 1
        or b"FAIL -" in data
        or any(int(value) for value in summaries[0][1:3])
    ):
        raise RuntimeError(f"QEMU test failed or skipped cases; see {log}")
    passed, _, _, total = map(int, summaries[0])
    if passed != total or not total:
        raise RuntimeError(f"Incomplete QEMU test summary; see {log}")
    return total


def source_inputs(build, ninja):
    dependencies = subprocess.check_output([ninja, "-C", str(build), "-t", "deps"], text=True)
    paths = {Path(line.strip()) for line in dependencies.splitlines()}
    paths.update(
        Path(token.rstrip(":/"))
        for token in re.findall(
            re.escape(str(ROOT)) + r'/[^\s|;"()<>]+', (build / "build.ninja").read_text()
        )
    )
    return {
        str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in sorted(paths)
        if path.is_file() and path.is_relative_to(ROOT)
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--toolchain", required=True, type=Path)
    parser.add_argument("--qemu", required=True, type=Path)
    parser.add_argument(
        "--output", required=True, type=Path, help="New project-owned output directory"
    )
    args = parser.parse_args()
    west, ninja = shutil.which("west"), shutil.which("ninja")
    if not west or not ninja:
        parser.error("west and ninja must be installed")
    if not args.qemu.is_file() or not (args.toolchain / "bin/armlink").is_file():
        parser.error("Actual Arm tools and QEMU executables are required")
    try:
        args.output.mkdir(parents=True)
    except OSError as error:
        parser.error(str(error))
    env = os.environ | {
        "ZEPHYR_BASE": str(ROOT),
        "ZEPHYR_TOOLCHAIN_VARIANT": "armclang",
        "ARMCLANG_TOOLCHAIN_PATH": str(args.toolchain.resolve()),
        "USER_CACHE_DIR": str(args.output.resolve() / "cache"),
    }
    report = {"passed": False, "board": BOARD, "cases": [], "source_sha256": {}}
    try:
        report["versions"] = {
            tool: subprocess.check_output(
                [str(args.toolchain / "bin" / tool), "--vsn"],
                env=env,
                text=True,
                stderr=subprocess.STDOUT,
            )
            for tool in ("armclang", "armlink")
        }
        report["qemu_version"] = subprocess.check_output(
            [str(args.qemu), "--version"], env=env, text=True
        )
        for name, app, configuration in CASES:
            directory = args.output.resolve() / name
            directory.mkdir()
            command = [
                west,
                "build",
                "-b",
                BOARD,
                str(ROOT / app),
                "-d",
                str(directory),
                "-o=-j2",
                "--",
                "-DBUILD_VERSION=isr-table-armclang-ci",
                "-DUSER_CACHE_DIR=" + env["USER_CACHE_DIR"],
            ]
            if name == "defaults":
                command += ["-DCONF_FILE=default.conf", "-DISR_TABLE_TEST_DEFAULTS=ON"]
            elif configuration:
                extra = directory / "case.conf"
                extra.write_text("\n".join(configuration) + "\n")
                command += ["-DEXTRA_CONF_FILE=" + str(extra)]
            command_log(command, directory / "build.log", env)
            zephyr = directory / "zephyr"
            config = (zephyr / ".config").read_text()
            for required in (
                "CONFIG_ISR_TABLES_LOCAL_DECLARATION=y",
                "CONFIG_CMAKE_LINKER_GENERATOR=y",
                *configuration,
            ):
                if required + "\n" not in config:
                    raise RuntimeError(
                        f"{name}: requested configuration was not enabled: {required}"
                    )
            checks = json.loads((zephyr / "isr_tables_check.json").read_text())
            if (
                checks["status"] != "pass"
                or not checks["checks"]
                or not all(check["passed"] for check in checks["checks"])
            ):
                raise RuntimeError(f"{name}: strict production ELF validation failed")
            for stage in ("zephyr_pre0", "zephyr_pre1", "zephyr"):
                export = json.loads((zephyr / (stage + ".elf.armlink-symbols.json")).read_text())
                if not export["program_image_unchanged"]:
                    raise RuntimeError(f"{name}: symbol export changed the load image")
            inputs = source_inputs(directory, ninja)
            for path, digest in inputs.items():
                previous = report["source_sha256"].setdefault(path, digest)
                if previous != digest:
                    raise RuntimeError(f"Source changed between configurations: {path}")
            qemu_command = [
                str(args.qemu.resolve()),
                "-machine",
                "lm3s6965evb",
                "-cpu",
                "cortex-m3",
                "-nographic",
                "-serial",
                "mon:stdio",
                "-kernel",
                str(zephyr / "zephyr.elf"),
            ]
            lto_evidence = {}
            if "CONFIG_LTO=y" in configuration:
                compilation = json.loads((directory / "compile_commands.json").read_text())
                main = next(
                    item for item in compilation if Path(item["file"]) == ROOT / app / "src/main.c"
                )
                compile_args = (
                    main["arguments"] if "arguments" in main else shlex.split(main["command"])
                )
                if not any(arg == "-flto" or arg.startswith("-flto=") for arg in compile_args):
                    raise RuntimeError(f"{name}: application compilation did not enable LTO")
                commands = subprocess.check_output(
                    [ninja, "-C", str(directory), "-t", "commands", "zephyr/zephyr.elf"],
                    text=True,
                ).splitlines()
                links = [
                    line
                    for line in commands
                    if any(Path(arg).name == "armlink" for arg in shlex.split(line))
                ]
                if len(links) != 3 or not all("--lto" in shlex.split(line) for line in links):
                    raise RuntimeError(f"{name}: all three native links must enable --lto")
                lto_evidence = {"compile_command": main["command"], "link_commands": links}
            count = run_qemu(qemu_command, directory / "qemu.log", env)
            report["cases"].append(
                {
                    "name": name,
                    "passed": True,
                    "tests": count,
                    "elf_sha256": hashlib.sha256((zephyr / "zephyr.elf").read_bytes()).hexdigest(),
                    "checks": len(checks["checks"]),
                    "command": command,
                    "qemu_command": qemu_command,
                    "source_inputs": len(inputs),
                    "lto_evidence": lto_evidence,
                }
            )
            print(f"{name}: build, ELF checker and {count} QEMU tests passed", flush=True)
        for path, digest in report["source_sha256"].items():
            if hashlib.sha256((ROOT / path).read_bytes()).hexdigest() != digest:
                raise RuntimeError(f"Source changed during validation: {path}")
        report["passed"] = True
    except (OSError, ValueError, KeyError, RuntimeError, subprocess.CalledProcessError) as error:
        report["error"] = str(error)
        print(error, file=sys.stderr)
    finally:
        (args.output / "result.json").write_text(json.dumps(report, indent=2) + "\n")
    if not report["passed"]:
        parser.exit(
            1, "Native Arm ISR validation failed; last working artifacts remain untouched\n"
        )


if __name__ == "__main__":
    main()
