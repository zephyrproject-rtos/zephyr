# ST Microelectronics IIS3DWB accelerometer sensor
#
# Copyright (c) 2025 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(iis3dwb.c)
zephyr_library_sources_ifdef(CONFIG_IIS3DWB_TRIGGER  iis3dwb_trigger.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API iis3dwb_decoder.c)
zephyr_library_sources_ifdef(CONFIG_IIS3DWB_STREAM iis3dwb_stream.c)

zephyr_library_include_directories(../stmemsc)
