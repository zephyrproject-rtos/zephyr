/* dw_i2c.c - I2C file for Design Ware */

/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2022 Andrei-Edward Popa
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <zephyr/types.h>
#include <stdlib.h>
#include <stdbool.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/pm/device.h>
#include <zephyr/arch/cpu.h>
#include <string.h>

#if defined(CONFIG_PINCTRL)
#include <zephyr/drivers/pinctrl.h>
#endif

#include <soc.h>
#include <errno.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/sys/util.h>

#ifdef CONFIG_IOAPIC
#include <zephyr/drivers/interrupt_controller/ioapic.h>
#endif

#include "i2c_dw.h"
#include "i2c_dw_registers.h"
#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_dw);

#include "i2c-priv.h"

static inline uint32_t get_regs(const struct device *dev)
{
	return (uint32_t)DEVICE_MMIO_GET(dev);
}

static inline void i2c_dw_data_ask(const struct device *dev)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	uint32_t data;
	uint8_t tx_empty;
	int8_t rx_empty;
	uint8_t cnt;
	uint8_t rx_buffer_depth, tx_buffer_depth;
	union ic_comp_param_1_register ic_comp_param_1;
	uint32_t reg_base = get_regs(dev);

	/* No more bytes to request, so command queue is no longer needed */
	if (dw->request_bytes == 0U) {
		clear_bit_intr_mask_tx_empty(reg_base);
		return;
	}

	/* Get the FIFO depth that could be from 2 to 256 from HW spec */
	ic_comp_param_1.raw = read_comp_param_1(reg_base);
	rx_buffer_depth = ic_comp_param_1.bits.rx_buffer_depth + 1;
	tx_buffer_depth = ic_comp_param_1.bits.tx_buffer_depth + 1;

	/* How many bytes we can actually ask */
	rx_empty = (rx_buffer_depth - read_rxflr(reg_base)) - dw->rx_pending;

	if (rx_empty < 0) {
		/* RX FIFO expected to be full.
		 * So don't request any bytes, yet.
		 */
		return;
	}

	/* How many empty slots in TX FIFO (as command queue) */
	tx_empty = tx_buffer_depth - read_txflr(reg_base);

	/* Figure out how many bytes we can request */
	cnt = MIN(rx_buffer_depth, dw->request_bytes);
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

		clear_bit_intr_mask_tx_empty(reg_base);

		write_cmd_data(data, reg_base);

		dw->rx_pending++;
		dw->request_bytes--;
		cnt--;
	}

}

static void i2c_dw_data_read(const struct device *dev)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	uint32_t reg_base = get_regs(dev);

	while (test_bit_status_rfne(reg_base) && (dw->xfr_len > 0)) {
		dw->xfr_buf[0] = (uint8_t)read_cmd_data(reg_base);

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


static int i2c_dw_data_send(const struct device *dev)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	uint32_t data = 0U;
	uint32_t reg_base = get_regs(dev);

	/* Nothing to send anymore, mask the interrupt */
	if (dw->xfr_len == 0U) {
		clear_bit_intr_mask_tx_empty(reg_base);

		dw->state &= ~I2C_DW_CMD_SEND;

		return 0;
	}

	while (test_bit_status_tfnt(reg_base) && (dw->xfr_len > 0)) {
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

		write_cmd_data(data, reg_base);

		dw->xfr_len--;
		dw->xfr_buf++;

		if (test_bit_intr_stat_tx_abrt(reg_base)) {
			return -EIO;
		}
	}

	return 0;
}

static inline void i2c_dw_transfer_complete(const struct device *dev)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	uint32_t value;
	uint32_t reg_base = get_regs(dev);

	write_intr_mask(DW_DISABLE_ALL_I2C_INT, reg_base);
	value = read_clr_intr(reg_base);

	k_sem_give(&dw->device_sync_sem);
}

#ifdef CONFIG_I2C_SLAVE
static inline uint8_t i2c_dw_read_byte_non_blocking(const struct device *dev);
static inline void i2c_dw_write_byte_non_blocking(const struct device *dev, uint8_t data);
static void i2c_dw_slave_read_clear_intr_bits(const struct device *dev);
#endif

