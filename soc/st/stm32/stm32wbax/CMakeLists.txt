# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(
  soc.c
  )

zephyr_sources_ifdef(CONFIG_PM
  power.c
  )

if(CONFIG_BT_STM32WBA)
  zephyr_include_directories(hci_if)

  zephyr_sources(hci_if/linklayer_plat_adapt.c)
  zephyr_sources(hci_if/bleplat.c)
  zephyr_sources(hci_if/host_stack_if.c)
  zephyr_sources(hci_if/ll_sys_if_adapt.c)
  zephyr_sources(hci_if/stm32_timer.c)
endif()

if(CONFIG_BT_STM32WBA_RT_DEBUG_GPIO_MODULE)
  zephyr_include_directories(debug)
  zephyr_sources(debug/RTDebug.c)
  zephyr_sources(debug/app_debug.c)
  zephyr_compile_definitions( -DCFG_RT_DEBUG_GPIO_MODULE=1 )
else()
  zephyr_compile_definitions( -DCFG_RT_DEBUG_GPIO_MODULE=0 )
endif()

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
