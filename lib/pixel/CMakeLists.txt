# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources(bayer.c)
zephyr_library_sources(convert.c)
zephyr_library_sources(distort.c)
zephyr_library_sources(image.c)
zephyr_library_sources(kernel.c)
zephyr_library_sources(print.c)
zephyr_library_sources(resize.c)
zephyr_library_sources(stats.c)
# zephyr-keep-sorted-stop

zephyr_linker_sources(DATA_SECTIONS pixel.ld)
