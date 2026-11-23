# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(crc4_sw.c)
zephyr_library_sources(crc8_sw.c)
zephyr_library_sources(crc16_sw.c)
zephyr_library_sources(crc7_sw.c)
zephyr_library_sources(crc24_sw.c)
zephyr_library_sources(crc32c_sw.c)
zephyr_library_sources(crc32_sw.c)
zephyr_library_sources(crc32k_4_2_sw.c)

zephyr_library_sources_ifdef(CONFIG_CRC_HW_HANDLER crc_hardware.c)
zephyr_library_sources_ifdef(CONFIG_CRC_SHELL crc_shell.c)
