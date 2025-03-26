# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
from pathlib import Path
from unittest.mock import patch

import pytest
from conftest import RC_KERNEL_ELF
from runners.nxp_pedbg import NXPPEDebugProbeConfig, NXPPEDebugProbeRunner

TEST_DEVICE = 'pedbg'
TEST_SPEED = 5000
TEST_SERVER_PORT = 7224
TEST_SOC_NAME = 'S32K148'
TEST_S32DS_PATH_OVERRIDE = None
TEST_PEMICRO_PLUGIN_PATH = Path('eclipse/plugins/com.pemicro.debug.gdbjtag.pne_5.9.5.202412131551')
TEST_TOOL_OPT = ['--test-opt-1', '--test-opt-2']

TEST_S32DS_CMD = 's32ds'
TEST_SERVER_LIN_CMD = (
    Path('eclipse')
    / 'plugins'
    / 'com.pemicro.debug.gdbjtag.pne_5.9.5.202412131551'
    / 'lin'
    / 'pegdbserver_console'
)
TEST_SERVER_WIN_CMD = (
    Path('eclipse')
    / 'plugins'
    / 'com.pemicro.debug.gdbjtag.pne_5.9.5.202412131551'
    / 'win32'
    / 'pegdbserver_console'
)
TEST_ARM_GDB_CMD = (
    Path('S32DS') / 'tools' / 'gdb-arm' / 'arm32-eabi' / 'bin' / 'arm-none-eabi-gdb-py'
)

TEST_S32DS_PYTHON_LIB = Path('S32DS') / 'build_tools' / 'msys32' / 'mingw32' / 'lib' / 'python2.7'
TEST_S32DS_RUNTIME_ENV = {
    'PYTHONPATH': f'{TEST_S32DS_PYTHON_LIB}{os.pathsep}{TEST_S32DS_PYTHON_LIB / "site-packages"}'
}

TEST_ALL_KWARGS = {
    'NXPPEDebugProbeConfig': {
        'dev_id': TEST_DEVICE,
        'server_port': TEST_SERVER_PORT,
        'speed': TEST_SPEED,
    },
    'NXPPEDebugProbeRunner': {
        'soc_name': TEST_SOC_NAME,
        'nxpide_path': TEST_S32DS_PATH_OVERRIDE,
        'pemicro_plugin_path': TEST_PEMICRO_PLUGIN_PATH,
        'tool_opt': TEST_TOOL_OPT,
    },
}

TEST_ALL_PARAMS = [
    # generic
    '--dev-id',
    TEST_DEVICE,
    *[f'--tool-opt={o}' for o in TEST_TOOL_OPT],
    # from runner
    '--nxpide-path',
    TEST_S32DS_PATH_OVERRIDE,
    '--pemicro-plugin-path',
    TEST_PEMICRO_PLUGIN_PATH,
    '--soc-name',
    TEST_SOC_NAME,
    '--server-port',
    TEST_SERVER_PORT,
    '--speed',
    TEST_SPEED,
]

DEBUGSERVER_WIN_ALL_EXPECTED_CALL = [
    str(TEST_SERVER_WIN_CMD),
    '-device=NXP_S32K1xx_S32K148F2M0M11',
    '-startserver',
    '-singlesession',
    f'-serverport={TEST_SERVER_PORT}',
    '-gdbmiport=6224',
    '-interface=OPENSDA',
    f'-speed={TEST_SPEED}',
    '-porD',
    *TEST_TOOL_OPT,
]

DEBUGSERVER_LIN_ALL_EXPECTED_CALL = [
    str(TEST_SERVER_LIN_CMD),
    '-device=NXP_S32K1xx_S32K148F2M0M11',
    '-startserver',
    '-singlesession',
    f'-serverport={TEST_SERVER_PORT}',
    '-gdbmiport=6224',
    '-interface=OPENSDA',
    f'-speed={TEST_SPEED}',
    '-porD',
    *TEST_TOOL_OPT,
]

