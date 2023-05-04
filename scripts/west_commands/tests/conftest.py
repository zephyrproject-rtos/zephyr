# Copyright (c) 2018 Foundries.io
#
# SPDX-License-Identifier: Apache-2.0

'''Common fixtures for use testing the runner package.'''

import pytest

from runners.core import RunnerConfig, FileType

RC_BUILD_DIR = '/test/build-dir'
RC_BOARD_DIR = '/test/zephyr/boards/test-arch/test-board'
RC_KERNEL_ELF = 'test-zephyr.elf'
RC_KERNEL_HEX = 'test-zephyr.hex'
RC_KERNEL_BIN = 'test-zephyr.bin'
RC_GDB = 'test-none-gdb'
RC_OPENOCD = 'test-openocd'
RC_OPENOCD_SEARCH = ['/test/openocd/search']


@pytest.fixture
def runner_config():
    '''Fixture which provides a runners.core.RunnerConfig.'''
    return RunnerConfig(RC_BUILD_DIR, RC_BOARD_DIR, RC_KERNEL_ELF,
                        RC_KERNEL_HEX, RC_KERNEL_BIN, None, FileType.OTHER,
                        gdb=RC_GDB, openocd=RC_OPENOCD,
                        openocd_search=RC_OPENOCD_SEARCH)
