# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import textwrap
from unittest import mock

import pytest
from twister_harness.helpers.mcumgr import MCUmgr, MCUmgrException


@pytest.fixture(name='mcumgr')
def fixture_mcumgr() -> MCUmgr:
    return MCUmgr.create_for_serial('SERIAL_PORT')


@mock.patch('twister_harness.helpers.mcumgr.MCUmgr.run_command', return_value='')
def test_if_mcumgr_fixture_generate_proper_command(
    patched_run_command: mock.Mock, mcumgr: MCUmgr
) -> None:
    mcumgr.reset_device()
    patched_run_command.assert_called_with('reset')

    mcumgr.get_image_list()
    patched_run_command.assert_called_with('image list')

    mcumgr.image_upload('/path/to/image', timeout=100)
    patched_run_command.assert_called_with('-t 100 image upload /path/to/image')

    mcumgr.image_upload('/path/to/image', slot=2, timeout=100)
    patched_run_command.assert_called_with('-t 100 image upload /path/to/image -e -n 2')

    mcumgr.image_test(hash='ABCD')
    patched_run_command.assert_called_with('image test ABCD')

    mcumgr.image_confirm(hash='ABCD')
    patched_run_command.assert_called_with('image confirm ABCD')


def test_if_mcumgr_fixture_raises_exception_when_no_hash_to_test(mcumgr: MCUmgr) -> None:
    cmd_output = textwrap.dedent("""
        Images:
        image=0 slot=0
            version: 0.0.0
            bootable: true
            flags: active confirmed
            hash: 1234
        Split status: N/A (0)
    """)
    mcumgr.run_command = mock.Mock(return_value=cmd_output)
    with pytest.raises(MCUmgrException):
        mcumgr.image_test()


def test_if_mcumgr_fixture_parse_image_list(mcumgr: MCUmgr) -> None:
    cmd_output = textwrap.dedent("""
        Images:
        image=0 slot=0
            version: 0.0.0
            bootable: true
            flags: active confirmed
            hash: 0000
        image=0 slot=1
            version: 1.1.1
            bootable: true
            flags:
            hash: 1111
        Split status: N/A (0)
    """)
    mcumgr.run_command = mock.Mock(return_value=cmd_output)
    image_list = mcumgr.get_image_list()
    assert image_list[0].image == 0
    assert image_list[0].slot == 0
    assert image_list[0].version == '0.0.0'
    assert image_list[0].flags == 'active confirmed'
    assert image_list[0].hash == '0000'
    assert image_list[1].image == 0
    assert image_list[1].slot == 1
    assert image_list[1].version == '1.1.1'
    assert image_list[1].flags == ''
    assert image_list[1].hash == '1111'

    # take second hash to test
    mcumgr.image_test()
    mcumgr.run_command.assert_called_with('image test 1111')

    # take first hash to confirm
    mcumgr.image_confirm()
    mcumgr.run_command.assert_called_with('image confirm 1111')
