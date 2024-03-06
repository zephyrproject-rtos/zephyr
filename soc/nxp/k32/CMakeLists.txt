# NXP K32 family

#
# Copyright (c) 2023 Sendrato
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(${SOC_SERIES})

# Sign secure binary image with public key
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
		COMMAND ${PYTHON_EXECUTABLE}
		${CMAKE_CURRENT_LIST_DIR}/dk6_image_tool.py -b1
		${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
		-gnu ${CROSS_COMPILE}
)
