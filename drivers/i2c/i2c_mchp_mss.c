/*
 * Copyright (c) 2023 Microchip Technology Inc.
 * Copyright (C) 2025 embedded brains GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(i2c_mchp, CONFIG_I2C_LOG_LEVEL);

#define DT_DRV_COMPAT microchip_mpfs_i2c

/* Is MSS I2C module 'resets' line property defined */
#define MSS_I2C_RESET_ENABLED DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)

#if MSS_I2C_RESET_ENABLED
#include <zephyr/drivers/reset.h>
#endif

#define CORE_I2C_CTRL      (0x00)
#define CORE_I2C_STATUS    (0x04)
#define CORE_I2C_DATA      (0x08)
#define CORE_I2C_ADDR_0    (0x0C)
#define CORE_I2C_FREQ      (0x14)
#define CORE_I2C_GLITCHREG (0x18)
#define CORE_I2C_ADDR_1    (0x1C)

#define CTRL_CR0  BIT(0)
#define CTRL_CR1  BIT(1)
#define CTRL_AA   BIT(2)
#define CTRL_SI   BIT(3)
#define CTRL_STO  BIT(4)
#define CTRL_STA  BIT(5)
#define CTRL_ENS1 BIT(6)
#define CTRL_CR2  BIT(7)

#define STATUS_BUS_ERROR                     (0x00)
#define STATUS_M_START_SENT                  (0x08)
#define STATUS_M_REPEATED_START_SENT         (0x10)
#define STATUS_M_SLAW_ACK                    (0x18)
#define STATUS_M_SLAW_NACK                   (0x20)
#define STATUS_M_TX_DATA_ACK                 (0x28)
#define STATUS_M_TX_DATA_NACK                (0x30)
#define STATUS_M_ARB_LOST                    (0x38)
#define STATUS_M_SLAR_ACK                    (0x40)
#define STATUS_M_SLAR_NACK                   (0x48)
#define STATUS_M_RX_DATA_ACKED               (0x50)
#define STATUS_M_RX_DATA_NACKED              (0x58)
#define STATUS_S_SLAW_ACKED                  (0x60)
#define STATUS_S_ARB_LOST_SLAW_ACKED         (0x68)
#define STATUS_S_GENERAL_CALL_ACKED          (0x70)
#define STATUS_S_ARB_LOST_GENERAL_CALL_ACKED (0x78)
#define STATUS_S_RX_DATA_ACKED               (0x80)
#define STATUS_S_RX_DATA_NACKED              (0x88)
#define STATUS_S_GENERAL_CALL_RX_DATA_ACKED  (0x90)
#define STATUS_S_GENERAL_CALL_RX_DATA_NACKED (0x98)
#define STATUS_S_RX_STOP                     (0xA0)
#define STATUS_S_SLAR_ACKED                  (0xA8)
#define STATUS_S_ARB_LOST_SLAR_ACKED         (0xB0)
#define STATUS_S_TX_DATA_ACK                 (0xB8)
#define STATUS_S_TX_DATA_NACK                (0xC0)
#define STATUS_LAST_DATA_ACK                 (0xC8)
#define STATUS_NO_INFO                       (0xF8)

#define PCLK_DIV_960 (CTRL_CR2)
#define PCLK_DIV_256 (0)
#define PCLK_DIV_224 (CTRL_CR0)
#define PCLK_DIV_192 (CTRL_CR1)
#define PCLK_DIV_160 (CTRL_CR0 | CTRL_CR1)
#define PCLK_DIV_120 (CTRL_CR0 | CTRL_CR2)
#define PCLK_DIV_60  (CTRL_CR1 | CTRL_CR2)
#define BCLK_DIV_8   (CTRL_CR0 | CTRL_CR1 | CTRL_CR2)
#define CLK_MASK     (CTRL_CR0 | CTRL_CR1 | CTRL_CR2)

