/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file I2C driver for Quark SE Sensor Subsystem.
 *
 * The I2C on Quark SE Sensor Subsystem is similar to DesignWare I2C IP block,
 * but with a different register set and different workflow.
 */

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#include <i2c.h>
#include <nanokernel.h>
#include <arch/cpu.h>

#include <sys_io.h>

#include <board.h>
#include <misc/util.h>

#include "i2c_quark_se_ss.h"
#include "i2c_quark_se_ss_registers.h"

#ifndef CONFIG_I2C_DEBUG
#define DBG(...) { ; }
#else
#include <misc/printk.h>
#define DBG printk
#endif /* CONFIG_I2C_DEBUG */

static inline uint32_t _i2c_qse_ss_memory_read(uint32_t base_addr,
					       uint32_t offset)
{
	return sys_read32(base_addr + offset);
}


static inline void _i2c_qse_ss_memory_write(uint32_t base_addr,
					    uint32_t offset, uint32_t val)
{
	sys_write32(val, base_addr + offset);
}

static inline uint32_t _i2c_qse_ss_reg_read(struct device *dev,
					    uint32_t reg)
{
	struct i2c_qse_ss_rom_config * const rom = dev->config->config_info;

	return _arc_v2_aux_reg_read(rom->base_address + reg);
}

static inline void _i2c_qse_ss_reg_write(struct device *dev,
					 uint32_t reg, uint32_t val)
{
	struct i2c_qse_ss_rom_config * const rom = dev->config->config_info;

	_arc_v2_aux_reg_write(rom->base_address + reg, val);
}

static inline void _i2c_qse_ss_reg_write_and(struct device *dev,
					     uint32_t reg, uint32_t mask)
{
	uint32_t r;

	r = _i2c_qse_ss_reg_read(dev, reg);
	r &= mask;
	_i2c_qse_ss_reg_write(dev, reg, r);
}

static inline void _i2c_qse_ss_reg_write_or(struct device *dev,
					    uint32_t reg, uint32_t mask)
{
	uint32_t r;

	r = _i2c_qse_ss_reg_read(dev, reg);
	r |= mask;
	_i2c_qse_ss_reg_write(dev, reg, r);
}

static inline int _i2c_qse_ss_reg_check_bit(struct device *dev,
					    uint32_t reg, uint32_t mask)
{
	return _i2c_qse_ss_reg_read(dev, reg) & mask;
}

/* Is the controller busy? */
static inline bool _i2c_qse_ss_is_busy(struct device *dev)
{
	return _i2c_qse_ss_reg_check_bit(dev, REG_STATUS, IC_STATUS_ACTIVITY);
}

/* Is RX FIFO not empty? */
static inline bool _i2c_qse_ss_is_rfne(struct device *dev)
{
	return _i2c_qse_ss_reg_check_bit(dev, REG_STATUS, IC_STATUS_RFNE);
}

/* Is TX FIFO not full? */
static inline bool _i2c_qse_ss_is_tfnf(struct device *dev)
{
	return _i2c_qse_ss_reg_check_bit(dev, REG_STATUS, IC_STATUS_TFNF);
}

/* Is TX FIFO empty? */
static inline bool _i2c_qse_ss_is_tfe(struct device *dev)
{
	return _i2c_qse_ss_reg_check_bit(dev, REG_STATUS, IC_STATUS_TFE);
}

/* Check a certain bit in the interrupt register */
static inline bool _i2c_qse_ss_check_irq(struct device *dev, uint32_t mask)
{
	return _i2c_qse_ss_reg_check_bit(dev, REG_INTR_STAT, mask);
}

