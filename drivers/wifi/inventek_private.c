/**
 ******************************************************************************
 * @file    es_wifi_io.c
 * @author  MCD Application Team
 * @brief   This file implments the IO operations to deal with the es-wifi
 *          module. It mainly Inits and Deinits the SPI interface. Send and
 *          receive data over it.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
 * All rights reserved.</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions are met:
 *
 * 1. Redistribution of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of other
 *    contributors to this software may be used to endorse or promote products
 *    derived from this software without specific written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
 * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
 * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

#include "es_wifi_io.h"
#include <string.h>

#include <zephyr.h>
#include <board.h>
#include <device.h>
#include <gpio.h>
#include <spi.h>

#define SYS_LOG_DOMAIN "INVENTEK_WIFI"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_WIFI_LEVEL
#include <logging/sys_log.h>

static struct spi_config spi_conf = {
	.frequency = CONFIG_WIFI_ISM43362_M3G_L44_SPI_CLK_FREQ,
	.operation = (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(16) |
		      SPI_LINES_SINGLE),
	.slave     = 0,
	.cs        = NULL,
};

struct device *dev_gpio_e;

void WIFI_RESET_MODULE(void)
{
	gpio_pin_write(dev_gpio_e, 8, 0);
	k_sleep(10);
	gpio_pin_write(dev_gpio_e, 8, 1);
	k_sleep(500);
}

void WIFI_ENABLE_NSS(void)
{
	gpio_pin_write(dev_gpio_e, 0, 0);
	k_sleep(10);
}

void WIFI_DISABLE_NSS(void)
{
	gpio_pin_write(dev_gpio_e, 0, 1);
	k_sleep(10);
}

u32_t WIFI_IS_CMDDATA_READY(void)
{
	u32_t value;

	gpio_pin_read(dev_gpio_e, 1, &value);
	return value;
}

/**
 * @brief  Initialize the SPI3
 * @param  None
 * @retval None
 */
s8_t SPI_WIFI_Init(void)
{
	u32_t tickstart = k_uptime_get_32();
	u16_t Prompt[3];

	spi_conf.dev = device_get_binding(
				CONFIG_WIFI_ISM43362_M3G_L44_SPI_DEV_NAME);
	if (!spi_conf.dev) {
		SYS_LOG_ERR("Failed to initialize SPI driver: %s",
		CONFIG_WIFI_ISM43362_M3G_L44_SPI_DEV_NAME);
		return -EIO;
	}

	dev_gpio_e = device_get_binding("GPIOE");
	gpio_pin_configure(dev_gpio_e, 8, GPIO_DIR_OUT);
	gpio_pin_configure(dev_gpio_e, 0, GPIO_DIR_OUT);
	gpio_pin_configure(dev_gpio_e, 1, GPIO_DIR_IN);

	WIFI_RESET_MODULE();

	WIFI_ENABLE_NSS();

	while (WIFI_IS_CMDDATA_READY()) {
		int status_spi;
		static struct spi_buf spi_rx_buf;

		spi_rx_buf.buf = Prompt;
		spi_rx_buf.len = 3;

		status_spi = spi_read(&spi_conf, &spi_rx_buf, 1);

		if (((k_uptime_get_32() - tickstart) > 0xFFFF) || status_spi) {
			SYS_LOG_ERR("Err reading SPI (status=%d) or timeout\n",
				    status_spi);
			WIFI_DISABLE_NSS();
			return -1;
		}
	}

	if ((Prompt[0] != 0x1515) || (Prompt[1] != 0x0A0D)
	    || (Prompt[2] != 0x203E)) {
		SYS_LOG_ERR("Wrong values returned by wifi chip.\n");
		WIFI_DISABLE_NSS();
		return -1;
	}

	WIFI_DISABLE_NSS();
	return 0;
}

/**
 * @brief  DeInitialize the SPI
 * @param  None
 * @retval None
 */
