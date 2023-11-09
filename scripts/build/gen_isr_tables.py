#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
# Copyright (c) 2018 Foundries.io
# Copyright (c) 2023 Nordic Semiconductor NA
#
# SPDX-License-Identifier: Apache-2.0
#

import argparse
import struct
import sys
import os
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection


class gen_isr_log:

    def __init__(self, debug = False):
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
    """All the constants and configuration gathered in single class for readability.
    """
    # Constants
    __ISR_FLAG_DIRECT = 1 << 0
    __swt_spurious_handler = "((uintptr_t)&z_irq_spurious)"
    __swt_shared_handler = "((uintptr_t)&z_shared_isr)"
    __vt_spurious_handler = "z_irq_spurious"
    __vt_irq_handler = "_isr_wrapper"

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

        if self.check_multi_level_interrupts():
            self.__max_irq_per = self.get_sym("CONFIG_MAX_IRQ_PER_AGGREGATOR")

            self.__int_bits[0] = self.get_sym("CONFIG_1ST_LEVEL_INTERRUPT_BITS")
            self.__int_bits[1] = self.get_sym("CONFIG_2ND_LEVEL_INTERRUPT_BITS")
            self.__int_bits[2] = self.get_sym("CONFIG_3RD_LEVEL_INTERRUPT_BITS")

            if sum(self.int_bits) > 32:
                raise ValueError("Too many interrupt bits")

            self.__int_lvl_masks[0] = self.__bm(self.int_bits[0])
            self.__int_lvl_masks[1] = self.__bm(self.int_bits[1]) << self.int_bits[0]
            self.__int_lvl_masks[2] = self.__bm(self.int_bits[2]) << (self.int_bits[0] + self.int_bits[1])

            self.__log.debug("Level    Bits        Bitmask")
            self.__log.debug("----------------------------")
            for i in range(3):
                bitmask_str = "0x" + format(self.__int_lvl_masks[i], '08X')
                self.__log.debug(f"{i + 1:>5} {self.__int_bits[i]:>7} {bitmask_str:>14}")

            if self.check_sym("CONFIG_2ND_LEVEL_INTERRUPTS"):
                num_aggregators = self.get_sym("CONFIG_NUM_2ND_LEVEL_AGGREGATORS")
                self.__irq2_baseoffset = self.get_sym("CONFIG_2ND_LVL_ISR_TBL_OFFSET")
                self.__irq2_offsets = [self.get_sym('CONFIG_2ND_LVL_INTR_{}_OFFSET'.
                                                  format(str(i).zfill(2))) for i in
                                     range(num_aggregators)]

                self.__log.debug('2nd level offsets: {}'.format(self.__irq2_offsets))

                if self.check_sym("CONFIG_3RD_LEVEL_INTERRUPTS"):
                    num_aggregators = self.get_sym("CONFIG_NUM_3RD_LEVEL_AGGREGATORS")
                    self.__irq3_baseoffset = self.get_sym("CONFIG_3RD_LVL_ISR_TBL_OFFSET")
                    self.__irq3_offsets = [self.get_sym('CONFIG_3RD_LVL_INTR_{}_OFFSET'.
                                                      format(str(i).zfill(2))) for i in
                                         range(num_aggregators)]

                    self.__log.debug('3rd level offsets: {}'.format(self.__irq3_offsets))

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
    def vt_default_handler(self):
        return self.__vt_default_handler

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
        self.__log.error("Unsupported irq level: {}".format(lvl))

    def get_irq_index(self, irq, lvl):
        if lvl == 2:
            offsets = self.__irq2_offsets
        elif lvl == 3:
            offsets = self.__irq3_offsets
        else:
            self.__log.error("Unsupported irq level: {}".format(lvl))
        try:
            return offsets.index(irq)
        except ValueError:
            self.__log.error("IRQ {} not present in parent offsets ({}). ".
                             format(irq, offsets) +
                             " Recheck interrupt configuration.")

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
            list_index = self.get_irq_index(irq2, 3)
            irq3_pos = self.get_irq_baseoffset(3) + self.__max_irq_per * list_index + irq3 - 1
            self.__log.debug('IRQ_level = 3')
            self.__log.debug('IRQ_Indx = ' + str(irq3))
            self.__log.debug('IRQ_Pos  = ' + str(irq3_pos))
            return irq3_pos - offset
        # Figure out second level interrupt position
        if irq2:
            list_index = self.get_irq_index(irq1, 2)
            irq2_pos = self.get_irq_baseoffset(2) + self.__max_irq_per * list_index + irq2 - 1
            self.__log.debug('IRQ_level = 2')
            self.__log.debug('IRQ_Indx = ' + str(irq2))
            self.__log.debug('IRQ_Pos  = ' + str(irq2_pos))
            return irq2_pos - offset
        # Figure out first level interrupt position
        self.__log.debug('IRQ_level = 1')
        self.__log.debug('IRQ_Indx = ' + str(irq1))
        self.__log.debug('IRQ_Pos  = ' + str(irq1))
        return irq1 - offset

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


