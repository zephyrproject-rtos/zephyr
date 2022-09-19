# Copyright (c) 2018 Foundries.io
#
# SPDX-License-Identifier: Apache-2.0

from runners.core import ZephyrBinaryRunner

def test_runner_imports():
    # Ensure that all runner modules are imported and returned by
    # get_runners().
    #
    # This is just a basic sanity check against errors introduced by
    # tree-wide refactorings for runners that don't have their own
    # test suites.
    runner_names = set(r.name() for r in ZephyrBinaryRunner.get_runners())

    # Please keep this sorted alphabetically.
    expected = set(('arc-nsim',
                    'blackmagicprobe',
                    'bossac',
                    'canopen',
                    'dediprog',
                    'dfu-util',
                    'esp32',
                    'ezflashcli',
                    'gd32isp',
                    'hifive1',
                    'intel_adsp',
                    'intel_cyclonev',
                    'jlink',
                    'mdb-nsim',
                    'mdb-hw',
                    'misc-flasher',
                    'nios2',
                    'nrfjprog',
                    'openocd',
                    'pyocd',
                    'qemu',
                    'spi_burn',
                    'stm32cubeprogrammer',
                    'stm32flash',
                    'xtensa'))
    assert runner_names == expected
