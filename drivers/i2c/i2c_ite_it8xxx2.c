/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_i2c

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <ilm.h>
#include <soc.h>
#include <soc_dt.h>
#include <zephyr/dt-bindings/i2c/it8xxx2-i2c.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ite_it8xxx2, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

/* Start smbus session from idle state */
#define I2C_MSG_START BIT(5)

#define I2C_LINE_SCL_HIGH BIT(0)
#define I2C_LINE_SDA_HIGH BIT(1)
#define I2C_LINE_IDLE (I2C_LINE_SCL_HIGH | I2C_LINE_SDA_HIGH)

#ifdef CONFIG_I2C_IT8XXX2_FIFO_MODE
#define I2C_FIFO_MODE_MAX_SIZE 32
#define I2C_FIFO_MODE_TOTAL_LEN 255
#define I2C_MSG_BURST_READ_MASK (I2C_MSG_RESTART | I2C_MSG_STOP | I2C_MSG_READ)
#endif

struct i2c_it8xxx2_config {
	void (*irq_config_func)(void);
	uint32_t bitrate;
	uint8_t *base;
	uint8_t *reg_mstfctrl;
	uint8_t i2c_irq_base;
	uint8_t port;
	/* SCL GPIO cells */
	struct gpio_dt_spec scl_gpios;
	/* SDA GPIO cells */
	struct gpio_dt_spec sda_gpios;
	/* I2C alternate configuration */
	const struct pinctrl_dev_config *pcfg;
	uint32_t clock_gate_offset;
	bool fifo_enable;
};

enum i2c_pin_fun {
	SCL = 0,
	SDA,
};

enum i2c_ch_status {
	I2C_CH_NORMAL = 0,
	I2C_CH_REPEAT_START,
	I2C_CH_WAIT_READ,
	I2C_CH_WAIT_NEXT_XFER,
};

struct i2c_it8xxx2_data {
	enum i2c_ch_status i2ccs;
	struct i2c_msg *active_msg;
	struct k_mutex mutex;
	struct k_sem device_sync_sem;
#ifdef CONFIG_I2C_IT8XXX2_FIFO_MODE
	struct i2c_msg *msgs_list;
	/* Read or write byte counts. */
	uint32_t bytecnt;
	/* Number of messages. */
	uint8_t num_msgs;
	uint8_t active_msg_index;
#endif
	/* Index into output data */
	size_t widx;
	/* Index into input data */
	size_t ridx;
	/* operation freq of i2c */
	uint32_t bus_freq;
	/* Error code, if any */
	uint32_t err;
	/* address of device */
	uint16_t addr_16bit;
	/* Frequency setting */
	uint8_t freq;
	/* wait for stop bit interrupt */
	uint8_t stop;
};

enum i2c_host_status {
	/* Host busy */
	HOSTA_HOBY = 0x01,
	/* Finish Interrupt */
	HOSTA_FINTR = 0x02,
	/* Device error */
	HOSTA_DVER = 0x04,
	/* Bus error */
	HOSTA_BSER = 0x08,
	/* Fail */
	HOSTA_FAIL = 0x10,
	/* Not response ACK */
	HOSTA_NACK = 0x20,
	/* Time-out error */
	HOSTA_TMOE = 0x40,
	/* Byte done status */
	HOSTA_BDS = 0x80,
	/* Error bit is set */
	HOSTA_ANY_ERROR = (HOSTA_DVER | HOSTA_BSER | HOSTA_FAIL |
			   HOSTA_NACK | HOSTA_TMOE),
	/* W/C for next byte */
	HOSTA_NEXT_BYTE = HOSTA_BDS,
	/* W/C host status register */
	HOSTA_ALL_WC_BIT = (HOSTA_FINTR | HOSTA_ANY_ERROR | HOSTA_BDS),
};

enum i2c_reset_cause {
	I2C_RC_NO_IDLE_FOR_START = 1,
	I2C_RC_TIMEOUT,
};

static int i2c_parsing_return_value(const struct device *dev)
{
	const struct i2c_it8xxx2_config *config = dev->config;
	struct i2c_it8xxx2_data *data = dev->data;

	if (!data->err) {
		return 0;
	}

	if (data->err == ETIMEDOUT) {
		/* Connection timed out */
		LOG_ERR("I2C ch%d Address:0x%X Transaction time out.",
			config->port, data->addr_16bit);
	} else {
		LOG_DBG("I2C ch%d Address:0x%X Host error bits message:",
			config->port, data->addr_16bit);
		/* Host error bits message*/
		if (data->err & HOSTA_TMOE) {
			LOG_ERR("Time-out error: hardware time-out error.");
		}
		if (data->err & HOSTA_NACK) {
			LOG_DBG("NACK error: device does not response ACK.");
		}
		if (data->err & HOSTA_FAIL) {
			LOG_ERR("Fail: a processing transmission is killed.");
		}
		if (data->err & HOSTA_BSER) {
			LOG_ERR("BUS error: SMBus has lost arbitration.");
		}
	}

	return -EIO;
}

static int i2c_get_line_levels(const struct device *dev)
{
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;

	return (IT8XXX2_SMB_SMBPCTL(base) &
		(IT8XXX2_SMB_SMBDCS | IT8XXX2_SMB_SMBCS));
}

static int i2c_is_busy(const struct device *dev)
{
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;

	return (IT8XXX2_SMB_HOSTA(base) &
		(HOSTA_HOBY | HOSTA_ALL_WC_BIT));
}

static int i2c_bus_not_available(const struct device *dev)
{
	if (i2c_is_busy(dev) ||
		(i2c_get_line_levels(dev) != I2C_LINE_IDLE)) {
		return -EIO;
	}

	return 0;
}

