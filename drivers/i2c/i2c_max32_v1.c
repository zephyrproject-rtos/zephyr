/*
 * Copyright (c) 2023 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_i2c

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>

/**
 * @ingroup  i2c_registers
 * @defgroup I2C_CTRL I2C_CTRL
 * @brief    Control Register0.
 * @{
 */

#define MAX32_I2C_CTRL_EN       BIT(0)  /**< CTRL_EN Mask */
#define MAX32_I2C_CTRL_MST_MODE BIT(1)  /**< CTRL_MST_MODE Mask */
#define MAX32_I2C_CTRL_SCL_OUT  BIT(6)  /**< CTRL_SCL_OUT Mask */
#define MAX32_I2C_CTRL_SDA_OUT  BIT(7)  /**< CTRL_SDA_OUT Mask */
#define MAX32_I2C_CTRL_SCL      BIT(8)  /**< CTRL_SCL Mask */
#define MAX32_I2C_CTRL_SDA      BIT(9)  /**< CTRL_SDA Mask */
#define MAX32_I2C_CTRL_BB_MODE  BIT(10) /**< CTRL_BB_MODE Mask */

/**@} end of group I2C_CTRL_Register */

/**
 * @ingroup  i2c_registers
 * @defgroup I2C_STATUS I2C_STATUS
 * @brief    Status Register.
 * @{
 */

#define MAX32_I2C_STATUS_RX_EM   BIT(1) /**< STATUS_RX_EM Mask */
#define MAX32_I2C_STATUS_TX_FULL BIT(4) /**< STATUS_TX_FULL Mask */

/**@} end of group I2C_STATUS_Register */

/**
 * @ingroup  i2c_registers
 * @defgroup I2C_INTFL0 I2C_INTFL0
 * @brief    Interrupt Status Register.
 * @{
 */

#define MAX32_I2C_INTFL0_DONE          BIT(0)  /**< INTFL0_DONE Mask */
#define MAX32_I2C_INTFL0_RX_THD        BIT(4)  /**< INTFL0_RX_THD Mask */
#define MAX32_I2C_INTFL0_TX_THD        BIT(5)  /**< INTFL0_TX_THD Mask */
#define MAX32_I2C_INTFL0_STOP          BIT(6)  /**< INTFL0_STOP Mask */
#define MAX32_I2C_INTFL0_ARB_ERR       BIT(8)  /**< INTFL0_ARB_ERR Mask */
#define MAX32_I2C_INTFL0_TO_ERR        BIT(9)  /**< INTFL0_TO_ERR Mask */
#define MAX32_I2C_INTFL0_ADDR_NACK_ERR BIT(10) /**< INTFL0_ADDR_NACK_ERR Mask */
#define MAX32_I2C_INTFL0_DATA_ERR      BIT(11) /**< INTFL0_DATA_ERR Mask */
#define MAX32_I2C_INTFL0_DNR_ERR       BIT(12) /**< INTFL0_DNR_ERR Mask */
#define MAX32_I2C_INTFL0_START_ERR     BIT(13) /**< INTFL0_START_ERR Mask */
#define MAX32_I2C_INTFL0_STOP_ERR      BIT(14) /**< INTFL0_STOP_ERR Mask */

/**@} end of group I2C_INTFL0_Register */

/**
 * @ingroup  i2c_registers
 * @defgroup I2C_FIFOLEN I2C_FIFOLEN
 * @brief    FIFO Configuration Register.
 * @{
 */

#define MAX32_I2C_FIFOLEN_RX_DEPTH_POS 0             /**< FIFOLEN_RX_DEPTH Mask */
#define MAX32_I2C_FIFOLEN_RX_DEPTH     GENMASK(7, 0) /**< FIFOLEN_RX_DEPTH Mask */

#define MAX32_I2C_FIFOLEN_TX_DEPTH_POS 8              /**< FIFOLEN_TX_DEPTH Mask */
#define MAX32_I2C_FIFOLEN_TX_DEPTH     GENMASK(15, 8) /**< FIFOLEN_TX_DEPTH Mask */

/**@} end of group I2C_FIFOLEN_Register */

