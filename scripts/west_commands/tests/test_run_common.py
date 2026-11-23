# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-FileCopyrightText: Copyright 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

import run_common


class FakeCache:
    def __init__(self, soc_dirs, board_dirs):
        self.lists = {'SOC_DIRECTORIES': soc_dirs, 'BOARD_DIRECTORIES': board_dirs}

    def get_list(self, name):
        return self.lists[name]


def gather(soc_dirs, board_dirs):
    check_files = []
    run_common.gather_runner_policy_files(
        FakeCache(soc_dirs, board_dirs), set(), set(), check_files
    )
    return check_files


def test_zephyr_base_abs_path_resolves_relative_cache_directory(monkeypatch, tmp_path):
    monkeypatch.setattr(run_common, 'ZEPHYR_BASE', tmp_path)

    assert (
        run_common.zephyr_base_abs_path(Path('soc/nxp/imxrt'), Path('soc.yml'))
        == tmp_path / 'soc/nxp/imxrt/soc.yml'
    )
    assert (
        run_common.zephyr_base_abs_path(Path('boards/nxp/mimxrt1064_evk'), Path('board.yml'))
        == tmp_path / 'boards/nxp/mimxrt1064_evk/board.yml'
    )


def test_zephyr_base_abs_path_preserves_absolute_cache_directory(monkeypatch, tmp_path):
    monkeypatch.setattr(run_common, 'ZEPHYR_BASE', tmp_path / 'zephyr')
    directory = tmp_path / 'external' / 'soc'

    assert run_common.zephyr_base_abs_path(directory, Path('soc.yml')) == directory / 'soc.yml'


def test_filter_used_cmds_drops_unmatched_boards_and_entries():
    used_cmds = [
        run_common.UsedFlashCommand(
            '--erase', ['a/x', 'b/y', 'nrf5340dk/nrf5340/cpuapp'], ['all'], True
        ),
        run_common.UsedFlashCommand('--erase', ['q/z'], ['all'], True),
        run_common.UsedFlashCommand('--erase', ['r/z'], ['all'], True),
        run_common.UsedFlashCommand('--reset', ['([^/]+)/nrf5340/cpunet'], ['all'], False),
    ]
    board_names = {'nrf5340dk/nrf5340/cpuapp', 'nrf5340dk/nrf5340/cpunet'}

    filtered = run_common.filter_used_cmds(used_cmds, board_names)

    assert [entry.boards for entry in filtered] == [
        ['nrf5340dk/nrf5340/cpuapp'],
        ['([^/]+)/nrf5340/cpunet'],
    ]


def test_do_run_common_single_domain_defines_board_names(monkeypatch):
    # Regression: a single-domain flash skips the `len(domains) > 1` block, so
    # board_names must be initialized beforehand; otherwise filter_used_cmds()
    # raises UnboundLocalError and every flash fails.
    from types import SimpleNamespace

    domain = SimpleNamespace(build_dir='build')
    flashed = []

    monkeypatch.setattr(
        run_common, 'zephyr_module', SimpleNamespace(parse_modules=lambda *a, **k: [])
    )
    monkeypatch.setattr(run_common, 'get_build_dir', lambda *a, **k: 'build')
    monkeypatch.setattr(run_common, 'rebuild', lambda *a, **k: None)
    monkeypatch.setattr(run_common, 'get_domains_to_process', lambda *a, **k: [domain])
    monkeypatch.setattr(run_common, 'forward_logging_to_west', lambda *a, **k: None)
    monkeypatch.setattr(run_common, 'do_run_common_image', lambda *a, **k: flashed.append(a))

    command = SimpleNamespace(
        name='flash',
        manifest=None,
        config=None,
        wrn=lambda *a, **k: None,
        die=lambda *a, **k: None,
    )
    user_args = SimpleNamespace(context=False)

    run_common.do_run_common(command, user_args, [])

    # Reached the per-domain flashing step without raising.
    assert len(flashed) == 1


def test_gathering_deduplicates_spellings_of_one_directory(monkeypatch, tmp_path):
    # Two spellings of one directory must not both be gathered, or each
    # loads the same policy at the same priority and the run is aborted
    # as a duplicate configuration.
    monkeypatch.setattr(run_common, 'ZEPHYR_BASE', tmp_path)

    gathered = gather(
        ['soc/nxp/imxrt', './soc/nxp/imxrt', 'soc/nxp/imxrt/', 'soc/nxp/../nxp/imxrt'], []
    )

    assert [check.filename for check in gathered] == [tmp_path / 'soc/nxp/imxrt/soc.yml']


def test_gathering_deduplicates_symlinked_directory(monkeypatch, tmp_path):
    monkeypatch.setattr(run_common, 'ZEPHYR_BASE', tmp_path)
    (tmp_path / 'soc/nxp/imxrt').mkdir(parents=True)
    (tmp_path / 'soc/link').symlink_to(tmp_path / 'soc/nxp/imxrt', target_is_directory=True)

    gathered = gather(['soc/nxp/imxrt', 'soc/link'], [])

    assert [check.filename for check in gathered] == [tmp_path / 'soc/nxp/imxrt/soc.yml']


def test_gathering_deduplicates_soc_and_board_directories_separately(monkeypatch, tmp_path):
    # SoC and board files carry different run_once priorities, so a
    # directory seen through both cache lists must yield both entries.
    monkeypatch.setattr(run_common, 'ZEPHYR_BASE', tmp_path)
    directory = 'boards/nxp/mimxrt1064_evk'

    gathered = gather([directory], [directory])

    assert [(check.filename, check.board) for check in gathered] == [
        (tmp_path / directory / 'soc.yml', False),
        (tmp_path / directory / 'board.yml', True),
    ]
