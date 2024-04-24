/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright © 2023 Calian Ltd.  All rights reserved.
 *
 * Driver for the Xilinx AXI IIC Bus Interface.
 * This is an FPGA logic core as described by Xilinx document PG090.
 */

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(i2c_xilinx_axi, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"
#include "i2c_xilinx_axi.h"

struct i2c_xilinx_axi_config {
	mem_addr_t base;
	void (*irq_config_func)(const struct device *dev);
	/* Whether device has working dynamic read (broken prior to core rev. 2.1) */
	bool dyn_read_working;
};

struct i2c_xilinx_axi_data {
	struct k_event irq_event;
	/* Serializes between ISR and other calls */
	struct k_spinlock lock;
	/* Provides exclusion against multiple concurrent requests */
	struct k_mutex mutex;

#if defined(CONFIG_I2C_TARGET)
	struct i2c_target_config *target_cfg;
	bool target_reading;
	bool target_read_aborted;
	bool target_writing;
#endif
};

#if defined(CONFIG_I2C_TARGET)

#define I2C_XILINX_AXI_TARGET_INTERRUPTS                                                           \
	(ISR_ADDR_TARGET | ISR_NOT_ADDR_TARGET | ISR_RX_FIFO_FULL | ISR_TX_FIFO_EMPTY |            \
	 ISR_TX_ERR_TARGET_COMP)

static int i2c_xilinx_axi_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	const struct i2c_xilinx_axi_config *config = dev->config;
	struct i2c_xilinx_axi_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t int_enable;
	uint32_t int_status;
	int ret;

	if (cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) {
		/* Optionally supported in core, but not implemented in driver yet */
		return -EOPNOTSUPP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	key = k_spin_lock(&data->lock);

	if (data->target_cfg) {
		ret = -EBUSY;
		goto out_unlock;
	}

	data->target_cfg = cfg;

	int_status = sys_read32(config->base + REG_ISR);
	if (int_status & I2C_XILINX_AXI_TARGET_INTERRUPTS) {
		sys_write32(int_status & I2C_XILINX_AXI_TARGET_INTERRUPTS, config->base + REG_ISR);
	}

	sys_write32(CR_EN, config->base + REG_CR);
	int_enable = sys_read32(config->base + REG_IER);
	int_enable |= ISR_ADDR_TARGET;
	sys_write32(int_enable, config->base + REG_IER);

	sys_write32(cfg->address << 1, config->base + REG_ADR);
	sys_write32(0, config->base + REG_RX_FIFO_PIRQ);

	ret = 0;

out_unlock:
	k_spin_unlock(&data->lock, key);
	LOG_DBG("Target register ret=%d", ret);
	k_mutex_unlock(&data->mutex);
	return ret;
}

static int i2c_xilinx_axi_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	const struct i2c_xilinx_axi_config *config = dev->config;
	struct i2c_xilinx_axi_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t int_enable;
	int ret;

	k_mutex_lock(&data->mutex, K_FOREVER);
	key = k_spin_lock(&data->lock);

	if (!data->target_cfg) {
		ret = -EINVAL;
		goto out_unlock;
	}

	if (data->target_reading || data->target_writing) {
		ret = -EBUSY;
		goto out_unlock;
	}

	data->target_cfg = NULL;
	sys_write32(0, config->base + REG_ADR);

	sys_write32(CR_EN, config->base + REG_CR);
	int_enable = sys_read32(config->base + REG_IER);
	int_enable &= ~I2C_XILINX_AXI_TARGET_INTERRUPTS;
	sys_write32(int_enable, config->base + REG_IER);

	ret = 0;

out_unlock:
	k_spin_unlock(&data->lock, key);
	LOG_DBG("Target unregister ret=%d", ret);
	k_mutex_unlock(&data->mutex);
	return ret;
}

