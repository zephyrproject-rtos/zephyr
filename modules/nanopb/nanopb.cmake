# Copyright (c) 2023, Basalte bv
#
# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_NANOPB_MODULE_DIR}/extra)

find_package(Nanopb REQUIRED)

if(NOT PROTOBUF_PROTOC_EXECUTABLE)
  message(FATAL_ERROR "'protoc' not found, please ensure protoc is installed\
and in path. See https://docs.zephyrproject.org/latest/samples/modules/nanopb/README.html")
else()
  message(STATUS "Found protoc: ${PROTOBUF_PROTOC_EXECUTABLE}")
endif()

add_custom_target(nanopb_generated_headers)

# Usage:
#   list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
#   include(nanopb)
#
#   zephyr_nanopb_sources(<target> <proto-files>)
#
# Generate source and header files from provided .proto files and
# add these as sources to the specified target.
function(zephyr_nanopb_sources target)
  # Turn off the default nanopb behavior
  set(NANOPB_GENERATE_CPP_STANDALONE OFF)

  nanopb_generate_cpp(proto_srcs proto_hdrs RELPATH ${CMAKE_CURRENT_SOURCE_DIR} ${ARGN})

  target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
  target_sources(${target} PRIVATE ${proto_srcs} ${proto_hdrs})

  # Create unique target name for generated header list
  string(MD5 unique_chars "${proto_hdrs}")
  set(gen_target_name ${target}_proto_${unique_chars})

  add_custom_target(${gen_target_name} DEPENDS ${proto_hdrs})
  add_dependencies(nanopb_generated_headers ${gen_target_name})
endfunction()
