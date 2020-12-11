# Copyright (c) 2018 Foundries.io
# Copyright (c) 2019 Nordic Semiconductor ASA.
# Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import platform
from unittest.mock import patch, call

import pytest

from runners.bossac import BossacBinaryRunner
from conftest import RC_KERNEL_BIN

if platform.system() != 'Linux':
    pytest.skip("skipping Linux-only bossac tests", allow_module_level=True)

TEST_BOSSAC_PORT = 'test-bossac-serial'
TEST_BOSSAC_SPEED = '1200'
TEST_OFFSET = 1234
TEST_FLASH_ADDRESS = 5678
TEST_BOARD_NAME = "my_board"

EXPECTED_COMMANDS = [
    ['stty', '-F', TEST_BOSSAC_PORT, 'raw', 'ispeed', '115200',
     'ospeed', '115200', 'cs8', '-cstopb', 'ignpar', 'eol', '255',
     'eof', '255'],
    ['bossac', '-p', TEST_BOSSAC_PORT, '-R', '-e', '-w', '-v',
     '-b', RC_KERNEL_BIN],
]

EXPECTED_COMMANDS_WITH_SPEED = [
    ['stty', '-F', TEST_BOSSAC_PORT, 'raw', 'ispeed', TEST_BOSSAC_SPEED,
     'ospeed', TEST_BOSSAC_SPEED, 'cs8', '-cstopb', 'ignpar', 'eol', '255',
     'eof', '255'],
    ['bossac', '-p', TEST_BOSSAC_PORT, '-R', '-e', '-w', '-v',
     '-b', RC_KERNEL_BIN],
]

EXPECTED_COMMANDS_WITH_OFFSET = [
    ['stty', '-F', TEST_BOSSAC_PORT, 'raw', 'ispeed', '115200',
     'ospeed', '115200', 'cs8', '-cstopb', 'ignpar', 'eol', '255',
     'eof', '255'],
    ['bossac', '-p', TEST_BOSSAC_PORT, '-R', '-e', '-w', '-v',
     '-b', RC_KERNEL_BIN, '-o', str(TEST_OFFSET)],
]

EXPECTED_COMMANDS_WITH_FLASH_ADDRESS = [
    [
        'stty', '-F', TEST_BOSSAC_PORT, 'raw', 'ispeed', '115200',
        'ospeed', '115200', 'cs8', '-cstopb', 'ignpar', 'eol', '255',
        'eof', '255'
    ],
    [
        'bossac', '-p', TEST_BOSSAC_PORT, '-R', '-e', '-w', '-v',
        '-b', RC_KERNEL_BIN, '-o', str(TEST_FLASH_ADDRESS),
    ],
]

EXPECTED_COMMANDS_WITH_EXTENDED = [
    [
        'stty', '-F', TEST_BOSSAC_PORT, 'raw', 'ispeed', '1200',
        'ospeed', '1200', 'cs8', '-cstopb', 'ignpar', 'eol', '255',
        'eof', '255'
    ],
    [
        'bossac', '-p', TEST_BOSSAC_PORT, '-R', '-e', '-w', '-v',
        '-b', RC_KERNEL_BIN, '-o', str(TEST_FLASH_ADDRESS),
    ],
]

BC_DICT_STD = [
	# SAM-BA ROM without offset
	('CONFIG_BOARD', TEST_BOARD_NAME.split()),
	# No code partition Kconfig
	# No zephyr,code-partition (defined on DT)
	('CONFIG_FLASH_LOAD_OFFSET', 0x162e)
]

BC_DICT_COND1 = [
	# SAM-BA ROM/FLASH with offset
	('CONFIG_BOARD', TEST_BOARD_NAME.split()),
	('CONFIG_USE_DT_CODE_PARTITION', 'y'),
	('CONFIG_HAS_FLASH_LOAD_OFFSET', 'y'),
	('CONFIG_FLASH_LOAD_OFFSET', 0x162e)
]

