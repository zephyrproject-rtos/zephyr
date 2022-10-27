/*
 * Copyright (c) 2021 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rcar_i2c

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <soc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_cpg_mssr.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_rcar);

#include "i2c-priv.h"

typedef void (*init_func_t)(const struct device *dev);

struct i2c_rcar_cfg {
	uint32_t reg_addr;
	init_func_t init_func;
	const struct device *clock_dev;
	struct rcar_cpg_clk mod_clk;
	uint32_t bitrate;
};

struct i2c_rcar_data {
	uint8_t status_mask;
	struct k_sem int_sem;
};

/* Registers */
#define RCAR_I2C_ICSCR          0x00    /* Slave Control Register */
#define RCAR_I2C_ICMCR          0x04    /* Master Control Register */
#define RCAR_I2C_ICSIER         0x10    /* Slave IRQ Enable */
#define RCAR_I2C_ICMIER         0x14    /* Master IRQ Enable */
#define RCAR_I2C_ICSSR          0x08    /* Slave Status */
#define RCAR_I2C_ICMSR          0x0c    /* Master Status */
#define RCAR_I2C_ICCCR          0x18    /* Clock Control Register */
#define RCAR_I2C_ICSAR          0x1c    /* Slave Address Register */
#define RCAR_I2C_ICMAR          0x20    /* Master Address Register */
#define RCAR_I2C_ICRXD_ICTXD    0x24    /* Receive Transmit Data Register */
#define RCAR_I2C_ICFBSCR        0x38    /* First Bit Setup Cycle (Gen3).*/
#define RCAR_I2C_ICFBSCR_TCYC17 0x0f    /* 17*Tcyc */

#define RCAR_I2C_ICMCR_MDBS     BIT(7)  /* Master Data Buffer Select */
#define RCAR_I2C_ICMCR_FSCL     BIT(6)  /* Forced SCL */
#define RCAR_I2C_ICMCR_FSDA     BIT(5)  /* Forced SDA */
#define RCAR_I2C_ICMCR_OBPC     BIT(4)  /* Override Bus Pin Control */
#define RCAR_I2C_ICMCR_MIE      BIT(3)  /* Master Interface Enable */
#define RCAR_I2C_ICMCR_TSBE     BIT(2)  /* Start Byte Transmission Enable */
#define RCAR_I2C_ICMCR_FSB      BIT(1)  /* Forced Stop onto the Bus */
#define RCAR_I2C_ICMCR_ESG      BIT(0)  /* Enable Start Generation */
#define RCAR_I2C_ICMCR_MASTER   (RCAR_I2C_ICMCR_MDBS | RCAR_I2C_ICMCR_MIE)

/* Bits to manage ICMIER and ICMSR registers */
#define RCAR_I2C_MNR            BIT(6)  /* Master Nack Received */
#define RCAR_I2C_MAL            BIT(5)  /* Master Arbitration lost */
#define RCAR_I2C_MST            BIT(4)  /* Master Stop Transmitted */
#define RCAR_I2C_MDE            BIT(3)  /* Master Data Empty */
#define RCAR_I2C_MDT            BIT(2)  /* Master Data Transmitted */
#define RCAR_I2C_MDR            BIT(1)  /* Master Data Received */
#define RCAR_I2C_MAT            BIT(0)  /* Master Address Transmitted */

/* Recommended bitrate settings from official documentation */
#define RCAR_I2C_ICCCR_CDF_100_KHZ  6
#define RCAR_I2C_ICCCR_CDF_400_KHZ  6
#define RCAR_I2C_ICCCR_SCGD_100_KHZ 21
#define RCAR_I2C_ICCCR_SCGD_400_KHZ 3

#define MAX_WAIT_US 100

static uint32_t i2c_rcar_read(const struct i2c_rcar_cfg *config,
			      uint32_t offs)
{
	return sys_read32(config->reg_addr + offs);
}

static void i2c_rcar_write(const struct i2c_rcar_cfg *config,
			   uint32_t offs, uint32_t value)
{
	sys_write32(value, config->reg_addr + offs);
}

static void i2c_rcar_isr(const struct device *dev)
{
	const struct i2c_rcar_cfg *config = dev->config;
	struct i2c_rcar_data *data = dev->data;

	if (((i2c_rcar_read(config, RCAR_I2C_ICMSR)) & data->status_mask) ==
	    data->status_mask) {
		k_sem_give(&data->int_sem);
		i2c_rcar_write(config, RCAR_I2C_ICMIER, 0);
	}
}

static int i2c_rcar_wait_for_state(const struct device *dev, uint8_t mask)
{
	const struct i2c_rcar_cfg *config = dev->config;
	struct i2c_rcar_data *data = dev->data;

	data->status_mask = mask;

	/* Reset interrupts semaphore */
	k_sem_reset(&data->int_sem);

	/* Enable interrupts */
	i2c_rcar_write(config, RCAR_I2C_ICMIER, mask);

	/* Wait for the interrupts */
	return k_sem_take(&data->int_sem, K_USEC(MAX_WAIT_US));
}

