zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_GPIO_IMX_IGPIO          gpio_imx.c)
zephyr_sources_ifdef(CONFIG_RDC_IMX_RDC             rdc_semaphore.c)
zephyr_sources_ifdef(CONFIG_CLOCK_CONTROL_IMX_CCM   ccm_imx7d.c)
zephyr_sources_ifdef(CONFIG_CLOCK_CONTROL_IMX_CCM   ccm_analog_imx7d.c)
zephyr_sources_ifdef(CONFIG_UART_IMX_UART           uart_imx.c)










