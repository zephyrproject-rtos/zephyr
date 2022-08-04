/*
 * Copyright (c) 2021 BrainCo Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_i2c

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/i2c.h>

#include <gd32_i2c.h>
#include <gd32_rcu.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_gd32, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

/* Bus error */
#define I2C_GD32_ERR_BERR BIT(0)
/* Arbitration lost */
#define I2C_GD32_ERR_LARB BIT(1)
/* No ACK received */
#define I2C_GD32_ERR_AERR BIT(2)
/* I2C bus busy */
#define I2C_GD32_ERR_BUSY BIT(4)

struct i2c_gd32_config {
	uint32_t reg;
	uint32_t bitrate;
	uint32_t rcu_periph_clock;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_cfg_func)(void);
};

struct i2c_gd32_data {
	struct k_sem bus_mutex;
	struct k_sem sync_sem;
	uint32_t dev_config;
	uint16_t addr1;
	uint16_t addr2;
	uint32_t xfer_len;
	struct i2c_msg *current;
	uint8_t errs;
	bool is_restart;
};

static inline void i2c_gd32_enable_interrupts(const struct i2c_gd32_config *cfg)
{
	I2C_CTL1(cfg->reg) |= I2C_CTL1_ERRIE;
	I2C_CTL1(cfg->reg) |= I2C_CTL1_EVIE;
	I2C_CTL1(cfg->reg) |= I2C_CTL1_BUFIE;
}

static inline void i2c_gd32_disable_interrupts(const struct i2c_gd32_config *cfg)
{
	I2C_CTL1(cfg->reg) &= ~I2C_CTL1_ERRIE;
	I2C_CTL1(cfg->reg) &= ~I2C_CTL1_EVIE;
	I2C_CTL1(cfg->reg) &= ~I2C_CTL1_BUFIE;
}

static inline void i2c_gd32_xfer_read(struct i2c_gd32_data *data,
				      const struct i2c_gd32_config *cfg)
{
	data->current->len--;
	*data->current->buf = I2C_DATA(cfg->reg);
	data->current->buf++;

	if ((data->xfer_len > 0U) &&
	    (data->current->len == 0U)) {
		data->current++;
	}
}

static inline void i2c_gd32_xfer_write(struct i2c_gd32_data *data,
				       const struct i2c_gd32_config *cfg)
{
	data->current->len--;
	I2C_DATA(cfg->reg) = *data->current->buf;
	data->current->buf++;

	if ((data->xfer_len > 0U) &&
	    (data->current->len == 0U)) {
		data->current++;
	}
}

static void i2c_gd32_handle_rbne(const struct device *dev)
{
	struct i2c_gd32_data *data = dev->data;
	const struct i2c_gd32_config *cfg = dev->config;

	switch (data->xfer_len) {
	case 0:
		/* Unwanted data received, ignore it. */
		k_sem_give(&data->sync_sem);
		break;
	case 1:
		/* If total_read_length == 1, read the data directly. */
		data->xfer_len--;
		i2c_gd32_xfer_read(data, cfg);

		k_sem_give(&data->sync_sem);

		break;
	case 2:
		__fallthrough;
	case 3:
		/*
		 * If total_read_length == 2, or total_read_length > 3
		 * and remaining_read_length == 3, disable the RBNE
		 * interrupt.
		 * Remaining data will be read from BTC interrupt.
		 */
		I2C_CTL1(cfg->reg) &= ~I2C_CTL1_BUFIE;
		break;
	default:
		/*
		 * If total_read_length > 3 and remaining_read_length > 3,
		 * read the data directly.
		 */
		data->xfer_len--;
		i2c_gd32_xfer_read(data, cfg);
		break;
	}

}

