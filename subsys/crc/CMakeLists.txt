# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CRC_HW_HANDLER)
  zephyr_sources(crc_hardware.c)
else()
zephyr_sources_ifdef(CONFIG_CRC4 crc4_sw.c)
zephyr_sources_ifdef(CONFIG_CRC7 crc7_sw.c)
zephyr_sources_ifdef(CONFIG_CRC8 crc8_sw.c)
zephyr_sources_ifdef(CONFIG_CRC16 crc16_sw.c)
zephyr_sources_ifdef(CONFIG_CRC24 crc24_sw.c)
zephyr_sources_ifdef(CONFIG_CRC32_C crc32c_sw.c)
zephyr_sources_ifdef(CONFIG_CRC32_IEEE crc32_sw.c)
endif()

zephyr_sources_ifdef(CONFIG_CRC_SHELL crc_shell.c)
