/*
 * Copyright (c) 2019 Derek Hageman <hageman@inthat.cloud>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam0_i2c

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_sam0, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

#ifndef SERCOM_I2CM_CTRLA_MODE_I2C_MASTER
#define SERCOM_I2CM_CTRLA_MODE_I2C_MASTER SERCOM_I2CM_CTRLA_MODE(5)
#endif

#if CONFIG_I2C_SAM0_TRANSFER_TIMEOUT
#define I2C_TRANSFER_TIMEOUT_MSEC K_MSEC(CONFIG_I2C_SAM0_TRANSFER_TIMEOUT)
#else
#define I2C_TRANSFER_TIMEOUT_MSEC K_FOREVER
#endif

struct i2c_sam0_dev_config {
	SercomI2cm *regs;
	const struct pinctrl_dev_config *pcfg;
	uint32_t bitrate;
#ifdef MCLK
	volatile uint32_t *mclk;
	uint32_t mclk_mask;
	uint16_t gclk_core_id;
#else
	uint32_t pm_apbcmask;
	uint16_t gclk_clkctrl_id;
#endif
	void (*irq_config_func)(const struct device *dev);

#ifdef CONFIG_I2C_SAM0_DMA_DRIVEN
	const struct device *dma_dev;
	uint8_t write_dma_request;
	uint8_t read_dma_request;
	uint8_t dma_channel;
#endif
};

struct i2c_sam0_msg {
	uint8_t *buffer;
	uint32_t size;
	uint32_t status;
};

struct i2c_sam0_dev_data {
	struct k_sem lock;
	struct k_sem sem;
	struct i2c_sam0_msg msg;
	struct i2c_msg *msgs;
	uint8_t num_msgs;
};

static void wait_synchronization(SercomI2cm *regs)
{
#if defined(SERCOM_I2CM_SYNCBUSY_MASK)
	/* SYNCBUSY is a register */
	while ((regs->SYNCBUSY.reg & SERCOM_I2CM_SYNCBUSY_MASK) != 0) {
	}
#elif defined(SERCOM_I2CM_STATUS_SYNCBUSY)
	/* SYNCBUSY is a bit */
	while ((regs->STATUS.reg & SERCOM_I2CM_STATUS_SYNCBUSY) != 0) {
	}
#else
#error Unsupported device
#endif
}

static bool i2c_sam0_terminate_on_error(const struct device *dev)
{
	struct i2c_sam0_dev_data *data = dev->data;
	const struct i2c_sam0_dev_config *const cfg = dev->config;
	SercomI2cm *i2c = cfg->regs;

	if (!(i2c->STATUS.reg & (SERCOM_I2CM_STATUS_ARBLOST |
				 SERCOM_I2CM_STATUS_RXNACK |
#ifdef SERCOM_I2CM_STATUS_LENERR
				 SERCOM_I2CM_STATUS_LENERR |
#endif
#ifdef SERCOM_I2CM_STATUS_SEXTTOUT
				 SERCOM_I2CM_STATUS_SEXTTOUT |
#endif
#ifdef SERCOM_I2CM_STATUS_MEXTTOUT
				 SERCOM_I2CM_STATUS_MEXTTOUT |
#endif
				 SERCOM_I2CM_STATUS_LOWTOUT |
				 SERCOM_I2CM_STATUS_BUSERR))) {
		return false;
	}

#ifdef CONFIG_I2C_SAM0_DMA_DRIVEN
	if (cfg->dma_channel != 0xFF) {
		dma_stop(cfg->dma_dev, cfg->dma_channel);
	}
#endif

	data->msg.status = i2c->STATUS.reg;

	/*
	 * Clear all the flags that require an explicit clear
	 * (as opposed to being cleared by ADDR writes, etc)
	 */
	i2c->STATUS.reg = SERCOM_I2CM_STATUS_ARBLOST |
#ifdef SERCOM_I2CM_STATUS_LENERR
			  SERCOM_I2CM_STATUS_LENERR |
#endif
			  SERCOM_I2CM_STATUS_LOWTOUT |
			  SERCOM_I2CM_STATUS_BUSERR;
	wait_synchronization(i2c);

	i2c->INTENCLR.reg = SERCOM_I2CM_INTENCLR_MASK;
	if (i2c->INTFLAG.reg & (SERCOM_I2CM_INTFLAG_MB | SERCOM_I2CM_INTFLAG_SB)) {
		i2c->CTRLB.bit.CMD = 3;
	}
	k_sem_give(&data->sem);
	return true;
}