static void i2c_xilinx_axi_target_isr(const struct i2c_xilinx_axi_config *config,
				      struct i2c_xilinx_axi_data *data, uint32_t int_status,
				      uint32_t *ints_to_clear, uint32_t *int_enable)
{
	if (int_status & ISR_ADDR_TARGET) {
		LOG_DBG("Addressed as target");
		*int_enable &= ~ISR_ADDR_TARGET;
		*int_enable |= ISR_NOT_ADDR_TARGET;
		*ints_to_clear |= ISR_NOT_ADDR_TARGET;

		if (sys_read32(config->base + REG_SR) & SR_SRW) {
			uint8_t read_byte;

			data->target_reading = true;
			*ints_to_clear |= ISR_TX_FIFO_EMPTY | ISR_TX_ERR_TARGET_COMP;
			*int_enable |= ISR_TX_FIFO_EMPTY | ISR_TX_ERR_TARGET_COMP;
			if ((*data->target_cfg->callbacks->read_requested)(data->target_cfg,
									   &read_byte)) {
				LOG_DBG("target read_requested rejected");
				data->target_read_aborted = true;
				read_byte = 0xFF;
			}
			sys_write32(read_byte, config->base + REG_TX_FIFO);
		} else {
			data->target_writing = true;
			*int_enable |= ISR_RX_FIFO_FULL;
			if ((*data->target_cfg->callbacks->write_requested)(data->target_cfg)) {
				uint32_t cr = sys_read32(config->base + REG_CR);

				LOG_DBG("target write_requested rejected");
				cr |= CR_TXAK;
				sys_write32(cr, config->base + REG_CR);
			}
		}
	} else if (int_status & ISR_NOT_ADDR_TARGET) {
		LOG_DBG("Not addressed as target");
		(*data->target_cfg->callbacks->stop)(data->target_cfg);
		data->target_reading = false;
		data->target_read_aborted = false;
		data->target_writing = false;

		sys_write32(CR_EN, config->base + REG_CR);
		*int_enable &= ~I2C_XILINX_AXI_TARGET_INTERRUPTS;
		*int_enable |= ISR_ADDR_TARGET;
		*ints_to_clear |= ISR_ADDR_TARGET;
	} else if (int_status & ISR_RX_FIFO_FULL) {
		const uint8_t written_byte =
			sys_read32(config->base + REG_RX_FIFO) & RX_FIFO_DATA_MASK;

		if ((*data->target_cfg->callbacks->write_received)(data->target_cfg,
								   written_byte)) {
			uint32_t cr = sys_read32(config->base + REG_CR);

			LOG_DBG("target write_received rejected");
			cr |= CR_TXAK;
			sys_write32(cr, config->base + REG_CR);
		}
	} else if (int_status & ISR_TX_ERR_TARGET_COMP) {
		if (data->target_reading) {
			/* Controller has NAKed the last byte read, so no more to send.
			 * Ignore TX FIFO empty so we don't write an extra byte.
			 */
			LOG_DBG("target read completed");
			*int_enable &= ~ISR_TX_FIFO_EMPTY;
			*ints_to_clear |= ISR_TX_FIFO_EMPTY;
		} else {
			LOG_WRN("Unexpected TX complete");
		}
	} else if (int_status & ISR_TX_FIFO_EMPTY) {
		if (data->target_reading) {
			uint8_t read_byte = 0xFF;

			if (!data->target_read_aborted &&
			    (*data->target_cfg->callbacks->read_processed)(data->target_cfg,
									   &read_byte)) {
				LOG_DBG("target read_processed rejected");
				data->target_read_aborted = true;
			}
			sys_write32(read_byte, config->base + REG_TX_FIFO);
		} else {
			LOG_WRN("Unexpected TX empty");
		}
	}
}
#endif

static void i2c_xilinx_axi_isr(const struct device *dev)
{
	const struct i2c_xilinx_axi_config *config = dev->config;
	struct i2c_xilinx_axi_data *data = dev->data;
	const k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t int_enable = sys_read32(config->base + REG_IER);
	const uint32_t int_status = sys_read32(config->base + REG_ISR) & int_enable;
	uint32_t ints_to_clear = int_status;
	uint32_t ints_to_mask = int_status;

	LOG_DBG("ISR called for 0x%08" PRIxPTR ", status 0x%02x", config->base, int_status);

	if (int_status & ISR_ARB_LOST) {
		/* Must clear MSMS before clearing interrupt */
		uint32_t cr = sys_read32(config->base + REG_CR);

		cr &= ~CR_MSMS;
		sys_write32(cr, config->base + REG_CR);
	}

#if defined(CONFIG_I2C_TARGET)
	if (data->target_cfg && (int_status & I2C_XILINX_AXI_TARGET_INTERRUPTS)) {
		ints_to_mask &= ~(int_status & I2C_XILINX_AXI_TARGET_INTERRUPTS);
		i2c_xilinx_axi_target_isr(config, data, int_status, &ints_to_clear, &int_enable);
	}
#endif

	sys_write32(int_enable & ~ints_to_mask, config->base + REG_IER);
	/* Be careful, writing 1 to a bit that is not currently set in ISR will SET it! */
	sys_write32(ints_to_clear & sys_read32(config->base + REG_ISR), config->base + REG_ISR);

	k_spin_unlock(&data->lock, key);
	k_event_post(&data->irq_event, int_status);
}

