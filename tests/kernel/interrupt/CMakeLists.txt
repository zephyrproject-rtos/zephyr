# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(interrupt)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

target_sources(app PRIVATE
    src/prevent_irq.c
    src/interrupt_offload.c
    src/nested_irq.c
    )

target_sources_ifdef(CONFIG_DYNAMIC_INTERRUPTS app PRIVATE src/dynamic_isr.c)
target_sources_ifdef(CONFIG_X86 app PRIVATE src/regular_isr.c)
