# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
from pathlib import Path
from unittest.mock import patch

import pytest
from conftest import RC_KERNEL_ELF
from runners.nxp_s32dbg import NXPS32DebugProbeConfig, NXPS32DebugProbeRunner

TEST_DEVICE = 's32dbg'
TEST_SPEED = 16000
TEST_SERVER_PORT = 45000
TEST_REMOTE_TIMEOUT = 30
TEST_CORE_NAME = 'R52_0_0'
TEST_SOC_NAME = 'S32Z270'
TEST_SOC_FAMILY_NAME = 's32e2z2'
TEST_START_ALL_CORES = True
TEST_S32DS_PATH_OVERRIDE = None
TEST_TOOL_OPT = ['--test-opt-1', '--test-opt-2']
TEST_RESET_TYPE = 'default'
TEST_RESET_DELAY = 0

TEST_S32DS_CMD = 's32ds'
TEST_SERVER_CMD = Path('S32DS') / 'tools' / 'S32Debugger' / 'Debugger' / 'Server' / 'gta' / 'gta'
TEST_ARM_GDB_CMD = Path('S32DS') / 'tools' / 'gdb-arm' / 'arm32-eabi' / 'bin' / 'arm-none-eabi-gdb-py'

TEST_S32DS_PYTHON_LIB = Path('S32DS') / 'build_tools' / 'msys32' / 'mingw32' / 'lib' / 'python2.7'
TEST_S32DS_RUNTIME_ENV = {
    'PYTHONPATH': f'{TEST_S32DS_PYTHON_LIB}{os.pathsep}{TEST_S32DS_PYTHON_LIB / "site-packages"}'
}

TEST_ALL_KWARGS = {
    'NXPS32DebugProbeConfig': {
        'conn_str': TEST_DEVICE,
        'server_port': TEST_SERVER_PORT,
        'speed': TEST_SPEED,
        'remote_timeout': TEST_REMOTE_TIMEOUT,
    },
    'NXPS32DebugProbeRunner': {
        'core_name': TEST_CORE_NAME,
        'soc_name': TEST_SOC_NAME,
        'soc_family_name': TEST_SOC_FAMILY_NAME,
        'start_all_cores': TEST_START_ALL_CORES,
        's32ds_path': TEST_S32DS_PATH_OVERRIDE,
        'tool_opt': TEST_TOOL_OPT
    }
}

TEST_ALL_PARAMS = [
    # generic
    '--dev-id', TEST_DEVICE,
    *[f'--tool-opt={o}' for o in TEST_TOOL_OPT],
    # from runner
    '--s32ds-path', TEST_S32DS_PATH_OVERRIDE,
    '--core-name', TEST_CORE_NAME,
    '--soc-name', TEST_SOC_NAME,
    '--soc-family-name', TEST_SOC_FAMILY_NAME,
    '--server-port', TEST_SERVER_PORT,
    '--speed', TEST_SPEED,
    '--remote-timeout', TEST_REMOTE_TIMEOUT,
    '--start-all-cores',
]

TEST_ALL_S32DBG_PY_VARS = [
    f'py _PROBE_IP = {repr(TEST_DEVICE)}',
    f'py _JTAG_SPEED = {repr(TEST_SPEED)}',
    f'py _GDB_SERVER_PORT = {repr(TEST_SERVER_PORT)}',
    f"py _RESET_TYPE = {repr(TEST_RESET_TYPE)}",
    f'py _RESET_DELAY = {repr(TEST_RESET_DELAY)}',
    f'py _REMOTE_TIMEOUT = {repr(TEST_REMOTE_TIMEOUT)}',
    f'py _CORE_NAME = {repr(f"{TEST_SOC_NAME}_{TEST_CORE_NAME}")}',
    f'py _SOC_NAME = {repr(TEST_SOC_NAME)}',
    'py _IS_LOGGING_ENABLED = False',
    'py _FLASH_NAME = None',
    'py _SECURE_TYPE = None',
    'py _SECURE_KEY = None',
]

DEBUGSERVER_ALL_EXPECTED_CALL = [
    str(TEST_SERVER_CMD),
    '-p', str(TEST_SERVER_PORT),
    *TEST_TOOL_OPT,
]

DEBUG_ALL_EXPECTED_CALL = {
    'client': [
        str(TEST_ARM_GDB_CMD),
        '-x', 'TEST_GDB_SCRIPT',
        *TEST_TOOL_OPT,
    ],
    'server': [
        str(TEST_SERVER_CMD),
        '-p', str(TEST_SERVER_PORT),
    ],
    'gdb_script': [
        *TEST_ALL_S32DBG_PY_VARS,
        f'source generic_bareboard{"_all_cores" if TEST_START_ALL_CORES else ""}.py',
        'py board_init()',
        'py core_init()',
        f'file {RC_KERNEL_ELF}',
        'load',
    ]
}

ATTACH_ALL_EXPECTED_CALL = {
    **DEBUG_ALL_EXPECTED_CALL,
    'gdb_script': [
        *TEST_ALL_S32DBG_PY_VARS,
        f'source attach.py',
        'py core_init()',
        f'file {RC_KERNEL_ELF}',
    ]
}


@pytest.fixture
def s32dbg(runner_config, tmp_path):
    '''NXPS32DebugProbeRunner from constructor kwargs or command line parameters'''
    def _factory(args):
        # create empty files to ensure kernel binaries exist
        (tmp_path / RC_KERNEL_ELF).touch()
        os.chdir(tmp_path)

        runner_config_patched = fix_up_runner_config(runner_config, tmp_path)

        if isinstance(args, dict):
            probe_cfg = NXPS32DebugProbeConfig(**args['NXPS32DebugProbeConfig'])
            return NXPS32DebugProbeRunner(runner_config_patched, probe_cfg,
                                          **args['NXPS32DebugProbeRunner'])
        elif isinstance(args, list):
            parser = argparse.ArgumentParser(allow_abbrev=False)
            NXPS32DebugProbeRunner.add_parser(parser)
            arg_namespace = parser.parse_args(str(x) for x in args)
            return NXPS32DebugProbeRunner.create(runner_config_patched, arg_namespace)
    return _factory


