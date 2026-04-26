/*
 * Copyright (c) 2026 Fiona Behrens (me@kloenk.dev)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numicro_i2c

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numicro.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2c_numicro, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"
#include <NuMicro.h>

/* i2c Controller Mode Status */
#define C_START           0x08 /* Start */
#define C_REPEAT_START    0x10 /* Controller Repeat Start */
#define C_TRANS_ADDR_ACK  0x18 /* Controller Transmit Address ACK */
#define C_TRANS_ADDR_NACK 0x20 /* Controller Transmit Address NACK */
#define C_TRANS_DATA_ACK  0x28 /* Controller Transmit Data ACK */
#define C_TRANS_DATA_NACK 0x30 /* Controller Transmit Data NACK */
#define C_ARB_LOST        0x38 /* Controller Arbitration Los */
#define C_RECE_ADDR_ACK   0x40 /* Controller Receive Address ACK */
#define C_RECE_ADDR_NACK  0x48 /* Controller Receive Address NACK */
#define C_RECE_DATA_ACK   0x50 /* Controller Receive Data ACK */
#define C_RECE_DATA_NACK  0x58 /* Controller Receive Data NACK */
#define BUS_ERROR         0x00 /* Bus error */

/* i2c Slave Mode Status */
#define T_REPEAT_START_STOP   0xA0 /* Target Transmit Repeat Start or Stop */
#define T_TRANS_ADDR_ACK      0xA8 /* Target Transmit Address ACK */
#define T_TRANS_DATA_ACK      0xB8 /* Target Transmit Data ACK */
#define T_TRANS_DATA_NACK     0xC0 /* Target Transmit Data NACK */
#define T_TRANS_LAST_DATA_ACK 0xC8 /* Target Transmit Last Data ACK */
#define T_RECE_ADDR_ACK       0x60 /* Target Receive Address ACK */
#define T_RECE_ARB_LOST       0x68 /* Target Receive Arbitration Lost */
#define T_RECE_DATA_ACK       0x80 /* Target Receive Data ACK */
#define T_RECE_DATA_NACK      0x88 /* Target Receive Data NACK */

/* i2c GC Mode Status */
#define GC_ADDR_ACK  0x70 /* GC mode Address ACK */
#define GC_ARB_LOST  0x78 /* GC mode Arbitration Lost */
#define GC_DATA_ACK  0x90 /* GC mode Data ACK */
#define GC_DATA_NACK 0x98 /* GC mode Data NACK */

/* i2c Other Status */
#define ADDR_TRANS_ARB_LOST 0xB0 /* Address Transmit Arbitration Lost */
#define BUS_RELEASED        0xF8 /* Bus Released */

struct numicro_i2c_config {
	I2C_T *regs;
	const struct reset_dt_spec reset;
	const struct numicro_scc_subsys clock_subsys;
	const struct device *clk_dev;

	uint32_t irq_n;
	void (*irq_config_func)(const struct device *dev);

	const struct pinctrl_dev_config *pincfg;
	uint32_t bitrate;
};

struct numicro_i2c_data {
	struct k_sem lock;
	uint32_t dev_config;
	/* Controller transfer context */
	struct {
		struct k_sem xfer_sync;
		uint16_t addr;
		struct i2c_msg *msgs_beg;
		struct i2c_msg *msgs_pos;
		struct i2c_msg *msgs_end;
		uint8_t *buf_beg;
		uint8_t *buf_pos;
		uint8_t *buf_end;
	} controller_xfer;
#ifdef CONFIG_I2C_TARGET
	/* Target transfer context */
	struct {
		struct i2c_target_config *target_config;
		bool target_addressed;
	} target_xfer;
#endif
};