/**
 * @ingroup  i2c_registers
 * @defgroup I2C_RXCTRL0 I2C_RXCTRL0
 * @brief    Receive Control Register 0.
 * @{
 */

#define MAX32_I2C_RXCTRL0_FLUSH BIT(7) /**< RXCTRL0_FLUSH Mask */

#define MAX32_I2C_RXCTRL0_THD_LVL_POS 8              /**< RXCTRL0_THD_LVL Mask */
#define MAX32_I2C_RXCTRL0_THD_LVL     GENMASK(11, 8) /**< RXCTRL0_THD_LVL Mask */

/**@} end of group I2C_RXCTRL0_Register */

/**
 * @ingroup  i2c_registers
 * @defgroup I2C_TXCTRL0 I2C_TXCTRL0
 * @brief    Transmit Control Register 0.
 * @{
 */

#define MAX32_I2C_TXCTRL0_FLUSH BIT(7) /**< TXCTRL0_FLUSH Mask */

#define MAX32_I2C_TXCTRL0_THD_LVL_POS 8              /**< TXCTRL0_THD_LVL Mask */
#define MAX32_I2C_TXCTRL0_THD_LVL     GENMASK(11, 8) /**< TXCTRL0_THD_LVL Mask */

/**@} end of group I2C_TXCTRL0_Register */

/**
 * @ingroup  i2c_registers
 * @defgroup I2C_MSTCTRL I2C_MSTCTRL
 * @brief    Master Control Register.
 * @{
 */

#define MAX32_I2C_MSTCTRL_START   BIT(0) /**< MSTCTRL_START Mask */
#define MAX32_I2C_MSTCTRL_RESTART BIT(1) /**< MSTCTRL_RESTART Mask */
#define MAX32_I2C_MSTCTRL_STOP    BIT(2) /**< MSTCTRL_STOP Mask */

/**@} end of group I2C_MSTCTRL_Register */

/**
 * @ingroup  i2c_registers
 * @defgroup I2C_SLAVE I2C_SLAVE
 * @brief    Slave Address Register.
 * @{
 */

#define MAX32_I2C_ERROR                                                                            \
	(MAX32_I2C_INTFL0_ARB_ERR | MAX32_I2C_INTFL0_TO_ERR | MAX32_I2C_INTFL0_ADDR_NACK_ERR |     \
	 MAX32_I2C_INTFL0_DATA_ERR | MAX32_I2C_INTFL0_DNR_ERR | MAX32_I2C_INTFL0_START_ERR |       \
	 MAX32_I2C_INTFL0_STOP_ERR)

#define MAX32_I2C_INTFL0_MASK 0x00FFFFFF
#define MAX32_I2C_INTFL1_MASK 0x00000007

#define MAX_MSGS_NUMBER 2

#define RX_THRESHOLD 6
#define TX_THRESHOLD 2

#define I2C_ADDRESS_SHIFT 1

struct max32_i2c_regs_t {
	uint32_t ctrl;    /**< <tt>\b 0x00:</tt> I2C CTRL Register */
	uint32_t status;  /**< <tt>\b 0x04:</tt> I2C STATUS Register */
	uint32_t intfl0;  /**< <tt>\b 0x08:</tt> I2C INTFL0 Register */
	uint32_t inten0;  /**< <tt>\b 0x0C:</tt> I2C INTEN0 Register */
	uint32_t intfl1;  /**< <tt>\b 0x10:</tt> I2C INTFL1 Register */
	uint32_t inten1;  /**< <tt>\b 0x14:</tt> I2C INTEN1 Register */
	uint32_t fifolen; /**< <tt>\b 0x18:</tt> I2C FIFOLEN Register */
	uint32_t rxctrl0; /**< <tt>\b 0x1C:</tt> I2C RXCTRL0 Register */
	uint32_t rxctrl1; /**< <tt>\b 0x20:</tt> I2C RXCTRL1 Register */
	uint32_t txctrl0; /**< <tt>\b 0x24:</tt> I2C TXCTRL0 Register */
	uint32_t txctrl1; /**< <tt>\b 0x28:</tt> I2C TXCTRL1 Register */
	uint32_t fifo;    /**< <tt>\b 0x2C:</tt> I2C FIFO Register */
	uint32_t mstctrl; /**< <tt>\b 0x30:</tt> I2C MSTCTRL Register */
	uint32_t clklo;   /**< <tt>\b 0x34:</tt> I2C CLKLO Register */
	uint32_t clkhi;   /**< <tt>\b 0x38:</tt> I2C CLKHI Register */
	uint32_t hsclk;   /**< <tt>\b 0x3C:</tt> I2C HSCLK Register */
	uint32_t timeout; /**< <tt>\b 0x40:</tt> I2C TIMEOUT Register */
	uint32_t rsv_0x44;
	uint32_t dma;   /**< <tt>\b 0x48:</tt> I2C DMA Register */
	uint32_t slave; /**< <tt>\b 0x4C:</tt> I2C SLAVE Register */
};

