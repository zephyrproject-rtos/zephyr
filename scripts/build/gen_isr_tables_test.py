#!/usr/bin/env python3
#
# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0

"""Tests for the interrupt-matrix placement in gen_isr_tables.py.

Covers the layout used by the Espressif interrupt matrix
(CONFIG_INTERRUPT_MATRIX_LAYOUT): lone sources land directly on their CPU line,
shared lines and lines hosting a 3rd-level aggregator get the 2nd-level
dispatcher, and each aggregator's 3rd-level window is packed densely from the
status-register bits that actually have a handler.
"""

import sys
from types import SimpleNamespace

import pytest
from gen_isr_tables import gen_isr_config, gen_isr_log

# Mirrors soc/espressif/esp32s3/Kconfig.defconfig.
L1_BITS = 8
L3_BITS = 8
L2_BASE = 32
L3_BASE = 132

# The catch-all leaf: the largest value the level-3 field can encode, which is why
# it can never collide with a real status-register bit. Derived the same way
# gen_isr_config.is_l3_catch_all() derives it, so the two cannot drift. Mirrors
# ESP_L3_CATCH_ALL in the dt-bindings headers.
CATCH_ALL = (1 << L3_BITS) - 2

SYMS = {
    "CONFIG_MULTI_LEVEL_INTERRUPTS": 1,
    "CONFIG_INTERRUPT_MATRIX_LAYOUT": 1,
    "CONFIG_2ND_LEVEL_INTERRUPTS": 1,
    "CONFIG_3RD_LEVEL_INTERRUPTS": 1,
    "CONFIG_1ST_LEVEL_INTERRUPT_BITS": L1_BITS,
    "CONFIG_2ND_LEVEL_INTERRUPT_BITS": 8,
    "CONFIG_3RD_LEVEL_INTERRUPT_BITS": L3_BITS,
    "CONFIG_NUM_2ND_LEVEL_AGGREGATORS": 1,
    "CONFIG_NUM_3RD_LEVEL_AGGREGATORS": 1,
    "CONFIG_2ND_LVL_ISR_TBL_OFFSET": L2_BASE,
    "CONFIG_3RD_LVL_ISR_TBL_OFFSET": L3_BASE,
    "CONFIG_2ND_LVL_INTR_00_OFFSET": L2_BASE,
    "CONFIG_MAX_IRQ_PER_AGGREGATOR": 0,
    "CONFIG_MAX_IRQ_PER_2ND_LEVEL_AGGREGATOR": 100,
    "CONFIG_MAX_IRQ_PER_3RD_LEVEL_AGGREGATOR": 32,
}


def l2(line, src):
    """Encode a level-2 IRQ: source @src routed to CPU line @line."""
    return ((src + 1) << L1_BITS) | line


def l3(line, src, bit):
    """Encode a level-3 IRQ: status bit @bit of the aggregator on source @src."""
    return ((bit + 1) << (2 * L1_BITS)) | l2(line, src)


def make_config(syms=None):
    args = SimpleNamespace(sw_isr_table=True, vector_table=False, big_endian=False)
    # The real logger, so error() aborts the way it does in a build.
    return gen_isr_config(args, dict(syms or SYMS), gen_isr_log())


def entries(*irqs):
    """intList tuples; only irq and flags are read by the pre-pass."""
    return [(irq, 0, 0, 0) for irq in irqs]


def index(cfg, irq):
    return cfg.get_swt_table_index(0, irq)


class TestSecondLevel:
    def test_lone_source_goes_to_its_cpu_line(self):
        cfg = make_config()
        cfg.note_multilevel_topology(entries(l2(5, 20)))

        assert index(cfg, l2(5, 20)) == 5
        assert not cfg.get_l1_dispatcher_line(5)

    def test_shared_line_uses_source_window(self):
        cfg = make_config()
        cfg.note_multilevel_topology(entries(l2(8, 20), l2(8, 21)))

        assert index(cfg, l2(8, 20)) == L2_BASE + 20
        assert index(cfg, l2(8, 21)) == L2_BASE + 21
        assert cfg.get_l1_dispatcher_line(8)

    def test_two_handlers_on_one_source_is_an_error(self):
        cfg = make_config()

        with pytest.raises(SystemExit, match="3rd-level interrupt handling"):
            cfg.note_multilevel_topology(entries(l2(8, 20), l2(8, 20)))