static void i2c_xilinx_axi_reinit(const struct i2c_xilinx_axi_config *config)
{
	LOG_DBG("Controller reinit");
	sys_write32(SOFTR_KEY, config->base + REG_SOFTR);
	sys_write32(CR_TX_FIFO_RST, config->base + REG_CR);
	sys_write32(CR_EN, config->base + REG_CR);
	sys_write32(GIE_ENABLE, config->base + REG_GIE);
}

static int i2c_xilinx_axi_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_xilinx_axi_config *config = dev->config;

	LOG_INF("Configuring %s at 0x%08" PRIxPTR, dev->name, config->base);
	i2c_xilinx_axi_reinit(config);
	return 0;
}

static uint32_t i2c_xilinx_axi_wait_interrupt(const struct i2c_xilinx_axi_config *config,
					      struct i2c_xilinx_axi_data *data, uint32_t int_mask)
{
	const k_spinlock_key_t key = k_spin_lock(&data->lock);
	const uint32_t int_enable = sys_read32(config->base + REG_IER) | int_mask;
	uint32_t events;

	LOG_DBG("Set IER to 0x%02x", int_enable);
	sys_write32(int_enable, config->base + REG_IER);
	k_event_clear(&data->irq_event, int_mask);
	k_spin_unlock(&data->lock, key);

	events = k_event_wait(&data->irq_event, int_mask, false, K_MSEC(100));

	LOG_DBG("Got ISR events 0x%02x", events);
	if (!events) {
		LOG_ERR("Timeout waiting for ISR events 0x%02x, SR 0x%02x, ISR 0x%02x", int_mask,
			sys_read32(config->base + REG_SR), sys_read32(config->base + REG_ISR));
	}
	return events;
}

static void i2c_xilinx_axi_clear_interrupt(const struct i2c_xilinx_axi_config *config,
					   struct i2c_xilinx_axi_data *data, uint32_t int_mask)
{
	const k_spinlock_key_t key = k_spin_lock(&data->lock);
	const uint32_t int_status = sys_read32(config->base + REG_ISR);

	if (int_status & int_mask) {
		sys_write32(int_status & int_mask, config->base + REG_ISR);
	}
	k_spin_unlock(&data->lock, key);
}

static int i2c_xilinx_axi_wait_rx_full(const struct i2c_xilinx_axi_config *config,
				       struct i2c_xilinx_axi_data *data, uint32_t read_bytes)
{
	uint32_t events;

	i2c_xilinx_axi_clear_interrupt(config, data, ISR_RX_FIFO_FULL);
	if (!(sys_read32(config->base + REG_SR) & SR_RX_FIFO_EMPTY) &&
	    (sys_read32(config->base + REG_RX_FIFO_OCY) & RX_FIFO_OCY_MASK) + 1 >= read_bytes) {
		LOG_DBG("RX already full on checking, SR 0x%02x RXOCY 0x%02x",
			sys_read32(config->base + REG_SR),
			sys_read32(config->base + REG_RX_FIFO_OCY));
		return 0;
	}
	events = i2c_xilinx_axi_wait_interrupt(config, data, ISR_RX_FIFO_FULL | ISR_ARB_LOST);
	if (!events) {
		return -ETIMEDOUT;
	}
	if (events & ISR_ARB_LOST) {
		LOG_ERR("Arbitration lost on RX");
		return -ENXIO;
	}
	return 0;
}

