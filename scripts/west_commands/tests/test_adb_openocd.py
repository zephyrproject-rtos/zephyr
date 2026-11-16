# Copyright (c) 2026 The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import subprocess
from unittest.mock import call, patch

import pytest
from conftest import RC_KERNEL_ELF

from runners.adb_openocd import AdbOpenOcdBinaryRunner

REMOTE_ELF = '/tmp/zephyr-adb-openocd/zephyr.elf'

OPENOCD_DEFAULT = (
    '/opt/openocd/bin/openocd -s /opt/openocd -s /opt/openocd/share/openocd/scripts '
    '-f openocd_gpiod.cfg -c init -c \'reset halt\' '
    f'-c \'flash write_image erase "{REMOTE_ELF}"\' -c \'verify_image "{REMOTE_ELF}"\' '
    '-c \'reset run\' -c shutdown'
)

OPENOCD_CUSTOM = (
    '/opt/openocd/bin/openocd -s /opt/openocd -s /opt/openocd/share/openocd/scripts '
    '-f openocd_gpiod.cfg -c \'reset_config srst_only\' -c init -c \'reset halt\' '
    f'-c \'flash write_image erase "{REMOTE_ELF}"\' '
    '-c \'reset run\' -c \'mww 0xE000EDF0 0xA05F0000\' -c shutdown'
)


def expected_calls(prefix, openocd_cmd):
    return [
        call(prefix + ['shell', 'mkdir', '-p', '/tmp/zephyr-adb-openocd']),
        call(prefix + ['push', RC_KERNEL_ELF, REMOTE_ELF]),
        call(prefix + ['shell', openocd_cmd]),
        call(prefix + ['shell', 'rm', '-f', REMOTE_ELF]),
    ]


TEST_CASES = [
    ([], ['adb'], OPENOCD_DEFAULT),
    (
        [
            '--dev-id',
            '1234',
            '--cmd-pre-init',
            'reset_config srst_only',
            '--cmd-post-reset',
            'mww 0xE000EDF0 0xA05F0000',
            '--no-verify',
        ],
        ['adb', '-s', '1234'],
        OPENOCD_CUSTOM,
    ),
]


@pytest.mark.parametrize('args, prefix, openocd_cmd', TEST_CASES)
@patch('runners.core.ZephyrBinaryRunner.require')
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_adb_openocd(cc, req, args, prefix, openocd_cmd, runner_config):
    parser = argparse.ArgumentParser(allow_abbrev=False)
    AdbOpenOcdBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = AdbOpenOcdBinaryRunner.create(runner_config, arg_namespace)
    with patch('os.path.isfile', return_value=True):
        runner.run('flash')
    assert cc.call_args_list == expected_calls(prefix, openocd_cmd)


@pytest.mark.parametrize('flash_fails, cleanup_fails', [(True, False), (False, True), (True, True)])
@patch('runners.core.ZephyrBinaryRunner.require')
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_adb_openocd_failures(cc, req, flash_fails, cleanup_fails, runner_config, caplog):
    flash_error = subprocess.CalledProcessError(1, ['adb', 'shell', OPENOCD_DEFAULT])
    cleanup_error = subprocess.CalledProcessError(1, ['adb', 'shell', 'rm'])
    cc.side_effect = [
        None,
        None,
        flash_error if flash_fails else None,
        cleanup_error if cleanup_fails else None,
    ]
    runner = AdbOpenOcdBinaryRunner(runner_config)

    with patch('os.path.isfile', return_value=True):
        if flash_fails:
            with pytest.raises(subprocess.CalledProcessError) as exc_info:
                runner.run('flash')
            assert exc_info.value is flash_error
        else:
            runner.run('flash')

    assert cc.call_args_list == expected_calls(['adb'], OPENOCD_DEFAULT)
    if cleanup_fails:
        assert f'failed to remove {REMOTE_ELF} on device: {cleanup_error}' in caplog.text
    else:
        assert 'failed to remove' not in caplog.text


def test_adb_openocd_west_flash_parser():
    # west flash adds common options like --openocd to the same parser, so the
    # runner's own options must not clash with them.
    from types import SimpleNamespace

    from run_common import add_parser_common

    parser = argparse.ArgumentParser(allow_abbrev=False)
    add_parser_common(SimpleNamespace(name='flash', description=''), parser=parser)
    AdbOpenOcdBinaryRunner.add_parser(parser)
