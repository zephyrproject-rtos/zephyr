zephyr_sources_ifdef(CONFIG_I2C_ATMEL_SAM3	i2c_atmel_sam3.c)
zephyr_sources_ifdef(CONFIG_I2C_BITBANG		i2c_bitbang.c)
zephyr_sources_ifdef(CONFIG_I2C_DW	        i2c_dw.c)
zephyr_sources_ifdef(CONFIG_I2C_ESP32       i2c_esp32.c)
zephyr_sources_ifdef(CONFIG_I2C_GPIO		i2c_gpio.c)
zephyr_sources_ifdef(CONFIG_I2C_MCUX		i2c_mcux.c)
zephyr_sources_ifdef(CONFIG_I2C_NRF5		i2c_nrf5.c)
zephyr_sources_ifdef(CONFIG_I2C_QMSI		i2c_qmsi.c)
zephyr_sources_ifdef(CONFIG_I2C_QMSI_SS		i2c_qmsi_ss.c)
zephyr_sources_ifdef(CONFIG_I2C_SAM_TWI     i2c_sam_twi.c)
zephyr_sources_ifdef(CONFIG_I2C_SAM_TWIHS	i2c_sam_twihs.c)
zephyr_sources_ifdef(CONFIG_I2C_SBCON		i2c_sbcon.c)

zephyr_sources_ifdef(CONFIG_I2C_STM32_V1
	i2c_ll_stm32_v1.c
	i2c_ll_stm32.c
	)
zephyr_sources_ifdef(CONFIG_I2C_STM32_V2
	i2c_ll_stm32_v2.c
	i2c_ll_stm32.c
	)

zephyr_sources_ifdef(CONFIG_USERSPACE   i2c_handlers.c)
