/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_unicomm_i2c

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_ti_unicomm, CONFIG_I2C_LOG_LEVEL);

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>

#include "i2c_ti_unicomm.h"

/*
 * Convenience Macros
 */

#define I2CC_IS_TX_FIFO_FULL(i2cc_base)                                                            \
	((sys_read32(i2cc_base + UNICOMM_I2CC_STATUS) & UNICOMM_I2CC_STATUS_TXFIFOFULL_BIT) != 0)

#define I2CT_IS_RX_FIFO_EMPTY(i2ct_base)                                                           \
	((sys_read32(i2ct_base + UNICOMM_I2CT_STATUS) & I2CT_STATUS_RXFIFOEMPTY_BIT) != 0)

#define I2CC_IS_BUSY(i2cc_base)                                                                    \
	((sys_read32(i2cc_base + UNICOMM_I2CC_STATUS) & UNICOMM_I2CC_STATUS_BUSY) != 0)

#define I2CC_IS_RX_FIFO_EMPTY(i2cc_base)                                                           \
	((sys_read32(i2cc_base + UNICOMM_I2CC_STATUS) & 0x00000800U) != 0)

#define UPDATE_REG(reg_offset, value, mask)                                                        \
	{                                                                                          \
		uint32_t tmp = sys_read32(reg_offset);                                             \
		tmp = tmp & ~(mask);                                                               \
		sys_write32(tmp | ((value) & (mask)), reg_offset);                                 \
	}

/*
 * Config and Data structs
 */

enum i2c_ti_unicomm_state {
	UC_I2CT_STARTED,
	UC_I2CT_TX_BUSY,
	UC_I2CT_RX_BUSY,
	UC_I2CT_PREEMPTED,
	UC_I2C_TIMEOUT,
	UC_I2C_ERROR
};

struct i2c_ti_unicomm_config {
	const struct pinctrl_dev_config *pcfg;

	uint32_t unicomm_inst_base;
	uint32_t unicomm_i2cc_base;
	uint32_t unicomm_i2ct_base;

	bool unicomm_is_advanced;

	uint8_t clkdiv; /* Clock divide ratio. Register value: 0=div1, 1=div2, ... 7=div8 */

	uint32_t busclk_hz; /* BUSCLK input frequency in Hz (before clkdiv) */

	uint32_t tx_fifo_threshold;
	uint32_t rx_fifo_threshold;

	uint32_t timer_period;

	void (*irq_config_func)(const struct device *dev);
};

struct i2c_ti_unicomm_data {
	volatile enum i2c_ti_unicomm_state state;

#if defined(CONFIG_I2C_TARGET)
	struct i2c_target_config *target_cfg;

	bool is_target;
#endif
};

/*
 * Helper functions
 */

/* Reset unicomm instance */
static inline void unicomm_reset(uint32_t base)
{
	sys_write32(RESET_CTL_KEY_UNLOCK | RESET_CTL_STICKY_BIT_CLEAR | RESET_CTL_ASSERT_RESET,
			base + UNICOMM_RESET_CTL);
}

/* Enable power for UNICOMM instance */
static inline void unicomm_enable_power(uint32_t base)
{
	sys_write32(PWREN_KEY | PWREN_ENABLE, base + UNICOMM_POWER_EN);
	k_sleep(K_CYC(20));
}

static int i2cc_fill_tx_fifo(uint32_t base, uint8_t *buf, uint32_t len)
{
	int n_filled_bytes = 0;

	for (; n_filled_bytes < len; n_filled_bytes++) {
		if (!(I2CC_IS_TX_FIFO_FULL(base))) {
			sys_write32(buf[n_filled_bytes], base + UNICOMM_I2CC_TXDATA);
		} else {
			break;
		}
	}

	return n_filled_bytes;
}

static void i2cc_set_flags(const struct device *dev, uint32_t flags)
{
	const struct i2c_ti_unicomm_config *cfg = dev->config;

	UPDATE_REG(cfg->unicomm_i2cc_base + UNICOMM_I2CC_CONTROL, flags,
		   UNICOMM_I2CC_CONTROL_START_MASK | UNICOMM_I2CC_CONTROL_STOP_MASK |
			   UNICOMM_I2CC_CONTROL_ACK_MASK | UNICOMM_I2CC_CONTROL_FRAME_START_MASK);
}

