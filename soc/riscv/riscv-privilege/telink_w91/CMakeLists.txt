# Copyright (c) 2023 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_TELINK_W91_2_WIRE_SPI_ENABLE)
	message(WARNING "2-wire SPI interface enabled!")
endif()

zephyr_sources(
	start.S
	soc_irq.S
	soc.c
	halt.c
	sys_heap_alloc_wrap.c
)

if(CONFIG_TELINK_W91_IPC_DISPATCHER)
zephyr_sources(
	ipc/ipc_dispatcher.c
	ipc/ipc_based_driver.c
)
endif()

if(CONFIG_TELINK_W91_DEBUG_ENABLE)
zephyr_sources(
	debug.c
)
endif()

# Force using BFD-LD
zephyr_ld_options(-fuse-ld=bfd)

# Set compile options
zephyr_compile_options_ifdef(CONFIG_TELINK_v_HWDSP -mext-dsp)
zephyr_compile_options_ifndef(CONFIG_RISCV_GP -mno-relax)
zephyr_linker_sources(ROM_START SORT_KEY 0x0 init.ld)

# Wrap allocator functions
zephyr_link_libraries(
	-Wl,--wrap,sys_heap_alloc
	-Wl,--wrap,sys_heap_aligned_alloc
	-Wl,--wrap,sys_heap_aligned_realloc
)
