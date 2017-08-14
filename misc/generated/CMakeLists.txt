if(CONFIG_MDEF)
  file(STRINGS ${AUTOCONF_H} CONFIG_LIST REGEX "^#define CONFIG_")
  foreach (CONFIG ${CONFIG_LIST})
    string(REGEX REPLACE "#define (CONFIG_[A-Z|_|0-9]*) (.*)" "GEN_ABSOLUTE_SYM(\\1, \\2)" CONFIG ${CONFIG})
    string(REGEX REPLACE "\"(.*)\"" "1" CONFIG ${CONFIG})
    set(GEN_ABS_SYM_LIST "${GEN_ABS_SYM_LIST}${CONFIG};\n")
  endforeach()
  configure_file(${CMAKE_CURRENT_LIST_DIR}/configs.c.in ${CMAKE_CURRENT_BINARY_DIR}/configs.c)

  target_sources(zephyr PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/configs.c)

endif()