class gen_isr_parser:
    source_header = """
/* AUTO-GENERATED by gen_isr_tables.py, do not edit! */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/arch/cpu.h>

typedef void (* ISR)(const void *);
"""

    source_assembly_header = """
#ifndef ARCH_IRQ_VECTOR_JUMP_CODE
#error "ARCH_IRQ_VECTOR_JUMP_CODE not defined"
#endif
"""

    def __init__(self, intlist_data, config, log):
        """Initialize the parser.

        The function prepares parser to work.
        Parameters:
        - intlist_data: The binnary data from intlist section
        - config: The configuration object
        - log: The logging object, has to have error and debug methods
        """
        self.__config = config
        self.__log = log
        intlist = self.__read_intlist(intlist_data)
        self.__vt, self.__swt, self.__nv = self.__parse_intlist(intlist)

    def __read_intlist(self, intlist_data):
        """read a binary file containing the contents of the kernel's .intList
        section. This is an instance of a header created by
        include/zephyr/linker/intlist.ld:

         struct {
           uint32_t num_vectors;       <- typically CONFIG_NUM_IRQS
           struct _isr_list isrs[]; <- Usually of smaller size than num_vectors
        }

        Followed by instances of struct _isr_list created by IRQ_CONNECT()
        calls:

        struct _isr_list {
            /** IRQ line number */
            int32_t irq;
            /** Flags for this IRQ, see ISR_FLAG_* definitions */
            int32_t flags;
            /** ISR to call */
            void *func;
            /** Parameter for non-direct IRQs */
            const void *param;
        };
        """
        intlist = {}
        prefix = self.__config.endian_prefix()

        # Extract header and the rest of the data
        intlist_header_fmt = prefix + "II"
        header_sz = struct.calcsize(intlist_header_fmt)
        header_raw = struct.unpack_from(intlist_header_fmt, intlist_data, 0)
        self.__log.debug(str(header_raw))

        intlist["num_vectors"]    = header_raw[0]
        intlist["offset"]         = header_raw[1]
        intdata = intlist_data[header_sz:]

        # Extract information about interrupts
        if self.__config.check_64b():
            intlist_entry_fmt = prefix + "iiQQ"
        else:
            intlist_entry_fmt = prefix + "iiII"

        intlist["interrupts"] = [i for i in
                struct.iter_unpack(intlist_entry_fmt, intdata)]

        self.__log.debug("Configured interrupt routing")
        self.__log.debug("handler    irq flags param")
        self.__log.debug("--------------------------")

        for irq in intlist["interrupts"]:
            self.__log.debug("{0:<10} {1:<3} {2:<3}   {3}".format(
                hex(irq[2]), irq[0], irq[1], hex(irq[3])))

        return intlist

    def __parse_intlist(self, intlist):
        """All the intlist data are parsed into swt and vt arrays.

        The vt array is prepared for hardware interrupt table.
        Every entry in the selected position would contain None or the name of the function pointer
        (address or string).

        The swt is a little more complex. At every position it would contain an array of parameter and
        function pointer pairs. If CONFIG_SHARED_INTERRUPTS is enabled there may be more than 1 entry.
        If empty array is placed on selected position - it means that the application does not implement
        this interrupt.

        Parameters:
        - intlist: The preprocessed list of intlist section content (see read_intlist)

        Return:
        vt, swt - parsed vt and swt arrays (see function description above)
        """
        nvec = intlist["num_vectors"]
        offset = intlist["offset"]

        if nvec > pow(2, 15):
            raise ValueError('nvec is too large, check endianness.')

        self.__log.debug('offset is ' + str(offset))
        self.__log.debug('num_vectors is ' + str(nvec))

        # Set default entries in both tables
        if not(self.__config.args.sw_isr_table or self.__config.args.vector_table):
            self.__log.error("one or both of -s or -V needs to be specified on command line")
        if self.__config.args.vector_table:
            vt = [None for i in range(nvec)]
        else:
            vt = None
        if self.__config.args.sw_isr_table:
            swt = [[] for i in range(nvec)]
        else:
            swt = None

        # Process intlist and write to the tables created
        for irq, flags, func, param in intlist["interrupts"]:
            if not vt:
                error("Direct Interrupt %d declared with parameter 0x%x "
                      "but no vector table in use"
                      % (irq, param))
            if self.__config.test_isr_direct(flags):
                if param != 0:
                    self.__log.error("Direct irq %d declared, but has non-NULL parameter"
                            % irq)
                if not 0 <= irq - offset < len(vt):
                    self.__log.error("IRQ %d (offset=%d) exceeds the maximum of %d" %
                          (irq - offset, offset, len(vt) - 1))
                vt[irq - offset] = func
            else:
                # Regular interrupt
                if not swt:
                    self.__log.error("Regular Interrupt %d declared with parameter 0x%x "
                                     "but no SW ISR_TABLE in use"
                                     % (irq, param))

                table_index = self.__config.get_swt_table_index(offset, irq)

                if not 0 <= table_index < len(swt):
                    self.__log.error("IRQ %d (offset=%d) exceeds the maximum of %d" %
                                     (table_index, offset, len(swt) - 1))
                if self.__config.check_shared_interrupts():
                    lst = swt[table_index]
                    if (param, func) in lst:
                        self.__log.error("Attempting to register the same ISR/arg pair twice.")
                    if len(lst) >= self.__config.get_sym("CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS"):
                        self.__log.error(f"Reached shared interrupt client limit. Maybe increase"
                                         + f" CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS?")
                else:
                    if len(swt[table_index]) > 0:
                        self.__log.error(f"multiple registrations at table_index {table_index} for irq {irq} (0x{irq:x})"
                                         + f"\nExisting handler 0x{swt[table_index][0][1]:x}, new handler 0x{func:x}"
                                         + "\nHas IRQ_CONNECT or IRQ_DIRECT_CONNECT accidentally been invoked on the same irq multiple times?"
                        )
                swt[table_index].append((param, func))

        return vt, swt, nvec

    def __write_code_irq_vector_table(self, fp):
        fp.write(self.source_assembly_header)

        fp.write("void __irq_vector_table __attribute__((naked)) _irq_vector_table(void) {\n")
        for i in range(self.__nv):
            func = self.__vt[i]

            if func is None:
                func = self.__config.vt_default_handler

            if isinstance(func, int):
                func_as_string = self.__config.get_sym_from_addr(func)
            else:
                func_as_string = func

            fp.write("\t__asm(ARCH_IRQ_VECTOR_JUMP_CODE({}));\n".format(func_as_string))
        fp.write("}\n")

    def __write_address_irq_vector_table(self, fp):
        fp.write("uintptr_t __irq_vector_table _irq_vector_table[%d] = {\n" % self.__nv)
        for i in range(self.__nv):
            func = self.__vt[i]

            if func is None:
                func = self.__config.vt_default_handler

            if isinstance(func, int):
                fp.write("\t{},\n".format(func))
            else:
                fp.write("\t((uintptr_t)&{}),\n".format(func))

        fp.write("};\n")

    def __write_shared_table(self, fp):
        fp.write("struct z_shared_isr_table_entry __shared_sw_isr_table"
                " z_shared_sw_isr_table[%d] = {\n" % self.__nv)

        for i in range(self.__nv):
            if self.__swt[i] is None:
                client_num = 0
                client_list = None
            else:
                client_num = len(self.__swt[i])
                client_list = self.__swt[i]

            if client_num <= 1:
                fp.write("\t{ },\n")
            else:
                fp.write(f"\t{{ .client_num = {client_num}, .clients = {{ ")
                for j in range(0, client_num):
                    routine = client_list[j][1]
                    arg = client_list[j][0]

                    fp.write(f"{{ .isr = (ISR){ hex(routine) if isinstance(routine, int) else routine }, "
                            f".arg = (const void *){hex(arg)} }},")

                fp.write(" },\n},\n")

        fp.write("};\n")

    def write_source(self, fp):
        fp.write(self.source_header)

        if self.__config.check_shared_interrupts():
            self.__write_shared_table(fp)

        if self.__vt:
            if self.__config.check_sym("CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_ADDRESS"):
                self.__write_address_irq_vector_table(fp)
            elif self.__config.check_sym("CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_CODE"):
                self.__write_code_irq_vector_table(fp)
            else:
                self.__log.error("CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_{ADDRESS,CODE} not set")

        if not self.__swt:
            return

        fp.write("struct _isr_table_entry __sw_isr_table _sw_isr_table[%d] = {\n"
                % self.__nv)

        level2_offset = self.__config.get_irq_baseoffset(2)
        level3_offset = self.__config.get_irq_baseoffset(3)

        for i in range(self.__nv):
            if len(self.__swt[i]) == 0:
                # Not used interrupt
                param = "0x0"
                func = self.__config.swt_spurious_handler
            elif len(self.__swt[i]) == 1:
                # Single interrupt
                param = "{0:#x}".format(self.__swt[i][0][0])
                func = self.__swt[i][0][1]
            else:
                # Shared interrupt
                param = "&z_shared_sw_isr_table[{0}]".format(i)
                func = self.__config.swt_shared_handler

            if isinstance(func, int):
                func_as_string = "{0:#x}".format(func)
            else:
                func_as_string = func

            if level2_offset is not None and i == level2_offset:
                fp.write("\t/* Level 2 interrupts start here (offset: {}) */\n".
                         format(level2_offset))
            if level3_offset is not None and i == level3_offset:
                fp.write("\t/* Level 3 interrupts start here (offset: {}) */\n".
                         format(level3_offset))

            fp.write("\t{{(const void *){0}, (ISR){1}}}, /* {2} */\n".format(param, func_as_string, i))
        fp.write("};\n")


