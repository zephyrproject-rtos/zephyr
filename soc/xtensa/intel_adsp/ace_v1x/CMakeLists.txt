# Intel ACE SoC family CMake file
#
# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(
	soc.c
	multiprocessing.c
	irq.c
	boot.c
	rimage_modules.c
	power_down.S
)