static inline void i2cc_set_target_address(uint32_t base, uint16_t addr, bool dir_transmit)
{
	UPDATE_REG(base + UNICOMM_I2CC_TARGET_ADDRESS,
		   (addr << I2CC_TARGET_ADDRESS_OFFSET) |
			   ((dir_transmit) ? I2CC_TARGET_ADDRESS_DIRECTION_TRANSMIT
					   : I2CC_TARGET_ADDRESS_DIRECTION_RECEIVE),
		   UNICOMM_I2CC_TARGET_ADDRESS_ADDR_MASK |
			   UNICOMM_I2CC_TARGET_ADDRESS_DIRECTION_MASK);
}

static inline void i2cc_send_frame(uint32_t base)
{
	UPDATE_REG(base + UNICOMM_I2CC_CONTROL, I2CC_CONTROL_START_FRAME,
		   UNICOMM_I2CC_CONTROL_FRAME_START_MASK);
}

static inline void i2cc_wait_not_busy(uint32_t base)
{
	while (I2CC_IS_BUSY(base)) {
		k_sleep(K_CYC(20));
	}
}

static inline void i2cc_set_blen(uint32_t base, uint32_t len)
{
	UPDATE_REG(base + UNICOMM_I2CC_CONTROL, (len << I2CC_CONTROL_BLEN_OFFSET),
		   UNICOMM_I2CC_CONTROL_BLEN_MASK);
}

static inline void i2cc_set_enable(uint32_t base, bool enable)
{
	uint32_t tmp = sys_read32(base + UNICOMM_I2CC_CONFIG);

	if (enable) {
		tmp |= I2CC_CONFIG_ENABLE;
	} else {
		tmp &= ~I2CC_CONFIG_ENABLE;
	}
	sys_write32(tmp, base + UNICOMM_I2CC_CONFIG);
}

#if defined(CONFIG_I2C_TARGET)
static inline void i2ct_clear_tx_fifo(uint32_t base)
{
	sys_write32(sys_read32(base + UNICOMM_I2CT_IFLS) | I2CT_IFLS_TXCLR_MASK,
			base + UNICOMM_I2CT_IFLS);
	while ((sys_read32(base + UNICOMM_I2CT_STATUS) & I2CT_STATUS_TXFE_BIT) == 0) {
	}
	sys_write32(sys_read32(base + UNICOMM_I2CT_IFLS) & ~I2CT_IFLS_TXCLR_MASK,
			base + UNICOMM_I2CT_IFLS);
}

static inline void i2ct_send_nack(uint32_t base)
{
	UPDATE_REG(base + UNICOMM_I2CT_ACKCTL, I2CT_ACKCTL_ACKOEN_ENABLE | I2CT_ACKCTL_ACKOVAL_NACK,
		   I2CT_ACKCTL_ACKOEN_MASK | I2CT_ACKCTL_ACKOVAL_MASK);
}
#endif /* CONFIG_I2C_TARGET */

static int i2c_ti_unicomm_init(const struct device *dev)
{
	const struct i2c_ti_unicomm_config *cfg = dev->config;
	struct i2c_ti_unicomm_data *data = dev->data;
	int err;

	unicomm_reset(cfg->unicomm_inst_base);

	/* Reset Peripheral */
	sys_write32(0, cfg->unicomm_i2cc_base + UNICOMM_I2CC_CONTROL);

	unicomm_enable_power(cfg->unicomm_inst_base);

	/* Set instance mode to I2CC */
	sys_write32(IPMODE_CONTROLLER, cfg->unicomm_inst_base + UNICOMM_MODE);

	/* Configure clock divide ratio and select BUSCLK as clock source */
	sys_write32(cfg->clkdiv, cfg->unicomm_i2cc_base + UNICOMM_I2CC_CLKDIV);
	sys_write32(I2CC_CLKSEL_BUSCLK_ENABLE, cfg->unicomm_i2cc_base + UNICOMM_I2CC_CLKSEL);

	/* Apply pinctrl config */
	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	/* Set timer period */
	sys_write32(cfg->timer_period, cfg->unicomm_i2cc_base + UNICOMM_I2CC_TPR);

	/* Enable module with clock stretching */
	sys_write32(sys_read32(cfg->unicomm_i2cc_base + UNICOMM_I2CC_CONFIG) | I2CC_CONFIG_ENABLE |
				I2CC_CONFIG_CLKSTRETCH_ENABLE,
			cfg->unicomm_i2cc_base + UNICOMM_I2CC_CONFIG);

#if defined(CONFIG_I2C_TARGET)
	data->is_target = false;
#endif

	LOG_INF("I2C Controller init done for 0x%08x", cfg->unicomm_i2cc_base);

	return 0;
}