static int numicro_i2c_configure(const struct device *dev, uint32_t dev_config)
{
	const struct numicro_i2c_config *config = dev->config;
	struct numicro_i2c_data *data = dev->data;
	uint32_t bitrate;
	int err;

	/* Check address size */
	if (dev_config & I2C_ADDR_10_BITS) {
		/* TODO: implement, hardware seems to support it */
		LOG_ERR("10-bits address not yet supported");
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		bitrate = KHZ(100);
		break;
	case I2C_SPEED_FAST:
		bitrate = KHZ(400);
		break;
	case I2C_SPEED_FAST_PLUS:
		bitrate = MHZ(1);
		break;
	case I2C_SPEED_HIGH:
		bitrate = KHZ(3400);
		break;
	default:
		LOG_ERR("Speed code %d not supported", I2C_SPEED_GET(dev_config));
		return -ENOTSUP;
	}

	if (k_sem_take(&data->lock, K_MSEC(100)) != 0) {
		return -EAGAIN;
	}
	irq_disable(config->irq_n);

#ifdef CONFIG_I2C_TARGET
	if (data->target_xfer.target_addressed) {
		LOG_ERR("Reconfigure with target being busy");
		err = -EBUSY;
		goto out;
	}
#endif

	/* I2C_Open */
	uint32_t i2c_div, pclk_freq;

	err = clock_control_get_rate(config->clk_dev, (clock_control_subsys_t)&config->clock_subsys,
				     &pclk_freq);
	if (err < 0) {
		goto out;
	}

	/* Compute proper divider for I2C clock */
	i2c_div = ((pclk_freq * 10U) / (bitrate * 4U) + 5U) / 10U - 1U;
	config->regs->CLKDIV = i2c_div;

	/* Enable I2C */
	config->regs->CTL0 |= (I2C_CTL0_INTEN_Msk | I2C_CTL0_I2CEN_Msk);
	/* end I2C_Open */

	data->dev_config = dev_config;

out:
	irq_enable(config->irq_n);
	k_sem_give(&data->lock);

	return err;
}

static int numicro_i2c_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct numicro_i2c_data *data = dev->data;

	if (!dev_config) {
		return -EINVAL;
	}

	if (k_sem_take(&data->lock, K_MSEC(100)) != 0) {
		return -EAGAIN;
	}

	*dev_config = data->dev_config;

	k_sem_give(&data->lock);

	return 0;
}

/*
 * Controller active transfer:
 * 1. Do I2C Start to start the transfer (thread)
 * 2. I2C FSM (ISR)
 * 3. FOrce I2C Stop to end the transfer (thread)
 * Target passive transfer:
 * 1. Prepare callback (thread)
 * 2. Do data transfer via above callback (ISR)
 */
static int numicro_i2c_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				uint16_t addr)
{
	const struct numicro_i2c_config *config = dev->config;
	struct numicro_i2c_data *data = dev->data;
	int err = 0;

	if (k_sem_take(&data->lock, K_MSEC(100)) != 0) {
		return -EAGAIN;
	}
	irq_disable(config->irq_n);

#ifdef CONFIG_I2C_TARGET
	if (data->target_xfer.target_addressed) {
		LOG_ERR("Controller transfer while target being busy");
		err = -EBUSY;
		goto out;
	}
#endif

	if (num_msgs == 0) {
		goto out;
	}

	/* Prepare to start transfer */
	data->controller_xfer.addr = addr;
	data->controller_xfer.msgs_beg = msgs;
	data->controller_xfer.msgs_pos = msgs;
	data->controller_xfer.msgs_end = msgs + num_msgs;

	/* Do I2C Start to start the transfer */
	I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_STA_Msk | I2C_CTL0_SI_Msk);

	irq_enable(config->irq_n);
	k_sem_take(&data->controller_xfer.xfer_sync, K_FOREVER);
	irq_disable(config->irq_n);

	/* Check transfer result */
	if (data->controller_xfer.msgs_pos != data->controller_xfer.msgs_end) {
		bool is_read =
			(data->controller_xfer.msgs_pos->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
		bool is_10bit = data->controller_xfer.msgs_pos->flags & I2C_MSG_ADDR_10_BITS;

		LOG_DBG("Failed message:");
		LOG_DBG("MSG IDX: %d",
			data->controller_xfer.msgs_pos - data->controller_xfer.msgs_beg);
		LOG_DBG("ADDR (%d-bit): 0x%04X", is_10bit ? 10 : 7, addr);
		LOG_DBG("DIR: %s", is_read ? "R" : "W");
		LOG_DBG("Expected %d bytes transferred, but actual %d",
			data->controller_xfer.msgs_pos->len,
			data->controller_xfer.buf_pos - data->controller_xfer.buf_beg);
		err = -EIO;
		goto i2c_stop;
	}

i2c_stop:
	/* DO I2C stop to release bus ownership */
	I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_STO_Msk | I2C_CTL0_SI_Msk);

