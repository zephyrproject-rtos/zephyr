# ST Microelectronics LPS22HH pressure and temperature sensor
#
# Copyright (c) 2019 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LPS22HH            lps22hh.c)
zephyr_library_sources_ifdef(CONFIG_LPS22HH            lps22hh_i2c.c)
zephyr_library_sources_ifdef(CONFIG_LPS22HH            lps22hh_spi.c)
zephyr_library_sources_ifdef(CONFIG_LPS22HH_TRIGGER    lps22hh_trigger.c)
