#
# Copyright (c) 2019 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(pinmux.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

if (CONFIG_BUILD_WITH_TFM)
	set (TFM_ROOT_PATH $ENV{ZEPHYR_BASE}/ext/tfm/tfm)
	set (MCUBOOT_DIR "${TFM_ROOT_PATH}/bl2/ext/mcuboot")
	set (PREPROCESSED_FILE "${TFM_ROOT_PATH}/build/image_macros_preprocessed")

	if (NOT DEFINED IMAGE_VERSION_S)
		set(IMAGE_VERSION_S 0.0.0+0)
	endif()
	if (NOT DEFINED IMAGE_VERSION_NS)
		set(IMAGE_VERSION_NS 0.0.0+0)
	endif()
	#Find Python3 interpreter
	find_package(PythonInterp 3)
	if (NOT PYTHONINTERP_FOUND)
		message(FATAL_ERROR "Failed to find Python3 interpreter. Pyhton3 must be installed and available on the PATH.")
	endif()

	set(KEY_FILE_S  "${MCUBOOT_DIR}/root-rsa-3072.pem")
	set(KEY_FILE_NS "${MCUBOOT_DIR}/root-rsa-3072_1.pem")

	#Configure in which format (full or hash) include the public key to the image manifest
	set(PUBLIC_KEY_FORMAT "full")

	#Create and sign for concatenated binary image should align with the TF-M BL2
	set_property(GLOBAL APPEND PROPERTY extra_post_build_commands

		#Sign secure binary image with default public key in mcuboot folder
		COMMAND ${PYTHON_EXECUTABLE} ${MCUBOOT_DIR}/scripts/imgtool.py
		ARGS sign
			 --layout ${PREPROCESSED_FILE}_s.c
			 -k ${KEY_FILE_S}
			 --public-key-format ${PUBLIC_KEY_FORMAT}
			 --align 1
			 -v ${IMAGE_VERSION_S}
			 ${ADD_NS_IMAGE_MIN_VER}
			 ${ADD_SECURITY_COUNTER_S}
			 -H 0x400
			 ${TFM_ROOT_PATH}/build/install/outputs/AN521/tfm_s.bin
			 ${CMAKE_BINARY_DIR}/tfm_s_signed.bin

		#Sign non-secure binary image with default public key in mcuboot folder
		COMMAND ${PYTHON_EXECUTABLE} ${MCUBOOT_DIR}/scripts/imgtool.py
		ARGS sign
			 --layout ${PREPROCESSED_FILE}_ns.c
			 -k ${KEY_FILE_NS}
			 --public-key-format ${PUBLIC_KEY_FORMAT}
			 --align 1
			 -v ${IMAGE_VERSION_NS}
			 ${ADD_S_IMAGE_MIN_VER}
			 ${ADD_SECURITY_COUNTER_NS}
			 -H 0x400
			 ${CMAKE_BINARY_DIR}/zephyr/zephyr.bin
			 ${CMAKE_BINARY_DIR}/zephyr_ns_signed.bin

		#Create concatenated binary image from the two independently signed binary file
		COMMAND ${PYTHON_EXECUTABLE} ${MCUBOOT_DIR}/scripts/assemble.py
		ARGS --layout ${PREPROCESSED_FILE}_s.c
			 -s ${CMAKE_BINARY_DIR}/tfm_s_signed.bin
			 -n ${CMAKE_BINARY_DIR}/zephyr_ns_signed.bin
			 -o ${CMAKE_BINARY_DIR}/tfm_sign.bin

		#Copy mcuboot.bin
		COMMAND cp ${TFM_ROOT_PATH}/build/bl2/ext/mcuboot/mcuboot.bin ${CMAKE_BINARY_DIR})
endif()
