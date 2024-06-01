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

    expected = set((
        # zephyr-keep-sorted-start
        'arc-nsim',
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
        'linkserver',
        'mdb-hw',
        'mdb-nsim',
        'minichlink',
        'misc-flasher',
        'native',
        'nios2',
        'nrfjprog',
        'nrfutil',
        'nxp_s32dbg',
        'openocd',
        'probe-rs',
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
    ))
    assert runner_names == expected
