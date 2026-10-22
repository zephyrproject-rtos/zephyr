# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(src/libc-hooks.c)
zephyr_system_include_directories(include)
