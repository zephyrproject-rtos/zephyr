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

#include <stdint.h>
#include <stdbool.h>

#include <i2c.h>
#include <nanokernel.h>
#include <arch/cpu.h>

#include <sys_io.h>

#include <board.h>

#include "i2c_quark_se_ss.h"
#include "i2c_quark_se_ss_registers.h"

#ifndef CONFIG_I2C_DEBUG
#define DBG(...) { ; }
#else
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

static inline void _i2c_qse_ss_data_ask(struct device *dev, uint8_t restart)
{
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;
	uint32_t data;

	/* No more bytes to request */
	if (dw->request_bytes == 0) {
		return;
	}

	/* Tell controller to get another byte */
	data = IC_DATA_CMD_CMD | IC_DATA_CMD_STROBE | IC_DATA_CMD_POP;

	/* Send restart if needed) */
	if (restart) {
		data |= IC_DATA_CMD_RESTART;
	}

	/* After receiving the last byte, send STOP */
	if (dw->request_bytes == 1) {
		data |= IC_DATA_CMD_STOP;
	}

	_i2c_qse_ss_reg_write(dev, REG_DATA_CMD, data);

	dw->request_bytes--;
}

static void _i2c_qse_ss_data_read(struct device *dev)
{
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;

	while (_i2c_qse_ss_is_rfne(dev) && (dw->rx_len > 0)) {
		/* Need to write 0 to POP bit to
		 * "pop" one byte from RX FIFO.
		 */
		_i2c_qse_ss_reg_write(dev, REG_DATA_CMD,
				      IC_DATA_CMD_STROBE);

		dw->rx_buffer[0] = _i2c_qse_ss_reg_read(dev, REG_DATA_CMD)
				   & IC_DATA_CMD_DATA_MASK;

		dw->rx_buffer += 1;
		dw->rx_len -= 1;

		if (dw->rx_len == 0) {
			break;
		}

		_i2c_qse_ss_data_ask(dev, 0);
	}

	/* Nothing to receive anymore */
	if (dw->rx_len == 0) {
		dw->state &= ~I2C_QSE_SS_CMD_RECV;
		return;
	}
}

static int _i2c_qse_ss_data_send(struct device *dev)
{
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;
	uint32_t data = 0;

	/* Nothing to send anymore, mask the interrupt */
	if (dw->tx_len == 0) {
		_i2c_qse_ss_reg_write_and(dev, REG_INTR_MASK,
					  ~(IC_INTR_TX_EMPTY));

		if (dw->rx_len > 0) {
			/* Tell controller to grab a byte.
			 * RESTART if something has ben sent.
			 */
			_i2c_qse_ss_data_ask(dev,
				(dw->state & I2C_QSE_SS_CMD_SEND));

			/* QUIRK:
			 * If requesting more than one byte, the process has
			 * to be jump-started by requesting two bytes first.
			 */
			_i2c_qse_ss_data_ask(dev, 0);
		}

		dw->state &= ~I2C_QSE_SS_CMD_SEND;

		return DEV_OK;
	}

	while (_i2c_qse_ss_is_tfnf(dev) && (dw->tx_len > 0)) {
		/* We have something to transmit to a specific host */
		data = dw->tx_buffer[0] | IC_DATA_CMD_STROBE | IC_DATA_CMD_POP;

		/* If this is the last byte to write
		 * and nothing to receive, send STOP.
		 */
		if ((dw->tx_len == 1) && (dw->rx_len == 0)) {
			data |= IC_DATA_CMD_STOP;
		}

		_i2c_qse_ss_reg_write(dev, REG_DATA_CMD, data);

		dw->tx_len -= 1;
		dw->tx_buffer += 1;

		if (_i2c_qse_ss_check_irq(dev, IC_INTR_TX_ABRT)) {
			return DEV_FAIL;
		}
	}

	return DEV_OK;
}

static inline void _i2c_qse_ss_transfer_complete(struct device *dev)
{
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;

	/* Disable and clear all pending interrupts */
	_i2c_qse_ss_reg_write(dev, REG_INTR_MASK, IC_INTR_MASK_ALL);
	_i2c_qse_ss_reg_write(dev, REG_INTR_CLR, IC_INTR_CLR_ALL);

	dw->state &= ~I2C_QSE_SS_BUSY;
}

