# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(xhci_bulk_ring)

zephyr_include_directories(${ZEPHYR_BASE}/drivers/usb/uhc)

target_sources(app PRIVATE
  src/test_xhci_bulk_ring.c
  ${ZEPHYR_BASE}/drivers/usb/uhc/xhci_bulk.c
  ${ZEPHYR_BASE}/drivers/usb/uhc/xhci_ring.c
)
