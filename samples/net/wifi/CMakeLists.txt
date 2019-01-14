cmake_minimum_required(VERSION 3.13.1)

macro(set_conf_file)
  if(EXISTS         ${APPLICATION_SOURCE_DIR}/prj_${BOARD}.conf)
    set(CONF_FILE "${APPLICATION_SOURCE_DIR}/prj_${BOARD}.conf")
  elseif(EXISTS         ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf)
    set(CONF_FILE "prj_base.conf ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf")
  else()
    set(CONF_FILE "prj_base.conf")
  endif()
endmacro()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(wifi)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
