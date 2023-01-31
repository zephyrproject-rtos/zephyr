# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(input.c)

zephyr_linker_sources(SECTIONS input.ld)
