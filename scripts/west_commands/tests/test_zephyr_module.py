# Copyright (c) 2025 Thorsten Klein <thorsten.klein@bshg.com>
# Copyright (c) 2025 Bosch Hausgeräte GmbH
#
# SPDX-License-Identifier: Apache-2.0


import os
import textwrap
from pathlib import Path
from typing import Any

import pytest
import yaml
import zephyr_module
from utils import chdir, update_env


def save_module_yml(
    path: str | Path, extra_modules: list[(str, Any)] | None = None, recursive=None
):
    data = {}
    if extra_modules:
        data['extra-modules'] = []
        for extra_module_path, recursive in extra_modules:
            m_path = os.path.relpath(extra_module_path, path)
            module = {'path': m_path}
            if recursive is not None:
                module['recursive'] = recursive
            data['extra-modules'].append(module)

    module_yml_path = Path(path) / 'zephyr' / 'module.yml'
    module_yml_path.parent.mkdir(parents=True, exist_ok=True)
    with open(module_yml_path, 'w') as file:
        yaml.dump(data, file, default_flow_style=False)
    return path, data


def create_west_workspace(topdir: str | Path, manifest: str | Path):
    manifest = Path(manifest)
    manifest.parent.mkdir(parents=True, exist_ok=True)
    manifest.write_text(
        textwrap.dedent('''
    manifest:
    ''')
    )

    west_config_path = Path(topdir) / '.west' / 'config'
    west_config_path.parent.mkdir(parents=True)
    west_config = textwrap.dedent(f'''
    [manifest]
    path = {os.path.relpath(manifest.parent, topdir)}
    file = {manifest.name}
    ''')
    west_config_path.write_text(west_config)
    return manifest


@pytest.fixture
def run_tests_in_tempdir(tmpdir_factory):
    # Fixture to ensure that the tests run in a tempdir
    tmpdir = Path(tmpdir_factory.mktemp("test-configs"))
    os.chdir(tmpdir)
    yield


@pytest.fixture
def setup_workspace_v1(run_tests_in_tempdir):
    '''
    setup a west workspace for a manifest project (west.yml) which contains
    some additional zephyr extra-modules. Those extra-modules are stacked,
    so each module specifies its child module in zephyr/module.yml
    ├── .west
    │   └── config
    ├── base
    │   ├── libA
    │   │   ├── module-1
    │   │   │   └── zephyr
    │   │   │       └── module.yml
    │   │   ├── module-2
    │   │   │   └── zephyr
    │   │   │       └── module.yml
    │   │   └── zephyr
    │   │       └── module.yml
    │   ├── libB
    │   │   └── zephyr
    │   │       └── module.yml
    │   └── zephyr
    │       └── module.yml
    ├── west.yml
    └── zephyr
        └── module.yml
    '''
    base_a_1, _ = save_module_yml(Path('base') / 'libA' / 'module-1', [])
    base_a_2, _ = save_module_yml(Path('base') / 'libA' / 'module-2', [])
    base_a, _ = save_module_yml(Path('base') / 'libA', [(base_a_1, None), (base_a_2, None)])
    base_b, _ = save_module_yml(Path('base') / 'libB', [])
    save_module_yml(Path('base'), [(base_a, None), (base_b, None)])

    # basic west.yml
    create_west_workspace('.', 'west.yml')
    save_module_yml(Path('.'), [])


def test_parse_modules(setup_workspace_v1):
    '''by default no extra-modules are considered if none are specified
    in the module.yml of the manifest project'''
    modules = zephyr_module.parse_modules('zephyr')
    assert len(modules) == 1
    assert modules[0].project == str(Path('.').resolve())


def test_parse_modules_extra_modules(setup_workspace_v1):
    '''consider extra-modules specified in manifest project (zephyr/module.yml)
    but not their child extra-modules by default'''
    save_module_yml(Path('.'), [('base', None)])
    modules = zephyr_module.parse_modules('zephyr')
    assert len(modules) == 2
    assert modules[1].project == Path('base').resolve()


def test_parse_modules_extra_modules_recursive_false(setup_workspace_v1):
    '''consider extra-modules from manifest project (zephyr/module.yml)
    but not their child extra-modules if 'recursive=False' '''
    save_module_yml(Path('.'), [('base', False)])
    modules = zephyr_module.parse_modules('zephyr')
    assert len(modules) == 2
    assert modules[1].project == Path('base').resolve()


def test_parse_modules_extra_modules_recursive_true(setup_workspace_v1):
    '''consider extra-modules from manifest project (zephyr/module.yml)
    and also their child extra-modules if 'recursive=True' '''
    save_module_yml(Path('.'), [('base', True)])
    modules = zephyr_module.parse_modules('zephyr')
    assert len(modules) == 6
    assert modules[1].project == Path('base').resolve()
    assert modules[2].project == (Path('base') / 'libA').resolve()
    assert modules[3].project == (Path('base') / 'libA' / 'module-1').resolve()
    assert modules[4].project == (Path('base') / 'libA' / 'module-2').resolve()
    assert modules[5].project == (Path('base') / 'libB').resolve()


def test_parse_modules_chdir(setup_workspace_v1):
    '''consider extra-modules from manifest project (zephyr/module.yml)
    and also their child extra-modules if 'recursive=True' '''
    save_module_yml(Path('.'), [('base', True)])
    with chdir('base'):
        modules = zephyr_module.parse_modules('zephyr')
    assert len(modules) == 6


def test_parse_modules_env(setup_workspace_v1):
    '''consider extra-modules from manifest project (zephyr/module.yml)
    and also their child extra-modules if 'recursive=True' '''
    save_module_yml(Path('.'), [])
    for env in ['EXTRA_ZEPHYR_MODULES', 'ZEPHYR_EXTRA_MODULES']:
        with update_env({env: 'base'}):
            modules = zephyr_module.parse_modules('zephyr')
        assert len(modules) == 2


def test_parse_modules_extra_modules_recursive_invalid(setup_workspace_v1):
    '''invalid value for 'recursive' should fail'''
    save_module_yml(Path('.'), [('base', 'invalid')])
    with pytest.raises(SystemExit) as e:
        zephyr_module.parse_modules('zephyr')
    assert "Value 'invalid' is not of type 'bool'" in str(e.value)


def test_parse_modules_extra_modules_recursive_recursion(setup_workspace_v1):
    '''recursion should be handled'''
    for path in [".", Path('base') / '..', Path('non-existent') / '..']:
        save_module_yml(Path('.'), [(path, True)])
        modules = zephyr_module.parse_modules('zephyr')
        assert len(modules) == 1


def test_parse_modules_extra_modules_child_recursive_false(setup_workspace_v1):
    '''a child module sets recursive=False'''
    save_module_yml(
        Path('base'), extra_modules=[(Path('base') / 'libA', False), (Path('base') / 'libB', False)]
    )

    # manifest project can override to resolve recursively
    save_module_yml(Path('.'), [('base', True)])
    modules = zephyr_module.parse_modules('zephyr')
    assert len(modules) == 6
