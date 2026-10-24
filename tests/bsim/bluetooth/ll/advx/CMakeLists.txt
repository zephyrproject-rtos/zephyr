# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_advx)

target_sources(app PRIVATE
  src/main.c
  src/test_aux_connect.c
  src/test_aux_connect_preempt.c
)

# This contains babblesim-specific helpers, e.g. the pass and fail macros.
add_subdirectory(${ZEPHYR_BASE}/tests/bsim/babblekit babblekit)
target_link_libraries(app PRIVATE babblekit)

zephyr_include_directories(
  ${ZEPHYR_BASE}
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/include
  )

# The preempt test recognizes the AUX_CONNECT_RSP that the Controller sets up.
target_link_options(app PUBLIC -Wl,--wrap=radio_pkt_tx_set)
