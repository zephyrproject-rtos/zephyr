# Copyright 2024-2025 NXP
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MIMX9131 OR CONFIG_SOC_MIMX9111)
  add_subdirectory(imx91)
elseif(CONFIG_SOC_MIMX9352)
  add_subdirectory(imx93)
elseif(CONFIG_SOC_MIMX94398)
  add_subdirectory(imx943)
elseif(CONFIG_SOC_MIMX9596)
  add_subdirectory(imx95)
endif()

if(CONFIG_CPU_CORTEX_A55 AND CONFIG_PM)
  zephyr_library_sources_ifdef(CONFIG_PM_CPU_OPS_PSCI pm_acore.c)
endif()
