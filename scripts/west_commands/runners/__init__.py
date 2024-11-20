# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

import importlib
import logging

from runners.core import ZephyrBinaryRunner

_logger = logging.getLogger('runners')

def _import_runner_module(runner_name):
    try:
        importlib.import_module(f'runners.{runner_name}')
    except ImportError as ie:
        # Runners are supposed to gracefully handle failures when they
        # import anything outside of stdlib, but they sometimes do
        # not. Catch ImportError to handle this.
        _logger.warning(f'The module for runner "{runner_name}" '
                        f'could not be imported ({ie}). This most likely '
                        'means it is not handling its dependencies properly. '
                        'Please report this to the zephyr developers.')

# We import these here to ensure the ZephyrBinaryRunner subclasses are
# defined; otherwise, ZephyrBinaryRunner.get_runners() won't work.

_names = [
    # zephyr-keep-sorted-start
    'blackmagicprobe',
    'bossac',
    'canopen_program',
    'dediprog',
    'dfu',
    'esp32',
    'ezflashcli',
    'gd32isp',
    'hifive1',
    'intel_adsp',
    'intel_cyclonev',
    'jlink',
    'linkserver',
    'mdb',
    'misc',
    'native',
    'nios2',
    'nrfjprog',
    'nrfutil',
    'nsim',
    'nxp_s32dbg',
    'openocd',
    'probe_rs',
    'pyocd',
    'qemu',
    'renode',
    'renode-robot',
    'silabs_commander',
    'spi_burn',
    'stm32cubeprogrammer',
    'stm32flash',
    'teensy',
    'trace32',
    'uf2',
    'xsdb',
    'xtensa',
    # zephyr-keep-sorted-stop
]

for _name in _names:
    _import_runner_module(_name)

def get_runner_cls(runner):
    '''Get a runner's class object, given its name.'''
    for cls in ZephyrBinaryRunner.get_runners():
        if cls.name() == runner:
            return cls
    raise ValueError('unknown runner "{}"'.format(runner))

__all__ = ['ZephyrBinaryRunner', 'get_runner_cls']