static int i2c_ti_unicomm_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_ti_unicomm_config *cfg = dev->config;
	struct i2c_ti_unicomm_data *data = dev->data;
	uint32_t speed_hz;
	uint32_t functional_clk_hz;
	uint32_t tpr;

	/* Reset a controller device */
	if ((dev_config & I2C_MODE_CONTROLLER)) {
		/* Changing controller/target mode at runtime is not supported */
		if (data->is_target == true) {
			return -ENOTSUP;
		}

		int ret = i2c_ti_unicomm_init(dev);
		if (ret < 0) {
			return ret;
		}
	} else {
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		speed_hz = 100000U;
		break;
	case I2C_SPEED_FAST:
		speed_hz = 400000U;
		break;
	case I2C_SPEED_FAST_PLUS:
		speed_hz = 1000000U;
		break;
	default:
		return -ENOTSUP;
	}

	/* Functional clock = BUSCLK / (clkdiv_reg_value + 1) */
	functional_clk_hz = cfg->busclk_hz / ((uint32_t)cfg->clkdiv + 1U);

	/* TPR = functional_clk_hz / (SCL_LP_HP * speed_hz) - 1 */
	tpr = functional_clk_hz / (I2CC_SCL_LP_HP * speed_hz);
	if (tpr == 0U) {
		return -EINVAL;
	}
	tpr -= 1U;

	if (tpr < 1U || tpr > 127U) {
		return -EINVAL;
	}

	/* Disable module, update TPR, re-enable */
	i2cc_set_enable(cfg->unicomm_i2cc_base, false);

	sys_write32(tpr, cfg->unicomm_i2cc_base + UNICOMM_I2CC_TPR);

	i2cc_set_enable(cfg->unicomm_i2cc_base, true);

	return 0;
}

