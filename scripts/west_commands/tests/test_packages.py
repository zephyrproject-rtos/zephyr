# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

'''Tests for which requirement files `west packages pip` collects.'''

import argparse
import sys
from pathlib import Path
from unittest import mock

import pytest

import packages
from packages import Packages, in_venv
from zephyr_ext_common import ZEPHYR_BASE

ZEPHYR_REQUIREMENTS = ZEPHYR_BASE / 'scripts/requirements.txt'


class Died(Exception):
    '''Stands in for what WestCommand.die() does: stop.'''


def module(name, project, requirement_files=None):
    '''A zephyr module as parse_modules() hands them over.'''
    meta = {'name': name}
    if requirement_files is not None:
        meta['package-managers'] = {'pip': {'requirement-files': requirement_files}}
    return mock.Mock(meta=meta, project=project)


@pytest.fixture
def cmd():
    command = Packages()
    for name in ('dbg', 'wrn', 'inf', 'err'):
        setattr(command, name, mock.Mock())
    command.die = mock.Mock(side_effect=Died)
    # WestCommand.manifest is a property that gives up when there is no west
    # workspace to read one from; nothing here needs its contents.
    with mock.patch.object(Packages, 'manifest', mock.Mock(), create=True):
        yield command


def pip_args(**kwargs):
    ns = argparse.Namespace(manager='pip', modules=[], install=False, ignore_venv_check=False)
    for key, value in kwargs.items():
        setattr(ns, key, value)
    return ns


def listed(cmd):
    '''The requirement files the command printed, in order.'''
    printed = cmd.inf.call_args[0][0]
    return [line[len('-r ') :] for line in printed.splitlines()]


def test_in_venv_follows_the_prefixes():
    with mock.patch.object(sys, 'prefix', '/a'), mock.patch.object(sys, 'base_prefix', '/a'):
        assert in_venv() is False
    with mock.patch.object(sys, 'prefix', '/a/venv'), mock.patch.object(sys, 'base_prefix', '/a'):
        assert in_venv() is True


def test_an_argument_before_the_separator_is_refused(cmd):
    with pytest.raises(Died):
        cmd.do_run(pip_args(), ['--dry-run'])

    assert 'should be passed after "--"' in cmd.die.call_args[0][0]


def test_an_unknown_module_is_refused(cmd):
    with (
        mock.patch.object(
            packages.zephyr_module, 'parse_modules', return_value=[module('foo', '/m/foo')]
        ),
        pytest.raises(Died),
    ):
        cmd.do_run(pip_args(modules=['nope']), [])

    assert 'Unknown zephyr module "nope"' in cmd.die.call_args[0][0]


def test_zephyr_is_always_a_known_module(cmd):
    """It has no module.yml of its own, but may be named."""
    with mock.patch.object(packages.zephyr_module, 'parse_modules', return_value=[]):
        cmd.do_run(pip_args(modules=['zephyr']), [])

    assert listed(cmd) == [str(ZEPHYR_REQUIREMENTS)]


def test_everything_is_listed_when_no_module_is_named(cmd):
    mods = [
        module('foo', '/m/foo', ['requirements.txt']),
        module('bar', '/m/bar', ['a.txt', 'b.txt']),
    ]
    with mock.patch.object(packages.zephyr_module, 'parse_modules', return_value=mods):
        cmd.do_run(pip_args(), [])

    assert listed(cmd) == [
        str(ZEPHYR_REQUIREMENTS),
        str(Path('/m/foo') / 'requirements.txt'),
        str(Path('/m/bar') / 'a.txt'),
        str(Path('/m/bar') / 'b.txt'),
    ]


def test_naming_a_module_leaves_the_others_out(cmd):
    mods = [
        module('foo', '/m/foo', ['requirements.txt']),
        module('bar', '/m/bar', ['a.txt']),
    ]
    with mock.patch.object(packages.zephyr_module, 'parse_modules', return_value=mods):
        cmd.do_run(pip_args(modules=['bar']), [])

    assert listed(cmd) == [str(Path('/m/bar') / 'a.txt')]


def test_a_module_with_nothing_for_pip_is_left_out(cmd):
    mods = [module('foo', '/m/foo'), module('bar', '/m/bar', ['a.txt'])]
    with mock.patch.object(packages.zephyr_module, 'parse_modules', return_value=mods):
        cmd.do_run(pip_args(), [])

    assert listed(cmd) == [str(ZEPHYR_REQUIREMENTS), str(Path('/m/bar') / 'a.txt')]


def test_manager_arguments_are_refused_when_only_listing(cmd):
    """They are for the install command line, and there is not one."""
    with (
        mock.patch.object(packages.zephyr_module, 'parse_modules', return_value=[]),
        pytest.raises(Died),
    ):
        cmd.do_run(pip_args(), ['--', '--dry-run'])

    assert 'does not support unknown arguments' in cmd.die.call_args[0][0]