struct mss_i2c_config {
	uint32_t clock_freq;
	uintptr_t i2c_base_addr;
	uint32_t i2c_irq_base;
#if MSS_I2C_RESET_ENABLED
	struct reset_dt_spec reset_spec;
#endif
};

struct mss_i2c_data {
	struct k_mutex mtx;
	struct k_sem done;
	const struct i2c_msg *msg_curr;
	const struct i2c_msg *msg_last;
	uint8_t *byte_curr;
	const uint8_t *byte_end;
	uint8_t addr;
	int ret;
};

static void mss_i2c_reset(const struct mss_i2c_config *cfg)
{
	/* Disable the module */
	uint8_t ctrl = sys_read8(cfg->i2c_base_addr + CORE_I2C_CTRL);

	ctrl &= ~CTRL_ENS1;
	sys_write8(ctrl, cfg->i2c_base_addr + CORE_I2C_CTRL);

	/* Make sure the write completed */
	ctrl = sys_read8(cfg->i2c_base_addr + CORE_I2C_CTRL);

	/* Enable the module */
	ctrl &= ~(CTRL_AA | CTRL_SI | CTRL_STA | CTRL_STO);
	ctrl |= CTRL_ENS1;
	sys_write8(ctrl, cfg->i2c_base_addr + CORE_I2C_CTRL);
}

static int mss_i2c_configure(const struct device *dev, uint32_t dev_config)
{
	const struct mss_i2c_config *cfg = dev->config;
	uint8_t ctrl = sys_read8(cfg->i2c_base_addr + CORE_I2C_CTRL);

	ctrl &= ~CLK_MASK;

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		ctrl |= PCLK_DIV_960;
		break;
	case I2C_SPEED_FAST:
		ctrl |= PCLK_DIV_256;
		break;
	default:
		return -EINVAL;
	}

	sys_write8(ctrl, cfg->i2c_base_addr + CORE_I2C_CTRL);
	return 0;
}

static uint8_t *mss_i2c_set_byte_end(struct mss_i2c_data *data, const struct i2c_msg *msg)
{
	uint8_t *byte_curr = msg->buf;

	data->byte_end = byte_curr + msg->len;
	return byte_curr;
}

