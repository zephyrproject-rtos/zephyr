#
# Copyright (c) 2019,2020 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(pinmux.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

if (CONFIG_BUILD_WITH_TFM)
	# Set default image versions if not defined elsewhere
	if (NOT DEFINED TFM_IMAGE_VERSION_S)
		set(TFM_IMAGE_VERSION_S 0.0.0+0)
	endif()

	if (NOT DEFINED TFM_IMAGE_VERSION_NS)
		set(TFM_IMAGE_VERSION_NS 0.0.0+0)
	endif()

	set(PREPROCESSED_FILE_S "${CMAKE_BINARY_DIR}/tfm/bl2/ext/mcuboot/CMakeFiles/signing_layout_s.dir/signing_layout_s.o")
	set(PREPROCESSED_FILE_NS "${CMAKE_BINARY_DIR}/tfm/bl2/ext/mcuboot/CMakeFiles/signing_layout_ns.dir/signing_layout_ns.o")
	set(TFM_MCUBOOT_DIR "${ZEPHYR_TFM_MODULE_DIR}/trusted-firmware-m/bl2/ext/mcuboot")

	# Configure which format (full or hash) to include the public key in
	# the image manifest
	set(TFM_PUBLIC_KEY_FORMAT "full")

	# Set srec_cat binary name
	find_program(SREC_CAT srec_cat)
	if(${SREC_CAT} STREQUAL SREC_CAT-NOTFOUND)
	    message(FATAL_ERROR "'srec_cat' not found. Please install it, or add it to $PATH.")
	endif()

	# Create and sign for concatenated binary image should align with the TF-M BL2
	set_property(GLOBAL APPEND PROPERTY extra_post_build_commands

		# Create concatenated binary image from the two binary files
		COMMAND ${PYTHON_EXECUTABLE} ${TFM_MCUBOOT_DIR}/scripts/assemble.py
		    --layout ${PREPROCESSED_FILE_NS}
			-s $<TARGET_PROPERTY:tfm,TFM_S_BIN_FILE>
			-n ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_BIN_NAME}
			-o ${CMAKE_BINARY_DIR}/tfm_full.bin

		#Sign concatenated binary image with default public key in mcuboot folder
		COMMAND ${PYTHON_EXECUTABLE} ${TFM_MCUBOOT_DIR}/scripts/wrapper/wrapper.py
			--layout ${PREPROCESSED_FILE_NS}
			-k ${CONFIG_TFM_KEY_FILE_NS}
			--public-key-format ${TFM_PUBLIC_KEY_FORMAT}
			--align 1
			-v ${TFM_IMAGE_VERSION_NS}
			--pad
			--pad-header
			${ADD_NS_IMAGE_MIN_VER}
			-s auto
			-H 0x400
			${CMAKE_BINARY_DIR}/tfm_full.bin
			${CMAKE_BINARY_DIR}/tfm_sign.bin

		#Copy mcuboot.bin
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:tfm,BL2_BIN_FILE> ${CMAKE_BINARY_DIR}/mcuboot.bin

		#srec_cat to combine images into hex for drag and drop
		COMMAND ${SREC_CAT}
		ARGS ${CMAKE_BINARY_DIR}/mcuboot.bin -Binary
			-offset 0xA000000
			${CMAKE_BINARY_DIR}/tfm_sign.bin -Binary
			-offset 0xA020000
			-o ${CMAKE_BINARY_DIR}/tfm_zephyr.hex -intel --line-length=44
	)
endif()
