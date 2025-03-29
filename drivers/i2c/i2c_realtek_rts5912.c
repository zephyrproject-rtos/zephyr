/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
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

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/realtek-rts5912-pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <errno.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/sys/util.h>
#include <reg/reg_gpio.h>
#include "i2c_realtek_rts5912.h"
#include "i2c_realtek_rts5912_api.h"
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(i2c_rts5912, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

#define REALTEK_RTS5912_PINMUX_GET_GPIO_PIN(n)                                                     \
	(((((n) >> REALTEK_RTS5912_GPIO_LOW_POS) & REALTEK_RTS5912_GPIO_LOW_MSK)) |                \
	 (((((n) >> REALTEK_RTS5912_GPIO_HIGH_POS) & REALTEK_RTS5912_GPIO_HIGH_MSK)) << 5))

#define RECOVERY_TIME 30 /* in ms */

static inline uint32_t get_regs(const struct device *dev)
{
	return (uint32_t)DEVICE_MMIO_GET(dev);
}

static int i2c_rts5912_error_chk(const struct device *dev)
{
	struct i2c_rts5912_dev_config *const bus = dev->data;
	uint32_t reg_base = get_regs(dev);
	union ic_interrupt_register intr_stat;
	union ic_txabrt_register ic_txabrt_src;
	uint32_t value;
	/* Cache ic_intr_stat and txabrt_src for processing,
	 * so there is no need to read the register multiple times.
	 */
	intr_stat.raw = read_intr_stat(reg_base);
	ic_txabrt_src.raw = read_txabrt_src(reg_base);
	/* NACK and SDA_STUCK are below TX_Abort */
	if (intr_stat.bits.tx_abrt) {
		/* check 7bit NACK Tx Abort */
		if (ic_txabrt_src.bits.ADDR7BNACK) {
			bus->state |= I2C_RTS5912_NACK;
			LOG_ERR("NACK on %s", dev->name);
		}
		/* check SDA stuck low Tx abort, need to do bus recover */
		if (ic_txabrt_src.bits.SDASTUCKLOW) {
			bus->state |= I2C_RTS5912_SDA_STUCK;
			LOG_ERR("SDA Stuck Low on %s", dev->name);
		}
		/* clear RTS5912_INTR_STAT_TX_ABRT */
		value = read_clr_tx_abrt(reg_base);
	}
	/* check SCL stuck low */
	if (intr_stat.bits.scl_stuck_low) {
		bus->state |= I2C_RTS5912_SCL_STUCK;
		LOG_ERR("SCL Stuck Low on %s", dev->name);
	}
	if (bus->state & I2C_RTS5912_ERR_MASK) {
		bus->need_setup = 1;
		LOG_ERR("IO Fail on %s", dev->name);
		return -EIO;
	}
	return 0;
}

static inline void i2c_rts5912_data_ask(const struct device *dev)
{
	struct i2c_rts5912_dev_config *const bus = dev->data;
	uint32_t data;
	int tx_empty;
	int rx_empty;
	int cnt;
	int rx_buffer_depth, tx_buffer_depth;
	union ic_comp_param_1_register ic_comp_param_1;
	uint32_t reg_base = get_regs(dev);

	/* No more bytes to request, so command queue is no longer needed */
	if (bus->request_bytes == 0U) {
		clear_bit_intr_mask_tx_empty(reg_base);
		return;
	}

	/* Get the FIFO depth that could be from 2 to 256 from HW spec */
	ic_comp_param_1.raw = read_comp_param_1(reg_base);
	rx_buffer_depth = ic_comp_param_1.bits.rx_buffer_depth + 1;
	tx_buffer_depth = ic_comp_param_1.bits.tx_buffer_depth + 1;

	/* How many bytes we can actually ask */
	rx_empty = (rx_buffer_depth - read_rxflr(reg_base)) - bus->rx_pending;

	if (rx_empty < 0) {
		/* RX FIFO expected to be full.
		 * So don't request any bytes, yet.
		 */
		return;
	}

	/* How many empty slots in TX FIFO (as command queue) */
	tx_empty = tx_buffer_depth - read_txflr(reg_base);

	/* Figure out how many bytes we can request */
	cnt = MIN(rx_buffer_depth, bus->request_bytes);
	cnt = MIN(MIN(tx_empty, rx_empty), cnt);

	while (cnt > 0) {
		/* Tell controller to get another byte */
		data = IC_DATA_CMD_CMD;

		/* Send RESTART if needed */
		if (bus->xfr_flags & I2C_MSG_RESTART) {
			data |= IC_DATA_CMD_RESTART;
			bus->xfr_flags &= ~(I2C_MSG_RESTART);
		}

		/* After receiving the last byte, send STOP if needed */
		if ((bus->xfr_flags & I2C_MSG_STOP) && (bus->request_bytes == 1U)) {
			data |= IC_DATA_CMD_STOP;
		}

#ifdef CONFIG_I2C_TARGET
		clear_bit_intr_mask_tx_empty(reg_base);
#endif /* CONFIG_I2C_TARGET */

		write_cmd_data(data, reg_base);

		if (i2c_rts5912_error_chk(dev)) {
			return;
		}

		bus->rx_pending++;
		bus->request_bytes--;
		cnt--;
	}
}

static void i2c_rts5912_data_read(const struct device *dev)
{
	struct i2c_rts5912_dev_config *const data = dev->data;
	uint32_t reg_base = get_regs(dev);

	while (test_bit_status_rfne(reg_base) && (data->xfr_len > 0)) {
		data->xfr_buf[0] = (uint8_t)read_cmd_data(reg_base);

		if (i2c_rts5912_error_chk(dev)) {
			return;
		}

		data->xfr_buf++;
		data->xfr_len--;
		data->rx_pending--;

		if (data->xfr_len == 0U) {
			break;
		}
	}

	/* Nothing to receive anymore */
	if (data->xfr_len == 0U) {
		data->state &= ~I2C_RTS5912_CMD_RECV;
		return;
	}
}

static int i2c_rts5912_recover_bus(const struct device *dev)
{
	static volatile GPIO_Type *pinctrl_base =
		(volatile GPIO_Type *)(DT_REG_ADDR(DT_NODELABEL(pinctrl)));

	struct i2c_rts5912_dev_config *const bus = dev->data;
	volatile uint32_t *GPIO_SDA = (volatile uint32_t *)&(pinctrl_base->GCR[bus->sda_gpio]);
	volatile uint32_t *GPIO_SCL = (volatile uint32_t *)&(pinctrl_base->GCR[bus->scl_gpio]);
	uint32_t GPIO_SDA_TEMP = *GPIO_SDA;
	uint32_t GPIO_SCL_TEMP = *GPIO_SCL;
	uint32_t reg_base = get_regs(dev);
	uint32_t value;
	uint32_t start;
	int ret = true;

	LOG_DBG("starting bus recover");
	LOG_DBG("sda_gpio=%d, GPIO_SDA=0x%08x", bus->sda_gpio, *GPIO_SDA);
	LOG_DBG("scl_gpio=%d, GPIO_SCL=0x%08x", bus->scl_gpio, *GPIO_SCL);

	if (k_sem_take(&bus->bus_sem, K_FOREVER)) {
		LOG_ERR("bus lock fail");
		return -1;
	}

	LOG_DBG("bus locked");
	/* disable all interrupt mask */
	write_intr_mask(RTS5912_DISABLE_ALL_I2C_INT, reg_base);
	/* enable controller to make sure function works */
	set_bit_enable_en(reg_base);

	if (bus->last_state & I2C_RTS5912_SDA_STUCK) {
		/*
		 * initiate the SDA Recovery Mechanism
		 * (that is, send at most 9 SCL clocks and STOP to release the
		 * SDA line) and then this bit gets auto clear
		 */
		LOG_DBG("CLK Recovery Start");
		/* initiate the Master Clock Reset */
		start = k_uptime_get_32();
		set_bit_enable_clk_reset(reg_base);
		while (test_bit_enable_clk_reset(reg_base) &&
		       (k_uptime_get_32() - start < RECOVERY_TIME)) {
			;
		}
		/* check if SCL bus clk is not reset */
		if (test_bit_enable_clk_reset(reg_base)) {
			LOG_ERR("ERROR: CLK recovery Fail");
			ret = false;
		} else {
			LOG_DBG("CLK Recovery Success");
		}

		LOG_DBG("SDA Recovery Start");
		start = k_uptime_get_32();
		set_bit_enable_sdarecov(reg_base);
		while (test_bit_enable_sdarecov(reg_base) &&
		       (k_uptime_get_32() - start < RECOVERY_TIME)) {
			;
		}
		/* Check if bus is not clear */
		if (test_bit_status_sdanotrecov(reg_base)) {
			LOG_ERR("ERROR: SDA Recovery Fail");
			ret = false;
		} else {
			LOG_DBG("SDA Recovery Success");
		}
	} else if (bus->last_state & I2C_RTS5912_SCL_STUCK) {
		/* the controller initiates the transfer abort */
		LOG_DBG("ABORT transfer");
		start = k_uptime_get_32();
		set_bit_enable_abort(reg_base);
		while (test_bit_enable_abort(reg_base) &&
		       (k_uptime_get_32() - start < RECOVERY_TIME)) {
			;
		}
		/* check if Controller is not abort */
		if (test_bit_enable_abort(reg_base)) {
			LOG_ERR("ERROR: ABORT Fail!");
			ret = false;
		} else {
			LOG_DBG("ABORT seccuess");
		}
	}
	value = read_clr_intr(reg_base);
	value = read_clr_tx_abrt(reg_base);
	/* disable controller */
	clear_bit_enable_en(reg_base);

	/* set SCL line to GPIO input mode */
	*GPIO_SCL = 0x8002;
	k_busy_wait(500);
	/* check does SCL line released to high level */
	if ((*GPIO_SCL & GPIO_GCR_PINSTS_Msk) == 0) {
		LOG_ERR("SCL still in Low! scl_gpio=%d, GPIO_SCL=0x%08x", bus->scl_gpio, *GPIO_SCL);
		*GPIO_SCL = GPIO_SCL_TEMP;
		/* unlock the bus */
		k_sem_give(&bus->bus_sem);
		return -1;
	}
	/* set high level to scl and sda line */
	*GPIO_SCL = 0x28003;
	*GPIO_SDA = 0x28003;
	k_busy_wait(10);

	/* send a ACK */
	*GPIO_SDA = 0x8003;
	k_busy_wait(10);
	*GPIO_SCL = 0x8003;
	k_busy_wait(10);
	*GPIO_SDA = 0x28003;
	k_busy_wait(10);
	/* send dummy clock */
	for (int i = 0; i < 9; i++) {
		*GPIO_SCL = 0x00028003;
		k_busy_wait(50);
		*GPIO_SCL = 0x00008003;
		k_busy_wait(50);
	}
	/* send a stop bit */
	*GPIO_SDA = 0x8003;
	k_busy_wait(10);
	*GPIO_SCL = 0x28003;
	k_busy_wait(10);
	*GPIO_SDA = 0x28003;
	k_busy_wait(10);

	/* set GPIO functoin to I2C */
	*GPIO_SCL = GPIO_SCL_TEMP;
	*GPIO_SDA = GPIO_SDA_TEMP;
	LOG_DBG("SCL=0x%08x, SDA=0x%08x", *GPIO_SCL, *GPIO_SDA);

	/* enable controller */
	set_bit_enable_en(reg_base);

	start = k_uptime_get_32();
	set_bit_enable_abort(reg_base);
	while (test_bit_enable_abort(reg_base) && (k_uptime_get_32() - start < RECOVERY_TIME)) {
		;
	}
	if (test_bit_enable_abort(reg_base)) {
		LOG_ERR("ERROR: ABORT Fail!");
		ret = false;
	} else {
		LOG_DBG("ABORT seccuess");
	}
	/* disable controller */
	clear_bit_enable_en(reg_base);
	/* reset last state */
	bus->last_state = I2C_RTS5912_STATE_READY;
	/* unlock the bus */
	k_sem_give(&bus->bus_sem);
	LOG_DBG("bus unlock");
	if (!ret) {
		LOG_ERR("ERROR: Bus Recover Fail, a slave device may be faulty or require a power "
			"reset");
		return -1;
	}
	LOG_DBG("BUS Recover success");
	return 0;
}

static int i2c_rts5912_data_send(const struct device *dev)
{
	struct i2c_rts5912_dev_config *const bus = dev->data;
	uint32_t data = 0U;
	uint32_t reg_base = get_regs(dev);

	/* Nothing to send anymore, mask the interrupt */
	if (bus->xfr_len == 0U) {
		clear_bit_intr_mask_tx_empty(reg_base);

		bus->state &= ~I2C_RTS5912_CMD_SEND;

		return 0;
	}

	while (test_bit_status_tfnt(reg_base) && (bus->xfr_len > 0)) {
		/* We have something to transmit to a specific host */
		data = bus->xfr_buf[0];

		/* Send RESTART if needed */
		if (bus->xfr_flags & I2C_MSG_RESTART) {
			data |= IC_DATA_CMD_RESTART;
			bus->xfr_flags &= ~(I2C_MSG_RESTART);
		}

		/* Send STOP if needed */
		if ((bus->xfr_len == 1U) && (bus->xfr_flags & I2C_MSG_STOP)) {
			data |= IC_DATA_CMD_STOP;
		}

		write_cmd_data(data, reg_base);
		bus->xfr_len--;
		bus->xfr_buf++;

		if (i2c_rts5912_error_chk(dev)) {
			return -EIO;
		}
	}

	return 0;
}

static inline void i2c_rts5912_transfer_complete(const struct device *dev)
{
	struct i2c_rts5912_dev_config *const data = dev->data;
	uint32_t value;
	uint32_t reg_base = get_regs(dev);

	write_intr_mask(RTS5912_DISABLE_ALL_I2C_INT, reg_base);

	value = read_clr_intr(reg_base);

	k_sem_give(&data->device_sync_sem);
}

#ifdef CONFIG_I2C_TARGET
static inline uint8_t i2c_rts5912_read_byte_non_blocking(const struct device *dev);
static inline void i2c_rts5912_write_byte_non_blocking(const struct device *dev, uint8_t data);
static void i2c_rts5912_slave_read_clear_intr_bits(const struct device *dev);
#endif

#ifdef CONFIG_I2C_CALLBACK
static void i2c_rts5912_async_done(const struct device *dev, struct i2c_rts5912_dev_config *data,
				   int result);
static void i2c_rts5912_async_iter(const struct device *dev);
#endif

static void i2c_rts5912_isr(const struct device *port)
{
	struct i2c_rts5912_dev_config *const bus = port->data;
	union ic_interrupt_register intr_stat;
	union ic_txabrt_register ic_txabrt;
	uint32_t value;
	int ret = 0;
	uint32_t reg_base = get_regs(port);

	/* Cache ic_intr_stat for processing, so there is no need to read
	 * the register multiple times.
	 */
	intr_stat.raw = read_rawintr_stat(reg_base);
	ic_txabrt.raw = read_txabrt_src(reg_base);
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
	 *   - scl_low_for_while (scl_stuck_low)
	 */

	LOG_DBG("I2C: interrupt received reg = 0x%x, raw = 0x%x", reg_base, intr_stat.raw);

	/* Check if we are configured as a master device */
	if (test_bit_con_master_mode(reg_base)) {
		/* Bail early if there is any error. */
		if ((RTS5912_INTR_STAT_SCL_STUCK_LOW | RTS5912_INTR_STAT_TX_ABRT |
		     RTS5912_INTR_STAT_TX_OVER | RTS5912_INTR_STAT_RX_OVER |
		     RTS5912_INTR_STAT_RX_UNDER) &
		    intr_stat.raw) {
			bus->state = I2C_RTS5912_CMD_ERROR;
			LOG_ERR("CMD ERROR on %s", port->name);
			i2c_rts5912_error_chk(port);
			bus->need_setup = true;
			goto done;
		}

		/* Check if the RX FIFO reached threshold */
		if (intr_stat.bits.rx_full) {
			i2c_rts5912_data_read(port);
		}

#ifdef CONFIG_I2C_TARGET
		/* Check if the TX FIFO is ready for commands.
		 * TX FIFO also serves as command queue where read requests
		 * are written to TX FIFO.
		 */
		if ((bus->xfr_flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			set_bit_intr_mask_tx_empty(reg_base);
		}
#endif /* CONFIG_I2C_TARGET */

		if (intr_stat.bits.tx_empty) {
			if ((bus->xfr_flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
				ret = i2c_rts5912_data_send(port);
			} else {
				i2c_rts5912_data_ask(port);
			}
			/* If STOP is not expected, finish processing this
			 * message if there is nothing left to do anymore.
			 */
			if (((bus->xfr_len == 0U) && !(bus->xfr_flags & I2C_MSG_STOP)) ||
			    (ret != 0)) {
				goto done;
			}
		}

		/* STOP detected: finish processing this message */
		if (intr_stat.bits.stop_det) {
			value = read_clr_stop_det(reg_base);
			bus->need_setup = 1;
			goto done;
		}
	} else {
#ifdef CONFIG_I2C_TARGET
		const struct i2c_target_callbacks *slave_cb = bus->slave_cfg->callbacks;
		uint32_t slave_activity = test_bit_status_activity(reg_base);
		uint8_t data;

		i2c_rts5912_slave_read_clear_intr_bits(port);

		if (intr_stat.bits.rx_full) {
			if (bus->state != I2C_RTS5912_CMD_SEND) {
				bus->state = I2C_RTS5912_CMD_SEND;
				if (slave_cb->write_requested) {
					slave_cb->write_requested(bus->slave_cfg);
				}
			}
			data = i2c_rts5912_read_byte_non_blocking(port);
			if (slave_cb->write_received) {
				slave_cb->write_received(bus->slave_cfg, data);
			}
		}

		if (intr_stat.bits.rd_req) {
			if (slave_activity) {
				read_clr_rd_req(reg_base);
				bus->state = I2C_RTS5912_CMD_RECV;
				if (slave_cb->read_requested) {
					slave_cb->read_requested(bus->slave_cfg, &data);
					i2c_rts5912_write_byte_non_blocking(port, data);
				}
				if (slave_cb->read_processed) {
					slave_cb->read_processed(bus->slave_cfg, &data);
				}
			}
		}
#endif
	}

	return;

done:
#ifdef CONFIG_I2C_CALLBACK
	if (bus->cb != NULL) {
		/* Async transfer */
		if (bus->state & I2C_RTS5912_ERR_MASK) {
			if ((bus->state & I2C_RTS5912_SDA_STUCK) ||
			    (bus->state & I2C_RTS5912_SCL_STUCK)) {
				ret = -ETIME;
			} else if ((bus->state & I2C_RTS5912_NACK) ||
				   (bus->state & I2C_RTS5912_CMD_ERROR)) {
				ret = -EIO;
			}
			LOG_ERR("result = %d", ret);
			i2c_rts5912_async_done(port, bus, ret);
		} else if (bus->msg_left == 1) {
			LOG_DBG("EDONE\r\n");
			i2c_rts5912_async_done(port, bus, 0);
		} else {
			LOG_DBG("ITER\r\n");
			bus->msg++;
			bus->msg_left--;
			write_intr_mask(RTS5912_DISABLE_ALL_I2C_INT, reg_base);
			value = read_clr_intr(reg_base);
			i2c_rts5912_async_iter(port);
		}
		return;
	}
#endif /* CONFIG_I2C_CALLBACK */
	i2c_rts5912_transfer_complete(port);
}

static int i2c_rts5912_setup(const struct device *dev, uint16_t slave_address)
{
	struct i2c_rts5912_dev_config *const bus = dev->data;
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
	if (I2C_MODE_CONTROLLER & bus->app_config) {
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
	ic_con.bits.bus_clear = 1U;
	write_sdatimeout(0x27AC40, reg_base);
	write_scltimeout(0x27AC40, reg_base);

	/* Set addressing mode - (initialization = 7 bit) */
	if (I2C_ADDR_10_BITS & bus->app_config) {
		LOG_DBG("I2C: using 10-bit address");
		ic_con.bits.addr_master_10bit = 1U;
		ic_con.bits.addr_slave_10bit = 1U;
	}

	/* Setup the clock frequency and speed mode */
	switch (I2C_SPEED_GET(bus->app_config)) {
	case I2C_SPEED_STANDARD:
		LOG_DBG("I2C: speed set to STANDARD");
		write_ss_scl_lcnt(bus->lcnt, reg_base);
		write_ss_scl_hcnt(bus->hcnt, reg_base);
		ic_con.bits.speed = I2C_RTS5912_SPEED_STANDARD;

		break;
	case I2C_SPEED_FAST:
		LOG_DBG("I2C: speed set to FAST");
		write_fs_scl_lcnt(bus->lcnt, reg_base);
		write_fs_scl_hcnt(bus->hcnt, reg_base);
		ic_con.bits.speed = I2C_RTS5912_SPEED_FAST;

		break;
	case I2C_SPEED_FAST_PLUS:
		LOG_DBG("I2C: speed set to FAST_PLUS");
		write_fs_scl_lcnt(bus->lcnt, reg_base);
		write_fs_scl_hcnt(bus->hcnt, reg_base);
		ic_con.bits.speed = I2C_RTS5912_SPEED_FAST_PLUS;

		break;
	case I2C_SPEED_HIGH:
		if (!bus->support_hs_mode) {
			return -EINVAL;
		}

		LOG_DBG("I2C: speed set to HIGH");
		write_hs_scl_lcnt(bus->lcnt, reg_base);
		write_hs_scl_hcnt(bus->hcnt, reg_base);
		ic_con.bits.speed = I2C_RTS5912_SPEED_HIGH;

		break;
	default:
		LOG_DBG("I2C: invalid speed requested");
		return -EINVAL;
	}

	LOG_DBG("I2C: lcnt = %d", bus->lcnt);
	LOG_DBG("I2C: hcnt = %d", bus->hcnt);
	LOG_DBG("slave_address = %02x", slave_address);
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
	 * bit in ic_tar register would control whether the RTS5912_apb_i2c starts
	 * its transfers in 7-bit or 10-bit addressing mode.
	 */
	if (I2C_MODE_CONTROLLER & bus->app_config) {
		if (I2C_ADDR_10_BITS & bus->app_config) {
			ic_tar.bits.ic_10bitaddr_master = 1U;
		} else {
			ic_tar.bits.ic_10bitaddr_master = 0U;
		}
	}

	write_tar(ic_tar.raw, reg_base);

	return 0;
}

static int i2c_rts5912_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				uint16_t slave_address)
{
	struct i2c_rts5912_dev_config *const bus = dev->data;
	struct i2c_msg *cur_msg = msgs;
	uint8_t msg_left = num_msgs;
	uint8_t pflags;
	int ret;
	uint32_t reg_base = get_regs(dev);

	__ASSERT_NO_MSG(msgs);
	if (!num_msgs) {
		return 0;
	}

	ret = k_sem_take(&bus->bus_sem, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	/* First step, check if there is current activity */
	if ((test_bit_status_activity(reg_base) || (bus->state & I2C_RTS5912_BUSY)) &&
	    bus->need_setup) {
		ret = -EBUSY;
		goto error;
	}

	if (bus->last_state & I2C_RTS5912_STUCK_ERR_MASK) {
		/* try to recovery */
		if (i2c_rts5912_recover_bus(dev)) {
			bus->state = bus->last_state;
			ret = -ETIME;
			goto error;
		}
	}

	bus->state |= I2C_RTS5912_BUSY;

	if (bus->need_setup == 1) {
		LOG_DBG("setup, %x", reg_base);
		ret = i2c_rts5912_setup(dev, slave_address);
		if (ret) {
			goto error;
		}

		/* Enable controller */
		set_bit_enable_en(reg_base);
		bus->need_setup = 0;
	}

	pm_device_busy_set(dev);

	/* Process all the messages */
	while (msg_left > 0) {
		pflags = bus->xfr_flags;

		bus->xfr_buf = cur_msg->buf;
		bus->xfr_len = cur_msg->len;
		bus->xfr_flags = cur_msg->flags;
		bus->rx_pending = 0U;

		/* Need to RESTART if changing transfer direction */
		if ((pflags & I2C_MSG_RW_MASK) != (bus->xfr_flags & I2C_MSG_RW_MASK)) {
			bus->xfr_flags |= I2C_MSG_RESTART;
		}

		bus->state &= ~(I2C_RTS5912_CMD_SEND | I2C_RTS5912_CMD_RECV);

		if ((bus->xfr_flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			bus->state |= I2C_RTS5912_CMD_SEND;
			bus->request_bytes = 0U;
		} else {
			bus->state |= I2C_RTS5912_CMD_RECV;
			bus->request_bytes = bus->xfr_len;
		}

		/* Enable interrupts to trigger ISR */
		if (test_bit_con_master_mode(reg_base)) {
			/* Enable necessary interrupts */
			write_intr_mask((RTS5912_ENABLE_TX_INT_I2C_MASTER |
					 RTS5912_ENABLE_RX_INT_I2C_MASTER),
					reg_base);
		} else {
			/* Enable necessary interrupts */
			write_intr_mask(RTS5912_ENABLE_TX_INT_I2C_SLAVE, reg_base);
		}

		/* Wait for transfer to be done */
		k_sem_take(&bus->device_sync_sem, K_FOREVER);

		if (bus->state & I2C_RTS5912_ERR_MASK) {
			if ((bus->state & I2C_RTS5912_SDA_STUCK) ||
			    (bus->state & I2C_RTS5912_SCL_STUCK)) {
				ret = -ETIME;
			} else if ((bus->state & I2C_RTS5912_NACK) ||
				   (bus->state & I2C_RTS5912_CMD_ERROR)) {
				ret = -EIO;
			}
			break;
		}

		/* avoid some i2c devices can't read data success */
		k_busy_wait(350);
		/* Something wrong if there is something left to do */
		if (bus->xfr_len > 0) {
			ret = -EIO;
			break;
		}

		cur_msg++;
		msg_left--;
	}

	pm_device_busy_clear(dev);

error:
	bus->last_state = bus->state;
	bus->state = I2C_RTS5912_STATE_READY;
	k_sem_give(&bus->bus_sem);

	return ret;
}

#ifdef CONFIG_I2C_CALLBACK
static void i2c_rts5912_async_done(const struct device *dev, struct i2c_rts5912_dev_config *bus,
				   int result)
{
	i2c_callback_t cb = bus->cb;
	void *userdata = bus->userdata;
	uint32_t value;
	uint32_t reg_base = get_regs(dev);

	bus->msg = 0;
	bus->msgs = NULL;
	bus->msg_left = 0;
	bus->cb = NULL;
	bus->addr = 0;
	write_intr_mask(RTS5912_DISABLE_ALL_I2C_INT, reg_base);
	value = read_clr_intr(reg_base);

	bus->last_state = bus->state;
	bus->state = I2C_RTS5912_STATE_READY;
	k_sem_give(&bus->bus_sem);

	pm_device_busy_clear(dev);
	/* Callback may wish to start another transfer */
	cb(dev, result, userdata);
}

/* Start a transfer asynchronously */
static void i2c_rts5912_async_iter(const struct device *dev)
{
	struct i2c_rts5912_dev_config *const bus = dev->data;
	uint8_t pflags;
	uint32_t reg_base = get_regs(dev);
	struct i2c_msg *msg = &bus->msgs[bus->msg];

	pflags = bus->xfr_flags;

	bus->xfr_buf = msg->buf;
	bus->xfr_len = msg->len;
	bus->xfr_flags = msg->flags;
	bus->rx_pending = 0U;

	/* Need to RESTART if changing transfer direction */
	if ((pflags & I2C_MSG_RW_MASK) != (bus->xfr_flags & I2C_MSG_RW_MASK)) {
		bus->xfr_flags |= I2C_MSG_RESTART;
	}

	bus->state &= ~(I2C_RTS5912_CMD_SEND | I2C_RTS5912_CMD_RECV);

	if ((bus->xfr_flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
		bus->state |= I2C_RTS5912_CMD_SEND;
		bus->request_bytes = 0U;
	} else {
		bus->state |= I2C_RTS5912_CMD_RECV;
		bus->request_bytes = bus->xfr_len;
	}

	/* Enable interrupts to trigger ISR */
	if (test_bit_con_master_mode(reg_base)) {
		/* Enable necessary interrupts */
		write_intr_mask(
			(RTS5912_ENABLE_TX_INT_I2C_MASTER | RTS5912_ENABLE_RX_INT_I2C_MASTER),
			reg_base);
	} else {
		/* Enable necessary interrupts */
		write_intr_mask(RTS5912_ENABLE_TX_INT_I2C_SLAVE, reg_base);
	}
}

static int i2c_rts5912_transfer_cb(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				   uint16_t slave_address, i2c_callback_t cb, void *userdata)
{
	struct i2c_rts5912_dev_config *const bus = dev->data;
	uint32_t reg_base = get_regs(dev);
	int ret;

	__ASSERT_NO_MSG(msgs);
	if (!num_msgs) {
		return 0;
	}

	ret = k_sem_take(&bus->bus_sem, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	/* First step, check if there is current activity */
	if ((test_bit_status_activity(reg_base) || (bus->state & I2C_RTS5912_BUSY)) &&
	    bus->need_setup) {
		ret = -EBUSY;
		goto error;
	}

	if (bus->last_state & I2C_RTS5912_STUCK_ERR_MASK) {
		/* try to recovery */
		if (i2c_rts5912_recover_bus(dev)) {
			bus->state = bus->last_state;
			ret = -ETIME;
			goto error;
		}
	}

	bus->state |= I2C_RTS5912_BUSY;

	if (bus->need_setup == 1) {
		LOG_DBG("setup, %x", reg_base);
		ret = i2c_rts5912_setup(dev, slave_address);
		if (ret) {
			goto error;
		}

		/* Enable controller */
		set_bit_enable_en(reg_base);
		bus->need_setup = 0;
	}

	pm_device_busy_set(dev);

	bus->msg = 0;
	bus->msg_left = num_msgs;
	bus->msgs = msgs;
	bus->addr = slave_address;
	bus->cb = cb;
	bus->userdata = userdata;

	i2c_rts5912_async_iter(dev);

	return 0;

error:
	bus->state = I2C_RTS5912_STATE_READY;
	k_sem_give(&bus->bus_sem);

	if (ret == -EBUSY) {
		cb(dev, -EBUSY, userdata);
	}

	return ret;
}
#endif

static int i2c_rts5912_runtime_configure(const struct device *dev, uint32_t config)
{
	struct i2c_rts5912_dev_config *const bus = dev->data;
	uint32_t value = 0U;
	uint32_t rc = 0U;
	uint32_t reg_base = get_regs(dev);

	bus->app_config = config;

	/* Make sure we have a supported speed for the Realtek model */
	/* and have setup the clock frequency and speed mode */
	switch (I2C_SPEED_GET(bus->app_config)) {
	case I2C_SPEED_STANDARD:
		/* Following the directions on RTS5912 spec page 59, IC_SS_SCL_LCNT
		 * must have register values larger than IC_FS_SPKLEN + 7
		 */
		if (I2C_STD_LCNT <= (read_fs_spklen(reg_base) + 7)) {
			value = read_fs_spklen(reg_base) + 8;
		} else {
			value = I2C_STD_LCNT;
		}

		bus->lcnt = value;

		/* Following the directions on RTS5912 spec page 59, IC_SS_SCL_HCNT
		 * must have register values larger than IC_FS_SPKLEN + 5
		 */
		if (I2C_STD_HCNT <= (read_fs_spklen(reg_base) + 5)) {
			value = read_fs_spklen(reg_base) + 6;
		} else {
			value = I2C_STD_HCNT;
		}

		bus->hcnt = value;
		break;
	case I2C_SPEED_FAST:
		/* Following the directions on RTS5912 spec page 59, IC_FS_SCL_LCNT
		 * must have register values larger than IC_FS_SPKLEN + 7
		 */
		if (I2C_FS_LCNT <= (read_fs_spklen(reg_base) + 7)) {
			value = read_fs_spklen(reg_base) + 8;
		} else {
			value = I2C_FS_LCNT;
		}

		bus->lcnt = value;

		/* Following the directions on RTS5912 spec page 59, IC_FS_SCL_HCNT
		 * must have register values larger than IC_FS_SPKLEN + 5
		 */
		if (I2C_FS_HCNT <= (read_fs_spklen(reg_base) + 5)) {
			value = read_fs_spklen(reg_base) + 6;
		} else {
			value = I2C_FS_HCNT;
		}

		bus->hcnt = value;

		break;
	case I2C_SPEED_FAST_PLUS:
		/*
		 * Following the directions on RTS5912 spec page 59, IC_FS_SCL_LCNT
		 * must have register values larger than IC_FS_SPKLEN + 7
		 */
		if (I2C_FSP_LCNT <= (read_fs_spklen(reg_base) + 7)) {
			value = read_fs_spklen(reg_base) + 8;
		} else {
			value = I2C_FSP_LCNT;
		}

		bus->lcnt = value;

		/*
		 * Following the directions on RTS5912 spec page 59, IC_FS_SCL_HCNT
		 * must have register values larger than IC_FS_SPKLEN + 5
		 */
		if (I2C_FSP_HCNT <= (read_fs_spklen(reg_base) + 5)) {
			value = read_fs_spklen(reg_base) + 6;
		} else {
			value = I2C_FSP_HCNT;
		}

		bus->hcnt = value;
		break;
	case I2C_SPEED_HIGH:
		if (bus->support_hs_mode) {
			if (I2C_HS_LCNT <= (read_hs_spklen(reg_base) + 7)) {
				value = read_hs_spklen(reg_base) + 8;
			} else {
				value = I2C_HS_LCNT;
			}

			bus->lcnt = value;

			if (I2C_HS_HCNT <= (read_hs_spklen(reg_base) + 5)) {
				value = read_hs_spklen(reg_base) + 6;
			} else {
				value = I2C_HS_HCNT;
			}

			bus->hcnt = value;
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
	bus->app_config |= I2C_MODE_CONTROLLER;

	return rc;
}

#ifdef CONFIG_I2C_TARGET
static inline uint8_t i2c_rts5912_read_byte_non_blocking(const struct device *dev)
{
	uint32_t reg_base = get_regs(dev);

	if (!test_bit_status_rfne(reg_base)) { /* Rx FIFO must not be empty */
		return -EIO;
	}

	return (uint8_t)read_cmd_data(reg_base);
}

static inline void i2c_rts5912_write_byte_non_blocking(const struct device *dev, uint8_t data)
{
	uint32_t reg_base = get_regs(dev);

	if (!test_bit_status_tfnt(reg_base)) { /* Tx FIFO must not be full */
		return;
	}

	write_cmd_data(data, reg_base);
}

static int i2c_rts5912_set_master_mode(const struct device *dev)
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

static int i2c_rts5912_set_slave_mode(const struct device *dev, uint8_t addr)
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
	write_intr_mask(~RTS5912_INTR_MASK_RESET, reg_base);

	set_bit_enable_en(reg_base);

	write_tx_tl(0, reg_base);
	write_rx_tl(0, reg_base);

	LOG_DBG("I2C: Host registered as Slave Device");

	return 0;
}

static int i2c_rts5912_slave_register(const struct device *dev, struct i2c_target_config *cfg)
{
	struct i2c_rts5912_dev_config *const data = dev->data;
	uint32_t reg_base = get_regs(dev);
	int ret;

	data->slave_cfg = cfg;
	ret = i2c_rts5912_set_slave_mode(dev, cfg->address);
	write_intr_mask(RTS5912_INTR_MASK_RX_FULL | RTS5912_INTR_MASK_RD_REQ |
				RTS5912_INTR_MASK_TX_ABRT | RTS5912_INTR_MASK_STOP_DET |
				RTS5912_INTR_MASK_START_DET,
			reg_base);

	return ret;
}

static int i2c_rts5912_slave_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	struct i2c_rts5912_dev_config *const data = dev->data;
	int ret;

	data->state = I2C_RTS5912_STATE_READY;
	ret = i2c_rts5912_set_master_mode(dev);

	return ret;
}

static void i2c_rts5912_slave_read_clear_intr_bits(const struct device *dev)
{
	struct i2c_rts5912_dev_config *const data = dev->data;
	union ic_interrupt_register intr_stat;
	uint32_t reg_base = get_regs(dev);

	const struct i2c_target_callbacks *slave_cb = data->slave_cfg->callbacks;

	intr_stat.raw = read_intr_stat(reg_base);

	if (intr_stat.bits.tx_abrt) {
		read_clr_tx_abrt(reg_base);
		data->state = I2C_RTS5912_STATE_READY;
	}

	if (intr_stat.bits.rx_under) {
		read_clr_rx_under(reg_base);
		data->state = I2C_RTS5912_STATE_READY;
	}

	if (intr_stat.bits.rx_over) {
		read_clr_rx_over(reg_base);
		data->state = I2C_RTS5912_STATE_READY;
	}

	if (intr_stat.bits.tx_over) {
		read_clr_tx_over(reg_base);
		data->state = I2C_RTS5912_STATE_READY;
	}

	if (intr_stat.bits.rx_done) {
		read_clr_rx_done(reg_base);
		data->state = I2C_RTS5912_STATE_READY;
	}

	if (intr_stat.bits.activity) {
		read_clr_activity(reg_base);
		data->state = I2C_RTS5912_STATE_READY;
	}

	if (intr_stat.bits.stop_det) {
		read_clr_stop_det(reg_base);
		data->state = I2C_RTS5912_STATE_READY;
		if (slave_cb->stop) {
			slave_cb->stop(data->slave_cfg);
		}
	}

	if (intr_stat.bits.start_det) {
		read_clr_start_det(reg_base);
		data->state = I2C_RTS5912_STATE_READY;
	}

	if (intr_stat.bits.gen_call) {
		read_clr_gen_call(reg_base);
		data->state = I2C_RTS5912_STATE_READY;
	}
}
#endif /* CONFIG_I2C_TARGET */

static DEVICE_API(i2c, funcs) = {
	.configure = i2c_rts5912_runtime_configure,
	.transfer = i2c_rts5912_transfer,
#ifdef CONFIG_I2C_CALLBACK
	.transfer_cb = i2c_rts5912_transfer_cb,
#endif
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_rts5912_slave_register,
	.target_unregister = i2c_rts5912_slave_unregister,
#endif /* CONFIG_I2C_TARGET */
	.recover_bus = i2c_rts5912_recover_bus,
};

static int i2c_rts5912_initialize(const struct device *dev)
{
	const struct i2c_rts5912_rom_config *const rom = dev->config;
	struct i2c_rts5912_dev_config *const data = dev->data;
	union ic_con_register ic_con;
	int ret = 0;
	int tmp = 0;

	ret = pinctrl_apply_state(rom->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	if (!device_is_ready(rom->clk_dev)) {
		return -ENODEV;
	}
	ret = clock_control_on(rom->clk_dev, (clock_control_subsys_t)&rom->sccon_cfg);
	if (ret != 0) {
		return ret;
	}

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	k_sem_init(&data->device_sync_sem, 0, K_SEM_MAX_LIMIT);
	k_sem_init(&data->bus_sem, 1, 1);

	uint32_t reg_base = get_regs(dev);
	/* clear enable register */
	clear_bit_enable_en(reg_base);
	/* disable block mode */
	clear_bit_enable_block(reg_base);

	/* verify that we have a valid Realtek register first */
	if (read_comp_type(reg_base) != I2C_RTS5912_MAGIC_KEY) {
		LOG_DBG("I2C: Realtek magic key not found, check base "
			"address. Stopping initialization");
		return -EIO;
	}

	/*
	 * grab the default value on initialization.  This should be set to the
	 * IC_MAX_SPEED_MODE in the hardware.  If it does support high speed we
	 * can move provide support for it
	 */
	ic_con.raw = read_con(reg_base);
	if (ic_con.bits.speed == I2C_RTS5912_SPEED_HIGH) {
		LOG_DBG("I2C: high speed supported");
		data->support_hs_mode = true;
	} else {
		LOG_DBG("I2C: high speed NOT supported");
		data->support_hs_mode = false;
	}

	rom->config_func(dev);

	data->app_config = I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(rom->bitrate);

	if (i2c_rts5912_runtime_configure(dev, data->app_config) != 0) {
		LOG_DBG("I2C: Cannot set default configuration");
		return -EIO;
	}

	data->state = I2C_RTS5912_STATE_READY;
	data->need_setup = 1;
	if (rom->pcfg->states->pin_cnt == 2) {
		data->scl_gpio =
			REALTEK_RTS5912_PINMUX_GET_GPIO_PIN((uint32_t)rom->pcfg->states->pins[0]);
		data->sda_gpio =
			REALTEK_RTS5912_PINMUX_GET_GPIO_PIN((uint32_t)rom->pcfg->states->pins[1]);
		if (data->scl_gpio > data->sda_gpio) {
			tmp = data->scl_gpio;
			data->scl_gpio = data->sda_gpio;
			data->sda_gpio = tmp;
		}
		LOG_DBG("sda_gpio=%d, scl_gpio=%d", data->sda_gpio, data->scl_gpio);
	}
	return ret;
}

#define DEV_CONFIG_CLK_DEV_INIT(n)                                                                 \
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                          \
	.sccon_cfg = {                                                                             \
		.clk_grp = DT_INST_CLOCKS_CELL_BY_NAME(n, i2c, clk_grp),                           \
		.clk_idx = DT_INST_CLOCKS_CELL_BY_NAME(n, i2c, clk_idx),                           \
	}
#define PINCTRL_RTS5912_DEFINE(n) PINCTRL_DT_INST_DEFINE(n)
#define PINCTRL_RTS5912_CONFIG(n) .pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),

#define I2C_RTS5912_IRQ_CONFIG(n)                                                                  \
	static void i2c_config_##n(const struct device *port)                                      \
	{                                                                                          \
		ARG_UNUSED(port);                                                                  \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_rts5912_isr,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define I2C_CONFIG_REG_INIT(n) DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),

#define I2C_DEVICE_INIT_RTS5912(n)                                                                 \
	PINCTRL_RTS5912_DEFINE(n);                                                                 \
	static void i2c_config_##n(const struct device *port);                                     \
	static const struct i2c_rts5912_rom_config i2c_config_rts5912_##n = {                      \
		I2C_CONFIG_REG_INIT(n).config_func = i2c_config_##n,                               \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                       \
		PINCTRL_RTS5912_CONFIG(n) DEV_CONFIG_CLK_DEV_INIT(n)};                             \
	static struct i2c_rts5912_dev_config i2c_##n##_runtime;                                    \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_rts5912_initialize, NULL, &i2c_##n##_runtime,             \
				  &i2c_config_rts5912_##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,  \
				  &funcs);                                                         \
	I2C_RTS5912_IRQ_CONFIG(n)

DT_INST_FOREACH_STATUS_OKAY(I2C_DEVICE_INIT_RTS5912)
