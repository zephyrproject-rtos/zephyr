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
