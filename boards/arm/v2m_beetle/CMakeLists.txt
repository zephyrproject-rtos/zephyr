# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PINMUX_BEETLE)
  zephyr_library_named(pinmux)
  zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
  zephyr_library_sources(pinmux.c)
endif()
