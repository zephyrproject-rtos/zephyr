/* dw_i2c.c - I2C file for Design Ware */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <zephyr/types.h>
#include <stdlib.h>
#include <stdbool.h>

#include <drivers/i2c.h>
#include <kernel.h>
#include <init.h>
#include <arch/cpu.h>
#include <string.h>

#include <soc.h>
#include <errno.h>
#include <sys/sys_io.h>

#include <sys/util.h>

#ifdef CONFIG_IOAPIC
#include <drivers/interrupt_controller/ioapic.h>
#endif

#include "i2c_dw.h"
#include "i2c_dw_registers.h"
#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_dw);

#include "i2c-priv.h"

static inline volatile struct i2c_dw_registers *get_regs(struct device *dev)
{
	return (volatile struct i2c_dw_registers *)DEVICE_MMIO_GET(dev);
}

static inline void i2c_dw_data_ask(struct device *dev)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	uint32_t data;
	uint8_t tx_empty;
	int8_t rx_empty;
	uint8_t cnt;

	volatile struct i2c_dw_registers * const regs = get_regs(dev);

	/* No more bytes to request, so command queue is no longer needed */
	if (dw->request_bytes == 0U) {
		regs->ic_intr_mask.bits.tx_empty = 0U;
		return;
	}

	/* How many bytes we can actually ask */
	rx_empty = (I2C_DW_FIFO_DEPTH - regs->ic_rxflr) - dw->rx_pending;

	if (rx_empty < 0) {
		/* RX FIFO expected to be full.
		 * So don't request any bytes, yet.
		 */
		return;
	}

	/* How many empty slots in TX FIFO (as command queue) */
	tx_empty = I2C_DW_FIFO_DEPTH - regs->ic_txflr;

	/* Figure out how many bytes we can request */
	cnt = MIN(I2C_DW_FIFO_DEPTH, dw->request_bytes);
	cnt = MIN(MIN(tx_empty, rx_empty), cnt);

	while (cnt > 0) {
		/* Tell controller to get another byte */
		data = IC_DATA_CMD_CMD;

		/* Send RESTART if needed */
		if (dw->xfr_flags & I2C_MSG_RESTART) {
			data |= IC_DATA_CMD_RESTART;
			dw->xfr_flags &= ~(I2C_MSG_RESTART);
		}

		/* After receiving the last byte, send STOP if needed */
		if ((dw->xfr_flags & I2C_MSG_STOP)
		    && (dw->request_bytes == 1U)) {
			data |= IC_DATA_CMD_STOP;
		}

		regs->ic_data_cmd.raw = data;

		dw->rx_pending++;
		dw->request_bytes--;
		cnt--;
	}
}

static void i2c_dw_data_read(struct device *dev)
{
	struct i2c_dw_dev_config * const dw = dev->data;

	volatile struct i2c_dw_registers * const regs = get_regs(dev);

	while (regs->ic_status.bits.rfne && (dw->xfr_len > 0)) {
		dw->xfr_buf[0] = regs->ic_data_cmd.raw;

		dw->xfr_buf++;
		dw->xfr_len--;
		dw->rx_pending--;

		if (dw->xfr_len == 0U) {
			break;
		}
	}

	/* Nothing to receive anymore */
	if (dw->xfr_len == 0U) {
		dw->state &= ~I2C_DW_CMD_RECV;
		return;
	}
}


static int i2c_dw_data_send(struct device *dev)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	uint32_t data = 0U;

	volatile struct i2c_dw_registers * const regs = get_regs(dev);

	/* Nothing to send anymore, mask the interrupt */
	if (dw->xfr_len == 0U) {
		regs->ic_intr_mask.bits.tx_empty = 0U;

		dw->state &= ~I2C_DW_CMD_SEND;

		return 0;
	}

	while (regs->ic_status.bits.tfnf && (dw->xfr_len > 0)) {
		/* We have something to transmit to a specific host */
		data = dw->xfr_buf[0];

		/* Send RESTART if needed */
		if (dw->xfr_flags & I2C_MSG_RESTART) {
			data |= IC_DATA_CMD_RESTART;
			dw->xfr_flags &= ~(I2C_MSG_RESTART);
		}

		/* Send STOP if needed */
		if ((dw->xfr_len == 1U) && (dw->xfr_flags & I2C_MSG_STOP)) {
			data |= IC_DATA_CMD_STOP;
		}

		regs->ic_data_cmd.raw = data;

		dw->xfr_len--;
		dw->xfr_buf++;

		if (regs->ic_intr_stat.bits.tx_abrt) {
			return -EIO;
		}
	}

	return 0;
}

