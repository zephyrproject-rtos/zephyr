/*
 * Copyright (c) 2017 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief I2C bus (TWI) driver for Atmel SAM MCU family.
 *
 * Limitations:
 * - Only I2C Master Mode with 7 bit addressing is currently supported.
 * - No reentrancy support.
 */

#include <errno.h>
#include <sys/__assert.h>
#include <stdbool.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/i2c.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_sam_twi);

#include "i2c-priv.h"

/** I2C bus speed [Hz] in Standard Mode */
#define BUS_SPEED_STANDARD_HZ         100000U
/** I2C bus speed [Hz] in Fast Mode */
#define BUS_SPEED_FAST_HZ             400000U
/* Maximum value of Clock Divider (CKDIV) */
#define CKDIV_MAX                          7

/* Device constant configuration parameters */
struct i2c_sam_twi_dev_cfg {
	Twi *regs;
	void (*irq_config)(void);
	u32_t bitrate;
	const struct soc_gpio_pin *pin_list;
	u8_t pin_list_size;
	u8_t periph_id;
	u8_t irq_id;
};

struct twi_msg {
	/* Buffer containing data to read or write */
	u8_t *buf;
	/* Length of the buffer */
	u32_t len;
	/* Index of the next byte to be read/written from/to the buffer */
	u32_t idx;
	/* Value of TWI_SR at the end of the message */
	u32_t twi_sr;
	/* Transfer flags as defined in the i2c.h file */
	u8_t flags;
};

/* Device run time data */
struct i2c_sam_twi_dev_data {
	struct k_sem sem;
	struct twi_msg msg;
};

#define DEV_NAME(dev) ((dev)->config->name)
#define DEV_CFG(dev) \
	((const struct i2c_sam_twi_dev_cfg *const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct i2c_sam_twi_dev_data *const)(dev)->driver_data)

