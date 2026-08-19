# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(openthread_border_router)

FILE(GLOB app_sources src/main.c)

# add Web Server source files
if(CONFIG_OTBR_WEB_SERVER)
  set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

  if(${CONFIG_OPENTHREAD_NET_DIAG_VENDOR_NAME} STREQUAL "NXP")
    set(VENDOR_LOGO "nxp_logo.png")
  else()
    set(VENDOR_LOGO "zephyr_logo.png")
  endif()

  file(READ ${CMAKE_CURRENT_SOURCE_DIR}/src/web_server/static_web_resources/index.html INDEX_HTML_CONTENT)
  string(REPLACE "@BOARD_NAME@" "${CONFIG_OPENTHREAD_NET_DIAG_VENDOR_MODEL}" INDEX_HTML_CONTENT "${INDEX_HTML_CONTENT}")
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/index.html "${INDEX_HTML_CONTENT}")

  generate_inc_file_for_target(app
    ${CMAKE_CURRENT_BINARY_DIR}/index.html
    ${gen_dir}/index.html.gz.inc
    --gzip
  )
  generate_inc_file_for_target(app
    src/web_server/static_web_resources/images/${VENDOR_LOGO}
    ${gen_dir}/images/vendor_logo.png.gz.inc
    --gzip
  )

  foreach(web_resource
    style.css
    app.js
    images/zephyr_logo.png
  )
  generate_inc_file_for_target(app
  src/web_server/static_web_resources/${web_resource}
  ${gen_dir}/${web_resource}.gz.inc
  --gzip
  )
  endforeach()

  message(STATUS "Adding Web Server and Rest APIs source files")
  list(APPEND app_sources src/web_server/web_server.c)
  list(APPEND app_sources src/web_server/rest_api.c)

  message(STATUS "Adding HTTP resource linker sections")
  zephyr_linker_sources(SECTIONS src/web_server/sections-rom.ld)
endif()

target_sources(app PRIVATE ${app_sources})
