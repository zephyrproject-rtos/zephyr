#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import subprocess
import sys
from twisterlib.error import TwisterRuntimeError


class SizeCalculator:
    alloc_sections = [
        "bss",
        "noinit",
        "app_bss",
        "app_noinit",
        "ccm_bss",
        "ccm_noinit"
    ]

    rw_sections = [
        "datas",
        "initlevel",
        "exceptions",
        "initshell",
        "_static_thread_data_area",
        "k_timer_area",
        "k_mem_slab_area",
        "k_mem_pool_area",
        "sw_isr_table",
        "k_sem_area",
        "k_mutex_area",
        "app_shmem_regions",
        "_k_fifo_area",
        "_k_lifo_area",
        "k_stack_area",
        "k_msgq_area",
        "k_mbox_area",
        "k_pipe_area",
        "net_if_area",
        "net_if_dev_area",
        "net_l2_area",
        "net_l2_data",
        "k_queue_area",
        "_net_buf_pool_area",
        "app_datas",
        "kobject_data",
        "mmu_tables",
        "app_pad",
        "priv_stacks",
        "ccm_data",
        "usb_descriptor",
        "usb_data", "usb_bos_desc",
        "uart_mux",
        'log_backends_sections',
        'log_dynamic_sections',
        'log_const_sections',
        "app_smem",
        'shell_root_cmds_sections',
        'log_const_sections',
        "font_entry_sections",
        "priv_stacks_noinit",
        "_GCOV_BSS_SECTION_NAME",
        "gcov",
        "nocache",
        "devices",
        "k_heap_area",
    ]

    # These get copied into RAM only on non-XIP
    ro_sections = [
        "rom_start",
        "text",
        "ctors",
        "init_array",
        "reset",
        "z_object_assignment_area",
        "rodata",
        "net_l2",
        "vector",
        "sw_isr_table",
        "settings_handler_static_area",
        "bt_l2cap_fixed_chan_area",
        "bt_l2cap_br_fixed_chan_area",
        "bt_gatt_service_static_area",
        "vectors",
        "net_socket_register_area",
        "net_ppp_proto",
        "shell_area",
        "tracing_backend_area",
        "ppp_protocol_handler_area",
    ]

    def __init__(self, filename, extra_sections):
        """Constructor

        @param filename Path to the output binary
            The <filename> is parsed by objdump to determine section sizes
        """
        # Make sure this is an ELF binary
        with open(filename, "rb") as f:
            magic = f.read(4)

        try:
            if magic != b'\x7fELF':
                raise TwisterRuntimeError("%s is not an ELF binary" % filename)
        except Exception as e:
            print(str(e))
            sys.exit(2)

        # Search for CONFIG_XIP in the ELF's list of symbols using NM and AWK.
        # GREP can not be used as it returns an error if the symbol is not
        # found.
        is_xip_command = "nm " + filename + \
                         " | awk '/CONFIG_XIP/ { print $3 }'"
        is_xip_output = subprocess.check_output(
            is_xip_command, shell=True, stderr=subprocess.STDOUT).decode(
            "utf-8").strip()
        try:
            if is_xip_output.endswith("no symbols"):
                raise TwisterRuntimeError("%s has no symbol information" % filename)
        except Exception as e:
            print(str(e))
            sys.exit(2)

        self.is_xip = (len(is_xip_output) != 0)

        self.filename = filename
        self.sections = []
        self.rom_size = 0
        self.ram_size = 0
        self.extra_sections = extra_sections

        self._calculate_sizes()


    def size_report(self):
        print(self.filename)
        print("SECTION NAME             VMA        LMA     SIZE  HEX SZ TYPE")
        for v in self.sections:
            print("%-17s 0x%08x 0x%08x %8d 0x%05x %-7s" %
                        (v["name"], v["virt_addr"], v["load_addr"], v["size"], v["size"],
                        v["type"]))

        print("Totals: %d bytes (ROM), %d bytes (RAM)" %
                    (self.rom_size, self.ram_size))
        print("")

    def get_ram_size(self):
        """Get the amount of RAM the application will use up on the device

        @return amount of RAM, in bytes
        """
        return self.ram_size

    def get_rom_size(self):
        """Get the size of the data that this application uses on device's flash

        @return amount of ROM, in bytes
        """
        return self.rom_size

    def unrecognized_sections(self):
        """Get a list of sections inside the binary that weren't recognized

        @return list of unrecognized section names
        """
        slist = []
        for v in self.sections:
            if not v["recognized"]:
                slist.append(v["name"])
        return slist

    def _calculate_sizes(self):
        """ Calculate RAM and ROM usage by section """
        objdump_command = "objdump -h " + self.filename
        objdump_output = subprocess.check_output(
            objdump_command, shell=True).decode("utf-8").splitlines()

        for line in objdump_output:
            words = line.split()

            if not words:  # Skip lines that are too short
                continue

            index = words[0]
            if not index[0].isdigit():  # Skip lines that do not start
                continue  # with a digit

            name = words[1]  # Skip lines with section names
            if name[0] == '.':  # starting with '.'
                continue

            # TODO this doesn't actually reflect the size in flash or RAM as
            # it doesn't include linker-imposed padding between sections.
            # It is close though.
            size = int(words[2], 16)
            if size == 0:
                continue

            load_addr = int(words[4], 16)
            virt_addr = int(words[3], 16)

            # Add section to memory use totals (for both non-XIP and XIP scenarios)
            # Unrecognized section names are not included in the calculations.
            recognized = True
            if name in SizeCalculator.alloc_sections:
                self.ram_size += size
                stype = "alloc"
            elif name in SizeCalculator.rw_sections:
                self.ram_size += size
                self.rom_size += size
                stype = "rw"
            elif name in SizeCalculator.ro_sections:
                self.rom_size += size
                if not self.is_xip:
                    self.ram_size += size
                stype = "ro"
            else:
                stype = "unknown"
                if name not in self.extra_sections:
                    recognized = False

            self.sections.append({"name": name, "load_addr": load_addr,
                                  "size": size, "virt_addr": virt_addr,
                                  "type": stype, "recognized": recognized})
