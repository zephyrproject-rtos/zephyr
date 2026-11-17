# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_gatt_notify_fanout)

add_subdirectory("${ZEPHYR_BASE}/tests/bsim/babblekit" babblekit)
target_link_libraries(app PRIVATE babblekit)
target_sources(app PRIVATE
  src/main.c
)
if(CONFIG_BT_AICS)
  target_sources(app PRIVATE src/audio.c)
  target_link_options(app PUBLIC -Wl,--wrap=bt_gatt_notify_cb)
endif()
target_include_directories(app PRIVATE "${ZEPHYR_BASE}/subsys/bluetooth")

zephyr_include_directories(
  "${BSIM_COMPONENTS_PATH}/libUtilv1/src/"
  "${BSIM_COMPONENTS_PATH}/libPhyComv1/src/"
)
