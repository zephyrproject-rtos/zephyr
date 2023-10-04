/*
 * Copyright (c) 2022 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_smartbond_i2c

#include <errno.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <DA1469xAB.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_smartbond);

#include "i2c-priv.h"

struct i2c_smartbond_cfg {
	I2C_Type *regs;
	int periph_clock_config;
	const struct pinctrl_dev_config *pcfg;
	uint32_t bitrate;
};

struct i2c_smartbond_data {
	struct k_spinlock lock;
	struct i2c_msg *msgs;
	uint8_t num_msgs;
	uint32_t transmit_cnt, receive_cnt;
	i2c_callback_t cb;
	void *userdata;
};

static int i2c_smartbond_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_smartbond_cfg *config = dev->config;
	struct i2c_smartbond_data *data = dev->data;
	uint32_t con_reg = 0x0UL;
	k_spinlock_key_t key;

	/* Configure Speed (SCL frequency) */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		con_reg |= 1UL << I2C_I2C_CON_REG_I2C_SPEED_Pos;
		break;
	case I2C_SPEED_FAST:
		con_reg |= 2UL << I2C_I2C_CON_REG_I2C_SPEED_Pos;
		break;
	/* TODO: Currently 1 MHz, add switching to 96 MHz PLL sys_clk to support 3.4 Mbit/s */
	/*
	 * case I2C_SPEED_HIGH:
	 *	con_reg |= 3UL << I2C_I2C_CON_REG_I2C_SPEED_Pos;
	 *	break;
	 */
	default:
		LOG_ERR("Speed not supported");
		return -ENOTSUP;
	}

	/* Configure Mode */
	if ((dev_config & I2C_MODE_CONTROLLER) == I2C_MODE_CONTROLLER) {
		con_reg |=
			I2C_I2C_CON_REG_I2C_MASTER_MODE_Msk | I2C_I2C_CON_REG_I2C_SLAVE_DISABLE_Msk;
	} else {
		LOG_ERR("Only I2C Controller mode supported");
		return -ENOTSUP;
	}

	/* Enable sending RESTART as master */
	con_reg |= I2C_I2C_CON_REG_I2C_RESTART_EN_Msk;

	key = k_spin_lock(&data->lock);

	if (!!(config->regs->I2C_ENABLE_REG & I2C_I2C_ENABLE_REG_I2C_EN_Msk)) {
		while (!!(config->regs->I2C_STATUS_REG & I2C_I2C_STATUS_REG_I2C_ACTIVITY_Msk)) {
		};
		config->regs->I2C_ENABLE_REG &= ~I2C_I2C_ENABLE_REG_I2C_EN_Msk;
	}

	/* Write control register*/
	config->regs->I2C_CON_REG = con_reg;

	/* Reset interrupt mask */
	config->regs->I2C_INTR_MASK_REG = 0x0000U;

	config->regs->I2C_ENABLE_REG |= I2C_I2C_ENABLE_REG_I2C_EN_Msk;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int i2c_smartbond_get_config(const struct device *dev, uint32_t *dev_config)
{
	const struct i2c_smartbond_cfg *config = dev->config;
	struct i2c_smartbond_data *data = data = dev->data;
	uint32_t reg;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* Read the value of the control register */
	reg = config->regs->I2C_CON_REG;

	k_spin_unlock(&data->lock, key);

	*dev_config = 0UL;

	/* Check if I2C is in controller or target mode */
	if ((reg & I2C_I2C_CON_REG_I2C_MASTER_MODE_Msk) &&
	    (reg & I2C_I2C_CON_REG_I2C_SLAVE_DISABLE_Msk)) {
		*dev_config |= I2C_MODE_CONTROLLER;
	} else if (!(reg & I2C_I2C_CON_REG_I2C_MASTER_MODE_Msk) &&
		   !(reg & I2C_I2C_CON_REG_I2C_SLAVE_DISABLE_Msk)) {
		*dev_config &= ~I2C_MODE_CONTROLLER;
	} else {
		return -EIO;
	}

	/* Get the operating speed */
	switch ((reg & I2C_I2C_CON_REG_I2C_SPEED_Msk) >> I2C_I2C_CON_REG_I2C_SPEED_Pos) {
	case 1UL:
		*dev_config |= I2C_SPEED_SET(I2C_SPEED_STANDARD);
		break;
	case 2UL:
		*dev_config |= I2C_SPEED_SET(I2C_SPEED_FAST);
		break;
	case 3UL:
		*dev_config |= I2C_SPEED_SET(I2C_SPEED_HIGH);
		break;
	default:
		return -ERANGE;
	}

	return 0;
}

