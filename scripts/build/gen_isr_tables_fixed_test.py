#!/usr/bin/env python3
#
# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0

"""Tests for the fixed-width aggregator layout in gen_isr_tables.py.

Covers the placement every platform uses that has not opted into
CONFIG_INTERRUPT_MATRIX_LAYOUT: each aggregator owns a window of
CONFIG_MAX_IRQ_PER_<n>_LEVEL_AGGREGATOR slots, selected by the aggregator's
position in the CONFIG_<n>_LVL_INTR_NN_OFFSET list.

Two things are pinned here. The window arithmetic, including the per-level
window sizes and their fallback to the common CONFIG_MAX_IRQ_PER_AGGREGATOR,
and the generated C itself: a fixed-width build must not grow a reference to
z_soc_2nd_lvl_isr, z_soc_3rd_lvl_isr or z_isr_l3_windows[], none of which such
a platform defines.
"""

import io
import struct
import sys
from types import SimpleNamespace

import pytest
from dump_isr_intlist import describe_layout
from gen_isr_tables import gen_isr_config, gen_isr_log
from gen_isr_tables_parser_carrays import gen_isr_parser as carrays_parser
from gen_isr_tables_parser_local import gen_isr_parser as local_parser

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

# Handler addresses. _sw_isr_table holds the raw address the linker resolved,
# so these are what the generated table is expected to contain verbatim.
HANDLER_A = 0x1000
HANDLER_B = 0x2000
HANDLER_C = 0x3000
HANDLER_D = 0x4000
HANDLER_E = 0x5000

ISR_FLAG_DIRECT = 1 << 0

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
    "CONFIG_GEN_SW_ISR_TABLE_ARRAY": 1,
}


def l2(line, local):
    """Encode a level-2 IRQ: local IRQ @local of the aggregator on CPU line @line."""
    return ((local + 1) << L1_BITS) | line


def l3(line, l2_local, local):
    """Encode a level-3 IRQ: local IRQ @local behind level-2 IRQ @l2_local."""
    return ((local + 1) << (L1_BITS + L2_BITS)) | l2(line, l2_local)


def make_config(syms=None, **args):
    opts = {"sw_isr_table": True, "vector_table": False, "big_endian": False}
    opts.update(args)
    # The real logger, so error() aborts the way it does in a build.
    return gen_isr_config(
        SimpleNamespace(**opts), dict(SYMS if syms is None else syms), gen_isr_log()
    )


def index(cfg, irq):
    return cfg.get_swt_table_index(0, irq)


def carrays_blob(entries, num_vectors=NUM_IRQS, offset=0):
    """Pack an intList section the way linker/intlist.ld lays it out."""
    data = struct.pack("<II", num_vectors, offset)
    for irq, flags, func, param in entries:
        data += struct.pack("<iiII", irq, flags, func, param)
    return data


def local_blob(entries, num_vectors=NUM_IRQS, offset=0):
    """Pack an intList section in the CONFIG_ISR_TABLES_LOCAL_DECLARATION shape."""
    data = struct.pack("<IIIII", num_vectors, offset, 8, 12, 8)
    for irq, flags, sname in entries:
        data += struct.pack("<ii", irq, flags)
        raw = sname.encode() + b"\0"
        data += raw + b"\0" * (-len(raw) % 4)
    # __read_intlist() stops at "> entry_sz" bytes left, so the section needs a
    # trailing zero pad for the last entry to be read.
    return data + b"\0" * 8


def generate(entries, syms=None, **args):
    """Run the carrays parser over @entries and return the generated C source."""
    cfg = make_config(syms, **args)
    parser = carrays_parser(carrays_blob(entries), cfg, gen_isr_log())
    fp = io.StringIO()
    parser.write_source(fp)
    return fp.getvalue()


def slot(handler, param=0):
    """The generated _sw_isr_table row for @handler called with @param."""
    return f"(const void *){param:#x}, (ISR){handler:#x}"


def spurious():
    return "(const void *)0x0, (ISR)z_irq_spurious"


def table_rows(source):
    """Map _sw_isr_table index -> "(param, handler)" from the generated array."""
    rows = {}
    for line in source.splitlines():
        stripped = line.strip()
        if not stripped.startswith("{(const void *)"):
            continue
        body, _, comment = stripped.partition("}, /* ")
        rows[int(comment.rstrip(" */"))] = body.lstrip("{")
    return rows


