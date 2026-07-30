# SPDX-License-Identifier: Apache-2.0

zephyr_compile_options_ifdef(CONFIG_CPU_M68000 -m68000)
zephyr_compile_options_ifdef(CONFIG_CPU_M68010 -m68010)

set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT "elf32-m68k")

zephyr_library_include_directories(${ZEPHYR_BASE}/arch/common/include)

zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD core/irq_offload.c)

zephyr_sources(core/exception.S)
zephyr_sources(core/swap.S)
zephyr_sources(core/vector_table.S)
zephyr_sources(core/exception.c)
zephyr_sources(core/fatal.c)
zephyr_sources(core/idle.c)
zephyr_sources(core/irq.c)
zephyr_sources(core/thread.c)
