/*
 * Copyright (c) 2017 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief I2C bus (TWIHS) driver for Atmel SAM MCU family.
 *
 * Only I2C Master Mode with 7 bit addressing is currently supported.
 */

#include <errno.h>
#include <misc/__assert.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <i2c.h>

#define SYS_LOG_DOMAIN "dev/twihs_sam"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_I2C_LEVEL
#include <logging/sys_log.h>

/** I2C bus speed [Hz] in Standard Mode */
#define BUS_SPEED_STANDARD_HZ         100000U
/** I2C bus speed [Hz] in Fast Mode */
#define BUS_SPEED_FAST_HZ             400000U
/** I2C bus speed [Hz] in High Speed Mode */
#define BUS_SPEED_HIGH_HZ            3400000U

/* Device constant configuration parameters */
struct twihs_sam_dev_cfg {
	Twihs *regs;
	void (*irq_config)(void);
	u8_t periph_id;
	u8_t irq_id;
	const struct soc_gpio_pin *pin_list;
	u8_t pin_list_size;
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
struct twihs_sam_dev_data {
	struct k_sem sem;
	union dev_config mode_config;
	struct twihs_msg msg;
};

#define DEV_CFG(dev) \
	((const struct twihs_sam_dev_cfg *const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct twihs_sam_dev_data *const)(dev)->driver_data)

static u32_t clk_div_calc(u32_t speed)
{
	u32_t ck_div = 0;
	u32_t cl_div;
	u32_t div_completed = 0;

	while (!div_completed) {
		cl_div =   ((SOC_ATMEL_SAM_MCK_FREQ_HZ / (2 * speed)) - 4)
			 / (1 << ck_div);

		if (cl_div <= 255) {
			div_completed = 1;
		} else {
			ck_div++;
		}
	}

	/* Set TWI clock duty cycle to 50% */
	return (ck_div << 16) | (cl_div << 8) | cl_div;
}

static int twihs_sam_configure(struct device *dev, u32_t config)
{
	const struct twihs_sam_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct twihs_sam_dev_data *const dev_data = DEV_DATA(dev);
	Twihs *const twihs = dev_cfg->regs;
	u32_t i2c_speed;
	u32_t clk;

	if (!(config & (I2C_MODE_MASTER | I2C_MODE_SLAVE_READ))) {
		SYS_LOG_ERR("Neither Master nor Slave I2C Mode is enabled");
		return -EIO;
	}

	if (config & I2C_MODE_SLAVE_READ) {
		SYS_LOG_ERR("I2C Slave Mode is currently not supported");
		SYS_LOG_ERR("Please submit a patch");
		return -EIO;
	}

	if (config & I2C_ADDR_10_BITS) {
		SYS_LOG_ERR("I2C 10-bit addressing is currently not supported");
		SYS_LOG_ERR("Please submit a patch");
		return -EIO;
	}

	dev_data->mode_config.raw = config;

	/* Configure clock */
	switch ((dev_data->mode_config.bits.speed)) {
	case I2C_SPEED_STANDARD:
		i2c_speed = BUS_SPEED_STANDARD_HZ;
		break;
	case I2C_SPEED_FAST:
		i2c_speed = BUS_SPEED_FAST_HZ;
		break;
	default:
		SYS_LOG_ERR("Unsupported I2C speed value");
		return -EIO;
	}

	clk = clk_div_calc(i2c_speed);
	twihs->TWIHS_CWGR = clk;

	/* Disable Slave Mode */
	twihs->TWIHS_CR = TWIHS_CR_SVDIS;

	/* Enable Master Mode */
	twihs->TWIHS_CR = TWIHS_CR_MSEN;

	return 0;
}

static void write_msg_start(Twihs *const twihs, struct twihs_msg *msg,
			    u8_t daddr)
{
	/* Set slave address and number of internal address bytes. */
	twihs->TWIHS_MMR = TWIHS_MMR_DADR(daddr);
	/* Set internal address bytes */
	twihs->TWIHS_IADR = 0;

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
	/* Set internal address bytes */
	twihs->TWIHS_IADR = 0;

	/* Enable Receive Ready and Transmission Completed interrupts */
	twihs->TWIHS_IER = TWIHS_IER_RXRDY | TWIHS_IER_TXCOMP | TWIHS_IER_NACK;

	/* In single data byte read the START and STOP must both be set */
	twihs_cr_stop = (msg->len == 1) ? TWIHS_CR_STOP : 0;
	/* Start the transfer by sending START condition */
	twihs->TWIHS_CR = TWIHS_CR_START | twihs_cr_stop;
}

static int twihs_sam_transfer(struct device *dev, struct i2c_msg *msgs,
			      u8_t num_msgs, u16_t addr)
{
	const struct twihs_sam_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct twihs_sam_dev_data *const dev_data = DEV_DATA(dev);
	Twihs *const twihs = dev_cfg->regs;

	__ASSERT_NO_MSG(msgs);
	if (!num_msgs) {
		return 0;
	}

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
			/* Something went wrong, send bus CLEAR command */
			twihs->TWIHS_CR = TWIHS_CR_CLEAR;
			return -EIO;
		}
	}

	return 0;
}