def get_symbols(obj):
    for section in obj.iter_sections():
        if isinstance(section, SymbolTableSection):
            return {sym.name: sym.entry.st_value
                    for sym in section.iter_symbols()}

    log.error("Could not find symbol table")

def read_intList_sect(elfobj, snames):
    """
    Load the raw intList section data in a form of byte array.
    """
    intList_sect = None

    for sname in snames:
        intList_sect = elfobj.get_section_by_name(sname)
        if intList_sect is not None:
            log.debug("Found intlist section: \"{}\"".format(sname))
            break

    if intList_sect is None:
        log.error("Cannot find the intlist section!")

    intdata = intList_sect.data()

    return intdata

def parse_args():
    parser = argparse.ArgumentParser(description=__doc__,
            formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)

    parser.add_argument("-e", "--big-endian", action="store_true",
            help="Target encodes data in big-endian format (little endian is "
                 "the default)")
    parser.add_argument("-d", "--debug", action="store_true",
            help="Print additional debugging information")
    parser.add_argument("-o", "--output-source", required=True,
            help="Output source file")
    parser.add_argument("-k", "--kernel", required=True,
            help="Zephyr kernel image")
    parser.add_argument("-s", "--sw-isr-table", action="store_true",
            help="Generate SW ISR table")
    parser.add_argument("-V", "--vector-table", action="store_true",
            help="Generate vector table")
    parser.add_argument("-i", "--intlist-section", action="append", required=True,
            help="The name of the section to search for the interrupt data. "
                 "This is accumulative argument. The first section found would be used.")

    return parser.parse_args()

def main():
    args = parse_args()
    # Configure logging as soon as possible
    log.set_debug(args.debug)

    with open(args.kernel, "rb") as fp:
        kernel = ELFFile(fp)
        config = gen_isr_config(args, get_symbols(kernel), log)
        intlist_data = read_intList_sect(kernel, config.get_intlist_snames())

        parser = gen_isr_parser(intlist_data, config, log)

    with open(args.output_source, "w") as fp:
        parser.write_source(fp)


if __name__ == "__main__":
    main()
