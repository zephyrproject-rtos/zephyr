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

#include "dw_i2c.h"
#include "dw_i2c_registers.h"

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

static inline uint32_t dw_i2c_memory_read(uint32_t base_addr, uint32_t offset)
{
	return sys_read32(base_addr + offset);
}


static inline void dw_i2c_memory_write(uint32_t base_addr, uint32_t offset,
				       uint32_t val)
{
	sys_write32(val, base_addr + offset);
}


static void dw_i2c_data_read(struct device *dev)
{
	struct dw_i2c_rom_config const * const rom = dev->config->config_info;
	struct dw_i2c_dev_config * const dw = dev->driver_data;
	volatile struct dw_i2c_registers * const regs =
		(struct dw_i2c_registers *)rom->base_address;
	uint32_t i = 0;
	uint32_t rx_cnt = 0;

	/* Make sure we have some buffer to read/write to */
	if (dw->rx_len == 0) {
		return;
	}

	rx_cnt = regs->ic_rxflr;

	if (rx_cnt > dw->rx_len) {
		rx_cnt = dw->rx_len;
	}

	for (i = 0; i < rx_cnt; i++) {
		dw->rx_buffer[i] = regs->ic_data_cmd.raw;
	}

	dw->rx_buffer += i;
	dw->rx_len -= i;
}


static void dw_i2c_data_send(struct device *dev)
{
	struct dw_i2c_rom_config const * const rom = dev->config->config_info;
	struct dw_i2c_dev_config * const dw = dev->driver_data;
	volatile struct dw_i2c_registers * const regs =
		(struct dw_i2c_registers *)rom->base_address;
	uint32_t i = 0;
	uint32_t tx_cnt = 0;
	uint32_t data = 0;


	if (dw->rx_tx_len == 0) {
		return;
	}

	tx_cnt = I2C_DW_FIFO_DEPTH - regs->ic_txflr;

	if (tx_cnt > dw->rx_tx_len) {
		tx_cnt = dw->rx_tx_len;
	}

	for (i = 0; i < tx_cnt; i++) {
		if (dw->tx_len > 0) {
			/* We have something to transmit to a specific host */
			data = dw->tx_buffer[i];

			/* Is this the last byte to write */
			if (dw->tx_len == 1) {
				data |= (dw->rx_len > 0) ?
					IC_DATA_CMD_RESTART : IC_DATA_CMD_STOP;
			}

			dw->tx_len -= 1;
		} else {
			/*
			 * We want to send out a request to read data from a
			 * specific host
			 */
			data = IC_DATA_CMD_CMD;

			/* This is the last dummy byte to write */
			if (dw->rx_tx_len == 1) {
				data |= IC_DATA_CMD_STOP;
			}
		}

		regs->ic_data_cmd.raw = data;
		dw->rx_tx_len -= 1;
	}

	dw->tx_buffer += i;

	if (dw->rx_tx_len <= 0) {
		regs->ic_intr_mask.bits.tx_empty = 0;
		regs->ic_intr_mask.bits.stop_det = 1;
	}
}

void dw_i2c_isr(struct device *port)
{
	struct dw_i2c_rom_config const * const rom = port->config->config_info;
	struct dw_i2c_dev_config * const dw = port->driver_data;
	volatile struct dw_i2c_registers * const regs =
		(struct dw_i2c_registers *)rom->base_address;

	uint32_t value = 0;

	/*
	 * Causes of an intterrupt:
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
		dw_i2c_data_read(port);
		regs->ic_intr_mask.raw = DW_DISABLE_ALL_I2C_INT;
		dw->state = I2C_DW_STATE_READY;

		regs->ic_clr_intr = 0;
	}

	/* Check if we are configured as a master device */
	if (regs->ic_con.bits.master_mode) {
		/* Check if the Master TX is ready for sending */
		if (regs->ic_intr_stat.bits.tx_empty) {
			dw_i2c_data_send(port);
		}

		/* Check if the Master RX buffer is full */
		if (regs->ic_intr_stat.bits.rx_full) {
			dw_i2c_data_read(port);
		}

		if ((DW_INTR_STAT_TX_ABRT | DW_INTR_STAT_TX_OVER |
		     DW_INTR_STAT_RX_OVER | DW_INTR_STAT_RX_UNDER) &
		    regs->ic_intr_stat.raw) {
				dw->state = I2C_DW_CMD_ERROR;
				regs->ic_intr_mask.raw = DW_DISABLE_ALL_I2C_INT;
				dw->state = I2C_DW_STATE_READY;
				regs->ic_clr_intr = 0;
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

			dw_i2c_data_send(port);
			value = regs->ic_clr_rd_req;
		}

		/* The slave device is ready to receive */
		if (regs->ic_intr_stat.bits.rx_full &&
		    dw->app_config.bits.is_slave_read) {
			dw_i2c_data_read(port);
		}
	}
}