static int i2c_xilinx_axi_read_nondyn(const struct i2c_xilinx_axi_config *config,
				      struct i2c_xilinx_axi_data *data, struct i2c_msg *msg,
				      uint16_t addr)
{
	uint8_t *read_ptr = msg->buf;
	uint32_t bytes_left = msg->len;
	uint32_t cr = CR_EN | CR_MSMS;

	if (!bytes_left) {
		return -EINVAL;
	}
	if (bytes_left == 1) {
		/* Set TXAK bit now, to NAK after the first byte is received */
		cr |= CR_TXAK;
	}

	/**
	 * The Xilinx core's RX FIFO full logic seems rather broken in that the interrupt
	 * is triggered, and the I2C receive is throttled, only when the FIFO occupancy
	 * equals the PIRQ threshold, not when greater or equal. In the non-dynamic mode
	 * of operation, we need to stop the read prior to the last bytes being received
	 * from the target in order to set the TXAK bit and clear MSMS to terminate the
	 * receive properly.
	 * However, if we previously allowed multiple bytes into the RX FIFO, this requires
	 * reducing the PIRQ threshold to 0 (single byte) during the receive operation. This
	 * can cause the receive to unthrottle (since FIFO occupancy now exceeds PIRQ
	 * threshold) and depending on timing between the driver code and the core,
	 * this can cause the core to try to receive more data into the FIFO than desired
	 * and cause various unexpected results.
	 *
	 * To avoid this, we only receive one byte at a time in the non-dynamic mode.
	 * Dynamic mode doesn't have this issue as it provides the RX byte count to the
	 * controller specifically and the TXAK and MSMS bits are handled automatically.
	 */
	sys_write32(0, config->base + REG_RX_FIFO_PIRQ);

	if (msg->flags & I2C_MSG_RESTART) {
		cr |= CR_RSTA;

		sys_write32(cr, config->base + REG_CR);
		sys_write32((addr << 1) | I2C_MSG_READ, config->base + REG_TX_FIFO);
	} else {
		sys_write32((addr << 1) | I2C_MSG_READ, config->base + REG_TX_FIFO);
		sys_write32(cr, config->base + REG_CR);
	}

	while (bytes_left) {
		int ret = i2c_xilinx_axi_wait_rx_full(config, data, 1);

		if (ret) {
			return ret;
		}

		if (bytes_left == 2) {
			/* Set TXAK so the last byte is NAKed */
			cr |= CR_TXAK;
		} else if (bytes_left == 1 && (msg->flags & I2C_MSG_STOP)) {
			/* Before reading the last byte, clear MSMS to issue a stop if required */
			cr &= ~CR_MSMS;
		}
		cr &= ~CR_RSTA;
		sys_write32(cr, config->base + REG_CR);

		*read_ptr++ = sys_read32(config->base + REG_RX_FIFO) & RX_FIFO_DATA_MASK;
		bytes_left--;
	}
	return 0;
}

static int i2c_xilinx_axi_read_dyn(const struct i2c_xilinx_axi_config *config,
				   struct i2c_xilinx_axi_data *data, struct i2c_msg *msg,
				   uint16_t addr)
{
	uint8_t *read_ptr = msg->buf;
	uint32_t bytes_left = msg->len;
	uint32_t bytes_to_read = bytes_left;
	uint32_t cr = CR_EN;
	uint32_t len_word = bytes_left;

	if (!bytes_left || bytes_left > MAX_DYNAMIC_READ_LEN) {
		return -EINVAL;
	}
	if (msg->flags & I2C_MSG_RESTART) {
		cr |= CR_MSMS | CR_RSTA;
	}
	sys_write32(cr, config->base + REG_CR);

	if (bytes_to_read > FIFO_SIZE) {
		bytes_to_read = FIFO_SIZE;
	}
	sys_write32(bytes_to_read - 1, config->base + REG_RX_FIFO_PIRQ);
	sys_write32((addr << 1) | I2C_MSG_READ | TX_FIFO_START, config->base + REG_TX_FIFO);

	if (msg->flags & I2C_MSG_STOP) {
		len_word |= TX_FIFO_STOP;
	}
	sys_write32(len_word, config->base + REG_TX_FIFO);

	while (bytes_left) {
		int ret;

		bytes_to_read = bytes_left;
		if (bytes_to_read > FIFO_SIZE) {
			bytes_to_read = FIFO_SIZE;
		}

		sys_write32(bytes_to_read - 1, config->base + REG_RX_FIFO_PIRQ);
		ret = i2c_xilinx_axi_wait_rx_full(config, data, bytes_to_read);
		if (ret) {
			return ret;
		}

		while (bytes_to_read) {
			*read_ptr++ = sys_read32(config->base + REG_RX_FIFO) & RX_FIFO_DATA_MASK;
			bytes_to_read--;
			bytes_left--;
		}
	}
	return 0;
}