static int i2c_rcar_finish(const struct device *dev)
{
	const struct i2c_rcar_cfg *config = dev->config;
	int ret;

	/* Enable STOP generation */
	i2c_rcar_write(config, RCAR_I2C_ICMCR, RCAR_I2C_ICMCR_MASTER | RCAR_I2C_ICMCR_FSB);
	i2c_rcar_write(config, RCAR_I2C_ICMSR, 0);

	/* Wait for STOP to be transmitted */
	ret = i2c_rcar_wait_for_state(dev, RCAR_I2C_MST);
	i2c_rcar_write(config, RCAR_I2C_ICMSR, 0);

	/* Disable STOP generation */
	i2c_rcar_write(config, RCAR_I2C_ICMCR, RCAR_I2C_ICMCR_MASTER);

	return ret;
}

static int i2c_rcar_set_addr(const struct device *dev,
			     uint8_t chip, uint8_t read)
{
	const struct i2c_rcar_cfg *config = dev->config;

	/* Set slave address & transfer mode */
	i2c_rcar_write(config, RCAR_I2C_ICMAR, (chip << 1) | read);
	/* Reset */
	i2c_rcar_write(config, RCAR_I2C_ICMCR, RCAR_I2C_ICMCR_MASTER | RCAR_I2C_ICMCR_ESG);
	/* Clear Status */
	i2c_rcar_write(config, RCAR_I2C_ICMSR, 0);

	/* Wait for address & transfer mode to be transmitted */
	if (read != 0) {
		return i2c_rcar_wait_for_state(dev, RCAR_I2C_MAT | RCAR_I2C_MDR);
	} else {
		return i2c_rcar_wait_for_state(dev, RCAR_I2C_MAT | RCAR_I2C_MDE);
	}
}

static int i2c_rcar_transfer_msg(const struct device *dev, struct i2c_msg *msg)
{
	const struct i2c_rcar_cfg *config = dev->config;
	uint32_t i, reg;
	int ret = 0;

	if ((msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
		/* Reading as master */
		i2c_rcar_write(config, RCAR_I2C_ICMCR, RCAR_I2C_ICMCR_MASTER);

		for (i = 0; i < msg->len; i++) {
			if (msg->len - 1 == i) {
				i2c_rcar_write(config, RCAR_I2C_ICMCR, RCAR_I2C_ICMCR_MASTER |
					       RCAR_I2C_ICMCR_FSB);
			}

			/* Start data reception */
			reg = i2c_rcar_read(config, RCAR_I2C_ICMSR);
			reg &= ~RCAR_I2C_MDR;
			i2c_rcar_write(config, RCAR_I2C_ICMSR, reg);

			/* Wait for data to be received */
			ret = i2c_rcar_wait_for_state(dev, RCAR_I2C_MDR);
			if (ret != 0) {
				return ret;
			}

			msg->buf[i] = i2c_rcar_read(config, RCAR_I2C_ICRXD_ICTXD) & 0xff;
		}
	} else {
		/* Writing as master */
		for (i = 0; i < msg->len; i++) {
			i2c_rcar_write(config, RCAR_I2C_ICRXD_ICTXD, msg->buf[i]);

			i2c_rcar_write(config, RCAR_I2C_ICMCR, RCAR_I2C_ICMCR_MASTER);

			/* Start data transmission */
			reg = i2c_rcar_read(config, RCAR_I2C_ICMSR);
			reg &= ~RCAR_I2C_MDE;
			i2c_rcar_write(config, RCAR_I2C_ICMSR, reg);

			/* Wait for all data to be transmitted */
			ret = i2c_rcar_wait_for_state(dev, RCAR_I2C_MDE);
			if (ret != 0) {
				return ret;
			}
		}
	}

	return ret;
}

static int i2c_rcar_transfer(const struct device *dev,
			     struct i2c_msg *msgs, uint8_t num_msgs,
			     uint16_t addr)
{
	const struct i2c_rcar_cfg *config = dev->config;
	uint16_t timeout = 0;
	int ret;

	if (!num_msgs) {
		return 0;
	}

	/* Wait for the bus to be available */
	while ((i2c_rcar_read(config, RCAR_I2C_ICMCR) & RCAR_I2C_ICMCR_FSDA) && (timeout < 10)) {
		k_busy_wait(USEC_PER_MSEC);
		timeout++;
	}
	if (timeout == 10) {
		return -EIO;
	}

	do {
		/* We are not supporting 10-bit addressing */
		if ((msgs->flags & I2C_MSG_ADDR_10_BITS) == I2C_MSG_ADDR_10_BITS) {
			return -ENOTSUP;
		}

		/* Send slave address */
		if (i2c_rcar_set_addr(dev, addr, !!(msgs->flags & I2C_MSG_READ))) {
			return -EIO; /* No ACK received */
		}

		/* Transfer data */
		if (msgs->len) {
			ret = i2c_rcar_transfer_msg(dev, msgs);
			if (ret != 0) {
				return ret;
			}
		}

		/* Finish the transfer */
		if ((msgs->flags & I2C_MSG_STOP) == I2C_MSG_STOP) {
			ret = i2c_rcar_finish(dev);
			if (ret != 0) {
				return ret;
			}
		}

		/* Next message */
		msgs++;
		num_msgs--;
	} while (num_msgs);

	/* Complete without error */
	return 0;
}

static int i2c_rcar_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_rcar_cfg *config = dev->config;
	uint8_t cdf, scgd;

	/* We only support Master mode */
	if ((dev_config & I2C_MODE_CONTROLLER) != I2C_MODE_CONTROLLER) {
		return -ENOTSUP;
	}

	/* We are not supporting 10-bit addressing */
	if ((dev_config & I2C_ADDR_10_BITS) == I2C_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		/* Use recommended value for 100 kHz bus */
		cdf = RCAR_I2C_ICCCR_CDF_100_KHZ;
		scgd = RCAR_I2C_ICCCR_SCGD_100_KHZ;
		break;
	case I2C_SPEED_FAST:
		/* Use recommended value for 400 kHz bus */
		cdf = RCAR_I2C_ICCCR_CDF_400_KHZ;
		scgd = RCAR_I2C_ICCCR_SCGD_400_KHZ;
		break;
	default:
		return -ENOTSUP;
	}

	/* Setting ICCCR to recommended value */
	i2c_rcar_write(config, RCAR_I2C_ICCCR, (scgd << 3) | cdf);

	/* Reset slave mode */
	i2c_rcar_write(config, RCAR_I2C_ICSIER, 0);
	i2c_rcar_write(config, RCAR_I2C_ICSAR, 0);
	i2c_rcar_write(config, RCAR_I2C_ICSCR, 0);
	i2c_rcar_write(config, RCAR_I2C_ICSSR, 0);

	/* Reset master mode */
	i2c_rcar_write(config, RCAR_I2C_ICMIER, 0);
	i2c_rcar_write(config, RCAR_I2C_ICMCR, 0);
	i2c_rcar_write(config, RCAR_I2C_ICMSR, 0);
	i2c_rcar_write(config, RCAR_I2C_ICMAR, 0);

	return 0;
}

