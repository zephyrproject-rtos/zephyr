#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import re
import subprocess
import sys

from twisterlib.error import TwisterRuntimeError

logger = logging.getLogger('twister')


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
        "_static_thread_data_area",
        "k_timer_area",
        "k_mem_slab_area",
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
        'log_backends_sections',
        'log_dynamic_sections',
        'log_const_sections',
        "app_smem",
        'shell_root_cmds_sections',
        'log_const_sections',
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
        "k_object_assignment_area",
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

    # Variable below is stored for calculating size using build.log
    USEFUL_LINES_AMOUNT = 4

    def __init__(self, elf_filename: str,\
        extra_sections: list[str],\
        buildlog_filepath: str = '',\
        generate_warning: bool = True):
        """Constructor

        @param elf_filename (str) Path to the output binary
            parsed by objdump to determine section sizes.
        @param extra_sections (list[str]) List of extra,
            unexpected sections, which Twister should not
            report as error and not include in the
            size calculation.
        @param buildlog_filepath (str, default: '') Path to the
            output build.log
        @param generate_warning (bool, default: True) Twister should
            (or not) warning about missing the information about
            footprint.
        """
        self.elf_filename = elf_filename
        self.buildlog_filename = buildlog_filepath
        self.sections = []
        self.used_rom = 0
        self.used_ram = 0
        self.available_ram = 0
        self.available_rom = 0
        self.extra_sections = extra_sections
        self.is_xip = True
        self.generate_warning = generate_warning

        self._calculate_sizes()

    def size_report(self):
        print(self.elf_filename)
        print("SECTION NAME             VMA        LMA     SIZE  HEX SZ TYPE")
        for v in self.sections:
            print(
                f'{v["name"]:<17} {v["virt_addr"]:#010x} {v["load_addr"]:#010x}'
                f' {v["size"]:>8} {v["size"]:#07x} {v["type"]:<7}'
            )

        print(f"Totals: {self.used_rom} bytes (ROM), {self.used_ram} bytes (RAM)")
        print("")

    def get_used_ram(self):
        """Get the amount of RAM the application will use up on the device

        @return amount of RAM, in bytes
        """
        return self.used_ram

    def get_used_rom(self):
        """Get the size of the data that this application uses on device's flash

        @return amount of ROM, in bytes
        """
        return self.used_rom

    def get_available_ram(self) -> int:
        """Get the total available RAM.

        @return total available RAM, in bytes (int)
        """
        return self.available_ram

    def get_available_rom(self) -> int:
        """Get the total available ROM.

        @return total available ROM, in bytes (int)
        """
        return self.available_rom

    def _calculate_sizes(self):
        """ELF file is analyzed, even if the option to read memory
        footprint from the build.log file is set.
        This is to detect potential problems contained in
        unrecognized sections of the file.
        """
        self._analyze_elf_file()
        if self.buildlog_filename.endswith("build.log"):
            self._get_footprint_from_buildlog()

    def _check_elf_file(self) -> None:
        # Make sure this is an ELF binary
        with open(self.elf_filename, "rb") as f:
            magic = f.read(4)

        try:
            if magic != b'\x7fELF':
                raise TwisterRuntimeError(f"{self.elf_filename} is not an ELF binary")
        except Exception as e:
            print(str(e))
            sys.exit(2)

    def _check_is_xip(self) -> None:
        # Search for CONFIG_XIP in the ELF's list of symbols using NM and AWK.
        # GREP can not be used as it returns an error if the symbol is not
        # found.
        is_xip_command = "nm " + self.elf_filename + \
                         " | awk '/CONFIG_XIP/ { print $3 }'"
        is_xip_output = subprocess.check_output(
            is_xip_command, shell=True, stderr=subprocess.STDOUT).decode(
            "utf-8").strip()
        try:
            if is_xip_output.endswith("no symbols"):
                raise TwisterRuntimeError(f"{self.elf_filename} has no symbol information")
        except Exception as e:
            print(str(e))
            sys.exit(2)

        self.is_xip = len(is_xip_output) != 0

    def _get_info_elf_sections(self) -> None:
        """Calculate RAM and ROM usage and information about issues by section"""
        objdump_command = "objdump -h " + self.elf_filename
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

            # If build.log file exists, check errors (unrecognized sections
            # in ELF file).
            if self.buildlog_filename:
                if name in SizeCalculator.alloc_sections or \
                   name in SizeCalculator.rw_sections or \
                   name in SizeCalculator.ro_sections:
                    continue
                else:
                    stype = "unknown"
            else:
                if name in SizeCalculator.alloc_sections:
                    self.used_ram += size
                    stype = "alloc"
                elif name in SizeCalculator.rw_sections:
                    self.used_ram += size
                    self.used_rom += size
                    stype = "rw"
                elif name in SizeCalculator.ro_sections:
                    self.used_rom += size
                    if not self.is_xip:
                        self.used_ram += size
                    stype = "ro"
                else:
                    stype = "unknown"

            self.sections.append({"name": name, "load_addr": load_addr,
                                  "size": size, "virt_addr": virt_addr,
                                  "type": stype})

    def _analyze_elf_file(self) -> None:
        self._check_elf_file()
        self._check_is_xip()
        self._get_info_elf_sections()

    def _get_buildlog_file_content(self) -> list[str]:
        """Get content of the build.log file.

        @return Content of the build.log file (list[str])
        """
        if os.path.exists(path=self.buildlog_filename):
            with open(file=self.buildlog_filename) as file:
                file_content = file.readlines()
        else:
            if self.generate_warning:
                logger.error(
                    msg="Incorrect path to build.log file to analyze footprints."
                       f" Please check the path {self.buildlog_filename}."
                )
            file_content = []
        return file_content

    def _find_offset_of_last_pattern_occurrence(self, file_content: list[str]) -> int:
        """Find the offset from which the information about the memory footprint is read.

        @param file_content (list[str]) Content of build.log.
        @return Offset with information about the memory footprint (int)
        """
        result = -1
        if len(file_content) == 0:
            logger.warning("Build.log file is empty.")
        else:
            # Pattern to first line with information about memory footprint
            PATTERN_SEARCHED_LINE = "Memory region"
            # Check the file in reverse order.
            for idx, line in enumerate(reversed(file_content)):
                # Condition is fulfilled if the pattern matches with the start of the line.
                if re.match(pattern=PATTERN_SEARCHED_LINE, string=line):
                    result = idx + 1
                    break
        # If the file does not contain information about memory footprint, the warning is raised.
        if result == -1:
            logger.warning(
                msg="Information about memory footprint for this test configuration is not found."
                   f" Please check file {self.buildlog_filename}."
            )
        return result

    def _get_lines_with_footprint(self, start_offset: int, file_content: list[str]) -> list[str]:
        """Get lines from the file with a memory footprint.

        @param start_offset (int) Offset with the memory footprint's first line.
        @param file_content (list[str]) Content of the build.log file.
        @return Lines with information about memory footprint (list[str])
        """
        if len(file_content) == 0:
            result = []
        else:
            if start_offset > len(file_content) or start_offset <= 0:
                info_line_idx_start = len(file_content) - 1
            else:
                info_line_idx_start = len(file_content) - start_offset

            info_line_idx_stop = info_line_idx_start + self.USEFUL_LINES_AMOUNT
            if info_line_idx_stop > len(file_content):
                info_line_idx_stop = len(file_content)

            result = file_content[info_line_idx_start:info_line_idx_stop]
        return result

    def _clear_whitespaces_from_lines(self, text_lines: list[str]) -> list[str]:
        """Clear text lines from whitespaces.

        @param text_lines (list[str]) Lines with useful information.
        @return  Cleared text lines with information (list[str])
        """
        return [line.strip("\n").rstrip("%") for line in text_lines] if text_lines else []

    def _divide_text_lines_into_columns(self, text_lines: list[str]) -> list[list[str]]:
        """Divide lines of text into columns.

        @param lines (list[list[str]]) Lines with information about memory footprint.
        @return Lines divided into columns (list[list[str]])
        """
        if text_lines:
            result = []
            PATTERN_SPLIT_COLUMNS = "  +"
            for line in text_lines:
                line = [
                    column.rstrip(":") for column in re.split(
                        pattern=PATTERN_SPLIT_COLUMNS,
                        string=line
                    )
                ]
                result.append(list(filter(None, line)))
        else:
            result = [[]]

        return result

    def _unify_prefixes_on_all_values(self, data_lines: list[list[str]]) -> list[list[str]]:
        """Convert all values in the table to unified order of magnitude.

        @param data_lines (list[list[str]]) Lines with information about memory footprint.
        @return Lines with unified values (list[list[str]])
        """
        if len(data_lines) != self.USEFUL_LINES_AMOUNT:
            data_lines = [[]]
            if self.generate_warning:
                logger.warning(
                    msg="Incomplete information about memory footprint."
                       f" Please check file {self.buildlog_filename}"
                )
        else:
            for idx, line in enumerate(data_lines):
                # Line with description of the columns
                if idx == 0:
                    continue
                line_to_replace = list(map(self._binary_prefix_converter, line))
                data_lines[idx] = line_to_replace

        return data_lines

    def _binary_prefix_converter(self, value: str) -> str:
        """Convert specific value to particular prefix.

        @param value (str) Value to convert.
        @return Converted value to output prefix (str)
        """
        PATTERN_VALUE = r"([0-9]?\s.?B\Z)"

        if not re.search(pattern=PATTERN_VALUE, string=value):
            converted_value = value.rstrip()
        else:
            PREFIX_POWER = {'B': 0, 'KB': 10, 'MB': 20, 'GB': 30}
            DEFAULT_DATA_PREFIX = 'B'

            data_parts = value.split()
            numeric_value = int(data_parts[0])
            unit = data_parts[1]
            shift = PREFIX_POWER.get(unit, 0) - PREFIX_POWER.get(DEFAULT_DATA_PREFIX, 0)
            unit_predictor = pow(2, shift)
            converted_value = str(numeric_value * unit_predictor)
        return converted_value

    def _create_data_table(self) -> list[list[str]]:
        """Create table with information about memory footprint.

        @return Table with information about memory usage (list[list[str]])
        """
        file_content = self._get_buildlog_file_content()
        data_line_start_idx = self._find_offset_of_last_pattern_occurrence(
            file_content=file_content
        )

        if data_line_start_idx < 0:
            data_from_content = [[]]
        else:
            # Clean lines and separate information to columns
            information_lines = self._get_lines_with_footprint(
                start_offset=data_line_start_idx,
                file_content=file_content
            )
            information_lines = self._clear_whitespaces_from_lines(text_lines=information_lines)
            data_from_content = self._divide_text_lines_into_columns(text_lines=information_lines)
            data_from_content = self._unify_prefixes_on_all_values(data_lines=data_from_content)

        return data_from_content

    def _get_footprint_from_buildlog(self) -> None:
        """Get memory footprint from build.log"""
        data_from_file = self._create_data_table()

        if data_from_file == [[]] or not data_from_file:
            self.used_ram = 0
            self.used_rom = 0
            self.available_ram = 0
            self.available_rom = 0
            if self.generate_warning:
                logger.warning(
                    msg="Missing information about memory footprint."
                       f" Check file {self.buildlog_filename}."
                )
        else:
            ROW_RAM_IDX = 2
            ROW_ROM_IDX = 1
            COLUMN_USED_SIZE_IDX = 1
            COLUMN_AVAILABLE_SIZE_IDX = 2

            self.used_ram = int(data_from_file[ROW_RAM_IDX][COLUMN_USED_SIZE_IDX])
            self.used_rom = int(data_from_file[ROW_ROM_IDX][COLUMN_USED_SIZE_IDX])
            self.available_ram = int(data_from_file[ROW_RAM_IDX][COLUMN_AVAILABLE_SIZE_IDX])
            self.available_rom = int(data_from_file[ROW_ROM_IDX][COLUMN_AVAILABLE_SIZE_IDX])