static int i2c_xilinx_axi_wait_tx_done(const struct i2c_xilinx_axi_config *config,
				       struct i2c_xilinx_axi_data *data)
{
	const uint32_t finish_bits = ISR_BUS_NOT_BUSY | ISR_TX_FIFO_EMPTY;

	uint32_t events = i2c_xilinx_axi_wait_interrupt(
		config, data, finish_bits | ISR_TX_ERR_TARGET_COMP | ISR_ARB_LOST);
	if (!(events & finish_bits) || (events & ~finish_bits)) {
		if (!events) {
			return -ETIMEDOUT;
		}
		if (events & ISR_ARB_LOST) {
			LOG_ERR("Arbitration lost on TX");
			return -EAGAIN;
		}
		LOG_ERR("TX received NAK");
		return -ENXIO;
	}
	return 0;
}

static int i2c_xilinx_axi_wait_not_busy(const struct i2c_xilinx_axi_config *config,
					struct i2c_xilinx_axi_data *data)
{
	if (sys_read32(config->base + REG_SR) & SR_BB) {
		uint32_t events = i2c_xilinx_axi_wait_interrupt(config, data, ISR_BUS_NOT_BUSY);

		if (events != ISR_BUS_NOT_BUSY) {
			LOG_ERR("Bus stuck busy");
			i2c_xilinx_axi_reinit(config);
			return -EBUSY;
		}
	}
	return 0;
}

static int i2c_xilinx_axi_write(const struct i2c_xilinx_axi_config *config,
				struct i2c_xilinx_axi_data *data, const struct i2c_msg *msg,
				uint16_t addr)
{
	const uint8_t *write_ptr = msg->buf;
	uint32_t bytes_left = msg->len;
	uint32_t cr = CR_EN | CR_TX;
	uint32_t fifo_space = FIFO_SIZE - 1; /* account for address being written */

	if (msg->flags & I2C_MSG_RESTART) {
		cr |= CR_MSMS | CR_RSTA;
	}

	i2c_xilinx_axi_clear_interrupt(config, data, ISR_TX_ERR_TARGET_COMP | ISR_ARB_LOST);

	sys_write32(cr, config->base + REG_CR);
	sys_write32((addr << 1) | TX_FIFO_START, config->base + REG_TX_FIFO);

	/* TX FIFO empty detection is somewhat fragile because the status register
	 * TX_FIFO_EMPTY bit can be set prior to the transaction actually being
	 * complete, so we have to rely on the TX empty interrupt.
	 * However, delays in writing data to the TX FIFO could cause it
	 * to run empty in the middle of the process, causing us to get a spurious
	 * completion detection from the interrupt. Therefore we disable interrupts
	 * while the TX FIFO is being filled up to try to avoid this.
	 */

	while (bytes_left) {
		uint32_t bytes_to_send = bytes_left;
		const k_spinlock_key_t key = k_spin_lock(&data->lock);
		int ret;

		if (bytes_to_send > fifo_space) {
			bytes_to_send = fifo_space;
		}
		while (bytes_to_send) {
			uint32_t write_word = *write_ptr++;

			if (bytes_left == 1 && (msg->flags & I2C_MSG_STOP)) {
				write_word |= TX_FIFO_STOP;
			}
			sys_write32(write_word, config->base + REG_TX_FIFO);
			bytes_to_send--;
			bytes_left--;
		}
		i2c_xilinx_axi_clear_interrupt(config, data, ISR_TX_FIFO_EMPTY | ISR_BUS_NOT_BUSY);
		k_spin_unlock(&data->lock, key);

		ret = i2c_xilinx_axi_wait_tx_done(config, data);
		if (ret) {
			return ret;
		}
		fifo_space = FIFO_SIZE;
	}
	return 0;
}

