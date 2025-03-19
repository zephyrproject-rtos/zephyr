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
#if defined(CONFIG_RESET)
#include <zephyr/drivers/reset.h>
#endif

#include <errno.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/sys/util.h>

#if defined(CONFIG_I2C_DW_LPSS_DMA)
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_intel_lpss.h>
#endif

#ifdef CONFIG_IOAPIC
#include <zephyr/drivers/interrupt_controller/ioapic.h>
#endif

#include "i2c_dw.h"
#include "i2c_dw_registers.h"
#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_dw);

#include "i2c-priv.h"

static inline uint32_t get_regs(const struct device *dev)
{
	return (uint32_t)DEVICE_MMIO_GET(dev);
}

#ifdef CONFIG_I2C_DW_LPSS_DMA
void i2c_dw_enable_idma(const struct device *dev, bool enable)
{
	uint32_t reg;
	uint32_t reg_base = get_regs(dev);

	if (enable) {
		write_dma_cr(DW_IC_DMA_ENABLE, reg_base);
		reg = sys_read32(reg_base + DW_IC_REG_DMA_CR);
	} else {
		reg = read_dma_cr(reg_base);
		reg &= ~DW_IC_DMA_ENABLE;
		write_dma_cr(reg, reg_base);
		reg = sys_read32(reg_base + DW_IC_REG_DMA_CR);
	}
}

void cb_i2c_idma_transfer(const struct device *dma, void *user_data, uint32_t channel, int status)
{
	const struct device *dev = (const struct device *)user_data;
	const struct i2c_dw_rom_config *const rom = dev->config;
	struct i2c_dw_dev_config *const dw = dev->data;

	dma_stop(rom->dma_dev, channel);
	i2c_dw_enable_idma(dev, false);

	if (status) {
		dw->xfr_status = true;
	} else {
		dw->xfr_status = false;
	}
}

void i2c_dw_set_fifo_th(const struct device *dev, uint8_t fifo_depth)
{
	uint32_t reg_base = get_regs(dev);

	write_tdlr(fifo_depth, reg_base);
	write_rdlr(fifo_depth - 1, reg_base);
}

inline void *i2c_dw_dr_phy_addr(const struct device *dev)
{
	struct i2c_dw_dev_config *const dw = dev->data;

	return (void *)(dw->phy_addr + DW_IC_REG_DATA_CMD);
}

int32_t i2c_dw_idma_rx_transfer(const struct device *dev)
{
	struct i2c_dw_dev_config *const dw = dev->data;
	const struct i2c_dw_rom_config *const rom = dev->config;

	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};

	if (!device_is_ready(rom->dma_dev)) {
		LOG_DBG("DMA device is not ready");
		return -ENODEV;
	}

	dma_cfg.dma_slot = 1U;
	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_burst_length = 1U;
	dma_cfg.dma_callback = cb_i2c_idma_transfer;
	dma_cfg.user_data = (void *)dev;
	dma_cfg.complete_callback_en = 0U;
	dma_cfg.error_callback_dis = 0U;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;

	dma_block_cfg.block_size = dw->xfr_len;
	dma_block_cfg.dest_address = (uint64_t)&dw->xfr_buf[0];
	dma_block_cfg.source_address = (uint64_t)i2c_dw_dr_phy_addr(dev);
	dw->xfr_status = false;

	if (dma_config(rom->dma_dev, DMA_INTEL_LPSS_RX_CHAN, &dma_cfg)) {
		LOG_DBG("Error transfer");
		return -EIO;
	}

	if (dma_start(rom->dma_dev, DMA_INTEL_LPSS_RX_CHAN)) {
		LOG_DBG("Error transfer");
		return -EIO;
	}

	i2c_dw_enable_idma(dev, true);
	i2c_dw_set_fifo_th(dev, 1);

	return 0;
}

