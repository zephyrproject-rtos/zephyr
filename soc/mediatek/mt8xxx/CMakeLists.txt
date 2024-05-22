# Copyright 2023 The ChromiumOS Authors
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(soc.c irq.c mbox.c)

zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_MT8195 cpuclk.c)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_SOC}/linker.ld CACHE INTERNAL "")

add_custom_target(dsp_img ALL
	DEPENDS ${ZEPHYR_FINAL_EXECUTABLE}
	COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen_img.py
		${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME}
		${CMAKE_BINARY_DIR}/zephyr/zephyr.img)