static int i2c_rcar_init(const struct device *dev)
{
	const struct i2c_rcar_cfg *config = dev->config;
	struct i2c_rcar_data *data = dev->data;
	uint32_t bitrate_cfg;
	int ret;

	k_sem_init(&data->int_sem, 0, 1);

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev,
			       (clock_control_subsys_t *)&config->mod_clk);

	if (ret != 0) {
		return ret;
	}

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);

	ret = i2c_rcar_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (ret != 0) {
		return ret;
	}

	config->init_func(dev);

	return 0;
}

static const struct i2c_driver_api i2c_rcar_driver_api = {
	.configure = i2c_rcar_configure,
	.transfer = i2c_rcar_transfer,
};

/* Device Instantiation */
#define I2C_RCAR_INIT(n)						       \
	static void i2c_rcar_##n##_init(const struct device *dev);	       \
	static const struct i2c_rcar_cfg i2c_rcar_cfg_##n = {		       \
		.reg_addr = DT_INST_REG_ADDR(n),			       \
		.init_func = i2c_rcar_##n##_init,			       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	       \
		.bitrate = DT_INST_PROP(n, clock_frequency),		       \
		.mod_clk.module =					       \
			DT_INST_CLOCKS_CELL_BY_IDX(n, 0, module),	       \
		.mod_clk.domain =					       \
			DT_INST_CLOCKS_CELL_BY_IDX(n, 0, domain),	       \
	};								       \
									       \
	static struct i2c_rcar_data i2c_rcar_data_##n;			       \
									       \
	I2C_DEVICE_DT_INST_DEFINE(n,					       \
			      i2c_rcar_init,				       \
			      NULL,					       \
			      &i2c_rcar_data_##n,			       \
			      &i2c_rcar_cfg_##n,			       \
			      POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,	       \
			      &i2c_rcar_driver_api			       \
			      );					       \
	static void i2c_rcar_##n##_init(const struct device *dev)	       \
	{								       \
		IRQ_CONNECT(DT_INST_IRQN(n),				       \
			    0,						       \
			    i2c_rcar_isr,				       \
			    DEVICE_DT_INST_GET(n), 0);			       \
									       \
		irq_enable(DT_INST_IRQN(n));				       \
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_RCAR_INIT)
