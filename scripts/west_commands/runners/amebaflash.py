#! /usr/bin/env python3

# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

"""Zephyr west runner for Ameba."""

import argparse
import importlib.util
import sys
from pathlib import Path

from runners.core import RunnerCaps, ZephyrBinaryRunner
from zephyr_ext_common import ZEPHYR_BASE


def _get_flash_module():
    """
    Lazy load the external flash.py module.
    This prevents FileNotFoundError during CI tests when the HAL is missing.
    """
    flash_path = (
        Path(ZEPHYR_BASE).parent / "modules" / "hal" / "realtek" / "ameba" / "scripts" / "flash.py"
    )

    if not flash_path.exists():
        return None

    mod_name = "realtek_ameba_flash"
    spec = importlib.util.spec_from_file_location(mod_name, str(flash_path))

    if spec is None or spec.loader is None:
        return None

    flash_mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(flash_mod)
    return flash_mod


class AmebaFlashBinaryRunner(ZephyrBinaryRunner):
    """Runner front-end for AmebaFlash.py."""

    def __init__(self, cfg, args):
        super().__init__(cfg)
        self.args = args

    @classmethod
    def name(cls):
        return "amebaflash"

    @classmethod
    def capabilities(cls):
        # Enable flash; support erase/reset toggles; flash addr; file override; tool passthrough
        return RunnerCaps(commands={"flash"}, erase=True, reset=True)

    @classmethod
    def do_add_parser(cls, parser: argparse.ArgumentParser):
        # Only setup parser if the module actually exists
        flash = _get_flash_module()
        if flash:
            flash.setup_parser(parser)

    @classmethod
    def do_create(cls, cfg, args):
        return AmebaFlashBinaryRunner(cfg, args)

    def do_run(self, command, **kwargs):
        flash = _get_flash_module()
        if flash is None:
            # If we are actually trying to run, and the file is missing, we must error out.
            print(
                "Error: Realtek Ameba flash script not found. "
                "Make sure the HAL module is initialized.",
                file=sys.stderr,
            )
            sys.exit(1)

        flash.main(self.args)