static inline void _i2c_qse_ss_data_ask(struct device *dev)
{
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;
	uint32_t data;
	uint8_t tx_empty;
	int8_t rx_empty;
	uint8_t cnt;

	/* No more bytes to request, so command queue is no longer needed */
	if (dw->request_bytes == 0) {
		_i2c_qse_ss_reg_write_and(dev, REG_INTR_MASK,
					  ~(IC_INTR_TX_EMPTY));

		return;
	}

	/* How many bytes we can actually ask */
	rx_empty = I2C_QSE_SS_FIFO_DEPTH
		   - _i2c_qse_ss_reg_read(dev, REG_RXFLR);
	rx_empty -= dw->rx_pending;

	if (rx_empty < 0) {
		/* RX FIFO expected to be full.
		 * So don't request any bytes, yet.
		 */
		return;
	}

	/* How many empty slots in TX FIFO (as command queue) */
	tx_empty = I2C_QSE_SS_FIFO_DEPTH
		   - _i2c_qse_ss_reg_read(dev, REG_TXFLR);

	/* Figure out how many bytes we can request */
	cnt = min(I2C_QSE_SS_FIFO_DEPTH, dw->request_bytes);
	cnt = min(min(tx_empty, rx_empty), cnt);

	while (cnt > 0) {
		/* Tell controller to get another byte */
		data = IC_DATA_CMD_CMD | IC_DATA_CMD_STROBE | IC_DATA_CMD_POP;

		/* Send RESTART if needed */
		if (dw->xfr_flags & I2C_MSG_RESTART) {
			data |= IC_DATA_CMD_RESTART;
			dw->xfr_flags &= ~(I2C_MSG_RESTART);
		}

		/* After receiving the last byte, send STOP if needed */
		if ((dw->xfr_flags & I2C_MSG_STOP)
		    && (dw->request_bytes == 1)) {
			data |= IC_DATA_CMD_STOP;
		}

		_i2c_qse_ss_reg_write(dev, REG_DATA_CMD, data);

		dw->rx_pending++;
		dw->request_bytes--;
		cnt--;
	}
}

static void _i2c_qse_ss_data_read(struct device *dev)
{
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;

	while (_i2c_qse_ss_is_rfne(dev) && (dw->xfr_len > 0)) {
		/* Need to write 0 to POP bit to
		 * "pop" one byte from RX FIFO.
		 */
		_i2c_qse_ss_reg_write(dev, REG_DATA_CMD,
				      IC_DATA_CMD_STROBE);

		dw->xfr_buf[0] = _i2c_qse_ss_reg_read(dev, REG_DATA_CMD)
				 & IC_DATA_CMD_DATA_MASK;

		dw->xfr_buf++;
		dw->xfr_len--;
		dw->rx_pending--;

		if (dw->xfr_len == 0) {
			break;
		}
	}

	/* Nothing to receive anymore */
	if (dw->xfr_len == 0) {
		dw->state &= ~I2C_QSE_SS_CMD_RECV;
		return;
	}
}

static int _i2c_qse_ss_data_send(struct device *dev)
{
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;
	uint32_t data;

	/* Nothing to send anymore, mask the interrupt */
	if (dw->xfr_len == 0) {
		_i2c_qse_ss_reg_write_and(dev, REG_INTR_MASK,
					  ~(IC_INTR_TX_EMPTY));

		dw->state &= ~I2C_QSE_SS_CMD_SEND;

		return 0;
	}

	while (_i2c_qse_ss_is_tfnf(dev) && (dw->xfr_len > 0)) {
		/* We have something to transmit to a specific host */
		data = dw->xfr_buf[0] | IC_DATA_CMD_STROBE | IC_DATA_CMD_POP;

		/* Send RESTART if needed */
		if (dw->xfr_flags & I2C_MSG_RESTART) {
			data |= IC_DATA_CMD_RESTART;
			dw->xfr_flags &= ~(I2C_MSG_RESTART);
		}

		/* Send STOP if needed */
		if ((dw->xfr_len == 1) && (dw->xfr_flags & I2C_MSG_STOP)) {
			data |= IC_DATA_CMD_STOP;
		}

		_i2c_qse_ss_reg_write(dev, REG_DATA_CMD, data);

		dw->xfr_len--;
		dw->xfr_buf++;

		if (_i2c_qse_ss_check_irq(dev, IC_INTR_TX_ABRT)) {
			return -EIO;
		}

	}

	return 0;
}

