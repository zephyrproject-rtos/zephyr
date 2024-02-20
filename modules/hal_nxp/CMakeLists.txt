#
# Copyright (c) 2021 Linaro, Limited
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_HAS_MCUX OR CONFIG_HAS_IMX_HAL OR CONFIG_HAS_NXP_S32_HAL)
  add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR} hal_nxp)
  add_subdirectory_ifdef(CONFIG_USB_DEVICE_DRIVER usb)

  zephyr_sources_ifdef(CONFIG_PWM_MCUX_CTIMER ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/ctimer/fsl_ctimer.c)
  zephyr_include_directories_ifdef(CONFIG_PWM_MCUX_CTIMER
    ${ZEPHYR_CURRENT_MODULE_DIR}/mcux/mcux-sdk/drivers/ctimer/)
  zephyr_include_directories(.)

  zephyr_compile_definitions_ifdef(CONFIG_CAN_MCUX_FLEXCAN
    FLEXCAN_WAIT_TIMEOUT=${CONFIG_CAN_MCUX_FLEXCAN_WAIT_TIMEOUT})

  if(CONFIG_CPU_HAS_DCACHE)
    zephyr_compile_definitions_ifdef(CONFIG_ENTROPY_MCUX_CAAM CACHE_MODE_WRITE_THROUGH)
  endif()

  if(CONFIG_NOCACHE_MEMORY)
    zephyr_compile_definitions_ifdef(CONFIG_USB_DEVICE_DRIVER DATA_SECTION_IS_CACHEABLE=1)
  endif()
endif()
