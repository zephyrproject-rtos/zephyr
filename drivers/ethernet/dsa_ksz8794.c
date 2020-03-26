/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *               Lukasz Majewski <lukma@denx.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME dsa
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <device.h>
#include <kernel.h>
#include <sys/util.h>
#include <drivers/spi.h>
#include "dsa_ksz8794.h"

static struct dsa_ksz8794_spi phy_spi;
static struct spi_cs_control cs_con;

static void dsa_ksz8794_write_reg(struct dsa_ksz8794_spi *sdev,
				  u16_t reg_addr, u8_t value)
{
	u8_t buf[3];

	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 3
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	buf[0] = KSZ8794_SPI_CMD_WR | ((reg_addr >> 7) & 0x1F);
	buf[1] = (reg_addr << 1) & 0xFE;
	buf[2] = value;

	spi_write(sdev->spi, &sdev->spi_cfg, &tx);
}

static void dsa_ksz8794_read_reg(struct dsa_ksz8794_spi *sdev,
				 u16_t reg_addr, u8_t *value)
{
	u8_t buf[3];

	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 3
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf = {
		.buf = buf,
		.len = 3
	};

	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	buf[0] = KSZ8794_SPI_CMD_RD | ((reg_addr >> 7) & 0x1F);
	buf[1] = (reg_addr << 1) & 0xFE;
	buf[2] = 0x0;

	if (!spi_transceive(sdev->spi, &sdev->spi_cfg, &tx, &rx)) {
		*value = buf[2];
	} else {
		LOG_DBG("Failure while reading register 0x%04x", reg_addr);
		*value = 0U;
	}

}

static bool dsa_ksz8794_port_link_status(struct dsa_ksz8794_spi *sdev,
					 u8_t port)
{
	u8_t tmp;

	if(port < KSZ8794_PORT1 || port > KSZ8794_PORT3)
		return false;

	dsa_ksz8794_read_reg(sdev, KSZ8794_STAT2_PORTn(port), &tmp);

	return tmp & KSZ8794_STAT2_LINK_GOOD;
}

static bool dsa_ksz8794_link_status(struct dsa_ksz8794_spi *sdev)
{
	bool ret = false;
	u8_t i;

	for (i = KSZ8794_PORT1; i <= KSZ8794_PORT3; i++) {
		if (dsa_ksz8794_port_link_status(sdev, i)) {
			LOG_INF("Port: %d link UP!", i);
			ret |= true;
		}
	}

	return ret;
}

static void dsa_ksz8794_soft_reset(struct dsa_ksz8794_spi *sdev)
{
	/* reset switch */
	dsa_ksz8794_write_reg(sdev, KSZ8794_PD_MGMT_CTRL1,
			      KSZ8794_PWR_MGNT_MODE_SOFT_DOWN);
	k_busy_wait(1000);
	dsa_ksz8794_write_reg(sdev, KSZ8794_PD_MGMT_CTRL1, 0);
}

static int dsa_ksz8794_spi_setup(struct dsa_ksz8794_spi *sdev, u32_t freq, u8_t cs_num,
				 char *cs_gpio_name, u8_t cs_gpio_num)
{
	u16_t timeout = 100;
        u8_t val[2], tmp;

	/* SPI config */
	sdev->spi_cfg.operation = SPI_WORD_SET(8) | SPI_MODE_CPOL
		| SPI_MODE_CPHA;

	sdev->spi_cfg.frequency = freq;
	sdev->spi_cfg.slave = cs_num;

	cs_con.gpio_dev = device_get_binding(cs_gpio_name);
	cs_con.delay = 0;
	cs_con.gpio_pin = cs_gpio_num;

	sdev->spi_cfg.cs = &cs_con;

	sdev->spi =
		device_get_binding((char *)DT_NXP_KINETIS_DSPI_SPI_1_LABEL);
	if (!sdev->spi) {
		return -EINVAL;
	}

	/*
	 * Wait for SPI of KSZ8794 being fully operational - up to 10 ms
	 */
	for (timeout = 100, tmp = 0;
	     tmp != KSZ8794_CHIP_ID0_ID_DEFAULT && timeout > 0; timeout--) {
		dsa_ksz8794_read_reg(sdev, KSZ8794_CHIP_ID0, &tmp);
		k_busy_wait(100);
	}

	if (timeout == 0) {
		LOG_ERR("KSZ8794: No SPI communication!");
		return -ENODEV;
	}

	dsa_ksz8794_read_reg(sdev, KSZ8794_CHIP_ID0, &val[0]);
	dsa_ksz8794_read_reg(sdev, KSZ8794_CHIP_ID1, &val[1]);

	LOG_DBG("KSZ8794: ID0: 0x%x ID1: 0x%x timeout: %d", val[1], val[0], timeout);

	return 0;
}

static int dsa_ksz8794_switch_setup(struct dsa_ksz8794_spi *sdev)
{
	u8_t tmp, i;

	/* Disable tail tag feature */
	dsa_ksz8794_read_reg(sdev, KSZ8794_GLOBAL_CTRL10, &tmp);
	tmp &= ~KSZ8794_GLOBAL_CTRL10_TAIL_TAG_EN;
	dsa_ksz8794_write_reg(sdev, KSZ8794_GLOBAL_CTRL10, tmp);

	/* Loop through ports */
	for (i = KSZ8794_PORT1; i <= KSZ8794_PORT3; i++)
	{
		/* Enable transmission, reception and switch address learning */
		dsa_ksz8794_read_reg(sdev, KSZ8794_CTRL2_PORTn(i), &tmp);
		tmp |= KSZ8794_CTRL2_TRANSMIT_EN;
		tmp |= KSZ8794_CTRL2_RECEIVE_EN;
		tmp &= ~KSZ8794_CTRL2_LEARNING_DIS;
		dsa_ksz8794_write_reg(sdev, KSZ8794_CTRL2_PORTn(i), tmp);
	}

	dsa_ksz8794_read_reg(sdev, KSZ8794_PORT4_IF_CTRL6, &tmp);
	LOG_DBG("KSZ8794: CONTROL6: 0x%x port4", tmp);

	dsa_ksz8794_read_reg(sdev, KSZ8794_PORT4_CTRL2, &tmp);
	LOG_DBG("KSZ8794: CONTROL2: 0x%x port4", tmp);

	return 0;
}

int dsa_init(struct device *dev)
{
	struct dsa_ksz8794_spi *swspi = &phy_spi;

	/* Time needed for KSZ8794 to completely power up (100ms) */
	k_busy_wait(100000);

	/* Configure SPI */
	dsa_ksz8794_spi_setup(swspi, DT_ALIAS_DSA_SPI_CLOCK_FREQUENCY,
			      0, DT_ALIAS_DSA_SPI_CS_GPIOS_CONTROLLER,
			      DT_ALIAS_DSA_SPI_CS_GPIOS_PIN);

	/* Soft reset */
	dsa_ksz8794_soft_reset(swspi);

	/* Setup KSZ8794 */
	dsa_ksz8794_switch_setup(swspi);

	/* Read ports status */
	dsa_ksz8794_link_status(swspi);

	return 0;
}