static void i2c_reset(const struct device *dev)
{
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;

	/* bit1, kill current transaction. */
	IT8XXX2_SMB_HOCTL(base) = IT8XXX2_SMB_KILL;
	IT8XXX2_SMB_HOCTL(base) = 0;
	/* W/C host status register */
	IT8XXX2_SMB_HOSTA(base) = HOSTA_ALL_WC_BIT;
}

/*
 * Set i2c standard port (A, B, or C) runs at 400kHz by using timing registers
 * (offset 0h ~ 7h).
 */
static void i2c_standard_port_timing_regs_400khz(uint8_t port)
{
	/* Port clock frequency depends on setting of timing registers. */
	IT8XXX2_SMB_SCLKTS(port) = 0;
	/* Suggested setting of timing registers of 400kHz. */
	IT8XXX2_SMB_4P7USL = 0x3;
	IT8XXX2_SMB_4P0USL = 0;
	IT8XXX2_SMB_300NS = 0x1;
	IT8XXX2_SMB_250NS = 0x5;
	IT8XXX2_SMB_45P3USL = 0x6a;
	IT8XXX2_SMB_45P3USH = 0x1;
	IT8XXX2_SMB_4P7A4P0H = 0;
}

/* Set clock frequency for i2c port A, B , or C */
static void i2c_standard_port_set_frequency(const struct device *dev,
					    int freq_hz, int freq_set)
{
	const struct i2c_it8xxx2_config *config = dev->config;

	/*
	 * If port's clock frequency is 400kHz, we use timing registers
	 * for setting. So we can adjust tlow to meet timing.
	 * The others use basic 50/100/1000 KHz setting.
	 */
	if (freq_hz == I2C_BITRATE_FAST) {
		i2c_standard_port_timing_regs_400khz(config->port);
	} else {
		IT8XXX2_SMB_SCLKTS(config->port) = freq_set;
	}

	/* This field defines the SMCLK0/1/2 clock/data low timeout. */
	IT8XXX2_SMB_25MS = I2C_CLK_LOW_TIMEOUT;
}

static int i2c_it8xxx2_configure(const struct device *dev,
				 uint32_t dev_config_raw)
{
	const struct i2c_it8xxx2_config *config = dev->config;
	struct i2c_it8xxx2_data *const data = dev->data;
	uint32_t freq_set;

	if (!(I2C_MODE_CONTROLLER & dev_config_raw)) {
		return -EINVAL;
	}

	if (I2C_ADDR_10_BITS & dev_config_raw) {
		return -EINVAL;
	}

	data->bus_freq = I2C_SPEED_GET(dev_config_raw);

	switch (data->bus_freq) {
	case I2C_SPEED_DT:
		freq_set = IT8XXX2_SMB_SMCLKS_50K;
		break;
	case I2C_SPEED_STANDARD:
		freq_set = IT8XXX2_SMB_SMCLKS_100K;
		break;
	case I2C_SPEED_FAST:
		freq_set = IT8XXX2_SMB_SMCLKS_400K;
		break;
	case I2C_SPEED_FAST_PLUS:
		freq_set = IT8XXX2_SMB_SMCLKS_1M;
		break;
	default:
		return -EINVAL;
	}

	i2c_standard_port_set_frequency(dev, config->bitrate, freq_set);

	return 0;
}

static int i2c_it8xxx2_get_config(const struct device *dev,
				  uint32_t *dev_config)
{
	struct i2c_it8xxx2_data *const data = dev->data;
	uint32_t speed;

	if (!data->bus_freq) {
		LOG_ERR("The bus frequency is not initially configured.");
		return -EIO;
	}

	switch (data->bus_freq) {
	case I2C_SPEED_DT:
	case I2C_SPEED_STANDARD:
	case I2C_SPEED_FAST:
	case I2C_SPEED_FAST_PLUS:
		speed = I2C_SPEED_SET(data->bus_freq);
		break;
	default:
		return -ERANGE;
	}

	*dev_config = (I2C_MODE_CONTROLLER | speed);

	return 0;
}

void __soc_ram_code i2c_r_last_byte(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;

	/*
	 * bit5, The firmware shall write 1 to this bit
	 * when the next byte will be the last byte for i2c read.
	 */
	if ((data->active_msg->flags & I2C_MSG_STOP) &&
	    (data->ridx == data->active_msg->len - 1)) {
		IT8XXX2_SMB_HOCTL(base) |= IT8XXX2_SMB_LABY;
	}
}

void __soc_ram_code i2c_w2r_change_direction(const struct device *dev)
{
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;

	/* I2C switch direction */
	if (IT8XXX2_SMB_HOCTL2(base) & IT8XXX2_SMB_I2C_SW_EN) {
		i2c_r_last_byte(dev);
		IT8XXX2_SMB_HOSTA(base) = HOSTA_NEXT_BYTE;
	} else {
		/*
		 * bit2, I2C switch direction wait.
		 * bit3, I2C switch direction enable.
		 */
		IT8XXX2_SMB_HOCTL2(base) |= IT8XXX2_SMB_I2C_SW_EN |
					    IT8XXX2_SMB_I2C_SW_WAIT;
		IT8XXX2_SMB_HOSTA(base) = HOSTA_NEXT_BYTE;
		i2c_r_last_byte(dev);
		IT8XXX2_SMB_HOCTL2(base) &= ~IT8XXX2_SMB_I2C_SW_WAIT;
	}
}

