# Copyright (c) 2018 Foundries.io
#
# SPDX-License-Identifier: Apache-2.0

'''Common fixtures for use testing the runner package.'''

import pytest

from runners.core import RunnerConfig, FileType

RC_BUILD_DIR = '/test/build-dir'
RC_BOARD_DIR = '/test/zephyr/boards/test-arch/test-board'
RC_KERNEL_ELF = 'test-zephyr.elf'
RC_KERNEL_EXE = 'test-zephyr.exe'
RC_KERNEL_HEX = 'test-zephyr.hex'
RC_KERNEL_BIN = 'test-zephyr.bin'
RC_KERNEL_MOT = 'test-zephyr.mot'
RC_GDB = 'test-none-gdb'
RC_OPENOCD = 'test-openocd'
RC_OPENOCD_SEARCH = ['/test/openocd/search']


@pytest.fixture
def runner_config():
    '''Fixture which provides a runners.core.RunnerConfig.'''
    return RunnerConfig(RC_BUILD_DIR, RC_BOARD_DIR, RC_KERNEL_ELF, RC_KERNEL_EXE,
                        RC_KERNEL_HEX, RC_KERNEL_BIN, RC_KERNEL_MOT, None, FileType.OTHER,
                        gdb=RC_GDB, openocd=RC_OPENOCD,
                        openocd_search=RC_OPENOCD_SEARCH)


class _FakeConfig:
    '''Minimal stand-in for west.configuration.Configuration in tests.'''

    def __init__(self, values=None):
        self._values = dict(values or {})

    def get(self, option, default=None):
        return self._values.get(option, default)

    def getboolean(self, option, default=False):
        value = self._values.get(option)
        if value is None:
            return default
        if isinstance(value, bool):
            return value
        return str(value).lower() in ('1', 'true', 'yes', 'on')