class TestFixedWidthPlacement:
    """The window arithmetic: base offset + aggregator index * window size."""

    def test_level_1_irq_is_its_own_index(self):
        cfg = make_config()

        assert index(cfg, AGG2_0_LINE) == AGG2_0_LINE

    def test_level_2_window_is_selected_by_the_cpu_line(self):
        cfg = make_config()

        assert index(cfg, l2(AGG2_0_LINE, 3)) == L2_BASE + 3
        assert index(cfg, l2(AGG2_1_LINE, 3)) == L2_BASE + L2_WINDOW + 3

    def test_level_3_window_is_selected_by_the_level_2_irq(self):
        cfg = make_config()

        assert index(cfg, l3(AGG2_0_LINE, AGG3_0_L2, 4)) == L3_BASE + 4
        assert index(cfg, l3(AGG2_0_LINE, AGG3_1_L2, 4)) == L3_BASE + L3_WINDOW + 4

    def test_level_3_placement_ignores_the_cpu_line(self):
        """CONFIG_3RD_LVL_INTR_NN_OFFSET names a level-2 IRQ, not a line.

        Two level-3 aggregators behind the same level-2 local IRQ on different
        CPU lines therefore share a window, which is a devicetree the fixed
        layout cannot express. Pinned so the behaviour is not mistaken for a
        placement bug.
        """
        cfg = make_config()

        assert index(cfg, l3(AGG2_0_LINE, AGG3_0_L2, 4)) == index(
            cfg, l3(AGG2_1_LINE, AGG3_0_L2, 4)
        )

    def test_levels_do_not_overlap_across_a_full_window(self):
        cfg = make_config()
        last_l2 = index(cfg, l2(AGG2_1_LINE, L2_WINDOW - 1))
        first_l3 = index(cfg, l3(AGG2_0_LINE, AGG3_0_L2, 0))

        assert last_l2 < L3_BASE <= first_l3

    def test_unknown_aggregator_line_is_an_error(self):
        cfg = make_config()

        with pytest.raises(SystemExit, match="not present in parent offsets"):
            index(cfg, l2(AGG2_0_LINE + 100, 3))


class TestAggregatorWindowSizes:
    """Per-level window sizes and their fallback to the common symbol."""

    def test_per_level_sizes_are_independent(self):
        cfg = make_config()

        # A level-3 local IRQ past the level-3 window still lands inside the
        # next window, which is what makes the smaller size observable.
        assert index(cfg, l3(AGG2_0_LINE, AGG3_1_L2, 0)) == L3_BASE + L3_WINDOW

    def test_both_levels_fall_back_to_the_common_symbol(self):
        syms = dict(SYMS, CONFIG_MAX_IRQ_PER_AGGREGATOR=48)
        del syms["CONFIG_MAX_IRQ_PER_2ND_LEVEL_AGGREGATOR"]
        del syms["CONFIG_MAX_IRQ_PER_3RD_LEVEL_AGGREGATOR"]
        cfg = make_config(syms)

        assert index(cfg, l2(AGG2_1_LINE, 0)) == L2_BASE + 48
        assert index(cfg, l3(AGG2_0_LINE, AGG3_1_L2, 0)) == L3_BASE + 48

    def test_one_level_overrides_while_the_other_falls_back(self):
        syms = dict(SYMS, CONFIG_MAX_IRQ_PER_AGGREGATOR=48)
        del syms["CONFIG_MAX_IRQ_PER_2ND_LEVEL_AGGREGATOR"]
        cfg = make_config(syms)

        assert index(cfg, l2(AGG2_1_LINE, 0)) == L2_BASE + 48
        assert index(cfg, l3(AGG2_0_LINE, AGG3_1_L2, 0)) == L3_BASE + L3_WINDOW

    def test_a_zero_per_level_size_falls_back(self):
        """Kconfig defaults the per-level symbols to the common one, so a zero
        here means "not configured at this level" rather than a zero-wide
        window.
        """
        syms = dict(
            SYMS, CONFIG_MAX_IRQ_PER_AGGREGATOR=48, CONFIG_MAX_IRQ_PER_2ND_LEVEL_AGGREGATOR=0
        )
        cfg = make_config(syms)

        assert index(cfg, l2(AGG2_1_LINE, 0)) == L2_BASE + 48


