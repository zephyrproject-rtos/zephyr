# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ina23x.c)
zephyr_library_sources_ifdef(CONFIG_INA23X_TRIGGER ina23x_trigger.c)
