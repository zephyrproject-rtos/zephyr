# SPDX-License-Identifier: Apache-2.0

set(CONFIGS_C ${CMAKE_CURRENT_BINARY_DIR}/misc/generated/configs.c)

file(READ
  ${AUTOCONF_H}
  config_content
  ENCODING "UTF-8"
)
string(APPEND config_content "\n")
while(config_content)
  # Match content to first newline and store regex match as line.
  # Remaining content is placed in new config_content to find next line on next iteration.
  string(REGEX MATCH "^([^\r\n]*)\r?\n(.*)$" _ "${config_content}")
  set(line "${CMAKE_MATCH_1}")
  set(config_content "${CMAKE_MATCH_2}")

  if(line MATCHES "^#define CONFIG_")
    string(REGEX REPLACE "#define (CONFIG_[A-Za-z|_|0-9]*) (.*)" "GEN_ABSOLUTE_SYM_KCONFIG(\\1, \\2)" config ${line})
    string(REGEX REPLACE "\"(.*)\"" "1" config ${config})
    set(GEN_ABS_SYM_LIST "${GEN_ABS_SYM_LIST}${config};\n")
  endif()
endwhile()

configure_file(${CMAKE_CURRENT_LIST_DIR}/configs.c.in ${CONFIGS_C})
zephyr_sources(                                       ${CONFIGS_C})
