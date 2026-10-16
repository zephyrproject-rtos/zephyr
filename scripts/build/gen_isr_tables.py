#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
# Copyright (c) 2018 Foundries.io
# Copyright (c) 2023 Nordic Semiconductor NA
#
# SPDX-License-Identifier: Apache-2.0
#

import argparse
import importlib
import os
import sys

from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection


class gen_isr_log:
    def __init__(self, debug=False):
        self.__debug = debug

    def debug(self, text):
        """Print debug message if debugging is enabled.

        Note - this function requires config global variable to be initialized.
        """
        if self.__debug:
            sys.stdout.write(os.path.basename(sys.argv[0]) + ": " + text + "\n")

    @staticmethod
    def error(text):
        sys.exit(os.path.basename(sys.argv[0]) + ": error: " + text + "\n")

    def set_debug(self, state):
        self.__debug = state


log = gen_isr_log()


class gen_isr_config:
    """All the constants and configuration gathered in single class for readability."""

    # Constants
    __ISR_FLAG_DIRECT = 1 << 0
    __swt_spurious_handler = "z_irq_spurious"
    __swt_shared_handler = "z_shared_isr"
    # Shared-line dispatcher of the interrupt-matrix layout
    # (CONFIG_INTERRUPT_MATRIX_LAYOUT): placed in the 1st-level slot of
    # every CPU line carrying two or more level-2 sources, with the line number
    # as its argument. Provided by the SoC's 2nd-level interrupt controller.
    __swt_l2_dispatcher = "z_soc_2nd_lvl_isr"
    # Per-peripheral status-register demux of the interrupt-matrix layout.
    # Placed in the 2nd-level slot of every INTMUX source that is itself a
    # 3rd-level aggregator (i.e. two or more handlers registered against that one
    # source), with its window index as argument. Provided by the SoC's
    # interrupt controller.
    __swt_l3_dispatcher = "z_soc_3rd_lvl_isr"
    __vt_spurious_handler = "z_irq_spurious"
    __vt_irq_handler = "_isr_wrapper"
    __shared_array_name = "z_shared_sw_isr_table"
    __sw_isr_array_name = "_sw_isr_table"
    __irq_vector_array_name = "_irq_vector_table"

    @staticmethod
    def __bm(bits):
        return (1 << bits) - 1

    def __init__(self, args, syms, log):
        """Initialize the configuration object.

        The configuration object initialization takes only arguments as a parameter.
        This is done to allow debug function work as soon as possible.
        """
        # Store the arguments required for work
        self.__args = args
        self.__syms = syms
        self.__log = log

        # Select the default interrupt vector handler
        if self.args.sw_isr_table:
            self.__vt_default_handler = self.__vt_irq_handler
        else:
            self.__vt_default_handler = self.__vt_spurious_handler
        # Calculate interrupt bits
        self.__int_bits = [8, 8, 8]
        # The below few hardware independent magic numbers represent various
        # levels of interrupts in a multi-level interrupt system.
        # 0x000000FF - represents the 1st level (i.e. the interrupts
        #              that directly go to the processor).
        # 0x0000FF00 - represents the 2nd level (i.e. the interrupts funnel
        #              into 1 line which then goes into the 1st level)
        # 0x00FF0000 - represents the 3rd level (i.e. the interrupts funnel
        #              into 1 line which then goes into the 2nd level)
        self.__int_lvl_masks = [0x000000FF, 0x0000FF00, 0x00FF0000]

        self.__irq2_baseoffset = None
        self.__irq3_baseoffset = None
        self.__irq2_offsets = None
        self.__irq3_offsets = None

        # Interrupt topology of the interrupt-matrix layout, filled by
        # note_multilevel_topology(). Left empty for every other layout, so the
        # generic multi-aggregator placement is unaffected.
        #
        #   __l2_line_srcs[line]        set of level-2 sources on that CPU line.
        #                               A 3rd-level aggregator counts once, not
        #                               once per flag.
        #   __l3_groups[(line, src)]    dict(index, bits, mask, win_base) of a
        #                               3rd-level aggregator: the status-register
        #                               bits actually connected, their OR as a
        #                               mask, and the densely allocated window.
        #   __l3_by_l2_slot[slot]       reverse map from a 2nd-level table slot to
        #                               the group demuxed behind it, for the
        #                               parsers.
        self.__l2_line_srcs = {}
        self.__l3_groups = {}
        self.__l3_by_l2_slot = {}

        if self.check_multi_level_interrupts():
            # Per-level aggregator window sizes, falling back to the common
            # CONFIG_MAX_IRQ_PER_AGGREGATOR when a per-level symbol is not set.
            max_irq_common = self.get_sym("CONFIG_MAX_IRQ_PER_AGGREGATOR")
            self.__max_irq_per = {
                2: self.get_sym("CONFIG_MAX_IRQ_PER_2ND_LEVEL_AGGREGATOR") or max_irq_common,
                3: self.get_sym("CONFIG_MAX_IRQ_PER_3RD_LEVEL_AGGREGATOR") or max_irq_common,
            }

            self.__int_bits[0] = self.get_sym("CONFIG_1ST_LEVEL_INTERRUPT_BITS")
            self.__int_bits[1] = self.get_sym("CONFIG_2ND_LEVEL_INTERRUPT_BITS")
            self.__int_bits[2] = self.get_sym("CONFIG_3RD_LEVEL_INTERRUPT_BITS")

            if sum(self.int_bits) > 32:
                raise ValueError("Too many interrupt bits")

            self.__int_lvl_masks[0] = self.__bm(self.int_bits[0])
            self.__int_lvl_masks[1] = self.__bm(self.int_bits[1]) << self.int_bits[0]
            self.__int_lvl_masks[2] = self.__bm(self.int_bits[2]) << (
                self.int_bits[0] + self.int_bits[1]
            )

            self.__log.debug("Level    Bits        Bitmask")
            self.__log.debug("----------------------------")
            for i in range(3):
                bitmask_str = "0x" + format(self.__int_lvl_masks[i], '08X')
                self.__log.debug(f"{i + 1:>5} {self.__int_bits[i]:>7} {bitmask_str:>14}")

            if self.check_sym("CONFIG_2ND_LEVEL_INTERRUPTS"):
                num_aggregators = self.get_sym("CONFIG_NUM_2ND_LEVEL_AGGREGATORS")
                self.__irq2_baseoffset = self.get_sym("CONFIG_2ND_LVL_ISR_TBL_OFFSET")
                self.__irq2_offsets = [
                    self.get_sym(f'CONFIG_2ND_LVL_INTR_{str(i).zfill(2)}_OFFSET')
                    for i in range(num_aggregators)
                ]

                self.__log.debug(f'2nd level offsets: {self.__irq2_offsets}')

                if self.check_sym("CONFIG_3RD_LEVEL_INTERRUPTS"):
                    num_aggregators = self.get_sym("CONFIG_NUM_3RD_LEVEL_AGGREGATORS")
                    self.__irq3_baseoffset = self.get_sym("CONFIG_3RD_LVL_ISR_TBL_OFFSET")
                    self.__irq3_offsets = [
                        self.get_sym(f'CONFIG_3RD_LVL_INTR_{str(i).zfill(2)}_OFFSET')
                        for i in range(num_aggregators)
                    ]

                    self.__log.debug(f'3rd level offsets: {self.__irq3_offsets}')

    @property
    def args(self):
        return self.__args

    @property
    def swt_spurious_handler(self):
        return self.__swt_spurious_handler

    @property
    def swt_shared_handler(self):
        return self.__swt_shared_handler

    @property
    def swt_l2_dispatcher(self):
        return self.__swt_l2_dispatcher

    @property
    def swt_l3_dispatcher(self):
        return self.__swt_l3_dispatcher

    @property
    def l3_windows(self):
        """The 3rd-level aggregator groups, ordered by window index."""
        return sorted(self.__l3_groups.items(), key=lambda kv: kv[1]["index"])

    def is_l3_catch_all(self, bit):
        """True if @bit is the reserved "catch-all" level-3 leaf number.

        A catch-all leaf claims no status-register bit: the dispatcher calls it
        unconditionally, after every bit that is actually pending. It exists for
        hardware where one interrupt source is shared by a handler that has a
        clean status bit and one that has none - the ESP32-H2 analog comparator
        (GPIO_EXT.int_st bit 0) sharing GPIO_INTR_SOURCE with the GPIO port
        driver, which reads its own per-pin status word and self-guards when
        nothing is pending.

        The value is the largest the level-3 field can encode. Deliberately not
        MAX_IRQ_PER_3RD_LEVEL_AGGREGATOR: that is a per-SoC bound, so a SoC
        setting it below the register width would turn a real status bit into
        the sentinel, and it would also shadow the first out-of-range bit that
        the bound check exists to reject. Any value above the highest real bit
        makes the dense-window arithmetic in get_swt_table_index() work
        unchanged, because __bm(N) is all-ones over a mask holding only lower
        bits, so the leaf lands at win_base + popcount(mask) - one slot past
        the masked window.
        """
        bits = self.__int_bits[2]
        # The field stores bit + 1, so the last encodable bit is 2**bits - 2.
        return bool(bits) and bit == (1 << bits) - 2

    def uses_matrix_layout(self):
        """True if the flat, source-indexed interrupt-matrix layout is in use.

        Read from CONFIG_INTERRUPT_MATRIX_LAYOUT rather than inferred from
        CONFIG_NUM_2ND_LEVEL_AGGREGATORS == 1. Several platforms already have a
        single 2nd-level aggregator and expect the fixed-width windows, so the
        aggregator count alone cannot tell the two layouts apart: it has to be
        the SoC that says its 1st-level field is a routing line.
        """
        if not self.check_multi_level_interrupts():
            return False
        return self.check_sym("CONFIG_INTERRUPT_MATRIX_LAYOUT")

    def emits_l3_windows(self):
        """True if the generated source must define z_isr_l3_windows[].

        Keyed on the configuration rather than on whether any window was
        allocated, so a devicetree that declares an aggregator nothing attaches
        to gets an empty table to fail against instead of a link error.
        """
        if not self.uses_matrix_layout():
            return False
        return self.check_sym("CONFIG_3RD_LEVEL_INTERRUPTS")

    def line_needs_l2_dispatcher(self, line):
        """True if CPU line @line must vector through the 2nd-level dispatcher.

        That is the case when the line carries two or more level-2 sources, and
        also when it hosts a 3rd-level aggregator even as its only source: the
        aggregator's flags live in the 3rd-level window, so there is no single
        leaf ISR that could be placed directly on the line.
        """
        if len(self.__l2_line_srcs.get(line, ())) >= 2:
            return True
        return any(key[0] == line for key in self.__l3_groups)

    def get_l1_dispatcher_line(self, index):
        """True if _sw_isr_table[index] is the 1st-level slot of a CPU line that
        needs the 2nd-level dispatcher, in the interrupt-matrix layout. Such a
        slot gets the dispatcher instead of the spurious handler so the whole
        static configuration lives in the generated table.
        """
        if index >= (self.get_irq_baseoffset(2) or 0):
            return False
        return self.line_needs_l2_dispatcher(index)

    def get_l3_dispatcher_slot(self, index):
        """The 3rd-level group demuxed behind 2nd-level slot @index, else None.

        Such a slot has no leaf of its own - only the aggregator's flags are
        connected - so it gets the 3rd-level dispatcher, keyed on the group's
        window index.
        """
        return self.__l3_by_l2_slot.get(index)

    @property
    def vt_default_handler(self):
        return self.__vt_default_handler

    @property
    def shared_array_name(self):
        return self.__shared_array_name

    @property
    def sw_isr_array_name(self):
        return self.__sw_isr_array_name

    @property
    def irq_vector_array_name(self):
        return self.__irq_vector_array_name

    @property
    def int_bits(self):
        return self.__int_bits

    @property
    def int_lvl_masks(self):
        return self.__int_lvl_masks

    def endian_prefix(self):
        if self.args.big_endian:
            return ">"
        else:
            return "<"

    def get_irq_baseoffset(self, lvl):
        if lvl == 2:
            return self.__irq2_baseoffset
        if lvl == 3:
            return self.__irq3_baseoffset
        self.__log.error(f"Unsupported irq level: {lvl}")

    def get_irq_index(self, irq, lvl):
        if lvl == 2:
            offsets = self.__irq2_offsets
        elif lvl == 3:
            offsets = self.__irq3_offsets
        else:
            self.__log.error(f"Unsupported irq level: {lvl}")
        try:
            return offsets.index(irq)
        except ValueError:
            self.__log.error(
                f"IRQ {irq} not present in parent offsets ({offsets}). "
                + " Recheck interrupt configuration."
            )

    def get_swt_table_index(self, offset, irq):
        if not self.check_multi_level_interrupts():
            return irq - offset
        # Calculate index for multi level interrupts
        self.__log.debug('IRQ = ' + hex(irq))
        irq3 = (irq & self.int_lvl_masks[2]) >> (self.int_bits[0] + self.int_bits[1])
        irq2 = (irq & self.int_lvl_masks[1]) >> (self.int_bits[0])
        irq1 = irq & self.int_lvl_masks[0]
        # Figure out third level interrupt position
        if irq3:
            # Interrupt-matrix layout: the 3rd-level window of an
            # aggregator is packed densely, one slot per status-register bit
            # that is actually connected, so a flag's position is its rank
            # among the mask's set bits rather than the raw bit number.
            group = self.__l3_groups.get((irq1, irq2 - 1))
            if group is not None:
                bit = irq3 - 1
                irq3_pos = group["win_base"] + self.__popcount(group["mask"] & self.__bm(bit))
                self.__log.debug('IRQ_level = 3 (dense window)')
                self.__log.debug('IRQ_Indx = ' + str(irq3))
                self.__log.debug('IRQ_Pos  = ' + str(irq3_pos))
                return irq3_pos - offset
            list_index = self.get_irq_index(irq2 - 1, 3)
            irq3_pos = self.get_irq_baseoffset(3) + self.__max_irq_per[3] * list_index + irq3 - 1
            self.__log.debug('IRQ_level = 3')
            self.__log.debug('IRQ_Indx = ' + str(irq3))
            self.__log.debug('IRQ_Pos  = ' + str(irq3_pos))
            return irq3_pos - offset
        # Figure out second level interrupt position
        if irq2:
            # In the interrupt-matrix layout (e.g. the Espressif INTMUX) the L1
            # field is the hardware CPU line used only for routing, not a window
            # selector, and all sources share one window indexed by the source
            # number. Every other layout keeps the L1 line selecting the
            # per-aggregator window as before.
            if self.uses_matrix_layout():
                # A CPU line with a single level-2 source needs no software
                # dispatcher: place that lone source's ISR directly in the
                # line's 1st-level slot so the CPU vector calls it. Lines
                # shared by two or more sources, and lines hosting a 3rd-level
                # aggregator, keep the source-indexed window.
                if not self.line_needs_l2_dispatcher(irq1):
                    self.__log.debug('IRQ_level = 2 (lone source -> L1 slot)')
                    self.__log.debug('IRQ_Pos  = ' + str(irq1))
                    return irq1 - offset
                list_index = 0
            else:
                list_index = self.get_irq_index(irq1, 2)
            irq2_pos = self.get_irq_baseoffset(2) + self.__max_irq_per[2] * list_index + irq2 - 1
            self.__log.debug('IRQ_level = 2')
            self.__log.debug('IRQ_Indx = ' + str(irq2))
            self.__log.debug('IRQ_Pos  = ' + str(irq2_pos))
            return irq2_pos - offset
        # Figure out first level interrupt position
        self.__log.debug('IRQ_level = 1')
        self.__log.debug('IRQ_Indx = ' + str(irq1))
        self.__log.debug('IRQ_Pos  = ' + str(irq1))
        return irq1 - offset

    def note_multilevel_topology(self, interrupts):
        """Pre-pass: derive the interrupt topology of the flat matrix layout.

        Only meaningful under CONFIG_INTERRUPT_MATRIX_LAYOUT (e.g. the Espressif
        INTMUX), where the 1st-level field of an encoded IRQ is a routing line
        rather than a window selector. Left empty otherwise, so every other
        platform keeps the fixed-width placement untouched.

        Two things come out of it. Which CPU lines need the 2nd-level software
        dispatcher (see line_needs_l2_dispatcher), and which level-2 sources are
        really 3rd-level aggregators - a source carrying two or more handlers
        that the devicetree has split into per-flag leaves. Each aggregator's
        status-register mask is the OR of the bits its leaves declare, and its
        window is packed densely, one slot per connected bit.

        `interrupts` entries carry the encoded IRQ at index 0 and flags at
        index 1 (both parser tuple shapes agree on this).
        """
        self.__l2_line_srcs = {}
        self.__l3_groups = {}
        self.__l3_by_l2_slot = {}
        if not self.uses_matrix_layout():
            return

        # (line, src) -> number of handlers connected straight to the source
        l2_direct = {}

        for entry in interrupts:
            irq, flags = entry[0], entry[1]
            if self.test_isr_direct(flags):
                continue
            irq3 = (irq & self.int_lvl_masks[2]) >> (self.int_bits[0] + self.int_bits[1])
            irq2 = (irq & self.int_lvl_masks[1]) >> self.int_bits[0]
            if not irq2:
                continue
            key = (irq & self.int_lvl_masks[0], irq2 - 1)
            self.__l2_line_srcs.setdefault(key[0], set()).add(key[1])
            if irq3:
                bits = self.__l3_groups.setdefault(key, set())
                bit = irq3 - 1
                if bit in bits:
                    if self.is_l3_catch_all(bit):
                        self.__log.error(
                            f"Level-2 source {key[1]} (CPU line {key[0]}) declares more"
                            " than one catch-all level-3 leaf. An aggregator may have"
                            " at most one, because a catch-all claims no status bit and"
                            " so nothing could tell two of them apart."
                        )
                    else:
                        self.__log.error(
                            f"Level-3 flag {bit} of source {key[1]} (CPU line {key[0]})"
                            " is registered twice. Two handlers cannot share one"
                            " status-register bit."
                        )
                bits.add(bit)
            else:
                l2_direct[key] = l2_direct.get(key, 0) + 1

        for key, count in l2_direct.items():
            if count > 1:
                self.__log.error(
                    f"{count} handlers registered on level-2 source {key[1]}"
                    f" (CPU line {key[0]}). Two or more handlers on one source is"
                    " 3rd-level interrupt handling: declare an"
                    " \"espressif,esp32-l3-intc\" node for the source and give each"
                    " handler its own status-register bit."
                )
            if key in self.__l3_groups:
                self.__log.error(
                    f"Level-2 source {key[1]} (CPU line {key[0]}) is a 3rd-level"
                    " aggregator but also has a handler connected directly to it."
                    " Connect every handler through the aggregator instead."
                )

        if not self.__l3_groups:
            return

        # Dense window allocation. Sorting by (source, line) keeps the layout a
        # function of the devicetree alone - link order must not move a window.
        base = self.get_irq_baseoffset(3)
        if base is None:
            self.__log.error(
                "3rd-level interrupts are declared but CONFIG_3RD_LEVEL_INTERRUPTS is not enabled."
            )

        # The level-2 and level-3 regions are positioned by two independent
        # Kconfig values, so nothing but this check stops them overlapping. An
        # overlap is near-invisible: both regions are sparsely occupied, so the
        # build succeeds until some day a level-2 source lands on a slot a
        # level-3 flag already took, and then it surfaces as a bare "multiple
        # registrations at table_index N" rather than as a bad offset.
        l2_end = self.get_irq_baseoffset(2) + (
            self.get_sym("CONFIG_NUM_2ND_LEVEL_AGGREGATORS") * self.__max_irq_per[2]
        )
        if base < l2_end:
            self.__log.error(
                f"CONFIG_3RD_LVL_ISR_TBL_OFFSET ({base}) overlaps the level-2 window,"
                f" which ends at {l2_end} (CONFIG_2ND_LVL_ISR_TBL_OFFSET"
                f" {self.get_irq_baseoffset(2)} + CONFIG_NUM_2ND_LEVEL_AGGREGATORS"
                f" {self.get_sym('CONFIG_NUM_2ND_LEVEL_AGGREGATORS')} *"
                f" CONFIG_MAX_IRQ_PER_2ND_LEVEL_AGGREGATOR {self.__max_irq_per[2]})."
                f" Set CONFIG_3RD_LVL_ISR_TBL_OFFSET to {l2_end} or more."
            )

        win_base = base
        for index, key in enumerate(sorted(self.__l3_groups, key=lambda k: (k[1], k[0]))):
            bits = self.__l3_groups[key]
            # The catch-all leaf owns no status bit, so it stays out of the mask
            # and takes the slot immediately after the masked ones. len(bits)
            # therefore still sizes the window: popcount(mask) + the catch-all.
            catch_all = any(self.is_l3_catch_all(bit) for bit in bits)
            self.__l3_groups[key] = {
                "index": index,
                "bits": bits,
                "mask": sum(1 << bit for bit in bits if not self.is_l3_catch_all(bit)),
                "catch_all": catch_all,
                "win_base": win_base,
            }
            self.__l3_by_l2_slot[self.get_irq_baseoffset(2) + key[1]] = self.__l3_groups[key]
            win_base += len(bits)

        max_irq_per = self.__max_irq_per[3]
        for key, group in self.__l3_groups.items():
            # The catch-all sentinel is deliberately one past the limit, so bound
            # the real bits only - group["mask"] already excludes it.
            bits = [bit for bit in group["bits"] if not self.is_l3_catch_all(bit)]
            if max_irq_per and bits and max(bits) >= max_irq_per:
                self.__log.error(
                    f"Level-3 flag {max(bits)} of source {key[1]} exceeds"
                    f" CONFIG_MAX_IRQ_PER_3RD_LEVEL_AGGREGATOR ({max_irq_per})."
                )

    def __popcount(self, value):
        return bin(value).count("1")

    def get_intlist_snames(self):
        return self.args.intlist_section

    def test_isr_direct(self, flags):
        return flags & self.__ISR_FLAG_DIRECT

    def get_sym_from_addr(self, addr):
        for key, value in self.__syms.items():
            if addr == value:
                return key
        return None

    def get_sym(self, name):
        return self.__syms.get(name)

    def check_sym(self, name):
        return name in self.__syms

    def check_multi_level_interrupts(self):
        return self.check_sym("CONFIG_MULTI_LEVEL_INTERRUPTS")

    def check_shared_interrupts(self):
        return self.check_sym("CONFIG_SHARED_INTERRUPTS")

    def check_64b(self):
        return self.check_sym("CONFIG_64BIT")