class TestGeneratedTable:
    """The emitted _sw_isr_table, which nothing else in the tree checks."""

    def test_handlers_land_on_their_computed_slots(self):
        source = generate(
            [
                (AGG2_0_LINE, 0, HANDLER_A, 0),
                (l2(AGG2_1_LINE, 3), 0, HANDLER_B, 0xAB),
                (l3(AGG2_0_LINE, AGG3_1_L2, 4), 0, HANDLER_C, 0),
            ]
        )
        rows = table_rows(source)

        assert len(rows) == NUM_IRQS
        assert rows[AGG2_0_LINE] == slot(HANDLER_A)
        assert rows[L2_BASE + L2_WINDOW + 3] == slot(HANDLER_B, 0xAB)
        assert rows[L3_BASE + L3_WINDOW + 4] == slot(HANDLER_C)

    def test_unconnected_slots_get_the_spurious_handler(self):
        source = generate([(AGG2_0_LINE, 0, HANDLER_A, 0)])
        rows = table_rows(source)

        assert rows[0] == spurious()
        assert rows[L2_BASE] == spurious()
        assert rows[NUM_IRQS - 1] == spurious()

    def test_no_matrix_layout_symbols_are_emitted(self):
        """The fixed-width platforms define none of these.

        A stray reference is an undefined-symbol link failure on every board
        from cavs to the PLIC ones, so it is worth asserting on the text rather
        than only on the placement.
        """
        source = generate(
            [
                (AGG2_0_LINE, 0, HANDLER_A, 0),
                (l2(AGG2_0_LINE, 3), 0, HANDLER_B, 0),
                (l2(AGG2_0_LINE, 4), 0, HANDLER_C, 0),
                (l3(AGG2_0_LINE, AGG3_0_L2, 0), 0, HANDLER_D, 0),
                (l3(AGG2_0_LINE, AGG3_0_L2, 1), 0, HANDLER_E, 0),
            ]
        )

        assert "z_soc_2nd_lvl_isr" not in source
        assert "z_soc_3rd_lvl_isr" not in source
        assert "z_isr_l3_windows" not in source
        assert "z_isr_l3_window_num" not in source
        assert "P-window" not in source

    def test_two_sources_on_one_line_stay_in_the_window(self):
        """The lone-source shortcut and the shared-line dispatcher are both
        interrupt-matrix behaviour; here every level-2 source keeps its window
        slot and the CPU line's own slot stays spurious.
        """
        source = generate(
            [
                (l2(AGG2_0_LINE, 3), 0, HANDLER_A, 0),
                (l2(AGG2_0_LINE, 4), 0, HANDLER_B, 0),
            ]
        )
        rows = table_rows(source)

        assert rows[L2_BASE + 3] == slot(HANDLER_A)
        assert rows[L2_BASE + 4] == slot(HANDLER_B)
        assert rows[AGG2_0_LINE] == spurious()

    def test_a_lone_source_stays_in_the_window(self):
        source = generate([(l2(AGG2_0_LINE, 3), 0, HANDLER_A, 0)])
        rows = table_rows(source)

        assert rows[L2_BASE + 3] == slot(HANDLER_A)
        assert rows[AGG2_0_LINE] == spurious()

    def test_a_level_1_handler_shares_a_line_with_level_2_sources(self):
        """The aggregator's own line carries its driver ISR while its sources
        sit in the window. Every multi-level platform is built this way.
        """
        source = generate(
            [
                (AGG2_0_LINE, 0, HANDLER_A, 0),
                (l2(AGG2_0_LINE, 3), 0, HANDLER_B, 0),
                (l2(AGG2_0_LINE, 4), 0, HANDLER_C, 0),
            ]
        )
        rows = table_rows(source)

        assert rows[AGG2_0_LINE] == slot(HANDLER_A)
        assert rows[L2_BASE + 3] == slot(HANDLER_B)
        assert rows[L2_BASE + 4] == slot(HANDLER_C)

    def test_two_handlers_on_one_slot_is_an_error(self):
        with pytest.raises(SystemExit, match="multiple registrations at table_index"):
            generate(
                [
                    (l2(AGG2_0_LINE, 3), 0, HANDLER_A, 0),
                    (l2(AGG2_0_LINE, 3), 0, HANDLER_B, 0),
                ]
            )

    def test_switch_form_emits_no_matrix_layout_symbols(self):
        syms = dict(SYMS)
        del syms["CONFIG_GEN_SW_ISR_TABLE_ARRAY"]
        syms["CONFIG_GEN_SW_ISR_TABLE_SWITCH"] = 1
        source = generate(
            [
                (l2(AGG2_0_LINE, 3), 0, HANDLER_A, 0),
                (l2(AGG2_0_LINE, 4), 0, HANDLER_B, 0),
                (l3(AGG2_0_LINE, AGG3_0_L2, 0), 0, HANDLER_C, 0),
            ],
            syms,
        )

        assert "case 19:" in source
        assert "z_soc_2nd_lvl_isr" not in source
        assert "z_soc_3rd_lvl_isr" not in source
        assert "z_isr_l3_windows" not in source

    def test_a_direct_isr_is_kept_out_of_the_sw_table(self):
        syms = dict(SYMS, CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_ADDRESS=1)
        source = generate(
            [
                (AGG2_1_LINE, ISR_FLAG_DIRECT, HANDLER_A, 0),
                (l2(AGG2_0_LINE, 3), 0, HANDLER_B, 0),
            ],
            syms,
            vector_table=True,
        )
        rows = table_rows(source)

        assert rows[AGG2_1_LINE] == spurious()
        assert rows[L2_BASE + 3] == slot(HANDLER_B)
        assert f"\t{HANDLER_A},\n" in source

    def test_shared_clients_land_on_one_slot(self):
        syms = dict(SYMS, CONFIG_SHARED_INTERRUPTS=1, CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS=4)
        source = generate(
            [
                (l2(AGG2_0_LINE, 3), 0, HANDLER_A, 0),
                (l2(AGG2_0_LINE, 3), 0, HANDLER_B, 0),
            ],
            syms,
        )
        rows = table_rows(source)

        assert rows[L2_BASE + 3].endswith("(ISR)z_shared_isr")
        assert "z_soc_2nd_lvl_isr" not in source
        assert f"{{ .isr = (ISR){HANDLER_A:#x}, .arg = (const void *)0x0 }}," in source


