# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_include_directories(${ZEPHYR_BASE}/soc/xtensa/intel_adsp/common/include)

zephyr_library_sources_ifdef(CONFIG_DYNAMIC_SLOTS dynamic_slots.c)
zephyr_library_sources_ifdef(CONFIG_DEBUG_MEMORY_WINDOW debug_memory_window.c)
