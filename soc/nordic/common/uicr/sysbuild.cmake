# SPDX-License-Identifier: Apache-2.0

# Add UICR generator as a utility image
ExternalZephyrProject_Add(
  APPLICATION uicr
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/gen_uicr
)

# Ensure UICR is configured and built after the default image so EDT/ELFs exist.
sysbuild_add_dependencies(CONFIGURE uicr ${DEFAULT_IMAGE})

# Add build dependencies for all images whose ELF files may be used by gen_uicr.
# The gen_uicr/CMakeLists.txt scans all sibling build directories and adds their
# ELF files as file dependencies. However, we also need target dependencies to
# ensure those images are built before uicr attempts to use their ELF files.
#
# Use cmake_language(DEFER DIRECTORY) to ensure this runs after ALL images have
# been added to the sysbuild_images global property, even if some module adds
# images after the soc subdirectory is processed. We defer to the source root
# directory to ensure we're at the top-level scope where all subdirectories have
# completed processing.
function(uicr_add_image_dependencies)
  get_property(all_images GLOBAL PROPERTY sysbuild_images)
  foreach(img ${all_images})
    if(NOT img STREQUAL "uicr")
      add_dependencies(uicr ${img})
    endif()
  endforeach()
endfunction()

cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL uicr_add_image_dependencies)
