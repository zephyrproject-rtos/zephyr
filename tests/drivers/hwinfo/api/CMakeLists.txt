# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(integration)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_HWINFO_NVMEM app PRIVATE src/nvmem.c)