static void i2c_sam0_isr(const struct device *dev)
{
	struct i2c_sam0_dev_data *data = dev->data;
	const struct i2c_sam0_dev_config *const cfg = dev->config;
	SercomI2cm *i2c = cfg->regs;

	/* Get present interrupts and clear them */
	uint32_t status = i2c->INTFLAG.reg;

	i2c->INTFLAG.reg = status;

	if (i2c_sam0_terminate_on_error(dev)) {
		return;
	}

	/*
	 * Directly send/receive next message if it is in the same direction and
	 * the current message has no stop flag and the next message has no
	 * restart flag.
	 */
	const bool continue_next = (data->msg.size == 1) && (data->num_msgs > 1) &&
				   ((data->msgs[0].flags & I2C_MSG_RW_MASK) ==
				    (data->msgs[1].flags & I2C_MSG_RW_MASK)) &&
				   !(data->msgs[0].flags & I2C_MSG_STOP) &&
				   !(data->msgs[1].flags & I2C_MSG_RESTART) &&
				   ((status & (SERCOM_I2CM_INTFLAG_MB | SERCOM_I2CM_INTFLAG_SB)));

	if (status & SERCOM_I2CM_INTFLAG_MB) {
		if (!data->msg.size) {
			i2c->INTENCLR.reg = SERCOM_I2CM_INTENCLR_MASK;

			/*
			 * Decide whether to issue a repeated start, or a stop condition...
			 *   - A repeated start can either be accomplished by writing a 0x1
			 *     to the CMD field, or by writing to ADDR - which is what this
			 *     driver does in i2c_sam0_transfer().
			 *   - A stop is accomplished by writing a 0x3 to CMD (below).
			 *
			 * This decision is not the same as continue_next, as the value of
			 * data->msg.size is already zero (not one), and i2c_sam0_transfer()
			 * is responsible for advancing to the next message, not the ISR.
			 */
			if ((data->num_msgs <= 1)
			    || (data->msgs[0].flags & I2C_MSG_STOP)
			    || !(data->msgs[1].flags & I2C_MSG_RESTART)) {
				i2c->CTRLB.bit.CMD = 3;
			}

			k_sem_give(&data->sem);
			return;
		}

		i2c->DATA.reg = *data->msg.buffer;
		data->msg.buffer++;
		data->msg.size--;
	} else if (status & SERCOM_I2CM_INTFLAG_SB) {
		if (!continue_next && (data->msg.size == 1)) {
			/*
			 * If this is the last byte, then prepare for an auto
			 * NACK before doing the actual read.  This does not
			 * require write synchronization.
			 */
			i2c->CTRLB.bit.ACKACT = 1;
			i2c->CTRLB.bit.CMD = 3;
		}

		*data->msg.buffer = i2c->DATA.reg;
		data->msg.buffer++;
		data->msg.size--;

		if (!continue_next && !data->msg.size) {
			i2c->INTENCLR.reg = SERCOM_I2CM_INTENCLR_MASK;
			k_sem_give(&data->sem);
			return;
		}
	}

	if (continue_next) {
		data->msgs++;
		data->num_msgs--;

		data->msg.buffer = data->msgs->buf;
		data->msg.size = data->msgs->len;
		data->msg.status = 0;
	}
}

#ifdef CONFIG_I2C_SAM0_DMA_DRIVEN