class TestThirdLevel:
    def test_dense_window_from_contiguous_bits(self):
        """LCD_CAM's four flags occupy the first four slots of the region."""
        cfg = make_config()
        cfg.note_multilevel_topology(entries(*(l3(17, 24, bit) for bit in range(4))))

        for bit in range(4):
            assert index(cfg, l3(17, 24, bit)) == L3_BASE + bit

    def test_dense_window_from_sparse_bits(self):
        """A slot is a bit's rank in the mask, not the bit number."""
        cfg = make_config()
        cfg.note_multilevel_topology(entries(l3(17, 24, 5), l3(17, 24, 19)))

        assert index(cfg, l3(17, 24, 5)) == L3_BASE
        assert index(cfg, l3(17, 24, 19)) == L3_BASE + 1

        ((_, group),) = cfg.l3_windows
        assert group["mask"] == (1 << 5) | (1 << 19)
        assert group["win_base"] == L3_BASE

    def test_windows_are_packed_back_to_back(self):
        cfg = make_config()
        cfg.note_multilevel_topology(
            entries(l3(17, 24, 0), l3(17, 24, 1), l3(9, 40, 0), l3(9, 40, 2), l3(9, 40, 3))
        )

        windows = dict(cfg.l3_windows)
        assert windows[(17, 24)]["win_base"] == L3_BASE
        assert windows[(9, 40)]["win_base"] == L3_BASE + 2
        assert index(cfg, l3(9, 40, 3)) == L3_BASE + 4

    def test_window_order_follows_source_not_link_order(self):
        forward = make_config()
        forward.note_multilevel_topology(entries(l3(17, 24, 0), l3(9, 40, 0)))
        reverse = make_config()
        reverse.note_multilevel_topology(entries(l3(9, 40, 0), l3(17, 24, 0)))

        assert dict(forward.l3_windows).keys() == dict(reverse.l3_windows).keys()
        assert index(forward, l3(9, 40, 0)) == index(reverse, l3(9, 40, 0))

    def test_aggregator_source_slot_gets_the_l3_dispatcher(self):
        cfg = make_config()
        cfg.note_multilevel_topology(entries(l3(17, 24, 0), l3(17, 24, 1)))

        group = cfg.get_l3_dispatcher_slot(L2_BASE + 24)
        assert group is not None
        assert group["index"] == 0
        assert cfg.get_l3_dispatcher_slot(L2_BASE + 25) is None

    def test_lone_aggregator_line_still_gets_the_l2_dispatcher(self):
        """The rule that keeps an L3 line off the lone-source shortcut."""
        cfg = make_config()
        cfg.note_multilevel_topology(entries(l3(17, 24, 0), l3(17, 24, 1)))

        assert cfg.get_l1_dispatcher_line(17)

    def test_duplicate_flag_is_an_error(self):
        cfg = make_config()

        with pytest.raises(SystemExit, match="registered twice"):
            cfg.note_multilevel_topology(entries(l3(17, 24, 2), l3(17, 24, 2)))

    def test_direct_handler_on_an_aggregator_source_is_an_error(self):
        cfg = make_config()

        with pytest.raises(SystemExit, match="also has a handler connected directly"):
            cfg.note_multilevel_topology(entries(l3(17, 24, 0), l2(17, 24)))

    def test_flag_beyond_the_bound_is_an_error(self):
        cfg = make_config()

        with pytest.raises(SystemExit, match="MAX_IRQ_PER_3RD_LEVEL_AGGREGATOR"):
            cfg.note_multilevel_topology(entries(l3(17, 24, 0), l3(17, 24, 32)))


