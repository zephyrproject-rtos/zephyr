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

	set(PREPROCESSED_FILE "${CMAKE_BINARY_DIR}/tfm/image_macros_preprocessed")
	set(TFM_MCUBOOT_DIR "${ZEPHYR_BASE}/../modules/tee/tfm/trusted-firmware-m/bl2/ext/mcuboot")

	# Configure which format (full or hash) to include the public key in
	# the image manifest
	set(TFM_PUBLIC_KEY_FORMAT "full")

	# Set srec_cat binary name
	find_program(SREC_CAT srec_cat)
	if(${SREC_CAT} STREQUAL SREC_CAT-NOTFOUND)
	    message(FATAL_ERROR "'srec_cat' not found. Please install it, or add it to $PATH.")
	endif()

	#Create and sign for concatenated binary image, should align with the TF-M BL2
	set_property(GLOBAL APPEND PROPERTY extra_post_build_commands

		#Sign secure binary image with public key
		COMMAND ${PYTHON_EXECUTABLE} ${TFM_MCUBOOT_DIR}/scripts/imgtool.py
		ARGS sign
			 --layout ${PREPROCESSED_FILE}_s.c
			 -k ${CONFIG_TFM_KEY_FILE_S}
			 --public-key-format ${TFM_PUBLIC_KEY_FORMAT}
			 --align 1
			 -v ${TFM_IMAGE_VERSION_S}
			 ${ADD_NS_IMAGE_MIN_VER}
			 ${ADD_SECURITY_COUNTER_S}
			 -H 0x400
			 ${CMAKE_BINARY_DIR}/tfm/install/outputs/AN521/tfm_s.bin
			 ${CMAKE_BINARY_DIR}/tfm_s_signed.bin

		#Sign non-secure binary image with public key
		COMMAND ${PYTHON_EXECUTABLE} ${TFM_MCUBOOT_DIR}/scripts/imgtool.py
		ARGS sign
			 --layout ${PREPROCESSED_FILE}_ns.c
			 -k ${CONFIG_TFM_KEY_FILE_NS}
			 --public-key-format ${TFM_PUBLIC_KEY_FORMAT}
			 --align 1
			 -v ${TFM_IMAGE_VERSION_NS}
			 ${ADD_S_IMAGE_MIN_VER}
			 ${ADD_SECURITY_COUNTER_NS}
			 -H 0x400
			 ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_BIN_NAME}
			 ${CMAKE_BINARY_DIR}/zephyr_ns_signed.bin

		#Create concatenated binary image from the two independently signed binary file
		COMMAND ${PYTHON_EXECUTABLE} ${TFM_MCUBOOT_DIR}/scripts/assemble.py
		ARGS --layout ${PREPROCESSED_FILE}_s.c
			 -s ${CMAKE_BINARY_DIR}/tfm_s_signed.bin
			 -n ${CMAKE_BINARY_DIR}/zephyr_ns_signed.bin
			 -o ${CMAKE_BINARY_DIR}/tfm_sign.bin

		#Copy mcuboot.bin
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/tfm/bl2/ext/mcuboot/mcuboot.bin ${CMAKE_BINARY_DIR}

		#Merge mcuboot.bin and tfm_sign.bin for QEMU
		COMMAND ${SREC_CAT}
		ARGS ${CMAKE_BINARY_DIR}/mcuboot.bin -Binary
			${CMAKE_BINARY_DIR}/tfm_sign.bin -Binary
			-offset 0x80000
			-o ${CMAKE_BINARY_DIR}/tfm_qemu.bin -Binary

		#Convert tfm_qemu.bin to .hex with an appropriate offset
		COMMAND ${SREC_CAT}
		ARGS ${CMAKE_BINARY_DIR}/tfm_qemu.bin -binary
			-offset 0x10000000
			-o ${CMAKE_BINARY_DIR}/tfm_qemu.hex -intel --line-length=44
	)
endif()
