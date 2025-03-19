# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bap_unicast_server)

target_sources(app PRIVATE
  src/main.c
  src/stream_rx.c
)

zephyr_sources_ifdef(CONFIG_LIBLC3 src/lc3.c)
zephyr_sources_ifdef(CONFIG_USB_DEVICE_AUDIO src/usb.c)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
