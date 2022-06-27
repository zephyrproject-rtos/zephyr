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

static
struct winc1500_gpio_configuration winc1500_gpios[WINC1500_GPIO_IDX_MAX] = {
	{ .dev = NULL, .pin = DT_INST_GPIO_PIN(0, enable_gpios) },
	{ .dev = NULL, .pin = DT_INST_GPIO_PIN(0, irq_gpios) },
	{ .dev = NULL, .pin = DT_INST_GPIO_PIN(0, reset_gpios) },
};

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

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
struct spi_cs_control cs_ctrl;
#endif

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

	if (spi_transceive(winc1500.spi, &winc1500.spi_cfg, &tx, &rx)) {
		LOG_ERR("spi_transceive fail");
		return M2M_ERR_BUS_FAIL;
	}

	return M2M_SUCCESS;
}

#endif

struct winc1500_gpio_configuration *winc1500_configure_gpios(void)
{
	const struct device *gpio_en, *gpio_irq, *gpio_reset;

	gpio_en = device_get_binding(
		DT_INST_GPIO_LABEL(0, enable_gpios));
	gpio_irq = device_get_binding(
		DT_INST_GPIO_LABEL(0, irq_gpios));
	gpio_reset = device_get_binding(
		DT_INST_GPIO_LABEL(0, reset_gpios));

	gpio_pin_configure(gpio_en,
			   winc1500_gpios[WINC1500_GPIO_IDX_CHIP_EN].pin,
			   GPIO_OUTPUT_LOW |
			   DT_INST_GPIO_FLAGS(0, enable_gpios));
	gpio_pin_configure(gpio_irq,
			   winc1500_gpios[WINC1500_GPIO_IDX_IRQN].pin,
			   GPIO_INPUT |
			   DT_INST_GPIO_FLAGS(0, irq_gpios));
	gpio_pin_configure(gpio_reset,
			   winc1500_gpios[WINC1500_GPIO_IDX_RESET_N].pin,
			   GPIO_OUTPUT_LOW |
			   DT_INST_GPIO_FLAGS(0, reset_gpios));

	winc1500_gpios[WINC1500_GPIO_IDX_CHIP_EN].dev = gpio_en;
	winc1500_gpios[WINC1500_GPIO_IDX_IRQN].dev = gpio_irq;
	winc1500_gpios[WINC1500_GPIO_IDX_RESET_N].dev = gpio_reset;

	return winc1500_gpios;
}

int8_t nm_bus_init(void *pvinit)
{
	/* configure GPIOs */
	winc1500.gpios = winc1500_configure_gpios();

#ifdef CONF_WINC_USE_I2C
	/* Not implemented */
#elif defined CONF_WINC_USE_SPI
	/* setup SPI device */
	winc1500.spi = device_get_binding(DT_INST_BUS_LABEL(0));
	if (!winc1500.spi) {
		LOG_ERR("spi device binding");
		return -1;
	}

	winc1500.spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB;
	winc1500.spi_cfg.frequency = DT_INST_PROP(0, spi_max_frequency);
	winc1500.spi_cfg.slave = DT_INST_REG_ADDR(0);

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	cs_ctrl.gpio_dev = device_get_binding(
		DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	if (!cs_ctrl.gpio_dev) {
		LOG_ERR("Unable to get GPIO SPI CS device");
		return -ENODEV;
	}

	cs_ctrl.gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	cs_ctrl.gpio_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
	cs_ctrl.delay = 0U;

	winc1500.spi_cfg.cs = &cs_ctrl;

	LOG_DBG("SPI GPIO CS configured on %s:%u",
		    DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
		    DT_INST_SPI_DEV_CS_GPIOS_PIN(0));
#endif

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