#ifdef CONFIG_I2C_IT8XXX2_FIFO_MODE
void __soc_ram_code i2c_fifo_en_w2r(const struct device *dev, bool enable)
{
	const struct i2c_it8xxx2_config *config = dev->config;
	unsigned int key = irq_lock();

	if (enable) {
		if (config->port == SMB_CHANNEL_A) {
			IT8XXX2_SMB_I2CW2RF |= IT8XXX2_SMB_MAIF |
					       IT8XXX2_SMB_MAIFI;
		} else if (config->port == SMB_CHANNEL_B) {
			IT8XXX2_SMB_I2CW2RF |= IT8XXX2_SMB_MBCIF |
					       IT8XXX2_SMB_MBIFI;
		} else if (config->port == SMB_CHANNEL_C) {
			IT8XXX2_SMB_I2CW2RF |= IT8XXX2_SMB_MBCIF |
					       IT8XXX2_SMB_MCIFI;
		}
	} else {
		if (config->port == SMB_CHANNEL_A) {
			IT8XXX2_SMB_I2CW2RF &= ~(IT8XXX2_SMB_MAIF |
						 IT8XXX2_SMB_MAIFI);
		} else if (config->port == SMB_CHANNEL_B) {
			IT8XXX2_SMB_I2CW2RF &= ~(IT8XXX2_SMB_MBCIF |
						 IT8XXX2_SMB_MBIFI);
		} else if (config->port == SMB_CHANNEL_C) {
			IT8XXX2_SMB_I2CW2RF &= ~(IT8XXX2_SMB_MBCIF |
						 IT8XXX2_SMB_MCIFI);
		}
	}

	irq_unlock(key);
}

void __soc_ram_code i2c_tran_fifo_write_start(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	uint32_t i;
	uint8_t *base = config->base;
	volatile uint8_t *reg_mstfctrl = config->reg_mstfctrl;

	/* Clear start flag. */
	data->active_msg->flags &= ~I2C_MSG_START;
	/* Enable SMB channel in FIFO mode. */
	*reg_mstfctrl |= IT8XXX2_SMB_FFEN;
	/* I2C enable. */
	IT8XXX2_SMB_HOCTL2(base) = IT8XXX2_SMB_SMD_TO_EN |
				   IT8XXX2_SMB_I2C_EN |
				   IT8XXX2_SMB_SMHEN;
	/* Set write byte counts. */
	IT8XXX2_SMB_D0REG(base) = data->active_msg->len;
	/*
	 * bit[7:1]: Address of the target.
	 * bit[0]: Direction of the host transfer.
	 */
	IT8XXX2_SMB_TRASLA(base) = (uint8_t)data->addr_16bit << 1;
	/* The maximum fifo size is 32 bytes. */
	data->bytecnt = MIN(data->active_msg->len, I2C_FIFO_MODE_MAX_SIZE);

	for (i = 0; i < data->bytecnt; i++) {
		/* Set host block data byte. */
		IT8XXX2_SMB_HOBDB(base) = *(data->active_msg->buf++);
	}
	/* Calculate the remaining byte counts. */
	data->bytecnt = data->active_msg->len - data->bytecnt;
	/*
	 * bit[6] = 1b: Start.
	 * bit[4:2] = 111b: Extend command.
	 * bit[0] = 1b: Host interrupt enable.
	 */
	IT8XXX2_SMB_HOCTL(base) = IT8XXX2_SMB_SRT |
				  IT8XXX2_SMB_SMCD_EXTND |
				  IT8XXX2_SMB_INTREN;
}

void __soc_ram_code i2c_tran_fifo_write_next_block(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	uint32_t i, _bytecnt;
	uint8_t *base = config->base;
	volatile uint8_t *reg_mstfctrl = config->reg_mstfctrl;

	/* The maximum fifo size is 32 bytes. */
	_bytecnt = MIN(data->bytecnt, I2C_FIFO_MODE_MAX_SIZE);

	for (i = 0; i < _bytecnt; i++) {
		/* Set host block data byte. */
		IT8XXX2_SMB_HOBDB(base) = *(data->active_msg->buf++);
	}
	/* Clear FIFO block done status. */
	*reg_mstfctrl |= IT8XXX2_SMB_BLKDS;
	/* Calculate the remaining byte counts. */
	data->bytecnt -= _bytecnt;
}

void __soc_ram_code i2c_tran_fifo_write_finish(const struct device *dev)
{
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;

	/* Clear byte count register. */
	IT8XXX2_SMB_D0REG(base) = 0;
	/* W/C */
	IT8XXX2_SMB_HOSTA(base) = HOSTA_ALL_WC_BIT;
	/* Disable the SMBus host interface. */
	IT8XXX2_SMB_HOCTL2(base) = 0x00;
}

int __soc_ram_code i2c_tran_fifo_w2r_change_direction(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;

	if (++data->active_msg_index >= data->num_msgs) {
		LOG_ERR("Current message index is error.");
		data->err = EINVAL;
		/* W/C */
		IT8XXX2_SMB_HOSTA(base) = HOSTA_ALL_WC_BIT;
		/* Disable the SMBus host interface. */
		IT8XXX2_SMB_HOCTL2(base) = 0x00;

		return 0;
	}
	/* Set I2C_SW_EN = 1 */
	IT8XXX2_SMB_HOCTL2(base) |= IT8XXX2_SMB_I2C_SW_EN |
				    IT8XXX2_SMB_I2C_SW_WAIT;
	IT8XXX2_SMB_HOCTL2(base) &= ~IT8XXX2_SMB_I2C_SW_WAIT;
	/* Point to the next msg for the read location. */
	data->active_msg = &data->msgs_list[data->active_msg_index];
	/* Set read byte counts. */
	IT8XXX2_SMB_D0REG(base) = data->active_msg->len;
	data->bytecnt = data->active_msg->len;
	/* W/C I2C W2R FIFO interrupt status. */
	IT8XXX2_SMB_IWRFISTA = BIT(config->port);

	return 1;
}

