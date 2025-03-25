# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/share/sysbuild/cmake/modules/sysbuild_extensions.cmake)

if(${images_added})
  # same source directory is used for all targets
  # to avoid recursive calls do not add project if remote was already added
  return()
endif()

if(SB_CONFIG_IMAGE_2_BOARD)

  list(APPEND IMAGES coremark_image_2)

  ExternalZephyrProject_Add(
    APPLICATION coremark_image_2
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
    BOARD ${SB_CONFIG_IMAGE_2_BOARD}
  )

endif()

if(SB_CONFIG_IMAGE_3_BOARD)

  list(APPEND IMAGES coremark_image_3)

  ExternalZephyrProject_Add(
    APPLICATION coremark_image_3
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
    BOARD ${SB_CONFIG_IMAGE_3_BOARD}
  )

endif()

set(images_added 1)
