/*
 * Copyright (c) 2024 CogniPilot Foundation
 * Copyright (c) 2024 NXP Semiconductors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT futaba_sbus

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/input/input.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/uart.h>

LOG_MODULE_REGISTER(futaba_sbus, CONFIG_INPUT_LOG_LEVEL);

/* Driver config */
struct sbus_input_channel {
	uint32_t sbus_channel;
	uint32_t type;
	uint32_t zephyr_code;
};

const struct uart_config uart_cfg_sbus = {
	.baudrate = 100000,
	.parity = UART_CFG_PARITY_EVEN,
	.stop_bits = UART_CFG_STOP_BITS_2,
	.data_bits = UART_CFG_DATA_BITS_8,
	.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
};

struct input_sbus_config {
	uint8_t num_channels;
	const struct sbus_input_channel *channel_info;
	const struct device *uart_dev;
	uart_irq_callback_user_data_t cb;
};

#define SBUS_FRAME_LEN 25
#define SBUS_HEADER    0x0F
#define SBUS_FOOTER    0x00

#define SBUS_SERVO_LEN     22
#define SBUS_SERVO_CH_MASK 0x7FF

#define SBUS_BYTE24_IDX        23
#define SBUS_BYTE24_CH17       0x01
#define SBUS_BYTE24_CH18       0x02
#define SBUS_BYTE24_FRAME_LOST 0x04
#define SBUS_BYTE24_FAILSAFE   0x08

#define SBUS_TRANSMISSION_TIME_MS  4  /* Max transmission of a single SBUS frame */
#define SBUS_INTERFRAME_SPACING_MS 20 /* Max spacing between SBUS frames */
#define SBUS_CHANNEL_COUNT         16

#define REPORT_FILTER      CONFIG_INPUT_SBUS_REPORT_FILTER
#define CHANNEL_VALUE_ZERO CONFIG_INPUT_SBUS_CHANNEL_VALUE_ZERO
#define CHANNEL_VALUE_ONE  CONFIG_INPUT_SBUS_CHANNEL_VALUE_ONE

struct input_sbus_data {
	struct k_thread thread;
	struct k_sem report_lock;

	uint16_t xfer_bytes;
	uint8_t rd_data[SBUS_FRAME_LEN];
	uint8_t sbus_frame[SBUS_FRAME_LEN];
	bool partial_sync;
	bool in_sync;
	uint32_t last_rx_time;

	uint16_t last_reported_value[SBUS_CHANNEL_COUNT];
	int8_t channel_mapping[SBUS_CHANNEL_COUNT];

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_INPUT_SBUS_THREAD_STACK_SIZE);
};

static void input_sbus_report(const struct device *dev, unsigned int sbus_channel,
			      unsigned int value)
{
	const struct input_sbus_config *const config = dev->config;
	struct input_sbus_data *const data = dev->data;

	int channel = data->channel_mapping[sbus_channel];

	/* Not Mapped */
	if (channel == -1) {
		return;
	}

	if (value >= (data->last_reported_value[channel] + REPORT_FILTER) ||
	    value <= (data->last_reported_value[channel] - REPORT_FILTER)) {
		switch (config->channel_info[channel].type) {
		case INPUT_EV_ABS:
		case INPUT_EV_MSC:
			input_report(dev, config->channel_info[channel].type,
				     config->channel_info[channel].zephyr_code, value, false,
				     K_FOREVER);
			break;

		default:
			if (value > CHANNEL_VALUE_ONE) {
				input_report_key(dev, config->channel_info[channel].zephyr_code, 1,
						 false, K_FOREVER);
			} else if (value < CHANNEL_VALUE_ZERO) {
				input_report_key(dev, config->channel_info[channel].zephyr_code, 0,
						 false, K_FOREVER);
			}
		}
		data->last_reported_value[channel] = value;
	}
}

