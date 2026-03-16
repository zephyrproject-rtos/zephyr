/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_ch9350l

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_ch9350l, CONFIG_INPUT_LOG_LEVEL);

/* The theorical maximum is 72 */
#define CH9350L_FRAME_SIZE_MAX	72
#define CH9350L_FRAME_SIZE_MIN	8
#define CH9350L_WAIT_TIMEOUT_MS	100

#define CH9350L_READBUF_SIZE	16

#define CH9350L_FRAME_HEAD_0		0x57
#define CH9350L_FRAME_HEAD_1		0xAB
#define CH9350L_FRAME_HEAD_KEY_0	0x83
#define CH9350L_FRAME_HEAD_KEY_1	0x88
#define CH9350L_FRAME_HEAD_OFF		3
#define CH9350L_FRAME_LENGTH_OFF	4

#define CH9350L_FRAME_VALUE_MAX		(CH9350L_FRAME_SIZE_MAX - CH9350L_FRAME_HEAD_OFF - 4)

#define CH9350L_FRAME_TYPE_MASK		GENMASK(5, 4)
#define CH9350L_FRAME_TYPE_POS		4
#define CH9350L_FRAME_TYPE_OTHER	0
#define CH9350L_FRAME_TYPE_KB		1
#define CH9350L_FRAME_TYPE_MOUSE	2
#define CH9350L_FRAME_TYPE_MM		3

#define CH9350L_FRAME_MOUSE_BUTTON_BYTE	0
#define CH9350L_FRAME_MOUSE_X_BYTE	1
#define CH9350L_FRAME_MOUSE_Y_BYTE	3
#define CH9350L_FRAME_MOUSE_RELMID	0x7FFF
#define CH9350L_FRAME_MOUSE_RELNEG	0x8000

#define CH9350L_RAWMOUSE_TO_REL(_val) (_val > CH9350L_FRAME_MOUSE_RELMID ?			\
	-(CH9350L_FRAME_MOUSE_RELMID - (const int16_t)(_val - CH9350L_FRAME_MOUSE_RELNEG))	\
	: (const int16_t)_val)

#define CH9350L_FRAME_MOUSE_BTN_LEFT	0x1
#define CH9350L_FRAME_MOUSE_BTN_RIGHT	0x2
#define CH9350L_FRAME_MOUSE_BTN_MID	0x4
#define CH9350L_FRAME_MOUSE_BTN_4	0x8
#define CH9350L_FRAME_MOUSE_BTN_5	0x10
#define CH9350L_FRAME_MOUSE_BTN_6	0x20
#define CH9350L_FRAME_MOUSE_BTN_7	0x40
#define CH9350L_FRAME_MOUSE_BTN_8	0x80

static const uint8_t ch9350l_enable_status_frame[] = {
	0x57, 0xab, 0x12, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x20
};
static const uint8_t ch9350l_disable_status_frame[] = {
	0x57, 0xab, 0x12, 0x00, 0x00, 0x00, 0x00, 0xff, 0x80, 0x00, 0x20
};
static const uint8_t ch9350l_valid_start_of_status_frame[] = { 0x57, 0xab, 0x82 };

struct ch9350l_frame {
	uint8_t data[CH9350L_FRAME_SIZE_MAX - CH9350L_FRAME_HEAD_OFF];
	size_t size;
};

#define CH9350L_MSGQBUF_SIZE (CONFIG_INPUT_CH9350L_FRAME_COUNT * sizeof(struct ch9350l_frame))
#define CH9350L_MSGQBUF_TYPE char __aligned(4)

struct ch9350l_data {
	uint8_t frame_buffer[CH9350L_FRAME_SIZE_MAX];
	size_t frame_buffer_size;
	bool frame_started;
	uint8_t last_kb_values[CH9350L_FRAME_VALUE_MAX];
	uint8_t last_mouse_btns;
	const struct device *dev;
	struct k_work work;
	struct k_msgq msgq;
	CH9350L_MSGQBUF_TYPE msgq_buffer[CH9350L_MSGQBUF_SIZE];
};

