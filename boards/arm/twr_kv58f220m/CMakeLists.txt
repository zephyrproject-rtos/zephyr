# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PINMUX_MCUX)
  zephyr_library_named(pinmux)
  zephyr_library_sources(pinmux.c)
endif()
