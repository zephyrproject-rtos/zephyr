#
# Copyright (c) 2019, Microchip Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(
  soc.c
  )

zephyr_sources_ifdef(CONFIG_SYS_POWER_MANAGEMENT
  device_power.c
  power.c
  )