void i2c_qse_ss_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;
	uint32_t ic_intr_stat;

	/*
	 * Causes of an intterrupt on ATP:
	 *   - STOP condition is detected
	 *   - Transfer is aborted
	 *   - Transmit FIFO is empy
	 *   - Transmit FIFO is overflowing
	 *   - Receive FIFO is full
	 *   - Receive FIFO overflow
	 *   - Received FIFO underrun
	 *   - Transmit data required (tx_req)
	 *   - Receive data available (rx_avail)
	 */

	DBG("I2C_SS: interrupt received\n");

	ic_intr_stat = _i2c_qse_ss_reg_read(dev, REG_INTR_STAT);

	/* Check if the Master TX is ready for sending */
	if (ic_intr_stat & IC_INTR_TX_EMPTY) {
		_i2c_qse_ss_data_send(dev);
		_i2c_qse_ss_reg_write(dev, REG_INTR_CLR, IC_INTR_TX_EMPTY);
	}

	/* Check if the RX FIFO reached threshold */
	if (ic_intr_stat & IC_INTR_RX_FULL) {
		_i2c_qse_ss_data_read(dev);
		_i2c_qse_ss_reg_write(dev, REG_INTR_CLR, IC_INTR_RX_FULL);
	}

	/* Error conditions */
	if ((IC_INTR_TX_ABRT | IC_INTR_TX_OVER |
	     IC_INTR_RX_OVER | IC_INTR_RX_UNDER) &
	    ic_intr_stat) {
		dw->state = I2C_QSE_SS_CMD_ERROR;
		_i2c_qse_ss_transfer_complete(dev);
		return;
	}

	/*
	 * We got a STOP_DET, this means stop right after this byte has been
	 * handled.
	 */
	if (ic_intr_stat & IC_INTR_STOP_DET) {
		_i2c_qse_ss_reg_write(dev, REG_INTR_CLR, IC_INTR_STOP_DET);
		_i2c_qse_ss_transfer_complete(dev);
	}
}

static int _i2c_qse_ss_setup(struct device *dev, uint16_t addr)
{
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;
	uint32_t ic_con;
	int rc = DEV_OK;

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
		rc = DEV_INVALID_CONF;
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
	 * the TX FIFO is empty.
	 *
	 * TODO: extend the threshold for multi-byte TX/RX FIFO.
	 */
	_i2c_qse_ss_reg_write(dev, REG_TL, 0x00000000);

	/* SDA Hold time has to setup to minimal 2 according to spec. */
	_i2c_qse_ss_reg_write(dev, REG_SDA_CONFIG, 0x00020000);

done:
	return rc;
}

static int _i2c_qse_ss_transfer_init(struct device *dev,
				    uint8_t *write_buf, uint32_t write_len,
				    uint8_t *read_buf,  uint32_t read_len,
				    uint16_t slave_address, uint32_t flags)
{
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;
	int ret = 0;

	dw->state |= I2C_QSE_SS_BUSY;
	if (write_len > 0) {
		dw->state |= I2C_QSE_SS_CMD_SEND;
	}
	if (read_len > 0) {
		dw->state |= I2C_QSE_SS_CMD_RECV;
	}

	dw->rx_len = read_len;
	dw->rx_buffer = read_buf;
	dw->tx_len = write_len;
	dw->tx_buffer = write_buf;
	dw->request_bytes = read_len;

	ret = _i2c_qse_ss_setup(dev, slave_address);
	if (ret) {
		return ret;
	}

	return DEV_OK;
}

static int i2c_qse_ss_intr_transfer(struct device *dev,
				    uint8_t *write_buf, uint32_t write_len,
				    uint8_t *read_buf,  uint32_t read_len,
				    uint16_t slave_address, uint32_t flags)
{
	int ret;

	/* First step, check if there is current activity */
	if (_i2c_qse_ss_is_busy(dev)) {
		return DEV_FAIL;
	}

	ret = _i2c_qse_ss_transfer_init(dev, write_buf, write_len,
				       read_buf, read_len, slave_address, 0);
	if (ret) {
		return ret;
	}

	/* Enable necessary interrupts */
	_i2c_qse_ss_reg_write(dev, REG_INTR_MASK,
			      (IC_INTR_MASK_TX | IC_INTR_MASK_RX));

	/* Enable controller */
	_i2c_qse_ss_reg_write_or(dev, REG_CON, IC_CON_ENABLE);

	return DEV_OK;
}

