# Copyright (c) 2024 CISPA Helmholtz Center for Information Security gGmbH
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${SOC_SERIES})

if(CONFIG_SOC_FAMILY_CVA6_PROVIDE_TEST_POWEROFF)
  # need to force the linker to keep the symbol, such that the Ariane testbench can find it
  set(CMAKE_EXE_LINKER_FLAGS "-u tohost" CACHE INTERNAL "" FORCE)
endif()

zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_CVA6_PROVIDE_TEST_POWEROFF soc_poweroff.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_CVA6_PROVIDE_NONSTANDARD_CACHE_OPTIONS soc_cache_management.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