/* Driver config */
struct i2c_max32_config {
	volatile struct max32_i2c_regs_t *i2c; /* Address of hardware registers */
	const struct pinctrl_dev_config *pctrl;
	const struct device *clock;
	uint32_t clock_bus;
	uint32_t clock_bit;
	uint32_t bitrate;
};

static void i2c_max32_clear_flags(const struct device *dev, unsigned int flags0,
				  unsigned int flags1)
{
	const struct i2c_max32_config *const cfg = dev->config;
	volatile struct max32_i2c_regs_t *i2c = cfg->i2c;

	i2c->intfl0 = flags0;
	i2c->intfl1 = flags1;
}

static void i2c_max32_clear_txfifo(const struct device *dev)
{
	const struct i2c_max32_config *const cfg = dev->config;
	volatile struct max32_i2c_regs_t *i2c = cfg->i2c;

	i2c->txctrl0 |= MAX32_I2C_TXCTRL0_FLUSH;
	while (i2c->txctrl0 & MAX32_I2C_TXCTRL0_FLUSH) {
		;
	}
}

static void i2c_max32_clear_rxfifo(const struct device *dev)
{
	const struct i2c_max32_config *const cfg = dev->config;
	volatile struct max32_i2c_regs_t *i2c = cfg->i2c;

	i2c->rxctrl0 |= MAX32_I2C_RXCTRL0_FLUSH;
	while (i2c->rxctrl0 & MAX32_I2C_RXCTRL0_FLUSH) {
		;
	}
}

static int i2c_max32_stop(const struct device *dev)
{
	const struct i2c_max32_config *const cfg = dev->config;
	volatile struct max32_i2c_regs_t *i2c = cfg->i2c;

	if (i2c == NULL) {
		return -ENXIO;
	}

	i2c->mstctrl |= MAX32_I2C_MSTCTRL_STOP;
	while (i2c->mstctrl & MAX32_I2C_MSTCTRL_STOP) {
		;
	}

	return 0;
}

static int i2c_max32_write_txfifo(const struct device *dev, unsigned char *bytes, unsigned int len)
{
	const struct i2c_max32_config *const cfg = dev->config;
	volatile struct max32_i2c_regs_t *i2c = cfg->i2c;
	unsigned int written = 0;

	if ((i2c == NULL) || (bytes == NULL)) {
		return -ENXIO;
	}

	while ((len > written) && (!(i2c->status & MAX32_I2C_STATUS_TX_FULL))) {
		i2c->fifo = bytes[written++];
	}

	return written;
}

static int i2c_max32_read_rxfifo(const struct device *dev, unsigned char *bytes, unsigned int len)
{
	const struct i2c_max32_config *const cfg = dev->config;
	volatile struct max32_i2c_regs_t *i2c = cfg->i2c;
	unsigned int read = 0;

	if ((i2c == NULL) || (bytes == NULL)) {
		return -ENXIO;
	}

	while ((len > read) && (!(i2c->status & MAX32_I2C_STATUS_RX_EM))) {
		bytes[read++] = i2c->fifo;
	}

	return read;
}

static int i2c_max32_configure(const struct device *dev, uint32_t dev_config)
{
	return 0;
}