#define POLLING_TIMEOUT		(sys_clock_ticks_per_sec / 10)
static int i2c_qse_ss_poll_transfer(struct device *dev,
				    uint8_t *write_buf, uint32_t write_len,
				    uint8_t *read_buf,  uint32_t read_len,
				    uint16_t slave_address, uint32_t flags)
{
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;
	uint32_t start_time;
	int ret = DEV_OK;

	/* Wait for bus idle */
	start_time = sys_tick_get_32();
	while (_i2c_qse_ss_is_busy(dev)) {
		if ((sys_tick_get_32() - start_time) > POLLING_TIMEOUT) {
			return DEV_FAIL;
		}
	}

	ret = _i2c_qse_ss_transfer_init(dev, write_buf, write_len,
					read_buf, read_len, slave_address, 0);
	if (ret) {
		return ret;
	}

	/* Enable controller */
	_i2c_qse_ss_reg_write_or(dev, REG_CON, IC_CON_ENABLE);

	if (dw->tx_len == 0) {
		goto do_receive;
	}

	/* Transmit */
	while (dw->tx_len > 0) {
		/* Wait for space in TX FIFO */
		start_time = sys_tick_get_32();
		while (!_i2c_qse_ss_is_tfnf(dev)) {
			if ((sys_tick_get_32() - start_time)
			    > POLLING_TIMEOUT) {
				ret = DEV_FAIL;
				goto finish;
			}
		}

		ret = _i2c_qse_ss_data_send(dev);
		if (ret != DEV_OK) {
			goto finish;
		}
	}

	/* Wait for TX FIFO empty to be sure everything is sent. */
	start_time = sys_tick_get_32();
	while (!_i2c_qse_ss_is_tfe(dev)) {
		if ((sys_tick_get_32() - start_time) > POLLING_TIMEOUT) {
			ret = DEV_FAIL;
			goto finish;
		}
	}

	dw->state &= ~I2C_QSE_SS_CMD_SEND;

do_receive:
	/* Finalize TX when there is nothing more to send as
	 * the data send function has code to deal with the end of
	 * TX phase.
	 */
	_i2c_qse_ss_data_send(dev);

	/* Finish transfer when there is nothing to receive */
	if (dw->rx_len == 0) {
		goto stop_det;
	}

	while (dw->rx_len > 0) {
		/* Wait for data in RX FIFO*/
		start_time = sys_tick_get_32();
		while (!_i2c_qse_ss_is_rfne(dev)) {
			if ((sys_tick_get_32() - start_time)
			    > POLLING_TIMEOUT) {
				ret = DEV_FAIL;
				goto finish;
			}
		}

		_i2c_qse_ss_data_read(dev);
	}

	dw->state &= ~I2C_QSE_SS_CMD_RECV;

stop_det:
	/* Wait for transfer to complete */
	start_time = sys_tick_get_32();
	while (!_i2c_qse_ss_check_irq(dev, IC_INTR_STOP_DET)) {
		if ((sys_tick_get_32() - start_time) > POLLING_TIMEOUT) {
			ret = DEV_FAIL;
			goto finish;
		}
	}
	_i2c_qse_ss_reg_write(dev, REG_INTR_CLR, IC_INTR_STOP_DET);

	/* Wait for bus idle */
	start_time = sys_tick_get_32();
	while (_i2c_qse_ss_is_busy(dev)) {
		if ((sys_tick_get_32() - start_time) > POLLING_TIMEOUT) {
			ret = DEV_FAIL;
			goto finish;
		}
	}

finish:
	/* Disable controller when done */
	_i2c_qse_ss_reg_write_and(dev, REG_CON, ~(IC_CON_ENABLE));

	_i2c_qse_ss_transfer_complete(dev);

	dw->state = I2C_QSE_SS_STATE_READY;

	return ret;
}

static int i2c_qse_ss_runtime_configure(struct device *dev, uint32_t config)
{
	struct i2c_qse_ss_dev_config * const dw = dev->driver_data;
	uint32_t	value = 0;
	uint32_t	rc = DEV_OK;
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
		rc = DEV_INVALID_CONF;
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
	return DEV_OK;
}

static int i2c_qse_ss_resume(struct device *dev)
{
	DBG("I2C_SS: resume called - function not yet implemented\n");
	/* TODO - add this code */
	return DEV_OK;
}

static struct i2c_driver_api ss_funcs = {
	.configure = i2c_qse_ss_runtime_configure,
	.transfer = i2c_qse_ss_intr_transfer,
	.poll_transfer = i2c_qse_ss_poll_transfer,
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

	if (i2c_qse_ss_runtime_configure(dev, dw->app_config.raw) != DEV_OK) {
		DBG("I2C_SS: Cannot set default configuration 0x%x\n",
		    dev->app_config.raw);
		return DEV_NOT_CONFIG;
	}

	dw->state = I2C_QSE_SS_STATE_READY;

	return DEV_OK;
}

