zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_PWM_PCA9685}   pwm_pca9685.c
            IFDEF:${CONFIG_PWM_DW}        pwm_dw.c
            IFDEF:${CONFIG_PWM_QMSI}      pwm_qmsi.c
            IFDEF:${CONFIG_PWM_STM32}     pwm_stm32.c
            IFDEF:${CONFIG_PWM_NRF5_SW}   pwm_nrf5_sw.c
            IFDEF:${CONFIG_PWM_NRFX}      pwm_nrfx.c
            IFDEF:${CONFIG_PWM_MCUX_FTM}  pwm_mcux_ftm.c
            IFDEF:${CONFIG_PWM_IMX}       pwm_imx.c
            IFDEF:${CONFIG_PWM_LED_ESP32} pwm_led_esp32.c
            IFDEF:${CONFIG_USERSPACE}     pwm_handlers.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
