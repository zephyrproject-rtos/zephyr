# Copyright 2023 The ChromiumOS Authors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MT8186_ADSP OR CONFIG_SOC_MT8188_ADSP OR CONFIG_SOC_MT8195_ADSP
   OR CONFIG_SOC_MT8196_ADSP OR CONFIG_SOC_MT8365_ADSP)
  add_subdirectory(common/adsp)
endif()

# Dispatch on the SoC rather than the series: a series may span several chips,
# so only the SoC name is guaranteed to match a directory here.
add_subdirectory(${CONFIG_SOC})
