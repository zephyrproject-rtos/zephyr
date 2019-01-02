#
# Copyright (c) 2018, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(.)

zephyr_library_compile_definitions_ifdef(
  CONFIG_PTP_CLOCK_MCUX ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
)

zephyr_library_compile_definitions_ifdef(
  CONFIG_HAS_MCUX_CACHE FSL_SDK_ENABLE_DRIVER_CACHE_CONTROL
)

zephyr_sources_ifdef(CONFIG_HAS_MCUX_CACHE    fsl_cache.c)
zephyr_sources_ifdef(CONFIG_DISPLAY_MCUX_ELCDIF	fsl_elcdif.c)
zephyr_sources_ifdef(CONFIG_GPIO_MCUX_IGPIO   fsl_gpio.c)
zephyr_sources_ifdef(CONFIG_I2C_MCUX_LPI2C    fsl_lpi2c.c)
zephyr_sources_ifdef(CONFIG_SPI_MCUX_LPSPI    fsl_lpspi.c)
zephyr_sources_ifdef(CONFIG_UART_MCUX_LPUART  fsl_lpuart.c)
zephyr_sources_ifdef(CONFIG_ETH_MCUX          fsl_enet.c)
zephyr_sources_ifdef(CONFIG_ENTROPY_MCUX_TRNG fsl_trng.c)