static void i2c_gd32_handle_tbe(const struct device *dev)
{
	struct i2c_gd32_data *data = dev->data;
	const struct i2c_gd32_config *cfg = dev->config;

	if (data->xfer_len > 0U) {
		data->xfer_len--;
		if (data->xfer_len == 0U) {
			/*
			 * This is the last data to transmit, disable the TBE interrupt.
			 * Use the BTC interrupt to indicate the write data complete state.
			 */
			I2C_CTL1(cfg->reg) &= ~I2C_CTL1_BUFIE;
		}
		i2c_gd32_xfer_write(data, cfg);

	} else {
		/* Enter stop condition */
		I2C_CTL0(cfg->reg) |= I2C_CTL0_STOP;

		k_sem_give(&data->sync_sem);
	}
}

static void i2c_gd32_handle_btc(const struct device *dev)
{
	struct i2c_gd32_data *data = dev->data;
	const struct i2c_gd32_config *cfg = dev->config;

	if (data->current->flags & I2C_MSG_READ) {
		uint32_t counter = 0U;

		switch (data->xfer_len) {
		case 2:
			/* Stop condition must be generated before reading the last two bytes. */
			I2C_CTL0(cfg->reg) |= I2C_CTL0_STOP;

			for (counter = 2U; counter > 0; counter--) {
				data->xfer_len--;
				i2c_gd32_xfer_read(data, cfg);
			}

			k_sem_give(&data->sync_sem);

			break;
		case 3:
			/* Clear ACKEN bit */
			I2C_CTL0(cfg->reg) &= ~I2C_CTL0_ACKEN;

			data->xfer_len--;
			i2c_gd32_xfer_read(data, cfg);

			break;
		default:
			i2c_gd32_handle_rbne(dev);
			break;
		}
	} else {
		i2c_gd32_handle_tbe(dev);
	}
}

static void i2c_gd32_handle_addsend(const struct device *dev)
{
	struct i2c_gd32_data *data = dev->data;
	const struct i2c_gd32_config *cfg = dev->config;

	if ((data->current->flags & I2C_MSG_READ) && (data->xfer_len <= 2U)) {
		I2C_CTL0(cfg->reg) &= ~I2C_CTL0_ACKEN;
	}

	/* Clear ADDSEND bit */
	I2C_STAT0(cfg->reg);
	I2C_STAT1(cfg->reg);

	if (data->is_restart) {
		data->is_restart = false;
		data->current->flags &= ~I2C_MSG_RW_MASK;
		data->current->flags |= I2C_MSG_READ;
		/* Enter repeated start condition */
		I2C_CTL0(cfg->reg) |= I2C_CTL0_START;

		return;
	}

	if ((data->current->flags & I2C_MSG_READ) && (data->xfer_len == 1U)) {
		/* Enter stop condition */
		I2C_CTL0(cfg->reg) |= I2C_CTL0_STOP;
	}
}

static void i2c_gd32_event_isr(const struct device *dev)
{
	struct i2c_gd32_data *data = dev->data;
	const struct i2c_gd32_config *cfg = dev->config;
	uint32_t stat;

	stat = I2C_STAT0(cfg->reg);

	if (stat & I2C_STAT0_SBSEND) {
		if (data->current->flags & I2C_MSG_READ) {
			I2C_DATA(cfg->reg) = (data->addr1 << 1U) | 1U;
		} else {
			I2C_DATA(cfg->reg) = (data->addr1 << 1U) | 0U;
		}
	} else if (stat & I2C_STAT0_ADD10SEND) {
		I2C_DATA(cfg->reg) = data->addr2;
	} else if (stat & I2C_STAT0_ADDSEND) {
		i2c_gd32_handle_addsend(dev);
	/*
	 * Must handle BTC first.
	 * For I2C_STAT0, BTC is the superset of RBNE and TBE.
	 */
	} else if (stat & I2C_STAT0_BTC) {
		i2c_gd32_handle_btc(dev);
	} else if (stat & I2C_STAT0_RBNE) {
		i2c_gd32_handle_rbne(dev);
	} else if (stat & I2C_STAT0_TBE) {
		i2c_gd32_handle_tbe(dev);
	}
}

