# SPDX-License-Identifier: Apache-2.0

zephyr_sources(idle.c)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
