#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Run ISR/linker host regressions with real ELF tools and reject skipped tests."""

import argparse
import json
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path
from xml.etree import ElementTree

ROOT = Path(__file__).resolve().parents[2]
SDK_TRIPLES = {
    "arm": "arm-zephyr-eabi",
    "aarch64": "aarch64-zephyr-elf",
    "riscv": "riscv64-zephyr-elf",
}


def executable(path):
    found = shutil.which(str(path))
    if found is None:
        raise FileNotFoundError(f"Required ISR regression tool not found: {path}")
    # LLVM's multicall executables select their driver from argv[0].
    return str(Path(found).absolute())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", required=True, type=Path, help="Zephyr SDK 1.x installation")
    parser.add_argument("--llvm", required=True, type=Path, help="LLVM installation prefix")
    parser.add_argument("--eld", required=True, type=Path, help="Actual ELD executable")
    parser.add_argument("--output", required=True, type=Path, help="New test output directory")
    args = parser.parse_args()
    tools = {}
    try:
        for arch, triple in SDK_TRIPLES.items():
            for suffix in ("gcc", "ld"):
                tools[f"{arch}_{suffix}"] = executable(
                    args.sdk / "gnu" / triple / "bin" / f"{triple}-{suffix}"
                )
        for name in ("clang", "ld.lld"):
            tools[name] = executable(args.llvm / "bin" / name)
        tools["eld"] = executable(args.eld)
        tools["cmake"] = executable("cmake")
        tools["host_gcc"] = executable("gcc")
        args.output.mkdir(parents=True)
    except OSError as error:
        parser.error(str(error))
    versions = {
        name: subprocess.check_output([path, "--version"], text=True, stderr=subprocess.STDOUT)
        for name, path in tools.items()
    }
    env = os.environ.copy()
    env.update(
        PATH=str(args.llvm / "bin") + os.pathsep + env["PATH"],
        CC=tools["host_gcc"],
        CMAKE=tools["cmake"],
        CLANG=tools["clang"],
        ELD=tools["eld"],
        ISR_DISCOVERY_CC=tools["aarch64_gcc"],
        ISR_DISCOVERY_LD=tools["aarch64_ld"],
        ISR_AARCH64_CC=tools["aarch64_gcc"],
        ISR_AARCH64_LD=tools["aarch64_ld"],
        ISR_METADATA_CC=tools["arm_gcc"],
        ISR_METADATA_LD=tools["arm_ld"],
        ISR_NO_ZLA_CC=tools["host_gcc"],
        ISR_RISCV_CC=tools["riscv_gcc"],
        ISR_RISCV_LLD=tools["ld.lld"],
        # The SDK's GCC uses .ctors/ABS32; Clang emits genuine TARGET1 constructors.
        ISR_TARGET1_CC=shlex.join(
            [tools["clang"], "--target=arm-none-eabi", "-mcpu=cortex-m3", "-mthumb"]
        ),
        ISR_TARGET1_LD=tools["arm_ld"],
    )
    report = args.output.resolve() / "junit.xml"
    command = [
        sys.executable,
        "-m",
        "pytest",
        str(ROOT / "scripts/tests/isr_tables"),
        str(ROOT / "scripts/tests/linker"),
        "-q",
        "-rs",
        "-p",
        "no:cacheprovider",
        "--basetemp=" + str(args.output.resolve() / "fixtures"),
        "--junitxml=" + str(report),
    ]
    result = subprocess.run(
        command, env=env, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
    )
    (args.output / "pytest.log").write_text(result.stdout)
    print(result.stdout, end="")
    counts = {}
    if report.is_file():
        suites = list(ElementTree.parse(report).getroot().iter("testsuite"))
        counts = {
            key: sum(int(suite.attrib.get(key, 0)) for suite in suites)
            for key in ("tests", "failures", "errors", "skipped")
        }
    passed = (
        result.returncode == 0
        and counts.get("tests", 0) > 0
        and all(counts.get(key) == 0 for key in ("failures", "errors", "skipped"))
    )
    (args.output / "result.json").write_text(
        json.dumps(
            {
                "passed": passed,
                "counts": counts,
                "versions": versions,
                "command": command,
                "scope": "Real GNU/LLVM/ELD ELF and script tests; not licensed vendor execution",
            },
            indent=2,
        )
        + "\n"
    )
    if not passed:
        parser.exit(1, "ISR regression job failed or skipped required cases; see result.json\n")


if __name__ == "__main__":
    main()
