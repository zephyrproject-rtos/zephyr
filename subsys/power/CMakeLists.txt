# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_PM    power.c)
zephyr_sources_ifdef(CONFIG_PM    pm_ctrl.c)
zephyr_sources_ifdef(CONFIG_PM_DEVICE device.c)
zephyr_sources_ifdef(CONFIG_PM_DEVICE device_pm.c)
add_subdirectory(policy)
