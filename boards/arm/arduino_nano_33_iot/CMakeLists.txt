# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PINMUX_SAM0)
  zephyr_library()
  zephyr_library_sources(pinmux.c)
endif()
