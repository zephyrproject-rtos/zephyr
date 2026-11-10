#!/usr/bin/env python3
# Copyright (c) 2026 Cody Gu <gujiaqi@iscas.ac.cn>
# SPDX-License-Identifier: Apache-2.0

"""Validate the Zephyr-side hal_hpmicro integration and its HAL module."""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
KCONFIG = ROOT / "Kconfig"

sys.path.insert(0, str(ROOT / "scripts"))

from gen_kconfig import EXTRA_USE_MODULES, sdk_capabilities, sdk_use_modules  # noqa: E402

REQUIRED_MODULE_FILES = (
    "CMakeLists.txt",
    "Kconfig",
    "include/board.h",
    "shim/board_compat.c",
    "scripts/gen_kconfig.py",
)
REQUIRED_HAL_FILES = (
    "hpm_sdk/LICENSE",
    "hpm_sdk/VERSION",
    "zephyr/module.yml",
)
FORBIDDEN_HAL_PATHS = (
    "CMakeLists.txt",
    "Kconfig",
    "include",
    "scripts",
    "src",
)

KCONFIG_SYMBOL_RE = re.compile(r"^config (HAS|USE)_HPMSDK_([A-Za-z0-9_]+)$", re.MULTILINE)
DRIVER_RE = re.compile(r"sdk_src_ifdef\(\s*HPMSOC_HAS_HPMSDK_([A-Za-z0-9_]+)")


def _read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def _kconfig_blocks(text: str) -> dict[str, str]:
    blocks: dict[str, str] = {}
    for match in re.finditer(r"^config (\S+)\n(.*?)(?=^config |\Z)", text, re.M | re.S):
        blocks[match.group(1)] = match.group(2)
    return blocks


def check_required_files(hal_dir: Path) -> list[str]:
    errors = [
        f"missing Zephyr module file: {name}"
        for name in REQUIRED_MODULE_FILES
        if not (ROOT / name).is_file()
    ]
    errors.extend(
        f"missing HAL module file: {name}"
        for name in REQUIRED_HAL_FILES
        if not (hal_dir / name).is_file()
    )
    return errors


def check_module_yml(hal_dir: Path) -> list[str]:
    text = _read_text(hal_dir / "zephyr/module.yml")
    expected = ("name: hal_hpmicro", "cmake-ext: True", "kconfig-ext: True")
    errors = [f"zephyr/module.yml: missing {item}" for item in expected if item not in text]
    forbidden = ("cmake: .", "kconfig: Kconfig")
    errors.extend(
        f"zephyr/module.yml: must not contain {item}" for item in forbidden if item in text
    )
    return errors


def check_hal_layout(hal_dir: Path) -> list[str]:
    return [
        f"hal_hpmicro: Zephyr integration path must not exist: {name}"
        for name in FORBIDDEN_HAL_PATHS
        if (hal_dir / name).exists()
    ]


def check_generated_kconfig(hal_dir: Path) -> list[str]:
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "scripts/gen_kconfig.py"),
            "--sdk",
            str(hal_dir / "hpm_sdk"),
            "--check",
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode == 0:
        return []
    return [result.stderr.strip() or "Kconfig is not up to date"]


def check_kconfig_symbols() -> list[str]:
    errors: list[str] = []
    text = _read_text(KCONFIG)
    blocks = _kconfig_blocks(text)
    symbols = KCONFIG_SYMBOL_RE.findall(text)
    has = {name for kind, name in symbols if kind == "HAS"}
    use = {name for kind, name in symbols if kind == "USE"}

    missing_has = sorted(use - has)
    if missing_has:
        errors.append("USE symbols without HAS symbol: " + ", ".join(missing_has))

    for name in sorted(use):
        block = blocks.get(f"USE_HPMSDK_{name}", "")
        if not re.search(rf"^\tdepends on HAS_HPMSDK_{name}$", block, re.MULTILINE):
            errors.append(f"USE_HPMSDK_{name} does not depend on HAS_HPMSDK_{name}")

    return errors


def check_sdk_mapping(hal_dir: Path) -> list[str]:
    errors: list[str] = []
    text = _read_text(KCONFIG)
    has = {name for kind, name in KCONFIG_SYMBOL_RE.findall(text) if kind == "HAS"}
    use = {name for kind, name in KCONFIG_SYMBOL_RE.findall(text) if kind == "USE"}
    sdk_dir = hal_dir / "hpm_sdk"

    try:
        capabilities = sdk_capabilities(sdk_dir)
        expected_use = sdk_use_modules(sdk_dir)
    except Exception as exc:  # noqa: BLE001 - report any SDK parsing failure
        return [f"cannot read SDK module information: {exc}"]

    missing_has = sorted(capabilities - has)
    extra_has = sorted(has - capabilities)
    if missing_has:
        errors.append("HAS symbols missing from Kconfig: " + ", ".join(missing_has))
    if extra_has:
        errors.append("HAS symbols not present in the SDK: " + ", ".join(extra_has))

    missing_use = sorted(expected_use - use)
    extra_use = sorted(use - expected_use)
    if missing_use:
        errors.append("USE symbols missing from Kconfig: " + ", ".join(missing_use))
    if extra_use:
        errors.append("USE symbols not backed by the SDK: " + ", ".join(extra_use))

    sdk_drivers: set[str] = set()
    for path in sorted(sdk_dir.rglob("CMakeLists.txt")):
        for line in _read_text(path).splitlines():
            if line.lstrip().startswith("#"):
                continue
            sdk_drivers.update(DRIVER_RE.findall(line))

    missing_driver_use = sorted(sdk_drivers - use)
    if missing_driver_use:
        errors.append("SDK driver modules without USE symbol: " + ", ".join(missing_driver_use))

    unknown_extra = sorted(EXTRA_USE_MODULES - has)
    if unknown_extra:
        errors.append("EXTRA_USE_MODULES not present in the SDK: " + ", ".join(unknown_extra))

    return errors


def check_cmake_mapping() -> list[str]:
    text = _read_text(ROOT / "CMakeLists.txt")
    expected = (
        "ZEPHYR_HAL_HPMICRO_MODULE_DIR",
        "HPM_SDK_DIR",
        "soc_modules.list",
        "USE_HPMSDK_",
        "CONFIG_HPM_SDK_BUILD_ALL_DRIVERS",
        "shim/board_compat.c",
    )
    return [f"CMakeLists.txt: missing {item}" for item in expected if item not in text]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument(
        "--hal-dir",
        type=Path,
        help="path to hal_hpmicro (default: ZEPHYR_HAL_HPMICRO_MODULE_DIR)",
    )
    args = parser.parse_args()

    hal_dir = args.hal_dir
    if hal_dir is None:
        module_dir = os.environ.get("ZEPHYR_HAL_HPMICRO_MODULE_DIR")
        hal_dir = Path(module_dir) if module_dir else None
    if hal_dir is None:
        print(
            "error: set ZEPHYR_HAL_HPMICRO_MODULE_DIR or pass --hal-dir",
            file=sys.stderr,
        )
        return 1

    errors: list[str] = []
    errors.extend(check_required_files(hal_dir))
    errors.extend(check_module_yml(hal_dir))
    errors.extend(check_hal_layout(hal_dir))
    errors.extend(check_generated_kconfig(hal_dir))
    errors.extend(check_kconfig_symbols())
    errors.extend(check_sdk_mapping(hal_dir))
    errors.extend(check_cmake_mapping())

    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return 1

    print("hal_hpmicro Zephyr integration: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
