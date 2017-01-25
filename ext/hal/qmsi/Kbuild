obj-$(CONFIG_QMSI_BUILTIN) += drivers/flash/qm_flash.o
ifeq ($(CONFIG_SOC_QUARK_SE_C1000_SS),y)
obj-$(CONFIG_QMSI_BUILTIN) += drivers/clk/ss_clk.o
obj-$(CONFIG_QMSI_BUILTIN) += soc/quark_se/drivers/vreg.o
obj-$(CONFIG_QMSI_BUILTIN) += soc/quark_se/drivers/ss_power_states.o
endif
obj-$(CONFIG_QMSI_BUILTIN) += soc/$(patsubst %_ss,%,$(SOC_SERIES))/drivers/power_states.o
obj-$(CONFIG_QMSI_BUILTIN) += soc/$(patsubst %_ss,%,$(SOC_SERIES))/drivers/clk.o
ifeq ($(CONFIG_SOC_QUARK_SE_C1000),y)
obj-$(CONFIG_QMSI_BUILTIN) += soc/$(SOC_SERIES)/drivers/vreg.o
endif
obj-$(CONFIG_RTC_QMSI) += drivers/rtc/qm_rtc.o
obj-$(CONFIG_WDT_QMSI) += drivers/wdt/qm_wdt.o
obj-$(CONFIG_I2C_QMSI) += drivers/i2c/qm_i2c.o
obj-$(CONFIG_PWM_QMSI) += drivers/pwm/qm_pwm.o
obj-$(CONFIG_AIO_COMPARATOR_QMSI) += drivers/comparator/qm_comparator.o
obj-$(CONFIG_AON_COUNTER_QMSI) += drivers/aon_counters/qm_aon_counters.o
obj-$(CONFIG_GPIO_QMSI) += drivers/gpio/qm_gpio.o
obj-$(CONFIG_ADC_QMSI) += drivers/adc/qm_adc.o
obj-$(CONFIG_UART_QMSI) += drivers/uart/qm_uart.o
obj-$(CONFIG_DMA_QMSI) += drivers/dma/qm_dma.o
obj-$(CONFIG_SPI_QMSI) += drivers/spi/qm_spi.o
obj-$(CONFIG_SOC_FLASH_QMSI) += drivers/flash/qm_flash.o
obj-$(CONFIG_PINMUX_QMSI) += drivers/pinmux/qm_pinmux.o
obj-$(CONFIG_SPI_QMSI_SS) += drivers/spi/qm_ss_spi.o
obj-$(CONFIG_GPIO_QMSI_SS) += drivers/gpio/qm_ss_gpio.o
obj-$(CONFIG_I2C_QMSI_SS) += drivers/i2c/qm_ss_i2c.o
obj-$(CONFIG_ADC_QMSI_SS) += drivers/adc/qm_ss_adc.o
obj-$(CONFIG_SOC_WATCH) += drivers/soc_watch.o