void __soc_ram_code i2c_tran_fifo_read_start(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;
	volatile uint8_t *reg_mstfctrl = config->reg_mstfctrl;

	/* Clear start flag. */
	data->active_msg->flags &= ~I2C_MSG_START;
	/* Enable SMB channel in FIFO mode. */
	*reg_mstfctrl |= IT8XXX2_SMB_FFEN;
	/* I2C enable. */
	IT8XXX2_SMB_HOCTL2(base) = IT8XXX2_SMB_SMD_TO_EN |
				   IT8XXX2_SMB_I2C_EN |
				   IT8XXX2_SMB_SMHEN;
	/* Set read byte counts. */
	IT8XXX2_SMB_D0REG(base) = data->active_msg->len;
	/*
	 * bit[7:1]: Address of the target.
	 * bit[0]: Direction of the host transfer.
	 */
	IT8XXX2_SMB_TRASLA(base) = (uint8_t)(data->addr_16bit << 1) |
				   IT8XXX2_SMB_DIR;

	data->bytecnt = data->active_msg->len;
	/*
	 * bit[6] = 1b: Start.
	 * bit[4:2] = 111b: Extend command.
	 * bit[0] = 1b: Host interrupt enable.
	 */
	IT8XXX2_SMB_HOCTL(base) = IT8XXX2_SMB_SRT |
				  IT8XXX2_SMB_SMCD_EXTND |
				  IT8XXX2_SMB_INTREN;
}

void __soc_ram_code i2c_tran_fifo_read_next_block(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	uint32_t i;
	uint8_t *base = config->base;
	volatile uint8_t *reg_mstfctrl = config->reg_mstfctrl;

	for (i = 0; i < I2C_FIFO_MODE_MAX_SIZE; i++) {
		/* To get received data. */
		*(data->active_msg->buf++) = IT8XXX2_SMB_HOBDB(base);
	}
	/* Clear FIFO block done status. */
	*reg_mstfctrl |= IT8XXX2_SMB_BLKDS;
	/* Calculate the remaining byte counts. */
	data->bytecnt -= I2C_FIFO_MODE_MAX_SIZE;
}

void __soc_ram_code i2c_tran_fifo_read_finish(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	uint32_t i;
	uint8_t *base = config->base;

	for (i = 0; i < data->bytecnt; i++) {
		/* To get received data. */
		*(data->active_msg->buf++) = IT8XXX2_SMB_HOBDB(base);
	}
	/* Clear byte count register. */
	IT8XXX2_SMB_D0REG(base) = 0;
	/* W/C */
	IT8XXX2_SMB_HOSTA(base) = HOSTA_ALL_WC_BIT;
	/* Disable the SMBus host interface. */
	IT8XXX2_SMB_HOCTL2(base) = 0x00;

}

int __soc_ram_code i2c_tran_fifo_write_to_read(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;
	volatile uint8_t *reg_mstfctrl = config->reg_mstfctrl;
	int ret = 1;

	if (data->active_msg->flags & I2C_MSG_START) {
		/* Enable I2C write to read FIFO mode. */
		i2c_fifo_en_w2r(dev, 1);
		i2c_tran_fifo_write_start(dev);
	} else {
		/* Check block done status. */
		if (*reg_mstfctrl & IT8XXX2_SMB_BLKDS) {
			if (IT8XXX2_SMB_HOCTL2(base) & IT8XXX2_SMB_I2C_SW_EN) {
				i2c_tran_fifo_read_next_block(dev);
			} else {
				i2c_tran_fifo_write_next_block(dev);
			}
		} else if (IT8XXX2_SMB_IWRFISTA & BIT(config->port)) {
			/*
			 * This function returns 0 on a failure to indicate
			 * that the current transaction is completed and
			 * returned the data->err.
			 */
			ret = i2c_tran_fifo_w2r_change_direction(dev);
		} else {
			/* Wait finish. */
			if ((IT8XXX2_SMB_HOSTA(base) & HOSTA_FINTR)) {
				i2c_tran_fifo_read_finish(dev);
				/* Done doing work. */
				ret = 0;
			}
		}
	}

	return ret;
}

int __soc_ram_code i2c_tran_fifo_read(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;
	volatile uint8_t *reg_mstfctrl = config->reg_mstfctrl;

	if (data->active_msg->flags & I2C_MSG_START) {
		i2c_tran_fifo_read_start(dev);
	} else {
		/* Check block done status. */
		if (*reg_mstfctrl & IT8XXX2_SMB_BLKDS) {
			i2c_tran_fifo_read_next_block(dev);
		} else {
			/* Wait finish. */
			if ((IT8XXX2_SMB_HOSTA(base) & HOSTA_FINTR)) {
				i2c_tran_fifo_read_finish(dev);
				/* Done doing work. */
				return 0;
			}
		}
	}

	return 1;
}

int __soc_ram_code i2c_tran_fifo_write(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;
	volatile uint8_t *reg_mstfctrl = config->reg_mstfctrl;

	if (data->active_msg->flags & I2C_MSG_START) {
		i2c_tran_fifo_write_start(dev);
	} else {
		/* Check block done status. */
		if (*reg_mstfctrl & IT8XXX2_SMB_BLKDS) {
			i2c_tran_fifo_write_next_block(dev);
		} else {
			/* Wait finish. */
			if ((IT8XXX2_SMB_HOSTA(base) & HOSTA_FINTR)) {
				i2c_tran_fifo_write_finish(dev);
				/* Done doing work. */
				return 0;
			}
		}
	}

	return 1;
}

int __soc_ram_code i2c_fifo_transaction(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;

	/* Any error. */
	if (IT8XXX2_SMB_HOSTA(base) & HOSTA_ANY_ERROR) {
		data->err = (IT8XXX2_SMB_HOSTA(base) & HOSTA_ANY_ERROR);
	} else {
		if (data->num_msgs == 2) {
			return i2c_tran_fifo_write_to_read(dev);
		} else if (data->active_msg->flags & I2C_MSG_READ) {
			return i2c_tran_fifo_read(dev);
		} else {
			return i2c_tran_fifo_write(dev);
		}
	}
	/* W/C */
	IT8XXX2_SMB_HOSTA(base) = HOSTA_ALL_WC_BIT;
	/* Disable the SMBus host interface. */
	IT8XXX2_SMB_HOCTL2(base) = 0x00;

	return 0;
}

