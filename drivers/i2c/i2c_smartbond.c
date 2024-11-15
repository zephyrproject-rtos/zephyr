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
#include <da1469x_pd.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device_runtime.h>

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
#ifdef CONFIG_I2C_CALLBACK
	k_spinlock_key_t spinlock_key;
#endif
};

#if defined(CONFIG_PM_DEVICE)
static inline void i2c_smartbond_pm_prevent_system_sleep(void)
{
	/*
	 * Prevent the SoC from etering the normal sleep state as PDC does not support
	 * waking up the application core following I2C events.
	 */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}

static inline void i2c_smartbond_pm_allow_system_sleep(void)
{
	/*
	 * Allow the SoC to enter the nornmal sleep state once I2C transactions are done.
	 */
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}
#endif

static inline void i2c_smartbond_pm_policy_state_lock_get(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE
#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_runtime_get(dev);
#else
	ARG_UNUSED(dev);
	i2c_smartbond_pm_prevent_system_sleep();
#endif
#endif
}

static inline void i2c_smartbond_pm_policy_state_lock_put(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE
#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_runtime_put(dev);
#else
	ARG_UNUSED(dev);
	i2c_smartbond_pm_allow_system_sleep();
#endif
#endif
}

static inline bool i2c_smartbond_is_idle(const struct device *dev)
{
	const struct i2c_smartbond_cfg *config = dev->config;
	uint32_t mask = I2C_I2C_STATUS_REG_I2C_ACTIVITY_Msk |
					I2C_I2C_STATUS_REG_RFNE_Msk |
					I2C_I2C_STATUS_REG_TFE_Msk;

	return ((config->regs->I2C_STATUS_REG & mask) == I2C_I2C_STATUS_REG_TFE_Msk);
}

static void i2c_smartbond_disable_when_inactive(const struct device *dev)
{
	const struct i2c_smartbond_cfg *config = dev->config;

	if ((config->regs->I2C_ENABLE_REG & I2C_I2C_ENABLE_REG_I2C_EN_Msk)) {
		while (!i2c_smartbond_is_idle(dev)) {
		};
		config->regs->I2C_ENABLE_REG &= ~I2C_I2C_ENABLE_REG_I2C_EN_Msk;
	}
}

static int i2c_smartbond_apply_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_smartbond_cfg *config = dev->config;
	struct i2c_smartbond_data *data = dev->data;
	uint32_t con_reg = 0x0UL;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

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

	i2c_smartbond_disable_when_inactive(dev);

	/* Write control register*/
	config->regs->I2C_CON_REG = con_reg;

	/* Reset interrupt mask */
	config->regs->I2C_INTR_MASK_REG = 0x0000U;

	config->regs->I2C_ENABLE_REG |= I2C_I2C_ENABLE_REG_I2C_EN_Msk;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int i2c_smartbond_configure(const struct device *dev, uint32_t dev_config)
{
	int ret = 0;

	pm_device_runtime_get(dev);
	ret = i2c_smartbond_apply_configure(dev, dev_config);
	pm_device_runtime_put(dev);

	return ret;
}