static void input_sbus_input_report_thread(const struct device *dev, void *dummy2, void *dummy3)
{
	struct input_sbus_data *const data = dev->data;

	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	uint8_t i, channel;
	uint8_t *sbus_channel_data = &data->sbus_frame[1]; /* Omit header */
	uint16_t value;
	int bits_read;
	unsigned int key;
	int ret;
	bool connected_reported = false;

	while (true) {
		if (!data->in_sync) {
			k_sem_take(&data->report_lock, K_FOREVER);
			if (data->in_sync) {
				LOG_DBG("SBUS receiver connected");
			} else {
				continue;
			}
		} else {
			ret = k_sem_take(&data->report_lock, K_MSEC(SBUS_INTERFRAME_SPACING_MS));
			if (ret == -EBUSY) {
				continue;
			} else if (ret < 0 || !data->in_sync) {
				/* We've lost sync with the UART receiver */
				key = irq_lock();

				data->partial_sync = false;
				data->in_sync = false;
				data->xfer_bytes = 0;
				irq_unlock(key);

				connected_reported = false;
				LOG_DBG("SBUS receiver connection lost");

				/* Report connection lost */
				continue;
			}
		}

		if (connected_reported &&
		    data->sbus_frame[SBUS_BYTE24_IDX] & SBUS_BYTE24_FRAME_LOST) {
			LOG_DBG("SBUS controller connection lost");
			connected_reported = false;
		} else if (!connected_reported &&
			   !(data->sbus_frame[SBUS_BYTE24_IDX] & SBUS_BYTE24_FRAME_LOST)) {
			LOG_DBG("SBUS controller connected");
			connected_reported = true;
		}

		/* Parse the data */
		channel = 0;
		value = 0;
		bits_read = 0;

		for (i = 0; i < SBUS_SERVO_LEN; i++) {
			/* Read the next byte */
			unsigned char byte = sbus_channel_data[i];

			/* Extract bits and construct the 11-bit value */
			value |= byte << bits_read;
			bits_read += 8;

			/* Check if we've read enough bits to form a full 11-bit value */
			while (bits_read >= 11) {
				input_sbus_report(dev, channel, value & SBUS_SERVO_CH_MASK);

				/* Shift right to prepare for the next 11 bits */
				value >>= 11;
				bits_read -= 11;
				channel++;
			}
		}

#ifdef CONFIG_INPUT_SBUS_SEND_SYNC
		input_report(dev, 0, 0, 0, true, K_FOREVER);
#endif
	}
}

static void sbus_resync(const struct device *uart_dev, struct input_sbus_data *const data)
{
	uint8_t *rd_data = data->rd_data;

	if (data->partial_sync) {
		data->xfer_bytes += uart_fifo_read(uart_dev, &rd_data[data->xfer_bytes],
						   SBUS_FRAME_LEN - data->xfer_bytes);
		if (data->xfer_bytes == SBUS_FRAME_LEN) {
			/* Transfer took longer then 4ms probably faulty */
			if (k_uptime_get_32() - data->last_rx_time > SBUS_TRANSMISSION_TIME_MS) {
				data->xfer_bytes = 0;
				data->partial_sync = false;
			} else if (rd_data[0] == SBUS_HEADER &&
				   rd_data[SBUS_FRAME_LEN - 1] == SBUS_FOOTER) {
				data->in_sync = true;
			} else {
				/* Dummy read to clear fifo */
				uart_fifo_read(uart_dev, &rd_data[0], 1);
				data->xfer_bytes = 0;
				data->partial_sync = false;
			}
		}
	} else {
		if (uart_fifo_read(uart_dev, &rd_data[0], 1) == 1) {
			if (rd_data[0] == SBUS_HEADER) {
				data->partial_sync = true;
				data->xfer_bytes = 1;
				data->last_rx_time = k_uptime_get_32();
			}
		}
	}
}

static void sbus_uart_isr(const struct device *uart_dev, void *user_data)
{
	const struct device *dev = user_data;
	struct input_sbus_data *const data = dev->data;
	uint8_t *rd_data = data->rd_data;

	if (uart_dev == NULL) {
		LOG_DBG("UART device is NULL");
		return;
	}

	if (!uart_irq_update(uart_dev)) {
		LOG_DBG("Unable to start processing interrupts");
		return;
	}

	while (uart_irq_rx_ready(uart_dev) && data->xfer_bytes <= SBUS_FRAME_LEN) {
		if (data->in_sync) {
			if (data->xfer_bytes == 0) {
				data->last_rx_time = k_uptime_get_32();
			}
			data->xfer_bytes += uart_fifo_read(uart_dev, &rd_data[data->xfer_bytes],
							   SBUS_FRAME_LEN - data->xfer_bytes);
		} else {
			sbus_resync(uart_dev, data);
		}
	}

	if (data->in_sync && (k_uptime_get_32() - data->last_rx_time >
	    SBUS_INTERFRAME_SPACING_MS)) {
		data->partial_sync = false;
		data->in_sync = false;
		data->xfer_bytes = 0;
		k_sem_give(&data->report_lock);
	} else if (data->in_sync && data->xfer_bytes == SBUS_FRAME_LEN) {
		data->xfer_bytes = 0;

		if (rd_data[0] == SBUS_HEADER && rd_data[SBUS_FRAME_LEN - 1] == SBUS_FOOTER) {
			memcpy(data->sbus_frame, rd_data, SBUS_FRAME_LEN);
			k_sem_give(&data->report_lock);
		} else {
			data->partial_sync = false;
			data->in_sync = false;
		}
	}
}

