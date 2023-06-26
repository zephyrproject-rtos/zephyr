# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
find_package(Python REQUIRED COMPONENTS Interpreter)

project(http_server_with_poll)

set(HTML_GZIP_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/html_gzip.py)

add_custom_target(
  html_gzip_tool
  COMMAND ${Python_EXECUTABLE} ${HTML_GZIP_SCRIPT}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Running HTML Gzip Tool"
  VERBATIM
)

add_dependencies(app html_gzip_tool)

target_sources(
  app PRIVATE
  ${ZEPHYR_BASE}/subsys/net/lib/http/http_parser.c
  ${ZEPHYR_BASE}/subsys/net/lib/http/http_parser_url.c
  src/main.c
)

target_link_libraries(app PRIVATE zephyr_interface zephyr)
