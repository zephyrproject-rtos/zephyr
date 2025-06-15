# Copyright (c) 2025 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_XEN)
  set(XEN_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  zephyr_include_directories(${XEN_DIR}/include)

  zephyr_compile_definitions(__XEN_TOOLS__=1)
endif()