static int i2c_ti_unicomm_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				   uint16_t addr)
{
	const struct i2c_ti_unicomm_config *cfg = dev->config;

	/* For each entry of msgs, loop over bytes of msgs[i].buf, filling TX FIFO and
	 * starting a transfer */
	for (int i = 0; i < num_msgs; i++) {
		struct i2c_msg msg = msgs[i];

		char *buf_ptr = msg.buf;
		int bytes_sent = 0;

		if ((msg.flags & (uint32_t)I2C_MSG_READ) != I2C_MSG_READ) {
			i2cc_set_target_address(cfg->unicomm_i2cc_base, addr, DIRECTION_WRITE);

			if (cfg->unicomm_is_advanced) {
				/* Advanced: set BLEN, pre-fill FIFO, trigger once, then keep
				 * filling FIFO until all bytes are queued.
				 */
				int filled =
					i2cc_fill_tx_fifo(cfg->unicomm_i2cc_base, buf_ptr, msg.len);

				buf_ptr += filled;
				bytes_sent += filled;

				i2cc_set_flags(dev, I2CC_CONTROL_START_ENABLE |
								I2CC_CONTROL_STOP_ENABLE |
								I2CC_CONTROL_ACK_ENABLE);

				i2cc_set_blen(cfg->unicomm_i2cc_base, msg.len);

				i2cc_send_frame(cfg->unicomm_i2cc_base);

				/* Polling Write */
				while (bytes_sent < msg.len) {
					if (!I2CC_IS_TX_FIFO_FULL(cfg->unicomm_i2cc_base)) {
						filled = i2cc_fill_tx_fifo(cfg->unicomm_i2cc_base,
									   buf_ptr,
									   msg.len - bytes_sent);

						buf_ptr += filled;
						bytes_sent += filled;
					} else if (!I2CC_IS_BUSY(cfg->unicomm_i2cc_base)) {
						/* HW aborted (e.g. NACK): FIFO not draining */
						return -EIO;
					}
				}

				i2cc_wait_not_busy(cfg->unicomm_i2cc_base);
			} else {
				/* Non-advanced: one byte per FRM_START trigger.
				 * START_ENABLE only on the first byte to generate START+address.
				 * Subsequent bytes use START_DISABLE to continue the same
				 * transaction (no RESTART, no repeated address phase).
				 * STOP after the last byte.
				 */

				for (int byte_idx = 0; byte_idx < msg.len; byte_idx++) {
					bool is_first_byte = (byte_idx == 0);
					bool is_last_byte = (byte_idx == msg.len - 1);

					sys_write32(msg.buf[byte_idx],
							cfg->unicomm_i2cc_base + UNICOMM_I2CC_TXDATA);

					i2cc_set_flags(
						dev,
						(is_first_byte ? I2CC_CONTROL_START_ENABLE
								   : I2CC_CONTROL_START_DISABLE) |
							(is_last_byte ? I2CC_CONTROL_STOP_ENABLE
									  : I2CC_CONTROL_STOP_DISABLE) |
							I2CC_CONTROL_ACK_ENABLE);

					i2cc_send_frame(cfg->unicomm_i2cc_base);

					i2cc_wait_not_busy(cfg->unicomm_i2cc_base);
				}

				/* Send STOP condition */
				i2cc_set_flags(dev, I2CC_CONTROL_START_DISABLE |
								I2CC_CONTROL_STOP_ENABLE |
								I2CC_CONTROL_ACK_DISABLE);

				i2cc_send_frame(cfg->unicomm_i2cc_base);
			}
		} else if ((msg.flags & (uint32_t)I2C_MSG_READ) == I2C_MSG_READ) {
			i2cc_set_target_address(cfg->unicomm_i2cc_base, addr, DIRECTION_READ);

			if (cfg->unicomm_is_advanced) {
				/* Advanced: BLEN tells the hardware how many bytes to receive in
				 * one transaction; it handles ACK/NACK/STOP automatically.
				 */
				i2cc_set_flags(dev, I2CC_CONTROL_START_ENABLE |
								I2CC_CONTROL_STOP_ENABLE |
								I2CC_CONTROL_ACK_DISABLE);

				i2cc_set_blen(cfg->unicomm_i2cc_base, msg.len);

				i2cc_send_frame(cfg->unicomm_i2cc_base);

				int bytes_received = 0;

				/* Polling Read */
				while (bytes_received < msg.len) {
					/* Wait until a byte arrives or the transfer ends */
					while (I2CC_IS_RX_FIFO_EMPTY(cfg->unicomm_i2cc_base)) {
						k_sleep(K_CYC(20));
					}

					while (!I2CC_IS_RX_FIFO_EMPTY(cfg->unicomm_i2cc_base) &&
						   bytes_received < msg.len) {
						msg.buf[bytes_received++] =
							sys_read32(cfg->unicomm_i2cc_base +
								   UNICOMM_I2CC_RXDATA) &
							BYTE_MASK;
					}
				}
			} else {
				/* Non-advanced: one byte per FRM_START trigger.
				 * START=ENABLE only on the first byte (generates START+address+R).
				 * ACK=ENABLE for all bytes except the last.
				 * Last byte: ACK=DISABLE (NACK) + STOP.
				 */
				for (int b = 0; b < msg.len; b++) {
					bool is_first_byte = (b == 0);
					bool is_last_byte = (b == msg.len - 1);

					i2cc_set_flags(
						dev,
						(is_first_byte ? I2CC_CONTROL_START_ENABLE
								   : I2CC_CONTROL_START_DISABLE) |
							(is_last_byte ? I2CC_CONTROL_STOP_ENABLE
									  : I2CC_CONTROL_STOP_DISABLE) |
							I2CC_CONTROL_ACK_ENABLE);

					i2cc_send_frame(cfg->unicomm_i2cc_base);

					/* Wait for the received byte to arrive in the RX FIFO.
					 * Polling RX FIFO empty is more reliable than polling BUSY
					 * for reads: BUSY may clear briefly between FRM_STARTs as
					 * the bus idles between bytes, causing while(!BUSY) to spin
					 * forever if the pulse is missed.
					 */
					while (I2CC_IS_RX_FIFO_EMPTY(cfg->unicomm_i2cc_base)) {
						k_sleep(K_CYC(20));
					}

					msg.buf[b] = sys_read32(cfg->unicomm_i2cc_base +
								UNICOMM_I2CC_RXDATA) &
							 0xFF;
				}
			}
		}
	}

	return 0;
}

