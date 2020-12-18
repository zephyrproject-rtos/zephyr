# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PINMUX)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
endif()

if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "zephyr")
	set(COMPILER_FULL_PATH ${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc)
elseif(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "gnuarmemb")
	set(COMPILER_FULL_PATH ${GNUARMEMB_TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc)
endif()

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
	set(TFM_MCUBOOT_DIR "${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/trusted-firmware-m/bl2/ext/mcuboot")

	# Configure which format (full or hash) to include the public key in
	# the image manifest
	set(TFM_PUBLIC_KEY_FORMAT "full")

	#Create and sign for concatenated binary image, should align with the TF-M BL2
	set_property(GLOBAL APPEND PROPERTY extra_post_build_commands

		#Sign secure binary image with public key
		COMMAND ${PYTHON_EXECUTABLE} ${TFM_MCUBOOT_DIR}/scripts/wrapper/wrapper.py
			 --layout ${PREPROCESSED_FILE_S}
			 -k ${CONFIG_TFM_KEY_FILE_S}
			 --public-key-format ${TFM_PUBLIC_KEY_FORMAT}
			 --align 1
			 -v ${TFM_IMAGE_VERSION_S}
			 --pad
			 --pad-header
			 ${ADD_NS_IMAGE_MIN_VER}
			 -s auto
			 -H 0x400
			 $<TARGET_PROPERTY:tfm,TFM_S_BIN_FILE>
			 ${CMAKE_BINARY_DIR}/tfm_s_signed.bin

		#Sign non-secure binary image with public key
		COMMAND ${PYTHON_EXECUTABLE} ${TFM_MCUBOOT_DIR}/scripts/wrapper/wrapper.py
			 --layout ${PREPROCESSED_FILE_NS}
			 -k ${CONFIG_TFM_KEY_FILE_NS}
			 --public-key-format ${TFM_PUBLIC_KEY_FORMAT}
			 --align 1
			 -v ${TFM_IMAGE_VERSION_NS}
			 -s auto
			 ${ADD_S_IMAGE_MIN_VER}
			 -H 0x400
			 ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_BIN_NAME}
			 ${CMAKE_BINARY_DIR}/zephyr_ns_signed.bin

		#Copy mcuboot.bin
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:tfm,BL2_BIN_FILE> ${CMAKE_BINARY_DIR}/mcuboot.bin

		#Execute post build script postbuild.sh
		COMMAND ${CMAKE_BINARY_DIR}/tfm/postbuild.sh ${COMPILER_FULL_PATH}
      )
endif()
