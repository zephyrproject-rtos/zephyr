import argparse
import os
from typing import Callable, Any


class ArgumentHandler:
    def __init__(self, argument_add_func: Callable[..., Any]):
        self.argument_add_func = argument_add_func

    def add_common_arguments(self):
        self.argument_add_func(
            "--twister-harness",
            action="store_true",
            default=False,
            help="Activate Twister harness plugin.",
        )
        self.argument_add_func(
            "--base-timeout",
            type=float,
            default=60.0,
            help="Set base timeout (in seconds) used during monitoring if some "
            "operations are finished in a finite amount of time (e.g. waiting "
            "for flashing).",
        )
        self.argument_add_func(
            "--build-dir", metavar="PATH", help="Directory with built application."
        )
        self.argument_add_func(
            "--device-type",
            choices=("native", "qemu", "hardware", "unit", "custom"),
            help="Choose type of device (hardware, qemu, etc.).",
        )
        self.argument_add_func(
            "--platform",
            help="Name of used platform (qemu_x86, nrf52840dk_nrf52840, etc.).",
        )
        self.argument_add_func(
            "--device-serial",
            help="Serial device for accessing the board (e.g., /dev/ttyACM0).",
        )
        self.argument_add_func(
            "--device-serial-baud",
            type=int,
            default=115200,
            help="Serial device baud rate (default 115200).",
        )
        self.argument_add_func(
            "--runner", help="Use the specified west runner (pyocd, nrfjprog, etc.)."
        )
        self.argument_add_func(
            "--device-id",
            help="ID of connected hardware device (for example 000682459367).",
        )
        self.argument_add_func(
            "--device-product",
            help='Product name of connected hardware device (e.g. "STM32 STLink").',
        )
        self.argument_add_func(
            "--device-serial-pty", help="Script for controlling pseudoterminal."
        )
        self.argument_add_func(
            "--west-flash-extra-args",
            help="Extend parameters for west flash. "
            'E.g. --west-flash-extra-args="--board-id=foobar,--erase" '
            'will translate to "west flash -- --board-id=foobar --erase".',
        )
        self.argument_add_func(
            "--pre-script",
            metavar="PATH",
            help="Script executed before flashing and connecting to serial.",
        )
        self.argument_add_func(
            "--post-flash-script",
            metavar="PATH",
            help="Script executed after flashing.",
        )
        self.argument_add_func(
            "--post-script",
            metavar="PATH",
            help="Script executed after closing serial connection.",
        )
        self.argument_add_func(
            "--dut-scope",
            choices=("function", "class", "module", "package", "session"),
            help="The scope for which `dut` and `shell` fixtures are shared.",
        )

    @classmethod
    def sanitize_options(cls, options: argparse.Namespace):
        _normalize_paths(options)
        _validate_options(options)


def _validate_options(options: argparse.Namespace) -> None:
    if not options.build_dir:
        raise Exception("--build-dir has to be provided")
    if not os.path.isdir(options.build_dir):
        raise Exception(f"Provided --build-dir does not exist: {options.build_dir}")
    if not options.device_type:
        raise Exception("--device-type has to be provided")


def _normalize_paths(options: argparse.Namespace) -> None:
    """Normalize paths provided by user via CLI"""
    options.build_dir = _normalize_path(options.build_dir)
    options.pre_script = _normalize_path(options.pre_script)
    options.post_script = _normalize_path(options.post_script)
    options.post_flash_script = _normalize_path(options.post_flash_script)


def _normalize_path(path: str | None) -> str:
    if path is not None:
        path = os.path.expanduser(os.path.expandvars(path))
        path = os.path.normpath(os.path.abspath(path))
    return path
