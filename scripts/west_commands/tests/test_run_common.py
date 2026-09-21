# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

import run_common


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
