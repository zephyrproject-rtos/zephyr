# Copyright (c) 2019 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_compile_options_ifdef(CONFIG_COVERAGE_GCOV
  -ftest-coverage
  -fprofile-arcs
  -fno-inline
  )

zephyr_library_sources_if_kconfig(pcie.c)
zephyr_library_sources_if_kconfig(reboot_rst_cnt.c)
zephyr_library_sources_ifdef(CONFIG_X86_MULTIBOOT multiboot.c)
zephyr_library_sources_ifdef(CONFIG_X86_VERY_EARLY_CONSOLE early_serial.c)

if(CONFIG_X86_LONGMODE)
  include(x64.cmake)
else()
  include(ia32.cmake)
endif()

