macro(toolchain_bintools_post_link)

  list_append_ifdef(CONFIG_CHECK_LINK_MAP
    post_link_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/check_link_map.py ${KERNEL_MAP_NAME}
  )

  list_append_ifdef(
    CONFIG_BUILD_OUTPUT_HEX
    post_link_commands
    COMMAND ${CMAKE_OBJCOPY} -S -Oihex --gap-fill 0xFF  -R .comment -R COMMON -R .eh_frame  ${KERNEL_ELF_NAME}    ${KERNEL_HEX_NAME}
  )

  list_append_ifdef(
    CONFIG_BUILD_OUTPUT_BIN
    post_link_commands
    COMMAND ${CMAKE_OBJCOPY} -S -Obinary --gap-fill 0xFF -R .comment -R COMMON -R .eh_frame  ${KERNEL_ELF_NAME}    ${KERNEL_BIN_NAME}
  )

  list_append_ifdef(
    CONFIG_BUILD_OUTPUT_S19
    post_link_commands
    COMMAND ${CMAKE_OBJCOPY} --gap-fill 0xFF --srec-len 1 --output-target=srec ${KERNEL_ELF_NAME} ${KERNEL_S19_NAME}
  )

  list_append_ifdef(
    CONFIG_OUTPUT_DISASSEMBLY
    post_link_commands
    COMMAND ${CMAKE_OBJDUMP} -S ${KERNEL_ELF_NAME} >  ${KERNEL_LST_NAME}
  )

  list_append_ifdef(
    CONFIG_OUTPUT_STAT
    post_link_commands
    COMMAND ${CMAKE_READELF} -e ${KERNEL_ELF_NAME} >  ${KERNEL_STAT_NAME}
  )

  list_append_ifdef(
    CONFIG_BUILD_OUTPUT_STRIPPED
    post_link_commands
    COMMAND ${CMAKE_STRIP}   --strip-all ${KERNEL_ELF_NAME} -o ${KERNEL_STRIP_NAME}
  )

  list_append_ifdef(
    CONFIG_BUILD_OUTPUT_EXE
    post_link_commands
    COMMAND ${CMAKE_COMMAND} -E copy ${KERNEL_ELF_NAME}    ${KERNEL_EXE_NAME}
  )

endmacro()
