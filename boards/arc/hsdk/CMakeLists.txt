#
# Copyright (c) 2019 Synopsys, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
if(CONFIG_PINMUX_HSDK)
  zephyr_library_named(pinmux)
  zephyr_library_sources(pinmux.c)
endif()
