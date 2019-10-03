/* SPDX-License-Identifier: Apache-2.0 */

/* SoC level DTS fixup file */

#define DT_INST_0_MICROCHIP_ENC28J60_LOCAL_MAC_ADDRESS  	{0x12,0x45,0xcd,0x92,0x89,0xae}
#define DT_INST_0_MICROCHIP_ENC28J60_INT_GPIOS_CONTROLLER      	"GPIO_0"
#define DT_INST_0_MICROCHIP_ENC28J60_INT_GPIOS_PIN             	30
#define DT_INST_0_MICROCHIP_ENC28J60_BUS_NAME                	"SPI_2"
#define DT_INST_0_MICROCHIP_ENC28J60_SPI_MAX_FREQUENCY         	12800000
#define DT_INST_0_MICROCHIP_ENC28J60_BASE_ADDRESS              	1
#define DT_INST_0_MICROCHIP_ENC28J60_LABEL                     	"ETH_0"
#define DT_INST_0_MICROCHIP_ENC28J60_CS_GPIOS_CONTROLLER       	"GPIO_0"
#define DT_INST_0_MICROCHIP_ENC28J60_CS_GPIOS_PIN              	29


#define DT_NUM_IRQ_PRIO_BITS	DT_ARM_V7M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS

#define DT_ADC_0_NAME			DT_NORDIC_NRF_SAADC_ADC_0_LABEL

#ifdef DT_NORDIC_NRF_UART_UART_0_LABEL
#define DT_UART_0_NAME			DT_NORDIC_NRF_UART_UART_0_LABEL
#else
#define DT_UART_0_NAME			DT_NORDIC_NRF_UARTE_UART_0_LABEL
#endif

#define DT_UART_1_NAME			DT_NORDIC_NRF_UARTE_UART_1_LABEL

#define DT_FLASH_DEV_NAME		DT_INST_0_NORDIC_NRF52_FLASH_CONTROLLER_LABEL

#define DT_GPIO_P0_DEV_NAME		DT_NORDIC_NRF_GPIO_GPIO_0_LABEL
#define DT_GPIO_P1_DEV_NAME		DT_NORDIC_NRF_GPIO_GPIO_1_LABEL

#define DT_I2C_0_NAME			DT_NORDIC_NRF_I2C_I2C_0_LABEL
#define DT_I2C_1_NAME			DT_NORDIC_NRF_I2C_I2C_1_LABEL

#define DT_SPI_0_NAME			DT_NORDIC_NRF_SPI_SPI_0_LABEL
#define DT_SPI_1_NAME			DT_NORDIC_NRF_SPI_SPI_1_LABEL
#define DT_SPI_2_NAME			DT_NORDIC_NRF_SPI_SPI_2_LABEL
#define DT_SPI_3_NAME			DT_NORDIC_NRF_SPI_SPI_3_LABEL

#define DT_WDT_0_NAME			DT_NORDIC_NRF_WATCHDOG_WDT_0_LABEL

#define DT_RTC_0_NAME			DT_NORDIC_NRF_RTC_RTC_0_LABEL
#define DT_RTC_1_NAME			DT_NORDIC_NRF_RTC_RTC_1_LABEL
#define DT_RTC_2_NAME			DT_NORDIC_NRF_RTC_RTC_2_LABEL

/* End of SoC Level DTS fixup file */
