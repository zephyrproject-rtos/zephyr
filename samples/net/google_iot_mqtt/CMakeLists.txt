cmake_minimum_required(VERSION 3.8.2)

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
project(NONE)

if(NOT EXISTS ${APPLICATION_SOURCE_DIR}/src/private_info/key.c)
  message(FATAL_ERROR "!!!!!! Generate key file before continuing.  See README !!!!!")
endif()

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
target_sources(app PRIVATE ${APPLICATION_SOURCE_DIR}/src/private_info/key.c)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/src/tls_config)