bool __soc_ram_code fifo_mode_allowed(const struct device *dev,
				      struct i2c_msg *msgs)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;

	/*
	 * If the transaction of write or read is divided into two
	 * transfers(not two messages), the FIFO mode does not support.
	 */
	if (data->i2ccs != I2C_CH_NORMAL) {
		return false;
	}
	/*
	 * FIFO2 only supports one channel of B or C. If the FIFO of
	 * channel is not enabled, it will select PIO mode.
	 */
	if (!config->fifo_enable) {
		return false;
	}
	/*
	 * When there is only one message, use the FIFO mode transfer
	 * directly.
	 * Transfer payload too long (>255 bytes), use PIO mode.
	 * Write or read of I2C target address without data, used by
	 * cmd_i2c_scan. Use PIO mode.
	 */
	if (data->num_msgs == 1 && (msgs[0].flags & I2C_MSG_STOP) &&
	    (msgs[0].len <= I2C_FIFO_MODE_TOTAL_LEN) && (msgs[0].len != 0)) {
		return true;
	}
	/*
	 * When there are two messages, we need to judge whether or not there
	 * is I2C_MSG_RESTART flag from the second message, and then decide to
	 * do the FIFO mode or PIO mode transfer.
	 */
	if (data->num_msgs == 2) {
		/*
		 * The first of two messages must be write.
		 * Transfer payload too long (>255 bytes), use PIO mode.
		 */
		if (((msgs[0].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) &&
		    (msgs[0].len <= I2C_FIFO_MODE_TOTAL_LEN)) {
			/*
			 * The transfer is i2c_burst_read().
			 *
			 * e.g. msg[0].flags = I2C_MSG_WRITE;
			 *      msg[1].flags = I2C_MSG_RESTART | I2C_MSG_READ |
			 *                     I2C_MSG_STOP;
			 */
			if ((msgs[1].flags == I2C_MSG_BURST_READ_MASK) &&
			    (msgs[1].len <= I2C_FIFO_MODE_TOTAL_LEN)) {
				return true;
			}
		}
	}

	return false;
}
#endif

int __soc_ram_code i2c_tran_read(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;

	if (data->active_msg->flags & I2C_MSG_START) {
		/* i2c enable */
		IT8XXX2_SMB_HOCTL2(base) = IT8XXX2_SMB_SMD_TO_EN |
					   IT8XXX2_SMB_I2C_EN |
					   IT8XXX2_SMB_SMHEN;
		/*
		 * bit0, Direction of the host transfer.
		 * bit[1:7}, Address of the targeted slave.
		 */
		IT8XXX2_SMB_TRASLA(base) = (uint8_t)(data->addr_16bit << 1) |
					   IT8XXX2_SMB_DIR;
		/* clear start flag */
		data->active_msg->flags &= ~I2C_MSG_START;
		/*
		 * bit0, Host interrupt enable.
		 * bit[2:4}, Extend command.
		 * bit5, The firmware shall write 1 to this bit
		 *       when the next byte will be the last byte.
		 * bit6, start.
		 */
		if ((data->active_msg->len == 1) &&
		    (data->active_msg->flags & I2C_MSG_STOP)) {
			IT8XXX2_SMB_HOCTL(base) = IT8XXX2_SMB_SRT |
						  IT8XXX2_SMB_LABY |
						  IT8XXX2_SMB_SMCD_EXTND |
						  IT8XXX2_SMB_INTREN;
		} else {
			IT8XXX2_SMB_HOCTL(base) = IT8XXX2_SMB_SRT |
						  IT8XXX2_SMB_SMCD_EXTND |
						  IT8XXX2_SMB_INTREN;
		}
	} else {
		if ((data->i2ccs == I2C_CH_REPEAT_START) ||
		    (data->i2ccs == I2C_CH_WAIT_READ)) {
			if (data->i2ccs == I2C_CH_REPEAT_START) {
				/* write to read */
				i2c_w2r_change_direction(dev);
			} else {
				/* For last byte */
				i2c_r_last_byte(dev);
				/* W/C for next byte */
				IT8XXX2_SMB_HOSTA(base) = HOSTA_NEXT_BYTE;
			}
			data->i2ccs = I2C_CH_NORMAL;
		} else if (IT8XXX2_SMB_HOSTA(base) & HOSTA_BDS) {
			if (data->ridx < data->active_msg->len) {
				/* To get received data. */
				*(data->active_msg->buf++) = IT8XXX2_SMB_HOBDB(base);
				data->ridx++;
				/* For last byte */
				i2c_r_last_byte(dev);
				/* done */
				if (data->ridx == data->active_msg->len) {
					data->active_msg->len = 0;
					if (data->active_msg->flags & I2C_MSG_STOP) {
						/* W/C for finish */
						IT8XXX2_SMB_HOSTA(base) =
						HOSTA_NEXT_BYTE;

						data->stop = 1;
					} else {
						data->i2ccs = I2C_CH_WAIT_READ;
						return 0;
					}
				} else {
					/* W/C for next byte */
					IT8XXX2_SMB_HOSTA(base) =
							HOSTA_NEXT_BYTE;
				}
			}
		}
	}
	return 1;

}