#if defined(CONFIG_I2C_TARGET)
static inline int i2c_ti_unicomm_target_register(const struct device *dev,
						 struct i2c_target_config *target_cfg)
{
	const struct i2c_ti_unicomm_config *cfg = dev->config;
	struct i2c_ti_unicomm_data *data = dev->data;

	unicomm_reset(cfg->unicomm_inst_base);
	unicomm_enable_power(cfg->unicomm_inst_base);

	/* Set I2C Target mode */
	sys_write32(IPMODE_TARGET, cfg->unicomm_inst_base + UNICOMM_MODE);
	k_sleep(K_CYC(20));

	/* Clock config */
	sys_write32(cfg->clkdiv, cfg->unicomm_i2ct_base + UNICOMM_I2CT_CLKDIV);
	sys_write32(I2CC_CLKSEL_BUSCLK_ENABLE, cfg->unicomm_i2ct_base + UNICOMM_I2CT_CLKSEL);

	/* Set target address */
	if (target_cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) {
		UPDATE_REG(cfg->unicomm_i2ct_base + UNICOMM_I2CT_OAR, I2CT_OAR_MODE_10BIT,
			   I2CT_OAR_MODE_MASK);
		UPDATE_REG(cfg->unicomm_i2ct_base + UNICOMM_I2CT_OAR, target_cfg->address,
			   I2CT_10BIT_ADDR_MASK);
	} else {
		UPDATE_REG(cfg->unicomm_i2ct_base + UNICOMM_I2CT_OAR, I2CT_OAR_MODE_7BIT,
			   I2CT_OAR_MODE_MASK);
		UPDATE_REG(cfg->unicomm_i2ct_base + UNICOMM_I2CT_OAR, target_cfg->address,
			   I2CT_7BIT_ADDR_MASK);
	}

	/* Enable Own Address */
	UPDATE_REG(cfg->unicomm_i2ct_base + UNICOMM_I2CT_OAR, I2CT_OAR_ENABLE,
		   I2CT_OAR_ENABLE_MASK);

	/* Enabled interrupts */
	uint32_t triggers_mask = IMASK_I2CT_RXDONE | IMASK_I2CT_TXDONE | IMASK_I2CT_TXEMPTY |
				 IMASK_I2CT_TXUNFL | IMASK_I2CT_RXOVFL | IMASK_I2CT_START |
				 IMASK_I2CT_STOP;

	/* Set RX FIFO Threshold */
	if (cfg->rx_fifo_threshold != 0) {
		UPDATE_REG(cfg->unicomm_i2ct_base + UNICOMM_I2CT_IFLS, cfg->rx_fifo_threshold,
			   I2CT_RX_FIFO_LEVEL_MASK);
		triggers_mask |= IMASK_I2CT_RXTRG; /* enable RXFIFO trigger */
	}

	/* Set TX FIFO Threshold */
	if (cfg->tx_fifo_threshold != 0) {
		UPDATE_REG(cfg->unicomm_i2ct_base + UNICOMM_I2CT_IFLS, cfg->tx_fifo_threshold,
			   I2CT_TX_FIFO_LEVEL_MASK);
		triggers_mask |= IMASK_I2CT_TXTRG; /* enable TXFIFO trigger */
	}

