#
# Copyright (c) 2020 Lemonbeat GmbH
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_PINMUX_MCUX_LPC)
  zephyr_library_named(pinmux)
  zephyr_library_sources(pinmux.c)
endif()
