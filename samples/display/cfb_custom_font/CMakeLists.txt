cmake_minimum_required(VERSION 3.8.2)
macro(set_conf_file)
  if(EXISTS ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf)
    set(CONF_FILE "prj.conf ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf")
  else()
    set(CONF_FILE "prj.conf")
  endif()
endmacro()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
include($ENV{ZEPHYR_BASE}/cmake/cfb.cmake NO_POLICY_SCOPE)
project(cfb_custom_font)

target_sources(app PRIVATE src/main.c)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
generate_cfb_font_for_target(app fonts/dice.png ${gen_dir}/cfb_font_dice.h 32 32 --first 49 --last 54)
