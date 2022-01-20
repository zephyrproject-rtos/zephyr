# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PM)
  zephyr_sources(pm.c constraint.c state.c)
  zephyr_sources_ifdef(CONFIG_PM_STATS pm_stats.c)
  zephyr_sources_ifdef(CONFIG_PM_POLICY_DEFAULT policy.c)
endif()

zephyr_sources_ifdef(CONFIG_PM_DEVICE device.c)
zephyr_sources_ifdef(CONFIG_PM_DEVICE_RUNTIME device_runtime.c)