static int i2c_xilinx_axi_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				   uint16_t addr)
{
	const struct i2c_xilinx_axi_config *config = dev->config;
	struct i2c_xilinx_axi_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->mutex, K_FOREVER);

	/**
	 * Reinitializing before each transfer shouldn't technically be needed, but
	 * seems to improve general reliability. The Linux driver also does this.
	 */
	i2c_xilinx_axi_reinit(config);

	ret = i2c_xilinx_axi_wait_not_busy(config, data);
	if (ret) {
		goto out_unlock;
	}

	if (!num_msgs) {
		goto out_unlock;
	}

	do {
		if (msgs->flags & I2C_MSG_ADDR_10_BITS) {
			/* Optionally supported in core, but not implemented in driver yet */
			ret = -EOPNOTSUPP;
			goto out_unlock;
		}
		if (msgs->flags & I2C_MSG_READ) {
			if (config->dyn_read_working && msgs->len <= MAX_DYNAMIC_READ_LEN) {
				ret = i2c_xilinx_axi_read_dyn(config, data, msgs, addr);
			} else {
				ret = i2c_xilinx_axi_read_nondyn(config, data, msgs, addr);
			}
		} else {
			ret = i2c_xilinx_axi_write(config, data, msgs, addr);
		}
		if (!ret && (msgs->flags & I2C_MSG_STOP)) {
			ret = i2c_xilinx_axi_wait_not_busy(config, data);
		}
		if (ret) {
			goto out_unlock;
		}
		msgs++;
		num_msgs--;
	} while (num_msgs);

out_unlock:
	k_mutex_unlock(&data->mutex);
	return ret;
}

static int i2c_xilinx_axi_init(const struct device *dev)
{
	const struct i2c_xilinx_axi_config *config = dev->config;
	struct i2c_xilinx_axi_data *data = dev->data;
	int error;

	k_event_init(&data->irq_event);
	k_mutex_init(&data->mutex);

	error = i2c_xilinx_axi_configure(dev, I2C_MODE_CONTROLLER);
	if (error) {
		return error;
	}

	config->irq_config_func(dev);

	LOG_INF("initialized");
	return 0;
}

static const struct i2c_driver_api i2c_xilinx_axi_driver_api = {
	.configure = i2c_xilinx_axi_configure,
	.transfer = i2c_xilinx_axi_transfer,
#if defined(CONFIG_I2C_TARGET)
	.target_register = i2c_xilinx_axi_target_register,
	.target_unregister = i2c_xilinx_axi_target_unregister,
#endif
};

#define I2C_XILINX_AXI_INIT(n, compat)                                                             \
	static void i2c_xilinx_axi_config_func_##compat##_##n(const struct device *dev);           \
                                                                                                   \
	static const struct i2c_xilinx_axi_config i2c_xilinx_axi_config_##compat##_##n = {         \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irq_config_func = i2c_xilinx_axi_config_func_##compat##_##n,                      \
		.dyn_read_working = DT_INST_NODE_HAS_COMPAT(n, xlnx_xps_iic_2_1)};                 \
                                                                                                   \
	static struct i2c_xilinx_axi_data i2c_xilinx_axi_data_##compat##_##n;                      \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_xilinx_axi_init, NULL,                                    \
				  &i2c_xilinx_axi_data_##compat##_##n,                             \
				  &i2c_xilinx_axi_config_##compat##_##n, POST_KERNEL,              \
				  CONFIG_I2C_INIT_PRIORITY, &i2c_xilinx_axi_driver_api);           \
                                                                                                   \
	static void i2c_xilinx_axi_config_func_##compat##_##n(const struct device *dev)            \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_xilinx_axi_isr,         \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define DT_DRV_COMPAT xlnx_xps_iic_2_1
DT_INST_FOREACH_STATUS_OKAY_VARGS(I2C_XILINX_AXI_INIT, DT_DRV_COMPAT)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT xlnx_xps_iic_2_00_a
DT_INST_FOREACH_STATUS_OKAY_VARGS(I2C_XILINX_AXI_INIT, DT_DRV_COMPAT)
