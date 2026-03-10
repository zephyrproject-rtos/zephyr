# ST Microelectronics LSM6DSVXXX accelerometer sensor
#
# Copyright (c) 2025 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(lsm6dsvxxx.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API lsm6dsvxxx_decoder.c)
zephyr_library_sources_ifdef(CONFIG_LSM6DSVXXX_STREAM lsm6dsvxxx_stream.c)
zephyr_library_sources_ifdef(CONFIG_LSM6DSVXXX_TRIGGER  lsm6dsvxxx_trigger.c)

zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_LSM6DSV320X_ENABLED lsm6dsv320x.c)
zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_LSM6DSV80X_ENABLED lsm6dsv80x.c)
zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_ISM6HG256X_ENABLED ism6hg256x.c)

zephyr_library_include_directories(../stmemsc)