class TestLocalDeclarationParser:
    """CONFIG_ISR_TABLES_LOCAL_DECLARATION, used by the LTO scenarios."""

    def test_fixed_width_multilevel_is_accepted(self):
        """The matrix-layout refusal must not fire here.

        A false positive would break arch.shared_interrupt.lto and every other
        local-declaration build on a multi-level platform.
        """
        cfg = make_config()
        entries = [
            (AGG2_0_LINE, 0, ".irq_handler.a"),
            (l2(AGG2_0_LINE, 3), 0, ".irq_handler.b"),
            (l3(AGG2_0_LINE, AGG3_0_L2, 0), 0, ".irq_handler.c"),
        ]
        parser = local_parser(local_blob(entries), cfg, gen_isr_log())
        fp = io.StringIO()
        parser.write_source(fp)

        assert "INTERRUPT_MATRIX_LAYOUT" not in fp.getvalue()


class TestIntlistDump:
    """dump_isr_intlist.py, which CMake runs on every multi-level build.

    It replays the generator's placement to print a layout map, so its own copy
    of the rules has to stay switched off wherever the generator's is.
    """

    def test_no_layout_map_without_the_symbol(self):
        line_srcs = {AGG2_0_LINE: {3, 4}, AGG2_1_LINE: {5}}
        l3_groups = {(AGG2_0_LINE, AGG3_0_L2): {0, 1, 2}}

        assert describe_layout(dict(SYMS), NUM_IRQS, line_srcs, l3_groups) == []

    def test_no_layout_map_for_an_empty_intlist(self):
        assert describe_layout(dict(SYMS), NUM_IRQS, {}, {}) == []

    def test_the_map_appears_once_the_symbol_is_set(self):
        """The negative tests above would pass against a stub, so show the
        function does produce a map when the layout is actually in use.
        """
        syms = dict(SYMS, CONFIG_INTERRUPT_MATRIX_LAYOUT=1)
        out = describe_layout(syms, NUM_IRQS, {AGG2_0_LINE: {3, 4}}, {})

        assert any("level-1 lines vectoring through z_soc_2nd_lvl_isr" in x for x in out)


if __name__ == "__main__":
    sys.exit(pytest.main([__file__, "-v"]))