#ifdef CONFIG_I2C_TARGET
	/* Enable target mode if one target is registered */
	if (data->target_xfer.target_config) {
		I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
	}
#endif

out:
	irq_enable(config->irq_n);
	k_sem_give(&data->lock);

	return err;
}

static int numicro_i2c_recover_bus(const struct device *dev)
{
	const struct numicro_i2c_config *config = dev->config;
	struct numicro_i2c_data *data = dev->data;

	if (k_sem_take(&data->lock, K_MSEC(100)) != 0) {
		return -EAGAIN;
	}
	k_sem_take(&data->lock, K_FOREVER);

	/* Do I2C Stop to release bus ownership */
	I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_STO_Msk | I2C_CTL0_SI_Msk);

	k_sem_give(&data->lock);

	return 0;
}

#ifdef CONFIG_I2C_TARGET
static int numicro_i2c_target_set_addr(const struct device *dev, uint8_t addr_no, uint16_t addr,
				       uint8_t gcmode)
{
	const struct numicro_i2c_config *config = dev->config;

	switch (addr_no) {
	case 0:
		config->regs->ADDR0 = ((uint32_t)addr << 1) | gcmode;
		break;
	case 1:
		config->regs->ADDR1 = ((uint32_t)addr << 1) | gcmode;
		break;
	case 2:
		config->regs->ADDR2 = ((uint32_t)addr << 1) | gcmode;
		break;
	case 3:
		config->regs->ADDR3 = ((uint32_t)addr << 1) | gcmode;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int numicro_i2c_target_register(const struct device *dev,
				       struct i2c_target_config *target_config)
{
	const struct numicro_i2c_config *config = dev->config;
	struct numicro_i2c_data *data = dev->data;
	int err = 0;

	if (!target_config || !target_config->callbacks) {
		return -EINVAL;
	}

	if (target_config->flags & I2C_ADDR_10_BITS) {
		/* TODO: implement */
		LOG_ERR("10-bits address yet not supported");
		return -ENOTSUP;
	}

	if (k_sem_take(&data->lock, K_MSEC(100)) != 0) {
		return -EAGAIN;
	}
	irq_disable(config->irq_n);

	if (data->target_xfer.target_config) {
		err = -EBUSY;
		goto out;
	}

	data->target_xfer.target_config = target_config;
	/* Target address */
	err = numicro_i2c_target_set_addr(dev, 0, target_config->address, I2C_GCMODE_DISABLE);
	if (err < 0) {
		goto out;
	}

	/* target address state */
	data->target_xfer.target_addressed = false;

	/* Enable Target mode */
	I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);

out:

	irq_enable(config->irq_n);
	k_sem_give(&data->lock);

	return err;
}

static int numicro_i2c_target_unregister(const struct device *dev,
					 struct i2c_target_config *target_config)
{
	const struct numicro_i2c_config *config = dev->config;
	struct numicro_i2c_data *data = dev->data;
	int err = 0;

	if (!target_config) {
		return -EINVAL;
	}

	if (k_sem_take(&data->lock, K_MSEC(100)) != 0) {
		return -EAGAIN;
	}
	irq_disable(config->irq_n);

	if (data->target_xfer.target_config != target_config) {
		err = -EINVAL;
		goto out;
	}

	if (data->target_xfer.target_addressed) {
		LOG_ERR("Unregister target driver while target is busy");
		err = -EBUSY;
		goto out;
	}

	err = numicro_i2c_target_set_addr(dev, 0, 0, I2C_GCMODE_DISABLE);
	if (err < 0) {
		goto out;
	}

	/* Target Address state */
	I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk);
	data->target_xfer.target_config = NULL;

out:
	irq_enable(config->irq_n);
	k_sem_give(&data->lock);

	return err;
}
#endif /* CONFIG_I2C_TARGET */

