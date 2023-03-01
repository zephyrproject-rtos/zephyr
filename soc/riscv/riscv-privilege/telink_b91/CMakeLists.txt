# Copyright (c) 2021 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_TELINK_B91_USB_SWI_ENABLE)
	message(STATUS "USB SWI interface is enabled")
endif()

if (CONFIG_TELINK_B91_2_WIRE_SPI_ENABLE)
	message(WARNING "2-wire SPI interface enabled!")
endif()

zephyr_sources(
	start.S
	soc_irq.S
	soc.c
	halt.c
)

zephyr_sources_ifdef(CONFIG_PM
	power.c
)

# Force using BFD-LD
zephyr_ld_options(-fuse-ld=bfd)

# Set compile options
zephyr_compile_options_ifdef(CONFIG_TELINK_B91_HWDSP -mext-dsp)
zephyr_compile_options_ifndef(CONFIG_RISCV_GP -mno-relax)
zephyr_linker_sources(ROM_START SORT_KEY 0x0 init.ld)
zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x0 ilm.ld)