static void i2c_dw_isr(const struct device *port)
{
	struct i2c_dw_dev_config * const dw = port->data;
	union ic_interrupt_register intr_stat;
	uint32_t value;
	int ret = 0;
	uint32_t reg_base = get_regs(port);

	/* Cache ic_intr_stat for processing, so there is no need to read
	 * the register multiple times.
	 */
	intr_stat.raw = read_intr_stat(reg_base);

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
	if (test_bit_con_master_mode(reg_base)) {
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
		if ((dw->xfr_flags & I2C_MSG_RW_MASK)
			    == I2C_MSG_READ) {
			set_bit_intr_mask_tx_empty(reg_base);
		}

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

		/* STOP detected: finish processing this message */
		if (intr_stat.bits.stop_det) {
			value = read_clr_stop_det(reg_base);
			goto done;
		}

	} else {
#ifdef CONFIG_I2C_SLAVE
		const struct i2c_slave_callbacks *slave_cb = dw->slave_cfg->callbacks;
		uint32_t slave_activity = test_bit_status_activity(reg_base);
		uint8_t data;

		i2c_dw_slave_read_clear_intr_bits(port);

		if (intr_stat.bits.rx_full) {
			if (dw->state != I2C_DW_CMD_SEND) {
				dw->state = I2C_DW_CMD_SEND;
				if (slave_cb->write_requested) {
					slave_cb->write_requested(dw->slave_cfg);
				}
			}
			data = i2c_dw_read_byte_non_blocking(port);
			if (slave_cb->write_received) {
				slave_cb->write_received(dw->slave_cfg, data);
			}
		}

		if (intr_stat.bits.rd_req) {
			if (slave_activity) {
				read_clr_rd_req(reg_base);
				dw->state = I2C_DW_CMD_RECV;
				if (slave_cb->read_requested) {
					slave_cb->read_requested(dw->slave_cfg, &data);
					i2c_dw_write_byte_non_blocking(port, data);
				}
				if (slave_cb->read_processed) {
					slave_cb->read_processed(dw->slave_cfg, &data);
				}
			}
		}
#endif
	}

	return;

done:
	i2c_dw_transfer_complete(port);
}


static int i2c_dw_setup(const struct device *dev, uint16_t slave_address)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	uint32_t value;
	union ic_con_register ic_con;
	union ic_tar_register ic_tar;
	uint32_t reg_base = get_regs(dev);

	ic_con.raw = 0U;

	/* Disable the device controller to be able set TAR */
	clear_bit_enable_en(reg_base);

	/* Disable interrupts */
	write_intr_mask(0, reg_base);

	/* Clear interrupts */
	value = read_clr_intr(reg_base);

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
		write_ss_scl_lcnt(dw->lcnt, reg_base);
		write_ss_scl_hcnt(dw->hcnt, reg_base);
		ic_con.bits.speed = I2C_DW_SPEED_STANDARD;

		break;
	case I2C_SPEED_FAST:
		__fallthrough;
	case I2C_SPEED_FAST_PLUS:
		LOG_DBG("I2C: speed set to FAST or FAST_PLUS");
		write_fs_scl_lcnt(dw->lcnt, reg_base);
		write_fs_scl_hcnt(dw->hcnt, reg_base);
		ic_con.bits.speed = I2C_DW_SPEED_FAST;

		break;
	case I2C_SPEED_HIGH:
		if (!dw->support_hs_mode) {
			return -EINVAL;
		}

		LOG_DBG("I2C: speed set to HIGH");
		write_hs_scl_lcnt(dw->lcnt, reg_base);
		write_hs_scl_hcnt(dw->hcnt, reg_base);
		ic_con.bits.speed = I2C_DW_SPEED_HIGH;

		break;
	default:
		LOG_DBG("I2C: invalid speed requested");
		return -EINVAL;
	}

	LOG_DBG("I2C: lcnt = %d", dw->lcnt);
	LOG_DBG("I2C: hcnt = %d", dw->hcnt);

	/* Set the IC_CON register */
	write_con(ic_con.raw, reg_base);

	/* Set RX fifo threshold level.
	 * Setting it to zero automatically triggers interrupt
	 * RX_FULL whenever there is data received.
	 *
	 * TODO: extend the threshold for multi-byte RX.
	 */
	write_rx_tl(0, reg_base);

	/* Set TX fifo threshold level.
	 * TX_EMPTY interrupt is triggered only when the
	 * TX FIFO is truly empty. So that we can let
	 * the controller do the transfers for longer period
	 * before we need to fill the FIFO again. This may
	 * cause some pauses during transfers, but this keeps
	 * the device from interrupting often.
	 */
	write_tx_tl(0, reg_base);

	ic_tar.raw = read_tar(reg_base);

	if (test_bit_con_master_mode(reg_base)) {
		/* Set address of target slave */
		ic_tar.bits.ic_tar = slave_address;
	} else {
		/* Set slave address for device */
		write_sar(slave_address, reg_base);
	}

	/* If I2C is being operated in master mode and I2C_DYNAMIC_TAR_UPDATE
	 * configuration parameter is set to Yes (1), the ic_10bitaddr_master
	 * bit in ic_tar register would control whether the DW_apb_i2c starts
	 * its transfers in 7-bit or 10-bit addressing mode.
	 */
	if (I2C_MODE_MASTER & dw->app_config) {
		if (I2C_ADDR_10_BITS & dw->app_config) {
			ic_tar.bits.ic_10bitaddr_master = 1U;
		} else {
			ic_tar.bits.ic_10bitaddr_master = 0U;
		}
	}

	write_tar(ic_tar.raw, reg_base);

	return 0;
}

