/* dw_i2c.c - I2C file for Design Ware */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <i2c.h>
#include <nanokernel.h>
#include <arch/cpu.h>
#include <string.h>

#include <board.h>
#include <errno.h>
#include <sys_io.h>

#ifdef CONFIG_SHARED_IRQ
#include <shared_irq.h>
#endif

#include "i2c-dw.h"
#include "i2c-dw-registers.h"

#ifndef CONFIG_I2C_DEBUG
#define DBG(...) {;}
#else
#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define DBG printf
#else
#define DBG printk
#endif /* CONFIG_STDOUT_CONSOLE */
#endif /* CONFIG_I2C_DEBUG */

static inline uint32_t i2c_dw_memory_read(uint32_t base_addr, uint32_t offset)
{
	return sys_read32(base_addr + offset);
}


static inline void i2c_dw_memory_write(uint32_t base_addr, uint32_t offset,
				       uint32_t val)
{
	sys_write32(val, base_addr + offset);
}


static inline void _i2c_dw_data_ask(struct device *dev, uint8_t restart)
{
	struct i2c_dw_rom_config const * const rom = dev->config->config_info;
	struct i2c_dw_dev_config * const dw = dev->driver_data;
	volatile struct i2c_dw_registers * const regs =
		(struct i2c_dw_registers *)rom->base_address;
	uint32_t data;

	/* No more bytes to request */
	if (dw->request_bytes == 0) {
		return;
	}

	/* Tell controller to get another byte */
	data = IC_DATA_CMD_CMD;

	/* Send restart if needed) */
	if (restart) {
		data |= IC_DATA_CMD_RESTART;
	}

	/* After receiving the last byte, send STOP */
	if (dw->request_bytes == 1) {
		data |= IC_DATA_CMD_STOP;
	}

	regs->ic_data_cmd.raw = data;

	dw->request_bytes--;
}

static void _i2c_dw_data_read(struct device *dev)
{
	struct i2c_dw_rom_config const * const rom = dev->config->config_info;
	struct i2c_dw_dev_config * const dw = dev->driver_data;
	volatile struct i2c_dw_registers * const regs =
		(struct i2c_dw_registers *)rom->base_address;

	while (regs->ic_status.bits.rfne && (dw->rx_len > 0)) {
		dw->rx_buffer[0] = regs->ic_data_cmd.raw;

		dw->rx_buffer += 1;
		dw->rx_len -= 1;

		if (dw->rx_len == 0) {
			break;
		}

		_i2c_dw_data_ask(dev, 0);
	}

	/* Nothing to receive anymore */
	if (dw->rx_len == 0) {
		dw->state &= ~I2C_DW_CMD_RECV;
		return;
	}
}


static void _i2c_dw_data_send(struct device *dev)
{
	struct i2c_dw_rom_config const * const rom = dev->config->config_info;
	struct i2c_dw_dev_config * const dw = dev->driver_data;
	volatile struct i2c_dw_registers * const regs =
		(struct i2c_dw_registers *)rom->base_address;
	uint32_t data = 0;

	/* Nothing to send anymore, mask the interrupt */
	if (dw->tx_len == 0) {
		regs->ic_intr_mask.bits.tx_empty = 0;

		if (dw->rx_len > 0) {
			/* Tell controller to grab a byte.
			 * RESTART if something has ben sent.
			 */
			_i2c_dw_data_ask(dev, (dw->state & I2C_DW_CMD_SEND));

			/* QUIRK:
			 * If requesting more than one byte, the process has
			 * to be jump-started by requesting two bytes first.
			 */
			_i2c_dw_data_ask(dev, 0);
		}

		dw->state &= ~I2C_DW_CMD_SEND;

		return;
	}

	while (regs->ic_status.bits.tfnf && (dw->tx_len > 0)) {
		/* We have something to transmit to a specific host */
		data = dw->tx_buffer[0];

		/* If this is the last byte to write
		 * and nothing to receive, send STOP.
		 */
		if ((dw->tx_len == 1) && (dw->rx_len == 0)) {
			data |= IC_DATA_CMD_STOP;
		}

		regs->ic_data_cmd.raw = data;
		dw->tx_len -= 1;
		dw->tx_buffer += 1;
	}
}

