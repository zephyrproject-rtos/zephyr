# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

EXECUTE_TIMEOUT = 120.0
VERBOSITY_LEVEL = 2


def test_autoconnect(board: str, simulation_id: str, babblesim):
    app_name = f'bs_{board}_tests_bsim_bluetooth_host_att_eatt_bluetooth_eatt_autoconnect'

    command_device_0 = [f'./{app_name}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=0', '-testid=central_autoconnect']
    command_device_1 = [f'./{app_name}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=1', '-testid=peripheral_autoconnect']
    command_phy = ['./bs_2G4_phy_v1', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-D=2', '-sim_length=200e6']

    device_commands = [command_device_0, command_device_1]

    babblesim.run_simulation(device_commands, command_phy, EXECUTE_TIMEOUT)


def test_collision(board: str, simulation_id: str, babblesim):
    app_name = f'bs_{board}_tests_bsim_bluetooth_host_att_eatt_bluetooth_eatt_collision'

    command_device_0 = [f'./{app_name}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=0', '-testid=central']
    command_device_1 = [f'./{app_name}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=1', '-testid=peripheral']
    command_phy = ['./bs_2G4_phy_v1', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-D=2', '-sim_length=200e6']

    device_commands = [command_device_0, command_device_1]

    babblesim.run_simulation(device_commands, command_phy, EXECUTE_TIMEOUT)


def test_lowres(board: str, simulation_id: str, babblesim):
    execute_time = 20.0
    app_name_0 = f'bs_{board}_tests_bsim_bluetooth_host_att_eatt_bluetooth_eatt_autoconnect'
    app_name_1 = f'bs_{board}_tests_bsim_bluetooth_host_att_eatt_bluetooth_eatt_lowers'

    command_device_0 = [f'./{app_name_0}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=0', '-testid=central_lowres']
    command_device_1 = [f'./{app_name_1}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=1', '-testid=peripheral_lowres']
    command_phy = ['./bs_2G4_phy_v1', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-D=2', '-sim_length=200e6']

    device_commands = [command_device_0, command_device_1]

    babblesim.run_simulation(device_commands, command_phy, execute_time)


def test_multiple_conn(board: str, simulation_id: str, babblesim):
    app_name = f'bs_{board}_tests_bsim_bluetooth_host_att_eatt_bluetooth_eatt_multiple_conn'

    command_device_0 = [f'./{app_name}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=0', '-testid=central']
    command_device_1 = [f'./{app_name}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=1', '-testid=peripheral']
    command_phy = ['./bs_2G4_phy_v1', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-D=2', '-sim_length=200e6']

    device_commands = [command_device_0, command_device_1]

    babblesim.run_simulation(device_commands, command_phy, EXECUTE_TIMEOUT)


def test_reconfigure(board: str, simulation_id: str, babblesim):
    app_name = f'bs_{board}_tests_bsim_bluetooth_host_att_eatt_bluetooth_eatt_autoconnect'

    command_device_0 = [f'./{app_name}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=0', '-testid=central_reconfigure']
    command_device_1 = [f'./{app_name}', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-d=1', '-testid=peripheral_reconfigure']
    command_phy = ['./bs_2G4_phy_v1', f'-v={VERBOSITY_LEVEL}', f'-s={simulation_id}', '-D=2', '-sim_length=60e6']

    device_commands = [command_device_0, command_device_1]

    babblesim.run_simulation(device_commands, command_phy, EXECUTE_TIMEOUT)
