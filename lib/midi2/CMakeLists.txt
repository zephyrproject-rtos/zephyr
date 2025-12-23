# Copyright (c) 2025 Titouan Christophe
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_MIDI2_UMP_STREAM_RESPONDER)
  zephyr_sources(ump_stream_responder.c)
endif()
