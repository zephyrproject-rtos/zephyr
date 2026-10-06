# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import subprocess
import sys
from pathlib import Path
from unittest.mock import patch

import pytest
import yaml

ZEPHYR_BASE = Path(__file__).resolve().parents[3]


@pytest.fixture
def compliance(monkeypatch):
    monkeypatch.syspath_prepend(str(ZEPHYR_BASE / 'scripts' / 'ci'))
    monkeypatch.delenv('EXTRA_ZEPHYR_MODULES', raising=False)
    monkeypatch.delenv('ZEPHYR_EXTRA_MODULES', raising=False)
    with patch.dict(os.environ):
        import check_compliance

        monkeypatch.setattr(check_compliance, 'logger', logging.getLogger(__name__))
        yield check_compliance


@pytest.mark.parametrize(
    ('method', 'filename', 'symbol'),
    [
        ('check_redefined_board_kconfigs', 'Kconfig', 'BOARD_TEST'),
        ('check_defining_kconfigs_in_board_defconfig_files', 'Kconfig.defconfig', 'TEST_FEATURE'),
    ],
)
@pytest.mark.parametrize(
    'layout', ['in_tree', 'module_board', 'module_soc', 'module_board_and_soc']
)
@pytest.mark.parametrize('typed', [False, True], ids=['valid', 'invalid'])
@pytest.mark.parametrize('cwd_settings', [False, True], ids=['clean_cwd', 'unrelated_settings'])
def test_board_checks_module_roots(
    compliance, monkeypatch, tmp_path, method, filename, symbol, layout, typed, cwd_settings
):
    tree = tmp_path / 'zephyr'
    (tree / 'scripts').mkdir(parents=True)
    (tree / 'scripts' / 'kconfig').symlink_to(ZEPHYR_BASE / 'scripts' / 'kconfig')
    (tree / 'Kconfig.constants').write_text('')
    monkeypatch.setattr(compliance, 'ZEPHYR_BASE', tree)
    monkeypatch.setenv('ZEPHYR_BASE', str(tree))
    monkeypatch.chdir(tree)

    module = tmp_path / 'test module'
    board_root = module if layout in ('module_board', 'module_board_and_soc') else tree
    soc_root = module if layout in ('module_soc', 'module_board_and_soc') else tree
    soc_dir = soc_root / 'soc' / 'test'
    soc_dir.mkdir(parents=True)
    (soc_dir / 'soc.yml').write_text(yaml.safe_dump({'socs': [{'name': 'test_soc'}]}))

    board_dir = board_root / 'boards' / 'test'
    board_dir.mkdir(parents=True)
    (board_dir / 'board.yml').write_text(
        yaml.safe_dump(
            {
                'board': {
                    'name': 'test',
                    'full_name': 'Test board',
                    'vendor': 'zephyr',
                    'socs': [{'name': 'test_soc'}],
                }
            }
        )
    )
    (board_dir / 'Kconfig.test').write_text('')
    definition = '\tbool\n' if typed else '\tdefault y\n'
    (board_dir / filename).write_text(f'config {symbol}\n{definition}')

    if cwd_settings:
        unrelated = tmp_path / 'unrelated'
        stale_board = unrelated / 'boards' / 'stale'
        stale_board.mkdir(parents=True)
        (stale_board / 'board.yml').write_text(
            yaml.safe_dump(
                {
                    'board': {
                        'name': 'stale',
                        'full_name': 'Stale board',
                        'vendor': 'zephyr',
                        'socs': [{'name': 'unavailable_soc'}],
                    }
                }
            )
        )
        (tree / 'settings_file.txt').write_text(
            f'"BOARD_ROOT":"{unrelated}"\n"SOC_ROOT":"{unrelated}"\n'
        )

    if layout == 'in_tree':
        check = compliance.KconfigBasicNoModulesCheck()
    else:
        (module / 'zephyr').mkdir(parents=True)
        (module / 'zephyr' / 'module.yml').write_text(
            yaml.safe_dump(
                {'name': 'test', 'build': {'settings': {'board_root': '.', 'soc_root': '.'}}}
            )
        )
        check = compliance.KconfigCheck()

        def get_modules(module_dirs, modules, sysbuild, settings):
            # Limit discovery to this fixture, independent of the west workspace.
            subprocess.run(
                [
                    sys.executable,
                    str(ZEPHYR_BASE / 'scripts' / 'zephyr_module.py'),
                    '--modules',
                    str(module),
                    '--kconfig-out',
                    modules,
                    '--sysbuild-kconfig-out',
                    sysbuild,
                    '--settings-out',
                    settings,
                ],
                check=True,
                capture_output=True,
                text=True,
            )

        monkeypatch.setattr(check, 'get_modules', get_modules)

    getattr(check, method)()

    assert len(check.fmtd_failures) == int(typed)
    if typed:
        assert f'CONFIG_{symbol}' in check.fmtd_failures[0].text