struct ch9350l_config {
	const struct device *uart;
	const int *kb_codemap;
	const size_t kb_codemap_len;
	const int *mouse_codemap;
	const size_t mouse_codemap_len;
};

static uint32_t ch9350l_kb_map(const struct device *dev, uint32_t code)
{
	const struct ch9350l_config *config = dev->config;
	const size_t code_map_len = config->kb_codemap_len / 2;

	for (size_t i = 0; i < code_map_len; i++) {
		if (config->kb_codemap[i * 2] == code) {
			return config->kb_codemap[i * 2 + 1];
		}
	}

	return code;
}

static void ch9350l_kb(const struct device *dev, const uint8_t *values, uint8_t length)
{
	struct ch9350l_data *data = dev->data;
	uint8_t *lkbv = data->last_kb_values;
	bool found;

	for (size_t j = 0; j < CH9350L_FRAME_VALUE_MAX; j++) {
		found = false;
		for (size_t i = 0; i < length; i++) {
			if (lkbv[j] == values[i]) {
				found = true;
			}
		}
		if (!found && lkbv[j] != 0) {
			if (input_report(dev, INPUT_EV_KEY,
				ch9350l_kb_map(dev, lkbv[j]), 0, true, K_FOREVER)) {
				LOG_ERR("Input failed to be enqueued");
			}
		}
	}

	for (size_t i = 0; i < length; i++) {
		found = false;
		for (size_t j = 0; j < CH9350L_FRAME_VALUE_MAX; j++) {
			if (lkbv[j] == values[i]) {
				found = true;
			}
		}
		if (!found && values[i] != 0) {
			if (input_report(dev, INPUT_EV_KEY,
				ch9350l_kb_map(dev, values[i]), 1, true, K_FOREVER)) {
				LOG_ERR("Input failed to be enqueued");
			}
		}
	}

	memcpy(lkbv, values, length);
	memset(&lkbv[length], 0, CH9350L_FRAME_VALUE_MAX - length);
}

static uint32_t ch9350l_mouse_map(const struct device *dev, uint8_t code)
{
	const struct ch9350l_config *config = dev->config;
	const size_t code_map_len = config->mouse_codemap_len / 2;

	for (size_t i = 0; i < code_map_len; i++) {
		if (config->mouse_codemap[i * 2] == code) {
			return config->mouse_codemap[i * 2 + 1];
		}
	}

	return code;
}

static void ch9350l_mouse(const struct device *dev, const uint8_t *values, uint8_t length)
{
	struct ch9350l_data *data = dev->data;
	const uint8_t button = values[CH9350L_FRAME_MOUSE_BUTTON_BYTE];
	const uint16_t raw_x = sys_get_le16(&values[CH9350L_FRAME_MOUSE_X_BYTE]);
	const uint16_t raw_y = sys_get_le16(&values[CH9350L_FRAME_MOUSE_Y_BYTE]);
	const int16_t x = CH9350L_RAWMOUSE_TO_REL(raw_x);
	const int16_t y = CH9350L_RAWMOUSE_TO_REL(raw_y);

	input_report(dev, INPUT_EV_REL, INPUT_REL_X, x, true, K_FOREVER);
	input_report(dev, INPUT_EV_REL, INPUT_REL_Y, y, true, K_FOREVER);

	for (size_t i = 0; i < 8; i++) {
		if (button & BIT(i) && !(data->last_mouse_btns & BIT(i))) {
			if (input_report(dev, INPUT_EV_DEVICE,
				ch9350l_mouse_map(dev, BIT(i)), 1, true, K_FOREVER)) {
				LOG_ERR("Input failed to be enqueued");
			}
		} else if (data->last_mouse_btns & BIT(i) && !(button & BIT(i))) {
			if (input_report(dev, INPUT_EV_DEVICE,
				ch9350l_mouse_map(dev, BIT(i)), 0, true, K_FOREVER)) {
				LOG_ERR("Input failed to be enqueued");
			}
		} else {
			/* Dont care about other cases */
		}
	}
	data->last_mouse_btns = button;
}