static inline void _i2c_dw_transfer_complete(struct device *dev)
{
	struct i2c_dw_rom_config const * const rom = dev->config->config_info;
	struct i2c_dw_dev_config * const dw = dev->driver_data;
	volatile struct i2c_dw_registers * const regs =
		(struct i2c_dw_registers *)rom->base_address;
	uint32_t cb_type = 0;
	uint32_t value;

	if (dw->state == I2C_DW_CMD_ERROR) {
		cb_type = I2C_CB_ERROR;
	} else if (dw->tx_buffer && !dw->tx_len) {
		cb_type = I2C_CB_WRITE;
	} else if (dw->rx_buffer && !dw->rx_len) {
		cb_type = I2C_CB_READ;
	}

	if (cb_type) {
		regs->ic_intr_mask.raw = DW_DISABLE_ALL_I2C_INT;
		dw->state = I2C_DW_STATE_READY;
		value = regs->ic_clr_intr;

		if (dw->cb) {
			dw->cb(dev, cb_type);
		}
	}

	dw->state &= ~I2C_DW_BUSY;
}

void i2c_dw_isr(struct device *port)
{
	struct i2c_dw_rom_config const * const rom = port->config->config_info;
	struct i2c_dw_dev_config * const dw = port->driver_data;
	volatile struct i2c_dw_registers * const regs =
		(struct i2c_dw_registers *)rom->base_address;

	uint32_t value = 0;

#if CONFIG_SHARED_IRQ
	/* If using with shared IRQ, this function will be called
	 * by the shared IRQ driver. So check here if the interrupt
	 * is coming from the I2C controller (or somewhere else).
	 */
	if (!regs->ic_intr_stat.raw) {
		return;
	}
#endif

	/*
	 * Causes of an interrupt:
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

	DBG("I2C: interrupt received\n");

	/*
	 * We got a STOP_DET, this means stop right after this byte has been
	 * handled.
	 */
	if (regs->ic_intr_stat.bits.stop_det) {
		value = regs->ic_clr_stop_det;
		_i2c_dw_transfer_complete(port);
	}

	/* Check if we are configured as a master device */
	if (regs->ic_con.bits.master_mode) {
		/* Check if the Master TX is ready for sending */
		if (regs->ic_intr_stat.bits.tx_empty) {
			_i2c_dw_data_send(port);
		}

		/* Check if the RX FIFO reached threshold */
		if (regs->ic_intr_stat.bits.rx_full) {
			_i2c_dw_data_read(port);
		}

		if ((DW_INTR_STAT_TX_ABRT | DW_INTR_STAT_TX_OVER |
		     DW_INTR_STAT_RX_OVER | DW_INTR_STAT_RX_UNDER) &
		    regs->ic_intr_stat.raw) {
			dw->state = I2C_DW_CMD_ERROR;
			_i2c_dw_transfer_complete(port);
		}
	} else { /* we must be configured as a slave device */

		/* We have a read requested by the master device */
		if (regs->ic_intr_stat.bits.rd_req &&
		    (!dw->app_config.bits.is_slave_read)) {

			/* data is not ready to send */
			if (regs->ic_intr_stat.bits.tx_abrt) {
				/* clear the TX_ABRT interrupt */
				value = regs->ic_clr_tx_abrt;
			}

			_i2c_dw_data_send(port);
			value = regs->ic_clr_rd_req;
		}

		/* The slave device is ready to receive */
		if (regs->ic_intr_stat.bits.rx_full &&
		    dw->app_config.bits.is_slave_read) {
			_i2c_dw_data_read(port);
		}
	}
}