static int i2c_clk_set(Twi *const twi, u32_t speed)
{
	u32_t ck_div = 0U;
	u32_t cl_div;
	bool div_completed = false;

	/*  From the datasheet "TWI Clock Waveform Generator Register"
	 *  T_low = ( ( CLDIV × 2^CKDIV ) + 4 ) × T_MCK
	 */
	while (!div_completed) {
		cl_div =   ((SOC_ATMEL_SAM_MCK_FREQ_HZ / (speed * 2U)) - 4)
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

	/* Set TWI clock duty cycle to 50% */
	twi->TWI_CWGR = TWI_CWGR_CLDIV(cl_div) | TWI_CWGR_CHDIV(cl_div)
			| TWI_CWGR_CKDIV(ck_div);

	return 0;
}

static int i2c_sam_twi_configure(struct device *dev, u32_t config)
{
	const struct i2c_sam_twi_dev_cfg *const dev_cfg = DEV_CFG(dev);
	Twi *const twi = dev_cfg->regs;
	u32_t bitrate;
	int ret;

	if (!(config & I2C_MODE_MASTER)) {
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
	ret = i2c_clk_set(twi, bitrate);
	if (ret < 0) {
		return ret;
	}

	/* Disable Slave Mode */
	twi->TWI_CR = TWI_CR_SVDIS;

	/* Enable Master Mode */
	twi->TWI_CR = TWI_CR_MSEN;

	return 0;
}

static void write_msg_start(Twi *const twi, struct twi_msg *msg, u8_t daddr)
{
	/* Set slave address and number of internal address bytes. */
	twi->TWI_MMR = TWI_MMR_DADR(daddr);

	/* Write first data byte on I2C bus */
	twi->TWI_THR = msg->buf[msg->idx++];

	/* Enable Transmit Ready and Transmission Completed interrupts */
	twi->TWI_IER = TWI_IER_TXRDY | TWI_IER_TXCOMP | TWI_IER_NACK;
}

static void read_msg_start(Twi *const twi, struct twi_msg *msg, u8_t daddr)
{
	u32_t twi_cr_stop;

	/* Set slave address and number of internal address bytes */
	twi->TWI_MMR = TWI_MMR_MREAD | TWI_MMR_DADR(daddr);

	/* In single data byte read the START and STOP must both be set */
	twi_cr_stop = (msg->len == 1U) ? TWI_CR_STOP : 0;
	/* Start the transfer by sending START condition */
	twi->TWI_CR = TWI_CR_START | twi_cr_stop;

	/* Enable Receive Ready and Transmission Completed interrupts */
	twi->TWI_IER = TWI_IER_RXRDY | TWI_IER_TXCOMP | TWI_IER_NACK;
}

static int i2c_sam_twi_transfer(struct device *dev, struct i2c_msg *msgs,
				u8_t num_msgs, u16_t addr)
{
	const struct i2c_sam_twi_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct i2c_sam_twi_dev_data *const dev_data = DEV_DATA(dev);
	Twi *const twi = dev_cfg->regs;

	__ASSERT_NO_MSG(msgs);
	if (!num_msgs) {
		return 0;
	}

	/* Clear pending interrupts, such as NACK. */
	(void)twi->TWI_SR;

	/* Set number of internal address bytes to 0, not used. */
	twi->TWI_IADR = 0;

	for (; num_msgs > 0; num_msgs--, msgs++) {
		dev_data->msg.buf = msgs->buf;
		dev_data->msg.len = msgs->len;
		dev_data->msg.idx = 0U;
		dev_data->msg.twi_sr = 0U;
		dev_data->msg.flags = msgs->flags;

		/*
		 * REMARK: Dirty workaround:
		 *
		 * The controller does not have a documented, generic way to
		 * issue RESTART when changing transfer direction as master.
		 * Send a stop condition in such a case.
		 */
		if (num_msgs > 1) {
			if ((msgs[0].flags & I2C_MSG_RW_MASK) !=
			    (msgs[1].flags & I2C_MSG_RW_MASK)) {
				dev_data->msg.flags |= I2C_MSG_STOP;
			}
		}

		if ((msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			read_msg_start(twi, &dev_data->msg, addr);
		} else {
			write_msg_start(twi, &dev_data->msg, addr);
		}
		/* Wait for the transfer to complete */
		k_sem_take(&dev_data->sem, K_FOREVER);

		if (dev_data->msg.twi_sr > 0) {
			/* Something went wrong */
			return -EIO;
		}
	}

	return 0;
}

static void i2c_sam_twi_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct i2c_sam_twi_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct i2c_sam_twi_dev_data *const dev_data = DEV_DATA(dev);
	Twi *const twi = dev_cfg->regs;
	struct twi_msg *msg = &dev_data->msg;
	u32_t isr_status;

	/* Retrieve interrupt status */
	isr_status = twi->TWI_SR & twi->TWI_IMR;

	/* Not Acknowledged */
	if (isr_status & TWI_SR_NACK) {
		msg->twi_sr = isr_status;
		goto tx_comp;
	}

	/* Byte received */
	if (isr_status & TWI_SR_RXRDY) {

		msg->buf[msg->idx++] = twi->TWI_RHR;

		if (msg->idx == msg->len - 1U) {
			/* Send a STOP condition on the TWI */
			twi->TWI_CR = TWI_CR_STOP;
		}
	}

	/* Byte sent */
	if (isr_status & TWI_SR_TXRDY) {
		if (msg->idx == msg->len) {
			if (msg->flags & I2C_MSG_STOP) {
				/* Send a STOP condition on the TWI */
				twi->TWI_CR = TWI_CR_STOP;
				/* Disable Transmit Ready interrupt */
				twi->TWI_IDR = TWI_IDR_TXRDY;
			} else {
				/* Transmission completed */
				goto tx_comp;
			}
		} else {
			twi->TWI_THR = msg->buf[msg->idx++];
		}
	}

	/* Transmission completed */
	if (isr_status & TWI_SR_TXCOMP) {
		goto tx_comp;
	}

	return;

tx_comp:
	/* Disable all enabled interrupts */
	twi->TWI_IDR = twi->TWI_IMR;
	/* We are done */
	k_sem_give(&dev_data->sem);
}

static int i2c_sam_twi_initialize(struct device *dev)
{
	const struct i2c_sam_twi_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct i2c_sam_twi_dev_data *const dev_data = DEV_DATA(dev);
	Twi *const twi = dev_cfg->regs;
	u32_t bitrate_cfg;
	int ret;

	/* Configure interrupts */
	dev_cfg->irq_config();

	/* Initialize semaphore */
	k_sem_init(&dev_data->sem, 0, 1);

	/* Connect pins to the peripheral */
	soc_gpio_list_configure(dev_cfg->pin_list, dev_cfg->pin_list_size);

	/* Enable module's clock */
	soc_pmc_peripheral_enable(dev_cfg->periph_id);

	/* Reset TWI module */
	twi->TWI_CR = TWI_CR_SWRST;

	bitrate_cfg = i2c_map_dt_bitrate(dev_cfg->bitrate);

	ret = i2c_sam_twi_configure(dev, I2C_MODE_MASTER | bitrate_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to initialize %s device", DEV_NAME(dev));
		return ret;
	}

	/* Enable module's IRQ */
	irq_enable(dev_cfg->irq_id);

	LOG_INF("Device %s initialized", DEV_NAME(dev));

	return 0;
}

static const struct i2c_driver_api i2c_sam_twi_driver_api = {
	.configure = i2c_sam_twi_configure,
	.transfer = i2c_sam_twi_transfer,
};

/* I2C0 */

#ifdef CONFIG_I2C_0
static struct device DEVICE_NAME_GET(i2c0_sam);

static void i2c0_sam_irq_config(void)
{
	IRQ_CONNECT(DT_I2C_0_IRQ, DT_I2C_0_IRQ_PRI, i2c_sam_twi_isr,
		    DEVICE_GET(i2c0_sam), 0);
}

static const struct soc_gpio_pin pins_twi0[] = PINS_TWI0;

static const struct i2c_sam_twi_dev_cfg i2c0_sam_config = {
	.regs = (Twi *)DT_I2C_0_BASE_ADDRESS,
	.irq_config = i2c0_sam_irq_config,
	.periph_id = DT_I2C_0_PERIPHERAL_ID,
	.irq_id = DT_I2C_0_IRQ,
	.pin_list = pins_twi0,
	.pin_list_size = ARRAY_SIZE(pins_twi0),
	.bitrate = DT_I2C_0_BITRATE,
};

static struct i2c_sam_twi_dev_data i2c0_sam_data;

DEVICE_AND_API_INIT(i2c0_sam, DT_I2C_0_NAME, &i2c_sam_twi_initialize,
		    &i2c0_sam_data, &i2c0_sam_config, POST_KERNEL,
		    CONFIG_I2C_INIT_PRIORITY, &i2c_sam_twi_driver_api);
#endif

/* I2C1 */

#ifdef CONFIG_I2C_1
static struct device DEVICE_NAME_GET(i2c1_sam);

static void i2c1_sam_irq_config(void)
{
	IRQ_CONNECT(DT_I2C_1_IRQ, DT_I2C_1_IRQ_PRI, i2c_sam_twi_isr,
		    DEVICE_GET(i2c1_sam), 0);
}

static const struct soc_gpio_pin pins_twi1[] = PINS_TWI1;

static const struct i2c_sam_twi_dev_cfg i2c1_sam_config = {
	.regs = (Twi *)DT_I2C_1_BASE_ADDRESS,
	.irq_config = i2c1_sam_irq_config,
	.periph_id = DT_I2C_1_PERIPHERAL_ID,
	.irq_id = DT_I2C_1_IRQ,
	.pin_list = pins_twi1,
	.pin_list_size = ARRAY_SIZE(pins_twi1),
	.bitrate = DT_I2C_1_BITRATE,
};

static struct i2c_sam_twi_dev_data i2c1_sam_data;

DEVICE_AND_API_INIT(i2c1_sam, DT_I2C_1_NAME, &i2c_sam_twi_initialize,
		    &i2c1_sam_data, &i2c1_sam_config, POST_KERNEL,
		    CONFIG_I2C_INIT_PRIORITY, &i2c_sam_twi_driver_api);
#endif
