/*
 * Copyright (c) 2017 Piotr Mienkowski
 * Copyright (c) 2023 Gerson Fernando Budke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_i2c_twihs

/** @file
 * @brief I2C bus (TWIHS) driver for Atmel SAM MCU family.
 *
 * Only I2C Master Mode with 7 bit addressing is currently supported.
 */


#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_sam_twihs);

#include "i2c-priv.h"

/** I2C bus speed [Hz] in Standard Mode */
#define BUS_SPEED_STANDARD_HZ         100000U
/** I2C bus speed [Hz] in Fast Mode */
#define BUS_SPEED_FAST_HZ             400000U
/** I2C bus speed [Hz] in High Speed Mode */
#define BUS_SPEED_HIGH_HZ            3400000U
/* Maximum value of Clock Divider (CKDIV) */
#define CKDIV_MAX                          7

/* Device constant configuration parameters */
struct i2c_sam_twihs_dev_cfg {
	Twihs *regs;
	void (*irq_config)(void);
	uint32_t bitrate;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	uint8_t irq_id;
};

struct twihs_msg {
	/* Buffer containing data to read or write */
	uint8_t *buf;
	/* Length of the buffer */
	uint32_t len;
	/* Index of the next byte to be read/written from/to the buffer */
	uint32_t idx;
	/* Value of TWIHS_SR at the end of the message */
	uint32_t twihs_sr;
	/* Transfer flags as defined in the i2c.h file */
	uint8_t flags;
};

/* Device run time data */
struct i2c_sam_twihs_dev_data {
	struct k_sem sem;
	struct twihs_msg msg;
};

static int i2c_clk_set(Twihs *const twihs, uint32_t speed)
{
	uint32_t ck_div = 0U;
	uint32_t cl_div;
	bool div_completed = false;

	/*  From the datasheet "TWIHS Clock Waveform Generator Register"
	 *  T_low = ( ( CLDIV × 2^CKDIV ) + 3 ) × T_MCK
	 */
	while (!div_completed) {
		cl_div =   ((SOC_ATMEL_SAM_MCK_FREQ_HZ / (speed * 2U)) - 3)
			 / (1 << ck_div);

		if (cl_div <= 255U) {
			div_completed = true;
		} else {
			ck_div++;
		}
	}

	if (ck_div > CKDIV_MAX) {
		LOG_ERR("Failed to configure I2C clock");
		return -EIO;
	}

	/* Set I2C bus clock duty cycle to 50% */
	twihs->TWIHS_CWGR = TWIHS_CWGR_CLDIV(cl_div) | TWIHS_CWGR_CHDIV(cl_div)
			    | TWIHS_CWGR_CKDIV(ck_div);

	return 0;
}

static int i2c_sam_twihs_configure(const struct device *dev, uint32_t config)
{
	const struct i2c_sam_twihs_dev_cfg *const dev_cfg = dev->config;
	Twihs *const twihs = dev_cfg->regs;
	uint32_t bitrate;
	int ret;

	if (!(config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("Master Mode is not enabled");
		return -EIO;
	}

	if (config & I2C_ADDR_10_BITS) {
		LOG_ERR("I2C 10-bit addressing is currently not supported");
		LOG_ERR("Please submit a patch");
		return -EIO;
	}

	/* Configure clock */
	switch (I2C_SPEED_GET(config)) {
	case I2C_SPEED_STANDARD:
		bitrate = BUS_SPEED_STANDARD_HZ;
		break;
	case I2C_SPEED_FAST:
		bitrate = BUS_SPEED_FAST_HZ;
		break;
	default:
		LOG_ERR("Unsupported I2C speed value");
		return -EIO;
	}

	/* Setup clock waveform */
	ret = i2c_clk_set(twihs, bitrate);
	if (ret < 0) {
		return ret;
	}

	/* Disable Slave Mode */
	twihs->TWIHS_CR = TWIHS_CR_SVDIS;

	/* Enable Master Mode */
	twihs->TWIHS_CR = TWIHS_CR_MSEN;

	return 0;
}

static void write_msg_start(Twihs *const twihs, struct twihs_msg *msg,
			    uint8_t daddr)
{
	/* Set slave address. */
	twihs->TWIHS_MMR = TWIHS_MMR_DADR(daddr);

	/* Write first data byte on I2C bus */
	twihs->TWIHS_THR = msg->buf[msg->idx++];

	/* Enable Transmit Ready and Transmission Completed interrupts */
	twihs->TWIHS_IER = TWIHS_IER_TXRDY | TWIHS_IER_TXCOMP | TWIHS_IER_NACK;
}

static void read_msg_start(Twihs *const twihs, struct twihs_msg *msg,
			   uint8_t daddr)
{
	uint32_t twihs_cr_stop;

	/* Set slave address and number of internal address bytes */
	twihs->TWIHS_MMR = TWIHS_MMR_MREAD | TWIHS_MMR_DADR(daddr);

	/* Enable Receive Ready and Transmission Completed interrupts */
	twihs->TWIHS_IER = TWIHS_IER_RXRDY | TWIHS_IER_TXCOMP | TWIHS_IER_NACK;

	/* In single data byte read the START and STOP must both be set */
	twihs_cr_stop = (msg->len == 1U) ? TWIHS_CR_STOP : 0;
	/* Start the transfer by sending START condition */
	twihs->TWIHS_CR = TWIHS_CR_START | twihs_cr_stop;
}

static int i2c_sam_twihs_transfer(const struct device *dev,
				  struct i2c_msg *msgs,
				  uint8_t num_msgs, uint16_t addr)
{
	const struct i2c_sam_twihs_dev_cfg *const dev_cfg = dev->config;
	struct i2c_sam_twihs_dev_data *const dev_data = dev->data;
	Twihs *const twihs = dev_cfg->regs;

	__ASSERT_NO_MSG(msgs);
	if (!num_msgs) {
		return 0;
	}

	/* Clear pending interrupts, such as NACK. */
	(void)twihs->TWIHS_SR;

	/* Set number of internal address bytes to 0, not used. */
	twihs->TWIHS_IADR = 0;

	for (int i = 0; i < num_msgs; i++) {
		dev_data->msg.buf = msgs[i].buf;
		dev_data->msg.len = msgs[i].len;
		dev_data->msg.idx = 0U;
		dev_data->msg.twihs_sr = 0U;
		dev_data->msg.flags = msgs[i].flags;
		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			read_msg_start(twihs, &dev_data->msg, addr);
		} else {
			write_msg_start(twihs, &dev_data->msg, addr);
		}
		/* Wait for the transfer to complete */
		k_sem_take(&dev_data->sem, K_FOREVER);

		if (dev_data->msg.twihs_sr > 0) {
			/* Something went wrong */
			return -EIO;
		}
	}

	return 0;
}

