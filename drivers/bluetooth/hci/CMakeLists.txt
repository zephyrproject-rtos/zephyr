# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_BT_H4       h4.c)
zephyr_sources_ifdef(CONFIG_BT_H5       h5.c)
zephyr_sources_ifdef(CONFIG_BT_SPI      spi.c)
zephyr_sources_ifdef(CONFIG_BT_RPMSG	rpmsg.c)
zephyr_sources_ifdef(CONFIG_BT_RPMSG_NRF53	rpmsg_nrf53.c)
zephyr_sources_ifdef(CONFIG_BT_STM32_IPM	ipm_stm32wb.c)
zephyr_sources_ifdef(CONFIG_BT_USERCHAN userchan.c)
zephyr_sources_ifdef(CONFIG_BT_ESP32    hci_esp32.c)