static inline void _i2c_qse_ss_transfer_complete(struct device *dev)
{
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;

	/* Disable and clear all pending interrupts */
	_i2c_qse_ss_reg_write(dev, REG_INTR_MASK, IC_INTR_MASK_ALL);
	_i2c_qse_ss_reg_write(dev, REG_INTR_CLR, IC_INTR_CLR_ALL);

	device_sync_call_complete(&dw->sync);
}


void i2c_qse_ss_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;
	uint32_t ic_intr_stat;
	int ret = 0;

	/*
	 * Causes of an intterrupts:
	 *   - STOP condition is detected
	 *   - Transfer is aborted
	 *   - Transmit FIFO is empy
	 *   - Transmit FIFO is overflowing
	 *   - Receive FIFO is full
	 *   - Receive FIFO overflow
	 *   - Received FIFO underrun
	 */

	DBG("I2C_SS: interrupt received\n");

	ic_intr_stat = _i2c_qse_ss_reg_read(dev, REG_INTR_STAT);

	/* Error conditions */
	if ((IC_INTR_TX_ABRT | IC_INTR_TX_OVER |
	     IC_INTR_RX_OVER | IC_INTR_RX_UNDER) &
	    ic_intr_stat) {
		dw->state = I2C_QSE_SS_CMD_ERROR;
		goto done;
	}

	/* Check if the RX FIFO reached threshold */
	if (ic_intr_stat & IC_INTR_RX_FULL) {
		_i2c_qse_ss_data_read(dev);
		_i2c_qse_ss_reg_write(dev, REG_INTR_CLR, IC_INTR_RX_FULL);
	}

	/* Check if the TX FIFO is ready for commands.
	 * TX FIFO also serves as command queue where read requests
	 * are written to TX FIFO.
	 */
	if (ic_intr_stat & IC_INTR_TX_EMPTY) {
		if ((dw->xfr_flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = _i2c_qse_ss_data_send(dev);
		} else {
			_i2c_qse_ss_data_ask(dev);
		}
		_i2c_qse_ss_reg_write(dev, REG_INTR_CLR, IC_INTR_TX_EMPTY);

		/* If STOP is not expected, finish processing this
		 * message if there is nothing left to do anymore.
		 * Or bail if there is any error.
		 */
		if (((dw->xfr_len == 0)
		     && !(dw->xfr_flags & I2C_MSG_STOP))
		    || (ret != 0)) {
			goto done;
		}
	}

	/* STOP detected */
	if (ic_intr_stat & IC_INTR_STOP_DET) {
		_i2c_qse_ss_reg_write(dev, REG_INTR_CLR, IC_INTR_STOP_DET);
		goto done;
	}

	return;

done:
	_i2c_qse_ss_transfer_complete(dev);
}

