# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Option to enable web server and REST API
option(ENABLE_WEB_SERVER "Enable HTTP web server and REST API" OFF)

# Conditionally add web server configuration BEFORE find_package(Zephyr)
if(ENABLE_WEB_SERVER)
  list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/overlay-webserver.conf)
  message(STATUS "Web server enabled - adding overlay-webserver.conf")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(openthread_border_router)


FILE(GLOB app_sources src/main.c)
  target_compile_definitions(app PRIVATE OTBR_ENABLE_WEB_SERVER=0)
# add web server and REST api source files
if(ENABLE_WEB_SERVER)
  message(STATUS "Adding Web Server and Rest APIs source files")
  target_compile_definitions(app PRIVATE OTBR_ENABLE_WEB_SERVER=1)
  list(APPEND app_sources src/web_server.c)
  list(APPEND app_sources src/rest_api.c)

  message(STATUS "Adding HTTP resource linker sections")
  zephyr_linker_sources(SECTIONS sections-rom.ld)
endif()

target_sources(app PRIVATE ${app_sources})