int32_t i2c_dw_idma_tx_transfer(const struct device *dev, uint64_t data)
{
	const struct i2c_dw_rom_config *const rom = dev->config;
	struct i2c_dw_dev_config *const dw = dev->data;

	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};

	if (!device_is_ready(rom->dma_dev)) {
		LOG_DBG("DMA device is not ready");
		return -ENODEV;
	}

	dma_cfg.dma_slot = 0U;
	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_burst_length = 1U;
	dma_cfg.dma_callback = cb_i2c_idma_transfer;
	dma_cfg.user_data = (void *)dev;
	dma_cfg.complete_callback_en = 0U;
	dma_cfg.error_callback_dis = 0U;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;

	dma_block_cfg.block_size = 1;
	dma_block_cfg.source_address = (uint64_t)&data;
	dma_block_cfg.dest_address = (uint64_t)i2c_dw_dr_phy_addr(dev);
	dw->xfr_status = false;

	if (dma_config(rom->dma_dev, DMA_INTEL_LPSS_TX_CHAN, &dma_cfg)) {
		LOG_DBG("Error transfer");
		return -EIO;
	}

	if (dma_start(rom->dma_dev, DMA_INTEL_LPSS_TX_CHAN)) {
		LOG_DBG("Error transfer");
		return -EIO;
	}
	i2c_dw_enable_idma(dev, true);
	i2c_dw_set_fifo_th(dev, 1);

	return 0;
}
#endif

static inline void i2c_dw_data_ask(const struct device *dev)
{
	struct i2c_dw_dev_config *const dw = dev->data;
	uint32_t data;
	int tx_empty;
	int rx_empty;
	int cnt;
	int rx_buffer_depth, tx_buffer_depth;
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
		if ((dw->xfr_flags & I2C_MSG_STOP) && (dw->request_bytes == 1U)) {
			data |= IC_DATA_CMD_STOP;
		}

#ifdef CONFIG_I2C_TARGET
		clear_bit_intr_mask_tx_empty(reg_base);
#endif /* CONFIG_I2C_TARGET */

		write_cmd_data(data, reg_base);

		dw->rx_pending++;
		dw->request_bytes--;
		cnt--;
	}
}

static void i2c_dw_data_read(const struct device *dev)
{
	struct i2c_dw_dev_config *const dw = dev->data;
	uint32_t reg_base = get_regs(dev);

#ifdef CONFIG_I2C_DW_LPSS_DMA
	if (test_bit_status_rfne(reg_base) && (dw->xfr_len > 0)) {
		i2c_dw_idma_rx_transfer(dev);
		dw->xfr_len = 0;
		dw->rx_pending = 0;
	}
#else
	while (test_bit_status_rfne(reg_base) && (dw->xfr_len > 0)) {
		dw->xfr_buf[0] = (uint8_t)read_cmd_data(reg_base);

		dw->xfr_buf++;
		dw->xfr_len--;
		dw->rx_pending--;

		if (dw->xfr_len == 0U) {
			break;
		}
	}
#endif
	/* Nothing to receive anymore */
	if (dw->xfr_len == 0U) {
		dw->state &= ~I2C_DW_CMD_RECV;
		return;
	}
}

static int i2c_dw_data_send(const struct device *dev)
{
	struct i2c_dw_dev_config *const dw = dev->data;
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

#ifdef CONFIG_I2C_DW_LPSS_DMA
		i2c_dw_idma_tx_transfer(dev, data);
#else
		write_cmd_data(data, reg_base);
#endif
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
	struct i2c_dw_dev_config *const dw = dev->data;
	uint32_t value;
	uint32_t reg_base = get_regs(dev);

	write_intr_mask(DW_DISABLE_ALL_I2C_INT, reg_base);
	value = read_clr_intr(reg_base);

	k_sem_give(&dw->device_sync_sem);
}

#ifdef CONFIG_I2C_TARGET
static inline uint8_t i2c_dw_read_byte_non_blocking(const struct device *dev);
static inline void i2c_dw_write_byte_non_blocking(const struct device *dev, uint8_t data);
static void i2c_dw_slave_read_clear_intr_bits(const struct device *dev);
#endif

