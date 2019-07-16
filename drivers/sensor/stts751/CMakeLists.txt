# ST Microelectronics STTS751 temperature sensor
#
# Copyright (c) 2019 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources_ifdef(CONFIG_STTS751            stts751.c)
zephyr_library_sources_ifdef(CONFIG_STTS751            stts751_i2c.c)
zephyr_library_sources_ifdef(CONFIG_STTS751_TRIGGER    stts751_trigger.c)
