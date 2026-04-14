# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2026 Linumiz
# Author: Sri Surya <srisurya@linumiz.com>

zephyr_library_sources(soc.c)

zephyr_include_directories(.)

if(CONFIG_SOC_TI_TM4C123GH6PM)
  zephyr_compile_definitions(PART_TM4C123GH6PM)
endif()
