# SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(code_relocation)

target_sources(app PRIVATE src/main.c src/reloc.c)

zephyr_code_relocate(FILES src/reloc.c LOCATION RAM)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/kernel/sem.c LOCATION RAM_TEXT NOKEEP)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/crc/crc32_sw.c
                     FILTER "\\.crc32_ieee$" LOCATION RAM_TEXT)

if(CONFIG_ESP_SPIRAM)
  target_sources(app PRIVATE src/extmem.c)
  zephyr_code_relocate(FILES src/extmem.c LOCATION EXTMEM_NOINIT)
endif()