int __soc_ram_code i2c_tran_write(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;

	if (data->active_msg->flags & I2C_MSG_START) {
		/* i2c enable */
		IT8XXX2_SMB_HOCTL2(base) = IT8XXX2_SMB_SMD_TO_EN |
					   IT8XXX2_SMB_I2C_EN |
					   IT8XXX2_SMB_SMHEN;
		/*
		 * bit0, Direction of the host transfer.
		 * bit[1:7}, Address of the targeted slave.
		 */
		IT8XXX2_SMB_TRASLA(base) = (uint8_t)data->addr_16bit << 1;
		/* Send first byte */
		IT8XXX2_SMB_HOBDB(base) = *(data->active_msg->buf++);

		data->widx++;
		/* clear start flag */
		data->active_msg->flags &= ~I2C_MSG_START;
		/*
		 * bit0, Host interrupt enable.
		 * bit[2:4}, Extend command.
		 * bit6, start.
		 */
		IT8XXX2_SMB_HOCTL(base) = IT8XXX2_SMB_SRT |
					  IT8XXX2_SMB_SMCD_EXTND |
					  IT8XXX2_SMB_INTREN;
	} else {
		/* Host has completed the transmission of a byte */
		if (IT8XXX2_SMB_HOSTA(base) & HOSTA_BDS) {
			if (data->widx < data->active_msg->len) {
				/* Send next byte */
				IT8XXX2_SMB_HOBDB(base) = *(data->active_msg->buf++);

				data->widx++;
				/* W/C byte done for next byte */
				IT8XXX2_SMB_HOSTA(base) = HOSTA_NEXT_BYTE;

				if (data->i2ccs == I2C_CH_REPEAT_START) {
					data->i2ccs = I2C_CH_NORMAL;
				}
			} else {
				/* done */
				data->active_msg->len = 0;
				if (data->active_msg->flags & I2C_MSG_STOP) {
					/* set I2C_EN = 0 */
					IT8XXX2_SMB_HOCTL2(base) = IT8XXX2_SMB_SMD_TO_EN |
								   IT8XXX2_SMB_SMHEN;
					/* W/C byte done for finish */
					IT8XXX2_SMB_HOSTA(base) = HOSTA_NEXT_BYTE;

					data->stop = 1;
				} else {
					data->i2ccs = I2C_CH_REPEAT_START;
					return 0;
				}
			}
		}
	}
	return 1;

}

int __soc_ram_code i2c_pio_transaction(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;

	/* any error */
	if (IT8XXX2_SMB_HOSTA(base) & HOSTA_ANY_ERROR) {
		data->err = (IT8XXX2_SMB_HOSTA(base) & HOSTA_ANY_ERROR);
	} else {
		if (!data->stop) {
			/*
			 * The return value indicates if there is more data
			 * to be read or written. If the return value = 1,
			 * it means that the interrupt cannot be disable and
			 * continue to transmit data.
			 */
			if (data->active_msg->flags & I2C_MSG_READ) {
				return i2c_tran_read(dev);
			} else {
				return i2c_tran_write(dev);
			}
		}
		/* wait finish */
		if (!(IT8XXX2_SMB_HOSTA(base) & HOSTA_FINTR)) {
			return 1;
		}
	}
	/* W/C */
	IT8XXX2_SMB_HOSTA(base) = HOSTA_ALL_WC_BIT;
	/* disable the SMBus host interface */
	IT8XXX2_SMB_HOCTL2(base) = 0x00;

	data->stop = 0;
	/* done doing work */
	return 0;
}

static int i2c_it8xxx2_transfer(const struct device *dev, struct i2c_msg *msgs,
				uint8_t num_msgs, uint16_t addr)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	int res, ret;

	/* Lock mutex of i2c controller */
	k_mutex_lock(&data->mutex, K_FOREVER);
	/*
	 * If the transaction of write to read is divided into two
	 * transfers, the repeat start transfer uses this flag to
	 * exclude checking bus busy.
	 */
	if (data->i2ccs == I2C_CH_NORMAL) {
		struct i2c_msg *start_msg = &msgs[0];

		/* Make sure we're in a good state to start */
		if (i2c_bus_not_available(dev)) {
			/* Recovery I2C bus */
			i2c_recover_bus(dev);
			/*
			 * After resetting I2C bus, if I2C bus is not available
			 * (No external pull-up), drop the transaction.
			 */
			if (i2c_bus_not_available(dev)) {
				/* Unlock mutex of i2c controller */
				k_mutex_unlock(&data->mutex);
				return -EIO;
			}
		}

		start_msg->flags |= I2C_MSG_START;
	}