static int _i2c_dw_setup(struct device *dev)
{
	struct i2c_dw_dev_config * const dw = dev->driver_data;
	struct i2c_dw_rom_config const * const rom = dev->config->config_info;
	volatile struct i2c_dw_registers * const regs =
		(struct i2c_dw_registers *)rom->base_address;
	uint32_t value = 0;
	union ic_con_register ic_con;
	int rc = DEV_OK;

	ic_con.raw = 0;

	/*
	 * Clear any interrupts currently waiting in the controller
	 * this is done by reading register 0x40
	 */
	value = regs->ic_clr_intr;

	/* Set master or slave mode - (initialization = slave) */
	if (dw->app_config.bits.is_master_device) {
		/*
		 * Make sure to set both the master_mode and slave_disable_bit
		 * to both 0 or both 1
		 */
		DBG("I2C: host configured as Master Device\n");
		ic_con.bits.master_mode = 1;
		ic_con.bits.slave_disable = 1;
	}

	ic_con.bits.restart_en = 1;

	/* Set addressing mode - (initialization = 7 bit) */
	if (dw->app_config.bits.use_10_bit_addr) {
		DBG("I2C: using 10-bit address\n");
		ic_con.bits.addr_master_10bit = 1;
		ic_con.bits.addr_slave_10bit = 1;
	}

	/* Setup the clock frequency and speed mode */
	switch (dw->app_config.bits.speed) {
	case I2C_SPEED_STANDARD:
		DBG("I2C: speed set to STANDARD\n");
		regs->ic_ss_scl_lcnt = dw->lcnt;
		regs->ic_ss_scl_hcnt = dw->hcnt;
		ic_con.bits.speed = I2C_DW_SPEED_STANDARD;

		break;
	case I2C_SPEED_FAST:
		/* fall through */
	case I2C_SPEED_FAST_PLUS:
		DBG("I2C: speed set to FAST or FAST_PLUS\n");
		regs->ic_fs_scl_lcnt = dw->lcnt;
		regs->ic_fs_scl_hcnt = dw->hcnt;
		ic_con.bits.speed = I2C_DW_SPEED_FAST;

		break;
	case I2C_SPEED_HIGH:
		if (!dw->support_hs_mode) {
			rc = DEV_INVALID_CONF;
			break;
		}

		DBG("I2C: speed set to HIGH\n");
		regs->ic_hs_scl_lcnt = dw->lcnt;
		regs->ic_hs_scl_hcnt = dw->hcnt;
		ic_con.bits.speed = I2C_DW_SPEED_HIGH;

		break;
	default:
		DBG("I2C: invalid speed requested\n");
		/* TODO change */
		rc = DEV_INVALID_CONF;
	}

	DBG("I2C: lcnt = %d\n", dw->lcnt);
	DBG("I2C: hcnt = %d\n", dw->hcnt);

	/* Set the IC_CON register */
	regs->ic_con = ic_con;
	/* END of setup IC_CON */

	/* Set RX fifo threshold level.
	 * Setting it to zero automatically triggers interrupt
	 * RX_FULL whenever there is data received.
	 *
	 * TODO: extend the threshold for multi-byte RX.
	 */
	regs->ic_rx_tl = 0;

	/* Set TX fifo threshold level.
	 * TX_EMPTY interrupt is triggered only when the
	 * TX FIFO is truly empty.
	 *
	 * TODO: threshold set to just enough for TX
	 */
	regs->ic_tx_tl = 0;

	return rc;
}


