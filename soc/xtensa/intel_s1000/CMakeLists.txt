# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_library_sources(soc.c)

if("${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "xcc")
add_subdirectory(xcc)
endif()

if(CONFIG_SMP)
zephyr_library_sources(soc_mp.c)
endif()