static void ch9350l_input_work_handler(struct k_work *item)
{
	struct ch9350l_data *data = CONTAINER_OF(item, struct ch9350l_data, work);
	struct ch9350l_frame frame;
	uint8_t fd_label;
	uint8_t *fd_value;
	uint8_t fd_sum;
	uint8_t sum;

	while (k_msgq_get(&data->msgq, &frame, K_NO_WAIT) == 0) {
		sum = 0;
		fd_label = frame.data[0];
		fd_value = &frame.data[1];
		fd_sum = frame.data[frame.size - 1];

		for (int i = 0; i < frame.size - 2; i++) {
			sum += fd_value[i];
		}
		if (sum != fd_sum) {
			LOG_ERR("Frame checksum is invalid");
			continue;
		}

		switch ((fd_label & CH9350L_FRAME_TYPE_MASK) >> CH9350L_FRAME_TYPE_POS) {
		case CH9350L_FRAME_TYPE_KB:
			ch9350l_kb(data->dev, fd_value,
				   min(frame.size - 3, CH9350L_FRAME_VALUE_MAX));
			continue;
		case CH9350L_FRAME_TYPE_MOUSE:
			ch9350l_mouse(data->dev, fd_value,
				      min(frame.size - 3, CH9350L_FRAME_VALUE_MAX));
			continue;
		case CH9350L_FRAME_TYPE_MM:
		default:
			LOG_ERR("Unknown or unsupported input type");
			continue;
		}
	}
}

static int ch9350l_queue_frame(struct ch9350l_data *dev_data, uint8_t *data, size_t size)
{
	int ret;
	struct ch9350l_frame frame = {
		.size = size,
	};
	memcpy(frame.data, data, size);

	ret = k_msgq_put(&dev_data->msgq, &frame, K_NO_WAIT);
	if (ret < 0) {
		LOG_WRN("Frame dropped, queue full");
	}

	k_work_submit(&dev_data->work);

	return ret;
}

static uint8_t ch9350l_is_valid_frame(uint8_t *data, size_t size)
{
	const uint8_t fd_id = data[2];
	const uint8_t fd_length = data[3];

	/* Too small to be valid */
	if (size < CH9350L_FRAME_SIZE_MIN) {
		return 0;
	}

	/* Drop non-input frames */
	if (fd_id != CH9350L_FRAME_HEAD_KEY_0 && fd_id != CH9350L_FRAME_HEAD_KEY_1) {
		return 0;
	}

	/* We don't have the full frame yet */
	if (fd_length > (size - CH9350L_FRAME_LENGTH_OFF)) {
		return 0;
	}

	return fd_length;
}

static void ch9350l_input_callback(const struct device *dev_uart, void *user_data)
{
	struct ch9350l_data *data = (struct ch9350l_data *)user_data;
	uint8_t read_buffer[CH9350L_READBUF_SIZE];
	int read;
	uint8_t frame_size;

	uart_irq_update(dev_uart);
	if (!uart_irq_rx_ready(dev_uart)) {
		return;
	}

	while (true) {
		read = uart_fifo_read(dev_uart, read_buffer, CH9350L_READBUF_SIZE);
		if (read <= 0) {
			break;
		}
		if (data->frame_buffer_size + read >= CH9350L_FRAME_SIZE_MAX) {
			LOG_ERR("Maximum frame size exceeded");
			data->frame_started = false;
			data->frame_buffer_size = 0;
			continue;
		}
		memcpy(&data->frame_buffer[data->frame_buffer_size], read_buffer, read);
		data->frame_buffer_size += read;
		for (size_t offset = max((int)data->frame_buffer_size - read - 2, 0);
		     offset < (data->frame_buffer_size - 1); offset++) {
			if (data->frame_buffer[offset] != CH9350L_FRAME_HEAD_0
				|| data->frame_buffer[offset+1] != CH9350L_FRAME_HEAD_1) {
				continue;
			}
			if (data->frame_started) {
				frame_size = ch9350l_is_valid_frame(data->frame_buffer,
								data->frame_buffer_size);
				if (frame_size) {
					ch9350l_queue_frame(data,
					&data->frame_buffer[CH9350L_FRAME_LENGTH_OFF],
					frame_size);
				}
			}
			data->frame_started = true;
			memcpy(data->frame_buffer, &data->frame_buffer[offset],
				data->frame_buffer_size - offset);
			data->frame_buffer_size = data->frame_buffer_size - offset;
			break;
		}
	}

	if (data->frame_started) {
		frame_size = ch9350l_is_valid_frame(data->frame_buffer, data->frame_buffer_size);
		if (frame_size) {
			ch9350l_queue_frame(data, &data->frame_buffer[CH9350L_FRAME_LENGTH_OFF],
					    frame_size);
			data->frame_started = false;
			data->frame_buffer_size = 0;
		}
	}

	if (read < 0) {
		LOG_ERR("Error reading UART");
	}
}

