# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(include)
zephyr_library_sources_ifdef(CONFIG_XTENSA_MMU mmu.c)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/include/xtensa-dc233c.ld CACHE INTERNAL "")
