/**
 * Copyright (c) 2017 IpTronix
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_WIFI_LEVEL
#define SYS_LOG_DOMAIN "dev/winc1500"
#include <logging/sys_log.h>

#include <stdio.h>
#include <stdint.h>

#include <board.h>
#include <device.h>
#include <spi.h>

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

static s8_t nm_i2c_write(u8_t *b, u16_t sz)
{
	/* Not implemented */
}

static s8_t nm_i2c_read(u8_t *rb, u16_t sz)
{
	/* Not implemented */
}

static s8_t nm_i2c_write_special(u8_t *wb1, u16_t sz1,
				 u8_t *wb2, u16_t sz2)
{
	/* Not implemented */
}
#endif

#ifdef CONF_WINC_USE_SPI

#ifdef CONFIG_WIFI_WINC1500_GPIO_SPI_CS
struct spi_cs_control cs_ctrl;
#endif

static s8_t spi_rw(u8_t *mosi, u8_t *miso, u16_t size)
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

	if (spi_transceive(winc1500.spi, &winc1500.spi_cfg, &tx, &rx)) {
		SYS_LOG_ERR("spi_transceive fail");
		return M2M_ERR_BUS_FAIL;
	}

	return M2M_SUCCESS;
}

#endif

s8_t nm_bus_init(void *pvinit)
{
	/* configure GPIOs */
	winc1500.gpios = winc1500_configure_gpios();

#ifdef CONF_WINC_USE_I2C
	/* Not implemented */
#elif defined CONF_WINC_USE_SPI
	/* setup SPI device */
	winc1500.spi = device_get_binding(CONFIG_WIFI_WINC1500_SPI_DRV_NAME);
	if (!winc1500.spi) {
		SYS_LOG_ERR("spi device binding");
		return -1;
	}

	winc1500.spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB;
	winc1500.spi_cfg.frequency = CONFIG_WIFI_WINC1500_SPI_FREQ;
	winc1500.spi_cfg.slave = CONFIG_WIFI_WINC1500_SPI_SLAVE;

#ifdef CONFIG_WIFI_WINC1500_GPIO_SPI_CS
	cs_ctrl.gpio_dev = device_get_binding(
		CONFIG_WIFI_WINC1500_GPIO_SPI_CS_DRV_NAME);
	if (!cs_ctrl.gpio_dev) {
		SYS_LOG_ERR("Unable to get GPIO SPI CS device");
		return -ENODEV;
	}

	cs_ctrl.gpio_pin = CONFIG_WIFI_WINC1500_GPIO_SPI_CS_PIN;
	cs_ctrl.delay = 0;

	winc1500.spi_cfg.cs = &cs_ctrl;

	SYS_LOG_DBG("SPI GPIO CS configured on %s:%u",
		    CONFIG_WIFI_WINC1500_GPIO_SPI_CS_DRV_NAME,
		    CONFIG_WIFI_WINC1500_GPIO_SPI_CS_PIN);
#endif /* CONFIG_WIFI_WINC1500_GPIO_SPI_CS */

	nm_bsp_reset();
	nm_bsp_sleep(1);

	nm_bsp_interrupt_ctrl(1);

	SYS_LOG_DBG("NOTICE:DONE");
#endif
	return 0;
}

s8_t nm_bus_ioctl(u8_t cmd, void *parameter)
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

s8_t nm_bus_deinit(void)
{
	return M2M_SUCCESS;
}

s8_t nm_bus_reinit(void *config)
{
	return 0;
}