static int i2c_dw_transfer(const struct device *dev,
			   struct i2c_msg *msgs, uint8_t num_msgs,
			   uint16_t slave_address)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	struct i2c_msg *cur_msg = msgs;
	uint8_t msg_left = num_msgs;
	uint8_t pflags;
	int ret;
	uint32_t reg_base = get_regs(dev);

	__ASSERT_NO_MSG(msgs);
	if (!num_msgs) {
		return 0;
	}

	/* First step, check if there is current activity */
	if (test_bit_status_activity(reg_base) || (dw->state & I2C_DW_BUSY)) {
		return -EIO;
	}

	dw->state |= I2C_DW_BUSY;

	ret = i2c_dw_setup(dev, slave_address);
	if (ret) {
		dw->state = I2C_DW_STATE_READY;
		return ret;
	}

	/* Enable controller */
	set_bit_enable_en(reg_base);

	/*
	 * While waiting at device_sync_sem, kernel can switch to idle
	 * task which in turn can call pm_system_suspend() hook of Power
	 * Management App (PMA).
	 * pm_device_busy_set() call here, would indicate to PMA that it should
	 * not execute PM policies that would turn off this ip block, causing an
	 * ongoing hw transaction to be left in an inconsistent state.
	 * Note : This is just a sample to show a possible use of the API, it is
	 * upto the driver expert to see, if he actually needs it here, or
	 * somewhere else, or not needed as the driver's suspend()/resume()
	 * can handle everything
	 */
	pm_device_busy_set(dev);

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
		if (test_bit_con_master_mode(reg_base)) {
			/* Enable necessary interrupts */
			write_intr_mask((DW_ENABLE_TX_INT_I2C_MASTER |
						  DW_ENABLE_RX_INT_I2C_MASTER), reg_base);
		} else {
			/* Enable necessary interrupts */
			write_intr_mask(DW_ENABLE_TX_INT_I2C_SLAVE, reg_base);
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

	pm_device_busy_clear(dev);

	dw->state = I2C_DW_STATE_READY;

	return ret;
}

