#
# Copyright (c) 2018-2019 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_LIS2MDL lis2mdl.c)
zephyr_library_sources_ifdef(CONFIG_LIS2MDL lis2mdl_i2c.c)
zephyr_library_sources_ifdef(CONFIG_LIS2MDL lis2mdl_spi.c)
zephyr_library_sources_ifdef(CONFIG_LIS2MDL_TRIGGER lis2mdl_trigger.c)