static int i2c_max32_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			      uint16_t slave_address)
{
	const struct i2c_max32_config *const cfg = dev->config;
	volatile struct max32_i2c_regs_t *i2c = cfg->i2c;
	struct i2c_msg *current = msgs;
	unsigned int msg_cnt = 0;
	unsigned int written = 0;
	unsigned int read = 0;

	i2c_max32_clear_flags(dev, MAX32_I2C_INTFL0_MASK, MAX32_I2C_INTFL1_MASK);
	i2c_max32_clear_rxfifo(dev);
	i2c_max32_clear_txfifo(dev);

	i2c->inten0 = 0;
	i2c->inten1 = 0;

	/* load slave address before write */
	if (!(current->flags & I2C_MSG_READ)) {
		i2c->fifo = slave_address << I2C_ADDRESS_SHIFT;
		i2c->mstctrl |= MAX32_I2C_MSTCTRL_START;
	}

	/* write */
	while (msg_cnt < num_msgs && !(current->flags & I2C_MSG_READ)) {
		if (i2c->intfl0 & MAX32_I2C_INTFL0_TX_THD) {
			written +=
				i2c_max32_write_txfifo(dev, current->buf, current->len - written);
			i2c->intfl0 = MAX32_I2C_INTFL0_TX_THD;
		}

		if (i2c->intfl0 & MAX32_I2C_ERROR) {
			num_msgs = written;
			i2c_max32_stop(dev);
			return -EIO;
		}

		written = 0;
		msg_cnt++;

		if (current->flags & I2C_MSG_STOP) {
			break;
		}
		/*next message */
		current++;
	}

	i2c_max32_clear_flags(dev, MAX32_I2C_INTFL0_DONE | MAX32_I2C_INTFL0_RX_THD, 0);

	/* read */
	if (current->flags & I2C_MSG_READ) {
		i2c->rxctrl1 = current->len;

		i2c->mstctrl |= MAX32_I2C_MSTCTRL_RESTART;
		while (i2c->mstctrl & MAX32_I2C_MSTCTRL_RESTART) {
			;
		}

		i2c->fifo = (slave_address << I2C_ADDRESS_SHIFT) | I2C_MSG_READ;

		while (msg_cnt < num_msgs && (current->flags & I2C_MSG_READ)) {
			if (i2c->intfl0 & (MAX32_I2C_INTFL0_RX_THD | MAX32_I2C_INTFL0_DONE)) {
				read += i2c_max32_read_rxfifo(dev, current->buf,
							      current->len - read);
				i2c->intfl0 = MAX32_I2C_INTFL0_RX_THD;
			}

			if (i2c->intfl0 & MAX32_I2C_ERROR) {
				current[0].len = read;
				i2c_max32_stop(dev);
				return -EIO;
			}

			read = 0;
			msg_cnt++;

			if (current->flags & I2C_MSG_STOP) {
				break;
			}

			/* next message */
			current++;
		}
		current--;
	}

	i2c->mstctrl |= MAX32_I2C_MSTCTRL_STOP;
	while (!(i2c->intfl0 & MAX32_I2C_INTFL0_STOP)) {
		;
	}

	while (!(i2c->intfl0 & MAX32_I2C_INTFL0_DONE)) {
		;
	}

	i2c->intfl0 = MAX32_I2C_INTFL0_DONE | MAX32_I2C_INTFL0_STOP;
	if (i2c->intfl0 & MAX32_I2C_ERROR) {
		return -EIO;
	}

	return 0;
}

static struct i2c_driver_api api = {
	.configure = i2c_max32_configure,
	.transfer = i2c_max32_transfer,
};

