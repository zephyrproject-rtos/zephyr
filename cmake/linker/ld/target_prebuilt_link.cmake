# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_prebuilt_link)

  # FIXME: Is there any way to get rid of empty_file.c?
  add_executable(       ${ZEPHYR_PREBUILT_EXECUTABLE} misc/empty_file.c)
  target_link_libraries(${ZEPHYR_PREBUILT_EXECUTABLE} ${TOPT} ${PROJECT_BINARY_DIR}/linker.cmd ${PRIV_STACK_LIB} ${zephyr_lnk} ${CODE_RELOCATION_DEP})
  set_property(TARGET   ${ZEPHYR_PREBUILT_EXECUTABLE} PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker.cmd)
  add_dependencies(     ${ZEPHYR_PREBUILT_EXECUTABLE} ${PRIV_STACK_DEP} ${LINKER_SCRIPT_TARGET} ${OFFSETS_LIB})

endmacro()
