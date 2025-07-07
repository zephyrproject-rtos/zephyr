# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_CRC
  crc32k_4_2_sw.c
  crc32c_sw.c
  crc32_sw.c
  crc24_sw.c
  crc16_sw.c
  crc8_sw.c
  crc7_sw.c
  crc4_sw.c
  )
zephyr_sources_ifdef(CONFIG_CRC_SHELL crc_shell.c)
