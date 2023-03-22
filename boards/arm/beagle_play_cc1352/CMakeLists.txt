# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
    board_antenna.c
    )

zephyr_library_compile_definitions("DeviceFamily_CC13X2X7")
