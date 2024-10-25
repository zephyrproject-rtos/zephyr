# Copyright (c) 2021-2024 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_TELINK_B91_USB_SWI_DISABLE)
	message(STATUS "USB SWI interface is disabled")
endif()

if (CONFIG_TELINK_B9X_2_WIRE_SPI_ENABLE)
	message(WARNING "2-wire SPI interface enabled!")
endif()

zephyr_include_directories(.)

zephyr_sources(
	start.S
	soc_irq.S
	soc.c
	halt.c
	sys_heap_alloc_wrap.c
)

zephyr_sources_ifdef(CONFIG_PM
	power.c
)

zephyr_sources_ifdef(CONFIG_MCUBOOT_ACTION_HOOKS
	mcuboot_status_change.c
)

# Force using BFD-LD
zephyr_ld_options(-fuse-ld=bfd)

# Set compile options
zephyr_compile_options_ifdef(CONFIG_TELINK_B9X_HWDSP -mext-dsp)
zephyr_compile_options_ifndef(CONFIG_RISCV_GP -mno-relax)
zephyr_linker_sources(ROM_START SORT_KEY 0x0 init.ld)
if(CONFIG_TELINK_B9X_MATTER_RETENTION_LAYOUT)
zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x0 matter_retention.ld)
endif()
zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x1 ilm.ld)
set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")

if(CONFIG_TELINK_B9X_MATTER_RETENTION_LAYOUT)
zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x0 matter_retention.ld)
endif()
zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x1 ilm.ld)

# Wrap allocator functions
zephyr_link_libraries(
	-Wl,--wrap,sys_heap_alloc
	-Wl,--wrap,sys_heap_aligned_alloc
	-Wl,--wrap,sys_heap_aligned_realloc
)

# Make MCUBoot early boot code injection
if (CONFIG_TELINK_B9X_MCUBOOT_STARTUP)

zephyr_sources(
	mcu_boot_startup.c
)

zephyr_link_libraries(
	-Wl,--wrap,main
)

endif()
