obj-$(CONFIG_CLOCK_CONTROL_NRF5) += nrf5_power_clock.o
obj-$(CONFIG_CLOCK_CONTROL_QUARK_SE) += quark_se_clock_control.o
ifeq ($(CONFIG_CLOCK_CONTROL_STM32_CUBE),y)
obj-y += stm32_ll_clock.o
obj-$(CONFIG_SOC_SERIES_STM32L4X) += stm32l4x_ll_clock.o
obj-$(CONFIG_SOC_SERIES_STM32F1X) += stm32f1x_ll_clock.o
obj-$(CONFIG_SOC_SERIES_STM32F3X) += stm32f3x_ll_clock.o
obj-$(CONFIG_SOC_SERIES_STM32F4X) += stm32f4x_ll_clock.o
endif
obj-$(CONFIG_CLOCK_CONTROL_BEETLE) += beetle_clock_control.o
