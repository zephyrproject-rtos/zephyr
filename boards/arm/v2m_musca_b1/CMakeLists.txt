#
# Copyright (c) 2019,2020 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(pinmux.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

if (CONFIG_BUILD_WITH_TFM)
	# Set srec_cat binary name
	find_program(SREC_CAT srec_cat)
	if(${SREC_CAT} STREQUAL SREC_CAT-NOTFOUND)
	    message(FATAL_ERROR "'srec_cat' not found. Please install it, or add it to $PATH.")
	endif()

	#Create and sign for concatenated binary image should align with the TF-M BL2
	set_property(GLOBAL APPEND PROPERTY extra_post_build_commands

		#srec_cat to combine images into hex for drag and drop
		COMMAND ${SREC_CAT}
		ARGS ${CMAKE_BINARY_DIR}/tfm/bl2/ext/mcuboot/mcuboot.bin -binary
			-offset 0xA000000
			${CMAKE_BINARY_DIR}/tfm_sign.bin -binary
			-offset 0xA020000
			-o ${CMAKE_BINARY_DIR}/tfm_zephyr.hex -Intel --line-length=44
	)
endif()