static int dw_i2c_setup(struct device *dev)
{
	struct dw_i2c_dev_config * const dw = dev->driver_data;
	struct dw_i2c_rom_config const * const rom = dev->config->config_info;
	volatile struct dw_i2c_registers * const regs =
		(struct dw_i2c_registers *)rom->base_address;
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

	/* Set TX interrupt mode */
	ic_con.bits.tx_empty_ctl = 1;

	/* Set the IC_CON register */
	regs->ic_con = ic_con;
	/* END of setup IC_CON */

	/* Set RX fifo threshold level */
	regs->ic_rx_tl = (regs->ic_comp_param_1.bits.rx_buffer_depth / 2);
	/* Set TX fifo threshold level */
	regs->ic_tx_tl = (regs->ic_comp_param_1.bits.tx_buffer_depth / 2);

	return rc;
}


static int dw_i2c_transfer(struct device *dev,
			     uint8_t *write_buf, uint32_t write_len,
			     uint8_t *read_buf,  uint32_t read_len,
			     uint16_t slave_address)
{
	struct dw_i2c_rom_config const * const rom = dev->config->config_info;
	struct dw_i2c_dev_config * const dw = dev->driver_data;
	volatile struct dw_i2c_registers * const regs =
		(struct dw_i2c_registers *)rom->base_address;
	uint32_t value = 0;

	/* First step, check if there is current activity */
	if (regs->ic_status.bits.activity) {
		return DEV_FAIL;
	}

	dw->rx_len = read_len;
	dw->rx_buffer = read_buf;
	dw->tx_len = write_len;
	dw->tx_buffer = write_buf;
	dw->rx_tx_len = dw->rx_len + dw->tx_len;

	/* Disable the device controller to be able set TAR */
	regs->ic_enable.bits.enable = 0;

	dw_i2c_setup(dev);

	/* Disable interrupts */
	regs->ic_intr_mask.raw = 0;

	/* Clear interrupts */
	value = regs->ic_clr_intr;

	if (regs->ic_con.bits.master_mode) {
		/* Set address of target slave */
		regs->ic_tar.bits.ic_tar = slave_address;
		/* Enable necessary interrupts */
		regs->ic_intr_mask.raw = (DW_ENABLE_TX_INT_I2C_MASTER |
					  DW_ENABLE_RX_INT_I2C_MASTER);
	} else {
		/* Set slave address for device */
		regs->ic_sar.bits.ic_sar = slave_address;
		/* Enable necessary interrupts */
		regs->ic_intr_mask.raw = DW_ENABLE_TX_INT_I2C_SLAVE;
	}

	/* Enable controller */
	regs->ic_enable.bits.enable = 1;

	return DEV_OK;
}


