# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ina23x_common.c ina23x_trigger.c)
zephyr_library_sources_ifdef(CONFIG_INA230 ina230.c)
zephyr_library_sources_ifdef(CONFIG_INA237 ina237.c)
zephyr_library_sources_ifdef(CONFIG_INA230_TRIGGER ina230_trigger.c)