static void i2c_gd32_error_isr(const struct device *dev)
{
	struct i2c_gd32_data *data = dev->data;
	const struct i2c_gd32_config *cfg = dev->config;
	uint32_t stat;

	stat = I2C_STAT0(cfg->reg);

	if (stat & I2C_STAT0_BERR) {
		I2C_STAT0(cfg->reg) &= ~I2C_STAT0_BERR;
		data->errs |= I2C_GD32_ERR_BERR;
	}

	if (stat & I2C_STAT0_LOSTARB) {
		I2C_STAT0(cfg->reg) &= ~I2C_STAT0_LOSTARB;
		data->errs |= I2C_GD32_ERR_LARB;
	}

	if (stat & I2C_STAT0_AERR) {
		I2C_STAT0(cfg->reg) &= ~I2C_STAT0_AERR;
		data->errs |= I2C_GD32_ERR_AERR;
	}

	if (data->errs != 0U) {
		/* Enter stop condition */
		I2C_CTL0(cfg->reg) |= I2C_CTL0_STOP;

		k_sem_give(&data->sync_sem);
	}
}

static void i2c_gd32_log_err(struct i2c_gd32_data *data)
{
	if (data->errs & I2C_GD32_ERR_BERR) {
		LOG_ERR("Bus error");
	}

	if (data->errs & I2C_GD32_ERR_LARB) {
		LOG_ERR("Arbitration lost");
	}

	if (data->errs & I2C_GD32_ERR_AERR) {
		LOG_ERR("No ACK received");
	}

	if (data->errs & I2C_GD32_ERR_BUSY) {
		LOG_ERR("I2C bus busy");
	}
}

static void i2c_gd32_xfer_begin(const struct device *dev)
{
	struct i2c_gd32_data *data = dev->data;
	const struct i2c_gd32_config *cfg = dev->config;

	k_sem_reset(&data->sync_sem);

	data->errs = 0U;
	data->is_restart = false;

	/* Default to set ACKEN bit. */
	I2C_CTL0(cfg->reg) |= I2C_CTL0_ACKEN;

	if (data->current->flags & I2C_MSG_READ) {
		/* For 2 bytes read, use POAP bit to give NACK for the last data receiving. */
		if (data->xfer_len == 2U) {
			I2C_CTL0(cfg->reg) |= I2C_CTL0_POAP;
		}

		/*
		 * For read on 10 bits address mode, start condition will happen twice.
		 * Transfer sequence as below:
		 *   S addr1+W addr2 S addr1+R
		 * Use a is_restart flag to cover this case.
		 */
		if (data->dev_config & I2C_ADDR_10_BITS) {
			data->is_restart = true;
			data->current->flags &= ~I2C_MSG_RW_MASK;
		}
	}

	i2c_gd32_enable_interrupts(cfg);

	/* Enter repeated start condition */
	I2C_CTL0(cfg->reg) |= I2C_CTL0_START;
}

static int i2c_gd32_xfer_end(const struct device *dev)
{
	struct i2c_gd32_data *data = dev->data;
	const struct i2c_gd32_config *cfg = dev->config;

	i2c_gd32_disable_interrupts(cfg);

	/* Wait for stop condition is done. */
	while (I2C_STAT1(cfg->reg) & I2C_STAT1_I2CBSY) {
		/* NOP */
	}

	if (data->errs) {
		return -EIO;
	}

	return 0;
}

static int i2c_gd32_msg_read(const struct device *dev)
{
	struct i2c_gd32_data *data = dev->data;
	const struct i2c_gd32_config *cfg = dev->config;

	if (I2C_STAT1(cfg->reg) & I2C_STAT1_I2CBSY) {
		data->errs = I2C_GD32_ERR_BUSY;
		return -EBUSY;
	}

	i2c_gd32_xfer_begin(dev);

	k_sem_take(&data->sync_sem, K_FOREVER);

	return i2c_gd32_xfer_end(dev);
}