class TestCatchAll:
    """The catch-all leaf: a handler with no status bit of its own.

    ESP32-H2 shares GPIO_INTR_SOURCE between the analog comparator, which owns
    GPIO_EXT.int_st bit 0, and the GPIO port driver, which reads its own per-pin
    status word and has no bit in that register at all. The catch-all takes the
    slot just past the masked ones and is dispatched unconditionally.
    """

    def test_catch_all_takes_the_slot_after_the_masked_bits(self):
        cfg = make_config()
        cfg.note_multilevel_topology(
            entries(l3(17, 24, 0), l3(17, 24, 5), l3(17, 24, CATCH_ALL))
        )

        # Two real bits compact to 132/133, so the catch-all lands on 134.
        assert index(cfg, l3(17, 24, 0)) == L3_BASE
        assert index(cfg, l3(17, 24, 5)) == L3_BASE + 1
        assert index(cfg, l3(17, 24, CATCH_ALL)) == L3_BASE + 2

    def test_catch_all_is_flagged_and_kept_out_of_the_mask(self):
        cfg = make_config()
        cfg.note_multilevel_topology(entries(l3(17, 24, 1), l3(17, 24, CATCH_ALL)))

        group = dict(cfg.l3_windows)[(17, 24)]
        assert group["mask"] == 0x2
        assert group["catch_all"] is True

    def test_catch_all_alone_gets_an_empty_mask(self):
        """A source whose only handler has no status bit is still an aggregator."""
        cfg = make_config()
        cfg.note_multilevel_topology(entries(l3(17, 24, CATCH_ALL)))

        group = dict(cfg.l3_windows)[(17, 24)]
        assert group["mask"] == 0
        assert group["catch_all"] is True
        assert index(cfg, l3(17, 24, CATCH_ALL)) == L3_BASE

    def test_window_without_catch_all_is_not_flagged(self):
        cfg = make_config()
        cfg.note_multilevel_topology(entries(l3(17, 24, 0), l3(17, 24, 1)))

        assert dict(cfg.l3_windows)[(17, 24)]["catch_all"] is False

    def test_catch_all_shifts_the_next_window(self):
        """It occupies a real slot, so the following aggregator must move up."""
        cfg = make_config()
        cfg.note_multilevel_topology(
            entries(l3(17, 24, 0), l3(17, 24, CATCH_ALL), l3(9, 40, 0))
        )

        windows = dict(cfg.l3_windows)
        assert windows[(17, 24)]["win_base"] == L3_BASE
        assert windows[(9, 40)]["win_base"] == L3_BASE + 2

    def test_two_catch_alls_on_one_source_is_an_error(self):
        cfg = make_config()

        with pytest.raises(SystemExit, match="more than one catch-all"):
            cfg.note_multilevel_topology(
                entries(l3(17, 24, CATCH_ALL), l3(17, 24, CATCH_ALL))
            )

    def test_catch_all_does_not_shadow_the_bound_check(self):
        """The sentinel is far above MAX_IRQ_PER_3RD, so 32 still errors."""
        cfg = make_config()

        with pytest.raises(SystemExit, match="MAX_IRQ_PER_3RD_LEVEL_AGGREGATOR"):
            cfg.note_multilevel_topology(entries(l3(17, 24, 32)))

    def test_catch_all_line_still_gets_the_l2_dispatcher(self):
        cfg = make_config()
        cfg.note_multilevel_topology(entries(l3(17, 24, CATCH_ALL)))

        assert cfg.get_l1_dispatcher_line(17)

    def test_region_overlap_is_an_error(self):
        """P must start at or after the end of S.

        The two regions are placed by independent Kconfig values and both are
        sparsely occupied, so an overlap otherwise builds cleanly until a level-2
        source happens to collide with a level-3 flag.
        """
        # Level-2 window is 32..132; starting level 3 at 131 steals one slot.
        syms = dict(SYMS, CONFIG_3RD_LVL_ISR_TBL_OFFSET=L2_BASE + 100 - 1)
        cfg = make_config(syms)

        with pytest.raises(SystemExit, match="overlaps the level-2 window"):
            cfg.note_multilevel_topology(entries(l3(17, 24, 0), l3(17, 24, 1)))

    def test_region_boundary_exactly_touching_is_fine(self):
        syms = dict(SYMS, CONFIG_3RD_LVL_ISR_TBL_OFFSET=L2_BASE + 100)
        cfg = make_config(syms)
        cfg.note_multilevel_topology(entries(l3(17, 24, 0), l3(17, 24, 1)))

        assert index(cfg, l3(17, 24, 0)) == L2_BASE + 100


