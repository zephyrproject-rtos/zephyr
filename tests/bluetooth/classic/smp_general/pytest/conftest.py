# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import contextlib
import logging
import re
import time

import pytest
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)


def pytest_addoption(parser) -> None:
    """Add local parser options to pytest."""
    parser.addoption('--hci-transport', default=None, help='Configuration HCI transport for bumble')


@pytest.fixture(name='initialize', scope='session')
def fixture_initialize(request, shell: Shell, dut: DeviceAdapter):
    """Session initializtion"""
    # Get HCI transport for bumble
    hci = request.config.getoption('--hci-transport')

    if hci is None:
        for fixture in dut.device_config.fixtures:
            if fixture.startswith('usb_hci:'):
                hci = fixture.split(sep=':', maxsplit=1)[1]
                break

    assert hci is not None

    shell.exec_command("bt init")
    dut.readlines_until("Settings Loaded")
    regex = r'(?P<bd_addr>([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2}) *\((.*?)\))'
    bd_addr = None
    lines = shell.exec_command("bt id-show")
    for line in lines:
        m = re.search(regex, line)
        if m:
            bd_addr = m.group('bd_addr')

    if bd_addr is None:
        logger.error('Fail to get IUT BD address')
        raise AssertionError

    shell.exec_command("br pscan on")
    shell.exec_command("br iscan on")
    logger.info('initialized')
    return hci, bd_addr


@pytest.fixture
def smp_initiator_dut(initialize):
    logger.info('Start running testcase')
    yield initialize
    logger.info('Done')


def app_handle_device_output(self) -> None:
    """
    This method is dedicated to run it in separate thread to read output
    from device and put them into internal queue and save to log file.
    """
    with open(self.handler_log_path, 'a+') as log_file:
        while self.is_device_running():
            if self.is_device_connected():
                output = self._read_device_output().decode(errors='replace').rstrip("\r\n")
                if output:
                    self._device_read_queue.put(output)
                    logger.debug(f'{output}\n')
                    try:
                        log_file.write(f'{output}\n')
                    except Exception:
                        contextlib.suppress(Exception)
                    log_file.flush()
            else:
                # ignore output from device
                self._flush_device_output()
                time.sleep(0.1)


# After reboot, there may be gbk character in the console, so replace _handle_device_output to
# handle the exception.
DeviceAdapter._handle_device_output = app_handle_device_output