/*
 * @brief Initialize sbus driver
 */
static int input_sbus_init(const struct device *dev)
{
	const struct input_sbus_config *const config = dev->config;
	struct input_sbus_data *const data = dev->data;
	int i, ret;

	uart_irq_rx_disable(config->uart_dev);
	uart_irq_tx_disable(config->uart_dev);

	LOG_DBG("Initializing SBUS driver");

	for (i = 0; i < SBUS_CHANNEL_COUNT; i++) {
		data->last_reported_value[i] = 0;
		data->channel_mapping[i] = -1;
	}

	data->xfer_bytes = 0;
	data->in_sync = false;
	data->partial_sync = false;
	data->last_rx_time = 0;

	for (i = 0; i < config->num_channels; i++) {
		data->channel_mapping[config->channel_info[i].sbus_channel - 1] = i;
	}

	ret = uart_configure(config->uart_dev, &uart_cfg_sbus);
	if (ret < 0) {
		LOG_ERR("Unable to configure UART port: %d", ret);
		return ret;
	}

	ret = uart_irq_callback_user_data_set(config->uart_dev, config->cb, (void *)dev);
	if (ret < 0) {
		if (ret == -ENOTSUP) {
			LOG_ERR("Interrupt-driven UART API support not enabled");
		} else if (ret == -ENOSYS) {
			LOG_ERR("UART device does not support interrupt-driven API");
		} else {
			LOG_ERR("Error setting UART callback: %d", ret);
		}
		return ret;
	}

	uart_irq_rx_enable(config->uart_dev);

	k_sem_init(&data->report_lock, 0, 1);

	k_thread_create(&data->thread, data->thread_stack,
			K_KERNEL_STACK_SIZEOF(data->thread_stack),
			(k_thread_entry_t)input_sbus_input_report_thread, (void *)dev, NULL, NULL,
			CONFIG_INPUT_SBUS_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(&data->thread, dev->name);

	return ret;
}

#define INPUT_CHANNEL_CHECK(input_channel_id)                                                      \
	BUILD_ASSERT(IN_RANGE(DT_PROP(input_channel_id, channel), 1, 16),                          \
		     "invalid channel number");                                                    \
	BUILD_ASSERT(DT_PROP(input_channel_id, type) == INPUT_EV_ABS ||                            \
			     DT_PROP(input_channel_id, type) == INPUT_EV_KEY ||                    \
			     DT_PROP(input_channel_id, type) == INPUT_EV_MSC,                      \
		     "invalid channel type");

#define SBUS_INPUT_CHANNEL_INITIALIZER(input_channel_id)                                           \
	{                                                                                          \
		.sbus_channel = DT_PROP(input_channel_id, channel),                                \
		.type = DT_PROP(input_channel_id, type),                                           \
		.zephyr_code = DT_PROP(input_channel_id, zephyr_code),                             \
	},

#define INPUT_SBUS_INIT(n)                                                                         \
                                                                                                   \
	static const struct sbus_input_channel input_##id[] = {                                    \
		DT_INST_FOREACH_CHILD(n, SBUS_INPUT_CHANNEL_INITIALIZER)                           \
	};                                                                                         \
	DT_INST_FOREACH_CHILD(n, INPUT_CHANNEL_CHECK)                                              \
                                                                                                   \
	static struct input_sbus_data sbus_data_##n;                                               \
                                                                                                   \
	static const struct input_sbus_config sbus_cfg_##n = {                                     \
		.channel_info = input_##id,                                                        \
		.uart_dev = DEVICE_DT_GET(DT_INST_BUS(n)),                                         \
		.num_channels = ARRAY_SIZE(input_##id),                                            \
		.cb = sbus_uart_isr,                                                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, input_sbus_init, NULL, &sbus_data_##n, &sbus_cfg_##n,             \
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INPUT_SBUS_INIT)
