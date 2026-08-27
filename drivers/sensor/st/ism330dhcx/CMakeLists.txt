# ST Microelectronics ISM330DHCX 6-axis IMU sensor driver
#
# Copyright (c) 2020 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(ism330dhcx.c)
zephyr_library_sources(ism330dhcx_i2c.c)
zephyr_library_sources(ism330dhcx_spi.c)
zephyr_library_sources_ifdef(CONFIG_ISM330DHCX_SENSORHUB  ism330dhcx_shub.c)
zephyr_library_sources_ifdef(CONFIG_ISM330DHCX_TRIGGER    ism330dhcx_trigger.c)

zephyr_library_include_directories(../stmemsc)