DEBUG_LIN_ALL_EXPECTED_CALL = {
    'client': [
        str(TEST_ARM_GDB_CMD),
        '-x',
        'TEST_GDB_SCRIPT',
        *TEST_TOOL_OPT,
    ],
    'server': [
        str(TEST_SERVER_LIN_CMD),
        '-device=NXP_S32K1xx_S32K148F2M0M11',
        '-startserver',
        '-singlesession',
        f'-serverport={TEST_SERVER_PORT}',
        '-gdbmiport=6224',
        '-interface=OPENSDA',
        f'-speed={TEST_SPEED}',
        '-porD',
    ],
    'gdb_script': [
        f'target remote localhost:{TEST_SERVER_PORT}',
        'monitor reset',
        f'file {RC_KERNEL_ELF}',
        'load',
    ],
}

DEBUG_WIN_ALL_EXPECTED_CALL = {
    'client': [
        str(TEST_ARM_GDB_CMD),
        '-x',
        'TEST_GDB_SCRIPT',
        *TEST_TOOL_OPT,
    ],
    'server': [
        str(TEST_SERVER_WIN_CMD),
        '-device=NXP_S32K1xx_S32K148F2M0M11',
        '-startserver',
        '-singlesession',
        f'-serverport={TEST_SERVER_PORT}',
        '-gdbmiport=6224',
        '-interface=OPENSDA',
        f'-speed={TEST_SPEED}',
        '-porD',
    ],
    'gdb_script': [
        f'target remote localhost:{TEST_SERVER_PORT}',
        'monitor reset',
        f'file {RC_KERNEL_ELF}',
        'load',
    ],
}

ATTACH_WIN_ALL_EXPECTED_CALL = {
    **DEBUG_WIN_ALL_EXPECTED_CALL,
    'gdb_script': [
        f'target remote localhost:{TEST_SERVER_PORT}',
        'monitor reset',
        f'file {RC_KERNEL_ELF}',
    ],
}

ATTACH_LIN_ALL_EXPECTED_CALL = {
    **DEBUG_LIN_ALL_EXPECTED_CALL,
    'gdb_script': [
        f'target remote localhost:{TEST_SERVER_PORT}',
        'monitor reset',
        f'file {RC_KERNEL_ELF}',
    ],
}


@pytest.fixture
def pedbg(runner_config, tmp_path):
    '''NXPPEDebugProbeRunner from constructor kwargs or command line parameters'''

    def _factory(args):
        # create empty files to ensure kernel binaries exist
        (tmp_path / RC_KERNEL_ELF).touch()
        os.chdir(tmp_path)

        runner_config_patched = fix_up_runner_config(runner_config, tmp_path)

        if isinstance(args, dict):
            probe_cfg = NXPPEDebugProbeConfig(**args['NXPPEDebugProbeConfig'])
            return NXPPEDebugProbeRunner(
                runner_config_patched, probe_cfg, **args['NXPPEDebugProbeRunner']
            )
        elif isinstance(args, list):
            parser = argparse.ArgumentParser(allow_abbrev=False)
            NXPPEDebugProbeRunner.add_parser(parser)
            arg_namespace = parser.parse_args(str(x) for x in args)
            return NXPPEDebugProbeRunner.create(runner_config_patched, arg_namespace)

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


