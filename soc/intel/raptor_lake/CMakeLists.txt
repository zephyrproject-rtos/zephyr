# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_cc_option(-march=goldmont)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