	/* Set a default RXFIFO trigger if none configured by user */
	if (cfg->rx_fifo_threshold == 0) {
		UPDATE_REG(cfg->unicomm_i2ct_base + UNICOMM_I2CT_IFLS, I2CT_RX_FIFO_LEVEL_NOT_EMPTY,
			   I2CT_RX_FIFO_LEVEL_MASK);
		triggers_mask |= IMASK_I2CT_RXTRG; /* enable RXFIFO trigger */
	}

	/* Clock stretching would deadlock when controller and target share a single
	 * CPU with a blocking polling transfer. Target can't drain its RX FIFO
	 * while the controller is busy-waiting for SCL to be released. */
	sys_write32(sys_read32(cfg->unicomm_i2ct_base + UNICOMM_I2CT_CONTROL) |
				I2CT_CONTROL_CLKSTRETCH_MASK | I2CT_CONTROL_TXEMPTY_ON_TREQ |
				I2CT_CONTROL_TXWAIT_STALE_TXFIFO,
			cfg->unicomm_i2ct_base + UNICOMM_I2CT_CONTROL);

	/* Enable module */
	sys_write32(sys_read32(cfg->unicomm_i2ct_base + UNICOMM_I2CT_CONTROL) | I2CT_CONTROL_ENABLE,
			cfg->unicomm_i2ct_base + UNICOMM_I2CT_CONTROL);

	/* Enable I2CT Interrupts */
	sys_write32(IMASK_ALL, cfg->unicomm_i2ct_base + UNICOMM_I2CT_CPU_INT_ICLR);
	sys_write32(triggers_mask, cfg->unicomm_i2ct_base + UNICOMM_I2CT_CPU_INT_IMASK);

	cfg->irq_config_func(dev);

	data->is_target = true;
	data->target_cfg = target_cfg;

	LOG_INF("I2C Target init done for 0x%08x", cfg->unicomm_i2ct_base);

	return 0;
}

static void i2c_ti_unicomm_target_isr(const struct device *dev)
{
	const struct i2c_ti_unicomm_config *cfg = dev->config;
	struct i2c_ti_unicomm_data *data = dev->data;

	uint32_t pending_interrupt;

	int ret;

	do {
		pending_interrupt = sys_read32(cfg->unicomm_i2ct_base + UNICOMM_I2CT_CPU_INT_IIDX);

		switch (pending_interrupt) {
		case I2CT_IIDX_TARGET_START:
			data->state = UC_I2CT_STARTED;
			i2ct_clear_tx_fifo(cfg->unicomm_i2ct_base);
			break;

		case I2CT_IIDX_TARGET_TXFIFO_EMPTY: {
			uint8_t byte = 0;
			if (data->state == UC_I2CT_STARTED) {
				/* First byte of a read transaction */
				int rr_ret = 0;

				if (data->target_cfg->callbacks->read_requested != NULL) {
					rr_ret = data->target_cfg->callbacks->read_requested(
						data->target_cfg, &byte);
				}
				if (rr_ret < 0) {
					byte = 0;
					data->state = UC_I2C_ERROR;
				} else {
					data->state = UC_I2CT_TX_BUSY;
				}
			} else if (data->state == UC_I2CT_TX_BUSY) {
				/* Subsequent bytes */
				if (data->target_cfg->callbacks->read_processed != NULL) {
					data->target_cfg->callbacks->read_processed(
						data->target_cfg, &byte);
				}
			}
			sys_write32((uint32_t)byte & 0xFFU,
					cfg->unicomm_i2ct_base + UNICOMM_I2CT_TXDATA);
			break;
		}

		case I2CT_IIDX_TARGET_TX_DONE:
			break;

		case I2CT_IIDX_TARGET_RX_DONE:
			if (data->state == UC_I2CT_STARTED) {
				data->state = UC_I2CT_RX_BUSY;
				if (data->target_cfg->callbacks->write_requested != NULL) {
					ret = data->target_cfg->callbacks->write_requested(
						data->target_cfg);
					if (ret < 0) {
						i2ct_send_nack(cfg->unicomm_i2ct_base);
					}
				}
			}
			break;

		case I2CT_IIDX_TARGET_RXFIFO_TRIGGER:
			if (data->state == UC_I2CT_RX_BUSY) {
				if (data->target_cfg->callbacks->write_received != NULL) {
					while (!I2CT_IS_RX_FIFO_EMPTY(cfg->unicomm_i2ct_base)) {
						ret = data->target_cfg->callbacks->write_received(
							data->target_cfg,
							sys_read32(cfg->unicomm_i2ct_base +
								   UNICOMM_I2CT_RXDATA));
						if (ret < 0) {
							i2ct_send_nack(cfg->unicomm_i2ct_base);
							break;
						}
					}
				}
			}
			break;

		case I2CT_IIDX_TARGET_STOP:
			sys_write32(I2CT_ACKCTL_ACKOEN_DISABLE,
					cfg->unicomm_i2ct_base + UNICOMM_I2CT_ACKCTL);
			if (data->target_cfg->callbacks->stop != NULL) {
				data->target_cfg->callbacks->stop(data->target_cfg);
			}
			break;

		case I2CT_IIDX_TARGET_TXFIFO_UNDERFLOW:
			break;

		default:
			if (pending_interrupt != 0) {
				LOG_DBG("TARGET ISR: pending interrupts = 0x%08x\n",
					pending_interrupt);
			}
			break;
		}
	} while (pending_interrupt != I2CT_IIDX_NO_INTR);
}
#endif /* CONFIG_I2C_TARGET */

