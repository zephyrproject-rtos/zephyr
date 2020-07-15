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

if (CONFIG_BUILD_WITH_TFM)
	# Set default image versions if not defined elsewhere
	if (NOT DEFINED TFM_IMAGE_VERSION_S)
		set(TFM_IMAGE_VERSION_S 0.0.0+0)
	endif()

	if (NOT DEFINED TFM_IMAGE_VERSION_NS)
		set(TFM_IMAGE_VERSION_NS 0.0.0+0)
	endif()

	set(PREPROCESSED_FILE "${CMAKE_BINARY_DIR}/tfm/image_macros_preprocessed")

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

  #Create concatenated binary image from the two binary file
  COMMAND ${PYTHON_EXECUTABLE} ${TFM_MCUBOOT_DIR}/scripts/assemble.py
  ARGS --layout ${PREPROCESSED_FILE}.c
    -s ${CMAKE_BINARY_DIR}/build/tfm/install/outputs/LPC55S69/tfm_s.bin
    -n ${CMAKE_BINARY_DIR}/zephyr/zephyr.bin
    -o ${CMAKE_BINARY_DIR}/tfm_full.bin
  )
endif()