static int _i2c_dw_transfer_init(struct device *dev,
				 uint8_t *write_buf, uint32_t write_len,
				 uint8_t *read_buf,  uint32_t read_len,
				 uint16_t slave_address)
{
	struct i2c_dw_rom_config const * const rom = dev->config->config_info;
	struct i2c_dw_dev_config * const dw = dev->driver_data;
	volatile struct i2c_dw_registers * const regs =
		(struct i2c_dw_registers *)rom->base_address;
	uint32_t value = 0;
	int ret;

	dw->state |= I2C_DW_BUSY;
	if (write_len > 0) {
		dw->state |= I2C_DW_CMD_SEND;
	}
	if (read_len > 0) {
		dw->state |= I2C_DW_CMD_RECV;
	}

	dw->rx_len = read_len;
	dw->rx_buffer = read_buf;
	dw->tx_len = write_len;
	dw->tx_buffer = write_buf;
	dw->request_bytes = read_len;

	/* Disable the device controller to be able set TAR */
	regs->ic_enable.bits.enable = 0;

	ret = _i2c_dw_setup(dev);
	if (ret) {
		return ret;
	}

	/* Disable interrupts */
	regs->ic_intr_mask.raw = 0;

	/* Clear interrupts */
	value = regs->ic_clr_intr;

	if (regs->ic_con.bits.master_mode) {
		/* Set address of target slave */
		regs->ic_tar.bits.ic_tar = slave_address;
	} else {
		/* Set slave address for device */
		regs->ic_sar.bits.ic_sar = slave_address;
	}

	return DEV_OK;
}

static int i2c_dw_transfer(struct device *dev,
			   uint8_t *write_buf, uint32_t write_len,
			   uint8_t *read_buf,  uint32_t read_len,
			   uint16_t slave_address, uint32_t flags)
{
	struct i2c_dw_rom_config const * const rom = dev->config->config_info;
	volatile struct i2c_dw_registers * const regs =
		(struct i2c_dw_registers *)rom->base_address;
	int ret;

	/* First step, check if there is current activity */
	if (regs->ic_status.bits.activity) {
		return DEV_FAIL;
	}

	ret = _i2c_dw_transfer_init(dev, write_buf, write_len,
				    read_buf, read_len, slave_address);
	if (ret) {
		return ret;
	}

	/* Trigger IRQ when TX_EMPTY */
	regs->ic_con.bits.tx_empty_ctl = 1;

	if (regs->ic_con.bits.master_mode) {
		/* Enable necessary interrupts */
		regs->ic_intr_mask.raw = (DW_ENABLE_TX_INT_I2C_MASTER |
					  DW_ENABLE_RX_INT_I2C_MASTER);
	} else {
		/* Enable necessary interrupts */
		regs->ic_intr_mask.raw = DW_ENABLE_TX_INT_I2C_SLAVE;
	}

	/* Enable controller */
	regs->ic_enable.bits.enable = 1;

	return DEV_OK;
}

#define POLLING_TIMEOUT		(sys_clock_ticks_per_sec / 10)
static int i2c_dw_poll_transfer(struct device *dev,
				uint8_t *write_buf, uint32_t write_len,
				uint8_t *read_buf,  uint32_t read_len,
				uint16_t slave_address, uint32_t flags)
{
	struct i2c_dw_rom_config const * const rom = dev->config->config_info;
	struct i2c_dw_dev_config * const dw = dev->driver_data;
	volatile struct i2c_dw_registers * const regs =
		(struct i2c_dw_registers *)rom->base_address;
	uint32_t value = 0;
	uint32_t start_time;
	int ret = DEV_OK;

	if (!regs->ic_con.bits.master_mode) {
		/* Only acting as master is supported */
		return DEV_INVALID_OP;
	}

	/* Wait for bus idle */
	start_time = nano_tick_get_32();
	while (regs->ic_status.bits.activity) {
		if ((nano_tick_get_32() - start_time) > POLLING_TIMEOUT) {
			return DEV_FAIL;
		}
	}

	ret = _i2c_dw_transfer_init(dev, write_buf, write_len,
				    read_buf, read_len, slave_address);
	if (ret) {
		return ret;
	}

	/* Enable controller */
	regs->ic_enable.bits.enable = 1;

	if (dw->tx_len == 0) {
		goto do_receive;
	}

	/* Transmit */
	while (dw->tx_len > 0) {
		/* Wait for space in TX FIFO */
		start_time = nano_tick_get_32();
		while (!regs->ic_status.bits.tfnf) {
			if ((nano_tick_get_32() - start_time) > POLLING_TIMEOUT) {
				ret = DEV_FAIL;
				goto finish;
			}
		}

		_i2c_dw_data_send(dev);
	}

	/* Wait for TX FIFO empty to be sure everything is sent. */
	start_time = nano_tick_get_32();
	while (!regs->ic_status.bits.tfe) {
		if ((nano_tick_get_32() - start_time) > POLLING_TIMEOUT) {
			ret = DEV_FAIL;
			goto finish;
		}
	}

do_receive:
	/* Finalize TX when there is nothing more to send as
	 * the data send function has code to deal with the end of
	 * TX phase.
	 */
	_i2c_dw_data_send(dev);

	/* Finish transfer when there is nothing to receive */
	if (dw->rx_len == 0) {
		goto stop_det;
	}

	while (dw->rx_len > 0) {
		/* Wait for data in RX FIFO*/
		start_time = nano_tick_get_32();
		while (!regs->ic_status.bits.rfne) {
			if ((nano_tick_get_32() - start_time) > POLLING_TIMEOUT) {
				ret = DEV_FAIL;
				goto finish;
			}
		}

		_i2c_dw_data_read(dev);
	}

stop_det:
	/* Wait for transfer to complete */
	start_time = nano_tick_get_32();
	while (!regs->ic_raw_intr_stat.bits.stop_det) {
		if ((nano_tick_get_32() - start_time) > POLLING_TIMEOUT) {
			ret = DEV_FAIL;
			goto finish;
		}
	}
	value = regs->ic_clr_stop_det;

	/* Wait for bus idle */
	start_time = nano_tick_get_32();
	while (regs->ic_status.bits.activity) {
		if ((nano_tick_get_32() - start_time) > POLLING_TIMEOUT) {
			ret = DEV_FAIL;
			goto finish;
		}
	}

finish:
	/* Disable controller when done */
	regs->ic_enable.bits.enable = 0;

	return ret;
}

