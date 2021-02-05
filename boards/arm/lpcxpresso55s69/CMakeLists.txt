#
# Copyright (c) 2019, NXP
# Copyright (c) 2020, Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_PINMUX_MCUX_LPC)
  zephyr_library()
  zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
  zephyr_library_sources(pinmux.c)
endif()


if (CONFIG_SECOND_CORE_MCUX)
	# Set srec_cat binary name
	find_program(SREC_CAT srec_cat)
	if(${SREC_CAT} STREQUAL SREC_CAT-NOTFOUND)
		message(FATAL_ERROR "'srec_cat' not found. Please install it, or add it to $PATH.")
	endif()

	#merge cpu0 and cpu1 to a single image
	#offset is taken from zephyr,code-cpu1-partition
	set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
		COMMAND ${SREC_CAT}
		ARGS ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_BIN_NAME} -Binary
			${REMOTE_ZEPHYR_DIR}/${KERNEL_BIN_NAME} -Binary
			-offset ${CONFIG_SECOND_CORE_BOOT_ADDRESS_MCUX}
			-o ${CMAKE_BINARY_DIR}/multicore.bin -Binary
	)
endif()
