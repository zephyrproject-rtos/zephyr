# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_ld_ordered_options dest_var_name)

  set(${dest_var_name}
    ${LINKERFLAGPREFIX},-Map=${PROJECT_BINARY_DIR}/${KERNEL_MAP_NAME}
    -u_OffsetAbsSyms
    -u_ConfigAbsSyms
    ${LINKERFLAGPREFIX},--whole-archive
    ${ZEPHYR_LIBS_PROPERTY}
    ${LINKERFLAGPREFIX},--no-whole-archive
    kernel
    $<TARGET_OBJECTS:${OFFSETS_LIB}>
    ${LIB_INCLUDE_DIR}
    -L${PROJECT_BINARY_DIR}
    ${TOOLCHAIN_LIBS}
  )

endmacro()