static int i2c_dw_runtime_configure(struct device *dev, uint32_t config)
{
	struct i2c_dw_rom_config const * const rom = dev->config->config_info;
	struct i2c_dw_dev_config * const dw = dev->driver_data;
	volatile struct i2c_dw_registers * const regs =
		(struct i2c_dw_registers *)rom->base_address;
	uint32_t	value = 0;
	uint32_t	rc = DEV_OK;

	dw->app_config.raw = config;

	/* Make sure we have a supported speed for the DesignWare model */
	/* and have setup the clock frequency and speed mode */
	switch (dw->app_config.bits.speed) {
	case I2C_SPEED_STANDARD:
		/* Following the directions on DW spec page 59, IC_SS_SCL_LCNT
		 * must have register values larger than IC_FS_SPKLEN + 7
		 */
		if (I2C_STD_LCNT <= (regs->ic_fs_spklen + 7)) {
		       value = regs->ic_fs_spklen + 8;
		} else {
		       value = I2C_STD_LCNT;
		}

		dw->lcnt = value;

		/* Following the directions on DW spec page 59, IC_SS_SCL_HCNT
		 * must have register values larger than IC_FS_SPKLEN + 5
		 */
		if (I2C_STD_HCNT <= (regs->ic_fs_spklen + 5)) {
		       value = regs->ic_fs_spklen + 6;
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
		if (I2C_FS_LCNT <= (regs->ic_fs_spklen + 7)) {
		       value = regs->ic_fs_spklen + 8;
		} else {
		       value = I2C_FS_LCNT;
		}

		dw->lcnt = value;

		/*
		 * Following the directions on DW spec page 59, IC_FS_SCL_HCNT
		 * must have register values larger than IC_FS_SPKLEN + 5
		 */
		if (I2C_FS_HCNT <= (regs->ic_fs_spklen + 5)) {
		       value = regs->ic_fs_spklen + 6;
		} else {
		       value = I2C_FS_HCNT;
		}

		dw->hcnt = value;
		break;
	case I2C_SPEED_HIGH:
		if (dw->support_hs_mode) {
			if (I2C_HS_LCNT <= (regs->ic_hs_spklen + 7)) {
			       value = regs->ic_hs_spklen + 8;
			} else {
			       value = I2C_HS_LCNT;
			}

			dw->lcnt = value;

			if (I2C_HS_HCNT <= (regs->ic_hs_spklen + 5)) {
			       value = regs->ic_hs_spklen + 6;
			} else {
			       value = I2C_HS_HCNT;
			}

			dw->hcnt = value;
		} else {
			rc = DEV_INVALID_CONF;
		}
		break;
	default:
		/* TODO change */
		rc = DEV_INVALID_CONF;
	}

	/*
	 * Clear any interrupts currently waiting in the controller
	 */
	value = regs->ic_clr_intr;

	/*
	 * TEMPORARY HACK - The I2C does not work in any mode other than Master
	 * currently.  This "hack" forces us to always be configured for master
	 * mode, until we can verify that Slave mode works correctly.
	 */
	dw->app_config.bits.is_master_device = 1;

	return rc;
}

static int i2c_dw_set_callback(struct device *dev, i2c_callback cb)
{
	struct i2c_dw_dev_config * const dw = dev->driver_data;

	dw->cb = cb;

	return DEV_OK;
}

static int i2c_dw_write(struct device *dev, uint8_t *buf,
			uint32_t len, uint16_t slave_addr)
{
	return i2c_dw_transfer(dev, buf, len, 0, 0, slave_addr, 0);
}


static int i2c_dw_read(struct device *dev, uint8_t *buf,
			uint32_t len, uint16_t slave_addr)
{
	return i2c_dw_transfer(dev, 0, 0, buf, len, slave_addr, 0);
}

static int i2c_dw_polling_write(struct device *dev,
				    uint8_t *write_buf, uint32_t write_len,
				    uint16_t slave_address)
{
	return i2c_dw_poll_transfer(dev, write_buf, write_len,
				    0, 0, slave_address, 0);
}

static int i2c_dw_suspend(struct device *dev)
{
	DBG("I2C: suspend called - function not yet implemented\n");
	/* TODO - add this code */
	return DEV_OK;
}


static int i2c_dw_resume(struct device *dev)
{
	DBG("I2C: resume called - function not yet implemented\n");
	/* TODO - add this code */
	return DEV_OK;
}


static struct i2c_driver_api funcs = {
	.configure = i2c_dw_runtime_configure,
	.set_callback = i2c_dw_set_callback,
	.write = i2c_dw_write,
	.read = i2c_dw_read,
	.transfer = i2c_dw_transfer,
	.suspend = i2c_dw_suspend,
	.resume = i2c_dw_resume,
	.polling_write = i2c_dw_polling_write,
	.poll_transfer = i2c_dw_poll_transfer,
};


#ifdef CONFIG_PCI
static inline int i2c_dw_pci_setup(struct device *dev)
{
	struct i2c_dw_rom_config *rom = dev->config->config_info;

	pci_bus_scan_init();

	if (!pci_bus_scan(&rom->pci_dev)) {
		DBG("Could not find device\n");
		return 0;
	}

#ifdef CONFIG_PCI_ENUMERATION
	rom->base_address = rom->pci_dev.addr;
	rom->interrupt_vector = rom->pci_dev.irq;
#endif
	pci_enable_regs(&rom->pci_dev);

	pci_show(&rom->pci_dev);

	return 1;
}
#else
#define i2c_dw_pci_setup(_unused_) (1)
#endif /* CONFIG_PCI */

int i2c_dw_initialize(struct device *port)
{
	struct i2c_dw_rom_config const * const rom = port->config->config_info;
	struct i2c_dw_dev_config * const dev = port->driver_data;
	volatile struct i2c_dw_registers *regs;

	if (!i2c_dw_pci_setup(port)) {
		return DEV_NOT_CONFIG;
	}

	regs = (struct i2c_dw_registers*) rom->base_address;

	/* verify that we have a valid DesignWare register first */
	if (regs->ic_comp_type != I2C_DW_MAGIC_KEY) {
		port->driver_api = NULL;
		DBG("I2C: DesignWare magic key not found, check base address.");
		DBG(" Stopping initialization\n");
		return DEV_NOT_CONFIG;
	}

	port->driver_api = &funcs;

	/*
	 * grab the default value on initialization.  This should be set to the
	 * IC_MAX_SPEED_MODE in the hardware.  If it does support high speed we
	 * can move provide support for it
	 */
	if (regs->ic_con.bits.speed == I2C_DW_SPEED_HIGH) {
		DBG("I2C: high speed supported\n");
		dev->support_hs_mode = true;
	} else {
		DBG("I2C: high speed NOT supported\n");
		dev->support_hs_mode = false;
	}

	rom->config_func(port);

	if (i2c_dw_runtime_configure(port, dev->app_config.raw) != DEV_OK) {
		DBG("I2C: Cannot set default configuration 0x%x\n",
		    dev->app_config.raw);
		return DEV_NOT_CONFIG;
	}

	dev->state = I2C_DW_STATE_READY;

	return DEV_OK;
}

/* system bindings */
#if CONFIG_I2C_DW_0
#include <init.h>
void i2c_config_0(struct device *port);

struct i2c_dw_rom_config i2c_config_dw_0 = {
	.base_address = CONFIG_I2C_DW_0_BASE,
#ifdef CONFIG_I2C_DW_0_IRQ_DIRECT
	.interrupt_vector = CONFIG_I2C_DW_0_IRQ,
#endif
#if CONFIG_PCI
	.pci_dev.class = CONFIG_I2C_DW_CLASS,
	.pci_dev.bus = CONFIG_I2C_DW_0_BUS,
	.pci_dev.dev = CONFIG_I2C_DW_0_DEV,
	.pci_dev.vendor_id = CONFIG_I2C_DW_VENDOR_ID,
	.pci_dev.device_id = CONFIG_I2C_DW_DEVICE_ID,
	.pci_dev.function = CONFIG_I2C_DW_0_FUNCTION,
	.pci_dev.bar = CONFIG_I2C_DW_0_BAR,
#endif
	.config_func = i2c_config_0,

#ifdef CONFIG_GPIO_DW_0_IRQ_SHARED
	.shared_irq_dev_name = CONFIG_I2C_DW_0_IRQ_SHARED_NAME,
#endif
};

struct i2c_dw_dev_config i2c_0_runtime = {
	.app_config.raw = CONFIG_I2C_DW_0_DEFAULT_CFG,
};

DECLARE_DEVICE_INIT_CONFIG(i2c_0,
			   CONFIG_I2C_DW_0_NAME,
			   &i2c_dw_initialize,
			   &i2c_config_dw_0);

pre_kernel_late_init(i2c_0, &i2c_0_runtime);

#ifdef CONFIG_I2C_DW_0_IRQ_DIRECT
IRQ_CONNECT_STATIC(i2c_dw_0,
		   CONFIG_I2C_DW_0_IRQ,
		   CONFIG_I2C_DW_0_INT_PRIORITY,
		   i2c_dw_isr_0,
		   0);
#endif

void i2c_config_0(struct device *port)
{
	struct i2c_dw_rom_config * const config = port->config->config_info;
	struct device *shared_irq_dev;

#if defined(CONFIG_I2C_DW_0_IRQ_DIRECT)
	ARG_UNUSED(shared_irq_dev);
	IRQ_CONFIG(i2c_dw_0, config->interrupt_vector);
	irq_enable(config->interrupt_vector);
#elif defined(CONFIG_I2C_DW_0_IRQ_SHARED)
	ARG_UNUSED(config);
	shared_irq_dev = device_get_binding(config->shared_irq_dev_name);
	shared_irq_isr_register(shared_irq_dev, (isr_t)i2c_dw_isr, port);
	shared_irq_enable(shared_irq_dev, port);
#endif
}

#ifdef CONFIG_I2C_DW_0_IRQ_DIRECT
void i2c_dw_isr_0(void *unused)
{
	i2c_dw_isr(&__initconfig_i2c_02);
}
#endif

#endif /* CONFIG_I2C_DW_0 */