static void i2c_sam0_dma_write_done(const struct device *dma_dev, void *arg,
				    uint32_t id, int error_code)
{
	const struct device *dev = arg;
	struct i2c_sam0_dev_data *data = dev->data;
	const struct i2c_sam0_dev_config *const cfg = dev->config;
	SercomI2cm *i2c = cfg->regs;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(id);

	unsigned int key = irq_lock();

	if (i2c_sam0_terminate_on_error(dev)) {
		irq_unlock(key);
		return;
	}

	if (error_code < 0) {
		LOG_ERR("DMA write error on %s: %d", dev->name, error_code);
		i2c->INTENCLR.reg = SERCOM_I2CM_INTENCLR_MASK;
		irq_unlock(key);

		data->msg.status = error_code;

		k_sem_give(&data->sem);
		return;
	}

	irq_unlock(key);

	/*
	 * DMA has written the whole message now, so just wait for the
	 * final I2C IRQ to indicate that it's finished transmitting.
	 */
	data->msg.size = 0;
	i2c->INTENSET.reg = SERCOM_I2CM_INTENSET_MB;
}

static bool i2c_sam0_dma_write_start(const struct device *dev)
{
	struct i2c_sam0_dev_data *data = dev->data;
	const struct i2c_sam0_dev_config *const cfg = dev->config;
	SercomI2cm *i2c = cfg->regs;
	int retval;

	if (cfg->dma_channel == 0xFF) {
		return false;
	}

	if (data->msg.size <= 1) {
		/*
		 * Catch empty writes and skip DMA on single byte transfers.
		 */
		return false;
	}

	struct dma_config dma_cfg = { 0 };
	struct dma_block_config dma_blk = { 0 };

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.user_data = (void *)dev;
	dma_cfg.dma_callback = i2c_sam0_dma_write_done;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_cfg.dma_slot = cfg->write_dma_request;

	dma_blk.block_size = data->msg.size;
	dma_blk.source_address = (uint32_t)data->msg.buffer;
	dma_blk.dest_address = (uint32_t)(&(i2c->DATA.reg));
	dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	retval = dma_config(cfg->dma_dev, cfg->dma_channel, &dma_cfg);
	if (retval != 0) {
		LOG_ERR("Write DMA configure on %s failed: %d",
			dev->name, retval);
		return false;
	}

	retval = dma_start(cfg->dma_dev, cfg->dma_channel);
	if (retval != 0) {
		LOG_ERR("Write DMA start on %s failed: %d",
			dev->name, retval);
		return false;
	}

	return true;
}

static void i2c_sam0_dma_read_done(const struct device *dma_dev, void *arg,
				   uint32_t id, int error_code)
{
	const struct device *dev = arg;
	struct i2c_sam0_dev_data *data = dev->data;
	const struct i2c_sam0_dev_config *const cfg = dev->config;
	SercomI2cm *i2c = cfg->regs;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(id);

	unsigned int key = irq_lock();

	if (i2c_sam0_terminate_on_error(dev)) {
		irq_unlock(key);
		return;
	}

	if (error_code < 0) {
		LOG_ERR("DMA read error on %s: %d", dev->name, error_code);
		i2c->INTENCLR.reg = SERCOM_I2CM_INTENCLR_MASK;
		irq_unlock(key);

		data->msg.status = error_code;

		k_sem_give(&data->sem);
		return;
	}

	irq_unlock(key);

	/*
	 * DMA has read all but the last byte now, so let the ISR handle
	 * that and the terminating NACK.
	 */
	data->msg.buffer += data->msg.size - 1;
	data->msg.size = 1;
	i2c->INTENSET.reg = SERCOM_I2CM_INTENSET_SB;
}

static bool i2c_sam0_dma_read_start(const struct device *dev)
{
	struct i2c_sam0_dev_data *data = dev->data;
	const struct i2c_sam0_dev_config *const cfg = dev->config;
	SercomI2cm *i2c = cfg->regs;
	int retval;

	if (cfg->dma_channel == 0xFF) {
		return false;
	}

	if (data->msg.size <= 2) {
		/*
		 * The last byte is always handled by the I2C ISR so
		 * just skip a two length read as well.
		 */
		return false;
	}

	struct dma_config dma_cfg = { 0 };
	struct dma_block_config dma_blk = { 0 };

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.user_data = (void *)dev;
	dma_cfg.dma_callback = i2c_sam0_dma_read_done;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_cfg.dma_slot = cfg->read_dma_request;

	dma_blk.block_size = data->msg.size - 1;
	dma_blk.dest_address = (uint32_t)data->msg.buffer;
	dma_blk.source_address = (uint32_t)(&(i2c->DATA.reg));
	dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	retval = dma_config(cfg->dma_dev, cfg->dma_channel, &dma_cfg);
	if (retval != 0) {
		LOG_ERR("Read DMA configure on %s failed: %d",
			dev->name, retval);
		return false;
	}

	retval = dma_start(cfg->dma_dev, cfg->dma_channel);
	if (retval != 0) {
		LOG_ERR("Read DMA start on %s failed: %d",
			dev->name, retval);
		return false;
	}

	return true;
}