BC_DICT_COND2 = [
	# SAM-BA ROM/FLASH without offset
	('CONFIG_BOARD', TEST_BOARD_NAME.split()),
	# No code partition Kconfig
	('CONFIG_HAS_FLASH_LOAD_OFFSET', 'y'),
	('CONFIG_FLASH_LOAD_OFFSET', 0x162e)
]

BC_DICT_COND3 = [
	# SAM-BA Extended Arduino with offset
	('CONFIG_BOARD', TEST_BOARD_NAME.split()),
	('CONFIG_USE_DT_CODE_PARTITION', 'y'),
	('CONFIG_BOOTLOADER_BOSSA_ARDUINO', 'y'),
	('CONFIG_HAS_FLASH_LOAD_OFFSET', 'y'),
	('CONFIG_FLASH_LOAD_OFFSET', 0x162e)
]

BC_DICT_COND4 = [
	# SAM-BA Extended Adafruit with offset
	('CONFIG_BOARD', TEST_BOARD_NAME.split()),
	('CONFIG_USE_DT_CODE_PARTITION', 'y'),
	('CONFIG_BOOTLOADER_BOSSA_ADAFRUIT_UF2', 'y'),
	('CONFIG_HAS_FLASH_LOAD_OFFSET', 'y'),
	('CONFIG_FLASH_LOAD_OFFSET', 0x162e)
]

BC_DICT_COND5 = [
	# SAM-BA omit offset
	('CONFIG_BOARD', TEST_BOARD_NAME.split()),
	('CONFIG_USE_DT_CODE_PARTITION', 'y'),
	('CONFIG_HAS_FLASH_LOAD_OFFSET', 'y'),
	('CONFIG_FLASH_LOAD_OFFSET', 0x0)
]


def require_patch(program):
    assert program in ['bossac', 'stty']

def bcfg_check_std(item):
    return item in dict(BC_DICT_STD)

def bcfg_get_std(item):
    return dict(BC_DICT_STD)[item]

def bcfg_check_cond1(item):
    return item in dict(BC_DICT_COND1)

def bcfg_get_cond1(item):
    return dict(BC_DICT_COND1)[item]

def bcfg_check_cond2(item):
    return item in dict(BC_DICT_COND2)

def bcfg_get_cond2(item):
    return dict(BC_DICT_COND2)[item]

def bcfg_check_cond3(item):
    return item in dict(BC_DICT_COND3)

def bcfg_get_cond3(item):
    return dict(BC_DICT_COND3)[item]

def bcfg_check_cond4(item):
    return item in dict(BC_DICT_COND4)

def bcfg_get_cond4(item):
    return dict(BC_DICT_COND4)[item]

def bcfg_check_cond5(item):
    return item in dict(BC_DICT_COND5)

def bcfg_get_cond5(item):
    return dict(BC_DICT_COND5)[item]


@patch('runners.bossac.BossacBinaryRunner.supports',
	return_value=False)
@patch('runners.bossac.BossacBinaryRunner.get_chosen_code_partition_node',
	return_value=None)
@patch('runners.core.BuildConfiguration.__getitem__',
	side_effect=bcfg_get_std)
@patch('runners.core.BuildConfiguration.__contains__',
	side_effect=bcfg_check_std)
@patch('runners.core.BuildConfiguration._init')
@patch('runners.core.ZephyrBinaryRunner.require',
	side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_init(cc, req, bcfg_ini, bcfg_check, bcfg_val,
		     get_cod_par, sup, runner_config):
    """
    Test commands using a runner created by constructor.

    Requirements:
	Any SDK

    Configuration:
	ROM bootloader
	CONFIG_USE_DT_CODE_PARTITION=n
	without zephyr,code-partition

    Input:
	none

    Output:
	no --offset
    """
    runner = BossacBinaryRunner(runner_config, port=TEST_BOSSAC_PORT)
    runner.run('flash')
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS]