static inline void i2c_dw_transfer_complete(struct device *dev)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	uint32_t value;

	volatile struct i2c_dw_registers * const regs = get_regs(dev);

	regs->ic_intr_mask.raw = DW_DISABLE_ALL_I2C_INT;
	value = regs->ic_clr_intr;

	k_sem_give(&dw->device_sync_sem);
}

static void i2c_dw_isr(void *arg)
{
	struct device *port = (struct device *)arg;
	struct i2c_dw_dev_config * const dw = port->data;
	union ic_interrupt_register intr_stat;
	uint32_t value;
	int ret = 0;

	volatile struct i2c_dw_registers * const regs = get_regs(port);

	/* Cache ic_intr_stat for processing, so there is no need to read
	 * the register multiple times.
	 */
	intr_stat.raw = regs->ic_intr_stat.raw;

	/*
	 * Causes of an interrupt:
	 *   - STOP condition is detected
	 *   - Transfer is aborted
	 *   - Transmit FIFO is empty
	 *   - Transmit FIFO has overflowed
	 *   - Receive FIFO is full
	 *   - Receive FIFO has overflowed
	 *   - Received FIFO has underrun
	 *   - Transmit data is required (tx_req)
	 *   - Receive data is available (rx_avail)
	 */

	LOG_DBG("I2C: interrupt received");

	/* Check if we are configured as a master device */
	if (regs->ic_con.bits.master_mode) {
		/* Bail early if there is any error. */
		if ((DW_INTR_STAT_TX_ABRT | DW_INTR_STAT_TX_OVER |
		     DW_INTR_STAT_RX_OVER | DW_INTR_STAT_RX_UNDER) &
		    intr_stat.raw) {
			dw->state = I2C_DW_CMD_ERROR;
			goto done;
		}

		/* Check if the RX FIFO reached threshold */
		if (intr_stat.bits.rx_full) {
			i2c_dw_data_read(port);
		}

		/* Check if the TX FIFO is ready for commands.
		 * TX FIFO also serves as command queue where read requests
		 * are written to TX FIFO.
		 */
		if (intr_stat.bits.tx_empty) {
			if ((dw->xfr_flags & I2C_MSG_RW_MASK)
			    == I2C_MSG_WRITE) {
				ret = i2c_dw_data_send(port);
			} else {
				i2c_dw_data_ask(port);
			}

			/* If STOP is not expected, finish processing this
			 * message if there is nothing left to do anymore.
			 */
			if (((dw->xfr_len == 0U)
			     && !(dw->xfr_flags & I2C_MSG_STOP))
			    || (ret != 0)) {
				goto done;
			}
		}
	}

	/* STOP detected: finish processing this message */
	if (intr_stat.bits.stop_det) {
		value = regs->ic_clr_stop_det;
		goto done;
	}

	return;

done:
	i2c_dw_transfer_complete(port);
}


