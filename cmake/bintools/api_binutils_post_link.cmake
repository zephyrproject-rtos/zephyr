macro(toolchain_bintools_post_link)

  if(NOT CONFIG_BUILD_NO_GAP_FILL)
    # Use ';' as separator to get proper space in resulting command.
    set(GAP_FILL "--gap-fill;0xff")
  endif()

  list_append_ifdef(
    CONFIG_BUILD_OUTPUT_HEX
    post_link_commands
    COMMAND ${CMAKE_OBJCOPY} -S -Oihex ${GAP_FILL}  -R .comment -R COMMON -R .eh_frame  ${KERNEL_ELF_NAME}    ${KERNEL_HEX_NAME}
  )

  list_append_ifdef(
    CONFIG_BUILD_OUTPUT_BIN
    post_link_commands
    COMMAND ${CMAKE_OBJCOPY} -S -Obinary ${GAP_FILL} -R .comment -R COMMON -R .eh_frame  ${KERNEL_ELF_NAME}    ${KERNEL_BIN_NAME}
  )

  list_append_ifdef(
    CONFIG_BUILD_OUTPUT_S19
    post_link_commands
    COMMAND ${CMAKE_OBJCOPY} ${GAP_FILL} --srec-len 1 --output-target=srec ${KERNEL_ELF_NAME} ${KERNEL_S19_NAME}
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

endmacro()