static void i2c_ti_unicomm_isr(const struct device *dev)
{
	struct i2c_ti_unicomm_data *data = dev->data;

#if defined(CONFIG_I2C_TARGET)
	if (data->is_target == true) {
		i2c_ti_unicomm_target_isr(dev);
		return;
	}
#endif /* CONFIG_I2C_TARGET */
}

static DEVICE_API(i2c, i2c_ti_unicomm_api) = {.configure = i2c_ti_unicomm_configure,
						  .transfer = i2c_ti_unicomm_transfer,
#if defined(CONFIG_I2C_TARGET)
						  .target_register = i2c_ti_unicomm_target_register
#endif
};

#define I2C_TI_UNICOMM_IRQ_FUNC(index)                                                             \
	static void i2c_ti_unicomm_irq_config_func_##index(const struct device *dev)               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), i2c_ti_unicomm_isr, \
				DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}

#define I2C_TI_UNICOMM_INIT(index)                                                                 \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
																								   \
	I2C_TI_UNICOMM_IRQ_FUNC(index);                                                            \
																								   \
	static const struct i2c_ti_unicomm_config i2c_config_##index = {                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.unicomm_inst_base =                                                               \
			(uint32_t)(DT_REG_ADDR_BY_IDX(DT_PARENT(DT_DRV_INST(index)), 0)),          \
		.unicomm_i2cc_base = (uint32_t)(DT_INST_REG_ADDR(index)),                          \
		.unicomm_i2ct_base = (uint32_t)(DT_INST_REG_ADDR(index)),                          \
		.unicomm_is_advanced = DT_INST_PROP(index, unicomm_advanced_i2c),                  \
		.clkdiv = CLKDIV_DIVIDE_BY_1,                                                      \
		.busclk_hz = DT_INST_PROP_OR(index, unicomm_clock_freq, 100000000U),               \
		.timer_period = 99,                                                                \
		.irq_config_func = i2c_ti_unicomm_irq_config_func_##index,                         \
		.rx_fifo_threshold = DT_INST_PROP_OR(index, rxfifo_threshold, 0),                  \
		.tx_fifo_threshold = DT_INST_PROP_OR(index, txfifo_threshold, 0),                  \
	};                                                                                         \
																								   \
	static struct i2c_ti_unicomm_data i2c_data_##index;                                        \
																								   \
	I2C_DEVICE_DT_INST_DEFINE(index, i2c_ti_unicomm_init, NULL, &i2c_data_##index,             \
				  &i2c_config_##index, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,      \
				  &i2c_ti_unicomm_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_TI_UNICOMM_INIT)
