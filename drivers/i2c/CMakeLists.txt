# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_I2C_BITBANG		i2c_bitbang.c)
zephyr_library_sources_ifdef(CONFIG_I2C_CC13XX_CC26XX		i2c_cc13xx_cc26xx.c)
zephyr_library_sources_ifdef(CONFIG_I2C_CC32XX		i2c_cc32xx.c)
zephyr_library_sources_ifdef(CONFIG_I2C_ESP32		i2c_esp32.c)
zephyr_library_sources_ifdef(CONFIG_I2C_GPIO		i2c_gpio.c)
zephyr_library_sources_ifdef(CONFIG_I2C_IMX		i2c_imx.c)
zephyr_library_sources_ifdef(CONFIG_I2C_XEC		i2c_mchp_xec.c)
zephyr_library_sources_ifdef(CONFIG_I2C_MCUX		i2c_mcux.c)
zephyr_library_sources_ifdef(CONFIG_I2C_MCUX_LPI2C	i2c_mcux_lpi2c.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_TWI		i2c_nrfx_twi.c)
zephyr_library_sources_ifdef(CONFIG_NRFX_TWIM		i2c_nrfx_twim.c)
zephyr_library_sources_ifdef(CONFIG_I2C_SAM_TWI		i2c_sam_twi.c)
zephyr_library_sources_ifdef(CONFIG_I2C_SAM_TWIHS	i2c_sam_twihs.c)
zephyr_library_sources_ifdef(CONFIG_I2C_SBCON		i2c_sbcon.c)
zephyr_library_sources_ifdef(CONFIG_I2C_SIFIVE		i2c_sifive.c)
zephyr_library_sources_ifdef(CONFIG_I2C_NIOS2		i2c_nios2.c)
zephyr_library_sources_ifdef(CONFIG_I2C_GECKO		i2c_gecko.c)
zephyr_library_sources_ifdef(CONFIG_I2C_RV32M1_LPI2C	i2c_rv32m1_lpi2c.c)
zephyr_library_sources_ifdef(CONFIG_I2C_SAM0		i2c_sam0.c)

zephyr_library_sources_ifdef(CONFIG_I2C_STM32_V1
	i2c_ll_stm32_v1.c
	i2c_ll_stm32.c
	)
zephyr_library_sources_ifdef(CONFIG_I2C_STM32_V2
	i2c_ll_stm32_v2.c
	i2c_ll_stm32.c
	)

if(CONFIG_I2C_DW)
  zephyr_library_sources(i2c_dw.c)
  foreach(NUM RANGE 0 7)
    if(CONFIG_I2C_${NUM})
      configure_file(
        i2c_dw_port_x.h
        ${PROJECT_BINARY_DIR}/include/generated/i2c_dw_port_${NUM}.h
        @ONLY
        )
    endif()
  endforeach(NUM)
endif()

zephyr_library_sources_ifdef(CONFIG_USERSPACE		i2c_handlers.c)

add_subdirectory_ifdef(CONFIG_I2C_SLAVE			slave)
