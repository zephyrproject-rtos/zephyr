# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
    icm42688.c
    icm42688_common.c
    icm42688_spi.c
)
