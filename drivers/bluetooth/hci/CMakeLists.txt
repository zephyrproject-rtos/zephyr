# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_BT_B91         hci_b91.c)
zephyr_library_sources_ifdef(CONFIG_BT_CYW43XXX    cyw43xxx.c)
zephyr_library_sources_ifdef(CONFIG_BT_ESP32       hci_esp32.c)
zephyr_library_sources_ifdef(CONFIG_BT_H4          h4.c)
zephyr_library_sources_ifdef(CONFIG_BT_H5          h5.c)
zephyr_library_sources_ifdef(CONFIG_BT_RPMSG       rpmsg.c)
zephyr_library_sources_ifdef(CONFIG_BT_SPI         spi.c)
zephyr_library_sources_ifdef(CONFIG_BT_STM32_IPM   ipm_stm32wb.c)
zephyr_library_sources_ifdef(CONFIG_BT_USERCHAN    userchan.c)