void _i2c_qse_ss_config_irq(struct device *port)
{
	struct i2c_qse_ss_rom_config * const rom = port->config->config_info;
	uint32_t mask = 0;

	/* Need to unmask the interrupts in System Control Subsystem (SCSS)
	 * so the interrupt controller can route these interrupts to
	 * the sensor subsystem.
	 */
	mask = _i2c_qse_ss_memory_read(SCSS_REGISTER_BASE, rom->isr_err_mask);
	mask &= INT_ENABLE_ARC;
	_i2c_qse_ss_memory_write(SCSS_REGISTER_BASE, rom->isr_err_mask, mask);

	mask = _i2c_qse_ss_memory_read(SCSS_REGISTER_BASE, rom->isr_tx_mask);
	mask &= INT_ENABLE_ARC;
	_i2c_qse_ss_memory_write(SCSS_REGISTER_BASE, rom->isr_tx_mask, mask);

	mask = _i2c_qse_ss_memory_read(SCSS_REGISTER_BASE, rom->isr_rx_mask);
	mask &= INT_ENABLE_ARC;
	_i2c_qse_ss_memory_write(SCSS_REGISTER_BASE, rom->isr_rx_mask, mask);

	mask = _i2c_qse_ss_memory_read(SCSS_REGISTER_BASE, rom->isr_stop_mask);
	mask &= INT_ENABLE_ARC;
	_i2c_qse_ss_memory_write(SCSS_REGISTER_BASE, rom->isr_stop_mask, mask);

	/* Connect the IRQs to ISR */
	irq_connect(rom->isr_err_vector, 1, i2c_qse_ss_isr, port);
	irq_connect(rom->isr_rx_vector, 1, i2c_qse_ss_isr, port);
	irq_connect(rom->isr_tx_vector, 1, i2c_qse_ss_isr, port);
	irq_connect(rom->isr_stop_vector, 1, i2c_qse_ss_isr, port);

	irq_enable(rom->isr_err_vector);
	irq_enable(rom->isr_rx_vector);
	irq_enable(rom->isr_tx_vector);
	irq_enable(rom->isr_stop_vector);
}


#if CONFIG_I2C_QUARK_SE_SS_0
#include <init.h>

struct i2c_qse_ss_rom_config i2c_config_ss_0 = {
	.base_address =  CONFIG_I2C_QUARK_SE_SS_0_BASE,

	.config_func = _i2c_qse_ss_config_irq,

	.isr_err_vector = I2C_SS_0_ERR_VECTOR,
	.isr_err_mask = I2C_SS_0_ERR_MASK,

	.isr_rx_vector = I2C_SS_0_RX_VECTOR,
	.isr_rx_mask = I2C_SS_0_RX_MASK,

	.isr_tx_vector = I2C_SS_0_TX_VECTOR,
	.isr_tx_mask = I2C_SS_0_TX_MASK,

	.isr_stop_vector = I2C_SS_0_STOP_VECTOR,
	.isr_stop_mask = I2C_SS_0_STOP_MASK,
};

struct i2c_qse_ss_dev_config i2c_ss_0_runtime = {
	.app_config.raw = CONFIG_I2C_QUARK_SE_SS_0_DEFAULT_CFG,
};

DECLARE_DEVICE_INIT_CONFIG(i2c_ss_0,
			   CONFIG_I2C_QUARK_SE_SS_0_NAME,
			   &i2c_qse_ss_initialize,
			   &i2c_config_ss_0);

SYS_DEFINE_DEVICE(i2c_ss_0, &i2c_ss_0_runtime,
					SECONDARY, CONFIG_I2C_INIT_PRIORITY);

#endif /* CONFIG_I2C_QUARK_SE_SS_0 */

#if CONFIG_I2C_QUARK_SE_SS_1
#include <init.h>

struct i2c_qse_ss_rom_config i2c_config_ss_1 = {
	.base_address =  CONFIG_I2C_QUARK_SE_SS_1_BASE,

	.config_func = _i2c_qse_ss_config_irq,

	.isr_err_vector = I2C_SS_1_ERR_VECTOR,
	.isr_err_mask = I2C_SS_1_ERR_MASK,

	.isr_rx_vector = I2C_SS_1_RX_VECTOR,
	.isr_rx_mask = I2C_SS_1_RX_MASK,

	.isr_tx_vector = I2C_SS_1_TX_VECTOR,
	.isr_tx_mask = I2C_SS_1_TX_MASK,

	.isr_stop_vector = I2C_SS_1_STOP_VECTOR,
	.isr_stop_mask = I2C_SS_1_STOP_MASK,
};

struct i2c_qse_ss_dev_config i2c_qse_ss_1_runtime = {
	.app_config.raw = CONFIG_I2C_QUARK_SE_SS_1_DEFAULT_CFG,
};

DECLARE_DEVICE_INIT_CONFIG(i2c_ss_1,
			   CONFIG_I2C_QUARK_SE_SS_1_NAME,
			   &i2c_qse_ss_initialize,
			   &i2c_config_ss_1);

SYS_DEFINE_DEVICE(i2c_ss_1, &i2c_qse_ss_1_runtime,
					SECONDARY, CONFIG_I2C_INIT_PRIORITY);

#endif /* CONFIG_I2C_QUARK_SE_SS_1 */