static void i2c_dw_isr(const struct device *port)
{
	struct i2c_dw_dev_config *const dw = port->data;
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
#ifdef CONFIG_I2C_DW_LPSS_DMA
		uint32_t stat = sys_read32(reg_base + IDMA_REG_INTR_STS);

		if (stat & IDMA_TX_RX_CHAN_MASK) {
			const struct i2c_dw_rom_config *const rom = port->config;
			/* Handle the DMA interrupt */
			dma_intel_lpss_isr(rom->dma_dev);
		}
#endif

		/* Bail early if there is any error. */
		if ((DW_INTR_STAT_TX_ABRT | DW_INTR_STAT_TX_OVER | DW_INTR_STAT_RX_OVER |
		     DW_INTR_STAT_RX_UNDER) &
		    intr_stat.raw) {
			dw->state = I2C_DW_CMD_ERROR;
			goto done;
		}

		/* Check if the RX FIFO reached threshold */
		if (intr_stat.bits.rx_full) {
			i2c_dw_data_read(port);
		}

#ifdef CONFIG_I2C_TARGET
		/* Check if the TX FIFO is ready for commands.
		 * TX FIFO also serves as command queue where read requests
		 * are written to TX FIFO.
		 */
		if ((dw->xfr_flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			set_bit_intr_mask_tx_empty(reg_base);
		}
#endif /* CONFIG_I2C_TARGET */

		if (intr_stat.bits.tx_empty) {
			if ((dw->xfr_flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
				ret = i2c_dw_data_send(port);
			} else {
				i2c_dw_data_ask(port);
			}

			/* If STOP is not expected, finish processing this
			 * message if there is nothing left to do anymore.
			 */
			if (((dw->xfr_len == 0U) && !(dw->xfr_flags & I2C_MSG_STOP)) ||
			    (ret != 0)) {
				goto done;
			}
		}

		/* STOP detected: finish processing this message */
		if (intr_stat.bits.stop_det) {
			value = read_clr_stop_det(reg_base);
			goto done;
		}

	} else {
#ifdef CONFIG_I2C_TARGET
		const struct i2c_target_callbacks *slave_cb = dw->slave_cfg->callbacks;
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
			/* FIFO needs to be drained here so we don't miss the next interrupt */
			do {
				data = i2c_dw_read_byte_non_blocking(port);
				if (slave_cb->write_received) {
					slave_cb->write_received(dw->slave_cfg, data);
				}
			} while (test_bit_status_rfne(reg_base));
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
	struct i2c_dw_dev_config *const dw = dev->data;
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
	if (I2C_MODE_CONTROLLER & dw->app_config) {
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
	if (I2C_MODE_CONTROLLER & dw->app_config) {
		if (I2C_ADDR_10_BITS & dw->app_config) {
			ic_tar.bits.ic_10bitaddr_master = 1U;
		} else {
			ic_tar.bits.ic_10bitaddr_master = 0U;
		}
	}

	write_tar(ic_tar.raw, reg_base);

	return 0;
}

static int i2c_dw_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			   uint16_t slave_address)
{
	struct i2c_dw_dev_config *const dw = dev->data;
	struct i2c_msg *cur_msg = msgs;
	uint8_t msg_left = num_msgs;
	uint8_t pflags;
	int ret;
	uint32_t reg_base = get_regs(dev);
	uint32_t value = 0;

	__ASSERT_NO_MSG(msgs);
	if (!num_msgs) {
		return 0;
	}

	ret = k_mutex_lock(&dw->bus_mutex, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	/* First step, check if there is current activity */
	if (test_bit_status_activity(reg_base) || (dw->state & I2C_DW_BUSY)) {
		ret = -EBUSY;
		goto error;
	}

	dw->state |= I2C_DW_BUSY;

	ret = i2c_dw_setup(dev, slave_address);
	if (ret) {
		goto error;
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
		/* Workaround for I2C scanner as DW HW does not support 0 byte transfers.*/
		if ((cur_msg->len == 0) && (cur_msg->buf != NULL)) {
			cur_msg->len = 1;
		}

		pflags = dw->xfr_flags;

		dw->xfr_buf = cur_msg->buf;
		dw->xfr_len = cur_msg->len;
		dw->xfr_flags = cur_msg->flags;
		dw->rx_pending = 0U;

		/* Need to RESTART if changing transfer direction */
		if ((pflags & I2C_MSG_RW_MASK) != (dw->xfr_flags & I2C_MSG_RW_MASK)) {
			dw->xfr_flags |= I2C_MSG_RESTART;
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
			write_intr_mask((DW_ENABLE_TX_INT_I2C_MASTER | DW_ENABLE_RX_INT_I2C_MASTER),
					reg_base);
		} else {
			/* Enable necessary interrupts */
			write_intr_mask(DW_ENABLE_TX_INT_I2C_SLAVE, reg_base);
		}

		/* Wait for transfer to be done */
		ret = k_sem_take(&dw->device_sync_sem, K_MSEC(CONFIG_I2C_DW_RW_TIMEOUT_MS));
		if (ret != 0) {
			if (test_bit_con_master_mode(reg_base)) {
				/* Trigger abort and wait for it to complete. */
				set_bit_enable_abort(reg_base);
				(void)k_sem_take(&dw->device_sync_sem, K_MSEC(1));
			}
			write_intr_mask(DW_DISABLE_ALL_I2C_INT, reg_base);
			value = read_clr_intr(reg_base);
			ret = -ETIMEDOUT;
			k_sem_reset(&dw->device_sync_sem);
			break;
		}

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

error:
	dw->state = I2C_DW_STATE_READY;
	k_mutex_unlock(&dw->bus_mutex);

	return ret;
}

static int i2c_dw_runtime_configure(const struct device *dev, uint32_t config)
{
	struct i2c_dw_dev_config *const dw = dev->data;
	const struct i2c_dw_rom_config *const rom = dev->config;
	uint32_t value = 0U;
	uint32_t rc = 0U;
	uint32_t reg_base = get_regs(dev);

	dw->app_config = config;

	/* Make sure we have a supported speed for the DesignWare model */
	/* and have setup the clock frequency and speed mode */
	switch (I2C_SPEED_GET(dw->app_config)) {
	case I2C_SPEED_STANDARD:
		/* Following the directions on DW spec page 59, IC_SS_SCL_LCNT
		 * must have register values larger than IC_FS_SPKLEN + 7
		 */
		value = I2C_STD_LCNT + rom->lcnt_offset;
		if (value <= (read_fs_spklen(reg_base) + 7)) {
			value = read_fs_spklen(reg_base) + 8;
		}

		dw->lcnt = value;

		/* Following the directions on DW spec page 59, IC_SS_SCL_HCNT
		 * must have register values larger than IC_FS_SPKLEN + 5
		 */
		value = I2C_STD_HCNT + rom->hcnt_offset;
		if (value <= (read_fs_spklen(reg_base) + 5)) {
			value = read_fs_spklen(reg_base) + 6;
		}

		dw->hcnt = value;
		break;
	case I2C_SPEED_FAST:
		/*
		 * Following the directions on DW spec page 59, IC_FS_SCL_LCNT
		 * must have register values larger than IC_FS_SPKLEN + 7
		 */
		value = I2C_FS_LCNT + rom->lcnt_offset;
		if (value <= (read_fs_spklen(reg_base) + 7)) {
			value = read_fs_spklen(reg_base) + 8;
		}

		dw->lcnt = value;

		/*
		 * Following the directions on DW spec page 59, IC_FS_SCL_HCNT
		 * must have register values larger than IC_FS_SPKLEN + 5
		 */
		value = I2C_FS_HCNT + rom->hcnt_offset;
		if (value <= (read_fs_spklen(reg_base) + 5)) {
			value = read_fs_spklen(reg_base) + 6;
		}

		dw->hcnt = value;
		break;
	case I2C_SPEED_FAST_PLUS:
		/*
		 * Following the directions on DW spec page 59, IC_FS_SCL_LCNT
		 * must have register values larger than IC_FS_SPKLEN + 7
		 */
		value = I2C_FSP_LCNT + rom->lcnt_offset;
		if (value <= (read_fs_spklen(reg_base) + 7)) {
			value = read_fs_spklen(reg_base) + 8;
		}

		dw->lcnt = value;

		/*
		 * Following the directions on DW spec page 59, IC_FS_SCL_HCNT
		 * must have register values larger than IC_FS_SPKLEN + 5
		 */
		value = I2C_FSP_HCNT + rom->hcnt_offset;
		if (value <= (read_fs_spklen(reg_base) + 5)) {
			value = read_fs_spklen(reg_base) + 6;
		}

		dw->hcnt = value;
		break;
	case I2C_SPEED_HIGH:
		if (dw->support_hs_mode) {
			value = I2C_HS_LCNT + rom->lcnt_offset;
			if (value <= (read_hs_spklen(reg_base) + 7)) {
				value = read_hs_spklen(reg_base) + 8;
			}

			dw->lcnt = value;

			value = I2C_HS_HCNT + rom->hcnt_offset;
			if (value <= (read_hs_spklen(reg_base) + 5)) {
				value = read_hs_spklen(reg_base) + 6;
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
	dw->app_config |= I2C_MODE_CONTROLLER;

	return rc;
}

#ifdef CONFIG_I2C_TARGET
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

	LOG_DBG("I2C: Host registered as Slave Device");

	return 0;
}

static int i2c_dw_slave_register(const struct device *dev, struct i2c_target_config *cfg)
{
	struct i2c_dw_dev_config *const dw = dev->data;
	uint32_t reg_base = get_regs(dev);
	int ret;

	dw->slave_cfg = cfg;
	ret = i2c_dw_set_slave_mode(dev, cfg->address);
	write_intr_mask(DW_INTR_MASK_RX_FULL | DW_INTR_MASK_RD_REQ | DW_INTR_MASK_TX_ABRT |
				DW_INTR_MASK_STOP_DET,
			reg_base);

	return ret;
}

static int i2c_dw_slave_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	struct i2c_dw_dev_config *const dw = dev->data;
	int ret;

	dw->state = I2C_DW_STATE_READY;
	ret = i2c_dw_set_master_mode(dev);

	return ret;
}

static void i2c_dw_slave_read_clear_intr_bits(const struct device *dev)
{
	struct i2c_dw_dev_config *const dw = dev->data;
	union ic_interrupt_register intr_stat;
	uint32_t reg_base = get_regs(dev);

	const struct i2c_target_callbacks *slave_cb = dw->slave_cfg->callbacks;

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
#endif /* CONFIG_I2C_TARGET */

static DEVICE_API(i2c, funcs) = {
	.configure = i2c_dw_runtime_configure,
	.transfer = i2c_dw_transfer,
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_dw_slave_register,
	.target_unregister = i2c_dw_slave_unregister,
#endif /* CONFIG_I2C_TARGET */
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

static int i2c_dw_initialize(const struct device *dev)
{
	const struct i2c_dw_rom_config *const rom = dev->config;
	struct i2c_dw_dev_config *const dw = dev->data;
	union ic_con_register ic_con;
	int ret = 0;

#if defined(CONFIG_RESET)
	if (rom->reset.dev) {
		ret = reset_line_toggle_dt(&rom->reset);
		if (ret) {
			return ret;
		}
	}
#endif

#if defined(CONFIG_PINCTRL)
	ret = pinctrl_apply_state(rom->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
	if (rom->pcie) {
		struct pcie_bar mbar;

		if (rom->pcie->bdf == PCIE_BDF_NONE) {
			return -EINVAL;
		}

		pcie_probe_mbar(rom->pcie->bdf, 0, &mbar);
		pcie_set_cmd(rom->pcie->bdf, PCIE_CONF_CMDSTAT_MEM, true);

		device_map(DEVICE_MMIO_RAM_PTR(dev), mbar.phys_addr, mbar.size, K_MEM_CACHE_NONE);

		pcie_set_cmd(rom->pcie->bdf, PCIE_CONF_CMDSTAT_MASTER, true);

#ifdef CONFIG_I2C_DW_LPSS_DMA
		uintptr_t base;

		base = DEVICE_MMIO_GET(dev) + DMA_INTEL_LPSS_OFFSET;
		dma_intel_lpss_set_base(rom->dma_dev, base);
		dma_intel_lpss_setup(rom->dma_dev);

		/* Assign physical & virtual address to dma instance */
		dw->phy_addr = mbar.phys_addr;
		dw->base_addr = (uint32_t)(DEVICE_MMIO_GET(dev) + DMA_INTEL_LPSS_OFFSET);
		sys_write32((uint32_t)dw->phy_addr,
			    DEVICE_MMIO_GET(dev) + DMA_INTEL_LPSS_REMAP_LOW);
		sys_write32((uint32_t)(dw->phy_addr >> DMA_INTEL_LPSS_ADDR_RIGHT_SHIFT),
			    DEVICE_MMIO_GET(dev) + DMA_INTEL_LPSS_REMAP_HI);
		LOG_DBG("i2c instance physical addr: [0x%lx], virtual addr: [0x%lx]", dw->phy_addr,
			dw->base_addr);
#endif
	} else
#endif
	{
		DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	}

	k_sem_init(&dw->device_sync_sem, 0, K_SEM_MAX_LIMIT);
	k_mutex_init(&dw->bus_mutex);

	uint32_t reg_base = get_regs(dev);

	clear_bit_enable_en(reg_base);

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

	dw->app_config = I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(rom->bitrate);

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

#if defined(CONFIG_RESET)
#define RESET_DW_CONFIG(n)                                                    \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, resets),                          \
		   (.reset = RESET_DT_SPEC_INST_GET(n),))
#else
#define RESET_DW_CONFIG(n)
#endif

#define I2C_DW_INIT_PCIE0(n)
#define I2C_DW_INIT_PCIE1(n) DEVICE_PCIE_INST_INIT(n, pcie),
#define I2C_DW_INIT_PCIE(n)  _CONCAT(I2C_DW_INIT_PCIE, DT_INST_ON_BUS(n, pcie))(n)

#define I2C_DEFINE_PCIE0(n)
#define I2C_DEFINE_PCIE1(n) DEVICE_PCIE_INST_DECLARE(n)
#define I2C_PCIE_DEFINE(n)  _CONCAT(I2C_DEFINE_PCIE, DT_INST_ON_BUS(n, pcie))(n)

#define I2C_DW_IRQ_FLAGS_SENSE0(n) 0
#define I2C_DW_IRQ_FLAGS_SENSE1(n) DT_INST_IRQ(n, sense)
#define I2C_DW_IRQ_FLAGS_SENSE(n)  _CONCAT(I2C_DW_IRQ_FLAGS_SENSE, DT_INST_IRQ_HAS_CELL(n, sense))
#define I2C_DW_IRQ_FLAGS(n)        I2C_DW_IRQ_FLAGS_SENSE(n)(n)

/* not PCI(e) */
#define I2C_DW_IRQ_CONFIG_PCIE0(n)                                                                 \
	static void i2c_config_##n(const struct device *port)                                      \
	{                                                                                          \
		ARG_UNUSED(port);                                                                  \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_dw_isr,                 \
			    DEVICE_DT_INST_GET(n), I2C_DW_IRQ_FLAGS(n));                           \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

/* PCI(e) with auto IRQ detection */
#define I2C_DW_IRQ_CONFIG_PCIE1(n)                                                                 \
	static void i2c_config_##n(const struct device *port)                                      \
	{                                                                                          \
		BUILD_ASSERT(DT_INST_IRQN(n) == PCIE_IRQ_DETECT,                                   \
			     "Only runtime IRQ configuration is supported");                       \
		BUILD_ASSERT(IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS),                                \
			     "DW I2C PCI needs CONFIG_DYNAMIC_INTERRUPTS");                        \
		const struct i2c_dw_rom_config *const dev_cfg = port->config;                      \
		unsigned int irq = pcie_alloc_irq(dev_cfg->pcie->bdf);                             \
		if (irq == PCIE_CONF_INTR_IRQ_NONE) {                                              \
			return;                                                                    \
		}                                                                                  \
		pcie_connect_dynamic_irq(dev_cfg->pcie->bdf, irq, DT_INST_IRQ(n, priority),        \
					 (void (*)(const void *))i2c_dw_isr,                       \
					 DEVICE_DT_INST_GET(n), I2C_DW_IRQ_FLAGS(n));              \
		pcie_irq_enable(dev_cfg->pcie->bdf, irq);                                          \
	}

#define I2C_DW_IRQ_CONFIG(n) _CONCAT(I2C_DW_IRQ_CONFIG_PCIE, DT_INST_ON_BUS(n, pcie))(n)

#define I2C_CONFIG_REG_INIT_PCIE0(n) DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),
#define I2C_CONFIG_REG_INIT_PCIE1(n)
#define I2C_CONFIG_REG_INIT(n) _CONCAT(I2C_CONFIG_REG_INIT_PCIE, DT_INST_ON_BUS(n, pcie))(n)

#define I2C_CONFIG_DMA_INIT(n)                                                                     \
	COND_CODE_1(CONFIG_I2C_DW_LPSS_DMA,				\
	(COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),			\
	(.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_IDX(n, 0)),),	\
	())), ())

#define I2C_DEVICE_INIT_DW(n)                                                                      \
	PINCTRL_DW_DEFINE(n);                                                                      \
	I2C_PCIE_DEFINE(n);                                                                        \
	static void i2c_config_##n(const struct device *port);                                     \
	static const struct i2c_dw_rom_config i2c_config_dw_##n = {                                \
		I2C_CONFIG_REG_INIT(n).config_func = i2c_config_##n,                               \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                       \
		.lcnt_offset = (int16_t)DT_INST_PROP_OR(n, lcnt_offset, 0),                        \
		.hcnt_offset = (int16_t)DT_INST_PROP_OR(n, hcnt_offset, 0),                        \
		RESET_DW_CONFIG(n) PINCTRL_DW_CONFIG(n) I2C_DW_INIT_PCIE(n)                        \
			I2C_CONFIG_DMA_INIT(n)};                                                   \
	static struct i2c_dw_dev_config i2c_##n##_runtime;                                         \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_dw_initialize, NULL, &i2c_##n##_runtime,                  \
				  &i2c_config_dw_##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,       \
				  &funcs);                                                         \
	I2C_DW_IRQ_CONFIG(n)

DT_INST_FOREACH_STATUS_OKAY(I2C_DEVICE_INIT_DW)
