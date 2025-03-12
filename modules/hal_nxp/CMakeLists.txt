#
# Copyright (c) 2021 Linaro, Limited
# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_HAS_MCUX OR CONFIG_HAS_IMX_HAL OR CONFIG_HAS_NXP_S32_HAL)
  zephyr_library()
  zephyr_compile_definitions_ifdef(CONFIG_CAN_MCUX_FLEXCAN
    FLEXCAN_WAIT_TIMEOUT=${CONFIG_CAN_MCUX_FLEXCAN_WAIT_TIMEOUT})

  if(CONFIG_CPU_HAS_DCACHE)
    zephyr_compile_definitions_ifdef(CONFIG_ENTROPY_MCUX_CAAM CACHE_MODE_WRITE_THROUGH)
  endif()

  if(CONFIG_NOCACHE_MEMORY)
    zephyr_compile_definitions_ifdef(CONFIG_USB_DEVICE_DRIVER DATA_SECTION_IS_CACHEABLE=1)
    zephyr_compile_definitions_ifdef(CONFIG_UDC_DRIVER DATA_SECTION_IS_CACHEABLE=1)
  endif()

  zephyr_compile_definitions_ifdef(CONFIG_ETH_NXP_IMX_NETC FSL_ETH_ENABLE_CACHE_CONTROL=1)

  if(CONFIG_I2S_MCUX_SAI)
    zephyr_compile_definitions(MCUX_SDK_SAI_ALLOW_NULL_FIFO_WATERMARK=1)
  endif()
endif()

if(CONFIG_HAS_NXP_S32_HAL)
  add_subdirectory(s32)
else()
  add_subdirectory_ifdef(CONFIG_HAS_IMX_HAL imx)
  add_subdirectory_ifdef(CONFIG_HAS_MCUX mcux)
endif()

add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/src hal_nxp)