static void i2c_sam_twihs_isr(const struct device *dev)
{
	const struct i2c_sam_twihs_dev_cfg *const dev_cfg = dev->config;
	struct i2c_sam_twihs_dev_data *const dev_data = dev->data;
	Twihs *const twihs = dev_cfg->regs;
	struct twihs_msg *msg = &dev_data->msg;
	uint32_t isr_status;

	/* Retrieve interrupt status */
	isr_status = twihs->TWIHS_SR & twihs->TWIHS_IMR;

	/* Not Acknowledged */
	if (isr_status & TWIHS_SR_NACK) {
		msg->twihs_sr = isr_status;
		goto tx_comp;
	}

	/* Byte received */
	if (isr_status & TWIHS_SR_RXRDY) {

		msg->buf[msg->idx++] = twihs->TWIHS_RHR;

		if (msg->idx == msg->len - 1U) {
			/* Send STOP condition */
			twihs->TWIHS_CR = TWIHS_CR_STOP;
		}
	}

	/* Byte sent */
	if (isr_status & TWIHS_SR_TXRDY) {
		if (msg->idx == msg->len) {
			if (msg->flags & I2C_MSG_STOP) {
				/* Send STOP condition */
				twihs->TWIHS_CR = TWIHS_CR_STOP;
				/* Disable Transmit Ready interrupt */
				twihs->TWIHS_IDR = TWIHS_IDR_TXRDY;
			} else {
				/* Transmission completed */
				goto tx_comp;
			}
		} else {
			twihs->TWIHS_THR = msg->buf[msg->idx++];
		}
	}

	/* Transmission completed */
	if (isr_status & TWIHS_SR_TXCOMP) {
		goto tx_comp;
	}

	return;

tx_comp:
	/* Disable all enabled interrupts */
	twihs->TWIHS_IDR = twihs->TWIHS_IMR;
	/* We are done */
	k_sem_give(&dev_data->sem);
}

static int i2c_sam_twihs_initialize(const struct device *dev)
{
	const struct i2c_sam_twihs_dev_cfg *const dev_cfg = dev->config;
	struct i2c_sam_twihs_dev_data *const dev_data = dev->data;
	Twihs *const twihs = dev_cfg->regs;
	uint32_t bitrate_cfg;
	int ret;

	/* Configure interrupts */
	dev_cfg->irq_config();

	/* Initialize semaphore */
	k_sem_init(&dev_data->sem, 0, 1);

	/* Connect pins to the peripheral */
	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Enable TWIHS clock in PMC */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&dev_cfg->clock_cfg);

	/* Reset the module */
	twihs->TWIHS_CR = TWIHS_CR_SWRST;

	bitrate_cfg = i2c_map_dt_bitrate(dev_cfg->bitrate);

	ret = i2c_sam_twihs_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to initialize %s device", dev->name);
		return ret;
	}

	/* Enable module's IRQ */
	irq_enable(dev_cfg->irq_id);

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static const struct i2c_driver_api i2c_sam_twihs_driver_api = {
	.configure = i2c_sam_twihs_configure,
	.transfer = i2c_sam_twihs_transfer,
};

#define I2C_TWIHS_SAM_INIT(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
	static void i2c##n##_sam_irq_config(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    i2c_sam_twihs_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
	}								\
									\
	static const struct i2c_sam_twihs_dev_cfg i2c##n##_sam_config = {\
		.regs = (Twihs *)DT_INST_REG_ADDR(n),			\
		.irq_config = i2c##n##_sam_irq_config,			\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),		\
		.irq_id = DT_INST_IRQN(n),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.bitrate = DT_INST_PROP(n, clock_frequency),		\
	};								\
									\
	static struct i2c_sam_twihs_dev_data i2c##n##_sam_data;		\
									\
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_sam_twihs_initialize,	\
			    NULL,					\
			    &i2c##n##_sam_data, &i2c##n##_sam_config,	\
			    POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,	\
			    &i2c_sam_twihs_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_TWIHS_SAM_INIT)