#endif

static int i2c_sam0_transfer(const struct device *dev, struct i2c_msg *msgs,
			     uint8_t num_msgs, uint16_t addr)
{
	struct i2c_sam0_dev_data *data = dev->data;
	const struct i2c_sam0_dev_config *const cfg = dev->config;
	SercomI2cm *i2c = cfg->regs;
	uint32_t addr_reg;
	int ret;

	if (!num_msgs) {
		return 0;
	}

	k_sem_take(&data->lock, K_FOREVER);

	data->num_msgs = num_msgs;
	data->msgs = msgs;

	for (; data->num_msgs > 0;) {
		if (!data->msgs->len) {
			if ((data->msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
				ret = -EINVAL;
				goto unlock;
			}
		}

		i2c->INTENCLR.reg = SERCOM_I2CM_INTENCLR_MASK;
		i2c->INTFLAG.reg = SERCOM_I2CM_INTFLAG_MASK;

		i2c->STATUS.reg = SERCOM_I2CM_STATUS_ARBLOST |
#ifdef SERCOM_I2CM_STATUS_LENERR
				  SERCOM_I2CM_STATUS_LENERR |
#endif
				  SERCOM_I2CM_STATUS_LOWTOUT |
				  SERCOM_I2CM_STATUS_BUSERR;
		wait_synchronization(i2c);

		data->msg.buffer = data->msgs->buf;
		data->msg.size = data->msgs->len;
		data->msg.status = 0;

		addr_reg = addr << 1U;
		if ((data->msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			addr_reg |= 1U;

			/* Set to auto ACK */
			i2c->CTRLB.bit.ACKACT = 0;
			wait_synchronization(i2c);
		}

		if (data->msgs->flags & I2C_MSG_ADDR_10_BITS) {
#ifdef SERCOM_I2CM_ADDR_TENBITEN
			addr_reg |= SERCOM_I2CM_ADDR_TENBITEN;
#else
			ret = -ENOTSUP;
			goto unlock;
#endif
		}

		unsigned int key = irq_lock();

		/*
		 * Writing the address starts the transaction, issuing
		 * a start/repeated start as required.
		 */
		i2c->ADDR.reg = addr_reg;

		/*
		 * Have to wait here to make sure the address write
		 * clears any pending requests or errors before DMA or
		 * ISR tries to handle it.
		 */
		wait_synchronization(i2c);

#ifdef SERCOM_I2CM_INTENSET_ERROR
		i2c->INTENSET.reg = SERCOM_I2CM_INTENSET_ERROR;
#endif

		if ((data->msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			/*
			 * Always set MB even when reading, since that's how
			 * some errors are indicated.
			 */
			i2c->INTENSET.reg = SERCOM_I2CM_INTENSET_MB;

#ifdef CONFIG_I2C_SAM0_DMA_DRIVEN
			if (!i2c_sam0_dma_read_start(dev))
#endif
			{
				i2c->INTENSET.reg = SERCOM_I2CM_INTENSET_SB;
			}

		} else {
#ifdef CONFIG_I2C_SAM0_DMA_DRIVEN
			if (!i2c_sam0_dma_write_start(dev))
#endif
			{
				i2c->INTENSET.reg = SERCOM_I2CM_INTENSET_MB;
			}
		}

		irq_unlock(key);

		/* Now wait for the ISR to handle everything */
		ret = k_sem_take(&data->sem, I2C_TRANSFER_TIMEOUT_MSEC);

		if (ret != 0) {
			ret = -EIO;
			goto unlock;
		}

		if (data->msg.status) {
			if (data->msg.status & SERCOM_I2CM_STATUS_ARBLOST) {
				LOG_DBG("Arbitration lost on %s",
					dev->name);
				ret = -EAGAIN;
				goto unlock;
			}

			LOG_ERR("Transaction error on %s: %08X",
				dev->name, data->msg.status);
			ret = -EIO;
			goto unlock;
		}

		data->num_msgs--;
		data->msgs++;
	}

	ret = 0;
unlock:
	k_sem_give(&data->lock);

	return ret;
}

static int i2c_sam0_set_apply_bitrate(const struct device *dev,
				      uint32_t config)
{
	const struct i2c_sam0_dev_config *const cfg = dev->config;
	SercomI2cm *i2c = cfg->regs;
	uint32_t baud;
	uint32_t baud_low;
	uint32_t baud_high;

	uint32_t CTRLA = i2c->CTRLA.reg;

#ifdef SERCOM_I2CM_CTRLA_SPEED_Msk
	CTRLA &= ~SERCOM_I2CM_CTRLA_SPEED_Msk;
#endif
	CTRLA &= ~SERCOM_I2CM_CTRLA_SDAHOLD_Msk;

	switch (I2C_SPEED_GET(config)) {
	case I2C_SPEED_STANDARD:
#ifdef SERCOM_I2CM_CTRLA_SPEED
		CTRLA |= SERCOM_I2CM_CTRLA_SPEED(0);
#endif
		CTRLA |= SERCOM_I2CM_CTRLA_SDAHOLD(0x0);
		i2c->CTRLA.reg = CTRLA;
		wait_synchronization(i2c);

		/* 5 is the nominal 100ns rise time from the app notes */
		baud = (SOC_ATMEL_SAM0_GCLK0_FREQ_HZ / 100000U - 5U - 10U) / 2U;
		if (baud > 255U || baud < 1U) {
			return -ERANGE;
		}

		LOG_DBG("Setting %s to standard mode with divisor %u",
			dev->name, baud);

		i2c->BAUD.reg = SERCOM_I2CM_BAUD_BAUD(baud);
		break;

	case I2C_SPEED_FAST:
		CTRLA |= SERCOM_I2CM_CTRLA_SDAHOLD(0x0);
		i2c->CTRLA.reg = CTRLA;
		wait_synchronization(i2c);

		/* 5 is the nominal 100ns rise time from the app notes */
		baud = (SOC_ATMEL_SAM0_GCLK0_FREQ_HZ / 400000U - 5U - 10U) / 2U;
		if (baud > 255U || baud < 1U) {
			return -ERANGE;
		}

		LOG_DBG("Setting %s to fast mode with divisor %u",
			dev->name, baud);

		i2c->BAUD.reg = SERCOM_I2CM_BAUD_BAUD(baud);
		break;

	case I2C_SPEED_FAST_PLUS:
#ifdef SERCOM_I2CM_CTRLA_SPEED
		CTRLA |= SERCOM_I2CM_CTRLA_SPEED(1);
#endif
		CTRLA |= SERCOM_I2CM_CTRLA_SDAHOLD(0x2);
		i2c->CTRLA.reg = CTRLA;
		wait_synchronization(i2c);

		/* 5 is the nominal 100ns rise time from the app notes */
		baud = (SOC_ATMEL_SAM0_GCLK0_FREQ_HZ / 1000000U - 5U - 10U);

		/* 2:1 low:high ratio */
		baud_high = baud;
		baud_high /= 3U;
		baud_high = CLAMP(baud_high, 1U, 255U);
		baud_low = baud - baud_high;
		if (baud_low < 1U && baud_high > 1U) {
			--baud_high;
			++baud_low;
		}

		if (baud_low < 1U || baud_low > 255U) {
			return -ERANGE;
		}

		LOG_DBG("Setting %s to fast mode plus with divisors %u/%u",
			dev->name, baud_high, baud_low);

		i2c->BAUD.reg = SERCOM_I2CM_BAUD_BAUD(baud_high) |
				SERCOM_I2CM_BAUD_BAUDLOW(baud_low);
		break;

	case I2C_SPEED_HIGH:
#ifdef SERCOM_I2CM_CTRLA_SPEED
		CTRLA |= SERCOM_I2CM_CTRLA_SPEED(2);
#endif
		CTRLA |= SERCOM_I2CM_CTRLA_SDAHOLD(0x2);
		i2c->CTRLA.reg = CTRLA;
		wait_synchronization(i2c);

		baud = (SOC_ATMEL_SAM0_GCLK0_FREQ_HZ / 3400000U) - 2U;

		/* 2:1 low:high ratio */
		baud_high = baud;
		baud_high /= 3U;
		baud_high = CLAMP(baud_high, 1U, 255U);
		baud_low = baud - baud_high;
		if (baud_low < 1U && baud_high > 1U) {
			--baud_high;
			++baud_low;
		}

		if (baud_low < 1U || baud_low > 255U) {
			return -ERANGE;
		}

#ifdef SERCOM_I2CM_BAUD_HSBAUD
		LOG_DBG("Setting %s to high speed with divisors %u/%u",
			dev->name, baud_high, baud_low);

		/*
		 * 48 is just from the app notes, but the datasheet says
		 * it's ignored
		 */
		i2c->BAUD.reg = SERCOM_I2CM_BAUD_HSBAUD(baud_high) |
				SERCOM_I2CM_BAUD_HSBAUDLOW(baud_low) |
				SERCOM_I2CM_BAUD_BAUD(48) |
				SERCOM_I2CM_BAUD_BAUDLOW(48);
#else
		return -ENOTSUP;
#endif
		break;

	default:
		return -ENOTSUP;
	}

	wait_synchronization(i2c);
	return 0;
}

static int i2c_sam0_configure(const struct device *dev, uint32_t config)
{
	const struct i2c_sam0_dev_config *const cfg = dev->config;
	SercomI2cm *i2c = cfg->regs;
	int retval;

	if (!(config & I2C_MODE_CONTROLLER)) {
		return -EINVAL;
	}

	if (config & I2C_SPEED_MASK) {
		i2c->CTRLA.bit.ENABLE = 0;
		wait_synchronization(i2c);

		retval = i2c_sam0_set_apply_bitrate(dev, config);

		i2c->CTRLA.bit.ENABLE = 1;
		wait_synchronization(i2c);

		if (retval != 0) {
			return retval;
		}
	}

	return 0;
}

static int i2c_sam0_initialize(const struct device *dev)
{
	struct i2c_sam0_dev_data *data = dev->data;
	const struct i2c_sam0_dev_config *const cfg = dev->config;
	SercomI2cm *i2c = cfg->regs;
	int retval;

#ifdef MCLK
	/* Enable the GCLK */
	GCLK->PCHCTRL[cfg->gclk_core_id].reg = GCLK_PCHCTRL_GEN_GCLK0 |
					       GCLK_PCHCTRL_CHEN;
	/* Enable SERCOM clock in MCLK */
	*cfg->mclk |= cfg->mclk_mask;
#else
	/* Enable the GCLK */
	GCLK->CLKCTRL.reg = cfg->gclk_clkctrl_id | GCLK_CLKCTRL_GEN_GCLK0 |
			    GCLK_CLKCTRL_CLKEN;

	/* Enable SERCOM clock in PM */
	PM->APBCMASK.reg |= cfg->pm_apbcmask;
#endif
	/* Disable all I2C interrupts */
	i2c->INTENCLR.reg = SERCOM_I2CM_INTENCLR_MASK;

	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

	/* I2C mode, enable timeouts */
	i2c->CTRLA.reg = SERCOM_I2CM_CTRLA_MODE_I2C_MASTER |
#ifdef SERCOM_I2CM_CTRLA_LOWTOUTEN
			 SERCOM_I2CM_CTRLA_LOWTOUTEN |
#endif
			 SERCOM_I2CM_CTRLA_INACTOUT(0x3);
	wait_synchronization(i2c);

	/* Enable smart mode (auto ACK) */
	i2c->CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;
	wait_synchronization(i2c);

	retval = i2c_sam0_set_apply_bitrate(dev,
					    i2c_map_dt_bitrate(cfg->bitrate));
	if (retval != 0) {
		return retval;
	}

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->sem, 0, 1);

	cfg->irq_config_func(dev);

#ifdef CONFIG_I2C_SAM0_DMA_DRIVEN
	if (!device_is_ready(cfg->dma_dev)) {
		return -ENODEV;
	}
#endif

	i2c->CTRLA.bit.ENABLE = 1;
	wait_synchronization(i2c);

	/* Force bus idle */
	i2c->STATUS.bit.BUSSTATE = 1;
	wait_synchronization(i2c);

	return 0;
}

static DEVICE_API(i2c, i2c_sam0_driver_api) = {
	.configure = i2c_sam0_configure,
	.transfer = i2c_sam0_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#ifdef CONFIG_I2C_SAM0_DMA_DRIVEN
#define I2C_SAM0_DMA_CHANNELS(n)					\
	.dma_dev = DEVICE_DT_GET(ATMEL_SAM0_DT_INST_DMA_CTLR(n, tx)),	\
	.write_dma_request = ATMEL_SAM0_DT_INST_DMA_TRIGSRC(n, tx),	\
	.read_dma_request = ATMEL_SAM0_DT_INST_DMA_TRIGSRC(n, rx),	\
	.dma_channel = ATMEL_SAM0_DT_INST_DMA_CHANNEL(n, rx),
#else
#define I2C_SAM0_DMA_CHANNELS(n)
#endif

#define SAM0_I2C_IRQ_CONNECT(n, m)					\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq),		\
			    DT_INST_IRQ_BY_IDX(n, m, priority),		\
			    i2c_sam0_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));		\
	} while (false)