static int mss_i2c_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			    uint16_t addr)
{
	const struct mss_i2c_config *cfg = dev->config;
	struct mss_i2c_data *data = dev->data;

	/*
	 * Check for validity of all messages, to prevent having to abort in
	 * the middle of a transfer.
	 */

	struct i2c_msg *curr = msgs;

	if (curr->len == 0) {
		/*
		 * There are potential issues with zero length buffers.  For example, zero length
		 * transfers seem to prevent that a following restart or stop condition is
		 * generated.  It is unclear if this is a hardware or driver issue.
		 *
		 * Independent of potential hardware issues, the driver definitely does not support
		 * zero length continuation buffers.  They would complicate the message handling.
		 */
		return -EINVAL;
	}

	for (int i = 1; i < num_msgs; ++i) {
		struct i2c_msg *next = curr + 1;

		if (next->len == 0) {
			/* Zero length buffers are not supported, see above */
			return -EINVAL;
		}

		if ((curr->flags & I2C_MSG_STOP) != 0) {
			/* Stop condition is only allowed on last message */
			return -EINVAL;
		}

		if ((curr->flags & I2C_MSG_RW_MASK) == (next->flags & I2C_MSG_RW_MASK)) {
			if ((next->flags & I2C_MSG_RESTART) != 0) {
				/*
				 * Restart condition between messages of the same direction
				 * is not supported.
				 */
				return -EINVAL;
			}
		} else if ((next->flags & I2C_MSG_RESTART) == 0) {
			/*
			 * Restart condition between messages of different directions
			 * is required.
			 */
			return -EINVAL;
		}

		curr = next;
	}

	/* Add RW bit to address, so that we can write it directly to the data register */
	addr <<= 1;
	if ((msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
		addr |= 1;
	}

	(void)k_mutex_lock(&data->mtx, K_FOREVER);

	data->ret = 0;
	data->addr = (uint8_t)addr;
	data->msg_curr = msgs;
	data->msg_last = msgs + num_msgs - 1;
	data->byte_curr = mss_i2c_set_byte_end(data, msgs);

	/* Start the transfer */
	uint8_t ctrl = sys_read8(cfg->i2c_base_addr + CORE_I2C_CTRL);

	ctrl &= ~(CTRL_AA | CTRL_SI | CTRL_STO);
	ctrl |= CTRL_STA;
	sys_write8(ctrl, cfg->i2c_base_addr + CORE_I2C_CTRL);

	/*
	 * Clear a potentially erroneous done condition caused by a spurious interrupt.  Enable
	 * interrupts and wait for the transfer completion.
	 */
	k_sem_reset(&data->done);
	irq_enable(cfg->i2c_irq_base);
	int ret = k_sem_take(&data->done, K_TICKS(1000));

	irq_disable(cfg->i2c_irq_base);

	if (unlikely(ret != 0)) {
		/*
		 * In case of a timeout, reset the module.  This could be caused by an SCL line held
		 * low.
		 */
		mss_i2c_reset(cfg);
	} else {
		ret = data->ret;
	}

	(void)k_mutex_unlock(&data->mtx);
	return ret;
}

static DEVICE_API(i2c, mss_i2c_driver_api) = {
	.configure = mss_i2c_configure,
	.transfer = mss_i2c_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

static void mss_i2c_init(const struct device *dev)
{
	const struct mss_i2c_config *cfg = dev->config;
	struct mss_i2c_data *data = dev->data;

	(void)k_mutex_init(&data->mtx);
	(void)k_sem_init(&data->done, 0, 1);

#if MSS_I2C_RESET_ENABLED
	if (cfg->reset_spec.dev != NULL) {
		(void)reset_line_deassert_dt(&cfg->reset_spec);
	}
#endif

	mss_i2c_reset(cfg);
}

static inline bool mss_i2c_is_last_write_byte(struct mss_i2c_data *data, const struct i2c_msg *msg)
{
	return msg == data->msg_last || ((msg + 1)->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
}

static inline bool mss_i2c_is_last_read_byte(struct mss_i2c_data *data, const struct i2c_msg *msg)
{
	return msg == data->msg_last || ((msg + 1)->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE;
}

static uint8_t mss_i2c_set_ctrl_aa(struct mss_i2c_data *data, const struct i2c_msg *msg,
				   uint8_t ctrl)
{
	if (mss_i2c_is_last_read_byte(data, msg)) {
		return ctrl;
	}

	return ctrl | CTRL_AA;
}

static void mss_i2c_irq_handler(const struct device *dev)
{
	const struct mss_i2c_config *cfg = dev->config;
	struct mss_i2c_data *data = dev->data;
	uintptr_t i2c_base_addr = cfg->i2c_base_addr;
	const struct i2c_msg *msg_curr = data->msg_curr;
	uint8_t *byte_curr = data->byte_curr;
	uint8_t ctrl = sys_read8(i2c_base_addr + CORE_I2C_CTRL);
	bool done = false;

	do {
		uint8_t status = sys_read8(i2c_base_addr + CORE_I2C_STATUS);

		if (status == STATUS_NO_INFO) {
			break;
		}

		ctrl &= ~(CTRL_AA | CTRL_STA | CTRL_STO);

		switch (status) {
		case STATUS_M_START_SENT:
		case STATUS_M_REPEATED_START_SENT:
			sys_write8(ctrl, i2c_base_addr + CORE_I2C_CTRL);
			sys_write8(data->addr, i2c_base_addr + CORE_I2C_DATA);
			break;
		case STATUS_M_TX_DATA_NACK:
			if (byte_curr != data->byte_end &&
			    !mss_i2c_is_last_write_byte(data, msg_curr)) {
				/*
				 * A not acknowledged write is only acceptable for the last byte
				 * written.
				 */
				data->ret = -EIO;
				done = true;
			}

			__fallthrough;
		case STATUS_M_SLAW_ACK:
		case STATUS_M_TX_DATA_ACK:
			if (byte_curr != data->byte_end) {
				sys_write8(*byte_curr, i2c_base_addr + CORE_I2C_DATA);
				++byte_curr;
			} else if (msg_curr == data->msg_last) {
				ctrl |= CTRL_STO;
				done = true;
			} else {
				++msg_curr;
				byte_curr = mss_i2c_set_byte_end(data, msg_curr);

				if ((msg_curr->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
					/* Direction change with repeated start */
					ctrl |= CTRL_STA;
					data->addr |= 1;
				} else {
					/*
					 * Continue write with the new buffer.  The message check in
					 * mss_i2c_transfer() ensures that this is a non-zero length
					 * buffer.
					 */
					sys_write8(*byte_curr, i2c_base_addr + CORE_I2C_DATA);
					++byte_curr;
				}
			}

			break;
		case STATUS_M_RX_DATA_ACKED:
		case STATUS_M_RX_DATA_NACKED:
			if (byte_curr != data->byte_end) {
				*byte_curr = sys_read8(cfg->i2c_base_addr + CORE_I2C_DATA);
				++byte_curr;
			} else {
				/* This is an error and should not happen */
				data->ret = -EIO;
				done = true;
			}

			__fallthrough;
		case STATUS_M_SLAR_ACK:
			if (byte_curr + 1 == data->byte_end) {
				ctrl = mss_i2c_set_ctrl_aa(data, msg_curr, ctrl);
			} else if (byte_curr != data->byte_end) {
				ctrl |= CTRL_AA;
			} else if (msg_curr == data->msg_last) {
				ctrl |= CTRL_STO;
				done = true;
			} else {
				++msg_curr;
				byte_curr = mss_i2c_set_byte_end(data, msg_curr);

				if ((msg_curr->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
					/* Direction change with repeated start */
					ctrl |= CTRL_STA;
					data->addr &= ~1;
				} else if (byte_curr + 1 == data->byte_end) {
					ctrl = mss_i2c_set_ctrl_aa(data, msg_curr, ctrl);
				} else {
					ctrl |= CTRL_AA;
				}
			}

			break;
		default:
			ctrl |= CTRL_STO;
			data->ret = -EIO;
			done = true;
			break;
		}

		ctrl &= ~CTRL_SI;
		sys_write8(ctrl, i2c_base_addr + CORE_I2C_CTRL);
	} while (!done);

	data->msg_curr = msg_curr;
	data->byte_curr = byte_curr;

	if (done) {
		k_sem_give(&data->done);
	}
}

#define MSS_I2C_INIT(n)                                                                            \
	static int mss_i2c_init_##n(const struct device *dev)                                      \
	{                                                                                          \
		mss_i2c_init(dev);                                                                 \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mss_i2c_irq_handler,        \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	static struct mss_i2c_data mss_i2c_data_##n;                                               \
                                                                                                   \
	static const struct mss_i2c_config mss_i2c_config_##n = {                                  \
		.i2c_base_addr = DT_INST_REG_ADDR(n),                                              \
		.i2c_irq_base = DT_INST_IRQN(n),                                                   \
		.clock_freq = DT_INST_PROP(n, clock_frequency),                                    \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, resets),                                       \
			(.reset_spec = RESET_DT_SPEC_INST_GET(n),))                                \
	};                                                                                         \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(n, mss_i2c_init_##n, NULL, &mss_i2c_data_##n,                    \
				  &mss_i2c_config_##n, PRE_KERNEL_1, CONFIG_I2C_INIT_PRIORITY,     \
				  &mss_i2c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSS_I2C_INIT)
