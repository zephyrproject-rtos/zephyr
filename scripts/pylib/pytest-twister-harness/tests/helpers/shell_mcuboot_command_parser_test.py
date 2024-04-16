# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import textwrap

from twister_harness.helpers.shell import ShellMCUbootCommandParsed, ShellMCUbootArea


def test_if_mcuboot_command_output_is_parsed_two_areas() -> None:
    cmd_output = textwrap.dedent("""
        \x1b[1;32muart:~$ \x1b[mmcuboot
        swap type: revert
        confirmed: 0
        primary area (1):
        version: 0.0.2+0
        image size: 68240
        magic: good
        swap type: test
        copy done: set
        image ok: unset
        secondary area (3):
        version: 0.0.0+0
        image size: 68240
        magic: unset
        swap type: none
        copy done: unset
        image ok: unset
        \x1b[1;32muart:~$ \x1b[m
    """)
    mcuboot_parsed = ShellMCUbootCommandParsed.create_from_cmd_output(cmd_output.splitlines())
    assert isinstance(mcuboot_parsed, ShellMCUbootCommandParsed)
    assert isinstance(mcuboot_parsed.areas[0], ShellMCUbootArea)
    assert len(mcuboot_parsed.areas) == 2
    assert mcuboot_parsed.areas[0].version == '0.0.2+0'
    assert mcuboot_parsed.areas[0].swap_type == 'test'


def test_if_mcuboot_command_output_is_parsed_with_failed_area() -> None:
    cmd_output = textwrap.dedent("""
        \x1b[1;32muart:~$ \x1b[mmcuboot
        swap type: revert
        confirmed: 0
        primary area (1):
        version: 1.1.1+1
        image size: 68240
        magic: good
        swap type: test
        copy done: set
        image ok: unset
        failed to read secondary area (1) header: -5
        \x1b[1;32muart:~$ \x1b[m
    """)
    mcuboot_parsed = ShellMCUbootCommandParsed.create_from_cmd_output(cmd_output.splitlines())
    assert len(mcuboot_parsed.areas) == 1
    assert mcuboot_parsed.areas[0].version == '1.1.1+1'