s8_t SPI_WIFI_DeInit(void)
{
	spi_release(&spi_conf);
	return 0;
}

/**
 * @brief  Receive wifi Data from SPI
 * @param  pdata : pointer to data
 * @param  len : Data length
 * @param  timeout : send timeout in mS
 * @retval Length of received data (payload)
 */
int16_t SPI_WIFI_ReceiveData(uint8_t *pData, uint16_t len, uint32_t timeout)
{
	u32_t tickstart = k_uptime_get_32();
	s16_t length = 0;
	u16_t tmp;
	int status_spi;
	static struct spi_buf spi_rx_buf;

	WIFI_DISABLE_NSS();

	while (!WIFI_IS_CMDDATA_READY()) {
		if ((k_uptime_get_32() - tickstart) > timeout) {
			SYS_LOG_ERR("Timeout, wifi not available.\n");
			return -1;
		}
	}

	WIFI_ENABLE_NSS();

	while (WIFI_IS_CMDDATA_READY()) {
		if ((length < len) || (!len)) {
			spi_rx_buf.buf = &tmp;
			spi_rx_buf.len = 1;
			status_spi = spi_read(&spi_conf, &spi_rx_buf, 1);

			if ((tmp & 0xFF00) == 0x1500) {
				/* last char reached */
				SPI_WIFI_Delay(1);
			}

			if (!WIFI_IS_CMDDATA_READY()) {
				/* end of reception reached */
				if ((tmp & 0XFF00) == 0x1500) {
					if ((length < len) || (!len)) {
						pData[0] = (u8_t)(tmp & 0xFF);
						length++;
					}
					break;
				}
			}

			pData[0] = (u8_t)(tmp & 0xFF);
			pData[1] = (u8_t)((tmp >> 8) & 0xFF);
			length += 2;
			pData  += 2;

			if ((k_uptime_get_32() - tickstart) > timeout) {
				SYS_LOG_ERR("Timeout while receiving data\n");
				WIFI_DISABLE_NSS();
				return -1;
			}
		} else {
			break;
		}
	}

	WIFI_DISABLE_NSS();
	return length;
}

/**
 * @brief  Send wifi Data thru SPI
 * @param  pdata : pointer to data
 * @param  len : Data length
 * @param  timeout : send timeout in mS
 * @retval Length of sent data
 */
int16_t SPI_WIFI_SendData(uint8_t *pdata, uint16_t len, uint32_t timeout)
{
	u32_t tickstart = k_uptime_get_32();
	u8_t Padding[2];
	int status_spi;
	static struct spi_buf spi_tx_buf;

	while (!WIFI_IS_CMDDATA_READY()) {
		if ((k_uptime_get_32() - tickstart) > timeout) {
			SYS_LOG_ERR("Timeout, wifi component not available.\n");
			WIFI_DISABLE_NSS();
			return -1;
		}
	}

	WIFI_ENABLE_NSS();
	if (len > 1) {
		spi_tx_buf.buf = pdata;
		spi_tx_buf.len = len;

		status_spi = spi_write(&spi_conf, &spi_tx_buf, 1);
		if (status_spi) {
			SYS_LOG_ERR("Error sending data on SPI (status=%d)\n",
				    status_spi);
			WIFI_DISABLE_NSS();
			return -1;
		}
	}

	if (len & 1) {
		Padding[0] = pdata[len-1];
		Padding[1] = '\n';

		spi_tx_buf.buf = Padding;
		spi_tx_buf.len = 2;
		status_spi = spi_write(&spi_conf, &spi_tx_buf, 1);

		if (status_spi) {
			SYS_LOG_ERR("Err sending padding on SPI (status=%d)\n",
				    status_spi);
			WIFI_DISABLE_NSS();
			return -1;
		}
	}

	return len;
}

/**
 * @brief  Delay
 * @param  Delay in ms
 * @retval None
 */
void SPI_WIFI_Delay(uint32_t Delay)
{
	k_sleep(Delay);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