static int _i2c_qse_ss_setup(struct device *dev, uint16_t addr)
{
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;
	uint32_t ic_con;
	int rc = 0;

	/* Disable the device controller but enable clock
	 * so we can setup the controller.
	 */
	_i2c_qse_ss_reg_write_and(dev, REG_CON, ~(IC_CON_ENABLE));

	/* Disable and clear all pending interrupts */
	_i2c_qse_ss_reg_write(dev, REG_INTR_MASK, IC_INTR_MASK_ALL);
	_i2c_qse_ss_reg_write(dev, REG_INTR_CLR, IC_INTR_CLR_ALL);

	ic_con = _i2c_qse_ss_reg_read(dev, REG_CON);
	ic_con &= IC_CON_SPKLEN_MASK;
	ic_con |= IC_CON_RESTART_EN | IC_CON_CLK_ENA;

	/* Set addressing mode - (initialization = 7 bit) */
	if (dw->app_config.bits.use_10_bit_addr) {
		DBG("I2C: using 10-bit address\n");
		ic_con |= IC_CON_10BIT_ADDR;
	}

	/* Setup the clock frequency and speed mode */
	switch (dw->app_config.bits.speed) {
	case I2C_SPEED_STANDARD:
		DBG("I2C: speed set to STANDARD\n");
		_i2c_qse_ss_reg_write(dev, REG_SS_SCL_CNT,
				      (dw->hcnt << 16) | (dw->lcnt & 0xFFFF));
		ic_con |= I2C_QSE_SS_SPEED_STANDARD << IC_CON_SPEED_POS;

		break;
	case I2C_SPEED_FAST:
		/* fall through */
	case I2C_SPEED_FAST_PLUS:
		DBG("I2C: speed set to FAST or FAST_PLUS\n");
		_i2c_qse_ss_reg_write(dev, REG_FS_SCL_CNT,
				      (dw->hcnt << 16) | (dw->lcnt & 0xFFFF));
		ic_con |= I2C_QSE_SS_SPEED_FAST << IC_CON_SPEED_POS;

		break;
	default:
		DBG("I2C: invalid speed requested\n");
		/* TODO change */
		rc = -EINVAL;
		goto done;
	}

	/* Set the target address */
	ic_con |= addr << IC_CON_TAR_SAR_POS;

	_i2c_qse_ss_reg_write(dev, REG_CON, ic_con);

	/* Set TX/RX fifo threshold level.
	 *
	 * RX:
	 * Setting it to 1 so RX_FULL is set whenever there is
	 * data in RX FIFO. (actual value is reg value +1)
	 *
	 * TX:
	 * Setting it to 0 so TX_EMPTY is set only when
	 * TX FIFO is truly empty. So that we can let
	 * the controller do the transfers for longer period
	 * before we need to fill the FIFO again. This may
	 * cause some pauses during transfers, but this keeps
	 * the device from interrupting often.
	 *
	 * TODO: extend the threshold for multi-byte RX FIFO.
	 */
	_i2c_qse_ss_reg_write(dev, REG_TL, 0x00000000);

	/* SDA Hold time has to setup to minimal 2 according to spec. */
	_i2c_qse_ss_reg_write(dev, REG_SDA_CONFIG, 0x00020000);

done:
	return rc;
}