class TestOtherLayouts:
    """Every platform that has not opted in keeps the fixed-width windows."""

    def test_multi_aggregator_layout_is_untouched(self):
        """With 2+ aggregators the pre-pass must stay inert.

        Here 2ND_LVL_INTR_NN_OFFSET is an aggregator's CPU line, and the L1 field
        selects a fixed-width window rather than being a routing line.
        """
        syms = dict(SYMS, CONFIG_NUM_2ND_LEVEL_AGGREGATORS=2)
        del syms["CONFIG_INTERRUPT_MATRIX_LAYOUT"]
        syms["CONFIG_2ND_LVL_INTR_00_OFFSET"] = 8
        syms["CONFIG_2ND_LVL_INTR_01_OFFSET"] = 9
        cfg = make_config(syms)
        cfg.note_multilevel_topology(entries(l2(8, 20), l2(8, 21)))

        assert cfg.l3_windows == []
        assert not cfg.get_l1_dispatcher_line(8)
        # Aggregator 0 is CPU line 8, so its window starts at the base.
        assert index(cfg, l2(8, 21)) == L2_BASE + 21
        # Aggregator 1 is CPU line 9, one full window further up.
        assert index(cfg, l2(9, 21)) == L2_BASE + 100 + 21

    def test_one_aggregator_without_the_symbol_is_untouched(self):
        """A single aggregator is not on its own the interrupt matrix.

        intel_adsp, cva6, mt8186/8188/8365 and the i.MX9 m7 cores all set
        NUM_2ND_LEVEL_AGGREGATORS to 1 and expect the fixed-width window, so the
        layout has to hang off CONFIG_INTERRUPT_MATRIX_LAYOUT alone. Two sources
        on one line stay in the window instead of putting z_soc_2nd_lvl_isr, a
        symbol only an interrupt-matrix SoC defines, into the line's slot.
        """
        syms = dict(SYMS)
        del syms["CONFIG_INTERRUPT_MATRIX_LAYOUT"]
        syms["CONFIG_2ND_LVL_INTR_00_OFFSET"] = 8
        cfg = make_config(syms)
        cfg.note_multilevel_topology(entries(l2(8, 20), l2(8, 21)))

        assert cfg.l3_windows == []
        assert not cfg.get_l1_dispatcher_line(8)
        assert not cfg.emits_l3_windows()
        assert index(cfg, l2(8, 20)) == L2_BASE + 20
        assert index(cfg, l2(8, 21)) == L2_BASE + 21

    def test_lone_source_stays_in_the_window_without_the_symbol(self):
        """The lone-source shortcut is the other half of the opt-in."""
        syms = dict(SYMS)
        del syms["CONFIG_INTERRUPT_MATRIX_LAYOUT"]
        syms["CONFIG_2ND_LVL_INTR_00_OFFSET"] = 8
        cfg = make_config(syms)
        cfg.note_multilevel_topology(entries(l2(8, 20)))

        assert index(cfg, l2(8, 20)) == L2_BASE + 20


if __name__ == "__main__":
    sys.exit(pytest.main([__file__, "-v"]))
