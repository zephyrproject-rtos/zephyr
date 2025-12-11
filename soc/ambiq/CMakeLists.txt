# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023 Antmicro <www.antmicro.com>

add_subdirectory(${SOC_SERIES})

zephyr_include_directories(.)

if(CONFIG_SOC_AMBIQ_BT_SUPPORTED)
  if(CONFIG_SOC_APOLLO4P)
    if(CONFIG_AMBIQ_COMPONENT_USE_BT)
      message(FATAL_ERROR "AMBIQ_COMPONENT_USE_BT is set to ${CONFIG_AMBIQ_COMPONENT_USE_BT} \
for not supported SoC ${CONFIG_SOC}")
    else()
      message(WARNING "SOC_AMBIQ_BT_SUPPORTED is set to ${CONFIG_SOC_AMBIQ_BT_SUPPORTED} \
for not supported SoC ${CONFIG_SOC}")
    endif()
  endif()
endif()
