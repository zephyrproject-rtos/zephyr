#!/usr/bin/env python3
#
# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0

"""Tests for the aggregator window map printed by dump_isr_intlist.py.

The map is derived from the same CONFIG_ symbols gen_isr_tables.py places the
table with, so it is checked here against gen_isr_config's own placement.
"""

import sys
from types import SimpleNamespace

import pytest
from dump_isr_intlist import describe_windows
from gen_isr_tables import gen_isr_config, gen_isr_log

# Mirrors a qemu_riscv32 build of tests/drivers/interrupt_controller/
# multi_level_backend, with the regions spread out so a misplaced slot cannot
# land on the right index by accident.
L1_BITS = 8
L2_BITS = 9
L3_BITS = 8
L2_BASE = 16
L3_BASE = 144
L2_WINDOW = 64
L3_WINDOW = 32
NUM_IRQS = 208

# CPU lines carrying the two level-2 aggregators.
AGG2_0_LINE = 11
AGG2_1_LINE = 12
# Level-2 local IRQs carrying the two level-3 aggregators.
AGG3_0_L2 = 5
AGG3_1_L2 = 7

SYMS = {
    "CONFIG_MULTI_LEVEL_INTERRUPTS": 1,
    "CONFIG_2ND_LEVEL_INTERRUPTS": 1,
    "CONFIG_3RD_LEVEL_INTERRUPTS": 1,
    "CONFIG_1ST_LEVEL_INTERRUPT_BITS": L1_BITS,
    "CONFIG_2ND_LEVEL_INTERRUPT_BITS": L2_BITS,
    "CONFIG_3RD_LEVEL_INTERRUPT_BITS": L3_BITS,
    "CONFIG_NUM_2ND_LEVEL_AGGREGATORS": 2,
    "CONFIG_NUM_3RD_LEVEL_AGGREGATORS": 2,
    "CONFIG_2ND_LVL_ISR_TBL_OFFSET": L2_BASE,
    "CONFIG_3RD_LVL_ISR_TBL_OFFSET": L3_BASE,
    "CONFIG_2ND_LVL_INTR_00_OFFSET": AGG2_0_LINE,
    "CONFIG_2ND_LVL_INTR_01_OFFSET": AGG2_1_LINE,
    "CONFIG_3RD_LVL_INTR_00_OFFSET": AGG3_0_L2,
    "CONFIG_3RD_LVL_INTR_01_OFFSET": AGG3_1_L2,
    "CONFIG_MAX_IRQ_PER_AGGREGATOR": 0,
    "CONFIG_MAX_IRQ_PER_2ND_LEVEL_AGGREGATOR": L2_WINDOW,
    "CONFIG_MAX_IRQ_PER_3RD_LEVEL_AGGREGATOR": L3_WINDOW,
}


def l2(line, local):
    """Encode a level-2 IRQ: local IRQ @local of the aggregator on CPU line @line."""
    return ((local + 1) << L1_BITS) | line


def l3(line, l2_local, local):
    """Encode a level-3 IRQ: local IRQ @local behind level-2 IRQ @l2_local."""
    return ((local + 1) << (L1_BITS + L2_BITS)) | l2(line, l2_local)


def index(irq):
    """The _sw_isr_table index gen_isr_tables.py places @irq at."""
    args = SimpleNamespace(sw_isr_table=True, vector_table=False, big_endian=False)
    return gen_isr_config(args, dict(SYMS), gen_isr_log()).get_swt_table_index(0, irq)


class TestIntlistDump:
    """The window map dump_isr_intlist.py prints on every multi-level build."""

    def test_windows_follow_the_per_level_sizes(self):
        out = describe_windows(dict(SYMS), NUM_IRQS, 0, [])

        assert f"level 1:  slots    0 .. {L2_BASE - 1}" in out
        assert any(
            x.startswith(
                f"level 2:  slots {L2_BASE + L2_WINDOW:>4} .. {L2_BASE + 2 * L2_WINDOW - 1}"
            )
            and f"on line {AGG2_1_LINE}," in x
            for x in out
        )
        assert any(
            x.startswith(
                f"level 3:  slots {L3_BASE + L3_WINDOW:>4} .. {L3_BASE + 2 * L3_WINDOW - 1}"
            )
            and f"on level-2 IRQ {AGG3_1_L2}," in x
            for x in out
        )
        assert not any(x.startswith("ERROR") for x in out)

    def test_windows_match_the_generator_placement(self):
        """The map and gen_isr_config must agree on every window's first slot."""
        slots = [
            index(l2(AGG2_1_LINE, 0)),
            index(l3(AGG2_0_LINE, AGG3_1_L2, 0)),
        ]
        out = describe_windows(dict(SYMS), NUM_IRQS, 0, slots)

        for slot_index in slots:
            assert any(
                x.startswith("level ") and f"slots {slot_index:>4} .." in x and "1 connected" in x
                for x in out
            )

    def test_the_common_size_is_the_fallback(self):
        syms = dict(SYMS, CONFIG_MAX_IRQ_PER_AGGREGATOR=48)
        del syms["CONFIG_MAX_IRQ_PER_2ND_LEVEL_AGGREGATOR"]
        out = describe_windows(syms, NUM_IRQS, 0, [])

        assert any(x.startswith(f"level 2:  slots {L2_BASE + 48:>4} ..") for x in out)

    def test_overlapping_regions_are_reported(self):
        syms = dict(SYMS, CONFIG_3RD_LVL_ISR_TBL_OFFSET=L2_BASE + L2_WINDOW)
        out = describe_windows(syms, NUM_IRQS, 0, [])

        assert any(x.startswith("ERROR: level 2 windows end") for x in out)

    def test_no_map_without_level_2(self):
        assert describe_windows({"CONFIG_MULTI_LEVEL_INTERRUPTS": 1}, NUM_IRQS, 0, []) == []


if __name__ == "__main__":
    sys.exit(pytest.main([__file__, "-v"]))