@patch('runners.bossac.BossacBinaryRunner.supports',
	return_value=False)
@patch('runners.bossac.BossacBinaryRunner.get_chosen_code_partition_node',
	return_value=None)
@patch('runners.core.BuildConfiguration.__getitem__',
	side_effect=bcfg_get_std)
@patch('runners.core.BuildConfiguration.__contains__',
	side_effect=bcfg_check_std)
@patch('runners.core.BuildConfiguration._init')
@patch('runners.core.ZephyrBinaryRunner.require',
	side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_create(cc, req, bcfg_ini, bcfg_check, bcfg_val,
		       get_cod_par, sup, runner_config):
    """
    Test commands using a runner created from command line parameters.

    Requirements:
	Any SDK

    Configuration:
	ROM bootloader
	CONFIG_USE_DT_CODE_PARTITION=n
	without zephyr,code-partition

    Input:
	--bossac-port

    Output:
	no --offset
    """
    args = ['--bossac-port', str(TEST_BOSSAC_PORT)]
    parser = argparse.ArgumentParser()
    BossacBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = BossacBinaryRunner.create(runner_config, arg_namespace)
    runner.run('flash')
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS]


@patch('runners.bossac.BossacBinaryRunner.supports',
	return_value=False)
@patch('runners.bossac.BossacBinaryRunner.get_chosen_code_partition_node',
	return_value=None)
@patch('runners.core.BuildConfiguration.__getitem__',
	side_effect=bcfg_get_std)
@patch('runners.core.BuildConfiguration.__contains__',
	side_effect=bcfg_check_std)
@patch('runners.core.BuildConfiguration._init')
@patch('runners.core.ZephyrBinaryRunner.require',
	side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_create_with_speed(cc, req, bcfg_ini, bcfg_check, bcfg_val,
                                  get_cod_par, sup, runner_config):
    """
    Test commands using a runner created from command line parameters.

    Requirements:
	Any SDK

    Configuration:
	ROM bootloader
	CONFIG_USE_DT_CODE_PARTITION=n
	without zephyr,code-partition

    Input:
	--bossac-port
	--speed

    Output:
	no --offset
    """
    args = ['--bossac-port', str(TEST_BOSSAC_PORT),
            '--speed', str(TEST_BOSSAC_SPEED)]
    parser = argparse.ArgumentParser()
    BossacBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = BossacBinaryRunner.create(runner_config, arg_namespace)
    runner.run('flash')
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS_WITH_SPEED]


@patch('runners.bossac.BossacBinaryRunner.supports',
	return_value=True)
@patch('runners.bossac.BossacBinaryRunner.get_chosen_code_partition_node',
	return_value=True)
@patch('runners.core.BuildConfiguration.__getitem__',
	side_effect=bcfg_get_cond1)
@patch('runners.core.BuildConfiguration.__contains__',
	side_effect=bcfg_check_cond1)
