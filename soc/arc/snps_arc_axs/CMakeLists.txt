# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_cc_option(-mcpu=${GCC_M_CPU})
zephyr_cc_option(-mno-sdata)
zephyr_cc_option_ifdef(CONFIG_FLOAT -mfpu=fpud_all)