static void twihs_sam_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct twihs_sam_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct twihs_sam_dev_data *const dev_data = DEV_DATA(dev);
	Twihs *const twihs = dev_cfg->regs;
	struct twihs_msg *msg = &dev_data->msg;
	u32_t isr_status;

	/* Retrieve interrupt status */
	isr_status = twihs->TWIHS_SR & twihs->TWIHS_IMR;

	/* Not Acknowledged */
	if (isr_status & TWIHS_SR_NACK) {
		msg->twihs_sr = isr_status;
	}

	/* Byte received */
	if (isr_status & TWIHS_SR_RXRDY) {

		msg->buf[msg->idx++] = twihs->TWIHS_RHR;

		if (msg->idx == msg->len - 1) {
			/* Send a STOP condition on the TWI */
			twihs->TWIHS_CR = TWIHS_CR_STOP;
		}
	}

	/* Byte sent */
	if (isr_status & TWIHS_SR_TXRDY) {
		if (msg->idx == msg->len) {
			if (msg->flags & I2C_MSG_STOP) {
				/* Send a STOP condition on the TWI */
				twihs->TWIHS_CR = TWIHS_CR_STOP;
				/* Disable Transmit Ready interrupt */
				twihs->TWIHS_IDR = TWIHS_IDR_TXRDY;
			} else {
				/* Transfer completed */
				isr_status |= TWIHS_SR_TXCOMP;
			}
		} else {
			twihs->TWIHS_THR = msg->buf[msg->idx++];
		}
	}

	/* Transfer completed */
	if (isr_status & TWIHS_SR_TXCOMP) {
		/* Disable all enabled interrupts */
		twihs->TWIHS_IDR = twihs->TWIHS_IMR;
		/* All data transferred, nothing else to do */
		k_sem_give(&dev_data->sem);
	}
}

static int twihs_sam_initialize(struct device *dev)
{
	const struct twihs_sam_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct twihs_sam_dev_data *const dev_data = DEV_DATA(dev);
	Twihs *const twihs = dev_cfg->regs;
	int result;

	/* Configure interrupts */
	dev_cfg->irq_config();

	/* Initialize semaphore */
	k_sem_init(&dev_data->sem, 0, 1);

	/* Connect pins to the peripheral */
	soc_gpio_list_configure(dev_cfg->pin_list, dev_cfg->pin_list_size);

	/* Enable module's clock */
	soc_pmc_peripheral_enable(dev_cfg->periph_id);

	/* Reset TWI module */
	twihs->TWIHS_CR = TWIHS_CR_SWRST;

	result = twihs_sam_configure(dev, dev_data->mode_config.raw);
	if (result < 0) {
		return result;
	}

	/* Enable module's IRQ */
	irq_enable(dev_cfg->irq_id);

	return 0;
}

static const struct i2c_driver_api twihs_sam_driver_api = {
	.configure = twihs_sam_configure,
	.transfer = twihs_sam_transfer,
};

