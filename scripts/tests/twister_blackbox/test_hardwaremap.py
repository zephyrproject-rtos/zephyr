#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Blackbox tests for twister's command line functions
"""
import importlib
from unittest import mock
import os
import pytest
import sys

# pylint: disable=no-name-in-module
from conftest import ZEPHYR_BASE, suite_filename_mock, clear_log_in_test
from twisterlib.testplan import TestPlan

sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister/twisterlib"))

@mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)
class TestHardwaremap:
    TESTDATA_1 = [
        (
            [
                'ARM',
                'SEGGER',
                'MBED'
            ],
            [
                'DAPLink CMSIS-DAP',
                'MBED CMSIS-DAP'
            ],
            [1234, 'abcd'],
            'pyocd'
        ),
        (
            [
                'STMicroelectronics',
                'Atmel Corp.'
            ],
            [
                'J-Link',
                'J-Link OB'
            ],
            [1234, 'abcd'],
            'jlink'
        ),
        (
            [
                'Silicon Labs',
                'NXP Semiconductors',
                'Microchip Technology Inc.'
            ],
            [
                'STM32 STLink',
                '^XDS110.*',
                'STLINK-V3'
            ],
            [1234, 'abcd'],
            'openocd'
        ),
        (
            [
                'FTDI',
                'Digilent',
                'Microsoft'
            ],
            [
                'TTL232R-3V3',
                'MCP2200 USB Serial Port Emulator'
            ],
            [1234, 'abcd'],
            'dediprog'
        )
    ]
    TESTDATA_2 = [
        (
            'FTDI',
            'DAPLink CMSIS-DAP',
            1234,
            'pyocd'
        )
    ]
    TESTDATA_3 = [
        (
            'Texas Instruments',
            'DAPLink CMSIS-DAP',
            'abcd', 'las'
        ),
        (
            'Texas Instruments',
            'DAPLink CMSIS-DAP',
            'abcd', 'dse0'
        )
    ]

    @classmethod
    def setup_class(cls):
        apath = os.path.join(ZEPHYR_BASE, 'scripts', 'twister')
        cls.loader = importlib.machinery.SourceFileLoader('__main__', apath)
        cls.spec = importlib.util.spec_from_loader(cls.loader.name, cls.loader)
        cls.twister_module = importlib.util.module_from_spec(cls.spec)

    @classmethod
    def teardown_class(cls):
        pass

    @pytest.mark.parametrize(
        ('manufacturer', 'product', 'serial', 'runner'),
        TESTDATA_1,
    )
    def test_generate(self, capfd, out_path, manufacturer, product, serial, runner):
        file_name = "test-map.yaml"
        path = os.path.join(ZEPHYR_BASE, file_name)
        args = ['--outdir', out_path, '--generate-hardware-map', file_name]

        if os.path.exists(path):
            os.remove(path)

        def mocked_comports():
            return [
                mock.Mock(device='/dev/ttyUSB23',
                          manufacturer=id_man,
                          product=id_pro,
                          serial_number=id_serial
                          )
            ]

        for id_man in manufacturer:
            for id_pro in product:
                for id_serial in serial:
                    with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                            mock.patch('serial.tools.list_ports.comports',
                                       side_effect=mocked_comports), \
                            pytest.raises(SystemExit) as sys_exit:
                        self.loader.exec_module(self.twister_module)

                    out, err = capfd.readouterr()
                    sys.stdout.write(out)
                    sys.stderr.write(err)

                    assert os.path.exists(path)

                    expected_data = '- connected: true\n' \
                                    f'  id: {id_serial}\n' \
                                    '  platform: unknown\n' \
                                    f'  product: {id_pro}\n' \
                                    f'  runner: {runner}\n' \
                                    '  serial: /dev/ttyUSB23\n'

                    load_data = open(path).read()
                    assert load_data == expected_data

                    if os.path.exists(path):
                        os.remove(path)

                    assert str(sys_exit.value) == '0'
                    clear_log_in_test()

    @pytest.mark.parametrize(
        ('manufacturer', 'product', 'serial', 'runner'),
        TESTDATA_2,
    )
    def test_few_generate(self, capfd, out_path, manufacturer, product, serial, runner):
        file_name = "test-map.yaml"
        path = os.path.join(ZEPHYR_BASE, file_name)
        args = ['--outdir', out_path, '--generate-hardware-map', file_name]

        if os.path.exists(path):
            os.remove(path)

        def mocked_comports():
            return [
                mock.Mock(device='/dev/ttyUSB23',
                          manufacturer=manufacturer,
                          product=product,
                          serial_number=serial
                          ),
                mock.Mock(device='/dev/ttyUSB24',
                          manufacturer=manufacturer,
                          product=product,
                          serial_number=serial + 1
                          ),
                mock.Mock(device='/dev/ttyUSB24',
                          manufacturer=manufacturer,
                          product=product,
                          serial_number=serial + 2
                          ),
                mock.Mock(device='/dev/ttyUSB25',
                          manufacturer=manufacturer,
                          product=product,
                          serial_number=serial + 3
                          )
            ]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                mock.patch('serial.tools.list_ports.comports',
                           side_effect=mocked_comports), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        assert os.path.exists(path)

        expected_data = '- connected: true\n' \
                        f'  id: {serial}\n' \
                        '  platform: unknown\n' \
                        f'  product: {product}\n' \
                        f'  runner: {runner}\n' \
                        '  serial: /dev/ttyUSB23\n' \
                        '- connected: true\n' \
                        f'  id: {serial + 1}\n' \
                        '  platform: unknown\n' \
                        f'  product: {product}\n' \
                        f'  runner: {runner}\n' \
                        '  serial: /dev/ttyUSB24\n' \
                        '- connected: true\n' \
                        f'  id: {serial + 2}\n' \
                        '  platform: unknown\n' \
                        f'  product: {product}\n' \
                        f'  runner: {runner}\n' \
                        '  serial: /dev/ttyUSB24\n' \
                        '- connected: true\n' \
                        f'  id: {serial + 3}\n' \
                        '  platform: unknown\n' \
                        f'  product: {product}\n' \
                        f'  runner: {runner}\n' \
                        '  serial: /dev/ttyUSB25\n'

        load_data = open(path).read()
        assert load_data == expected_data

        if os.path.exists(path):
            os.remove(path)

        assert str(sys_exit.value) == '0'

    @pytest.mark.parametrize(
        ('manufacturer', 'product', 'serial', 'location'),
        TESTDATA_3,
    )
    def test_texas_exeption(self, capfd, out_path, manufacturer, product, serial, location):
        file_name = "test-map.yaml"
        path = os.path.join(ZEPHYR_BASE, file_name)
        args = ['--outdir', out_path, '--generate-hardware-map', file_name]

        if os.path.exists(path):
            os.remove(path)

        def mocked_comports():
            return [
                mock.Mock(device='/dev/ttyUSB23',
                          manufacturer=manufacturer,
                          product=product,
                          serial_number=serial,
                          location=location
                          )
            ]

        with mock.patch.object(sys, 'argv', [sys.argv[0]] + args), \
                mock.patch('serial.tools.list_ports.comports',
                           side_effect=mocked_comports), \
                pytest.raises(SystemExit) as sys_exit:
            self.loader.exec_module(self.twister_module)

        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        assert os.path.exists(path)

        expected_data = '- connected: true\n' \
                        f'  id: {serial}\n' \
                        '  platform: unknown\n' \
                        f'  product: {product}\n' \
                        '  runner: pyocd\n' \
                        '  serial: /dev/ttyUSB23\n'
        expected_data2 = '[]\n'

        load_data = open(path).read()
        if location.endswith('0'):
            assert load_data == expected_data
        else:
            assert load_data == expected_data2
        if os.path.exists(path):
            os.remove(path)

        assert str(sys_exit.value) == '0'
