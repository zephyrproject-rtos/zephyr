# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bap_broadcast_sink)

target_sources(app PRIVATE
  src/main.c
  src/stream_rx.c
)

target_sources_ifdef(CONFIG_ENABLE_LC3 app PRIVATE src/lc3.c)
target_sources_ifdef(CONFIG_STEREO_AUDIO_OUTPUT app PRIVATE src/stereo_out.c)

if(CONFIG_USE_CODEC_AUDIO_OUTPUT)
  target_sources(app PRIVATE src/hw_codec.c)
endif()

target_sources_ifdef(CONFIG_USE_I2S_CODEC_AUDIO_OUTPUT app PRIVATE src/i2s_codec.c)

if(CONFIG_USE_USB_AUDIO_OUTPUT)
  include(${ZEPHYR_BASE}/samples/subsys/usb/common/common.cmake)
  target_sources(app PRIVATE src/usb.c)
endif()
