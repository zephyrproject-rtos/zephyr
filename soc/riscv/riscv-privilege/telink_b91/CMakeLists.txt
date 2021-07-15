# Copyright (c) 2021 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
	start.S
	soc_irq.S
	soc.c
)

# Force using BFD-LD
zephyr_ld_options(-fuse-ld=bfd)

# Set compile options
zephyr_compile_options_ifdef(CONFIG_TELINK_B91_HWDSP -mext-dsp)
zephyr_compile_options(-mno-relax)