#if DT_INST_IRQ_HAS_IDX(0, 3)
#define I2C_SAM0_IRQ_HANDLER(n)						\
static void i2c_sam0_irq_config_##n(const struct device *dev)			\
{									\
	SAM0_I2C_IRQ_CONNECT(n, 0);					\
	SAM0_I2C_IRQ_CONNECT(n, 1);					\
	SAM0_I2C_IRQ_CONNECT(n, 2);					\
	SAM0_I2C_IRQ_CONNECT(n, 3);					\
}
#else
#define I2C_SAM0_IRQ_HANDLER(n)						\
static void i2c_sam0_irq_config_##n(const struct device *dev)			\
{									\
	SAM0_I2C_IRQ_CONNECT(n, 0);					\
}
#endif

#ifdef MCLK
#define I2C_SAM0_CONFIG(n)						\
static const struct i2c_sam0_dev_config i2c_sam0_dev_config_##n = {	\
	.regs = (SercomI2cm *)DT_INST_REG_ADDR(n),			\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
	.bitrate = DT_INST_PROP(n, clock_frequency),			\
	.mclk = (volatile uint32_t *)MCLK_MASK_DT_INT_REG_ADDR(n),	\
	.mclk_mask = BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, bit)),	\
	.gclk_core_id = DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, periph_ch),\
	.irq_config_func = &i2c_sam0_irq_config_##n,			\
	I2C_SAM0_DMA_CHANNELS(n)					\
}
#else /* !MCLK */
#define I2C_SAM0_CONFIG(n)						\
static const struct i2c_sam0_dev_config i2c_sam0_dev_config_##n = {	\
	.regs = (SercomI2cm *)DT_INST_REG_ADDR(n),			\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
	.bitrate = DT_INST_PROP(n, clock_frequency),			\
	.pm_apbcmask = BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, pm, bit)),	\
	.gclk_clkctrl_id = DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, clkctrl_id),\
	.irq_config_func = &i2c_sam0_irq_config_##n,			\
	I2C_SAM0_DMA_CHANNELS(n)					\
}
#endif

#define I2C_SAM0_DEVICE(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
	static void i2c_sam0_irq_config_##n(const struct device *dev);	\
	I2C_SAM0_CONFIG(n);						\
	static struct i2c_sam0_dev_data i2c_sam0_dev_data_##n;		\
	I2C_DEVICE_DT_INST_DEFINE(n,					\
			    i2c_sam0_initialize,			\
			    NULL,					\
			    &i2c_sam0_dev_data_##n,			\
			    &i2c_sam0_dev_config_##n, POST_KERNEL,	\
			    CONFIG_I2C_INIT_PRIORITY,			\
			    &i2c_sam0_driver_api);			\
	I2C_SAM0_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(I2C_SAM0_DEVICE)
