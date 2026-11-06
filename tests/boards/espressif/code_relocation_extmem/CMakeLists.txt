# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(code_relocation_extmem)

target_sources(app PRIVATE src/main.c src/extmem.c)

# PSRAM is initialized after .bss is zeroed: only NOINIT can be relocated there.
zephyr_code_relocate(FILES src/extmem.c LOCATION EXTMEM_NOINIT)
