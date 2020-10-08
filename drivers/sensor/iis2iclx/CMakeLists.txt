# ST Microelectronics IIS2ICLX 2-axis accelerometer sensor driver
#
# Copyright (c) 2020 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources_ifdef(CONFIG_IIS2ICLX            iis2iclx.c)
zephyr_library_sources_ifdef(CONFIG_IIS2ICLX            iis2iclx_i2c.c)
zephyr_library_sources_ifdef(CONFIG_IIS2ICLX            iis2iclx_spi.c)
zephyr_library_sources_ifdef(CONFIG_IIS2ICLX_SENSORHUB  iis2iclx_shub.c)
zephyr_library_sources_ifdef(CONFIG_IIS2ICLX_TRIGGER    iis2iclx_trigger.c)
