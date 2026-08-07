# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ina2xx_attr.c ina2xx_common.c ina2xx_fetch.c ina2xx_get.c ina2xx_trigger.c)
zephyr_library_sources_ifdef(CONFIG_INA226 ina226.c)
zephyr_library_sources_ifdef(CONFIG_INA230 ina230.c)
zephyr_library_sources_ifdef(CONFIG_INA237 ina237.c)
zephyr_library_sources_ifdef(CONFIG_INA230_TRIGGER ina230_trigger.c)
