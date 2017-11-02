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

static s8_t spi_rw(u8_t *mosi, u8_t *miso, u16_t size)
{
	int ret;

	spi_slave_select(winc1500.spi_dev, winc1500.spi_slave);

	ret = spi_transceive(winc1500.spi_dev,
			     mosi, size,
			     miso, miso ? size : 0);
	if (ret) {
		SYS_LOG_ERR("spi_transceive fail %d", ret);
		return M2M_ERR_BUS_FAIL;
	}

	return M2M_SUCCESS;
}

#endif

s8_t nm_bus_init(void *pvinit)
{
	s8_t result = 0;

#ifdef CONF_WINC_USE_I2C
	/* Not implemented */
#elif defined CONF_WINC_USE_SPI
	struct spi_config spi_config;
	int ret;

	/* configure GPIOs */
	winc1500.gpios = winc1500_configure_gpios();

	/* setup SPI device */
	winc1500.spi_dev = device_get_binding(
		CONFIG_WIFI_WINC1500_SPI_DRV_NAME);
	if (!winc1500.spi_dev) {
		SYS_LOG_ERR("spi device binding");
		return -1;
	}

	spi_config.config = SPI_WORD(8) | SPI_TRANSFER_MSB;
	spi_config.max_sys_freq = CONFIG_WIFI_WINC1500_SPI_FREQ;

	ret = spi_configure(winc1500.spi_dev, &spi_config);
	if (ret) {
		SYS_LOG_ERR("spi_configure fail %d", ret);
		return -1;
	}

	winc1500.spi_slave = CONFIG_WIFI_WINC1500_SPI_SLAVE;
	SYS_LOG_INF("spi_configure OK");

	nm_bsp_reset();
	nm_bsp_sleep(1);

	nm_bsp_interrupt_ctrl(1);

	SYS_LOG_INF("NOTICE:DONE");
#endif
	return result;
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