def get_symbols(obj):
    for section in obj.iter_sections():
        if isinstance(section, SymbolTableSection):
            return {sym.name: sym.entry.st_value for sym in section.iter_symbols()}

    log.error("Could not find symbol table")


def read_intList_sect(elfobj, snames):
    """
    Load the raw intList section data in a form of byte array.
    """
    intList_sect = None

    for sname in snames:
        intList_sect = elfobj.get_section_by_name(sname)
        if intList_sect is not None:
            log.debug(f"Found intlist section: \"{sname}\"")
            break

    if intList_sect is None:
        log.error("Cannot find the intlist section!")

    intdata = intList_sect.data()

    return intdata


def parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument(
        "-e",
        "--big-endian",
        action="store_true",
        help="Target encodes data in big-endian format (little endian is the default)",
    )
    parser.add_argument(
        "-d", "--debug", action="store_true", help="Print additional debugging information"
    )
    parser.add_argument("-o", "--output-source", required=True, help="Output source file")
    parser.add_argument(
        "-l",
        "--linker-output-files",
        nargs=2,
        metavar=("vector_table_link", "software_interrupt_link"),
        help="Output linker files. "
        "Used only if CONFIG_ISR_TABLES_LOCAL_DECLARATION is enabled. "
        "In other case empty file would be generated.",
    )
    parser.add_argument("-k", "--kernel", required=True, help="Zephyr kernel image")
    parser.add_argument("-s", "--sw-isr-table", action="store_true", help="Generate SW ISR table")
    parser.add_argument("-V", "--vector-table", action="store_true", help="Generate vector table")
    parser.add_argument(
        "-i",
        "--intlist-section",
        action="append",
        required=True,
        help="The name of the section to search for the interrupt data. "
        "This is accumulative argument. The first section found would be used.",
    )

    return parser.parse_args()


