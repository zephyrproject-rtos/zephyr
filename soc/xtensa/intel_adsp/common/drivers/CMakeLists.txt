# SPDX-License-Identifier: Apache-2.0

add_definitions(-D__ZEPHYR_SUPERVISOR__)

zephyr_include_directories(${ZEPHYR_BASE}/soc/xtensa/intel_adsp/common/drivers)

add_subdirectory_ifdef(CONFIG_DEBUG_MEMORY_WINDOW memory_window)
add_subdirectory_ifdef(CONFIG_DEBUG_MEMORY_WINDOW  telemetry)