static DEVICE_API(i2c, numicro_i2c_driver_api) = {
	.configure = numicro_i2c_configure,
	.get_config = numicro_i2c_get_config,
	.transfer = numicro_i2c_transfer,
	.recover_bus = numicro_i2c_recover_bus,
#ifdef CONFIG_I2C_TARGET
	.target_register = numicro_i2c_target_register,
	.target_unregister = numicro_i2c_target_unregister,
#endif
};

/* ACK/NACK last data byte, dependent on whether message merge is allowed */
static void numicro_i2c_controller_xfer_msg_read_last_byte(const struct device *dev)
{
	const struct numicro_i2c_config *config = dev->config;
	struct numicro_i2c_data *data = dev->data;

	/* Shouldn't invoke with message pointer OOB */
	__ASSERT_NO_MSG(data->controller_xfer.msgs_pos < data->controller_xfer.msgs_end);
	/* Should invode with exactly one data byte remaining for read */
	__ASSERT_NO_MSG((data->controller_xfer.msgs_pos->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ);
	__ASSERT_NO_MSG((data->controller_xfer.buf_end - data->controller_xfer.buf_pos) == 1);

	/* Flags of previous message */
	bool do_stop_prev = data->controller_xfer.msgs_pos->flags & I2C_MSG_STOP;

	/* Advance to next message temporarily */
	data->controller_xfer.msgs_pos++;

	/* Has next message? */
	if (data->controller_xfer.msgs_pos < data->controller_xfer.msgs_end) {
		/* Flags of next message */
		struct i2c_msg *msgs_pos = data->controller_xfer.msgs_pos;
		bool is_read_next = (msgs_pos->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
		bool do_restart_next = data->controller_xfer.msgs_pos->flags & I2C_MSG_RESTART;

		/*
		 * Different R/W bit so message merge is disallowed.
		 * Force I2C Repeat Start on I2C Stop/Repeat start missing
		 */
		if (!is_read_next) {
			if (!do_stop_prev && !do_restart_next) {
				do_restart_next = true;
			}
		}

		if (do_stop_prev || do_restart_next) {
			/* NACK last data byte (required for controller) */
			I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk);
		} else {
			/* ACK last data byte, so to merge adjacent messages into one transaction */
			I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		}
	} else {
		/* NACK last data byte (required for controller) */
		I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk);
	}

	/* Roll back message pointer */
	data->controller_xfer.msgs_pos--;
}

/* End the transfer, Involving I2C Stop and signal to thread */
static void numicro_i2c_controller_xfer_end(const struct device *dev, bool do_stop)
{
	const struct numicro_i2c_config *config = dev->config;
	struct numicro_i2c_data *data = dev->data;

	if (do_stop) {
		I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_STO_Msk | I2C_CTL0_SI_Msk);
	}

	/* Signal controller transfer end */
	k_sem_give(&data->controller_xfer.xfer_sync);
}

