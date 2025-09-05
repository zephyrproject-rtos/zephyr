# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(
  soc.c
  )

zephyr_sources_ifdef(CONFIG_PM
  power.c
  )

zephyr_sources_ifdef(CONFIG_POWEROFF poweroff.c)

if(CONFIG_BT_STM32WBA)
  zephyr_include_directories(hci_if/ble)
  zephyr_sources(hci_if/ble/linklayer_plat_adapt.c)
  zephyr_sources(hci_if/ble/bleplat.c)
  zephyr_sources(hci_if/ble/host_stack_if.c)
  zephyr_sources(hci_if/ble/ll_sys_if_adapt.c)
  zephyr_sources(hci_if/Utilities/tim_serv/stm32_timer.c)
endif()

if(CONFIG_IEEE802154_STM32WBA)
  zephyr_include_directories(hci_if/IEEE802154)
  zephyr_sources(hci_if/IEEE802154/802154_plat.c)
  zephyr_sources(hci_if/IEEE802154/host_stack_if.c)
  zephyr_sources(hci_if/IEEE802154/linklayer_plat_adapt.c)
  zephyr_sources(hci_if/IEEE802154/ll_sys_if_adapt.c)
  zephyr_sources(hci_if/Utilities/tim_serv/stm32_timer.c)
endif()

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
