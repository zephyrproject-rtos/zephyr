# SPDX-License-Identifier: Apache-2.0

# Apple ld64 shares everything with the common ld backend except the link rule:
# it takes neither a linker script nor --whole-archive, and spells -Map -map.
include(${ZEPHYR_BASE}/cmake/linker/ld/target.cmake)

# Link a target to given libraries with Darwin argument order
#
# Usage:
#   toolchain_ld_link_elf(
#     TARGET_ELF             <target_elf>
#     OUTPUT_MAP             <output_map_file_of_target>
#     LIBRARIES_PRE_SCRIPT   [libraries_pre_script]
#     LINKER_SCRIPT          <linker_script>
#     LIBRARIES_POST_SCRIPT  [libraries_post_script]
#     DEPENDENCIES           [dependencies]
#   )
function(toolchain_ld_link_elf)
  cmake_parse_arguments(
    TOOLCHAIN_LD_LINK_ELF                                     # prefix of output variables
    ""                                                        # list of names of the boolean arguments
    "TARGET_ELF;OUTPUT_MAP;LINKER_SCRIPT"                     # list of names of scalar arguments
    "LIBRARIES_PRE_SCRIPT;LIBRARIES_POST_SCRIPT;DEPENDENCIES" # list of names of list arguments
    ${ARGN}                                                   # input args to parse
  )

  # ld64 has no --whole-archive, each archive is named with -force_load instead
  set(whole_archive_flags)
  foreach(lib ${WHOLE_ARCHIVE_LIBS})
    list(APPEND whole_archive_flags ${LINKERFLAGPREFIX},-force_load,$<TARGET_FILE:${lib}>)
  endforeach()
  if(WHOLE_ARCHIVE_LIBS)
    add_dependencies(${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF} ${WHOLE_ARCHIVE_LIBS})
  endif()

  target_link_libraries(
    ${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF}
    ${TOOLCHAIN_LD_LINK_ELF_LIBRARIES_PRE_SCRIPT}
    ${TOOLCHAIN_LD_LINK_ELF_LIBRARIES_POST_SCRIPT}

    ${LINKERFLAGPREFIX},-map,${TOOLCHAIN_LD_LINK_ELF_OUTPUT_MAP}
    ${whole_archive_flags}
    ${NO_WHOLE_ARCHIVE_LIBS}
    $<TARGET_OBJECTS:${OFFSETS_LIB}>
    -L${PROJECT_BINARY_DIR}

    ${TOOLCHAIN_LD_LINK_ELF_DEPENDENCIES}
  )
endfunction(toolchain_ld_link_elf)
