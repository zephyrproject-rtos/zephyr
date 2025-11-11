# ST Microelectronics LSM6DSV16X 6-axis IMU sensor driver
#
# Copyright (c) 2023 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(lsm6dsv16x.c)
zephyr_library_sources_ifdef(CONFIG_LSM6DSV16X_SENSORHUB  lsm6dsv16x_shub.c)
zephyr_library_sources_ifdef(CONFIG_LSM6DSV16X_TRIGGER    lsm6dsv16x_trigger.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API lsm6dsv16x_rtio.c lsm6dsv16x_decoder.c)
zephyr_library_sources_ifdef(CONFIG_LSM6DSV16X_STREAM lsm6dsv16x_rtio_stream.c)

zephyr_library_include_directories(../stmemsc)