@pytest.mark.parametrize(
    'pedbg_args,expected,osname',
    [
        (TEST_ALL_KWARGS, DEBUGSERVER_WIN_ALL_EXPECTED_CALL, 'Windows'),
        (TEST_ALL_PARAMS, DEBUGSERVER_WIN_ALL_EXPECTED_CALL, 'Windows'),
        (TEST_ALL_KWARGS, DEBUGSERVER_LIN_ALL_EXPECTED_CALL, 'Linux'),
        (TEST_ALL_PARAMS, DEBUGSERVER_LIN_ALL_EXPECTED_CALL, 'Linux'),
    ],
)
@patch('platform.system')
@patch('runners.core.ZephyrBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debugserver(require, check_call, system, pedbg_args, expected, osname, pedbg):
    system.return_value = osname

    runner = pedbg(pedbg_args)
    runner.run('debugserver')

    assert require.called
    check_call.assert_called_once_with(expected)


@pytest.mark.parametrize(
    'pedbg_args,expected,osname',
    [
        (TEST_ALL_KWARGS, DEBUG_WIN_ALL_EXPECTED_CALL, 'Windows'),
        (TEST_ALL_PARAMS, DEBUG_WIN_ALL_EXPECTED_CALL, 'Windows'),
        (TEST_ALL_KWARGS, DEBUG_LIN_ALL_EXPECTED_CALL, 'Linux'),
        (TEST_ALL_PARAMS, DEBUG_LIN_ALL_EXPECTED_CALL, 'Linux'),
    ],
)
@patch.dict(os.environ, TEST_S32DS_RUNTIME_ENV, clear=True)
@patch('platform.system')
@patch('tempfile.TemporaryDirectory')
@patch('runners.core.ZephyrBinaryRunner.popen_ignore_int')
@patch('runners.core.ZephyrBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debug(
    require,
    check_call,
    popen_ignore_int,
    temporary_dir,
    system,
    pedbg_args,
    expected,
    osname,
    pedbg,
    tmp_path,
):
    # mock tempfile.TemporaryDirectory to return `tmp_path` and create gdb init script there
    temporary_dir.return_value.__enter__.return_value = tmp_path
    gdb_script = tmp_path / 'runner.nxp_pedbg'
    expected_client = [
        e.replace('TEST_GDB_SCRIPT', gdb_script.as_posix()) for e in expected['client']
    ]

    system.return_value = osname
    expected_env = TEST_S32DS_RUNTIME_ENV if osname == 'Windows' else None

    runner = pedbg(pedbg_args)
    runner.run('debug')

    assert require.called
    assert gdb_script.read_text().splitlines() == expected['gdb_script']
    popen_ignore_int.assert_called_once_with(expected['server'], env=expected_env)
    check_call.assert_called_once_with(expected_client, env=expected_env)


@pytest.mark.parametrize(
    'pedbg_args,expected,osname',
    [
        (TEST_ALL_KWARGS, ATTACH_WIN_ALL_EXPECTED_CALL, 'Windows'),
        (TEST_ALL_PARAMS, ATTACH_WIN_ALL_EXPECTED_CALL, 'Windows'),
        (TEST_ALL_KWARGS, ATTACH_LIN_ALL_EXPECTED_CALL, 'Linux'),
        (TEST_ALL_PARAMS, ATTACH_LIN_ALL_EXPECTED_CALL, 'Linux'),
    ],
)
@patch.dict(os.environ, TEST_S32DS_RUNTIME_ENV, clear=True)
@patch('platform.system')
@patch('tempfile.TemporaryDirectory')
@patch('runners.core.ZephyrBinaryRunner.popen_ignore_int')
@patch('runners.core.ZephyrBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_attach(
    require,
    check_call,
    popen_ignore_int,
    temporary_dir,
    system,
    pedbg_args,
    expected,
    osname,
    pedbg,
    tmp_path,
):
    # mock tempfile.TemporaryDirectory to return `tmp_path` and create gdb init script there
    temporary_dir.return_value.__enter__.return_value = tmp_path
    gdb_script = tmp_path / 'runner.nxp_pedbg'
    expected_client = [
        e.replace('TEST_GDB_SCRIPT', gdb_script.as_posix()) for e in expected['client']
    ]

    system.return_value = osname
    expected_env = TEST_S32DS_RUNTIME_ENV if osname == 'Windows' else None

    runner = pedbg(pedbg_args)
    runner.run('attach')

    assert require.called
    assert gdb_script.read_text().splitlines() == expected['gdb_script']
    popen_ignore_int.assert_called_once_with(expected['server'], env=expected_env)
    check_call.assert_called_once_with(expected_client, env=expected_env)