static int i2c_dw_setup(struct device *dev, uint16_t slave_address)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	uint32_t value;
	union ic_con_register ic_con;
	volatile struct i2c_dw_registers * const regs = get_regs(dev);

	ic_con.raw = 0U;

	/* Disable the device controller to be able set TAR */
	regs->ic_enable.bits.enable = 0U;

	/* Disable interrupts */
	regs->ic_intr_mask.raw = 0U;

	/* Clear interrupts */
	value = regs->ic_clr_intr;

	/* Set master or slave mode - (initialization = slave) */
	if (I2C_MODE_MASTER & dw->app_config) {
		/*
		 * Make sure to set both the master_mode and slave_disable_bit
		 * to both 0 or both 1
		 */
		LOG_DBG("I2C: host configured as Master Device");
		ic_con.bits.master_mode = 1U;
		ic_con.bits.slave_disable = 1U;
	} else {
		return -EINVAL;
	}

	ic_con.bits.restart_en = 1U;

	/* Set addressing mode - (initialization = 7 bit) */
	if (I2C_ADDR_10_BITS & dw->app_config) {
		LOG_DBG("I2C: using 10-bit address");
		ic_con.bits.addr_master_10bit = 1U;
		ic_con.bits.addr_slave_10bit = 1U;
	}

	/* Setup the clock frequency and speed mode */
	switch (I2C_SPEED_GET(dw->app_config)) {
	case I2C_SPEED_STANDARD:
		LOG_DBG("I2C: speed set to STANDARD");
		regs->ic_ss_scl_lcnt = dw->lcnt;
		regs->ic_ss_scl_hcnt = dw->hcnt;
		ic_con.bits.speed = I2C_DW_SPEED_STANDARD;

		break;
	case I2C_SPEED_FAST:
		/* fall through */
	case I2C_SPEED_FAST_PLUS:
		LOG_DBG("I2C: speed set to FAST or FAST_PLUS");
		regs->ic_fs_scl_lcnt = dw->lcnt;
		regs->ic_fs_scl_hcnt = dw->hcnt;
		ic_con.bits.speed = I2C_DW_SPEED_FAST;

		break;
	case I2C_SPEED_HIGH:
		if (!dw->support_hs_mode) {
			return -EINVAL;
		}

		LOG_DBG("I2C: speed set to HIGH");
		regs->ic_hs_scl_lcnt = dw->lcnt;
		regs->ic_hs_scl_hcnt = dw->hcnt;
		ic_con.bits.speed = I2C_DW_SPEED_HIGH;

		break;
	default:
		LOG_DBG("I2C: invalid speed requested");
		return -EINVAL;
	}

	LOG_DBG("I2C: lcnt = %d", dw->lcnt);
	LOG_DBG("I2C: hcnt = %d", dw->hcnt);

	/* Set the IC_CON register */
	regs->ic_con = ic_con;

	/* Set RX fifo threshold level.
	 * Setting it to zero automatically triggers interrupt
	 * RX_FULL whenever there is data received.
	 *
	 * TODO: extend the threshold for multi-byte RX.
	 */
	regs->ic_rx_tl = 0U;

	/* Set TX fifo threshold level.
	 * TX_EMPTY interrupt is triggered only when the
	 * TX FIFO is truly empty. So that we can let
	 * the controller do the transfers for longer period
	 * before we need to fill the FIFO again. This may
	 * cause some pauses during transfers, but this keeps
	 * the device from interrupting often.
	 */
	regs->ic_tx_tl = 0U;

	if (regs->ic_con.bits.master_mode) {
		/* Set address of target slave */
		regs->ic_tar.bits.ic_tar = slave_address;
	} else {
		/* Set slave address for device */
		regs->ic_sar.bits.ic_sar = slave_address;
	}

	/* If I2C is being operated in master mode and I2C_DYNAMIC_TAR_UPDATE
	 * configuration parameter is set to Yes (1), the ic_10bitaddr_master
	 * bit in ic_tar register would control whether the DW_apb_i2c starts
	 * its transfers in 7-bit or 10-bit addressing mode.
	 */
	if (I2C_MODE_MASTER & dw->app_config) {
		if (I2C_ADDR_10_BITS & dw->app_config) {
			regs->ic_tar.bits.ic_10bitaddr_master = 1U;
		} else {
			regs->ic_tar.bits.ic_10bitaddr_master = 0U;
		}
	}

	return 0;
}

