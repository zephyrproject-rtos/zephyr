obj-$(CONFIG_SPI_FLASH_W25QXXDV) += spi_flash_w25qxxdv.o
obj-$(CONFIG_SOC_FLASH_QMSI) += soc_flash_qmsi.o
obj-$(CONFIG_SOC_FLASH_NRF5) += soc_flash_nrf5.o
obj-$(CONFIG_SOC_FLASH_MCUX) += soc_flash_mcux.o

ifeq ($(CONFIG_SOC_SERIES_STM32F3X),y)
obj-$(CONFIG_SOC_FLASH_STM32) += flash_stm32f3x.o
obj-$(CONFIG_SOC_FLASH_STM32) += flash_stm32f3x_priv.o
endif
