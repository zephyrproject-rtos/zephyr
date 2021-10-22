# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.8.2)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(wasm_app_manager)

target_sources(app PRIVATE src/main.c)

set(WAMR_DIR ${ZEPHYR_WASM_MICRO_RUNTIME_MODULE_DIR})

add_custom_target(wamr-sdk ALL
  COMMAND
    echo "Building wamr-sdk .." &&
    ./build_sdk.sh -n simple
      -x ${CMAKE_CURRENT_SOURCE_DIR}/wamr_sdk_config.cmake -c
  BYPRODUCTS
    ${WAMR_DIR}/wamr-sdk/out/simple
  WORKING_DIRECTORY
    ${WAMR_DIR}/wamr-sdk
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
  add_custom_target(${WASM_APP}_WASM ALL
    COMMAND
      echo "Building ${WASM_APP}.wasm .." &&
      mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps &&
      /opt/wasi-sdk/bin/clang -O3 
        -I${WAMR_DIR}/wamr-sdk/out/simple/app-sdk/wamr-app-framework/include
        -L${WAMR_DIR}/wamr-sdk/out/simple/app-sdk/wamr-app-framework/lib
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
    BYPRODUCTS
      ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/${WASM_APP}.wasm
    USES_TERMINAL
  )
endforeach()

add_custom_target(host-tool ALL
  COMMAND
    echo "Building host-tool .." &&
    rm -fr build && mkdir build && cd build &&
    cmake .. && make &&
    cp -a host_tool ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS
    wamr-sdk
  BYPRODUCTS
    ${CMAKE_CURRENT_BINARY_DIR}/host_tool
  WORKING_DIRECTORY
    ${WAMR_DIR}/test-tools/host-tool
  USES_TERMINAL
)
