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
