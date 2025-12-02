# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(
	CONFIG_EC_HOST_CMD_BACKEND_SIMULATOR
	ec_host_cmd_backend_simulator.c)

zephyr_library_sources_ifdef(
	CONFIG_EC_HOST_CMD_BACKEND_ESPI
	ec_host_cmd_backend_espi.c)

zephyr_library_sources_ifdef(
	CONFIG_EC_HOST_CMD_BACKEND_SHI_NPCX
	ec_host_cmd_backend_shi_npcx.c)

zephyr_library_sources_ifdef(
	CONFIG_EC_HOST_CMD_BACKEND_SHI_ITE
	ec_host_cmd_backend_shi_ite.c)

zephyr_library_sources_ifdef(
	CONFIG_EC_HOST_CMD_BACKEND_UART
	ec_host_cmd_backend_uart.c)

zephyr_library_sources_ifdef(
	CONFIG_EC_HOST_CMD_BACKEND_SPI_STM32
	ec_host_cmd_backend_spi_stm32.c)

zephyr_library_sources_ifdef(
	CONFIG_EC_HOST_CMD_BACKEND_USB
	ec_host_cmd_backend_usb.c)
