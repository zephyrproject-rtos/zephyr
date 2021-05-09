# Copyright (c) 2020 Vestas Wind Systems A/S
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
from unittest.mock import patch, call

import pytest

from runners.canopen_program import CANopenBinaryRunner
from conftest import RC_KERNEL_BIN

#
# Test values
#

TEST_DEF_CONTEXT = 'default'
TEST_ALT_CONTEXT = 'alternate'

#
# Test cases
#

TEST_CASES = [(n, x, p, c, o, t)
              for n in range(1, 3)
              for x in (None, TEST_ALT_CONTEXT)
              for p in range(1, 3)
              for c in (False, True)
              for o in (False, True)
              for t in range(1, 3)]

def os_path_isfile_patch(filename):
    if filename == RC_KERNEL_BIN:
        return True
    return os.path.isfile(filename)

@pytest.mark.parametrize('test_case', TEST_CASES)
@patch('runners.canopen_program.CANopenProgramDownloader')
def test_canopen_program_create(cpd, test_case, runner_config):
    '''Test CANopen runner created from command line parameters.'''
    node_id, context, program_number, confirm, confirm_only, timeout = test_case

    args = ['--node-id', str(node_id)]
    if context is not None:
        args.extend(['--can-context', context])
    if program_number:
        args.extend(['--program-number', str(program_number)])
    if not confirm:
        args.append('--no-confirm')
    if confirm_only:
        args.append('--confirm-only')
    if timeout:
        args.extend(['--timeout', str(timeout)])

    mock = cpd.return_value
    mock.flash_status.return_value = 0
    mock.swid.return_value = 0

    parser = argparse.ArgumentParser()
    CANopenBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = CANopenBinaryRunner.create(runner_config, arg_namespace)
    with patch('os.path.isfile', side_effect=os_path_isfile_patch):
        runner.run('flash')

    cpd.assert_called_once()
    if context:
        assert cpd.call_args == call(node_id=node_id,
                                     can_context=context,
                                     logger=runner.logger,
                                     program_number=program_number)
    else:
        assert cpd.call_args == call(node_id=node_id,
                                     can_context=TEST_DEF_CONTEXT,
                                     logger=runner.logger,
                                     program_number=program_number)

    mock.connect.assert_called_once()

    if confirm_only:
        mock.flash_status.assert_called_once()
        mock.swid.assert_called_once()
        mock.enter_pre_operational.assert_called_once()
        mock.zephyr_confirm_program.assert_called_once()
        mock.clear_program.assert_not_called()
        mock.stop_program.assert_not_called()
        mock.download.assert_not_called()
        mock.start_program.assert_not_called()
        mock.wait_for_bootup.assert_not_called()
    else:
        mock.enter_pre_operational.assert_called()
        mock.flash_status.assert_called()
        mock.swid.assert_called()
        mock.stop_program.assert_called_once()
        mock.clear_program.assert_called_once()
        mock.download.assert_called_once_with(RC_KERNEL_BIN)
        mock.start_program.assert_called_once()
        mock.wait_for_bootup.assert_called_once_with(timeout)
        if confirm:
            mock.zephyr_confirm_program.assert_called_once()
        else:
            mock.zephyr_confirm_program.assert_not_called()

    mock.disconnect.assert_called_once()