static int i2c_max32_init(const struct device *dev)
{
	const struct i2c_max32_config *const cfg = dev->config;
	volatile struct max32_i2c_regs_t *i2c = cfg->i2c;
	uint32_t clkcfg;
	int ret;

	if (!device_is_ready(cfg->clock)) {
		return -ENODEV;
	}

	/* enable clock */
	clkcfg = (cfg->clock_bus << 16) | cfg->clock_bit;

	ret = clock_control_on(cfg->clock, (clock_control_subsys_t)clkcfg);
	if (ret) {
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pctrl, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	i2c->ctrl |= MAX32_I2C_CTRL_EN;

	int swBit = i2c->ctrl & MAX32_I2C_CTRL_BB_MODE;

	i2c->ctrl |= MAX32_I2C_CTRL_BB_MODE;

	i2c->clklo = 20;
	i2c->clkhi = 20;

	int retries = 16;
	for (int i = 0; i < retries; i++) {
		i2c->ctrl &= ~MAX32_I2C_CTRL_SCL_OUT;
		if (i2c->ctrl & MAX32_I2C_CTRL_SCL) {
			i2c->ctrl |= MAX32_I2C_CTRL_SCL_OUT | MAX32_I2C_CTRL_SDA_OUT;
			continue; /* Give up and try again */
		}

		i2c->ctrl &= ~MAX32_I2C_CTRL_SDA_OUT;
		if (i2c->ctrl & MAX32_I2C_CTRL_SDA) {
			i2c->ctrl |= MAX32_I2C_CTRL_SCL_OUT | MAX32_I2C_CTRL_SDA_OUT;
			continue; /* Give up and try again */
		}

		i2c->ctrl |= MAX32_I2C_CTRL_SDA_OUT;
		if (!(i2c->ctrl & MAX32_I2C_CTRL_SDA)) {
			i2c->ctrl |= MAX32_I2C_CTRL_SCL_OUT | MAX32_I2C_CTRL_SDA_OUT;
			continue; /* Give up and try again */
		}

		i2c->ctrl |= MAX32_I2C_CTRL_SCL_OUT;
		if (i2c->ctrl & MAX32_I2C_CTRL_SCL) {
			break;
		}
	}

	if (swBit == 0) {
		i2c->ctrl &= ~MAX32_I2C_CTRL_BB_MODE;
	}

	i2c->ctrl &= ~MAX32_I2C_CTRL_EN;
	i2c->ctrl |= MAX32_I2C_CTRL_EN;

	i2c->rxctrl0 |= MAX32_I2C_RXCTRL0_FLUSH;
	while (i2c->rxctrl0 & MAX32_I2C_RXCTRL0_FLUSH) {
		;
	}

	i2c->txctrl0 |= MAX32_I2C_TXCTRL0_FLUSH;
	while (i2c->txctrl0 & MAX32_I2C_TXCTRL0_FLUSH) {
		;
	}

	unsigned int txFIFOlen =
		(i2c->fifolen & MAX32_I2C_FIFOLEN_TX_DEPTH) >> MAX32_I2C_FIFOLEN_TX_DEPTH_POS;
	if (TX_THRESHOLD > txFIFOlen) {
		return -EINVAL;
	}

	i2c->txctrl0 = (i2c->txctrl0 & ~MAX32_I2C_TXCTRL0_THD_LVL) |
		       (TX_THRESHOLD << MAX32_I2C_TXCTRL0_THD_LVL_POS);

	unsigned int rxFIFOlen =
		(i2c->fifolen & MAX32_I2C_FIFOLEN_RX_DEPTH) >> MAX32_I2C_FIFOLEN_RX_DEPTH_POS;
	if (RX_THRESHOLD > rxFIFOlen) {
		return -EINVAL;
	}

	i2c->rxctrl0 = (i2c->rxctrl0 & ~MAX32_I2C_RXCTRL0_THD_LVL) |
		       (RX_THRESHOLD << MAX32_I2C_RXCTRL0_THD_LVL_POS);

	i2c->ctrl |= MAX32_I2C_CTRL_MST_MODE;

	return 0;
}

#define DEFINE_I2C_MAX32(_num)                                                                     \
	PINCTRL_DT_INST_DEFINE(_num);                                                              \
                                                                                                   \
	static const struct i2c_max32_config i2c_max32_dev_cfg_##_num = {                          \
		.i2c = (void *)DT_INST_REG_ADDR(_num),                                             \
		.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(_num),                                     \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(_num)),                                 \
		.clock_bus = DT_INST_CLOCKS_CELL(_num, offset),                                    \
		.clock_bit = DT_INST_CLOCKS_CELL(_num, bit),                                       \
		.bitrate = DT_INST_PROP(_num, clock_frequency),                                    \
	};                                                                                         \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(                                                                 \
		_num, i2c_max32_init, NULL, (struct max32_i2c_regs_t *)DT_INST_REG_ADDR(_num),     \
		&i2c_max32_dev_cfg_##_num, PRE_KERNEL_2, CONFIG_I2C_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_I2C_MAX32)
