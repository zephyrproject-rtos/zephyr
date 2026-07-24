# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BT_CONN AND CONFIG_SILABS_GECKO_POWER_MANAGER)
  message(WARNING "
  TYZS3 has an inaccurate sleep clock. Due to that it has to disable EM2 sleep
  while connected to another Bluetooth device. This causes much higher power
  consumption.
  ")
endif()

zephyr_library()
zephyr_library_sources(board.c)
