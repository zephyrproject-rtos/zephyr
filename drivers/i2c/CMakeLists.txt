zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_I2C_ATMEL_SAM3} i2c_atmel_sam3.c
            IFDEF:${CONFIG_I2C_BITBANG}    i2c_bitbang.c
            IFDEF:${CONFIG_I2C_CC32XX}     i2c_cc32xx.c
            IFDEF:${CONFIG_I2C_DW}         i2c_dw.c
            IFDEF:${CONFIG_I2C_ESP32}      i2c_esp32.c
            IFDEF:${CONFIG_I2C_GPIO}       i2c_gpio.c
            IFDEF:${CONFIG_I2C_IMX}        i2c_imx.c
            IFDEF:${CONFIG_I2C_MCUX}       i2c_mcux.c
            IFDEF:${CONFIG_NRFX_TWI}       i2c_nrfx_twi.c
            IFDEF:${CONFIG_NRFX_TWIM}      i2c_nrfx_twim.c
            IFDEF:${CONFIG_I2C_QMSI}       i2c_qmsi.c
            IFDEF:${CONFIG_I2C_QMSI_SS}    i2c_qmsi_ss.c
            IFDEF:${CONFIG_I2C_SAM_TWI}    i2c_sam_twi.c
            IFDEF:${CONFIG_I2C_SAM_TWIHS}  i2c_sam_twihs.c
            IFDEF:${CONFIG_I2C_SBCON}      i2c_sbcon.c
            IFDEF:${CONFIG_I2C_NIOS2}      i2c_nios2.c
            IFDEF:${CONFIG_I2C_GECKO}	   i2c_gecko.c
            IFDEF:${CONFIG_I2C_STM32_V1}   i2c_ll_stm32_v1.c
                                           i2c_ll_stm32.c
            IFDEF:${CONFIG_I2C_STM32_V2}   i2c_ll_stm32_v2.c
                                           i2c_ll_stm32.c
            IFDEF:${CONFIG_USERSPACE}      i2c_handlers.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

include_relative_ifdef(CONFIG_I2C_SLAVE slave/CMakeLists.txt)