static int i2c_dw_runtime_configure(const struct device *dev, uint32_t config)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	uint32_t	value = 0U;
	uint32_t	rc = 0U;
	uint32_t reg_base = get_regs(dev);

	dw->app_config = config;

	/* Make sure we have a supported speed for the DesignWare model */
	/* and have setup the clock frequency and speed mode */
	switch (I2C_SPEED_GET(dw->app_config)) {
	case I2C_SPEED_STANDARD:
		/* Following the directions on DW spec page 59, IC_SS_SCL_LCNT
		 * must have register values larger than IC_FS_SPKLEN + 7
		 */
		if (I2C_STD_LCNT <= (read_fs_spklen(reg_base) + 7)) {
			value = read_fs_spklen(reg_base) + 8;
		} else {
			value = I2C_STD_LCNT;
		}

		dw->lcnt = value;

		/* Following the directions on DW spec page 59, IC_SS_SCL_HCNT
		 * must have register values larger than IC_FS_SPKLEN + 5
		 */
		if (I2C_STD_HCNT <= (read_fs_spklen(reg_base) + 5)) {
			value = read_fs_spklen(reg_base) + 6;
		} else {
			value = I2C_STD_HCNT;
		}

		dw->hcnt = value;
		break;
	case I2C_SPEED_FAST:
		__fallthrough;
	case I2C_SPEED_FAST_PLUS:
		/*
		 * Following the directions on DW spec page 59, IC_FS_SCL_LCNT
		 * must have register values larger than IC_FS_SPKLEN + 7
		 */
		if (I2C_FS_LCNT <= (read_fs_spklen(reg_base) + 7)) {
			value = read_fs_spklen(reg_base) + 8;
		} else {
			value = I2C_FS_LCNT;
		}

		dw->lcnt = value;

		/*
		 * Following the directions on DW spec page 59, IC_FS_SCL_HCNT
		 * must have register values larger than IC_FS_SPKLEN + 5
		 */
		if (I2C_FS_HCNT <= (read_fs_spklen(reg_base) + 5)) {
			value = read_fs_spklen(reg_base) + 6;
		} else {
			value = I2C_FS_HCNT;
		}

		dw->hcnt = value;
		break;
	case I2C_SPEED_HIGH:
		if (dw->support_hs_mode) {
			if (I2C_HS_LCNT <= (read_hs_spklen(reg_base) + 7)) {
				value = read_hs_spklen(reg_base) + 8;
			} else {
				value = I2C_HS_LCNT;
			}

			dw->lcnt = value;

			if (I2C_HS_HCNT <= (read_hs_spklen(reg_base) + 5)) {
				value = read_hs_spklen(reg_base) + 6;
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
	value = read_clr_intr(reg_base);

	/*
	 * TEMPORARY HACK - The I2C does not work in any mode other than Master
	 * currently.  This "hack" forces us to always be configured for master
	 * mode, until we can verify that Slave mode works correctly.
	 */
	dw->app_config |= I2C_MODE_MASTER;

	return rc;
}

#ifdef CONFIG_I2C_SLAVE
static inline uint8_t i2c_dw_read_byte_non_blocking(const struct device *dev)
{
	uint32_t reg_base = get_regs(dev);

	if (!test_bit_status_rfne(reg_base)) { /* Rx FIFO must not be empty */
		return -EIO;
	}

	return (uint8_t)read_cmd_data(reg_base);
}

static inline void i2c_dw_write_byte_non_blocking(const struct device *dev, uint8_t data)
{
	uint32_t reg_base = get_regs(dev);

	if (!test_bit_status_tfnt(reg_base)) { /* Tx FIFO must not be full */
		return;
	}

	write_cmd_data(data, reg_base);
}

static int i2c_dw_set_master_mode(const struct device *dev)
{
	union ic_comp_param_1_register ic_comp_param_1;
	uint32_t reg_base = get_regs(dev);
	union ic_con_register ic_con;

	clear_bit_enable_en(reg_base);

	ic_con.bits.master_mode = 1U;
	ic_con.bits.slave_disable = 1U;
	ic_con.bits.rx_fifo_full = 0U;
	write_con(ic_con.raw, reg_base);

	set_bit_enable_en(reg_base);

	ic_comp_param_1.raw = read_comp_param_1(reg_base);

	write_tx_tl(ic_comp_param_1.bits.tx_buffer_depth + 1, reg_base);
	write_rx_tl(ic_comp_param_1.bits.rx_buffer_depth + 1, reg_base);

	return 0;
}

static int i2c_dw_set_slave_mode(const struct device *dev, uint8_t addr)
{
	uint32_t reg_base = get_regs(dev);
	union ic_con_register ic_con;

	ic_con.raw = read_con(reg_base);

	clear_bit_enable_en(reg_base);

	ic_con.bits.master_mode = 0U;
	ic_con.bits.slave_disable = 0U;
	ic_con.bits.rx_fifo_full = 1U;
	ic_con.bits.restart_en = 1U;
	ic_con.bits.stop_det = 1U;

	write_con(ic_con.raw, reg_base);
	write_sar(addr, reg_base);
	write_intr_mask(~DW_INTR_MASK_RESET, reg_base);

	set_bit_enable_en(reg_base);

	write_tx_tl(0, reg_base);
	write_rx_tl(0, reg_base);

	LOG_DBG("I2C: Host registed as Slave Device");

	return 0;
}

static int i2c_dw_slave_register(const struct device *dev,
				 struct i2c_slave_config *cfg)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	uint32_t reg_base = get_regs(dev);
	int ret;

	dw->slave_cfg = cfg;
	ret = i2c_dw_set_slave_mode(dev, cfg->address);
	write_intr_mask(DW_INTR_MASK_RX_FULL |
			DW_INTR_MASK_RD_REQ |
			DW_INTR_MASK_TX_ABRT |
			DW_INTR_MASK_STOP_DET |
			DW_INTR_MASK_START_DET, reg_base);

	return ret;
}

static int i2c_dw_slave_unregister(const struct device *dev,
				   struct i2c_slave_config *cfg)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	int ret;

	dw->state = I2C_DW_STATE_READY;
	ret = i2c_dw_set_master_mode(dev);

	return ret;
}

