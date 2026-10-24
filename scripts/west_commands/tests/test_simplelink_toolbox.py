# Copyright (c) 2026 Conclusive Engineering Sp. z o.o.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import subprocess
from unittest.mock import Mock

import pytest

from runners.simplelink_toolbox import SimpleLinkBinaryRunner


def test_parser_uses_common_gdb_init_option(runner_config):
    parser = argparse.ArgumentParser()
    SimpleLinkBinaryRunner.add_parser(parser)
    args = parser.parse_args(['--gdb-init', 'monitor halt'])
    runner = SimpleLinkBinaryRunner.create(runner_config, args)

    assert runner.gdb_init == ['monitor halt']


@pytest.mark.parametrize('returncode', [0, 1])
def test_reset_without_openocd_or_elftools(runner_config, monkeypatch, returncode):
    monkeypatch.setattr('runners.simplelink_toolbox.ELFFile', None)
    runner = SimpleLinkBinaryRunner(runner_config, simplelink_tool='simplelink-wifi-toolbox')
    require = Mock()
    check_call = Mock()
    monkeypatch.setattr(runner, 'require', require)
    monkeypatch.setattr(runner, 'check_call', check_call)

    if returncode:
        check_call.side_effect = subprocess.CalledProcessError(returncode, 'device_reset')
        with pytest.raises(subprocess.CalledProcessError):
            runner.run('reset')
    else:
        runner.run('reset')

    require.assert_called_once_with('simplelink-wifi-toolbox')
    check_call.assert_called_once_with([
        'simplelink-wifi-toolbox',
        'communicator', '-i', 'XDS110', '-param1', 'auto',
        'general', 'device_reset',
    ])
