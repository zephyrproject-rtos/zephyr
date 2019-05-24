# SPDX-License-Identifier: Apache-2.0

set(CONFIGS_C ${CMAKE_CURRENT_BINARY_DIR}/misc/generated/configs.c)

file(STRINGS
  ${AUTOCONF_H}
  LIST_OF_CONFIGS
  REGEX "^#define CONFIG_"
  ENCODING "UTF-8"
  )
foreach (CONFIG ${LIST_OF_CONFIGS})
  string(REGEX REPLACE "#define (CONFIG_[A-Z|_|0-9]*) (.*)" "GEN_ABSOLUTE_SYM(\\1, \\2)" CONFIG ${CONFIG})
  string(REGEX REPLACE "\"(.*)\"" "1" CONFIG ${CONFIG})
  set(GEN_ABS_SYM_LIST "${GEN_ABS_SYM_LIST}${CONFIG};\n")
endforeach()

configure_file(${CMAKE_CURRENT_LIST_DIR}/configs.c.in ${CONFIGS_C})
zephyr_sources(                                       ${CONFIGS_C})