static void numicro_i2c_controller_xfer_msg_end(const struct device *dev);
/* Read next data byte, involving ACK/NACK last data byte and message merge */
static void numicro_i2c_controller_xfer_msg_read_next_byte(const struct device *dev)
{
	const struct numicro_i2c_config *config = dev->config;
	struct numicro_i2c_data *data = dev->data;

	switch (data->controller_xfer.buf_end - data->controller_xfer.buf_pos) {
	case 0:
		/* Last data byte ACKed, we'll do message merge */
		numicro_i2c_controller_xfer_msg_end(dev);
		break;
	case 1:
		/* Read last data yte for this message */
		numicro_i2c_controller_xfer_msg_read_last_byte(dev);
		break;
	default:
		/* ACK non-last data byte */
		I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
	}
}

static void numicro_i2c_controller_xfer_msg_end(const struct device *dev)
{
	const struct numicro_i2c_config *config = dev->config;
	struct numicro_i2c_data *data = dev->data;

	/* Shouldn't invoke with message pointer OOB */
	__ASSERT_NO_MSG(data->controller_xfer.msgs_pos < data->controller_xfer.msgs_end);
	/* Should have transferred up */
	__ASSERT_NO_MSG((data->controller_xfer.buf_end - data->controller_xfer.buf_pos) == 0);

	/* Flags of previous message */
	bool is_read_prev =
		(data->controller_xfer.msgs_pos->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
	bool do_stop_prev = data->controller_xfer.msgs_pos->flags & I2C_MSG_STOP;

	/* Advance to next message */
	data->controller_xfer.msgs_pos++;

	/* Has next message? */
	if (data->controller_xfer.msgs_pos < data->controller_xfer.msgs_end) {
		/* Flags of next message */
		struct i2c_msg *msgs_pos = data->controller_xfer.msgs_pos;
		bool is_read_next = (msgs_pos->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
		bool do_restart_next = msgs_pos->flags & I2C_MSG_RESTART;

		/*
		 * Different R/W bit so message merge is disallowed.
		 * Force I2C Repeat stat on I2C stop/repeat start missing
		 */
		if (!is_read_prev != !is_read_next /* Logical XOR idiom */) {
			if (!do_stop_prev && !do_restart_next) {
				LOG_WRN("Cannot merge adjacent message, force I2C Repeat Start");
				do_restart_next = true;
			}
		}

		if (do_stop_prev) {
			/* Do I2C Stop and then Start */
			I2C_SET_CONTROL_REG(config->regs,
					    I2C_CTL0_STA_Msk | I2C_CTL0_STO_Msk | I2C_CTL0_SI_Msk);
		} else if (do_restart_next) {
			/* Do I2C Repeat Start */
			I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_STA_Msk | I2C_CTL0_SI_Msk);
		} else {
			/* Merge into the same transaction */

			/* Prepare buffer for current message */
			data->controller_xfer.buf_beg = data->controller_xfer.msgs_pos->buf;
			data->controller_xfer.buf_pos = data->controller_xfer.msgs_pos->buf;
			data->controller_xfer.buf_end = data->controller_xfer.msgs_pos->buf +
							data->controller_xfer.msgs_pos->len;

			if (is_read_prev) {
				numicro_i2c_controller_xfer_msg_read_next_byte(dev);
			} else {
				/*
				 * Interrupt flag not cleared, expect to re-enter ISR with context
				 * unchanged, except buffer changed for message change.
				 */
			}
		}
	} else {
		if (!do_stop_prev) {
			LOG_WRN("Last message not marged I2C Stop");
		}

		numicro_i2c_controller_xfer_end(dev, do_stop_prev);
	}
}

static void numicro_i2c_isr(const struct device *dev)
{
	const struct numicro_i2c_config *config = dev->config;
	struct numicro_i2c_data *data = dev->data;
#ifdef CONFIG_I2C_TARGET
	struct i2c_target_config *target_config = data->target_xfer.target_config;
	const struct i2c_target_callbacks *target_callbacks =
		target_config ? target_config->callbacks : NULL;
	uint8_t data_byte;
#endif
	uint32_t status;

	if (I2C_GET_TIMEOUT_FLAG(config->regs)) {
		config->regs->TOCTL |= I2C_TOCTL_TOIF_Msk;
		return;
	}

	status = I2C_GET_STATUS(config->regs);

	switch (status) {
	case C_START:
	case C_REPEAT_START:
		/* Prepare buffer for current message */
		data->controller_xfer.buf_beg = data->controller_xfer.msgs_pos->buf;
		data->controller_xfer.buf_pos = data->controller_xfer.msgs_pos->buf;
		data->controller_xfer.buf_end =
			data->controller_xfer.msgs_pos->buf + data->controller_xfer.msgs_pos->len;

		/* Write I2C Address */
		struct i2c_msg *msgs_pos = data->controller_xfer.msgs_pos;
		bool is_read = (msgs_pos->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
		uint16_t addr = data->controller_xfer.addr;
		int addr_rw = is_read ? ((addr << 1) | 1) : (addr << 1);

		I2C_SET_DATA(config->regs, (uint8_t)(addr_rw & 0xFF));
		I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk);
		/* TODO: 10bit addrs? */
		break;
	case C_TRANS_ADDR_ACK:
	case C_TRANS_DATA_ACK:
		__ASSERT_NO_MSG(data->controller_xfer.buf_pos);
		if (data->controller_xfer.buf_pos < data->controller_xfer.buf_end) {
			I2C_SET_DATA(config->regs, *data->controller_xfer.buf_pos++);
			I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		} else {
			/* End this message */
			numicro_i2c_controller_xfer_msg_end(dev);
		}
		break;
	case C_TRANS_ADDR_NACK:
	case C_TRANS_DATA_NACK:
	case C_RECE_ADDR_NACK:
	case C_ARB_LOST:
		numicro_i2c_controller_xfer_end(dev, true);
		break;
	case C_RECE_ADDR_ACK:
	case C_RECE_DATA_ACK:
		__ASSERT_NO_MSG(data->controller_xfer.buf_pos);

		if (status == C_RECE_ADDR_ACK) {
			__ASSERT_NO_MSG(data->controller_xfer.buf_pos <
					data->controller_xfer.buf_end);
		} else if (status == C_RECE_DATA_ACK) {
			__ASSERT_NO_MSG((data->controller_xfer.buf_end -
					 data->controller_xfer.buf_pos) >= 1);
			*data->controller_xfer.buf_pos++ = I2C_GET_DATA(config->regs);
		}

		numicro_i2c_controller_xfer_msg_read_next_byte(dev);
		break;
	case C_RECE_DATA_NACK:
		__ASSERT_NO_MSG((data->controller_xfer.buf_end - data->controller_xfer.buf_pos) ==
				1);
		*data->controller_xfer.buf_pos++ = I2C_GET_DATA(config->regs);
		/* End this message */
		numicro_i2c_controller_xfer_msg_end(dev);
		break;
	case BUS_ERROR:
		numicro_i2c_controller_xfer_end(dev, true);
		break;
#ifdef CONFIG_I2C_TARGET
		/* NOTE: Don't disable interrupt here because slave mode relies on
		 * it for passive transfer in ISR.
		 */

	case T_TRANS_ADDR_ACK:
	case ADDR_TRANS_ARB_LOST:
		data->target_xfer.target_addressed = true;
		if (target_callbacks->read_processed(target_config, &data_byte) == 0) {
			/* Non-last data byte */
			I2C_SET_DATA(config->regs, data_byte);
			I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		} else {
			/* GO T_TRANS_LAST_DATA_ACK on error */
			I2C_SET_DATA(config->regs, 0xFF);
			I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk);
		}
		break;
	case T_TRANS_DATA_NACK:
	case T_TRANS_LAST_DATA_ACK:
		data->target_xfer.target_addressed = false;
		target_callbacks->stop(target_config);
		I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		break;
	case T_RECE_DATA_ACK:
		data_byte = I2C_GET_DATA(config->regs);
		if (target_callbacks->write_received(target_config, data_byte) == 0) {
			/* Write OK, Ack next data byte */
			I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		} else {
			/* Write Failed, NACK next data byte */
			I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk);
		}
		break;
	case T_RECE_DATA_NACK:
		data->target_xfer.target_addressed = false;
		target_callbacks->stop(target_config);
		I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		break;
	case T_RECE_ADDR_ACK:
	case T_RECE_ARB_LOST:
		data->target_xfer.target_addressed = true;
		if (target_callbacks->write_requested(target_config) == 0) {
			/* Write ready, ACK next byte */
			I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		} else {
			/* Write not ready, NACK next byte */
			I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk);
		}
		break;
	case T_REPEAT_START_STOP:
		data->target_xfer.target_addressed = false;
		target_callbacks->stop(target_config);
		I2C_SET_CONTROL_REG(config->regs, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
		break;
#endif

	case BUS_RELEASED:
		break;
	default:
		__ASSERT(false, "Uncaught I2C FSM state");
		numicro_i2c_controller_xfer_end(dev, true);
	}
}

