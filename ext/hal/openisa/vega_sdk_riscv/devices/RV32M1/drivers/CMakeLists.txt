zephyr_include_directories(.)

zephyr_sources(fsl_clock.c)
zephyr_sources_ifdef(CONFIG_UART_RV32M1_LPUART       fsl_lpuart.c)
zephyr_sources_ifdef(CONFIG_I2C_RV32M1_LPI2C         fsl_lpi2c.c)
