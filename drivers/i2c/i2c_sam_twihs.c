/*
 * Copyright (c) 2017 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief I2C bus (TWIHS) driver for Atmel SAM MCU family.
 *
 * Only I2C Master Mode with 7 bit addressing is currently supported.
 */

#define SYS_LOG_DOMAIN "dev/i2c_sam_twihs"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_I2C_LEVEL
#include <logging/sys_log.h>

#include <errno.h>
#include <misc/__assert.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <i2c.h>
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
	u32_t bitrate;
	const struct soc_gpio_pin *pin_list;
	u8_t pin_list_size;
	u8_t periph_id;
	u8_t irq_id;
};

struct twihs_msg {
	/* Buffer containing data to read or write */
	u8_t *buf;
	/* Length of the buffer */
	u32_t len;
	/* Index of the next byte to be read/written from/to the buffer */
	u32_t idx;
	/* Value of TWIHS_SR at the end of the message */
	u32_t twihs_sr;
	/* Transfer flags as defined in the i2c.h file */
	u8_t flags;
};

/* Device run time data */
struct i2c_sam_twihs_dev_data {
	struct k_sem sem;
	struct twihs_msg msg;
};

#define DEV_NAME(dev) ((dev)->config->name)
#define DEV_CFG(dev) \
	((const struct i2c_sam_twihs_dev_cfg *const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct i2c_sam_twihs_dev_data *const)(dev)->driver_data)

static int i2c_clk_set(Twihs *const twihs, u32_t speed)
{
	u32_t ck_div = 0;
	u32_t cl_div;
	bool div_completed = false;

	/*  From the datasheet "TWIHS Clock Waveform Generator Register"
	 *  T_low = ( ( CLDIV × 2^CKDIV ) + 3 ) × T_MCK
	 */
	while (!div_completed) {
		cl_div =   ((SOC_ATMEL_SAM_MCK_FREQ_HZ / (2 * speed)) - 3)
			 / (1 << ck_div);

		if (cl_div <= 255) {
			div_completed = true;
		} else {
			ck_div++;
		}
	}

	if (ck_div > CKDIV_MAX) {
		SYS_LOG_ERR("Failed to configure I2C clock");
		return -EIO;
	}

	/* Set I2C bus clock duty cycle to 50% */
	twihs->TWIHS_CWGR = TWIHS_CWGR_CLDIV(cl_div) | TWIHS_CWGR_CHDIV(cl_div)
			    | TWIHS_CWGR_CKDIV(ck_div);

	return 0;
}

