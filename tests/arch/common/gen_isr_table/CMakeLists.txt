# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gen_isr_table)

FILE(GLOB app_sources src/main.c)

target_sources(app PRIVATE ${app_sources})
target_sources_ifdef(CONFIG_MULTI_LEVEL_INTERRUPTS app PRIVATE
  src/multilevel_irq.c
)

if(CONFIG_RISCV_CORE_NORDIC_VPR)
  zephyr_compile_options(-msmall-data-limit=0)
endif()