static int ch9350l_init(struct device const *dev)
{
	const struct ch9350l_config *config = dev->config;
	struct ch9350l_data *data = dev->data;
	k_timepoint_t end_timeout =
		sys_timepoint_calc(K_MSEC(CH9350L_WAIT_TIMEOUT_MS));
	size_t check_p = 0;
	int ret = 0;
	char c;

	data->dev = dev;
	data->frame_buffer_size = 0;
	data->frame_started = false;
	k_work_init(&data->work, ch9350l_input_work_handler);
	k_msgq_init(&data->msgq, data->msgq_buffer, sizeof(struct ch9350l_frame),
		    CONFIG_INPUT_CH9350L_FRAME_COUNT);

	if (!device_is_ready(config->uart)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	for (int i = 0; i < sizeof(ch9350l_enable_status_frame); i++) {
		uart_poll_out(config->uart, ch9350l_enable_status_frame[i]);
	}

	while ((ret == 0 || ret == -1) && !sys_timepoint_expired(end_timeout)
		&& check_p < ARRAY_SIZE(ch9350l_valid_start_of_status_frame)) {
		ret = uart_poll_in(config->uart, &c);
		if (c == ch9350l_valid_start_of_status_frame[check_p]) {
			check_p++;
		}
	}
	if (check_p != ARRAY_SIZE(ch9350l_valid_start_of_status_frame)) {
		LOG_ERR("CH9350L not detected");
		return -ENXIO;
	}

	/* Flush FIFO if applicable */
	while (uart_poll_in(config->uart, &c)) {
	};

	for (int i = 0; i < sizeof(ch9350l_disable_status_frame); i++) {
		uart_poll_out(config->uart, ch9350l_disable_status_frame[i]);
	}

	ret = uart_irq_callback_user_data_set(config->uart, ch9350l_input_callback, data);
	if (ret < 0) {
		LOG_ERR("Couldn't set UART callback");
		return ret;
	}
	uart_irq_rx_enable(config->uart);

	return ret;
}

#define CH9350L_DEFINE(inst)									\
	static struct ch9350l_data ch9350l_data_##inst = {0};					\
												\
	static struct ch9350l_config const ch9350l_config_##inst = {				\
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),					\
		.kb_codemap = COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(inst), kb_codemap),	\
					  ((int []) DT_INST_PROP(inst, kb_codemap)), NULL),	\
		.kb_codemap_len = DT_INST_PROP_LEN_OR(inst, kb_codemap, 0),			\
		.mouse_codemap = COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(inst), mouse_codemap),\
				((int []) DT_INST_PROP(inst, mouse_codemap)), NULL),		\
		.mouse_codemap_len = DT_INST_PROP_LEN_OR(inst, mouse_codemap, 0),		\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, ch9350l_init, NULL, &ch9350l_data_##inst,			\
			      &ch9350l_config_##inst, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,	\
			      NULL);								\
	BUILD_ASSERT((DT_INST_PROP_LEN_OR(inst, kb_code_map, 0) & 0x1) == 0,			\
		"kb-codemap is not of a valid size");						\
	BUILD_ASSERT((DT_INST_PROP_LEN_OR(inst, mouse_code_map, 0) & 0x1) == 0,			\
		"mouse-codemap is not of a valid size");

DT_INST_FOREACH_STATUS_OKAY(CH9350L_DEFINE)
