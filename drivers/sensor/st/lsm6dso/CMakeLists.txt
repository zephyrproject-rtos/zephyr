# ST Microelectronics LSM6DSO 6-axis IMU sensor driver
#
# Copyright (c) 2019 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(lsm6dso.c)
zephyr_library_sources_ifdef(CONFIG_LSM6DSO_SENSORHUB  lsm6dso_shub.c)
zephyr_library_sources_ifdef(CONFIG_LSM6DSO_TRIGGER    lsm6dso_trigger.c)

zephyr_library_include_directories(../stmemsc)
