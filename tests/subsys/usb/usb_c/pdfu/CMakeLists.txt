# Copyright (c) 2026 Meta Platforms, Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(usbc_pdfu)

# Both roles, built against the fake usbc_send_ext_msg() in main.c rather than
# the USB-C stack, which needs a TCPC.
set(PDFU_DIR ${ZEPHYR_BASE}/subsys/usb/usb_c/pdfu)
target_sources(app PRIVATE
  src/main.c
  ${PDFU_DIR}/pdfu_responder.c
  ${PDFU_DIR}/pdfu_initiator.c
)
target_include_directories(app PRIVATE ${PDFU_DIR})
