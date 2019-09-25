/*
 * Copyright (c) 2019 Derek Hageman <hageman@inthat.cloud>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/i2c.h>
#include <drivers/dma.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_sam0, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

struct i2c_sam0_dev_config {
	SercomI2cm *regs;
	u32_t bitrate;
	u32_t pm_apbcmask;
	u16_t gclk_clkctrl_id;

	void (*irq_config_func)(struct device *dev);

#ifdef CONFIG_I2C_SAM0_DMA_DRIVEN
	u8_t write_dma_request;
	u8_t read_dma_request;
	u8_t dma_channel;
#endif
};

struct i2c_sam0_msg {
	u8_t *buffer;
	u32_t size;
	u32_t status;
};

struct i2c_sam0_dev_data {
	struct k_sem sem;
	struct i2c_sam0_msg msg;

#ifdef CONFIG_I2C_SAM0_DMA_DRIVEN
	struct device *dma;
#endif
};

#define DEV_NAME(dev) ((dev)->config->name)
#define DEV_CFG(dev) \
	((const struct i2c_sam0_dev_config *const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct i2c_sam0_dev_data *const)(dev)->driver_data)

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

static bool i2c_sam0_terminate_on_error(struct device *dev)
{
	struct i2c_sam0_dev_data *data = DEV_DATA(dev);
	const struct i2c_sam0_dev_config *const cfg = DEV_CFG(dev);
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
	if (data->dma && cfg->dma_channel != 0xFF) {
		dma_stop(data->dma, cfg->dma_channel);
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
	k_sem_give(&data->sem);
	return true;
}

static void i2c_sam0_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct i2c_sam0_dev_data *data = DEV_DATA(dev);
	const struct i2c_sam0_dev_config *const cfg = DEV_CFG(dev);
	SercomI2cm *i2c = cfg->regs;

	/* Get present interrupts and clear them */
	u32_t status = i2c->INTFLAG.reg;

	i2c->INTFLAG.reg = status;

	if (i2c_sam0_terminate_on_error(dev)) {
		return;
	}

	if (status & SERCOM_I2CM_INTFLAG_MB) {
		if (!data->msg.size) {
			i2c->INTENCLR.reg = SERCOM_I2CM_INTENCLR_MASK;
			k_sem_give(&data->sem);
			return;
		}

		i2c->DATA.reg = *data->msg.buffer;
		data->msg.buffer++;
		data->msg.size--;

		return;
	}

	if (status & SERCOM_I2CM_INTFLAG_SB) {
		if (data->msg.size == 1) {
			/*
			 * If this is the last byte, then prepare for an auto
			 * NACK before doing the actual read.  This does not
			 * require write synchronization.
			 */
			i2c->CTRLB.bit.ACKACT = 1;
		}

		*data->msg.buffer = i2c->DATA.reg;
		data->msg.buffer++;
		data->msg.size--;

		if (!data->msg.size) {
			i2c->INTENCLR.reg = SERCOM_I2CM_INTENCLR_MASK;
			k_sem_give(&data->sem);
			return;
		}
		return;
	}
}

#ifdef CONFIG_I2C_SAM0_DMA_DRIVEN

