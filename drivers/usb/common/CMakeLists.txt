# SPDX-License-Identifier: Apache-2.0

add_subdirectory(buf)
add_subdirectory_ifdef(CONFIG_HAS_NRFX nrf_usbd_common)
add_subdirectory_ifdef(CONFIG_SOC_FAMILY_STM32 stm32)

if(CONFIG_UDC_DRIVER OR CONFIG_UHC_DRIVER)
  zephyr_include_directories(.)
endif()
