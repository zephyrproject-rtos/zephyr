# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

import run_common


def test_soc_board_config_path_resolves_relative_cache_directory(monkeypatch, tmp_path):
    monkeypatch.setattr(run_common, 'ZEPHYR_BASE', tmp_path)

    assert (
        run_common.soc_board_config_path('soc/nxp/imxrt', 'soc.yml')
        == tmp_path / 'soc/nxp/imxrt/soc.yml'
    )
    assert (
        run_common.soc_board_config_path('boards/nxp/mimxrt1064_evk', 'board.yml')
        == tmp_path / 'boards/nxp/mimxrt1064_evk/board.yml'
    )


def test_soc_board_config_path_preserves_absolute_cache_directory(monkeypatch, tmp_path):
    monkeypatch.setattr(run_common, 'ZEPHYR_BASE', tmp_path / 'zephyr')
    directory = tmp_path / 'external' / 'soc'

    assert run_common.soc_board_config_path(directory, 'soc.yml') == directory / 'soc.yml'
