/*
 * Copyright (c) 2019 Derek Hageman <hageman@inthat.cloud>
 * Copyright (c) 2024 Gerson Fernando Budke <nandojve@gmail.com>
 * Copyright (c) 2025 GP Orcullo
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

/* clang-format off */

#include "i2c-priv.h"
#include "i2c_sam0.h"

#define CTRLA_MODE_I2C_MASTER CTRLA_MODE(5)

#if CONFIG_I2C_SAM0_TRANSFER_TIMEOUT
#define I2C_TRANSFER_TIMEOUT_MSEC K_MSEC(CONFIG_I2C_SAM0_TRANSFER_TIMEOUT)
#else
#define I2C_TRANSFER_TIMEOUT_MSEC K_FOREVER
#endif

struct i2c_sam0_dev_config {
	uintptr_t regs;
	const struct pinctrl_dev_config *pcfg;
	uint32_t bitrate;
	volatile uint32_t *mclk;
	uint32_t mclk_mask;
	uint32_t gclk_gen;
	uint16_t gclk_id;
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

static void wait_synchronization(uintptr_t regs)
{
#ifndef CONFIG_SOC_SERIES_SAMD20
	/* SYNCBUSY is a register */
	while ((sys_read32(regs + SYNCBUSY_OFFSET) & SYNCBUSY_MASK) != 0) {
	}
#else
	/* SYNCBUSY is a bit */
	while (sys_test_bit(regs + STATUS_OFFSET, STATUS_SYNCBUSY_BIT)) {
	}
#endif
}

static bool i2c_sam0_terminate_on_error(const struct device *dev)
{
	struct i2c_sam0_dev_data *data = dev->data;
	const struct i2c_sam0_dev_config *const cfg = dev->config;
	uintptr_t i2c = cfg->regs;

	if (!(sys_read16(i2c + STATUS_OFFSET) & (STATUS_ARBLOST | STATUS_RXNACK | STATUS_LENERR |
						 STATUS_SEXTTOUT | STATUS_MEXTTOUT |
						 STATUS_LOWTOUT | STATUS_BUSERR))) {
		return false;
	}

#ifdef CONFIG_I2C_SAM0_DMA_DRIVEN
	if (cfg->dma_channel != 0xFF) {
		dma_stop(cfg->dma_dev, cfg->dma_channel);
	}
#endif

	data->msg.status = sys_read16(i2c + STATUS_OFFSET);

	/*
	 * Clear all the flags that require an explicit clear
	 * (as opposed to being cleared by ADDR writes, etc)
	 */
	sys_write16(STATUS_ARBLOST | STATUS_LENERR | STATUS_LOWTOUT | STATUS_BUSERR,
		    i2c + STATUS_OFFSET);
	wait_synchronization(i2c);

	sys_write8(INTENCLR_MASK, i2c + INTENCLR_OFFSET);
	if (sys_read8(i2c + INTFLAG_OFFSET) & (INTFLAG_MB | INTFLAG_SB)) {
		sys_set_bits(i2c + CTRLB_OFFSET, CTRLB_CMD_MASK);
	}
	k_sem_give(&data->sem);
	return true;
}

static void i2c_sam0_isr(const struct device *dev)
{
	struct i2c_sam0_dev_data *data = dev->data;
	const struct i2c_sam0_dev_config *const cfg = dev->config;
	uintptr_t i2c = cfg->regs;

	/* Get present interrupts and clear them */
	uint8_t status = sys_read8(i2c + INTFLAG_OFFSET);

	sys_write8(status, i2c + INTFLAG_OFFSET);

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
				   ((status & (INTFLAG_MB | INTFLAG_SB)));

	if (status & INTFLAG_MB) {
		if (!data->msg.size) {
			sys_write8(INTENCLR_MASK, i2c + INTENCLR_OFFSET);

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
				sys_set_bits(i2c + CTRLB_OFFSET, CTRLB_CMD_MASK);
			}

			k_sem_give(&data->sem);
			return;
		}

		sys_write8(*data->msg.buffer, i2c + DATA_OFFSET);
		data->msg.buffer++;
		data->msg.size--;
	} else if (status & INTFLAG_SB) {
		if (!continue_next && (data->msg.size == 1)) {
			/*
			 * If this is the last byte, then prepare for an auto
			 * NACK before doing the actual read.  This does not
			 * require write synchronization.
			 */
			sys_set_bit(i2c + CTRLB_OFFSET, CTRLB_ACKACT_BIT);
			sys_set_bits(i2c + CTRLB_OFFSET, CTRLB_CMD_MASK);
		}

		*data->msg.buffer = sys_read8(i2c + DATA_OFFSET);
		data->msg.buffer++;
		data->msg.size--;

		if (!continue_next && !data->msg.size) {
			sys_write8(INTENCLR_MASK, i2c + INTENCLR_OFFSET);
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
	uintptr_t i2c = cfg->regs;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(id);

	unsigned int key = irq_lock();

	if (i2c_sam0_terminate_on_error(dev)) {
		irq_unlock(key);
		return;
	}

	if (error_code < 0) {
		LOG_ERR("DMA write error on %s: %d", dev->name, error_code);
		sys_write8(INTENCLR_MASK, i2c + INTENCLR_OFFSET);
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
	sys_write8(INTENSET_MB, i2c + INTENSET_OFFSET);
}

static bool i2c_sam0_dma_write_start(const struct device *dev)
{
	struct i2c_sam0_dev_data *data = dev->data;
	const struct i2c_sam0_dev_config *const cfg = dev->config;
	uintptr_t i2c = cfg->regs;
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
	dma_blk.dest_address = (uint32_t)(i2c + DATA_OFFSET);
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
	uintptr_t i2c = cfg->regs;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(id);

	unsigned int key = irq_lock();

	if (i2c_sam0_terminate_on_error(dev)) {
		irq_unlock(key);
		return;
	}

	if (error_code < 0) {
		LOG_ERR("DMA read error on %s: %d", dev->name, error_code);
		sys_write8(INTENCLR_MASK, i2c + INTENCLR_OFFSET);
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
	sys_write8(INTENSET_SB, i2c + INTENSET_OFFSET);
}

static bool i2c_sam0_dma_read_start(const struct device *dev)
{
	struct i2c_sam0_dev_data *data = dev->data;
	const struct i2c_sam0_dev_config *const cfg = dev->config;
	uintptr_t i2c = cfg->regs;
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
	dma_blk.source_address = (uint32_t)(i2c + DATA_OFFSET);
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
	uintptr_t i2c = cfg->regs;
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

		sys_write8(INTENCLR_MASK, i2c + INTENCLR_OFFSET);
		sys_write8(INTFLAG_MASK, i2c + INTFLAG_OFFSET);

		sys_write16(STATUS_ARBLOST | STATUS_LENERR | STATUS_LOWTOUT | STATUS_BUSERR,
			    i2c + STATUS_OFFSET);
		wait_synchronization(i2c);

		data->msg.buffer = data->msgs->buf;
		data->msg.size = data->msgs->len;
		data->msg.status = 0;

		addr_reg = addr << 1U;
		if ((data->msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			addr_reg |= 1U;

			/* Set to auto ACK */
			sys_clear_bit(i2c + CTRLB_OFFSET, CTRLB_ACKACT_BIT);
			wait_synchronization(i2c);
		}

		if (data->msgs->flags & I2C_MSG_ADDR_10_BITS) {
#ifndef CONFIG_SOC_SERIES_SAMD20
			addr_reg |= ADDR_TENBITEN;
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
		sys_write32(addr_reg, i2c + ADDR_OFFSET);

		/*
		 * Have to wait here to make sure the address write
		 * clears any pending requests or errors before DMA or
		 * ISR tries to handle it.
		 */
		wait_synchronization(i2c);

#ifndef CONFIG_SOC_SERIES_SAMD20
		sys_write8(INTENSET_ERROR, i2c + INTENSET_OFFSET);
#endif

		if ((data->msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			/*
			 * Always set MB even when reading, since that's how
			 * some errors are indicated.
			 */
			sys_write8(INTENSET_MB, i2c + INTENSET_OFFSET);

#ifdef CONFIG_I2C_SAM0_DMA_DRIVEN
			if (!i2c_sam0_dma_read_start(dev))
#endif
			{
				sys_write8(INTENSET_SB, i2c + INTENSET_OFFSET);
			}

		} else {
#ifdef CONFIG_I2C_SAM0_DMA_DRIVEN
			if (!i2c_sam0_dma_write_start(dev))
#endif
			{
				sys_write8(INTENSET_MB, i2c + INTENSET_OFFSET);
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
			if (data->msg.status & STATUS_ARBLOST) {
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
	uintptr_t i2c = cfg->regs;
	uint32_t baud;
	uint32_t baud_low;
	uint32_t baud_high;

	uint32_t CTRLA = sys_read32(i2c + CTRLA_OFFSET);

	CTRLA &= ~(CTRLA_SPEED_MASK | CTRLA_SDAHOLD_MASK);

	switch (I2C_SPEED_GET(config)) {
	case I2C_SPEED_STANDARD:
		CTRLA |= CTRLA_SPEED(0) | CTRLA_SDAHOLD(0);
		sys_write32(CTRLA, i2c + CTRLA_OFFSET);
		wait_synchronization(i2c);

		/* 5 is the nominal 100ns rise time from the app notes */
		baud = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 100000U - 5U - 10U) / 2U;
		if (baud > 255U || baud < 1U) {
			return -ERANGE;
		}

		LOG_DBG("Setting %s to standard mode with divisor %u",
			dev->name, baud);

		sys_write32(BAUD_BAUD(baud), i2c + BAUD_OFFSET);
		break;

	case I2C_SPEED_FAST:
		CTRLA |= CTRLA_SDAHOLD(0);
		sys_write32(CTRLA, i2c + CTRLA_OFFSET);
		wait_synchronization(i2c);

		/* 5 is the nominal 100ns rise time from the app notes */
		baud = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 400000U - 5U - 10U) / 2U;
		if (baud > 255U || baud < 1U) {
			return -ERANGE;
		}

		LOG_DBG("Setting %s to fast mode with divisor %u",
			dev->name, baud);

		sys_write32(BAUD_BAUD(baud), i2c + BAUD_OFFSET);
		break;

	case I2C_SPEED_FAST_PLUS:
		CTRLA |= CTRLA_SPEED(1) | CTRLA_SDAHOLD(2);
		sys_write32(CTRLA, i2c + CTRLA_OFFSET);
		wait_synchronization(i2c);

		/* 5 is the nominal 100ns rise time from the app notes */
		baud = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000000U - 5U - 10U);

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

		sys_write32(BAUD_BAUD(baud_high) | BAUD_BAUDLOW(baud_low), i2c + BAUD_OFFSET);
		break;

	case I2C_SPEED_HIGH:
		CTRLA |= CTRLA_SPEED(2) | CTRLA_SDAHOLD(2);
		sys_write32(CTRLA, i2c + CTRLA_OFFSET);
		wait_synchronization(i2c);

		baud = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 3400000U) - 2U;

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

#ifndef CONFIG_SOC_SERIES_SAMD20
		LOG_DBG("Setting %s to high speed with divisors %u/%u",
			dev->name, baud_high, baud_low);

		/*
		 * 48 is just from the app notes, but the datasheet says
		 * it's ignored
		 */
		sys_write32(BAUD_HSBAUD(baud_high) | BAUD_HSBAUDLOW(baud_low) |
				BAUD_BAUD(48) | BAUD_BAUDLOW(48),
			    i2c + BAUD_OFFSET);
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
	uintptr_t i2c = cfg->regs;
	int retval;

	if (!(config & I2C_MODE_CONTROLLER)) {
		return -EINVAL;
	}

	if (config & I2C_SPEED_MASK) {
		sys_clear_bit(i2c + CTRLA_OFFSET, CTRLA_ENABLE_BIT);
		wait_synchronization(i2c);

		retval = i2c_sam0_set_apply_bitrate(dev, config);

		sys_set_bit(i2c + CTRLA_OFFSET, CTRLA_ENABLE_BIT);
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
	const uintptr_t gclk = DT_REG_ADDR(DT_INST(0, atmel_sam0_gclk));
	uintptr_t i2c = cfg->regs;
	int retval;

	*cfg->mclk |= cfg->mclk_mask;

#if !defined(CONFIG_SOC_SERIES_SAMD20) && !defined(CONFIG_SOC_SERIES_SAMD21) &&                    \
	!defined(CONFIG_SOC_SERIES_SAMR21)

	sys_write32(PCHCTRL_CHEN | PCHCTRL_GEN(cfg->gclk_gen),
		    gclk + PCHCTRL_OFFSET + (4 * cfg->gclk_id));
#else
	sys_write16(CLKCTRL_CLKEN | CLKCTRL_GEN(cfg->gclk_gen) | CLKCTRL_ID(cfg->gclk_id),
		    gclk + CLKCTRL_OFFSET);
#endif

	/* Disable all I2C interrupts */
	sys_write8(INTENCLR_MASK, i2c + INTENCLR_OFFSET);

	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

	/* I2C mode, enable timeouts */
	sys_write32(CTRLA_MODE_I2C_MASTER | CTRLA_LOWTOUTEN | CTRLA_INACTOUT(0x3),
		    i2c + CTRLA_OFFSET);
	wait_synchronization(i2c);

	/* Enable smart mode (auto ACK) */
	sys_write32(CTRLB_SMEN, i2c + CTRLB_OFFSET);
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

	sys_set_bit(i2c + CTRLA_OFFSET, CTRLA_ENABLE_BIT);
	wait_synchronization(i2c);

	/* Force bus idle */
	uint16_t bstate = sys_read16(i2c + STATUS_OFFSET);

	sys_write16(bstate | STATUS_BUSSTATE, i2c + STATUS_OFFSET);
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
static void i2c_sam0_irq_config_##n(const struct device *dev)		\
{									\
	SAM0_I2C_IRQ_CONNECT(n, 0);					\
	SAM0_I2C_IRQ_CONNECT(n, 1);					\
	SAM0_I2C_IRQ_CONNECT(n, 2);					\
	SAM0_I2C_IRQ_CONNECT(n, 3);					\
}
#else
#define I2C_SAM0_IRQ_HANDLER(n)						\
static void i2c_sam0_irq_config_##n(const struct device *dev)		\
{									\
	SAM0_I2C_IRQ_CONNECT(n, 0);					\
}
#endif

#ifndef ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET
#define ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET(n, cell)		\
	(volatile uint32_t *)						\
	(DT_REG_ADDR(DT_INST_PHANDLE_BY_NAME(n, clocks, cell)) +	\
	 DT_INST_CLOCKS_CELL_BY_NAME(n, cell, offset))
#endif

#ifndef ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET
#define ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET(n)			\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(mclk)),	\
		(ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET(n, mclk)),	\
		(ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET(n, pm)))
#endif

#ifndef ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK
#define ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK(n, cell)			\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(mclk)),	\
		(BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, cell))),	\
		(BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, pm, cell))))
#endif

#define I2C_SAM0_CONFIG(n)						\
static const struct i2c_sam0_dev_config i2c_sam0_dev_config_##n = {	\
	.regs = DT_INST_REG_ADDR(n),					\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
	.bitrate = DT_INST_PROP(n, clock_frequency),			\
	.gclk_gen = DT_PHA_BY_NAME(DT_DRV_INST(n), atmel_assigned_clocks, gclk, gen),	\
	.gclk_id = DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, id),		\
	.mclk = ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET(n),		\
	.mclk_mask = ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK(n, bit),	\
	.irq_config_func = &i2c_sam0_irq_config_##n,			\
	I2C_SAM0_DMA_CHANNELS(n)					\
}

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

/* clang-format on */
