/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>

#include <device.h>
#include <spi.h>

#include <misc/printk.h>

#if (!defined(CONFIG_SPI_QMSI_CS_GPIO) || !defined(CONFIG_SPI_QMSI_CS_GPIO))
#error "This sample requires the GPIO pin as Chip Select feature"
#endif

#if defined(CONFIG_SPI_QMSI_PORT_0_DRV_NAME)
#define SPI_PORT_0 CONFIG_SPI_QMSI_PORT_0_DRV_NAME
#elif defined(CONFIG_SPI_DW_PORT_0_DRV_NAME)
#define SPI_PORT_0 CONFIG_SPI_DW_PORT_0_DRV_NAME
#else
#error "Unknown SPI driver implementation"
#endif

#define W25Q80BL_MANUFACTURER_ID 0x90

static uint8_t rx_buffer[6], tx_buffer[6];

int w25q80bl_read_id(struct device *dev, uint8_t *manufacturer, uint8_t *deviceid)
{
	int err;

	tx_buffer[0] = W25Q80BL_MANUFACTURER_ID;

	err = spi_transceive(dev, tx_buffer, sizeof(tx_buffer),
			     rx_buffer, sizeof(rx_buffer));
	if (err) {
		printk("Error during SPI transfer\n");
		return DEV_FAIL;
	}

	if (manufacturer)
		*manufacturer = rx_buffer[4];

	if (deviceid)
		*deviceid = rx_buffer[5];

	return 0;
}

int main(void)
{
	struct spi_config config = { 0 };
	struct device *spi_mst_0 = device_get_binding(SPI_PORT_0);
	uint8_t manufacturer, device_id;
	int err;

	printk("SPI Example application\n");

	if (!spi_mst_0)
		return DEV_FAIL;

	config.config = SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD(8);
	config.max_sys_freq = 256;

	err = spi_configure(spi_mst_0, &config);
	if (err) {
		printk("Could not configure SPI device\n");
		return DEV_FAIL;
	}

	err = spi_slave_select(spi_mst_0, 1);
	if (err) {
		printk("Could not select SPI slave\n");
		return DEV_FAIL;
	}

	err = w25q80bl_read_id(spi_mst_0, &manufacturer, &device_id);
	if (err) {
		printk("Could not get Manufacturer and Device ID from SPI Flash\n");
		return DEV_FAIL;
	}

	printk("SPI Flash Manufacturer %x Device Id %x\n", manufacturer, device_id);

	return DEV_OK;
}