static int i2c_gd32_msg_write(const struct device *dev)
{
	struct i2c_gd32_data *data = dev->data;
	const struct i2c_gd32_config *cfg = dev->config;

	if (I2C_STAT1(cfg->reg) & I2C_STAT1_I2CBSY) {
		data->errs = I2C_GD32_ERR_BUSY;
		return -EBUSY;
	}

	i2c_gd32_xfer_begin(dev);

	k_sem_take(&data->sync_sem, K_FOREVER);

	return i2c_gd32_xfer_end(dev);
}

static int i2c_gd32_transfer(const struct device *dev,
			     struct i2c_msg *msgs,
			     uint8_t num_msgs,
			     uint16_t addr)
{
	struct i2c_gd32_data *data = dev->data;
	const struct i2c_gd32_config *cfg = dev->config;
	struct i2c_msg *current, *next;
	uint8_t itr;
	int err = 0;

	current = msgs;

	/* First message flags implicitly contain I2C_MSG_RESTART flag. */
	current->flags |= I2C_MSG_RESTART;

	for (uint8_t i = 1; i <= num_msgs; i++) {

		if (i < num_msgs) {
			next = current + 1;

			/*
			 * If there have a R/W transfer state change between messages,
			 * An explicit I2C_MSG_RESTART flag is needed for the second message.
			 */
			if ((current->flags & I2C_MSG_RW_MASK) !=
			(next->flags & I2C_MSG_RW_MASK)) {
				if ((next->flags & I2C_MSG_RESTART) == 0U) {
					return -EINVAL;
				}
			}

			/* Only the last message need I2C_MSG_STOP flag to free the Bus. */
			if (current->flags & I2C_MSG_STOP) {
				return -EINVAL;
			}
		} else {
			/* Last message flags implicitly contain I2C_MSG_STOP flag. */
			current->flags |= I2C_MSG_STOP;
		}

		if ((current->buf == NULL) ||
		    (current->len == 0U)) {
			return -EINVAL;
		}

		current++;
	}

	k_sem_take(&data->bus_mutex, K_FOREVER);

	/* Enable i2c device */
	I2C_CTL0(cfg->reg) |= I2C_CTL0_I2CEN;

	if (data->dev_config & I2C_ADDR_10_BITS) {
		data->addr1 = 0xF0 | ((addr & BITS(8, 9)) >> 8U);
		data->addr2 = addr & BITS(0, 7);
	} else {
		data->addr1 = addr & BITS(0, 6);
	}

	for (uint8_t i = 0; i < num_msgs; i = itr) {
		data->current = &msgs[i];
		data->xfer_len = msgs[i].len;

		for (itr = i + 1; itr < num_msgs; itr++) {
			if ((data->current->flags & I2C_MSG_RW_MASK) !=
			    (msgs[itr].flags & I2C_MSG_RW_MASK)) {
				break;
			}
			data->xfer_len += msgs[itr].len;
		}

		if (data->current->flags & I2C_MSG_READ) {
			err = i2c_gd32_msg_read(dev);
		} else {
			err = i2c_gd32_msg_write(dev);
		}

		if (err < 0) {
			i2c_gd32_log_err(data);
			break;
		}
	}

	/* Disable I2C device */
	I2C_CTL0(cfg->reg) &= ~I2C_CTL0_I2CEN;

	k_sem_give(&data->bus_mutex);

	return err;
}

