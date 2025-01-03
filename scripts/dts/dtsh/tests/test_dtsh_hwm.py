# Copyright (c) 2024 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.hwm module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring


from dtsh.hwm import DTShBoard


def test_board_parse_v2_name() -> None:
    # Full target
    target = "board_name@1.0/soc/cpus/variant"
    (name, revision, soc, cpus, variant) = DTShBoard.v2_parse_board(target)
    assert "board_name" == name
    assert "1.0" == revision
    assert "soc" == soc
    assert "cpus" == cpus
    assert "variant" == variant

    # Full target, SoC omitted (no meta-data otherwise available).
    target = "board_name@1.0//cpus/variant"
    (name, revision, soc, cpus, variant) = DTShBoard.v2_parse_board(target)
    assert "board_name" == name
    assert "1.0" == revision
    assert "" == soc
    assert "cpus" == cpus
    assert "variant" == variant

    # Only board name (no meta-data otherwise available).
    target = "board_name"
    (name, revision, soc, cpus, variant) = DTShBoard.v2_parse_board(target)
    assert "board_name" == name
    assert revision is None
    assert soc is None
    assert cpus is None
    assert variant is None
