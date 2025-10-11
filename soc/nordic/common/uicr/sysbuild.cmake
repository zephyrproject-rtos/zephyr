# SPDX-License-Identifier: Apache-2.0

# Add UICR generator as a utility image
ExternalZephyrProject_Add(
  APPLICATION uicr
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/gen_uicr
)

# Ensure UICR is configured and built after the default image so EDT/ELFs exist.
sysbuild_add_dependencies(CONFIGURE uicr ${DEFAULT_IMAGE})

add_dependencies(uicr ${DEFAULT_IMAGE})
if(DEFINED image)
  add_dependencies(uicr ${image})
endif()
