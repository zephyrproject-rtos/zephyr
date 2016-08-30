subdir-ccflags-$(CONFIG_QMSI_BUILTIN) +=-DISR_HANDLED

obj-$(CONFIG_QMSI_BUILTIN) += drivers/clk.o drivers/qm_flash.o
ifeq ($(CONFIG_ARC),y)
obj-$(CONFIG_QMSI_BUILTIN) += drivers/sensor/ss_clk.o
obj-$(CONFIG_QMSI_BUILTIN) += drivers/sensor/ss_power_states.o
endif
obj-$(CONFIG_QMSI_BUILTIN) += soc/$(patsubst %_ss,%,$(SOC_SERIES))/drivers/power_states.o
ifeq ($(CONFIG_SOC_QUARK_SE_C1000),y)
obj-$(CONFIG_QMSI_BUILTIN) += soc/$(SOC_SERIES)/drivers/vreg.o
endif
ifeq ($(CONFIG_SOC_QUARK_D2000),y)
obj-$(CONFIG_QMSI_BUILTIN) += soc/$(SOC_SERIES)/drivers/rar.o
endif
obj-$(CONFIG_RTC_QMSI) += drivers/qm_rtc.o
obj-$(CONFIG_WDT_QMSI) += drivers/qm_wdt.o
obj-$(CONFIG_I2C_QMSI) += drivers/qm_i2c.o drivers/qm_dma.o
obj-$(CONFIG_PWM_QMSI) += drivers/qm_pwm.o
obj-$(CONFIG_AIO_COMPARATOR_QMSI) += drivers/qm_comparator.o
obj-$(CONFIG_AON_COUNTER_QMSI) += drivers/qm_aon_counters.o
obj-$(CONFIG_GPIO_QMSI) += drivers/qm_gpio.o
obj-$(CONFIG_ADC_QMSI) += drivers/qm_adc.o
obj-$(CONFIG_UART_QMSI) += drivers/qm_uart.o
obj-$(CONFIG_DMA_QMSI) += drivers/qm_dma.o
obj-$(CONFIG_SPI_QMSI) += drivers/qm_spi.o
obj-$(CONFIG_SOC_FLASH_QMSI) += drivers/qm_flash.o
obj-$(CONFIG_PINMUX_DEV_QMSI) += drivers/qm_pinmux.o
obj-$(CONFIG_SPI_QMSI_SS) += drivers/sensor/qm_ss_spi.o
obj-$(CONFIG_GPIO_QMSI_SS) += drivers/sensor/qm_ss_gpio.o
obj-$(CONFIG_I2C_QMSI_SS) += drivers/sensor/qm_ss_i2c.o
obj-$(CONFIG_ADC_QMSI_SS) += drivers/sensor/qm_ss_adc.o
