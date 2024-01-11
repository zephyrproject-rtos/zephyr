# Copyright (c) 2018 Foundries.io
# Copyright (c) 2019 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from unittest.mock import patch, call
import os

import pytest

from runners import blackmagicprobe
from runners.blackmagicprobe import BlackMagicProbeRunner
from conftest import RC_KERNEL_ELF, RC_KERNEL_HEX, RC_GDB
import serial.tools.list_ports

TEST_GDB_SERIAL = 'test-gdb-serial'

# Expected subprocesses to be run for each command. Using the
# runner_config fixture (and always specifying gdb-serial) means we
# don't get 100% coverage, but it's a starting out point.
EXPECTED_COMMANDS = {
    'attach':
    ([RC_GDB,
      '-ex', "set confirm off",
      '-ex', "target extended-remote {}".format(TEST_GDB_SERIAL),
      '-ex', "monitor swdp_scan",
      '-ex', "attach 1",
      '-ex', "file {}".format(RC_KERNEL_ELF)],),
    'debug':
    ([RC_GDB,
      '-ex', "set confirm off",
      '-ex', "target extended-remote {}".format(TEST_GDB_SERIAL),
      '-ex', "monitor swdp_scan",
      '-ex', "attach 1",
      '-ex', "file {}".format(RC_KERNEL_ELF),
      '-ex', "load {}".format(RC_KERNEL_ELF)],),
    'flash':
    ([RC_GDB,
      '-ex', "set confirm off",
      '-ex', "target extended-remote {}".format(TEST_GDB_SERIAL),
      '-ex', "monitor swdp_scan",
      '-ex', "attach 1",
      '-ex', "load {}".format(RC_KERNEL_HEX),
      '-ex', "kill",
      '-ex', "quit",
      '-silent'],),
}

EXPECTED_CONNECT_SRST_COMMAND = {
        'attach': 'monitor connect_rst disable',
        'debug': 'monitor connect_rst enable',
        'flash': 'monitor connect_rst enable',
}

def require_patch(program):
    assert program == RC_GDB

@pytest.mark.parametrize('command', EXPECTED_COMMANDS)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_blackmagicprobe_init(cc, req, command, runner_config):
    '''Test commands using a runner created by constructor.'''
    runner = BlackMagicProbeRunner(runner_config, TEST_GDB_SERIAL)
    runner.run(command)
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS[command]]

@pytest.mark.parametrize('command', EXPECTED_COMMANDS)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_blackmagicprobe_create(cc, req, command, runner_config):
    '''Test commands using a runner created from command line parameters.'''
    args = ['--gdb-serial', TEST_GDB_SERIAL]
    parser = argparse.ArgumentParser(allow_abbrev=False)
    BlackMagicProbeRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = BlackMagicProbeRunner.create(runner_config, arg_namespace)
    runner.run(command)
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS[command]]

@pytest.mark.parametrize('command', EXPECTED_CONNECT_SRST_COMMAND)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_blackmagicprobe_connect_rst(cc, req, command, runner_config):
    '''Test that commands list the correct connect_rst value when enabled.'''
    args = ['--gdb-serial', TEST_GDB_SERIAL, '--connect-rst']
    parser = argparse.ArgumentParser(allow_abbrev=False)
    BlackMagicProbeRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = BlackMagicProbeRunner.create(runner_config, arg_namespace)
    runner.run(command)
    expected = EXPECTED_CONNECT_SRST_COMMAND[command]
    assert expected in cc.call_args_list[0][0][0]

@pytest.mark.parametrize('arg, env, expected', [
    # Argument has priority
    ('/dev/XXX', None, '/dev/XXX'),
    ('/dev/XXX', '/dev/YYYY', '/dev/XXX'),

    # Then BMP_GDB_SERIAL env variable
    (None, '/dev/XXX', '/dev/XXX'),
    ])
def test_blackmagicprobe_gdb_serial_generic(arg, env, expected):
    if env:
        os.environ['BMP_GDB_SERIAL'] = env
    else:
        if 'BMP_GDB_SERIAL' in os.environ:
            os.environ.pop('BMP_GDB_SERIAL')

    ret = blackmagicprobe.blackmagicprobe_gdb_serial(arg)
    assert expected == ret