def main():
    args = parse_args()
    # Configure logging as soon as possible
    log.set_debug(args.debug)

    with open(args.kernel, "rb") as fp:
        kernel = ELFFile(fp)
        config = gen_isr_config(args, get_symbols(kernel), log)
        intlist_data = read_intList_sect(kernel, config.get_intlist_snames())

        if config.check_sym("CONFIG_ISR_TABLES_LOCAL_DECLARATION"):
            parser_module = importlib.import_module('gen_isr_tables_parser_local')
            parser = parser_module.gen_isr_parser(intlist_data, config, log)
        else:
            parser_module = importlib.import_module('gen_isr_tables_parser_carrays')
            parser = parser_module.gen_isr_parser(intlist_data, config, log)

    with open(args.output_source, "w") as fp:
        parser.write_source(fp)

    if args.linker_output_files is not None:
        with (
            open(args.linker_output_files[0], "w") as fp_vt,
            open(args.linker_output_files[1], "w") as fp_swi,
        ):
            if hasattr(parser, 'write_linker_vt'):
                parser.write_linker_vt(fp_vt)
            else:
                log.debug("Chosen parser does not support vector table linker file")
                fp_vt.write('/* Empty */\n')
            if hasattr(parser, 'write_linker_swi'):
                parser.write_linker_swi(fp_swi)
            else:
                log.debug("Chosen parser does not support software interrupt linker file")
                fp_swi.write('/* Empty */\n')


if __name__ == "__main__":
    main()
