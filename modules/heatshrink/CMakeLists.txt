# Copyright (c) 2023 Kickmaker
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HEATSHRINK)

set(HEATSHRINK_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

zephyr_library()
zephyr_include_directories(${HEATSHRINK_DIR})

zephyr_library_sources_ifdef(CONFIG_HEATSHRINK_DECODER
  ${HEATSHRINK_DIR}/heatshrink_decoder.c
)
zephyr_library_sources_ifdef(CONFIG_HEATSHRINK_ENCODER
  ${HEATSHRINK_DIR}/heatshrink_encoder.c
)

endif()
