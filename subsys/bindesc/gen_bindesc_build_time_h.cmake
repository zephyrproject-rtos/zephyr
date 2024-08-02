# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Wrapper macro around string(TIMESTAMP ...), that returns the time
# in either local time or UTC, depending on CONFIG_BINDESC_BUILD_TIME_USE_LOCAL_TIME.
macro(get_time out_var format)
  if(BUILD_TIME_TYPE STREQUAL LOCAL)
    string(TIMESTAMP ${out_var} ${format})
  else()
    string(TIMESTAMP ${out_var} ${format} UTC)
  endif()
endmacro()

macro(gen_build_time_int_definition def_name format)
  get_time(${def_name} ${format})
  # remove leading zeros so that the output will not be interpreted as octal
  math(EXPR ${def_name} ${${def_name}})
endmacro()

macro(gen_build_time_str_definition def_name format)
  get_time(${def_name} ${${format}})
endmacro()

gen_build_time_int_definition(BUILD_TIME_YEAR "%Y")
gen_build_time_int_definition(BUILD_TIME_MONTH "%m")
gen_build_time_int_definition(BUILD_TIME_DAY "%d")
gen_build_time_int_definition(BUILD_TIME_HOUR "%H")
gen_build_time_int_definition(BUILD_TIME_MINUTE "%M")
gen_build_time_int_definition(BUILD_TIME_SECOND "%S")
gen_build_time_int_definition(BUILD_TIME_UNIX "%s")

gen_build_time_str_definition(BUILD_DATE_TIME_STRING BUILD_DATE_TIME_STRING_FORMAT)
gen_build_time_str_definition(BUILD_DATE_STRING BUILD_DATE_STRING_FORMAT)
gen_build_time_str_definition(BUILD_TIME_STRING BUILD_TIME_STRING_FORMAT)

file(READ ${IN_FILE} content)
string(CONFIGURE "${content}" content)
file(WRITE ${OUT_FILE} "${content}")