static int i2c_gd32_configure(const struct device *dev,
			      uint32_t dev_config)
{
	struct i2c_gd32_data *data = dev->data;
	const struct i2c_gd32_config *cfg = dev->config;
	uint32_t pclk1, freq, clkc;
	int err = 0;

	k_sem_take(&data->bus_mutex, K_FOREVER);

	/* Disable I2C device */
	I2C_CTL0(cfg->reg) &= ~I2C_CTL0_I2CEN;

	/* GD32 i2c interface always connect to APB1. */
	pclk1 = rcu_clock_freq_get(CK_APB1);

	/* i2c clock frequency, us */
	freq = pclk1 / 1000000U;
	if (freq > I2CCLK_MAX) {
		LOG_ERR("I2C max clock freq %u, current is %u\n",
			I2CCLK_MAX, freq);
		err = -ENOTSUP;
		goto error;
	}

	/*
	 * Refer from SoC user manual.
	 * In standard mode:
	 *   T_high = CLKC * T_pclk1
	 *   T_low  = CLKC * T_pclk1
	 *
	 * In fast mode and fast mode plus with DTCY=1:
	 *   T_high = 9 * CLKC * T_pclk1
	 *   T_low  = 16 * CLKC * T_pclk1
	 *
	 * T_pclk1 is reciprocal of pclk1:
	 *   T_pclk1 = 1 / pclk1
	 *
	 * T_high and T_low construct the bit transfer:
	 *  T_high + T_low = 1 / bitrate
	 *
	 * And then, we can get the CLKC equation.
	 * Standard mode:
	 *   CLKC = pclk1 / (bitrate * 2)
	 * Fast mode and fast mode plus:
	 *   CLKC = pclk1 / (bitrate * 25)
	 *
	 * Variable list:
	 *   T_high:  high period of the SCL clock
	 *   T_low:   low period of the SCL clock
	 *   T_pclk1: duration of single pclk1 pulse
	 *   pclk1:   i2c device clock frequency
	 *   bitrate: 100 Kbits for standard mode
	 */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		if (freq < I2CCLK_MIN) {
			LOG_ERR("I2C standard-mode min clock freq %u, current is %u\n",
				I2CCLK_MIN, freq);
			err = -ENOTSUP;
			goto error;
		}
		I2C_CTL1(cfg->reg) &= ~I2C_CTL1_I2CCLK;
		I2C_CTL1(cfg->reg) |= freq;

		/* Standard-mode risetime maximum value: 1000ns */
		if (freq == I2CCLK_MAX) {
			I2C_RT(cfg->reg) = I2CCLK_MAX;
		} else {
			I2C_RT(cfg->reg) = freq + 1U;
		}

		/* CLKC = pclk1 / (bitrate * 2) */
		clkc = pclk1 / (I2C_BITRATE_STANDARD * 2U);

		I2C_CKCFG(cfg->reg) &= ~I2C_CKCFG_CLKC;
		I2C_CKCFG(cfg->reg) |= clkc;
		/* standard-mode */
		I2C_CKCFG(cfg->reg) &= ~I2C_CKCFG_FAST;

		break;
	case I2C_SPEED_FAST:
		if (freq < I2CCLK_FM_MIN) {
			LOG_ERR("I2C fast-mode min clock freq %u, current is %u\n",
				I2CCLK_FM_MIN, freq);
			err = -ENOTSUP;
			goto error;
		}

		/* Fast-mode risetime maximum value: 300ns */
		I2C_RT(cfg->reg) = freq * 300U / 1000U + 1U;

		/* CLKC = pclk1 / (bitrate * 25) */
		clkc = pclk1 / (I2C_BITRATE_FAST * 25U);
		if (clkc == 0U) {
			clkc = 1U;
		}

		/* Default DCTY to 1 */
		I2C_CKCFG(cfg->reg) |= I2C_CKCFG_DTCY;
		I2C_CKCFG(cfg->reg) &= ~I2C_CKCFG_CLKC;
		I2C_CKCFG(cfg->reg) |= clkc;
		/* Transfer mode: fast-mode */
		I2C_CKCFG(cfg->reg) |= I2C_CKCFG_FAST;

#ifdef I2C_FMPCFG
		/* Disable transfer mode: fast-mode plus */
		I2C_FMPCFG(cfg->reg) &= ~I2C_FMPCFG_FMPEN;
#endif /* I2C_FMPCFG */

		break;
#ifdef I2C_FMPCFG
	case I2C_SPEED_FAST_PLUS:
		if (freq < I2CCLK_FM_PLUS_MIN) {
			LOG_ERR("I2C fast-mode plus min clock freq %u, current is %u\n",
				I2CCLK_FM_PLUS_MIN, freq);
			err = -ENOTSUP;
			goto error;
		}

		/* Fast-mode plus risetime maximum value: 120ns */
		I2C_RT(cfg->reg) = freq * 120U / 1000U + 1U;

		/* CLKC = pclk1 / (bitrate * 25) */
		clkc = pclk1 / (I2C_BITRATE_FAST_PLUS * 25U);
		if (clkc == 0U) {
			clkc = 1U;
		}

		/* Default DCTY to 1 */
		I2C_CKCFG(cfg->reg) |= I2C_CKCFG_DTCY;
		I2C_CKCFG(cfg->reg) &= ~I2C_CKCFG_CLKC;
		I2C_CKCFG(cfg->reg) |= clkc;
		/* Transfer mode: fast-mode */
		I2C_CKCFG(cfg->reg) |= I2C_CKCFG_FAST;

		/* Enable transfer mode: fast-mode plus */
		I2C_FMPCFG(cfg->reg) |= I2C_FMPCFG_FMPEN;

		break;
#endif /* I2C_FMPCFG */
	default:
		err = -EINVAL;
		goto error;
	}

	data->dev_config = dev_config;