#ifdef CONFIG_I2C_IT8XXX2_FIFO_MODE
	/* Store num_msgs to data struct. */
	data->num_msgs = num_msgs;
	/* Store msgs to data struct. */
	data->msgs_list = msgs;
	bool fifo_mode_enable = fifo_mode_allowed(dev, msgs);

	if (fifo_mode_enable) {
		/* Block to enter power policy. */
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
#endif
	for (int i = 0; i < num_msgs; i++) {

		data->widx = 0;
		data->ridx = 0;
		data->err = 0;
		data->active_msg = &msgs[i];
		data->addr_16bit = addr;

#ifdef CONFIG_I2C_IT8XXX2_FIFO_MODE
		data->active_msg_index = 0;
		/*
		 * Start transaction.
		 * The return value indicates if the initial configuration
		 * of I2C transaction for read or write has been completed.
		 */
		if (fifo_mode_enable) {
			if (i2c_fifo_transaction(dev)) {
				/* Enable i2c interrupt */
				irq_enable(config->i2c_irq_base);
			}
		} else
#endif
		{
			if (i2c_pio_transaction(dev)) {
				/* Enable i2c interrupt */
				irq_enable(config->i2c_irq_base);
			}
		}
		/* Wait for the transfer to complete */
		/* TODO: the timeout should be adjustable */
		res = k_sem_take(&data->device_sync_sem, K_MSEC(100));
		/*
		 * The irq will be enabled at the condition of start or
		 * repeat start of I2C. If timeout occurs without being
		 * wake up during suspend(ex: interrupt is not fired),
		 * the irq should be disabled immediately.
		 */
		irq_disable(config->i2c_irq_base);
		/*
		 * The transaction is dropped on any error(timeout, NACK, fail,
		 * bus error, device error).
		 */
		if (data->err) {
			break;
		}

		if (res != 0) {
			data->err = ETIMEDOUT;
			/* reset i2c port */
			i2c_reset(dev);
			LOG_ERR("I2C ch%d:0x%X reset cause %d",
				config->port, data->addr_16bit, I2C_RC_TIMEOUT);
			/* If this message is sent fail, drop the transaction. */
			break;
		}

#ifdef CONFIG_I2C_IT8XXX2_FIFO_MODE
		/*
		 * In FIFO mode, messages are compressed into a single
		 * transaction.
		 */
		if (fifo_mode_enable) {
			break;
		}
#endif
	}
#ifdef CONFIG_I2C_IT8XXX2_FIFO_MODE
	if (fifo_mode_enable) {
		volatile uint8_t *reg_mstfctrl = config->reg_mstfctrl;

		/* Disable SMB channels in FIFO mode. */
		*reg_mstfctrl &= ~IT8XXX2_SMB_FFEN;
		/* Disable I2C write to read FIFO mode. */
		if (data->num_msgs == 2) {
			i2c_fifo_en_w2r(dev, 0);
		}
		/* Permit to enter power policy. */
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
#endif
	/* reset i2c channel status */
	if (data->err || (data->active_msg->flags & I2C_MSG_STOP)) {
		data->i2ccs = I2C_CH_NORMAL;
	}
	/* Save return value. */
	ret = i2c_parsing_return_value(dev);
	/* Unlock mutex of i2c controller */
	k_mutex_unlock(&data->mutex);

	return ret;
}

static void i2c_it8xxx2_isr(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;

#ifdef CONFIG_I2C_IT8XXX2_FIFO_MODE
	volatile uint8_t *reg_mstfctrl = config->reg_mstfctrl;

	/* If done doing work, wake up the task waiting for the transfer. */
	if (config->fifo_enable && (*reg_mstfctrl & IT8XXX2_SMB_FFEN)) {
		if (i2c_fifo_transaction(dev)) {
			return;
		}
	} else
#endif
	{
		if (i2c_pio_transaction(dev)) {
			return;
		}
	}
	irq_disable(config->i2c_irq_base);
	k_sem_give(&data->device_sync_sem);
}

static int i2c_it8xxx2_init(const struct device *dev)
{
	struct i2c_it8xxx2_data *data = dev->data;
	const struct i2c_it8xxx2_config *config = dev->config;
	uint8_t *base = config->base;
	uint32_t bitrate_cfg;
	int error, status;

	/* Initialize mutex and semaphore */
	k_mutex_init(&data->mutex);
	k_sem_init(&data->device_sync_sem, 0, K_SEM_MAX_LIMIT);

	/* Enable clock to specified peripheral */
	volatile uint8_t *reg = (volatile uint8_t *)
		(IT8XXX2_ECPM_BASE + (config->clock_gate_offset >> 8));
	uint8_t reg_mask = config->clock_gate_offset & 0xff;
	*reg &= ~reg_mask;

	/* Enable SMBus function */
	/*
	 * bit0, The SMBus host interface is enabled.
	 * bit1, Enable to communicate with I2C device
	 *		  and support I2C-compatible cycles.
	 * bit4, This bit controls the reset mechanism
	 *		  of SMBus master to handle the SMDAT
	 *		  line low if 25ms reg timeout.
	 */
	IT8XXX2_SMB_HOCTL2(base) = IT8XXX2_SMB_SMD_TO_EN | IT8XXX2_SMB_SMHEN;
	/*
	 * bit1, Kill SMBus host transaction.
	 * bit0, Enable the interrupt for the master interface.
	 */
	IT8XXX2_SMB_HOCTL(base) = IT8XXX2_SMB_KILL | IT8XXX2_SMB_SMHEN;
	IT8XXX2_SMB_HOCTL(base) = IT8XXX2_SMB_SMHEN;

	/* W/C host status register */
	IT8XXX2_SMB_HOSTA(base) = HOSTA_ALL_WC_BIT;
	IT8XXX2_SMB_HOCTL2(base) = 0x00;

#ifdef CONFIG_I2C_IT8XXX2_FIFO_MODE
	volatile uint8_t *reg_mstfctrl = config->reg_mstfctrl;

	if (config->port == SMB_CHANNEL_B && config->fifo_enable) {
		/* Select channel B in FIFO2. */
		*reg_mstfctrl = IT8XXX2_SMB_FFCHSEL2_B;
	} else if (config->port == SMB_CHANNEL_C && config->fifo_enable) {
		/* Select channel C in FIFO2. */
		*reg_mstfctrl = IT8XXX2_SMB_FFCHSEL2_C;
	}
#endif

	/* Set clock frequency for I2C ports */
	if (config->bitrate == I2C_BITRATE_STANDARD ||
		config->bitrate == I2C_BITRATE_FAST ||
		config->bitrate == I2C_BITRATE_FAST_PLUS) {
		bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);
	} else {
		/* Device tree specified speed */
		bitrate_cfg = I2C_SPEED_DT << I2C_SPEED_SHIFT;
	}

	error = i2c_it8xxx2_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	data->i2ccs = I2C_CH_NORMAL;

	if (error) {
		LOG_ERR("i2c: failure initializing");
		return error;
	}

	/* Set the pin to I2C alternate function. */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure I2C pins");
		return status;
	}

	return 0;
}