static int i2c_sam_twihs_configure(struct device *dev, u32_t config)
{
	const struct i2c_sam_twihs_dev_cfg *const dev_cfg = DEV_CFG(dev);
	Twihs *const twihs = dev_cfg->regs;
	u32_t bitrate;
	int ret;

	if (!(config & I2C_MODE_MASTER)) {
		SYS_LOG_ERR("Master Mode is not enabled");
		return -EIO;
	}

	if (config & I2C_ADDR_10_BITS) {
		SYS_LOG_ERR("I2C 10-bit addressing is currently not supported");
		SYS_LOG_ERR("Please submit a patch");
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
		SYS_LOG_ERR("Unsupported I2C speed value");
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
			    u8_t daddr)
{
	/* Set slave address. */
	twihs->TWIHS_MMR = TWIHS_MMR_DADR(daddr);

	/* Write first data byte on I2C bus */
	twihs->TWIHS_THR = msg->buf[msg->idx++];

	/* Enable Transmit Ready and Transmission Completed interrupts */
	twihs->TWIHS_IER = TWIHS_IER_TXRDY | TWIHS_IER_TXCOMP | TWIHS_IER_NACK;
}

static void read_msg_start(Twihs *const twihs, struct twihs_msg *msg,
			   u8_t daddr)
{
	u32_t twihs_cr_stop;

	/* Set slave address and number of internal address bytes */
	twihs->TWIHS_MMR = TWIHS_MMR_MREAD | TWIHS_MMR_DADR(daddr);

	/* Enable Receive Ready and Transmission Completed interrupts */
	twihs->TWIHS_IER = TWIHS_IER_RXRDY | TWIHS_IER_TXCOMP | TWIHS_IER_NACK;

	/* In single data byte read the START and STOP must both be set */
	twihs_cr_stop = (msg->len == 1) ? TWIHS_CR_STOP : 0;
	/* Start the transfer by sending START condition */
	twihs->TWIHS_CR = TWIHS_CR_START | twihs_cr_stop;
}

static int i2c_sam_twihs_transfer(struct device *dev, struct i2c_msg *msgs,
			      u8_t num_msgs, u16_t addr)
{
	const struct i2c_sam_twihs_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct i2c_sam_twihs_dev_data *const dev_data = DEV_DATA(dev);
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
		dev_data->msg.idx = 0;
		dev_data->msg.twihs_sr = 0;
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

static void i2c_sam_twihs_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct i2c_sam_twihs_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct i2c_sam_twihs_dev_data *const dev_data = DEV_DATA(dev);
	Twihs *const twihs = dev_cfg->regs;
	struct twihs_msg *msg = &dev_data->msg;
	u32_t isr_status;

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

		if (msg->idx == msg->len - 1) {
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

static int i2c_sam_twihs_initialize(struct device *dev)
{
	const struct i2c_sam_twihs_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct i2c_sam_twihs_dev_data *const dev_data = DEV_DATA(dev);
	Twihs *const twihs = dev_cfg->regs;
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

	/* Reset the module */
	twihs->TWIHS_CR = TWIHS_CR_SWRST;

	bitrate_cfg = _i2c_map_dt_bitrate(dev_cfg->bitrate);

	ret = i2c_sam_twihs_configure(dev, I2C_MODE_MASTER | bitrate_cfg);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to initialize %s device", DEV_NAME(dev));
		return ret;
	}

	/* Enable module's IRQ */
	irq_enable(dev_cfg->irq_id);

	SYS_LOG_INF("Device %s initialized", DEV_NAME(dev));

	return 0;
}

static const struct i2c_driver_api i2c_sam_twihs_driver_api = {
	.configure = i2c_sam_twihs_configure,
	.transfer = i2c_sam_twihs_transfer,
};

/* I2C0 */

#ifdef CONFIG_I2C_0
static struct device DEVICE_NAME_GET(i2c0_sam);

static void i2c0_sam_irq_config(void)
{
	IRQ_CONNECT(CONFIG_I2C_0_IRQ, CONFIG_I2C_0_IRQ_PRI, i2c_sam_twihs_isr,
		    DEVICE_GET(i2c0_sam), 0);
}

static const struct soc_gpio_pin pins_twihs0[] = PINS_TWIHS0;

static const struct i2c_sam_twihs_dev_cfg i2c0_sam_config = {
	.regs = (Twihs *)CONFIG_I2C_0_BASE_ADDRESS,
	.irq_config = i2c0_sam_irq_config,
	.periph_id = CONFIG_I2C_0_PERIPHERAL_ID,
	.irq_id = CONFIG_I2C_0_IRQ,
	.pin_list = pins_twihs0,
	.pin_list_size = ARRAY_SIZE(pins_twihs0),
	.bitrate = CONFIG_I2C_0_BITRATE,
};

static struct i2c_sam_twihs_dev_data i2c0_sam_data;

DEVICE_AND_API_INIT(i2c0_sam, CONFIG_I2C_0_NAME, &i2c_sam_twihs_initialize,
		    &i2c0_sam_data, &i2c0_sam_config, POST_KERNEL,
		    CONFIG_I2C_INIT_PRIORITY, &i2c_sam_twihs_driver_api);
#endif

/* I2C1 */

#ifdef CONFIG_I2C_1
static struct device DEVICE_NAME_GET(i2c1_sam);

static void i2c1_sam_irq_config(void)
{
	IRQ_CONNECT(CONFIG_I2C_1_IRQ, CONFIG_I2C_1_IRQ_PRI, i2c_sam_twihs_isr,
		    DEVICE_GET(i2c1_sam), 0);
}

static const struct soc_gpio_pin pins_twihs1[] = PINS_TWIHS1;

static const struct i2c_sam_twihs_dev_cfg i2c1_sam_config = {
	.regs = (Twihs *)CONFIG_I2C_1_BASE_ADDRESS,
	.irq_config = i2c1_sam_irq_config,
	.periph_id = CONFIG_I2C_1_PERIPHERAL_ID,
	.irq_id = CONFIG_I2C_1_IRQ,
	.pin_list = pins_twihs1,
	.pin_list_size = ARRAY_SIZE(pins_twihs1),
	.bitrate = CONFIG_I2C_1_BITRATE,
};

static struct i2c_sam_twihs_dev_data i2c1_sam_data;

DEVICE_AND_API_INIT(i2c1_sam, CONFIG_I2C_1_NAME, &i2c_sam_twihs_initialize,
		    &i2c1_sam_data, &i2c1_sam_config, POST_KERNEL,
		    CONFIG_I2C_INIT_PRIORITY, &i2c_sam_twihs_driver_api);
#endif

/* I2C2 */

#ifdef CONFIG_I2C_2
static struct device DEVICE_NAME_GET(i2c2_sam);

static void i2c2_sam_irq_config(void)
{
	IRQ_CONNECT(CONFIG_I2C_2_IRQ, CONFIG_I2C_2_IRQ_PRI, i2c_sam_twihs_isr,
		    DEVICE_GET(i2c2_sam), 0);
}

static const struct soc_gpio_pin pins_twihs2[] = PINS_TWIHS2;

static const struct i2c_sam_twihs_dev_cfg i2c2_sam_config = {
	.regs = (Twihs *)CONFIG_I2C_2_BASE_ADDRESS,
	.irq_config = i2c2_sam_irq_config,
	.periph_id = CONFIG_I2C_2_PERIPHERAL_ID,
	.irq_id = CONFIG_I2C_2_IRQ,
	.pin_list = pins_twihs2,
	.pin_list_size = ARRAY_SIZE(pins_twihs2),
	.bitrate = CONFIG_I2C_2_BITRATE,
};

static struct i2c_sam_twihs_dev_data i2c2_sam_data;

DEVICE_AND_API_INIT(i2c2_sam, CONFIG_I2C_2_NAME, &i2c_sam_twihs_initialize,
		    &i2c2_sam_data, &i2c2_sam_config, POST_KERNEL,
		    CONFIG_I2C_INIT_PRIORITY, &i2c_sam_twihs_driver_api);
#endif
