# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PM)
  zephyr_sources(pm.c state.c)
  zephyr_sources_ifdef(CONFIG_PM_STATS pm_stats.c)
endif()

add_subdirectory(policy)

zephyr_sources_ifdef(CONFIG_PM_DEVICE device.c)
zephyr_sources_ifdef(CONFIG_PM_DEVICE_RUNTIME device_runtime.c)
zephyr_sources_ifdef(CONFIG_PM_DEVICE_SHELL pm_shell.c)
zephyr_sources_ifdef(CONFIG_PM_DEVICE_SYSTEM_MANAGED device_system_managed.c)
