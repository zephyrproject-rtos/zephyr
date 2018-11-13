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
endif()
