# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(vesc.c)
zephyr_library_include_directories(lib)
zephyr_library_compile_definitions(-DVESC_OP_MODE_Zephyr)