@pytest.mark.parametrize('known_path, comports, globs, expected', [
    (True, False, ['/dev/ttyACM0', '/dev/ttyACM1'],
     blackmagicprobe.DEFAULT_LINUX_BMP_PATH),
    (False, True, [], '/dev/ttyACM3'),
    (False, False,  ['/dev/ttyACM0', '/dev/ttyACM1'], '/dev/ttyACM0'),
    (False, False, ['/dev/ttyACM1', '/dev/ttyACM0'], '/dev/ttyACM0'),
    ])
@patch('serial.tools.list_ports.comports')
@patch('os.path.exists')
@patch('glob.glob')
def test_blackmagicprobe_gdb_serial_linux(gg, ope, stlpc, known_path, comports,
                                          globs, expected):
    gg.return_value = globs
    ope.return_value = known_path
    if comports:
        fake_comport1 = serial.tools.list_ports_common.ListPortInfo(
                '/dev/ttyACM1')
        fake_comport1.interface = 'something'
        fake_comport2 = serial.tools.list_ports_common.ListPortInfo(
                '/dev/ttyACM2')
        fake_comport2.interface = None
        fake_comport3 = serial.tools.list_ports_common.ListPortInfo(
                '/dev/ttyACM3')
        fake_comport3.interface = blackmagicprobe.BMP_GDB_INTERFACE
        stlpc.return_value = [fake_comport1, fake_comport2, fake_comport3]
    else:
        stlpc.return_value = []

    ret = blackmagicprobe.blackmagicprobe_gdb_serial_linux()
    assert expected == ret

@pytest.mark.parametrize('comports, globs, expected', [
    (True, [], '/dev/cu.usbmodem3'),
    (False, ['/dev/cu.usbmodemAABBCC0', '/dev/cu.usbmodemAABBCC1'],
     '/dev/cu.usbmodemAABBCC0'),
    (False, ['/dev/cu.usbmodemAABBCC1', '/dev/cu.usbmodemAABBCC0'],
     '/dev/cu.usbmodemAABBCC0'),
    ])
@patch('serial.tools.list_ports.comports')
@patch('glob.glob')
def test_blackmagicprobe_gdb_serial_darwin(gg, stlpc, comports, globs, expected):
    gg.return_value = globs
    if comports:
        fake_comport1 = serial.tools.list_ports_common.ListPortInfo(
                '/dev/cu.usbmodem1')
        fake_comport1.description = 'unrelated'
        fake_comport2 = serial.tools.list_ports_common.ListPortInfo(
                '/dev/cu.usbmodem2')
        fake_comport2.description = None
        fake_comport3 = serial.tools.list_ports_common.ListPortInfo(
                '/dev/cu.usbmodem3')
        fake_comport3.description = f'{blackmagicprobe.BMP_GDB_PRODUCT} v1234'
        fake_comport4 = serial.tools.list_ports_common.ListPortInfo(
                '/dev/cu.usbmodem4')
        fake_comport4.description = f'{blackmagicprobe.BMP_GDB_PRODUCT} v1234'
        stlpc.return_value = [fake_comport1, fake_comport2,
                              fake_comport4, fake_comport3]
    else:
        stlpc.return_value = []

    ret = blackmagicprobe.blackmagicprobe_gdb_serial_darwin()
    assert expected == ret

@pytest.mark.parametrize('comports, expected', [
    (True, 'COM4'),
    (False, 'COM1'),
    ])
@patch('serial.tools.list_ports.comports')
def test_blackmagicprobe_gdb_serial_win32(stlpc, comports, expected):
    if comports:
        fake_comport1 = serial.tools.list_ports_common.ListPortInfo('COM2')
        fake_comport1.vid = 123
        fake_comport1.pid = 456
        fake_comport2 = serial.tools.list_ports_common.ListPortInfo('COM3')
        fake_comport2.vid = None
        fake_comport2.pid = None
        fake_comport3 = serial.tools.list_ports_common.ListPortInfo('COM4')
        fake_comport3.vid = blackmagicprobe.BMP_GDB_VID
        fake_comport3.pid = blackmagicprobe.BMP_GDB_PID
        fake_comport4 = serial.tools.list_ports_common.ListPortInfo('COM5')
        fake_comport4.vid = blackmagicprobe.BMP_GDB_VID
        fake_comport4.pid = blackmagicprobe.BMP_GDB_PID
        stlpc.return_value = [fake_comport1, fake_comport2,
                              fake_comport4, fake_comport3]
    else:
        stlpc.return_value = []

    ret = blackmagicprobe.blackmagicprobe_gdb_serial_win32()
    assert expected == ret