static void i2c_dw_slave_read_clear_intr_bits(const struct device *dev)
{
	struct i2c_dw_dev_config * const dw = dev->data;
	union ic_interrupt_register intr_stat;
	uint32_t reg_base = get_regs(dev);

	const struct i2c_slave_callbacks *slave_cb = dw->slave_cfg->callbacks;

	intr_stat.raw = read_intr_stat(reg_base);

	if (intr_stat.bits.tx_abrt) {
		read_clr_tx_abrt(reg_base);
		dw->state = I2C_DW_STATE_READY;
	}

	if (intr_stat.bits.rx_under) {
		read_clr_rx_under(reg_base);
		dw->state = I2C_DW_STATE_READY;
	}

	if (intr_stat.bits.rx_over) {
		read_clr_rx_over(reg_base);
		dw->state = I2C_DW_STATE_READY;
	}

	if (intr_stat.bits.tx_over) {
		read_clr_tx_over(reg_base);
		dw->state = I2C_DW_STATE_READY;
	}

	if (intr_stat.bits.rx_done) {
		read_clr_rx_done(reg_base);
		dw->state = I2C_DW_STATE_READY;
	}

	if (intr_stat.bits.activity) {
		read_clr_activity(reg_base);
		dw->state = I2C_DW_STATE_READY;
	}

	if (intr_stat.bits.stop_det) {
		read_clr_stop_det(reg_base);
		dw->state = I2C_DW_STATE_READY;
		if (slave_cb->stop) {
			slave_cb->stop(dw->slave_cfg);
		}
	}

	if (intr_stat.bits.start_det) {
		read_clr_start_det(reg_base);
		dw->state = I2C_DW_STATE_READY;
	}

	if (intr_stat.bits.gen_call) {
		read_clr_gen_call(reg_base);
		dw->state = I2C_DW_STATE_READY;
	}
}
#endif /* CONFIG_I2C_SLAVE */

static const struct i2c_driver_api funcs = {
	.configure = i2c_dw_runtime_configure,
	.transfer = i2c_dw_transfer,
#ifdef CONFIG_I2C_SLAVE
	.slave_register = i2c_dw_slave_register,
	.slave_unregister = i2c_dw_slave_unregister,
#endif /* CONFIG_I2C_SLAVE */
};

