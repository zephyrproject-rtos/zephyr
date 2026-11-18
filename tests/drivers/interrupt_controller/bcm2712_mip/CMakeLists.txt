# Copyright (c) 2026 Process Mission
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bcm2712_mip)

if(TEST_BCM2712_MIP_ROUTES)
  target_sources(app PRIVATE
    src/routes.c
    "${ZEPHYR_BASE}/soc/brcm/bcm2712/soc_irq.c"
  )
  target_compile_definitions(app PRIVATE CONFIG_RP1_IRQ=1)
  zephyr_iterable_section(NAME bcm2712_irq_route GROUP RODATA_REGION)
  zephyr_linker_sources(SECTIONS "${ZEPHYR_BASE}/soc/brcm/bcm2712/soc_irq.ld")
  target_include_directories(app PRIVATE
    "${ZEPHYR_BASE}/soc/brcm/bcm2712"
  )
  return()
endif()

target_sources(app PRIVATE
  src/mip.c
)
target_include_directories(app PRIVATE
  "${ZEPHYR_BASE}/drivers/interrupt_controller"
)
