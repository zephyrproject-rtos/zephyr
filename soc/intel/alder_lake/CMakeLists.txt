# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library()
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_cc_option(-march=goldmont)

zephyr_library_sources(cpu.c)
zephyr_library_sources(../common/soc_gpio.c)

zephyr_library_sources_ifdef(CONFIG_POWEROFF ../common/power.c)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