static inline void i2c_smartbond_set_target_address(const struct i2c_smartbond_cfg *const config,
						    struct i2c_smartbond_data *data,
						    const struct i2c_msg *const msg, uint16_t addr)
{
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* Disable I2C Controller */
	config->regs->I2C_ENABLE_REG &= ~I2C_I2C_ENABLE_REG_I2C_EN_Msk;
	/* Configure addressing mode*/
	if (msg->flags & I2C_MSG_ADDR_10_BITS) {
		config->regs->I2C_CON_REG |= I2C_I2C_CON_REG_I2C_10BITADDR_MASTER_Msk;
	} else {
		config->regs->I2C_CON_REG &= ~(I2C_I2C_CON_REG_I2C_10BITADDR_MASTER_Msk);
	}
	/* Change the Target Address */
	config->regs->I2C_TAR_REG = ((config->regs->I2C_TAR_REG & ~I2C_I2C_TAR_REG_IC_TAR_Msk) |
				     (addr & I2C_I2C_TAR_REG_IC_TAR_Msk));
	/* Enable again the I2C to use the new address */
	config->regs->I2C_ENABLE_REG |= I2C_I2C_ENABLE_REG_I2C_EN_Msk;

	k_spin_unlock(&data->lock, key);
}

static inline int i2c_smartbond_set_msg_flags(struct i2c_msg *msgs, uint8_t num_msgs)
{
	struct i2c_msg *current, *next;

	current = msgs;
	for (uint8_t i = 1; i <= num_msgs; i++) {
		if (i < num_msgs) {
			next = current + 1;
			if ((current->flags & I2C_MSG_RW_MASK) != (next->flags & I2C_MSG_RW_MASK)) {
				next->flags |= I2C_MSG_RESTART;
			}
			if (current->flags & I2C_MSG_STOP) {
				return -EINVAL;
			}
		} else {
			current->flags |= I2C_MSG_STOP;
		}
		current++;
	}

	return 0;
}

static inline int i2c_smartbond_prep_transfer(const struct device *dev, struct i2c_msg *msgs,
					      uint8_t num_msgs, uint16_t addr)
{
	const struct i2c_smartbond_cfg *config = dev->config;
	struct i2c_smartbond_data *data = dev->data;
	int ret = 0;

	ret = i2c_smartbond_set_msg_flags(msgs, num_msgs);
	if (ret != 0) {
		return ret;
	}

	i2c_smartbond_set_target_address(config, data, msgs, addr);

	data->msgs = msgs;
	data->num_msgs = num_msgs;
	data->transmit_cnt = 0;
	data->receive_cnt = 0;

	return 0;
}