static int dw_i2c_runtime_configure(struct device *dev, uint32_t config)
{
	struct dw_i2c_rom_config const * const rom = dev->config->config_info;
	struct dw_i2c_dev_config * const dw = dev->driver_data;
	volatile struct dw_i2c_registers * const regs =
		(struct dw_i2c_registers *)rom->base_address;
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


static int dw_i2c_write(struct device *dev, uint8_t *buf,
			uint32_t len, uint16_t slave_addr)
{
	struct dw_i2c_dev_config * const dw = dev->driver_data;

	dw->state = I2C_DW_CMD_SEND;

	return dw_i2c_transfer(dev, buf, len, 0, 0, slave_addr);
}


static int dw_i2c_read(struct device *dev, uint8_t *buf,
			uint32_t len, uint16_t slave_addr)
{
	struct dw_i2c_dev_config * const dw = dev->driver_data;

	dw->state = I2C_DW_CMD_RECV;

	return dw_i2c_transfer(dev, 0, 0, buf, len, slave_addr);
}


static int dw_i2c_suspend(struct device *dev)
{
	DBG("I2C: suspend called - function not yet implemented\n");
	/* TODO - add this code */
	return DEV_OK;
}


static int dw_i2c_resume(struct device *dev)
{
	DBG("I2C: resume called - function not yet implemented\n");
	/* TODO - add this code */
	return DEV_OK;
}


static struct i2c_driver_api funcs = {
	.configure = dw_i2c_runtime_configure,
	.write = dw_i2c_write,
	.read = dw_i2c_read,
	.suspend = dw_i2c_suspend,
	.resume = dw_i2c_resume,
};


#ifdef CONFIG_PCI
static inline int dw_i2c_pci_setup(struct device *dev)
{
	struct dw_i2c_rom_config *rom = dev->config->config_info;

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
#define dw_i2c_pci_setup(_unused_) (1)
#endif /* CONFIG_PCI */

int dw_i2c_initialize(struct device *port)
{
	struct dw_i2c_rom_config const * const rom = port->config->config_info;
	struct dw_i2c_dev_config * const dev = port->driver_data;
	volatile struct dw_i2c_registers *regs;

	if (!dw_i2c_pci_setup(port)) {
		return DEV_NOT_CONFIG;
	}

	regs = (struct dw_i2c_registers*) rom->base_address;

	/* verify that we have a valid DesignWare register first */
	if (regs->ic_comp_type != I2C_DW_MAGIC_KEY) {
		port->driver_api = NULL;
		DBG("I2C: DesignWare magic key not found, check base address.");
		DBG(" Stopping initialization\n");
		return DEV_NOT_CONFIG;
	}

	port->driver_api = &funcs;

	dev->app_config.raw = 0;

	rom->config_func(port);

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

	dev->state = I2C_DW_STATE_READY;

	irq_enable(rom->interrupt_vector);

	return DEV_OK;
}

/* system bindings */

#if CONFIG_I2C_DW0
#include <init.h>

void i2c_config_0_irq(struct device *port);

struct dw_i2c_rom_config i2c_config_dw_0 = {
	.base_address = CONFIG_I2C_DW0_BASE,
	.interrupt_vector = CONFIG_I2C_DW0_IRQ,
#if CONFIG_PCI
	.pci_dev.class = CONFIG_I2C_DW_CLASS,
	.pci_dev.bus = CONFIG_I2C_DW0_BUS,
	.pci_dev.dev = CONFIG_I2C_DW0_DEV,
	.pci_dev.vendor_id = CONFIG_I2C_DW_VENDOR_ID,
	.pci_dev.device_id = CONFIG_I2C_DW_DEVICE_ID,
	.pci_dev.function = CONFIG_I2C_DW0_FUNCTION,
	.pci_dev.bar = CONFIG_I2C_DW0_BAR,
#endif
	.config_func = i2c_config_0_irq
};

struct dw_i2c_dev_config i2c_0_runtime;

DECLARE_DEVICE_INIT_CONFIG(i2c_0,
			   CONFIG_I2C_DW0_NAME,
			   &dw_i2c_initialize,
			   &i2c_config_dw_0);

pure_init(i2c_0, &i2c_0_runtime);

IRQ_CONNECT_STATIC(dw_i2c_0,
		   CONFIG_I2C_DW0_IRQ,
		   CONFIG_I2C_DW0_INT_PRIORITY,
		   dw_i2c_isr_0,
		   0);

void i2c_config_0_irq(struct device *port)
{
	struct dw_i2c_rom_config *config = port->config->config_info;
	IRQ_CONFIG(dw_i2c_0, config->interrupt_vector);
}

void dw_i2c_isr_0(void *unused)
{
	dw_i2c_isr(&__initconfig_i2c_01);
}

#endif /* CONFIG_I2C_DW0 */
