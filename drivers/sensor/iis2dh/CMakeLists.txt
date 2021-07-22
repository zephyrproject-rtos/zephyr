# ST Microelectronics IIS2DH 3-axis accelerometer driver
#
# Copyright (c) 2020 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(iis2dh.c)
zephyr_library_sources(iis2dh_i2c.c)
zephyr_library_sources(iis2dh_spi.c)
zephyr_library_sources_ifdef(CONFIG_IIS2DH_TRIGGER    iis2dh_trigger.c)