static int i2c_dw_transfer(struct device *dev,
			   struct i2c_msg *msgs, uint8_t num_msgs,
			   uint16_t slave_address)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	struct i2c_msg *cur_msg = msgs;
	uint8_t msg_left = num_msgs;
	uint8_t pflags;
	int ret;

	volatile struct i2c_dw_registers * const regs = get_regs(dev);

	__ASSERT_NO_MSG(msgs);
	if (!num_msgs) {
		return 0;
	}

	/* First step, check if there is current activity */
	if ((regs->ic_status.bits.activity) || (dw->state & I2C_DW_BUSY)) {
		return -EIO;
	}

	dw->state |= I2C_DW_BUSY;

	ret = i2c_dw_setup(dev, slave_address);
	if (ret) {
		dw->state = I2C_DW_STATE_READY;
		return ret;
	}

	/* Enable controller */
	regs->ic_enable.bits.enable = 1U;

	/*
	 * While waiting at device_sync_sem, kernel can switch to idle
	 * task which in turn can call sys_suspend() hook of Power
	 * Management App (PMA).
	 * device_busy_set() call here, would indicate to PMA that it should not
	 * execute PM policies that would turn off this ip block, causing an
	 * ongoing hw transaction to be left in an inconsistent state.
	 * Note : This is just a sample to show a possible use of the API, it is
	 * upto the driver expert to see, if he actually needs it here, or
	 * somewhere else, or not needed as the driver's suspend()/resume()
	 * can handle everything
	 */
	device_busy_set(dev);

		/* Process all the messages */
	while (msg_left > 0) {
		pflags = dw->xfr_flags;

		dw->xfr_buf = cur_msg->buf;
		dw->xfr_len = cur_msg->len;
		dw->xfr_flags = cur_msg->flags;
		dw->rx_pending = 0U;

		/* Need to RESTART if changing transfer direction */
		if ((pflags & I2C_MSG_RW_MASK)
		    != (dw->xfr_flags & I2C_MSG_RW_MASK)) {
			dw->xfr_flags |= I2C_MSG_RESTART;
		}

		/* Send STOP if this is the last message */
		if (msg_left == 1U) {
			dw->xfr_flags |= I2C_MSG_STOP;
		}

		dw->state &= ~(I2C_DW_CMD_SEND | I2C_DW_CMD_RECV);

		if ((dw->xfr_flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			dw->state |= I2C_DW_CMD_SEND;
			dw->request_bytes = 0U;
		} else {
			dw->state |= I2C_DW_CMD_RECV;
			dw->request_bytes = dw->xfr_len;
		}

		/* Enable interrupts to trigger ISR */
		if (regs->ic_con.bits.master_mode) {
			/* Enable necessary interrupts */
			regs->ic_intr_mask.raw = (DW_ENABLE_TX_INT_I2C_MASTER |
						  DW_ENABLE_RX_INT_I2C_MASTER);
		} else {
			/* Enable necessary interrupts */
			regs->ic_intr_mask.raw = DW_ENABLE_TX_INT_I2C_SLAVE;
		}

		/* Wait for transfer to be done */
		k_sem_take(&dw->device_sync_sem, K_FOREVER);

		if (dw->state & I2C_DW_CMD_ERROR) {
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

	device_busy_clear(dev);

	dw->state = I2C_DW_STATE_READY;

	return ret;
}

static int i2c_dw_runtime_configure(struct device *dev, uint32_t config)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	uint32_t	value = 0U;
	uint32_t	rc = 0U;

	volatile struct i2c_dw_registers * const regs = get_regs(dev);

	dw->app_config = config;

	/* Make sure we have a supported speed for the DesignWare model */
	/* and have setup the clock frequency and speed mode */
	switch (I2C_SPEED_GET(dw->app_config)) {
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
			rc = -EINVAL;
		}
		break;
	default:
		/* TODO change */
		rc = -EINVAL;
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
	dw->app_config |= I2C_MODE_MASTER;

	return rc;
}

static const struct i2c_driver_api funcs = {
	.configure = i2c_dw_runtime_configure,
	.transfer = i2c_dw_transfer,
};

static int i2c_dw_initialize(struct device *dev)
{
	const struct i2c_dw_rom_config * const rom = dev->config;
	struct i2c_dw_dev_config * const dw = dev->data;
	volatile struct i2c_dw_registers *regs;

#ifdef I2C_DW_PCIE_ENABLED
	if (rom->pcie) {
		uintptr_t mmio_phys_addr;

		if (!pcie_probe(rom->pcie_bdf, rom->pcie_id)) {
			return -EINVAL;
		}

		mmio_phys_addr = pcie_get_mbar(rom->pcie_bdf, 0);
		pcie_set_cmd(rom->pcie_bdf, PCIE_CONF_CMDSTAT_MEM, true);

		device_map(DEVICE_MMIO_RAM_PTR(dev), mmio_phys_addr,
			   0x1000, K_MEM_CACHE_NONE);
	} else
#endif
	{
		DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	}

	k_sem_init(&dw->device_sync_sem, 0, UINT_MAX);

	regs = get_regs(dev);
	/* verify that we have a valid DesignWare register first */
	if (regs->ic_comp_type != I2C_DW_MAGIC_KEY) {
		LOG_DBG("I2C: DesignWare magic key not found, check base "
			    "address. Stopping initialization");
		return -EIO;
	}

	/*
	 * grab the default value on initialization.  This should be set to the
	 * IC_MAX_SPEED_MODE in the hardware.  If it does support high speed we
	 * can move provide support for it
	 */
	if (regs->ic_con.bits.speed == I2C_DW_SPEED_HIGH) {
		LOG_DBG("I2C: high speed supported");
		dw->support_hs_mode = true;
	} else {
		LOG_DBG("I2C: high speed NOT supported");
		dw->support_hs_mode = false;
	}

	rom->config_func(dev);

	dw->app_config = I2C_MODE_MASTER | i2c_map_dt_bitrate(rom->bitrate);

	if (i2c_dw_runtime_configure(dev, dw->app_config) != 0) {
		LOG_DBG("I2C: Cannot set default configuration");
		return -EIO;
	}

	dw->state = I2C_DW_STATE_READY;

	return 0;
}

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)
#include <i2c_dw_port_0.h>
#endif

#if DT_NODE_HAS_STATUS(DT_DRV_INST(1), okay)
#include <i2c_dw_port_1.h>
#endif

#if DT_NODE_HAS_STATUS(DT_DRV_INST(2), okay)
#include <i2c_dw_port_2.h>
#endif

#if DT_NODE_HAS_STATUS(DT_DRV_INST(3), okay)
#include <i2c_dw_port_3.h>
#endif

#if DT_NODE_HAS_STATUS(DT_DRV_INST(4), okay)
#include <i2c_dw_port_4.h>
#endif

#if DT_NODE_HAS_STATUS(DT_DRV_INST(5), okay)
#include <i2c_dw_port_5.h>
#endif

#if DT_NODE_HAS_STATUS(DT_DRV_INST(6), okay)
#include <i2c_dw_port_6.h>
#endif

#if DT_NODE_HAS_STATUS(DT_DRV_INST(7), okay)
#include <i2c_dw_port_7.h>
#endif