static int i2c_smartbond_get_config(const struct device *dev, uint32_t *dev_config)
{
	const struct i2c_smartbond_cfg *config = dev->config;
	struct i2c_smartbond_data *data = data = dev->data;
	uint32_t reg;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	pm_device_runtime_get(dev);
	/* Read the value of the control register */
	reg = config->regs->I2C_CON_REG;
	pm_device_runtime_put(dev);

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
	const bool rw = ((data->msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ);
	int ret = 0;

	if (!data->msgs->buf || data->msgs->len == 0) {
		return -EINVAL;
	}

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

	if (config->regs->I2C_TX_ABRT_SOURCE_REG & 0x1FFFF) {
		ret = -EIO;
	}

	if (ret) {
		(void)config->regs->I2C_CLR_TX_ABRT_REG;
	}

	return ret;
}

static inline int i2c_smartbond_rx(const struct i2c_smartbond_cfg *const config,
				   struct i2c_smartbond_data *data)
{
	int ret = 0;

	if (!data->msgs->buf || data->msgs->len == 0) {
		return -EINVAL;
	}

	/* Reads the data register until fifo is empty */
	while ((data->receive_cnt < data->transmit_cnt) &&
	       (config->regs->I2C_STATUS_REG & I2C2_I2C2_STATUS_REG_RFNE_Msk)) {
		data->msgs->buf[data->receive_cnt] =
			config->regs->I2C_DATA_CMD_REG & I2C_I2C_DATA_CMD_REG_I2C_DAT_Msk;
		data->receive_cnt++;
	}

	return ret;
}

static int i2c_smartbond_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				  uint16_t addr)
{
	const struct i2c_smartbond_cfg *config = dev->config;
	struct i2c_smartbond_data *data = dev->data;
	int ret = 0;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	i2c_smartbond_pm_policy_state_lock_get(dev);

	ret = i2c_smartbond_prep_transfer(dev, msgs, num_msgs, addr);
	if (ret != 0) {
		goto finish;
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
	while (!i2c_smartbond_is_idle(dev)) {
	};
	i2c_smartbond_pm_policy_state_lock_put(dev);
	k_spin_unlock(&data->lock, key);

	return ret;
}

#ifdef CONFIG_I2C_CALLBACK

#define TX_FIFO_DEPTH 32

static int i2c_smartbond_enable_msg_interrupts(const struct i2c_smartbond_cfg *const config,
					       struct i2c_smartbond_data *data)
{
	if ((data->msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
		uint32_t remaining = data->msgs->len - data->receive_cnt;
		uint32_t tx_space = TX_FIFO_DEPTH - config->regs->I2C_TXFLR_REG;
		uint32_t rx_tl = ((remaining < tx_space) ? remaining : tx_space) - 1;

		config->regs->I2C_RX_TL_REG = rx_tl & I2C_I2C_RX_TL_REG_RX_TL_Msk;
		config->regs->I2C_INTR_MASK_REG |= I2C_I2C_INTR_MASK_REG_M_RX_FULL_Msk;
	} else {
		config->regs->I2C_INTR_MASK_REG &= ~I2C_I2C_INTR_MASK_REG_M_RX_FULL_Msk;
	}

	config->regs->I2C_TX_TL_REG = 0UL;
	config->regs->I2C_INTR_MASK_REG |= I2C_I2C_INTR_MASK_REG_M_TX_EMPTY_Msk;

	return 0;
}

static int i2c_smartbond_transfer_cb(const struct device *dev, struct i2c_msg *msgs,
				     uint8_t num_msgs, uint16_t addr, i2c_callback_t cb,
				     void *userdata)
{
	const struct i2c_smartbond_cfg *config = dev->config;
	struct i2c_smartbond_data *data = dev->data;
	int ret = 0;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (cb == NULL) {
		return -EINVAL;
	}

	if (data->cb != NULL) {
		return -EWOULDBLOCK;
	}

	data->spinlock_key = key;
	data->cb = cb;
	data->userdata = userdata;

	i2c_smartbond_pm_policy_state_lock_get(dev);

	ret = i2c_smartbond_prep_transfer(dev, msgs, num_msgs, addr);
	if (ret != 0) {
		i2c_smartbond_pm_policy_state_lock_put(dev);
		k_spin_unlock(&data->lock, key);
		return ret;
	}

	i2c_smartbond_enable_msg_interrupts(config, data);

	LOG_INF("async transfer started");

	return 0;
}

static inline void isr_tx(const struct i2c_smartbond_cfg *config, struct i2c_smartbond_data *data)
{
	const bool rw = ((data->msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ);

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
}

static inline void isr_rx(const struct i2c_smartbond_cfg *config, struct i2c_smartbond_data *data)
{
	while ((data->receive_cnt < data->transmit_cnt) &&
	       (config->regs->I2C_STATUS_REG & I2C2_I2C2_STATUS_REG_RFNE_Msk)) {
		data->msgs->buf[data->receive_cnt] =
			config->regs->I2C_DATA_CMD_REG & I2C_I2C_DATA_CMD_REG_I2C_DAT_Msk;
		data->receive_cnt++;
	}
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
		while (!i2c_smartbond_is_idle(dev)) {
		};
		i2c_smartbond_pm_policy_state_lock_put(dev);
		k_spin_unlock(&data->lock, data->spinlock_key);
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
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

static int i2c_smartbond_resume(const struct device *dev)
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

	return i2c_smartbond_apply_configure(dev,
				       I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate));
}

#if defined(CONFIG_PM_DEVICE)
static int i2c_smartbond_suspend(const struct device *dev)
{
	int ret;
	const struct i2c_smartbond_cfg *config = dev->config;

	/* Disable the I2C digital block */
	config->regs->I2C_ENABLE_REG &= ~I2C_I2C_ENABLE_REG_I2C_EN_Msk;
	/* Gate I2C clocking */
	CRG_COM->RESET_CLK_COM_REG = config->periph_clock_config;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
	if (ret < 0) {
		LOG_WRN("Fialid to configure the I2C pins to inactive state");
	}

	return ret;
}

static int i2c_smartbond_pm_action(const struct device *dev,
	enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
#ifdef CONFIG_PM_DEVICE_RUNTIME
		i2c_smartbond_pm_prevent_system_sleep();
#endif
		/*
		 * Although the GPIO driver should already be initialized, make sure PD_COM
		 * is up and running before accessing the I2C block.
		 */
		da1469x_pd_acquire(MCU_PD_DOMAIN_COM);
		ret = i2c_smartbond_resume(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = i2c_smartbond_suspend(dev);
		/*
		 * Once the I2C block is turned off its power domain can
		 * be released, as well.
		 */
		da1469x_pd_release(MCU_PD_DOMAIN_COM);
#ifdef CONFIG_PM_DEVICE_RUNTIME
		i2c_smartbond_pm_allow_system_sleep();
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif


static int i2c_smartbond_init(const struct device *dev)
{
	int ret;

#ifdef CONFIG_PM_DEVICE_RUNTIME
	/* Make sure device state is marked as suspended */
	pm_device_init_suspended(dev);

	ret = pm_device_runtime_enable(dev);
#else
	da1469x_pd_acquire(MCU_PD_DOMAIN_COM);
	ret = i2c_smartbond_resume(dev);
#endif

	return ret;
}

#define I2C_SMARTBOND_DEVICE(id)                                                                   \
	PM_DEVICE_DT_INST_DEFINE(id, i2c_smartbond_pm_action);	\
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
	I2C_DEVICE_DT_INST_DEFINE(id, i2c_smartbond_##id##_init, PM_DEVICE_DT_INST_GET(id), \
				&i2c_smartbond_##id##_data, \
				&i2c_smartbond_##id##_cfg, POST_KERNEL,                          \
				CONFIG_I2C_INIT_PRIORITY, &i2c_smartbond_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_SMARTBOND_DEVICE)
