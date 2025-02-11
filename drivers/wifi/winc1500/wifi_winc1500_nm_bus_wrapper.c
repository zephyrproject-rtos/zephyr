/**
 * Copyright (c) 2017 IpTronix
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_winc1500

#define LOG_LEVEL CONFIG_WIFI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(winc1500);

#include <stdio.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>

#include "wifi_winc1500_nm_bsp_internal.h"

#include <bsp/include/nm_bsp.h>
#include <common/include/nm_common.h>
#include <bus_wrapper/include/nm_bus_wrapper.h>

#include "wifi_winc1500_config.h"

#define NM_BUS_MAX_TRX_SZ	256

tstrNmBusCapabilities egstrNmBusCapabilities = {
	NM_BUS_MAX_TRX_SZ
};

#ifdef CONF_WINC_USE_I2C

#define SLAVE_ADDRESS		0x60

/** Number of times to try to send packet if failed. */
#define I2C_TIMEOUT		100

static int8_t nm_i2c_write(uint8_t *b, uint16_t sz)
{
	/* Not implemented */
}

static int8_t nm_i2c_read(uint8_t *rb, uint16_t sz)
{
	/* Not implemented */
}

static int8_t nm_i2c_write_special(uint8_t *wb1, uint16_t sz1,
				 uint8_t *wb2, uint16_t sz2)
{
	/* Not implemented */
}
#endif

#ifdef CONF_WINC_USE_SPI

static int8_t spi_rw(uint8_t *mosi, uint8_t *miso, uint16_t size)
{
	const struct spi_buf buf_tx = {
		.buf = mosi,
		.len = size
	};
	const struct spi_buf_set tx = {
		.buffers = &buf_tx,
		.count = 1
	};
	const struct spi_buf buf_rx = {
		.buf = miso,
		.len = miso ? size : 0
	};
	const struct spi_buf_set rx = {
		.buffers = &buf_rx,
		.count = 1
	};

	if (spi_transceive_dt(&winc1500_config.spi, &tx, &rx)) {
		LOG_ERR("spi_transceive fail");
		return M2M_ERR_BUS_FAIL;
	}

	return M2M_SUCCESS;
}

#endif

int8_t nm_bus_init(void *pvinit)
{
	/* configure GPIOs */
	if (!gpio_is_ready_dt(&winc1500_config.chip_en_gpio)) {
		return -ENODEV;
	}
	gpio_pin_configure_dt(&winc1500_config.chip_en_gpio, GPIO_OUTPUT_LOW);


	if (!gpio_is_ready_dt(&winc1500_config.irq_gpio)) {
		return -ENODEV;
	}
	gpio_pin_configure_dt(&winc1500_config.irq_gpio, GPIO_INPUT);

	if (!gpio_is_ready_dt(&winc1500_config.reset_gpio)) {
		return -ENODEV;
	}
	gpio_pin_configure_dt(&winc1500_config.reset_gpio, GPIO_OUTPUT_LOW);


#ifdef CONF_WINC_USE_I2C
	/* Not implemented */
#elif defined CONF_WINC_USE_SPI
	/* setup SPI device */
	if (!spi_is_ready_dt(&winc1500_config.spi)) {
		LOG_ERR("spi device binding");
		return -ENODEV;
	}

	nm_bsp_reset();
	nm_bsp_sleep(1);

	nm_bsp_interrupt_ctrl(1);

	LOG_DBG("NOTICE:DONE");
#endif
	return 0;
}

int8_t nm_bus_ioctl(uint8_t cmd, void *parameter)
{
	sint8 ret = 0;

	switch (cmd) {
#ifdef CONF_WINC_USE_I2C
	case NM_BUS_IOCTL_R: {
		struct nm_i2c_default *param =
			(struct nm_i2c_default *)parameter;

		ret = nm_i2c_read(param->buffer, param->size);
	}
	break;
	case NM_BUS_IOCTL_W: {
		struct nm_i2c_default *param =
			(struct nm_i2c_default *)parameter;

		ret = nm_i2c_write(param->buffer, param->size);
	}
	break;
	case NM_BUS_IOCTL_W_SPECIAL: {
		struct nm_i2c_special *param =
			(struct nm_i2c_special *)parameter;

		ret = nm_i2c_write_special(param->buffer1, param->size1,
					   param->buffer2, param->size2);
	}
	break;
#elif defined CONF_WINC_USE_SPI
	case NM_BUS_IOCTL_RW: {
		tstrNmSpiRw *param = (tstrNmSpiRw *)parameter;

		ret = spi_rw(param->pu8InBuf, param->pu8OutBuf, param->u16Sz);
	}
	break;
#endif
	default:
		ret = -1;
		M2M_ERR("ERROR:invalid ioclt cmd\n");
		break;
	}

	return ret;
}

int8_t nm_bus_deinit(void)
{
	return M2M_SUCCESS;
}

int8_t nm_bus_reinit(void *config)
{
	return 0;
}
