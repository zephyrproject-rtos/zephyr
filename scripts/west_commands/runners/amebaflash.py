#! /usr/bin/env python3

# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

"""Zephyr west runner for Ameba."""

import argparse
import importlib.util
import sys
from functools import partial
from pathlib import Path

from runners.core import RunnerCaps, ZephyrBinaryRunner, depr_action
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


class _HalParser:
    """
    Parser proxy handed to the HAL flash script's setup_parser().

    The HAL registers its own --baudrate option. Store it in the same
    destination as the common --baud-rate option, so that the last one given
    wins: a user --baudrate on the command line then overrides a board
    --baud-rate from runners.yaml, instead of being silently replaced by it.
    """

    def __init__(self, parser, cls):
        self._parser = parser
        self._cls = cls

    def add_argument(self, *names, **kwargs):
        if '--baudrate' in names:
            self._parser.set_defaults(baud_rate=kwargs.get('default'))
            return self._parser.add_argument(
                *names,
                dest='baud_rate',
                action=partial(depr_action, cls=self._cls, replacement='--baud-rate'),
                help='Deprecated, use --baud-rate instead.',
            )
        return self._parser.add_argument(*names, **kwargs)

    def __getattr__(self, name):
        return getattr(self._parser, name)


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
        return RunnerCaps(commands={"flash"}, erase=True, reset=True, baud_rate=True)

    @classmethod
    def do_add_parser(cls, parser: argparse.ArgumentParser):
        # Only setup parser if the module actually exists
        flash = _get_flash_module()
        if flash:
            flash.setup_parser(_HalParser(parser, cls))

    @classmethod
    def do_create(cls, cfg, args):
        # The HAL flash script reads args.baudrate
        args.baudrate = args.baud_rate
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
