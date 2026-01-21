# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_HAS_NRFX nrf_usbd_common)
add_subdirectory_ifdef(CONFIG_SOC_FAMILY_STM32 stm32)
