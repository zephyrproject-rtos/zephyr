# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(tfm_ipc)

set (TFM_ROOT_PATH $ENV{ZEPHYR_BASE}/ext/tfm/tfm)

target_sources(app PRIVATE src/main.c)

target_include_directories(app PRIVATE
  ${TFM_ROOT_PATH}/build/install/export/tfm/include
  )

target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE
  ${TFM_ROOT_PATH}/build/install/export/tfm/veneers/s_veneers.o
  )

target_compile_definitions(app PRIVATE
  -DTFM_PSA_API
  )
target_compile_definitions(app PRIVATE
  -DTFM_PARTITION_TEST_CORE_IPC
  )
