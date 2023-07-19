/*
 * Copyright (c) 2023 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/barrier.h>
LOG_MODULE_REGISTER(i2c_mchp, CONFIG_I2C_LOG_LEVEL);

#define DT_DRV_COMPAT microchip_mpfs_i2c

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

#define PCLK_DIV_960 (CTRL_CR2)
#define PCLK_DIV_256 (0)
#define PCLK_DIV_224 (CTRL_CR0)
#define PCLK_DIV_192 (CTRL_CR1)
#define PCLK_DIV_160 (CTRL_CR0 | CTRL_CR1)
#define PCLK_DIV_120 (CTRL_CR0 | CTRL_CR2)
#define PCLK_DIV_60  (CTRL_CR1 | CTRL_CR2)
#define BCLK_DIV_8   (CTRL_CR0 | CTRL_CR1 | CTRL_CR2)
#define CLK_MASK     (CTRL_CR0 | CTRL_CR1 | CTRL_CR2)

/* -- Transactions types -- */
#define NO_TRANSACTION                     (0x00)
#define CONTROLLER_WRITE_TRANSACTION       (0x01)
#define CONTROLLER_READ_TRANSACTION        (0x02)
#define CONTROLLER_RANDOM_READ_TRANSACTION (0x03)
#define WRITE_TARGET_TRANSACTION           (0x04)
#define READ_TARGET_TRANSACTION            (0x05)

#define MSS_I2C_RELEASE_BUS (0x00)
#define MSS_I2C_HOLD_BUS    (0x01)
#define TARGET_ADDR_SHIFT   (0x01)

#define MSS_I2C_SUCCESS     (0x00)
#define MSS_I2C_IN_PROGRESS (0x01)
#define MSS_I2C_FAILED      (0x02)
#define MSS_I2C_TIMED_OUT   (0x03)

struct mss_i2c_config {
	uint32_t clock_freq;
	uintptr_t i2c_base_addr;
	uint32_t i2c_irq_base;
};

struct mss_i2c_data {
	uint8_t ser_address;
	uint8_t target_addr;
	uint8_t options;
	uint8_t transaction;
	const uint8_t *controller_tx_buffer;
	uint16_t controller_tx_size;
	uint16_t controller_tx_idx;
	uint8_t dir;
	uint8_t *controller_rx_buffer;
	uint16_t controller_rx_size;
	uint16_t controller_rx_idx;
	atomic_t controller_status;
	uint32_t controller_timeout_ms;
	const uint8_t *target_tx_buffer;
	uint16_t target_tx_size;
	uint16_t target_tx_idx;
	uint8_t *target_rx_buffer;
	uint16_t target_rx_size;
	uint16_t target_rx_idx;
	atomic_t target_status;
	uint8_t target_mem_offset_length;
	uint8_t is_target_enabled;
	uint8_t bus_status;
	uint8_t is_transaction_pending;
	uint8_t pending_transaction;

	sys_slist_t cb;
};