static int i2c_dw_initialize(const struct device *dev)
{
	const struct i2c_dw_rom_config * const rom = dev->config;
	struct i2c_dw_dev_config * const dw = dev->data;
	union ic_con_register ic_con;
	int ret = 0;

#if defined(CONFIG_PINCTRL)
	ret = pinctrl_apply_state(rom->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
	if (rom->pcie) {
		struct pcie_mbar mbar;

		if (!pcie_probe(rom->pcie_bdf, rom->pcie_id)) {
			return -EINVAL;
		}

		pcie_probe_mbar(rom->pcie_bdf, 0, &mbar);
		pcie_set_cmd(rom->pcie_bdf, PCIE_CONF_CMDSTAT_MEM, true);

		device_map(DEVICE_MMIO_RAM_PTR(dev), mbar.phys_addr,
			   mbar.size, K_MEM_CACHE_NONE);
	} else
#endif
	{
		DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	}

	k_sem_init(&dw->device_sync_sem, 0, K_SEM_MAX_LIMIT);
	uint32_t reg_base = get_regs(dev);

	/* verify that we have a valid DesignWare register first */
	if (read_comp_type(reg_base) != I2C_DW_MAGIC_KEY) {
		LOG_DBG("I2C: DesignWare magic key not found, check base "
			    "address. Stopping initialization");
		return -EIO;
	}

	/*
	 * grab the default value on initialization.  This should be set to the
	 * IC_MAX_SPEED_MODE in the hardware.  If it does support high speed we
	 * can move provide support for it
	 */
	ic_con.raw = read_con(reg_base);
	if (ic_con.bits.speed == I2C_DW_SPEED_HIGH) {
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

	return ret;
}

#if defined(CONFIG_PINCTRL)
#define PINCTRL_DW_DEFINE(n) PINCTRL_DT_INST_DEFINE(n)
#define PINCTRL_DW_CONFIG(n) .pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#else
#define PINCTRL_DW_DEFINE(n)
#define PINCTRL_DW_CONFIG(n)
#endif

#define I2C_DW_INIT_PCIE0(n)
#define I2C_DW_INIT_PCIE1(n)                                                  \
		.pcie = true,                                                 \
		.pcie_bdf = DT_INST_REG_ADDR(n),                              \
		.pcie_id = DT_INST_REG_SIZE(n),
#define I2C_DW_INIT_PCIE(n) \
	_CONCAT(I2C_DW_INIT_PCIE, DT_INST_ON_BUS(n, pcie))(n)

#define I2C_DW_IRQ_FLAGS_SENSE0(n) 0
#define I2C_DW_IRQ_FLAGS_SENSE1(n) DT_INST_IRQ(n, sense)
#define I2C_DW_IRQ_FLAGS(n) \
	_CONCAT(I2C_DW_IRQ_FLAGS_SENSE, DT_INST_IRQ_HAS_CELL(n, sense))(n)

/* not PCI(e) */
#define I2C_DW_IRQ_CONFIG_PCIE0(n)                                            \
	static void i2c_config_##n(const struct device *port)                 \
	{                                                                     \
		ARG_UNUSED(port);                                             \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	      \
			    i2c_dw_isr, DEVICE_DT_INST_GET(n),                \
			    I2C_DW_IRQ_FLAGS(n));                             \
		irq_enable(DT_INST_IRQN(n));                                  \
	}

/* PCI(e) with auto IRQ detection */
#define I2C_DW_IRQ_CONFIG_PCIE1(n)                                            \
	static void i2c_config_##n(const struct device *port)                 \
	{                                                                     \
		ARG_UNUSED(port);                                             \
		BUILD_ASSERT(DT_INST_IRQN(n) == PCIE_IRQ_DETECT,              \
			     "Only runtime IRQ configuration is supported");  \
		BUILD_ASSERT(IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS),           \
			     "DW I2C PCI needs CONFIG_DYNAMIC_INTERRUPTS");   \
		unsigned int irq = pcie_alloc_irq(DT_INST_REG_ADDR(n));       \
		if (irq == PCIE_CONF_INTR_IRQ_NONE) {                         \
			return;                                               \
		}                                                             \
		pcie_connect_dynamic_irq(DT_INST_REG_ADDR(n), irq,	      \
				     DT_INST_IRQ(n, priority),		      \
				    (void (*)(const void *))i2c_dw_isr,       \
				    DEVICE_DT_INST_GET(n),                    \
				    I2C_DW_IRQ_FLAGS(n));                     \
		pcie_irq_enable(DT_INST_REG_ADDR(n), irq);                    \
	}

#define I2C_DW_IRQ_CONFIG(n) \
	_CONCAT(I2C_DW_IRQ_CONFIG_PCIE, DT_INST_ON_BUS(n, pcie))(n)

#define I2C_DEVICE_INIT_DW(n)                                                 \
	PINCTRL_DW_DEFINE(n);                                                 \
	static void i2c_config_##n(const struct device *port);                \
	static const struct i2c_dw_rom_config i2c_config_dw_##n = {           \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                         \
		.config_func = i2c_config_##n,                                \
		.bitrate = DT_INST_PROP(n, clock_frequency),                  \
		PINCTRL_DW_CONFIG(n)                                          \
		I2C_DW_INIT_PCIE(n)                                           \
	};                                                                    \
	static struct i2c_dw_dev_config i2c_##n##_runtime;                    \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_dw_initialize, NULL,                 \
			      &i2c_##n##_runtime, &i2c_config_dw_##n,         \
			      POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,          \
			      &funcs);                                        \
	I2C_DW_IRQ_CONFIG(n)

DT_INST_FOREACH_STATUS_OKAY(I2C_DEVICE_INIT_DW)
