# Makefile - STLIB SDK
#
# Copyright (c) 2016 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HAS_STLIB)

zephyr_library()

if(CONFIG_VL53L0X)
zephyr_include_directories(${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l0x/api/core/inc
			   ${PROJECT_SOURCE_DIR}/drivers/sensor/st/vl53l0x)
zephyr_library_sources(${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l0x/api/core/src/vl53l0x_api.c)
zephyr_library_sources(${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l0x/api/core/src/vl53l0x_api_ranging.c)
zephyr_library_sources(${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l0x/api/core/src/vl53l0x_api_calibration.c)
zephyr_library_sources(${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l0x/api/core/src/vl53l0x_api_core.c)
zephyr_library_sources(${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l0x/api/core/src/vl53l0x_api_strings.c)
endif()

if(CONFIG_VL53L1X)
zephyr_include_directories(${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l1x/api/core/inc
			   ${PROJECT_SOURCE_DIR}/drivers/sensor/st/vl53l1x)
zephyr_library_sources(${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l1x/api/core/src/vl53l1_api.c)
zephyr_library_sources(${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l1x/api/core/src/vl53l1_api_calibration.c)
zephyr_library_sources(${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l1x/api/core/src/vl53l1_wait.c)
zephyr_library_sources(${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l1x/api/core/src/vl53l1_core.c)
zephyr_library_sources(${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l1x/api/core/src/vl53l1_api_core.c)
zephyr_library_sources(${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l1x/api/core/src/vl53l1_api_preset_modes.c)
zephyr_library_sources(${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l1x/api/core/src/vl53l1_core_support.c)
zephyr_library_sources(${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l1x/api/core/src/vl53l1_register_funcs.c)
endif()


if(CONFIG_AUDIO_DMIC_MPXXDTYY)
zephyr_include_directories(${ZEPHYR_HAL_ST_MODULE_DIR}/audio/microphone)
zephyr_library_sources(${ZEPHYR_HAL_ST_MODULE_DIR}/audio/microphone/OpenPDMFilter.c)
endif()

endif()

# STMEMSC - Hardware Abstraction Layer for ST sensor
add_subdirectory_ifdef(CONFIG_HAS_STMEMSC sensor/stmemsc)

# Add sensor common include files
zephyr_include_directories_ifdef(CONFIG_HAS_STMEMSC ${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/inc)
