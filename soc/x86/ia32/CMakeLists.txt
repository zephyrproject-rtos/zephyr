# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

if(CONFIG_X86_64)
zephyr_cc_option(-march=x86-64)
else()
zephyr_cc_option(-march=pentium)
endif()

zephyr_library_sources(soc.c)
