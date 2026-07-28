# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(
    ${ZEPHYR_BASE}/drivers/modem # Allow including AT driver without ".."
)

zephyr_library_sources(
    st67w611m1_drv.c
    st67w611m1_spi.c
)