/* I2C0 */

#ifdef CONFIG_I2C_0
static struct device DEVICE_NAME_GET(i2c0_sam);

static void i2c0_sam_irq_config(void)
{
	IRQ_CONNECT(TWIHS0_IRQn, CONFIG_I2C_0_IRQ_PRI, twihs_sam_isr,
		    DEVICE_GET(i2c0_sam), 0);
}

static const struct soc_gpio_pin pins_twihs0[] = PINS_TWIHS0;

static const struct twihs_sam_dev_cfg i2c0_sam_config = {
	.regs = TWIHS0,
	.irq_config = i2c0_sam_irq_config,
	.periph_id = ID_TWIHS0,
	.irq_id = TWIHS0_IRQn,
	.pin_list = pins_twihs0,
	.pin_list_size = ARRAY_SIZE(pins_twihs0),
};

static struct twihs_sam_dev_data i2c0_sam_data = {
	.mode_config.raw = CONFIG_I2C_0_DEFAULT_CFG,
};

DEVICE_AND_API_INIT(i2c0_sam, CONFIG_I2C_0_NAME, &twihs_sam_initialize,
		    &i2c0_sam_data, &i2c0_sam_config, POST_KERNEL,
		    CONFIG_I2C_INIT_PRIORITY, &twihs_sam_driver_api);
#endif

/* I2C1 */

#ifdef CONFIG_I2C_1
static struct device DEVICE_NAME_GET(i2c1_sam);

static void i2c1_sam_irq_config(void)
{
	IRQ_CONNECT(TWIHS0_IRQn, CONFIG_I2C_1_IRQ_PRI, twihs_sam_isr,
		    DEVICE_GET(i2c1_sam), 0);
}

static const struct soc_gpio_pin pins_twihs1[] = PINS_TWIHS1;

static const struct twihs_sam_dev_cfg i2c1_sam_config = {
	.regs = TWIHS1,
	.irq_config = i2c1_sam_irq_config,
	.periph_id = ID_TWIHS1,
	.irq_id = TWIHS1_IRQn,
	.pin_list = pins_twihs1,
	.pin_list_size = ARRAY_SIZE(pins_twihs1),
};

static struct twihs_sam_dev_data i2c1_sam_data = {
	.mode_config.raw = CONFIG_I2C_1_DEFAULT_CFG,
};

DEVICE_AND_API_INIT(i2c1_sam, CONFIG_I2C_1_NAME, &twihs_sam_initialize,
		    &i2c1_sam_data, &i2c1_sam_config, POST_KERNEL,
		    CONFIG_I2C_INIT_PRIORITY, &twihs_sam_driver_api);
#endif

/* I2C2 */

#ifdef CONFIG_I2C_2
static struct device DEVICE_NAME_GET(i2c2_sam);

static void i2c2_sam_irq_config(void)
{
	IRQ_CONNECT(TWIHS2_IRQn, CONFIG_I2C_2_IRQ_PRI, twihs_sam_isr,
		    DEVICE_GET(i2c2_sam), 0);
}

static const struct soc_gpio_pin pins_twihs2[] = PINS_TWIHS2;

static const struct twihs_sam_dev_cfg i2c2_sam_config = {
	.regs = TWIHS2,
	.irq_config = i2c2_sam_irq_config,
	.periph_id = ID_TWIHS2,
	.irq_id = TWIHS2_IRQn,
	.pin_list = pins_twihs2,
	.pin_list_size = ARRAY_SIZE(pins_twihs2),
};

static struct twihs_sam_dev_data i2c2_sam_data = {
	.mode_config.raw = CONFIG_I2C_2_DEFAULT_CFG,
};

DEVICE_AND_API_INIT(i2c2_sam, CONFIG_I2C_2_NAME, &twihs_sam_initialize,
		    &i2c2_sam_data, &i2c2_sam_config, POST_KERNEL,
		    CONFIG_I2C_INIT_PRIORITY, &twihs_sam_driver_api);
#endif
