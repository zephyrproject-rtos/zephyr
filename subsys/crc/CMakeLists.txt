# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_CRC_SW_HANDLER)
  if(CONFIG_CRC4 OR CONFIG_CRC4_TI)
    zephyr_library_sources(crc4_sw.c)
  endif()

  if(CONFIG_CRC8 OR CONFIG_CRC8_ROHC OR CONFIG_CRC8_CCITT)
    zephyr_library_sources(crc8_sw.c)
  endif()

  if(CONFIG_CRC16 OR CONFIG_CRC16_CCITT OR CONFIG_CRC16_ITU_T OR
     CONFIG_CRC16_REFLECT OR CONFIG_CRC16_ANSI)
    zephyr_library_sources(crc16_sw.c)
  endif()

  zephyr_library_sources_ifdef(CONFIG_CRC7_BE crc7_sw.c)
  zephyr_library_sources_ifdef(CONFIG_CRC24_PGP crc24_sw.c)
  zephyr_library_sources_ifdef(CONFIG_CRC32_C crc32c_sw.c)
  zephyr_library_sources_ifdef(CONFIG_CRC32_IEEE crc32_sw.c)

  if(CONFIG_CRC32_K_4_2_TABLE_256 OR CONFIG_CRC32_K_4_2)
    zephyr_library_sources(crc32k_4_2_sw.c)
  endif()

endif()

zephyr_library_sources_ifdef(CONFIG_CRC_HW_HANDLER crc_hardware.c)
zephyr_library_sources_ifdef(CONFIG_CRC_SHELL crc_shell.c)