static void i2c_sam0_dma_write_done(void *arg, u32_t id, int error_code)
{
	struct device *dev = arg;
	struct i2c_sam0_dev_data *data = DEV_DATA(dev);
	const struct i2c_sam0_dev_config *const cfg = DEV_CFG(dev);
	SercomI2cm *i2c = cfg->regs;

	ARG_UNUSED(id);

	int key = irq_lock();

	if (i2c_sam0_terminate_on_error(dev)) {
		irq_unlock(key);
		return;
	}

	if (error_code < 0) {
		LOG_ERR("DMA write error on %s: %d", DEV_NAME(dev), error_code);
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

static bool i2c_sam0_dma_write_start(struct device *dev)
{
	struct i2c_sam0_dev_data *data = DEV_DATA(dev);
	const struct i2c_sam0_dev_config *const cfg = DEV_CFG(dev);
	SercomI2cm *i2c = cfg->regs;
	int retval;

	if (!data->dma) {
		return false;
	}

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
	dma_cfg.callback_arg = dev;
	dma_cfg.dma_callback = i2c_sam0_dma_write_done;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_cfg.dma_slot = cfg->write_dma_request;

	dma_blk.block_size = data->msg.size;
	dma_blk.source_address = (u32_t)data->msg.buffer;
	dma_blk.dest_address = (u32_t)(&(i2c->DATA.reg));
	dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	retval = dma_config(data->dma, cfg->dma_channel, &dma_cfg);
	if (retval != 0) {
		LOG_ERR("Write DMA configure on %s failed: %d",
			DEV_NAME(dev), retval);
		return false;
	}

	retval = dma_start(data->dma, cfg->dma_channel);
	if (retval != 0) {
		LOG_ERR("Write DMA start on %s failed: %d",
			DEV_NAME(dev), retval);
		return false;
	}

	return true;
}

static void i2c_sam0_dma_read_done(void *arg, u32_t id, int error_code)
{
	struct device *dev = arg;
	struct i2c_sam0_dev_data *data = DEV_DATA(dev);
	const struct i2c_sam0_dev_config *const cfg = DEV_CFG(dev);
	SercomI2cm *i2c = cfg->regs;

	ARG_UNUSED(id);

	int key = irq_lock();

	if (i2c_sam0_terminate_on_error(dev)) {
		irq_unlock(key);
		return;
	}

	if (error_code < 0) {
		LOG_ERR("DMA read error on %s: %d", DEV_NAME(dev), error_code);
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

static bool i2c_sam0_dma_read_start(struct device *dev)
{
	struct i2c_sam0_dev_data *data = DEV_DATA(dev);
	const struct i2c_sam0_dev_config *const cfg = DEV_CFG(dev);
	SercomI2cm *i2c = cfg->regs;
	int retval;

	if (!data->dma) {
		return false;
	}

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
	dma_cfg.callback_arg = dev;
	dma_cfg.dma_callback = i2c_sam0_dma_read_done;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_cfg.dma_slot = cfg->read_dma_request;

	dma_blk.block_size = data->msg.size - 1;
	dma_blk.dest_address = (u32_t)data->msg.buffer;
	dma_blk.source_address = (u32_t)(&(i2c->DATA.reg));
	dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	retval = dma_config(data->dma, cfg->dma_channel, &dma_cfg);
	if (retval != 0) {
		LOG_ERR("Read DMA configure on %s failed: %d",
			DEV_NAME(dev), retval);
		return false;
	}

	retval = dma_start(data->dma, cfg->dma_channel);
	if (retval != 0) {
		LOG_ERR("Read DMA start on %s failed: %d",
			DEV_NAME(dev), retval);
		return false;
	}

	return true;
}

#endif

static int i2c_sam0_transfer(struct device *dev, struct i2c_msg *msgs,
			     u8_t num_msgs, u16_t addr)
{
	struct i2c_sam0_dev_data *data = DEV_DATA(dev);
	const struct i2c_sam0_dev_config *const cfg = DEV_CFG(dev);
	SercomI2cm *i2c = cfg->regs;
	u32_t addr_reg;

	if (!num_msgs) {
		return 0;
	}

	for (; num_msgs > 0;) {
		if (!msgs->len) {
			if ((msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
				return -EINVAL;
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

		data->msg.buffer = msgs->buf;
		data->msg.size = msgs->len;
		data->msg.status = 0;

		addr_reg = addr << 1U;
		if ((msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			addr_reg |= 1U;

			/* Set to auto ACK */
			i2c->CTRLB.bit.ACKACT = 0;
			wait_synchronization(i2c);
		}

		if (msgs->flags & I2C_MSG_ADDR_10_BITS) {
#ifdef SERCOM_I2CM_ADDR_TENBITEN
			addr_reg |= SERCOM_I2CM_ADDR_TENBITEN;
#else
			return -ENOTSUP;
#endif
		}

		int key = irq_lock();

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

		if ((msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
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
		k_sem_take(&data->sem, K_FOREVER);

		if (data->msg.status) {
			if (data->msg.status & SERCOM_I2CM_STATUS_ARBLOST) {
				LOG_DBG("Arbitration lost on %s",
					DEV_NAME(dev));
				return -EAGAIN;
			}

			LOG_ERR("Transaction error on %s: %08X",
				DEV_NAME(dev), data->msg.status);
			return -EIO;
		}

		if (msgs->flags & I2C_MSG_STOP) {
			i2c->CTRLB.bit.CMD = 3;
		} else if ((msgs->flags & I2C_MSG_RESTART) && num_msgs > 1) {
			/*
			 * No action, since we do this automatically if we
			 * don't send an explicit stop
			 */
		} else {
			/*
			 * Neither present, so assume we want to release
			 * the bus (by sending a stop)
			 */
			i2c->CTRLB.bit.CMD = 3;
		}

		num_msgs--;
		msgs++;
	}

	return 0;
}

static int i2c_sam0_set_apply_bitrate(struct device *dev, u32_t config)
{
	const struct i2c_sam0_dev_config *const cfg = DEV_CFG(dev);
	SercomI2cm *i2c = cfg->regs;
	u32_t baud;
	u32_t baud_low;
	u32_t baud_high;

	u32_t CTRLA = i2c->CTRLA.reg;

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
			DEV_NAME(dev), baud);

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
			DEV_NAME(dev), baud);

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
		baud_high = MAX(MIN(baud_high, 255U), 1U);
		baud_low = baud - baud_high;
		if (baud_low < 1U && baud_high > 1U) {
			--baud_high;
			++baud_low;
		}

		if (baud_low < 1U || baud_low > 255U) {
			return -ERANGE;
		}

		LOG_DBG("Setting %s to fast mode plus with divisors %u/%u",
			DEV_NAME(dev), baud_high, baud_low);

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
		baud_high = MAX(MIN(baud_high, 255U), 1U);
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
			DEV_NAME(dev), baud_high, baud_low);

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

static int i2c_sam0_configure(struct device *dev, u32_t config)
{
	const struct i2c_sam0_dev_config *const cfg = DEV_CFG(dev);
	SercomI2cm *i2c = cfg->regs;
	int retval;

	if (!(config & I2C_MODE_MASTER)) {
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

static int i2c_sam0_initialize(struct device *dev)
{
	struct i2c_sam0_dev_data *data = DEV_DATA(dev);
	const struct i2c_sam0_dev_config *const cfg = DEV_CFG(dev);
	SercomI2cm *i2c = cfg->regs;
	int retval;

	/* Enable the GCLK */
	GCLK->CLKCTRL.reg = cfg->gclk_clkctrl_id | GCLK_CLKCTRL_GEN_GCLK0 |
			    GCLK_CLKCTRL_CLKEN;

	/* Enable SERCOM clock in PM */
	PM->APBCMASK.reg |= cfg->pm_apbcmask;

	/* Disable all I2C interrupts */
	i2c->INTENCLR.reg = SERCOM_I2CM_INTENCLR_MASK;

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

	k_sem_init(&data->sem, 0, 1);

	cfg->irq_config_func(dev);

#ifdef CONFIG_I2C_SAM0_DMA_DRIVEN

	data->dma = device_get_binding(CONFIG_DMA_0_NAME);

#endif

	i2c->CTRLA.bit.ENABLE = 1;
	wait_synchronization(i2c);

	/* Force bus idle */
	i2c->STATUS.bit.BUSSTATE = 1;
	wait_synchronization(i2c);

	return 0;
}


static const struct i2c_driver_api i2c_sam0_driver_api = {
	.configure = i2c_sam0_configure,
	.transfer = i2c_sam0_transfer,
};

#ifdef CONFIG_I2C_SAM0_DMA_DRIVEN
#ifndef DT_ATMEL_SAM0_I2C_SERCOM_0_DMA
#define DT_ATMEL_SAM0_I2C_SERCOM_0_DMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_I2C_SERCOM_1_DMA
#define DT_ATMEL_SAM0_I2C_SERCOM_1_DMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_I2C_SERCOM_2_DMA
#define DT_ATMEL_SAM0_I2C_SERCOM_2_DMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_I2C_SERCOM_3_DMA
#define DT_ATMEL_SAM0_I2C_SERCOM_3_DMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_I2C_SERCOM_4_DMA
#define DT_ATMEL_SAM0_I2C_SERCOM_4_DMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_I2C_SERCOM_5_DMA
#define DT_ATMEL_SAM0_I2C_SERCOM_5_DMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_I2C_SERCOM_6_DMA
#define DT_ATMEL_SAM0_I2C_SERCOM_6_DMA 0xFF
#endif
#ifndef DT_ATMEL_SAM0_I2C_SERCOM_7_DMA
#define DT_ATMEL_SAM0_I2C_SERCOM_7_DMA 0xFF
#endif

#define I2C_SAM0_DMA_CHANNELS(n)		     \
	.write_dma_request = SERCOM##n##_DMAC_ID_TX, \
	.read_dma_request = SERCOM##n##_DMAC_ID_RX,  \
	.dma_channel = DT_ATMEL_SAM0_I2C_SERCOM_##n##_DMA,
#else
#define I2C_SAM0_DMA_CHANNELS(n)
#endif

#define I2C_SAM0_DEVICE(n)						    \
	static void i2c_sam_irq_config_##n(struct device *dev);		    \
	static const struct i2c_sam0_dev_config i2c_sam0_dev_config_##n = { \
		.regs = (SercomI2cm *)DT_ATMEL_SAM0_I2C_SERCOM_##n##_BASE_ADDRESS, \
		.bitrate = DT_ATMEL_SAM0_I2C_SERCOM_##n##_CLOCK_FREQUENCY,  \
		.pm_apbcmask = PM_APBCMASK_SERCOM##n,			    \
		.gclk_clkctrl_id = GCLK_CLKCTRL_ID_SERCOM##n##_CORE,	    \
		.irq_config_func = &i2c_sam_irq_config_##n,		    \
		I2C_SAM0_DMA_CHANNELS(n)				    \
	};								    \
	static struct i2c_sam0_dev_data i2c_sam0_dev_data_##n;		    \
	DEVICE_AND_API_INIT(i2c_sam0_##n,				    \
			    DT_ATMEL_SAM0_I2C_SERCOM_##n##_LABEL,	    \
			    &i2c_sam0_initialize, &i2c_sam0_dev_data_##n,   \
			    &i2c_sam0_dev_config_##n, POST_KERNEL,	    \
			    CONFIG_I2C_INIT_PRIORITY, &i2c_sam0_driver_api);\
	static void i2c_sam_irq_config_##n(struct device *dev)		    \
	{								    \
		IRQ_CONNECT(DT_ATMEL_SAM0_I2C_SERCOM_##n##_IRQ_0,	    \
			    DT_ATMEL_SAM0_I2C_SERCOM_##n##_IRQ_0_PRIORITY,  \
			    i2c_sam0_isr, DEVICE_GET(i2c_sam0_##n),	    \
			    0);						    \
		irq_enable(DT_ATMEL_SAM0_I2C_SERCOM_##n##_IRQ_0);	    \
	}

#if DT_ATMEL_SAM0_I2C_SERCOM_0_BASE_ADDRESS
I2C_SAM0_DEVICE(0);
#endif

#if DT_ATMEL_SAM0_I2C_SERCOM_1_BASE_ADDRESS
I2C_SAM0_DEVICE(1);
#endif

#if DT_ATMEL_SAM0_I2C_SERCOM_2_BASE_ADDRESS
I2C_SAM0_DEVICE(2);
#endif

#if DT_ATMEL_SAM0_I2C_SERCOM_3_BASE_ADDRESS
I2C_SAM0_DEVICE(3);
#endif

#if DT_ATMEL_SAM0_I2C_SERCOM_4_BASE_ADDRESS
I2C_SAM0_DEVICE(4);
#endif

#if DT_ATMEL_SAM0_I2C_SERCOM_5_BASE_ADDRESS
I2C_SAM0_DEVICE(5);
#endif

#if DT_ATMEL_SAM0_I2C_SERCOM_6_BASE_ADDRESS
I2C_SAM0_DEVICE(6);
#endif

#if DT_ATMEL_SAM0_I2C_SERCOM_7_BASE_ADDRESS
I2C_SAM0_DEVICE(7);
#endif
