#
# Copyright (c) 2019 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_HTS221         hts221.c)
zephyr_library_sources_ifdef(CONFIG_HTS221         hts221_i2c.c)
zephyr_library_sources_ifdef(CONFIG_HTS221         hts221_spi.c)
zephyr_library_sources_ifdef(CONFIG_HTS221_TRIGGER hts221_trigger.c)