@patch('runners.core.BuildConfiguration._init')
@patch('runners.core.ZephyrBinaryRunner.require',
	side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_create_with_flash_address(cc, req, bcfg_ini, bcfg_check,
					  bcfg_val, get_cod_par, sup,
					  runner_config):
    """
    Test command with offset parameter

    Requirements:
	SDK >= 0.12.0

    Configuration:
	Any bootloader
	CONFIG_USE_DT_CODE_PARTITION=y
	with zephyr,code-partition

    Input:
	--bossac-port

    Output:
	--offset
    """
    args = [
        '--bossac-port',
        str(TEST_BOSSAC_PORT),
    ]
    parser = argparse.ArgumentParser()
    BossacBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = BossacBinaryRunner.create(runner_config, arg_namespace)
    runner.run('flash')
    assert cc.call_args_list == [
        call(x) for x in EXPECTED_COMMANDS_WITH_FLASH_ADDRESS
    ]


@patch('runners.bossac.BossacBinaryRunner.supports',
	return_value=False)
@patch('runners.bossac.BossacBinaryRunner.get_chosen_code_partition_node',
	return_value=True)
@patch('runners.core.BuildConfiguration.__getitem__',
	side_effect=bcfg_get_cond5)
@patch('runners.core.BuildConfiguration.__contains__',
	side_effect=bcfg_check_cond5)
@patch('runners.core.BuildConfiguration._init')
@patch('runners.core.ZephyrBinaryRunner.require',
	side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_create_with_omit_address(cc, req, bcfg_ini, bcfg_check,
					  bcfg_val, get_cod_par, sup,
					  runner_config):
    """
    Test command that will omit offset because CONFIG_FLASH_LOAD_OFFSET is 0.
    This case is valid for ROM bootloaders that define image start at 0 and
    define flash partitions, to use the storage capabilities, for instance.

    Requirements:
	Any SDK

    Configuration:
	ROM bootloader
	CONFIG_USE_DT_CODE_PARTITION=y
	with zephyr,code-partition

    Input:
	--bossac-port

    Output:
	no --offset
    """
    runner = BossacBinaryRunner(runner_config, port=TEST_BOSSAC_PORT)
    runner.run('flash')
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS]


@patch('runners.bossac.BossacBinaryRunner.supports',
	return_value=True)
@patch('runners.bossac.BossacBinaryRunner.get_chosen_code_partition_node',
	return_value=True)
@patch('runners.core.BuildConfiguration.__getitem__',
	side_effect=bcfg_get_cond3)
@patch('runners.core.BuildConfiguration.__contains__',
	side_effect=bcfg_check_cond3)
@patch('runners.core.BuildConfiguration._init')
@patch('runners.core.ZephyrBinaryRunner.require',
	side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_create_with_arduino(cc, req, bcfg_ini, bcfg_check,
				    bcfg_val, get_cod_par, sup,
				    runner_config):
    """
    Test SAM-BA extended protocol with Arduino variation

    Requirements:
	SDK >= 0.12.0

    Configuration:
	Extended bootloader
	CONFIG_USE_DT_CODE_PARTITION=y
	CONFIG_BOOTLOADER_BOSSA_ARDUINO=y
	with zephyr,code-partition

    Input:
	--bossac-port

    Output:
	--offset
    """
    runner = BossacBinaryRunner(runner_config, port=TEST_BOSSAC_PORT)
    runner.run('flash')
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS_WITH_EXTENDED]

@patch('runners.bossac.BossacBinaryRunner.supports',
	return_value=True)
@patch('runners.bossac.BossacBinaryRunner.get_chosen_code_partition_node',
	return_value=True)
@patch('runners.core.BuildConfiguration.__getitem__',
	side_effect=bcfg_get_cond4)
@patch('runners.core.BuildConfiguration.__contains__',
	side_effect=bcfg_check_cond4)
@patch('runners.core.BuildConfiguration._init')
@patch('runners.core.ZephyrBinaryRunner.require',
	side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_create_with_adafruit(cc, req, bcfg_ini, bcfg_check,
				     bcfg_val, get_cod_par, sup,
				     runner_config):
    """
    Test SAM-BA extended protocol with Adafruit UF2 variation

    Requirements:
	SDK >= 0.12.0

    Configuration:
	Extended bootloader
	CONFIG_USE_DT_CODE_PARTITION=y
	CONFIG_BOOTLOADER_BOSSA_ADAFRUIT_UF2=y
	with zephyr,code-partition

    Input:
	--bossac-port

    Output:
	--offset
    """
    runner = BossacBinaryRunner(runner_config, port=TEST_BOSSAC_PORT)
    runner.run('flash')
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS_WITH_EXTENDED]


@patch('runners.bossac.BossacBinaryRunner.supports',
	return_value=False)
@patch('runners.bossac.BossacBinaryRunner.get_chosen_code_partition_node',
	return_value=True)
@patch('runners.core.BuildConfiguration.__getitem__',
	side_effect=bcfg_get_cond1)
@patch('runners.core.BuildConfiguration.__contains__',
	side_effect=bcfg_check_cond1)
@patch('runners.core.BuildConfiguration._init')
@patch('runners.core.ZephyrBinaryRunner.require',
	side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_create_with_oldsdk(cc, req, bcfg_ini, bcfg_check,
				   bcfg_val, get_cod_par, sup,
				   runner_config):
    """
    Test old SDK and ask user to upgrade

    Requirements:
	SDK <= 0.12.0

    Configuration:
	Any bootloader
	CONFIG_USE_DT_CODE_PARTITION=y
	with zephyr,code-partition

    Input:

    Output:
	Abort
    """
    runner = BossacBinaryRunner(runner_config)
    with pytest.raises(RuntimeError) as rinfo:
        runner.run('flash')
    assert str(rinfo.value) == "This version of BOSSA does not support the" \
                               " --offset flag. Please upgrade to a newer" \
                               " Zephyr SDK version >= 0.12.0."


@patch('runners.bossac.BossacBinaryRunner.supports',
	return_value=False)
@patch('runners.bossac.BossacBinaryRunner.get_chosen_code_partition_node',
	return_value=None)
@patch('runners.core.BuildConfiguration.__getitem__',
	side_effect=bcfg_get_cond1)
@patch('runners.core.BuildConfiguration.__contains__',
	side_effect=bcfg_check_cond1)
@patch('runners.core.BuildConfiguration._init')
@patch('runners.core.ZephyrBinaryRunner.require',
	side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_create_error_missing_dt_info(cc, req, bcfg_ini, bcfg_check,
					     bcfg_val, get_cod_par, sup,
					     runner_config):
    """
    Test SAM-BA offset wrong configuration. No chosen code partition.

    Requirements:
	Any SDK

    Configuration:
	Any bootloader
	CONFIG_USE_DT_CODE_PARTITION=y
	with zephyr,code-partition (missing)

    Input:

    Output:
	Abort
    """
    runner = BossacBinaryRunner(runner_config)
    with pytest.raises(RuntimeError) as rinfo:
        runner.run('flash')
    assert str(rinfo.value) == "The device tree zephyr,code-partition" \
                               " chosen node must be defined."


@patch('runners.bossac.BossacBinaryRunner.supports',
	return_value=False)
@patch('runners.bossac.BossacBinaryRunner.get_chosen_code_partition_node',
	return_value=True)
@patch('runners.core.BuildConfiguration.__getitem__',
	side_effect=bcfg_get_cond2)
@patch('runners.core.BuildConfiguration.__contains__',
	side_effect=bcfg_check_cond2)
@patch('runners.core.BuildConfiguration._init')
@patch('runners.core.ZephyrBinaryRunner.require',
	side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_create_error_missing_kconfig(cc, req, bcfg_ini, bcfg_check,
					     bcfg_val, get_cod_par, sup,
					     runner_config):
    """
    Test SAM-BA offset wrong configuration. No CONFIG_USE_DT_CODE_PARTITION
    Kconfig definition.

    Requirements:
	Any SDK

    Configuration:
	Any bootloader
	CONFIG_USE_DT_CODE_PARTITION=y (missing)
	with zephyr,code-partition

    Input:

    Output:
	Abort
    """
    runner = BossacBinaryRunner(runner_config)
    with pytest.raises(RuntimeError) as rinfo:
        runner.run('flash')
    assert str(rinfo.value) == \
        "There is no CONFIG_USE_DT_CODE_PARTITION Kconfig defined at " \
        + TEST_BOARD_NAME + "_defconfig file.\n This means that" \
        " zephyr,code-partition device tree node should not be defined." \
        " Check Zephyr SAM-BA documentation."
