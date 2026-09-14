# SPDX-License-Identifier: Apache-2.0

set(CONFIGS_C ${CMAKE_CURRENT_BINARY_DIR}/misc/generated/configs.c)

file(STRINGS
  ${AUTOCONF_H}
  LIST_OF_CONFIGS
  REGEX "^#define CONFIG_"
  ENCODING "UTF-8"
  )

# CMake's list parsing treats '[' and ']' as bracket quoting, so the ';'
# separators that follow an unmatched bracket are not honoured and every
# remaining line is merged into a single element. Without this, every CONFIG_
# defined after the first one whose value contains a bracket is missing from
# the generated absolute symbols. Substitute the brackets with placeholders
# that cannot occur in autoconf.h and restore them inside the loop.
string(ASCII 1 LBRACKET_PLACEHOLDER)
string(ASCII 2 RBRACKET_PLACEHOLDER)
string(REPLACE "[" "${LBRACKET_PLACEHOLDER}" LIST_OF_CONFIGS "${LIST_OF_CONFIGS}")
string(REPLACE "]" "${RBRACKET_PLACEHOLDER}" LIST_OF_CONFIGS "${LIST_OF_CONFIGS}")

foreach (CONFIG IN LISTS LIST_OF_CONFIGS)
  string(REPLACE "${LBRACKET_PLACEHOLDER}" "[" CONFIG "${CONFIG}")
  string(REPLACE "${RBRACKET_PLACEHOLDER}" "]" CONFIG "${CONFIG}")
  string(REGEX REPLACE "#define (CONFIG_[A-Za-z|_|0-9]*) (.*)" "GEN_ABSOLUTE_SYM_KCONFIG(\\1, \\2)" CONFIG "${CONFIG}")
  string(REGEX REPLACE "\"(.*)\"" "1" CONFIG "${CONFIG}")
  set(GEN_ABS_SYM_LIST "${GEN_ABS_SYM_LIST}${CONFIG};\n")
endforeach()

configure_file(${CMAKE_CURRENT_LIST_DIR}/configs.c.in ${CONFIGS_C})
zephyr_sources(                                       ${CONFIGS_C})