static inline int i2c_smartbond_tx(const struct i2c_smartbond_cfg *const config,
				   struct i2c_smartbond_data *data)
{
	k_spinlock_key_t key;
	const bool rw = ((data->msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ);
	int ret = 0;

	if (!data->msgs->buf || data->msgs->len == 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	/* Transmits data or read commands with correct flags */
	while ((data->transmit_cnt < data->msgs->len) &&
	       (config->regs->I2C_STATUS_REG & I2C_I2C_STATUS_REG_TFNF_Msk)) {
		config->regs->I2C_DATA_CMD_REG =
			(rw ? I2C_I2C_DATA_CMD_REG_I2C_CMD_Msk
			    : (data->msgs->buf[data->transmit_cnt] &
			       I2C_I2C_DATA_CMD_REG_I2C_DAT_Msk)) |
			((data->transmit_cnt == 0) && (data->msgs->flags & I2C_MSG_RESTART)
				 ? I2C_I2C_DATA_CMD_REG_I2C_RESTART_Msk
				 : 0) |
			((data->transmit_cnt == (data->msgs->len - 1)) &&
					 (data->msgs->flags & I2C_MSG_STOP)
				 ? I2C_I2C_DATA_CMD_REG_I2C_STOP_Msk
				 : 0);
		data->transmit_cnt++;

		/* Return IO error if any of the abort flags are set */
		if (config->regs->I2C_TX_ABRT_SOURCE_REG & 0x1FFFF) {
			ret = -EIO;
		}
	}

	while (!(config->regs->I2C_STATUS_REG & I2C_I2C_STATUS_REG_TFE_Msk)) {
	};

	if (config->regs->I2C_TX_ABRT_SOURCE_REG & 0x1FFFF) {
		ret = -EIO;
	}

	if (ret) {
		(void)config->regs->I2C_CLR_TX_ABRT_REG;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static inline int i2c_smartbond_rx(const struct i2c_smartbond_cfg *const config,
				   struct i2c_smartbond_data *data)
{
	k_spinlock_key_t key;
	int ret = 0;

	if (!data->msgs->buf || data->msgs->len == 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	/* Reads the data register until fifo is empty */
	while ((data->receive_cnt < data->transmit_cnt) &&
	       (config->regs->I2C_STATUS_REG & I2C2_I2C2_STATUS_REG_RFNE_Msk)) {
		data->msgs->buf[data->receive_cnt] =
			config->regs->I2C_DATA_CMD_REG & I2C_I2C_DATA_CMD_REG_I2C_DAT_Msk;
		data->receive_cnt++;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int i2c_smartbond_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				  uint16_t addr)
{
	const struct i2c_smartbond_cfg *config = dev->config;
	struct i2c_smartbond_data *data = dev->data;
	int ret = 0;

	while (!!(config->regs->I2C_STATUS_REG & I2C_I2C_STATUS_REG_I2C_ACTIVITY_Msk)) {
	};

	ret = i2c_smartbond_prep_transfer(dev, msgs, num_msgs, addr);
	if (ret != 0) {
		return ret;
	}

	for (; data->num_msgs > 0; data->num_msgs--, data->msgs++) {
		data->transmit_cnt = 0;
		data->receive_cnt = 0;
		if ((data->msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			/* Repeating transmit and receives until all data has been read */
			while (data->receive_cnt < data->msgs->len) {
				/* Transmit read commands */
				ret = i2c_smartbond_tx(config, data);
				if (ret < 0) {
					goto finish;
				}
				/* Read received data */
				ret = i2c_smartbond_rx(config, data);
				if (ret < 0) {
					goto finish;
				}
			}
		} else {
			while (data->transmit_cnt < data->msgs->len) {
				/* Transmit data */
				ret = i2c_smartbond_tx(config, data);
				if (ret < 0) {
					goto finish;
				}
			}
		}
	}

finish:

	return ret;
}

#ifdef CONFIG_I2C_CALLBACK

static int i2c_smartbond_enable_msg_interrupts(const struct i2c_smartbond_cfg *const config,
					       struct i2c_smartbond_data *data)
{
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if ((data->msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
		uint32_t remaining = data->msgs->len - data->receive_cnt;
		uint32_t tx_space = 32 - config->regs->I2C_TXFLR_REG;
		uint32_t rx_tl = ((remaining < tx_space) ? remaining : tx_space) - 1;

		config->regs->I2C_RX_TL_REG = rx_tl & I2C_I2C_RX_TL_REG_RX_TL_Msk;
		config->regs->I2C_INTR_MASK_REG |= I2C_I2C_INTR_MASK_REG_M_RX_FULL_Msk;
	} else {
		config->regs->I2C_INTR_MASK_REG &= ~I2C_I2C_INTR_MASK_REG_M_RX_FULL_Msk;
	}

	config->regs->I2C_TX_TL_REG = 0UL;
	config->regs->I2C_INTR_MASK_REG |= I2C_I2C_INTR_MASK_REG_M_TX_EMPTY_Msk;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int i2c_smartbond_transfer_cb(const struct device *dev, struct i2c_msg *msgs,
				     uint8_t num_msgs, uint16_t addr, i2c_callback_t cb,
				     void *userdata)
{
	const struct i2c_smartbond_cfg *config = dev->config;
	struct i2c_smartbond_data *data = dev->data;
	int ret = 0;

	if (cb == NULL) {
		return -EINVAL;
	}

	if (data->cb != NULL) {
		return -EWOULDBLOCK;
	}

	data->cb = cb;
	data->userdata = userdata;

	ret = i2c_smartbond_prep_transfer(dev, msgs, num_msgs, addr);
	if (ret != 0) {
		return ret;
	}

	i2c_smartbond_enable_msg_interrupts(config, data);

	LOG_INF("async transfer started");

	return 0;
}

static inline void isr_tx(const struct i2c_smartbond_cfg *config, struct i2c_smartbond_data *data)
{

	const bool rw = ((data->msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	while ((data->transmit_cnt < data->msgs->len) &&
	       (config->regs->I2C_STATUS_REG & I2C_I2C_STATUS_REG_TFNF_Msk)) {
		config->regs->I2C_DATA_CMD_REG =
			(rw ? I2C_I2C_DATA_CMD_REG_I2C_CMD_Msk
			    : (data->msgs->buf[data->transmit_cnt] &
			       I2C_I2C_DATA_CMD_REG_I2C_DAT_Msk)) |
			((data->transmit_cnt == 0) && (data->msgs->flags & I2C_MSG_RESTART)
				 ? I2C_I2C_DATA_CMD_REG_I2C_RESTART_Msk
				 : 0) |
			((data->transmit_cnt == (data->msgs->len - 1)) &&
					 (data->msgs->flags & I2C_MSG_STOP)
				 ? I2C_I2C_DATA_CMD_REG_I2C_STOP_Msk
				 : 0);
		data->transmit_cnt++;
	}

	k_spin_unlock(&data->lock, key);
}

static inline void isr_rx(const struct i2c_smartbond_cfg *config, struct i2c_smartbond_data *data)
{
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	while ((data->receive_cnt < data->transmit_cnt) &&
	       (config->regs->I2C_STATUS_REG & I2C2_I2C2_STATUS_REG_RFNE_Msk)) {
		data->msgs->buf[data->receive_cnt] =
			config->regs->I2C_DATA_CMD_REG & I2C_I2C_DATA_CMD_REG_I2C_DAT_Msk;
		data->receive_cnt++;
	}

	k_spin_unlock(&data->lock, key);
}

static inline void i2c_smartbond_async_msg_done(const struct device *dev)
{
	const struct i2c_smartbond_cfg *config = dev->config;
	struct i2c_smartbond_data *data = dev->data;

	data->num_msgs--;
	if (data->num_msgs > 0) {
		data->msgs++;
		data->transmit_cnt = 0;
		data->receive_cnt = 0;
		i2c_smartbond_enable_msg_interrupts(config, data);
	} else {
		i2c_callback_t cb = data->cb;

		data->msgs = NULL;
		data->cb = NULL;
		LOG_INF("async transfer finished");
		cb(dev, 0, data->userdata);
	}
}

static void i2c_smartbond_isr(const struct device *dev)
{
	const struct i2c_smartbond_cfg *config = dev->config;
	struct i2c_smartbond_data *data = dev->data;
	uint32_t flags = config->regs->I2C_INTR_STAT_REG;

	if (flags & I2C_I2C_INTR_STAT_REG_R_TX_EMPTY_Msk) {
		isr_tx(config, data);
		if (data->transmit_cnt == data->msgs->len) {
			config->regs->I2C_INTR_MASK_REG &= ~I2C_I2C_INTR_MASK_REG_M_TX_EMPTY_Msk;
			if ((data->msgs->flags & I2C_MSG_RW_MASK) != I2C_MSG_READ) {
				i2c_smartbond_async_msg_done(dev);
			}
		}
	}

	if (flags & I2C_I2C_INTR_STAT_REG_R_RX_FULL_Msk) {
		isr_rx(config, data);
		if (data->receive_cnt == data->msgs->len) {
			config->regs->I2C_INTR_MASK_REG &= ~I2C_I2C_INTR_MASK_REG_M_RX_FULL_Msk;
			i2c_smartbond_async_msg_done(dev);
		} else {
			uint32_t remaining = data->msgs->len - data->receive_cnt;
			uint32_t tx_space = 32 - config->regs->I2C_TXFLR_REG;
			uint32_t rx_tl = ((remaining < tx_space) ? remaining : tx_space) - 1;

			config->regs->I2C_RX_TL_REG = rx_tl & I2C_I2C_RX_TL_REG_RX_TL_Msk;
			config->regs->I2C_INTR_MASK_REG |= I2C_I2C_INTR_MASK_REG_M_RX_FULL_Msk;
		}
	}
}

#define I2C_SMARTBOND_CONFIGURE(id)                                                                \
	IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), i2c_smartbond_isr,                \
		    DEVICE_DT_INST_GET(id), 0);                                                    \
	irq_enable(DT_INST_IRQN(id));
#else
#define I2C_SMARTBOND_CONFIGURE(id)
#endif

static const struct i2c_driver_api i2c_smartbond_driver_api = {
	.configure = i2c_smartbond_configure,
	.get_config = i2c_smartbond_get_config,
	.transfer = i2c_smartbond_transfer,
#ifdef CONFIG_I2C_CALLBACK
	.transfer_cb = i2c_smartbond_transfer_cb,
#endif
};

static int i2c_smartbond_init(const struct device *dev)
{
	const struct i2c_smartbond_cfg *config = dev->config;
	int err;

	config->regs->I2C_ENABLE_REG &= ~I2C_I2C_ENABLE_REG_I2C_EN_Msk;

	/* Reset I2C CLK_SEL */
	CRG_COM->RESET_CLK_COM_REG = (config->periph_clock_config << 1);
	/* Set I2C CLK ENABLE */
	CRG_COM->SET_CLK_COM_REG = config->periph_clock_config;

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("Failed to configure I2C pins");
		return err;
	}

	return i2c_smartbond_configure(dev,
				       I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate));
}

#define I2C_SMARTBOND_DEVICE(id)                                                                   \
	PINCTRL_DT_INST_DEFINE(id);                                                                \
	static const struct i2c_smartbond_cfg i2c_smartbond_##id##_cfg = {                         \
		.regs = (I2C_Type *)DT_INST_REG_ADDR(id),                                          \
		.periph_clock_config = DT_INST_PROP(id, periph_clock_config),                      \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),                                        \
		.bitrate = DT_INST_PROP_OR(id, clock_frequency, 100000)};                          \
	static struct i2c_smartbond_data i2c_smartbond_##id##_data = {.msgs = NULL, .cb = NULL};   \
	static int i2c_smartbond_##id##_init(const struct device *dev)                             \
	{                                                                                          \
		int ret = i2c_smartbond_init(dev);                                                 \
		I2C_SMARTBOND_CONFIGURE(id);                                                       \
		return ret;                                                                        \
	}                                                                                          \
	I2C_DEVICE_DT_INST_DEFINE(id, i2c_smartbond_##id##_init, NULL, &i2c_smartbond_##id##_data, \
				  &i2c_smartbond_##id##_cfg, POST_KERNEL,                          \
				  CONFIG_I2C_INIT_PRIORITY, &i2c_smartbond_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_SMARTBOND_DEVICE)