def fix_up_runner_config(runner_config, tmp_path):
    to_replace = {}

    zephyr = tmp_path / 'zephyr'
    zephyr.mkdir()
    dotconfig = zephyr / '.config'
    dotconfig.write_text('CONFIG_ARCH="arm"')
    to_replace['build_dir'] = tmp_path

    return runner_config._replace(**to_replace)


def require_patch(program, path=None):
    assert Path(program).stem == TEST_S32DS_CMD
    return program


def s32dbg_get_script(name):
    return Path(f'{name}.py')


@pytest.mark.parametrize('s32dbg_args,expected,osname', [
    (TEST_ALL_KWARGS, DEBUGSERVER_ALL_EXPECTED_CALL, 'Windows'),
    (TEST_ALL_PARAMS, DEBUGSERVER_ALL_EXPECTED_CALL, 'Windows'),
    (TEST_ALL_KWARGS, DEBUGSERVER_ALL_EXPECTED_CALL, 'Linux'),
    (TEST_ALL_PARAMS, DEBUGSERVER_ALL_EXPECTED_CALL, 'Linux'),
])
@patch('platform.system')
@patch('runners.core.ZephyrBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debugserver(require, check_call, system,
                     s32dbg_args, expected, osname, s32dbg):
    system.return_value = osname

    runner = s32dbg(s32dbg_args)
    runner.run('debugserver')

    assert require.called
    check_call.assert_called_once_with(expected)


@pytest.mark.parametrize('s32dbg_args,expected,osname', [
    (TEST_ALL_KWARGS, DEBUG_ALL_EXPECTED_CALL, 'Windows'),
    (TEST_ALL_PARAMS, DEBUG_ALL_EXPECTED_CALL, 'Windows'),
    (TEST_ALL_KWARGS, DEBUG_ALL_EXPECTED_CALL, 'Linux'),
    (TEST_ALL_PARAMS, DEBUG_ALL_EXPECTED_CALL, 'Linux'),
])
@patch.dict(os.environ, TEST_S32DS_RUNTIME_ENV, clear=True)
@patch('platform.system')
@patch('tempfile.TemporaryDirectory')
@patch('runners.nxp_s32dbg.NXPS32DebugProbeRunner.get_script', side_effect=s32dbg_get_script)
@patch('runners.core.ZephyrBinaryRunner.popen_ignore_int')
@patch('runners.core.ZephyrBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debug(require, check_call, popen_ignore_int, get_script, temporary_dir, system,
               s32dbg_args, expected, osname, s32dbg, tmp_path):

    # mock tempfile.TemporaryDirectory to return `tmp_path` and create gdb init script there
    temporary_dir.return_value.__enter__.return_value = tmp_path
    gdb_script = tmp_path / 'runner.nxp_s32dbg'
    expected_client = [e.replace('TEST_GDB_SCRIPT', gdb_script.as_posix())
                       for e in expected['client']]

    system.return_value = osname
    expected_env = TEST_S32DS_RUNTIME_ENV if osname == 'Windows' else None

    runner = s32dbg(s32dbg_args)
    runner.run('debug')

    assert require.called
    assert gdb_script.read_text().splitlines() == expected['gdb_script']
    popen_ignore_int.assert_called_once_with(expected['server'], env=expected_env)
    check_call.assert_called_once_with(expected_client, env=expected_env)


@pytest.mark.parametrize('s32dbg_args,expected,osname', [
    (TEST_ALL_KWARGS, ATTACH_ALL_EXPECTED_CALL, 'Windows'),
    (TEST_ALL_PARAMS, ATTACH_ALL_EXPECTED_CALL, 'Windows'),
    (TEST_ALL_KWARGS, ATTACH_ALL_EXPECTED_CALL, 'Linux'),
    (TEST_ALL_PARAMS, ATTACH_ALL_EXPECTED_CALL, 'Linux'),
])
@patch.dict(os.environ, TEST_S32DS_RUNTIME_ENV, clear=True)
@patch('platform.system')
@patch('tempfile.TemporaryDirectory')
@patch('runners.nxp_s32dbg.NXPS32DebugProbeRunner.get_script', side_effect=s32dbg_get_script)
@patch('runners.core.ZephyrBinaryRunner.popen_ignore_int')
@patch('runners.core.ZephyrBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_attach(require, check_call, popen_ignore_int, get_script, temporary_dir, system,
                s32dbg_args, expected, osname, s32dbg, tmp_path):

    # mock tempfile.TemporaryDirectory to return `tmp_path` and create gdb init script there
    temporary_dir.return_value.__enter__.return_value = tmp_path
    gdb_script = tmp_path / 'runner.nxp_s32dbg'
    expected_client = [e.replace('TEST_GDB_SCRIPT', gdb_script.as_posix())
                       for e in expected['client']]

    system.return_value = osname
    expected_env = TEST_S32DS_RUNTIME_ENV if osname == 'Windows' else None

    runner = s32dbg(s32dbg_args)
    runner.run('attach')

    assert require.called
    assert gdb_script.read_text().splitlines() == expected['gdb_script']
    popen_ignore_int.assert_called_once_with(expected['server'], env=expected_env)
    check_call.assert_called_once_with(expected_client, env=expected_env)
