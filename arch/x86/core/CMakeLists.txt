# Copyright (c) 2019 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if (CONFIG_COVERAGE)
  toolchain_cc_coverage()
endif ()

zephyr_library_sources(cpuhalt.c)
zephyr_library_sources(memmap.c)
zephyr_library_sources(prep_c.c)

zephyr_library_sources_if_kconfig(pcie.c)
zephyr_library_sources_if_kconfig(reboot_rst_cnt.c)
zephyr_library_sources_if_kconfig(multiboot.c)
zephyr_library_sources_if_kconfig(acpi.c)
zephyr_library_sources_if_kconfig(x86_mmu.c)

zephyr_library_sources_ifdef(CONFIG_X86_VERY_EARLY_CONSOLE early_serial.c)

if(CONFIG_X86_64)
  include(intel64.cmake)
else()
  include(ia32.cmake)
endif()

