
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import pytest
import shlex
import subprocess

from typing import List, Union

logger = logging.getLogger(__name__)


class BabbleSim:

    def __init__(self, phy_extra_args: Union[List, None] = None):
        if phy_extra_args is None:
            phy_extra_args = []
        self.phy_extra_args: List[str] = phy_extra_args

    def run_simulation(self, device_commands: List[List[str]], phy_command: List[str], execute_timeout: float = 30.0):
        phy_command.extend(self.phy_extra_args)

        commands = device_commands + [phy_command]
        self._log_commands(commands)

        bsim_out_path: str = os.getenv('BSIM_OUT_PATH', '')
        bsim_bin_path: str = os.path.join(bsim_out_path, 'bin')

        processes: List[subprocess.Popen] = []
        for command in commands:
            process = subprocess.Popen(command, cwd=bsim_bin_path)
            processes.append(process)

        for process in processes:
            process.communicate(timeout=execute_timeout)
            assert process.returncode == 0, f'Subprocess returncode: {process.returncode}'

    @staticmethod
    def _log_commands(commands: List[List[str]]):
        command_to_print = ''
        for command in commands[:-1]:
            command_to_print += f'{shlex.join(command)} &\n'
        command_to_print += shlex.join(commands[-1])
        logger.info('Running commands: \n%s', command_to_print)


@pytest.fixture
def board() -> str:
    default_board = 'nrf52_bsim'
    board_name = os.getenv('BOARD', default_board)
    return board_name


@pytest.fixture
def simulation_id(request: pytest.FixtureRequest, board: str) -> str:
    unique_simulation_id = request.node.nodeid
    for character_to_replace in ['/', '\\', '.', '::', ':', '[', ']']:
        unique_simulation_id = unique_simulation_id.replace(character_to_replace, '_')
    unique_simulation_id = f'{unique_simulation_id}_{board}'
    return unique_simulation_id


@pytest.fixture
def babblesim(request: pytest.FixtureRequest) -> BabbleSim:
    phy_extra_args = request.config.getoption('--phy-extra-arg', default=[])
    return BabbleSim(phy_extra_args)


def pytest_addoption(parser):
    parser.addoption('--phy-extra-arg', action='append',
                     help='Pass additional argument to PHY. Option can be applied multiple times.')
