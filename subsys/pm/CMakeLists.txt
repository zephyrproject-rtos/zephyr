# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PM)
  zephyr_sources(pm.c policy.c state.c)
  zephyr_sources_ifdef(CONFIG_PM_STATS pm_stats.c)
endif()

if(CONFIG_PM_DEVICE OR CONFIG_PM_DEVICE_RUNTIME)
  zephyr_sources(device.c)
endif()

zephyr_sources_ifdef(CONFIG_PM_DEVICE_RUNTIME device_runtime.c)
