# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_app)

target_sources(app PRIVATE
  src/main.c
  src/test_empty.c
  src/test_connect1.c
  src/test_connect2.c
  src/test_scan_window.c
)

if(CONFIG_BT_CTLR_CONN_META)
  # The controller's lll_conn.h includes the test's lll_conn_meta.h.
  zephyr_include_directories(meta)
  target_sources(app PRIVATE src/conn_meta.c)
  target_include_directories(app PRIVATE
    ${ZEPHYR_BASE}/subsys/bluetooth/controller
    ${ZEPHYR_BASE}/subsys/bluetooth/controller/include
    ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw
    ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic
  )
endif()

# This contains babblesim-specific helpers, e.g. the pass and fail macros.
add_subdirectory(${ZEPHYR_BASE}/tests/bsim/babblekit babblekit)
target_link_libraries(app PRIVATE babblekit)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  ${ZEPHYR_BASE}/samples/bluetooth
)
