# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(avdtp_disconnect)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/bluetooth)
target_sources(app PRIVATE src/main.c)
set_property(SOURCE ${ZEPHYR_BASE}/subsys/bluetooth/host/l2cap.c
  DIRECTORY ${ZEPHYR_BASE}/subsys/bluetooth/host
  APPEND PROPERTY COMPILE_DEFINITIONS
  bt_l2cap_chan_connect=real_bt_l2cap_chan_connect
  bt_l2cap_create_pdu_timeout=real_bt_l2cap_create_pdu_timeout
)

set_property(SOURCE ${ZEPHYR_BASE}/subsys/bluetooth/host/classic/l2cap_br.c
  DIRECTORY ${ZEPHYR_BASE}/subsys/bluetooth/host/classic
  APPEND PROPERTY COMPILE_DEFINITIONS
  bt_l2cap_br_chan_send_cb=real_bt_l2cap_br_chan_send_cb
)

set_property(SOURCE ${ZEPHYR_BASE}/subsys/bluetooth/host/hci_core.c
  DIRECTORY ${ZEPHYR_BASE}/subsys/bluetooth/host
  APPEND PROPERTY COMPILE_DEFINITIONS
  bt_work_submit=real_bt_work_submit
  bt_work_reschedule=real_bt_work_reschedule
)
