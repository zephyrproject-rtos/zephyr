# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_WIFI_NXP)
  set(CONFIG_MCUX_COMPONENT_component.wifi_bt_module.tx_pwr_limits ON)
endif()

if(CONFIG_NXP_FW_LOADER)
  set(CONFIG_MCUX_COMPONENT_driver.conn_fwloader ON)
  set(CONFIG_USE_component_osa_zephyr ON)
endif()

if(${MCUX_DEVICE} MATCHES "RW61")
  set(CONFIG_USE_component_osa_zephyr ON)
  if(CONFIG_NXP_FW_LOADER)
    set(CONFIG_MCUX_COMPONENT_component.mflash_offchip ON)
    set(CONFIG_MCUX_COMPONENT_driver.cache_cache64 ON)
    set(CONFIG_MCUX_COMPONENT_driver.flexspi ON)
  endif()
endif()

if(CONFIG_USB_DEVICE_DRIVER OR CONFIG_UDC_DRIVER OR CONFIG_BT)
  set(CONFIG_USE_component_osa_zephyr ON)
endif()

if(CONFIG_NXP_RF_IMU)
  if(CONFIG_SOC_SERIES_RW6XX)
    set(CONFIG_MCUX_COMPONENT_driver.imu ON)
    set(CONFIG_MCUX_COMPONENT_driver.gdma ON)
    set(CONFIG_MCUX_COMPONENT_component.wireless_imu_adapter ON)
    set(CONFIG_MCUX_PRJSEG_component.osa_interface.osa_macro_used ON)
  elseif(CONFIG_SOC_SERIES_MCXW)
    set(CONFIG_MCUX_COMPONENT_component.lists ON)
    set(CONFIG_MCUX_COMPONENT_component.rpmsg_adapter ON)
    zephyr_compile_definitions(HAL_RPMSG_SELECT_ROLE=0U)
  endif()
endif()

if(CONFIG_SOC_SERIES_MCXW)
  if(CONFIG_NET_L2_IEEE802154 OR CONFIG_NET_L2_OPENTHREAD)
    set(CONFIG_MCUX_COMPONENT_component.lists ON)
    set(CONFIG_USE_component_osa_zephyr ON)
    zephyr_compile_definitions(OSA_USED=1U)
  endif()
endif()

if(CONFIG_USE_component_osa_zephyr)
  set(CONFIG_MCUX_COMPONENT_component.osa_template_config ON)
  set(CONFIG_MCUX_COMPONENT_component.osa_zephyr ON)
  set(CONFIG_MCUX_COMPONENT_component.osa_interface ON)
endif()

add_subdirectory(${MCUX_SDK_NG_DIR}/components/osa
  ${CMAKE_CURRENT_BINARY_DIR}/osa
  )

add_subdirectory(${MCUX_SDK_NG_DIR}/components/wifi_bt_module
  ${CMAKE_CURRENT_BINARY_DIR}/wifi_bt_module
  )

add_subdirectory(${MCUX_SDK_NG_DIR}/components/conn_fwloader
  ${CMAKE_CURRENT_BINARY_DIR}/conn_fwloader
  )

add_subdirectory(${MCUX_SDK_NG_DIR}/components/lists
  ${CMAKE_CURRENT_BINARY_DIR}/lists
  )

add_subdirectory(${MCUX_SDK_NG_DIR}/components/rpmsg
  ${CMAKE_CURRENT_BINARY_DIR}/rpmsg
  )

add_subdirectory(${MCUX_SDK_NG_DIR}/components/imu_adapter
  ${CMAKE_CURRENT_BINARY_DIR}/imu_adapter
  )

if(${MCUX_DEVICE} MATCHES "RW61")
  add_subdirectory(${MCUX_SDK_NG_DIR}/components/flash/mflash/rdrw612bga
    ${CMAKE_CURRENT_BINARY_DIR}/flash/mflash
    )
endif()
