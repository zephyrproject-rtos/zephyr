# Copyright (c) 2026 Hsiu-Chi Tsai
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import logging
from unittest.mock import patch

import pytest
from conftest import RC_KERNEL_BIN

from runners.esp32 import Esp32BinaryRunner

TEST_DEVICE = '/dev/ttyACM0'
TEST_IDF_PATH = '/test/esp-idf'


def require_esptool(program):
    assert program == 'esptool'


def parse_args(*extra):
    parser = argparse.ArgumentParser(allow_abbrev=False)
    Esp32BinaryRunner.add_parser(parser)
    return parser.parse_args(['--esp-idf-path', TEST_IDF_PATH, '--esp-device', TEST_DEVICE, *extra])


def after_value(cmd):
    '''Value passed to esptool's --after, or None if absent.'''
    return cmd[cmd.index('--after') + 1] if '--after' in cmd else None


@pytest.mark.parametrize(
    'reset_type, expected',
    [
        (None, 'hard-reset'),
        ('hard-reset', 'hard-reset'),
        ('watchdog-reset', 'watchdog-reset'),
    ],
)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_esptool)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_flash_after(cc, req, runner_config, reset_type, expected):
    '''--reset-type selects esptool's post-flash --after value.'''
    extra = ['--reset-type', reset_type] if reset_type else []
    runner = Esp32BinaryRunner.create(runner_config, parse_args(*extra))
    runner.run('flash')

    cmd = cc.call_args_list[-1].args[0]
    assert after_value(cmd) == expected
    assert RC_KERNEL_BIN in cmd


@pytest.mark.parametrize('reset_type', ['hard-reset', 'watchdog-reset'])
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_esptool)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_no_reset_ignores_reset_type(cc, req, runner_config, caplog, reset_type):
    '''--no-reset drops --after and warns for any explicit --reset-type.'''
    args = parse_args('--no-reset', '--reset-type', reset_type)
    runner = Esp32BinaryRunner.create(runner_config, args)
    with caplog.at_level(logging.WARNING, logger='runners.esp32'):
        runner.run('flash')

    cmd = cc.call_args_list[-1].args[0]
    assert '--after' not in cmd
    assert 'write-flash' in cmd
    assert any('reset is disabled' in r.message for r in caplog.records)


@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_esptool)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_skip_flashed_off_by_default(cc, req, runner_config):
    '''skip-flashed is opt-in, so -s is absent unless requested.'''
    runner = Esp32BinaryRunner.create(runner_config, parse_args())
    runner.run('flash')

    cmd = cc.call_args_list[-1].args[0]
    assert '-s' not in cmd
    assert '--diff-with' not in cmd


@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_esptool)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_skip_flashed(cc, req, runner_config):
    '''--esp-skip-flashed adds -s.'''
    runner = Esp32BinaryRunner.create(runner_config, parse_args('--esp-skip-flashed'))
    runner.run('flash')

    cmd = cc.call_args_list[-1].args[0]
    assert '-s' in cmd


@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_esptool)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_skip_flashed_suppressed_when_encrypt(cc, req, runner_config):
    '''-s is dropped under --esp-encrypt since device MD5 would never match.'''
    runner = Esp32BinaryRunner.create(
        runner_config, parse_args('--esp-skip-flashed', '--esp-encrypt')
    )
    runner.run('flash')

    cmd = cc.call_args_list[-1].args[0]
    assert '-s' not in cmd
    assert '--encrypt' in cmd


@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_esptool)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_no_progress(cc, req, runner_config):
    '''--esp-no-progress adds -p.'''
    runner = Esp32BinaryRunner.create(runner_config, parse_args('--esp-no-progress'))
    runner.run('flash')

    cmd = cc.call_args_list[-1].args[0]
    assert '-p' in cmd


@patch('runners.esp32.os.path.exists', side_effect=lambda p: p.endswith('.prev'))
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_esptool)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_diff_disabled_by_default(cc, req, exists, runner_config):
    '''A stray previous binary does not trigger --diff-with unless --esp-diff is set.'''
    runner = Esp32BinaryRunner.create(runner_config, parse_args())
    runner.run('flash')

    cmd = cc.call_args_list[-1].args[0]
    assert '--diff-with' not in cmd


@patch('runners.esp32.shutil.copy2')
@patch('runners.esp32.os.path.exists', side_effect=lambda p: p.endswith('.prev'))
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_esptool)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_diff_supersedes_skip_flashed(cc, req, exists, copy2, runner_config):
    '''--esp-diff with a previous binary uses --diff-with instead of -s.'''
    runner = Esp32BinaryRunner.create(runner_config, parse_args('--esp-diff', '--esp-skip-flashed'))
    runner.run('flash')

    cmd = cc.call_args_list[-1].args[0]
    assert '--diff-with' in cmd
    assert '-s' not in cmd


@patch('runners.esp32.shutil.copy2')
@patch('runners.esp32.os.path.exists', return_value=True)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_esptool)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_diff_does_not_cache_when_encrypt(cc, req, exists, copy2, runner_config):
    '''Under --esp-encrypt the plaintext .prev cache is not written.'''
    runner = Esp32BinaryRunner.create(runner_config, parse_args('--esp-diff', '--esp-encrypt'))
    runner.run('flash')

    cmd = cc.call_args_list[-1].args[0]
    assert '--diff-with' not in cmd
    copy2.assert_not_called()


@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_esptool)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_soc_name_from_config(cc, req, runner_config, caplog, tmp_path):
    '''The flash log reports CONFIG_SOC when .config is available.'''
    config = tmp_path / 'zephyr' / '.config'
    config.parent.mkdir(parents=True)
    config.write_text('CONFIG_SOC="esp32s3"\n')
    cfg = runner_config._replace(build_dir=str(tmp_path))

    runner = Esp32BinaryRunner.create(cfg, parse_args())
    with caplog.at_level(logging.INFO, logger='runners.esp32'):
        runner.run('flash')

    assert any('Flashing esp32s3 chip' in r.message for r in caplog.records)


@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_esptool)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_soc_name_falls_back_without_config(cc, req, runner_config, caplog):
    '''The flash log falls back to "esp32" when .config is missing.'''
    runner = Esp32BinaryRunner.create(runner_config, parse_args())
    with caplog.at_level(logging.INFO, logger='runners.esp32'):
        runner.run('flash')

    assert any('Flashing esp32 chip' in r.message for r in caplog.records)
