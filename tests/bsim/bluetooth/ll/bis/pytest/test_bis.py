# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import pytest

VERBOSITY_LEVEL = 2


@pytest.mark.nrf5340bsim_nrf5340_cpunet
@pytest.mark.nrf5340bsim_nrf5340_cpuapp
def test_broadcast_iso(board: str, simulation_id: str, babblesim):
    if board == 'nrf5340bsim_nrf5340_cpuapp':
        app_name = f'bs_{board}_tests_bsim_bluetooth_ll_bis_bluetooth_eatt_bis_nrf5340_cpuapp'
    else:
        app_name = f'bs_{board}_tests_bsim_bluetooth_ll_bis_bluetooth_eatt_bis'

    command_device_0 = [f'./{app_name}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=0', '-testid=receive']
    command_device_1 = [f'./{app_name}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=1', '-testid=broadcast']
    command_phy = ['./bs_2G4_phy_v1', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-D=2', '-sim_length=30e6']

    device_commands = [command_device_0, command_device_1]

    babblesim.run_simulation(device_commands, command_phy)


@pytest.mark.nrf5340bsim_nrf5340_cpunet
def test_broadcast_iso_vs_dp(board: str, simulation_id: str, babblesim):
    app_name_0 = f'bs_{board}_tests_bsim_bluetooth_ll_bis_bluetooth_eatt_bis_vs_dp'
    app_name_1 = f'bs_{board}_tests_bsim_bluetooth_ll_bis_bluetooth_eatt_bis'

    command_device_0 = [f'./{app_name_0}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=0', '-testid=receive_vs_dp']
    command_device_1 = [f'./{app_name_1}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=1', '-testid=broadcast']
    command_phy = ['./bs_2G4_phy_v1', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-D=2', '-sim_length=30e6']

    device_commands = [command_device_0, command_device_1]

    babblesim.run_simulation(device_commands, command_phy)


@pytest.mark.nrf5340bsim_nrf5340_cpunet
def test_broadcast_iso_ticker_expire_info(board: str, simulation_id: str, babblesim):
    app_name = f'bs_{board}_tests_bsim_bluetooth_ll_bis_bluetooth_eatt_bis_ticker_expire_info'

    command_device_0 = [f'./{app_name}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=0', '-testid=receive']
    command_device_1 = [f'./{app_name}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=1', '-testid=broadcast']
    command_phy = ['./bs_2G4_phy_v1', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-D=2', '-sim_length=30e6']

    device_commands = [command_device_0, command_device_1]

    babblesim.run_simulation(device_commands, command_phy)
