# Copyright (c) 2026 Igalia S.L.
# SPDX-License-Identifier: Apache-2.0

set(cyw43_driver ${ZEPHYR_HAL_CYW43_MODULE_DIR})

zephyr_library_sources_ifdef(CONFIG_WIFI_RPI_PICO_CYW43
  rpi_pico_cyw43_wifi.c
  rpi_pico_cyw43_spi.c
  rpi_pico_cyw43_gpio.c
  ${cyw43_driver}/src/cyw43_ll.c
  ${cyw43_driver}/src/cyw43_ctrl.c
)

zephyr_library_include_directories_ifdef(CONFIG_WIFI_RPI_PICO_CYW43
  ./
)
