# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bt_hci_pkt)

target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/subsys/bluetooth/common/hci_pkt.c
)