error:
	k_sem_give(&data->bus_mutex);

	return err;
}

static struct i2c_driver_api i2c_gd32_driver_api = {
	.configure = i2c_gd32_configure,
	.transfer = i2c_gd32_transfer,
};

static int i2c_gd32_init(const struct device *dev)
{
	struct i2c_gd32_data *data = dev->data;
	const struct i2c_gd32_config *cfg = dev->config;
	uint32_t bitrate_cfg;
	int err;

	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	/* Mutex semaphore to protect the i2c api in multi-thread env. */
	k_sem_init(&data->bus_mutex, 1, 1);

	/* Sync semaphore to sync i2c state between isr and transfer api. */
	k_sem_init(&data->sync_sem, 0, K_SEM_MAX_LIMIT);

	rcu_periph_clock_enable(cfg->rcu_periph_clock);

	cfg->irq_cfg_func();

	bitrate_cfg = i2c_map_dt_bitrate(cfg->bitrate);

	i2c_gd32_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);

	return 0;
}

#define I2C_GD32_INIT(inst)							\
	PINCTRL_DT_INST_DEFINE(inst);						\
	static void i2c_gd32_irq_cfg_func_##inst(void)				\
	{									\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, event, irq),		\
			    DT_INST_IRQ_BY_NAME(inst, event, priority),		\
			    i2c_gd32_event_isr,					\
			    DEVICE_DT_INST_GET(inst),				\
			    0);							\
		irq_enable(DT_INST_IRQ_BY_NAME(inst, event, irq));		\
										\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, error, irq),		\
			    DT_INST_IRQ_BY_NAME(inst, error, priority),		\
			    i2c_gd32_error_isr,					\
			    DEVICE_DT_INST_GET(inst),				\
			    0);							\
		irq_enable(DT_INST_IRQ_BY_NAME(inst, error, irq));		\
	}									\
	static struct i2c_gd32_data i2c_gd32_data_##inst;			\
	const static struct i2c_gd32_config i2c_gd32_cfg_##inst = {		\
		.reg = DT_INST_REG_ADDR(inst),					\
		.bitrate = DT_INST_PROP(inst, clock_frequency),			\
		.rcu_periph_clock = DT_INST_PROP(inst, rcu_periph_clock),	\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
		.irq_cfg_func = i2c_gd32_irq_cfg_func_##inst,			\
	};									\
	I2C_DEVICE_DT_INST_DEFINE(inst,						\
				  i2c_gd32_init, NULL,				\
				  &i2c_gd32_data_##inst, &i2c_gd32_cfg_##inst,	\
				  POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,	\
				  &i2c_gd32_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(I2C_GD32_INIT)
