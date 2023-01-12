# Copyright (c) 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(rpc_demo)

add_definitions(-DVERSION=5)
add_definitions(-DCONFIG_RPC_BUFFER_SIZE=1024)
add_definitions(-DPW_LOG_LEVEL=PW_LOG_LEVEL_INFO)
add_definitions(-DPW_LOG_SHOW_MODULE=1)

# Define the proto library
include($ENV{PW_ROOT}/pw_protobuf_compiler/proto.cmake)
pw_proto_library(
    rpc_demo.protos
    SOURCES proto/demo.proto
)

file(GLOB app_sources src/*.cpp)
target_sources(app
  PRIVATE
    ${app_sources}
)
zephyr_include_directories(include)
zephyr_link_libraries(
    rpc_demo.protos.nanopb
    rpc_demo.protos.nanopb_rpc
)
target_link_libraries(app
  PRIVATE
    rpc_demo.protos.nanopb
    rpc_demo.protos.nanopb_rpc
)
