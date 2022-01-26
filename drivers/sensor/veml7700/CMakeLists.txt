# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_VEML7700 veml7700.c)
zephyr_library_sources_ifdef(CONFIG_VEML7700_INT_ENABLE veml7700_trigger.c)
