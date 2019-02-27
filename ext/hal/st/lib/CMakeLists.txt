# Makefile - STLIB SDK
#
# Copyright (c) 2016 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HAS_STLIB)
  if(CONFIG_VL53L0X)
    zephyr_include_directories(
	sensor/vl53l0x/api/core/inc
	${PROJECT_SOURCE_DIR}/drivers/sensor/vl53l0x
    )

    zephyr_sources(sensor/vl53l0x/api/core/src/vl53l0x_api.c)
    zephyr_sources(sensor/vl53l0x/api/core/src/vl53l0x_api_ranging.c)
    zephyr_sources(sensor/vl53l0x/api/core/src/vl53l0x_api_calibration.c)
    zephyr_sources(sensor/vl53l0x/api/core/src/vl53l0x_api_core.c)
    zephyr_sources(sensor/vl53l0x/api/core/src/vl53l0x_api_strings.c)
  endif()
  if(CONFIG_AUDIO_MPXXDTYY)
    zephyr_include_directories(audio/microphone)
    zephyr_sources(audio/microphone/OpenPDMFilter.c)
  endif()
  if(CONFIG_BT_STM32_IPM)
    zephyr_include_directories(stm32wb/hci)
    zephyr_sources(stm32wb/hci/shci.c)
    zephyr_sources(stm32wb/hci/shci_tl.c)
    zephyr_sources(stm32wb/hci/hw_ipcc.c)
    zephyr_sources(stm32wb/hci/stm_list.c)
    zephyr_sources(stm32wb/hci/tl_mbox.c)
    zephyr_sources(stm32wb/hci/tl_if.c)
  endif()
endif()
