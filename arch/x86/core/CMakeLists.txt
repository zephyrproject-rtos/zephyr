# Copyright (c) 2019 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if (CONFIG_COVERAGE)
  zephyr_compile_options($<TARGET_PROPERTY:compiler,coverage>)
  zephyr_link_libraries($<TARGET_PROPERTY:linker,coverage>)
endif ()

zephyr_library_sources(cpuhalt.c)
zephyr_library_sources(memmap.c)
zephyr_library_sources(prep_c.c)
zephyr_library_sources(fatal.c)
zephyr_library_sources(spec_ctrl.c)

zephyr_library_sources_ifdef(CONFIG_PCIE pcie.c)
zephyr_library_sources_ifdef(CONFIG_REBOOT_RST_CNT reboot_rst_cnt.c)
zephyr_library_sources_ifdef(CONFIG_MULTIBOOT multiboot.c)
zephyr_library_sources_ifdef(CONFIG_ACPI acpi.c)
zephyr_library_sources_ifdef(CONFIG_X86_MMU x86_mmu.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE userspace.c)

zephyr_library_sources_ifdef(CONFIG_X86_VERY_EARLY_CONSOLE early_serial.c)

if(CONFIG_X86_64)
  include(intel64.cmake)
else()
  include(ia32.cmake)
endif()
