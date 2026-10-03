# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(handsfree)

target_sources(app PRIVATE
  src/codec.c
  src/main.c
  )
if(CONFIG_BT_VOICE_OVER_HCI)
  target_sources(app PRIVATE src/voice_over_hci.c)
else()
  target_sources(app PRIVATE src/voice_over_pcm.c)
endif()

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