def test_installing_outside_a_virtual_environment_is_refused(cmd):
    with (
        mock.patch.object(packages.zephyr_module, 'parse_modules', return_value=[]),
        mock.patch.object(packages, 'in_venv', return_value=False),
        pytest.raises(Died),
    ):
        cmd.do_run(pip_args(install=True), [])

    assert 'outside of a virtual environment' in cmd.die.call_args[0][0]


def test_the_venv_check_can_be_waived(cmd):
    """With the check waived it goes on to build the pip command line.

    Nothing here runs pip: os.execv would replace this process, and
    check_call would install into the environment the tests run in.
    """
    with (
        mock.patch.object(packages.zephyr_module, 'parse_modules', return_value=[]),
        mock.patch.object(packages, 'in_venv', return_value=False),
        mock.patch.object(packages.os, 'execv') as execv,
        mock.patch.object(packages.subprocess, 'check_call') as check_call,
    ):
        cmd.do_run(pip_args(install=True, ignore_venv_check=True, modules=['zephyr']), [])

    assert cmd.die.call_count == 0
    # execv is mocked here, so on a POSIX host the code carries on into
    # the Windows branch too; either call carries the same command line.
    ran = execv.call_args or check_call.call_args
    assert ran is not None
    cmdline = ran[0][-1]
    assert cmdline[:4] == [sys.executable, '-m', 'pip', 'install']
    assert str(ZEPHYR_REQUIREMENTS) in cmdline


def test_the_manager_arguments_go_on_the_install_command_line(cmd):
    with (
        mock.patch.object(packages.zephyr_module, 'parse_modules', return_value=[]),
        mock.patch.object(packages, 'in_venv', return_value=True),
        mock.patch.object(packages.os, 'execv') as execv,
        mock.patch.object(packages.subprocess, 'check_call') as check_call,
    ):
        cmd.do_run(pip_args(install=True, modules=['zephyr']), ['--', '--dry-run'])

    ran = execv.call_args or check_call.call_args
    cmdline = ran[0][-1]
    assert cmdline[-1] == '--dry-run'
    # The separator marks where the manager's own arguments start; it is not
    # one of them.
    assert '--' not in cmdline


def test_nothing_to_install(cmd):
    mods = [module('foo', '/m/foo')]
    with (
        mock.patch.object(packages.zephyr_module, 'parse_modules', return_value=mods),
        mock.patch.object(packages, 'in_venv', return_value=True),
        # install=True reaches the install branch if the early return ever
        # stops happening, and that branch runs pip for real.
        mock.patch.object(packages.os, 'execv') as execv,
        mock.patch.object(packages.subprocess, 'check_call') as check_call,
    ):
        cmd.do_run(pip_args(install=True, modules=['foo']), [])

    cmd.inf.assert_called_once_with('Nothing to install')
    execv.assert_not_called()
    check_call.assert_not_called()


def test_a_posix_install_hands_the_process_over_to_pip(cmd):
    """os.execv replaces the west process, so nothing after it runs.

    side_effect stands in for that: the real call does not return, and a
    plain Mock would let the Windows branch below it run as well.
    """
    with (
        mock.patch.object(packages.zephyr_module, 'parse_modules', return_value=[]),
        mock.patch.object(packages, 'in_venv', return_value=True),
        mock.patch.object(packages.platform, 'system', return_value='Linux'),
        mock.patch.object(packages.os, 'execv', side_effect=SystemExit) as execv,
        mock.patch.object(packages.subprocess, 'check_call') as check_call,
        pytest.raises(SystemExit),
    ):
        cmd.do_run(pip_args(install=True, modules=['zephyr']), [])

    execv.assert_called_once()
    assert execv.call_args[0][0] == sys.executable
    check_call.assert_not_called()
    cmd.wrn.assert_not_called()


def test_a_windows_install_warns_and_runs_pip_as_a_child(cmd):
    """Windows has no working os.execv, so west runs pip and stays alive."""
    with (
        mock.patch.object(packages.zephyr_module, 'parse_modules', return_value=[]),
        mock.patch.object(packages, 'in_venv', return_value=True),
        mock.patch.object(packages.platform, 'system', return_value='Windows'),
        mock.patch.object(packages.os, 'execv') as execv,
        mock.patch.object(packages.subprocess, 'check_call') as check_call,
    ):
        cmd.do_run(pip_args(install=True, modules=['zephyr']), [])

    execv.assert_not_called()
    check_call.assert_called_once()
    assert 'permission errors' in cmd.wrn.call_args[0][0]