static int mss_i2c_configure(const struct device *dev, uint32_t dev_config_raw)
{
	const struct mss_i2c_config *cfg = dev->config;

	uint8_t ctrl = sys_read8(cfg->i2c_base_addr + CORE_I2C_CTRL);

	switch (I2C_SPEED_GET(dev_config_raw)) {
	case I2C_SPEED_STANDARD:
		sys_write8((ctrl | PCLK_DIV_960), cfg->i2c_base_addr + CORE_I2C_CTRL);
		break;
	case I2C_SPEED_FAST:
		sys_write8((ctrl | PCLK_DIV_256), cfg->i2c_base_addr + CORE_I2C_CTRL);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mss_wait_complete(const struct device *dev)
{
	struct mss_i2c_data *const data = dev->data;
	atomic_t i2c_status = 0;

	do {
		i2c_status = atomic_get(&data->controller_status);
	} while (i2c_status == MSS_I2C_IN_PROGRESS);

	return i2c_status;
}

static int mss_i2c_read(const struct device *dev, uint8_t serial_addr, uint8_t *read_buffer,
			uint32_t read_size)
{
	struct mss_i2c_data *const data = dev->data;
	const struct mss_i2c_config *cfg = dev->config;

	uint8_t ctrl = sys_read8(cfg->i2c_base_addr + CORE_I2C_CTRL);

	data->target_addr = serial_addr << TARGET_ADDR_SHIFT;
	data->pending_transaction = CONTROLLER_READ_TRANSACTION;
	data->dir = I2C_MSG_READ;
	data->controller_rx_buffer = read_buffer;
	data->controller_rx_size = read_size;
	data->controller_rx_idx = 0u;

	sys_write8((ctrl | CTRL_STA), cfg->i2c_base_addr + CORE_I2C_CTRL);

	return 0;
}

static int mss_i2c_write(const struct device *dev, uint8_t serial_addr, uint8_t *tx_buffer,
			 uint32_t tx_num_write)
{
	struct mss_i2c_data *const data = dev->data;
	const struct mss_i2c_config *cfg = dev->config;
	uint8_t ctrl = sys_read8(cfg->i2c_base_addr + CORE_I2C_CTRL);

	atomic_t target_status = data->target_status;

	if (data->transaction == NO_TRANSACTION) {
		data->transaction = CONTROLLER_WRITE_TRANSACTION;
	}

	data->pending_transaction = CONTROLLER_WRITE_TRANSACTION;
	data->target_addr = serial_addr << TARGET_ADDR_SHIFT;
	data->dir = I2C_MSG_WRITE;
	data->controller_tx_buffer = tx_buffer;
	data->controller_tx_size = tx_num_write;
	data->controller_tx_idx = 0u;
	atomic_set(&data->controller_status, MSS_I2C_IN_PROGRESS);

	if (target_status == MSS_I2C_IN_PROGRESS) {
		data->is_transaction_pending = CONTROLLER_WRITE_TRANSACTION;
	} else {
		sys_write8((ctrl | CTRL_STA), cfg->i2c_base_addr + CORE_I2C_CTRL);
	}

	if (data->bus_status == MSS_I2C_HOLD_BUS) {
		sys_write8((ctrl & ~CTRL_SI), cfg->i2c_base_addr + CORE_I2C_CTRL);
	}

	return 0;
}


static int mss_i2c_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			    uint16_t addr)
{
	for (int i = 0; i < num_msgs; i++) {
		struct i2c_msg *current = &msgs[i];

		if ((current->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			mss_i2c_read(dev, addr, current->buf, current->len);
			mss_wait_complete(dev);
		} else {
			mss_i2c_write(dev, addr, current->buf, current->len);
			mss_wait_complete(dev);
		}
	}

	return 0;
}

static const struct i2c_driver_api mss_i2c_driver_api = {
	.configure = mss_i2c_configure,
	.transfer = mss_i2c_transfer,
};

static void mss_i2c_reset(const struct device *dev)
{
	const struct mss_i2c_config *cfg = dev->config;
	uint8_t ctrl = sys_read8(cfg->i2c_base_addr + CORE_I2C_CTRL);

	sys_write8((ctrl & ~CTRL_ENS1), cfg->i2c_base_addr + CORE_I2C_CTRL);

	ctrl = sys_read8(cfg->i2c_base_addr + CORE_I2C_CTRL);

	sys_write8((ctrl | CTRL_ENS1), cfg->i2c_base_addr + CORE_I2C_CTRL);
}


static void mss_i2c_irq_handler(const struct device *dev)
{
	struct mss_i2c_data *const data = dev->data;
	const struct mss_i2c_config *cfg = dev->config;

	uint8_t ctrl = sys_read8(cfg->i2c_base_addr + CORE_I2C_CTRL);

	uint8_t status = sys_read8(cfg->i2c_base_addr + CORE_I2C_STATUS);

	uint8_t hold_bus = 0;

	switch (status) {
	case STATUS_M_START_SENT:
	case STATUS_M_REPEATED_START_SENT:
		sys_write8((ctrl & ~CTRL_STA), cfg->i2c_base_addr + CORE_I2C_CTRL);

		sys_write8(data->target_addr | data->dir, cfg->i2c_base_addr + CORE_I2C_DATA);

		data->controller_tx_idx = 0;
		data->controller_rx_idx = 0;

		data->is_transaction_pending = false;
		data->transaction = data->pending_transaction;
		break;
	case STATUS_M_ARB_LOST:
		sys_write8((ctrl | CTRL_STA), cfg->i2c_base_addr + CORE_I2C_CTRL);
		LOG_WRN("lost arbitration: %x\n", status);
		break;
	case STATUS_M_SLAW_ACK:
	case STATUS_M_TX_DATA_ACK:
		if (data->controller_tx_idx < data->controller_tx_size) {
			sys_write8(data->controller_tx_buffer[data->controller_tx_idx],
				   cfg->i2c_base_addr + CORE_I2C_DATA);

			data->controller_tx_idx++;

		} else if (data->transaction == CONTROLLER_RANDOM_READ_TRANSACTION) {
			data->dir = I2C_MSG_READ;
			sys_write8((ctrl | CTRL_STA), cfg->i2c_base_addr + CORE_I2C_CTRL);

		} else {
			data->transaction = NO_TRANSACTION;
			hold_bus = data->options & MSS_I2C_HOLD_BUS;
			data->bus_status = hold_bus;

			if (hold_bus == MSS_I2C_RELEASE_BUS) {
				sys_write8((ctrl | CTRL_STO), cfg->i2c_base_addr + CORE_I2C_CTRL);
			}
		}
		atomic_set(&data->controller_status, MSS_I2C_SUCCESS);
		break;
	case STATUS_M_TX_DATA_NACK:
	case STATUS_M_SLAR_NACK:
	case STATUS_M_SLAW_NACK:
		sys_write8((ctrl | CTRL_STO), cfg->i2c_base_addr + CORE_I2C_CTRL);
		atomic_set(&data->controller_status, MSS_I2C_FAILED);
		data->transaction = NO_TRANSACTION;
		break;
	case STATUS_M_SLAR_ACK:
		if (data->controller_rx_size > 1u) {
			sys_write8((ctrl | CTRL_AA), cfg->i2c_base_addr + CORE_I2C_CTRL);

		} else if (data->controller_rx_size == 1u) {
			sys_write8((ctrl & ~CTRL_AA), cfg->i2c_base_addr + CORE_I2C_CTRL);

		} else {
			sys_write8((ctrl | CTRL_AA | CTRL_STO), cfg->i2c_base_addr + CORE_I2C_CTRL);
			atomic_set(&data->controller_status, MSS_I2C_SUCCESS);
			data->transaction = NO_TRANSACTION;
		}
		break;
	case STATUS_M_RX_DATA_ACKED:
		data->controller_rx_buffer[data->controller_rx_idx] =
			sys_read8(cfg->i2c_base_addr + CORE_I2C_DATA);

		data->controller_rx_idx++;

		/* Second Last byte */
		if (data->controller_rx_idx >= (data->controller_rx_size - 1u)) {
			sys_write8((ctrl & ~CTRL_AA), cfg->i2c_base_addr + CORE_I2C_CTRL);
		} else {
			atomic_set(&data->controller_status, MSS_I2C_IN_PROGRESS);
		}
		break;
	case STATUS_M_RX_DATA_NACKED:

		data->controller_rx_buffer[data->controller_rx_idx] =
			sys_read8(cfg->i2c_base_addr + CORE_I2C_DATA);

		hold_bus = data->options & MSS_I2C_HOLD_BUS;
		data->bus_status = hold_bus;

		if (hold_bus == 0u) {
			sys_write8((ctrl | CTRL_STO), cfg->i2c_base_addr + CORE_I2C_CTRL);
		}

		data->transaction = NO_TRANSACTION;
		atomic_set(&data->controller_status, MSS_I2C_SUCCESS);
		break;
	default:
		break;
	}

	ctrl = sys_read8(cfg->i2c_base_addr + CORE_I2C_CTRL);

	sys_write8((ctrl & ~CTRL_SI), cfg->i2c_base_addr + CORE_I2C_CTRL);
}

#define MSS_I2C_INIT(n)                                                                            \
	static int mss_i2c_init_##n(const struct device *dev)                                      \
	{                                                                                          \
		mss_i2c_reset(dev);                                                                \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mss_i2c_irq_handler,        \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	static struct mss_i2c_data mss_i2c_data_##n;                                               \
                                                                                                   \
	static const struct mss_i2c_config mss_i2c_config_##n = {                                  \
		.i2c_base_addr = DT_INST_REG_ADDR(n),                                              \
		.i2c_irq_base = DT_INST_IRQN(n),                                                   \
		.clock_freq = DT_INST_PROP(n, clock_frequency),                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mss_i2c_init_##n, NULL, &mss_i2c_data_##n, &mss_i2c_config_##n,   \
			      PRE_KERNEL_1, CONFIG_I2C_INIT_PRIORITY, &mss_i2c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSS_I2C_INIT)
