# ST Microelectronics IIS3DHHC accelerometer sensor
#
# Copyright (c) 2019 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(iis3dhhc.c)
zephyr_library_sources(iis3dhhc_spi.c)
zephyr_library_sources_ifdef(CONFIG_IIS3DHHC_TRIGGER    iis3dhhc_trigger.c)

zephyr_library_include_directories(../stmemsc)
