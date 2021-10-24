# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.8.2)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(wasm_app_manager)

target_sources(app PRIVATE src/main.c)

if(NOT $ENV{WASI_SDK_PATH} STREQUAL "")
  set(WASI_SDK_PATH $ENV{WASI_SDK_PATH})
else()
  find_program(WASM_C_COMPILER clang /opt/wasi-sdk/bin NO_DEFAULT_PATH)
  if(NOT WASM_C_COMPILER)
    message(FATAL_ERROR "'wasi-sdk' not found, please ensure wasi-sdk is installed.\
                         You can download and install it from\
                         https://github.com/WebAssembly/wasi-sdk/releases")
  else()
    set(WASI_SDK_PATH /opt/wasi-sdk)
  endif()
endif()

message("wasi-sdk was found at ${WASI_SDK_PATH}")

set(WAMR_DIR ${ZEPHYR_WASM_MICRO_RUNTIME_MODULE_DIR})

set(WAMR_APP_FRAMEWORK_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/wamr-sdk/app-sdk/wamr-app-framework
)

include(ExternalProject)

ExternalProject_Add(wamr-sdk
  SOURCE_DIR
    ${WAMR_DIR}/wamr-sdk/app
  BINARY_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/wamr-sdk
  CONFIGURE_COMMAND
    ${CMAKE_COMMAND} -G${CMAKE_GENERATOR} ${WAMR_DIR}/wamr-sdk/app
      -DWAMR_BUILD_SDK_PROFILE=simple
      -DCONFIG_PATH=${CMAKE_CURRENT_SOURCE_DIR}/wamr_sdk_config.cmake
      -DCMAKE_TOOLCHAIN_FILE=${WAMR_DIR}/wamr-sdk/app/wamr_toolchain.cmake
      -DOUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/wamr-sdk
      -DWASI_SDK_DIR=${WASI_SDK_PATH}
  BUILD_COMMAND
    ${CMAKE_COMMAND} --build .
  INSTALL_COMMAND
    ""
  BUILD_BYPRODUCTS
    ${WAMR_APP_FRAMEWORK_DIR}/include/wasm_app.h
    ${WAMR_APP_FRAMEWORK_DIR}/lib/libapp_framework.a
  USES_TERMINAL
)

set(WASM_APPS
    timer
    request_handler
    request_sender
    event_publisher
    event_subscriber
)

foreach(WASM_APP ${WASM_APPS})
  add_custom_target(${WASM_APP} ALL
    COMMENT
      "Building ${WASM_APP}.wasm .."
    COMMAND
      ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps
    COMMAND
      ${WASI_SDK_PATH}/bin/clang -O3
        -I${WAMR_APP_FRAMEWORK_DIR}/include
        -L${WAMR_APP_FRAMEWORK_DIR}/lib
        -lapp_framework
        -z stack-size=8192 -Wl,--initial-memory=65536
        -Wl,--no-entry -nostdlib -Wl,--allow-undefined
        -Wl,--export=__heap_base,--export=__data_end
        -Wl,--export=on_init -Wl,--export=on_destroy
        -Wl,--export=on_request -Wl,--export=on_response
        -Wl,--export=on_sensor_event -Wl,--export=on_timer_callback
        -Wl,--export=on_connection_data
        -o ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/${WASM_APP}.wasm
        ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/${WASM_APP}.c
    DEPENDS
      wamr-sdk
      ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/${WASM_APP}.c
    BYPRODUCTS
      ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/${WASM_APP}.wasm
    USES_TERMINAL
  )
endforeach()

ExternalProject_Add(host-tool
  SOURCE_DIR
    ${WAMR_DIR}/test-tools/host-tool
  BINARY_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/host-tool-build
  CONFIGURE_COMMAND
    ${CMAKE_COMMAND} -G${CMAKE_GENERATOR} ${WAMR_DIR}/test-tools/host-tool
  BUILD_COMMAND
    ${CMAKE_COMMAND} --build .
  INSTALL_COMMAND
    ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_BINARY_DIR}/host-tool-build/host_tool
      ${CMAKE_CURRENT_BINARY_DIR}/host_tool
  BUILD_BYPRODUCTS
    ${CMAKE_CURRENT_BINARY_DIR}/host_tool
  USES_TERMINAL
)