static int i2c_qse_ss_intr_transfer(struct device *dev,
				    struct i2c_msg *msgs, uint8_t num_msgs,
				    uint16_t slave_address)
{
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;
	struct i2c_msg *cur_msg = msgs;
	uint8_t msg_left = num_msgs;
	uint8_t pflags;
	int ret;

	/* Why bother processing no messages */
	if (!msgs || !num_msgs) {
		return -ENOTSUP;
	}

	/* First step, check if device is idle */
	if (_i2c_qse_ss_is_busy(dev) || (dw->state & I2C_QSE_SS_BUSY)) {
		return -EBUSY;
	}

	dw->state |= I2C_QSE_SS_BUSY;

	ret = _i2c_qse_ss_setup(dev, slave_address);
	if (ret) {
		dw->state = I2C_QSE_SS_STATE_READY;
		return ret;
	}

	/* To prevent RESTART for first message */
	dw->xfr_flags = msgs[0].flags;

	/* Enable controller */
	_i2c_qse_ss_reg_write_or(dev, REG_CON, IC_CON_ENABLE);

	/* Process all the messages */
	while (msg_left > 0) {
		pflags = dw->xfr_flags;

		dw->xfr_buf = cur_msg->buf;
		dw->xfr_len = cur_msg->len;
		dw->xfr_flags = cur_msg->flags;
		dw->rx_pending = 0;

		/* Need to RESTART if changing transfer direction */
		if ((pflags & I2C_MSG_RW_MASK)
		    != (dw->xfr_flags & I2C_MSG_RW_MASK)) {
			dw->xfr_flags |= I2C_MSG_RESTART;
		}

		/* Send STOP if this is the last message */
		if (msg_left == 1) {
			dw->xfr_flags |= I2C_MSG_STOP;
		}

		dw->state &= ~(I2C_QSE_SS_CMD_SEND | I2C_QSE_SS_CMD_RECV);

		if ((dw->xfr_flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			dw->state |= I2C_QSE_SS_CMD_SEND;
			dw->request_bytes = 0;
		} else {
			dw->state |= I2C_QSE_SS_CMD_RECV;
			dw->request_bytes = dw->xfr_len;
		}

		/* Enable interrupts to trigger ISR */
		_i2c_qse_ss_reg_write(dev, REG_INTR_MASK,
				      (IC_INTR_MASK_TX | IC_INTR_MASK_RX));

		/* Wait for transfer to be done */
		device_sync_call_wait(&dw->sync);
		if (dw->state & I2C_QSE_SS_CMD_ERROR) {
			ret = -EIO;
			break;
		}

		/* Something wrong if there is something left to do */
		if (dw->xfr_len > 0) {
			ret = -EIO;
			break;
		}

		cur_msg++;
		msg_left--;
	}

	dw->state = I2C_QSE_SS_STATE_READY;
	return ret;
}

static int i2c_qse_ss_runtime_configure(struct device *dev, uint32_t config)
{
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;
	uint32_t	value = 0;
	uint32_t	rc = 0;
	uint32_t	ic_con;
	uint32_t	spklen;

	dw->app_config.raw = config;

	ic_con = _i2c_qse_ss_reg_read(dev, REG_CON);

	spklen = (ic_con & IC_CON_SPKLEN_MASK) >> IC_CON_SPKLEN_POS;

	/* Make sure we have a supported speed for the DesignWare model */
	/* and have setup the clock frequency and speed mode */
	switch (dw->app_config.bits.speed) {
	case I2C_SPEED_STANDARD:
		/* Following the directions on DW spec page 59, IC_SS_SCL_LCNT
		 * must have register values larger than IC_FS_SPKLEN + 7
		 */
		if (I2C_STD_LCNT <= (spklen + 7)) {
			value = spklen + 8;
		} else {
			value = I2C_STD_LCNT;
		}

		dw->lcnt = value;

		/* Following the directions on DW spec page 59, IC_SS_SCL_HCNT
		 * must have register values larger than IC_FS_SPKLEN + 5
		 */
		if (I2C_STD_HCNT <= (spklen + 5)) {
			value = spklen + 6;
		} else {
			value = I2C_STD_HCNT;
		}

		dw->hcnt = value;
		break;
	case I2C_SPEED_FAST:
		/* fall through */
	case I2C_SPEED_FAST_PLUS:
		/*
		 * Following the directions on DW spec page 59, IC_FS_SCL_LCNT
		 * must have register values larger than IC_FS_SPKLEN + 7
		 */
		if (I2C_FS_LCNT <= (spklen + 7)) {
			value = spklen + 8;
		} else {
			value = I2C_FS_LCNT;
		}

		dw->lcnt = value;

		/*
		 * Following the directions on DW spec page 59, IC_FS_SCL_HCNT
		 * must have register values larger than IC_FS_SPKLEN + 5
		 */
		if (I2C_FS_HCNT <= (spklen + 5)) {
			value = spklen + 6;
		} else {
			value = I2C_FS_HCNT;
		}

		dw->hcnt = value;
		break;
	default:
		/* TODO change */
		rc = -EINVAL;
	}

	/*
	 * Clear any interrupts currently waiting in the controller
	 */
	_i2c_qse_ss_reg_write(dev, REG_INTR_CLR, IC_INTR_CLR_ALL);

	return rc;
}

static int i2c_qse_ss_suspend(struct device *dev)
{
	DBG("I2C_SS: suspend called - function not yet implemented\n");
	/* TODO - add this code */
	return 0;
}

static int i2c_qse_ss_resume(struct device *dev)
{
	DBG("I2C_SS: resume called - function not yet implemented\n");
	/* TODO - add this code */
	return 0;
}

static struct i2c_driver_api ss_funcs = {
	.configure = i2c_qse_ss_runtime_configure,
	.transfer = i2c_qse_ss_intr_transfer,
	.suspend = i2c_qse_ss_suspend,
	.resume = i2c_qse_ss_resume,
};

int i2c_qse_ss_initialize(struct device *dev)
{
	struct i2c_qse_ss_rom_config * const rom = dev->config->config_info;
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;

	dev->driver_api = &ss_funcs;

	if (rom->config_func) {
		rom->config_func(dev);
	}

	/* Enable clock for controller so we can talk to it */
	_i2c_qse_ss_reg_write_or(dev, REG_CON, IC_CON_CLK_ENA);

	device_sync_call_init(&dw->sync);

	if (i2c_qse_ss_runtime_configure(dev, dw->app_config.raw) != 0) {
		DBG("I2C_SS: Cannot set default configuration 0x%x\n",
		    dw->app_config.raw);
		return DEV_NOT_CONFIG;
	}

	dw->state = I2C_QSE_SS_STATE_READY;

	return 0;
}

#if CONFIG_I2C_QUARK_SE_SS_0
#include <init.h>

void _i2c_qse_ss_config_irq_0(struct device *port);

struct i2c_qse_ss_rom_config i2c_config_ss_0 = {
	.base_address =  CONFIG_I2C_QUARK_SE_SS_0_BASE,

	.config_func = _i2c_qse_ss_config_irq_0,
};

struct i2c_qse_ss_dev_config i2c_ss_0_runtime = {
	.app_config.raw = CONFIG_I2C_QUARK_SE_SS_0_DEFAULT_CFG,
};

DEVICE_INIT(i2c_ss_0, CONFIG_I2C_QUARK_SE_SS_0_NAME, &i2c_qse_ss_initialize,
			&i2c_ss_0_runtime, &i2c_config_ss_0,
			SECONDARY, CONFIG_I2C_INIT_PRIORITY);

void _i2c_qse_ss_config_irq_0(struct device *port)
{
	uint32_t mask = 0;

	/* Need to unmask the interrupts in System Control Subsystem (SCSS)
	 * so the interrupt controller can route these interrupts to
	 * the sensor subsystem.
	 */
	mask = _i2c_qse_ss_memory_read(SCSS_REGISTER_BASE, I2C_SS_0_ERR_MASK);
	mask &= INT_ENABLE_ARC;
	_i2c_qse_ss_memory_write(SCSS_REGISTER_BASE, I2C_SS_0_ERR_MASK, mask);

	mask = _i2c_qse_ss_memory_read(SCSS_REGISTER_BASE, I2C_SS_0_TX_MASK);
	mask &= INT_ENABLE_ARC;
	_i2c_qse_ss_memory_write(SCSS_REGISTER_BASE, I2C_SS_0_TX_MASK, mask);

	mask = _i2c_qse_ss_memory_read(SCSS_REGISTER_BASE, I2C_SS_0_RX_MASK);
	mask &= INT_ENABLE_ARC;
	_i2c_qse_ss_memory_write(SCSS_REGISTER_BASE, I2C_SS_0_RX_MASK, mask);

	mask = _i2c_qse_ss_memory_read(SCSS_REGISTER_BASE, I2C_SS_0_STOP_MASK);
	mask &= INT_ENABLE_ARC;
	_i2c_qse_ss_memory_write(SCSS_REGISTER_BASE, I2C_SS_0_STOP_MASK, mask);

	/* Connect the IRQs to ISR */
	IRQ_CONNECT(I2C_SS_0_ERR_VECTOR, 1, i2c_qse_ss_isr,
		    DEVICE_GET(i2c_ss_0), 0);
	IRQ_CONNECT(I2C_SS_0_RX_VECTOR, 1, i2c_qse_ss_isr,
		    DEVICE_GET(i2c_ss_0), 0);
	IRQ_CONNECT(I2C_SS_0_TX_VECTOR, 1, i2c_qse_ss_isr,
		    DEVICE_GET(i2c_ss_0), 0);
	IRQ_CONNECT(I2C_SS_0_STOP_VECTOR, 1, i2c_qse_ss_isr,
		    DEVICE_GET(i2c_ss_0), 0);

	irq_enable(I2C_SS_0_ERR_VECTOR);
	irq_enable(I2C_SS_0_RX_VECTOR);
	irq_enable(I2C_SS_0_TX_VECTOR);
	irq_enable(I2C_SS_0_STOP_VECTOR);
}

#endif /* CONFIG_I2C_QUARK_SE_SS_0 */

#if CONFIG_I2C_QUARK_SE_SS_1
#include <init.h>

void _i2c_qse_ss_config_irq_1(struct device *port);

struct i2c_qse_ss_rom_config i2c_config_ss_1 = {
	.base_address =  CONFIG_I2C_QUARK_SE_SS_1_BASE,

	.config_func = _i2c_qse_ss_config_irq_1,
};

struct i2c_qse_ss_dev_config i2c_qse_ss_1_runtime = {
	.app_config.raw = CONFIG_I2C_QUARK_SE_SS_1_DEFAULT_CFG,
};

DEVICE_INIT(i2c_ss_1, CONFIG_I2C_QUARK_SE_SS_1_NAME, &i2c_qse_ss_initialize,
			&i2c_qse_ss_1_runtime, &i2c_config_ss_1,
			SECONDARY, CONFIG_I2C_INIT_PRIORITY);


void _i2c_qse_ss_config_irq_1(struct device *port)
{
	uint32_t mask = 0;

	/* Need to unmask the interrupts in System Control Subsystem (SCSS)
	 * so the interrupt controller can route these interrupts to
	 * the sensor subsystem.
	 */
	mask = _i2c_qse_ss_memory_read(SCSS_REGISTER_BASE, I2C_SS_1_ERR_MASK);
	mask &= INT_ENABLE_ARC;
	_i2c_qse_ss_memory_write(SCSS_REGISTER_BASE, I2C_SS_1_ERR_MASK, mask);

	mask = _i2c_qse_ss_memory_read(SCSS_REGISTER_BASE, I2C_SS_1_TX_MASK);
	mask &= INT_ENABLE_ARC;
	_i2c_qse_ss_memory_write(SCSS_REGISTER_BASE, I2C_SS_1_TX_MASK, mask);

	mask = _i2c_qse_ss_memory_read(SCSS_REGISTER_BASE, I2C_SS_1_RX_MASK);
	mask &= INT_ENABLE_ARC;
	_i2c_qse_ss_memory_write(SCSS_REGISTER_BASE, I2C_SS_1_RX_MASK, mask);

	mask = _i2c_qse_ss_memory_read(SCSS_REGISTER_BASE, I2C_SS_1_STOP_MASK);
	mask &= INT_ENABLE_ARC;
	_i2c_qse_ss_memory_write(SCSS_REGISTER_BASE, I2C_SS_1_STOP_MASK, mask);

	/* Connect the IRQs to ISR */
	IRQ_CONNECT(I2C_SS_1_ERR_VECTOR, 1, i2c_qse_ss_isr,
		    DEVICE_GET(i2c_ss_1), 0);
	IRQ_CONNECT(I2C_SS_1_RX_VECTOR, 1, i2c_qse_ss_isr,
		    DEVICE_GET(i2c_ss_1), 0);
	IRQ_CONNECT(I2C_SS_1_TX_VECTOR, 1, i2c_qse_ss_isr,
		    DEVICE_GET(i2c_ss_1), 0);
	IRQ_CONNECT(I2C_SS_1_STOP_VECTOR, 1, i2c_qse_ss_isr,
		    DEVICE_GET(i2c_ss_1), 0);

	irq_enable(I2C_SS_1_ERR_VECTOR);
	irq_enable(I2C_SS_1_RX_VECTOR);
	irq_enable(I2C_SS_1_TX_VECTOR);
	irq_enable(I2C_SS_1_STOP_VECTOR);
}

#endif /* CONFIG_I2C_QUARK_SE_SS_1 */