static int i2c_it8xxx2_recover_bus(const struct device *dev)
{
	const struct i2c_it8xxx2_config *config = dev->config;
	int i, status;

	/* Set SCL of I2C as GPIO pin */
	gpio_pin_configure_dt(&config->scl_gpios, GPIO_OUTPUT);
	/* Set SDA of I2C as GPIO pin */
	gpio_pin_configure_dt(&config->sda_gpios, GPIO_OUTPUT);

	/*
	 * In I2C recovery bus, 1ms sleep interval for bitbanging i2c
	 * is mainly to ensure that gpio has enough time to go from
	 * low to high or high to low.
	 */
	/* Pull SCL and SDA pin to high */
	gpio_pin_set_dt(&config->scl_gpios, 1);
	gpio_pin_set_dt(&config->sda_gpios, 1);
	k_msleep(1);

	/* Start condition */
	gpio_pin_set_dt(&config->sda_gpios, 0);
	k_msleep(1);
	gpio_pin_set_dt(&config->scl_gpios, 0);
	k_msleep(1);

	/* 9 cycles of SCL with SDA held high */
	for (i = 0; i < 9; i++) {
		/* SDA */
		gpio_pin_set_dt(&config->sda_gpios, 1);
		/* SCL */
		gpio_pin_set_dt(&config->scl_gpios, 1);
		k_msleep(1);
		/* SCL */
		gpio_pin_set_dt(&config->scl_gpios, 0);
		k_msleep(1);
	}
	/* SDA */
	gpio_pin_set_dt(&config->sda_gpios, 0);
	k_msleep(1);

	/* Stop condition */
	gpio_pin_set_dt(&config->scl_gpios, 1);
	k_msleep(1);
	gpio_pin_set_dt(&config->sda_gpios, 1);
	k_msleep(1);

	/* Set GPIO back to I2C alternate function of SCL */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure I2C pins");
		return status;
	}

	/* reset i2c port */
	i2c_reset(dev);
	LOG_ERR("I2C ch%d reset cause %d", config->port,
		I2C_RC_NO_IDLE_FOR_START);

	return 0;
}

static const struct i2c_driver_api i2c_it8xxx2_driver_api = {
	.configure = i2c_it8xxx2_configure,
	.get_config = i2c_it8xxx2_get_config,
	.transfer = i2c_it8xxx2_transfer,
	.recover_bus = i2c_it8xxx2_recover_bus,
};

#ifdef CONFIG_I2C_IT8XXX2_FIFO_MODE
/*
 * Sometimes, channel C may write wrong register to the target device.
 * This issue occurs when FIFO2 is enabled on channel C. The problem
 * arises because FIFO2 is shared between channel B and channel C.
 * FIFO2 will be disabled when data access is completed, at which point
 * FIFO2 is set to the default configuration for channel B.
 * The byte counter of FIFO2 may be affected by channel B. There is a chance
 * that channel C may encounter wrong register being written due to FIFO2
 * byte counter wrong write after channel B's write operation.
 */
BUILD_ASSERT((DT_INST_PROP(SMB_CHANNEL_C, fifo_enable) == false),
	     "Channel C cannot use FIFO mode.");
#endif

#define I2C_ITE_IT8XXX2_INIT(inst)                                              \
	PINCTRL_DT_INST_DEFINE(inst);                                           \
	BUILD_ASSERT((DT_INST_PROP(inst, clock_frequency) ==                    \
		     50000) ||                                                  \
		     (DT_INST_PROP(inst, clock_frequency) ==                    \
		     I2C_BITRATE_STANDARD) ||                                   \
		     (DT_INST_PROP(inst, clock_frequency) ==                    \
		     I2C_BITRATE_FAST) ||                                       \
		     (DT_INST_PROP(inst, clock_frequency) ==                    \
		     I2C_BITRATE_FAST_PLUS), "Not support I2C bit rate value"); \
	static void i2c_it8xxx2_config_func_##inst(void);                       \
										\
	static const struct i2c_it8xxx2_config i2c_it8xxx2_cfg_##inst = {       \
		.base = (uint8_t *)(DT_INST_REG_ADDR_BY_IDX(inst, 0)),          \
		.reg_mstfctrl = (uint8_t *)(DT_INST_REG_ADDR_BY_IDX(inst, 1)),  \
		.irq_config_func = i2c_it8xxx2_config_func_##inst,              \
		.bitrate = DT_INST_PROP(inst, clock_frequency),                 \
		.i2c_irq_base = DT_INST_IRQN(inst),                             \
		.port = DT_INST_PROP(inst, port_num),                           \
		.scl_gpios = GPIO_DT_SPEC_INST_GET(inst, scl_gpios),            \
		.sda_gpios = GPIO_DT_SPEC_INST_GET(inst, sda_gpios),            \
		.clock_gate_offset = DT_INST_PROP(inst, clock_gate_offset),     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                   \
		.fifo_enable = DT_INST_PROP(inst, fifo_enable),                 \
	};                                                                      \
										\
	static struct i2c_it8xxx2_data i2c_it8xxx2_data_##inst;                 \
										\
	I2C_DEVICE_DT_INST_DEFINE(inst, i2c_it8xxx2_init,                       \
				  NULL,                                         \
				  &i2c_it8xxx2_data_##inst,                     \
				  &i2c_it8xxx2_cfg_##inst,                      \
				  POST_KERNEL,                                  \
				  CONFIG_I2C_INIT_PRIORITY,                     \
				  &i2c_it8xxx2_driver_api);                     \
										\
	static void i2c_it8xxx2_config_func_##inst(void)                        \
	{                                                                       \
		IRQ_CONNECT(DT_INST_IRQN(inst),                                 \
			0,                                                      \
			i2c_it8xxx2_isr,                                        \
			DEVICE_DT_INST_GET(inst), 0);                           \
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_ITE_IT8XXX2_INIT)
