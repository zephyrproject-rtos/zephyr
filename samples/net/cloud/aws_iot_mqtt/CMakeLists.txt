# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aws_iot_mqtt)

if(USE_DUMMY_CREDS)
  set(creds "src/creds/dummy.c")
else()
  if(NOT EXISTS ${APPLICATION_SOURCE_DIR}/src/creds/key.c OR
     NOT EXISTS ${APPLICATION_SOURCE_DIR}/src/creds/cert.c OR
     NOT EXISTS ${APPLICATION_SOURCE_DIR}/src/creds/ca.c)
    message(FATAL_ERROR
        "Credentials not found. Please run "
        "'python3 src/creds/convert_keys.py' before building"
    )
  endif()

  set(creds "src/creds/ca.c" "src/creds/key.c" "src/creds/cert.c")
endif()

target_sources(app PRIVATE "src/main.c" ${creds})

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
