# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_library_sources(soc.c)

if("${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "xcc")
if(CONFIG_NEWLIB_LIBC)
  zephyr_library_sources(xcc_newlib_fix.c)
endif()
endif()
