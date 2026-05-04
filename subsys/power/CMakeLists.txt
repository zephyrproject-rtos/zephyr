# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_POWER_ENERGY_MODEL energy_model.c)
zephyr_library_sources_ifdef(CONFIG_POWER_QOS qos.c)
