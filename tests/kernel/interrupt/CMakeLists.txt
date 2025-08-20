# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(interrupt)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  ${ZEPHYR_BASE}/arch/common/include
  )

target_sources(app PRIVATE
    src/prevent_irq.c
    src/interrupt_offload.c
    src/nested_irq.c
    )

if(CONFIG_ARM64)
  add_compile_options(-mgeneral-regs-only)
endif()

target_sources_ifdef(CONFIG_DYNAMIC_INTERRUPTS app PRIVATE src/dynamic_isr.c)
target_sources_ifdef(CONFIG_X86 app PRIVATE src/regular_isr.c)
target_sources_ifdef(CONFIG_SHARED_INTERRUPTS app PRIVATE src/static_shared_irq.c)

if (CONFIG_SHARED_INTERRUPTS)
target_sources_ifdef(CONFIG_DYNAMIC_INTERRUPTS app PRIVATE src/dynamic_shared_irq.c)
endif()

if(CONFIG_MULTI_LEVEL_INTERRUPTS)
  target_sources_ifdef(CONFIG_DYNAMIC_INTERRUPTS app PRIVATE src/sw_isr_table.c)
  target_sources_ifdef(CONFIG_TEST_MULTILEVEL_IRQ app PRIVATE src/multilevel_irq.c)
endif()
