include(${TOOLCHAIN_ROOT}/cmake/linker/api_ld_hosted.cmake)
include(${TOOLCHAIN_ROOT}/cmake/linker/api_ld_cpp.cmake)

macro(toolchain_ld)

  set_ifndef(LINKERFLAGPREFIX -Wl)

  toolchain_ld_hosted()
  toolchain_ld_cpp()

  # TODO: Archiver arguments
  # ar_option(D)

  zephyr_ld_options(
    ${LINKERFLAGPREFIX},--gc-sections
    ${LINKERFLAGPREFIX},--build-id=none
  )

endmacro()


macro(toolchain_ld_link_step_0_init)
  set(zephyr_lnk
    ${LINKERFLAGPREFIX},-Map=${PROJECT_BINARY_DIR}/${KERNEL_MAP_NAME}
    -u_OffsetAbsSyms
    -u_ConfigAbsSyms
    ${LINKERFLAGPREFIX},--whole-archive
    ${ZEPHYR_LIBS_PROPERTY}
    ${LINKERFLAGPREFIX},--no-whole-archive
    kernel
    ${OFFSETS_O_PATH}
    ${LIB_INCLUDE_DIR}
    -L${PROJECT_BINARY_DIR}
    ${TOOLCHAIN_LIBS}
  )
endmacro()

macro(toolchain_ld_link_step_1_base)
  target_link_libraries(zephyr_prebuilt -T ${PROJECT_BINARY_DIR}/linker.cmd ${PRIV_STACK_LIB} ${zephyr_lnk})
endmacro()

macro(toolchain_ld_link_step_2_generated)
  target_link_libraries(kernel_elf ${GKOF} -T ${PROJECT_BINARY_DIR}/linker_pass_final.cmd ${zephyr_lnk})
endmacro()