static int numicro_i2c_init(const struct device *dev)
{
	const struct numicro_i2c_config *config = dev->config;
	struct numicro_i2c_data *data = dev->data;
	int err = 0;

	/* Validate this module's reset object */
	if (!device_is_ready(config->reset.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}

	/* Clean mutable context */
	memset(data, 0x00, sizeof(*data));

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->controller_xfer.xfer_sync, 0, 1);

	err = clock_control_on(config->clk_dev, (clock_control_subsys_t)&config->clock_subsys);
	if (err != 0) {
		return err;
	}

	err = clock_control_configure(config->clk_dev,
				      (clock_control_subsys_t)&config->clock_subsys, NULL);
	if (err != 0) {
		return err;
	}

	/* Configure pinmux */
	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	/* Reset I2C to default state */
	reset_line_toggle_dt(&config->reset);

	err = numicro_i2c_configure(dev, I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate));
	if (err != 0) {
		return err;
	}

	config->irq_config_func(dev);

	return err;
}

#define NUMICRO_I2C_INIT(inst)                                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static void numicro_i2c_irq_config_func_##inst(const struct device *dev)                   \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), numicro_i2c_isr,      \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
	}                                                                                          \
                                                                                                   \
	static const struct numicro_i2c_config numicro_i2c_config_##inst = {                       \
		.regs = (I2C_T *)DT_INST_REG_ADDR(inst),                                           \
		.reset = RESET_DT_SPEC_INST_GET(inst),                                             \
		.clock_subsys =                                                                    \
			{                                                                          \
				.subsys_id = NUMICRO_SCC_SUBSYS_ID_PCC,                            \
				.pcc =                                                             \
					{                                                          \
						.clk_mod = DT_INST_CLOCKS_CELL(                    \
							inst, clock_module_index),                 \
						.clk_src =                                         \
							DT_INST_CLOCKS_CELL(inst, clock_source),   \
						.clk_div =                                         \
							DT_INST_CLOCKS_CELL(inst, clock_divider),  \
					},                                                         \
			},                                                                         \
		.clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))),                    \
		.irq_n = DT_INST_IRQN(inst),                                                       \
		.irq_config_func = numicro_i2c_irq_config_func_##inst,                             \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.bitrate = DT_INST_PROP(inst, clock_frequency),                                    \
	};                                                                                         \
                                                                                                   \
	static struct numicro_i2c_data numicro_i2c_data_##inst;                                    \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(inst, numicro_i2c_init, NULL, &numicro_i2c_data_##inst,          \
				  &numicro_i2c_config_##inst, POST_KERNEL,                         \
				  CONFIG_I2C_INIT_PRIORITY, &numicro_i2c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NUMICRO_I2C_INIT);
