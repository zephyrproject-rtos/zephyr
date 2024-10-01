/*
 * Copyright (c) 2023, Vitrolife A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.gassensing.co.uk/wp-content/uploads/2023/05/ExplorIR-M-Data-Sheet-Rev-4.13_3.pdf
 *
 */

#define DT_DRV_COMPAT gss_explorir_m

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/explorir_m.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(explorir_m_sensor, CONFIG_SENSOR_LOG_LEVEL);

#define EXPLORIR_M_BEGIN_CHAR ' '

#define EXPLORIR_M_SET_FILTER_CHAR     'A'
#define EXPLORIR_M_GET_FILTER_CHAR     'a'
#define EXPLORIR_M_MODE_CHAR           'K'
#define EXPLORIR_M_CO2_FILTERED_CHAR   'Z'
#define EXPLORIR_M_SCALING_CHAR        '.'
#define EXPLORIR_M_NOT_RECOGNISED_CHAR '?'

#define EXPLORIR_M_SEPARATOR_CHAR ' '
#define EXPLORIR_M_PRE_END_CHAR   '\r'
#define EXPLORIR_M_END_CHAR       '\n'

#define EXPLORIR_M_TYPE_INDEX  1
#define EXPLORIR_M_VALUE_INDEX 3

#define EXPLORIR_M_BUFFER_LENGTH 16

#define EXPLORIR_M_MAX_RESPONSE_DELAY 200 /* Add margin to the specified 100 in datasheet */
#define EXPLORIR_M_CO2_VALID_DELAY    1200

struct explorir_m_data {
	struct k_mutex uart_mutex;
	struct k_sem uart_rx_sem;
	uint16_t filtered;
	uint16_t scaling;
	uint8_t read_index;
	uint8_t read_buffer[EXPLORIR_M_BUFFER_LENGTH];
};

struct explorir_m_cfg {
	const struct device *uart_dev;
	uart_irq_callback_user_data_t cb;
};

enum explorir_m_uart_set_usage {
	EXPLORIR_M_SET_NONE,
	EXPLORIR_M_SET_VAL_ONE,
	EXPLORIR_M_SET_VAL_ONE_TWO,
};

enum EXPLORIR_M_MODE {
	EXPLORIR_M_MODE_COMMAND,
	EXPLORIR_M_MODE_STREAM,
	EXPLORIR_M_MODE_POLL,
};

static void explorir_m_uart_flush(const struct device *uart_dev)
{
	uint8_t tmp;

	while (uart_fifo_read(uart_dev, &tmp, 1) > 0) {
		continue;
	}
}

static void explorir_m_uart_flush_until_end(const struct device *uart_dev)
{
	uint8_t tmp;
	uint32_t uptime;

	uptime = k_uptime_get_32();
	do {
		uart_poll_in(uart_dev, &tmp);
	} while (tmp != EXPLORIR_M_END_CHAR &&
		 k_uptime_get_32() - uptime < EXPLORIR_M_MAX_RESPONSE_DELAY);
}

static void explorir_m_buffer_reset(struct explorir_m_data *data)
{
	memset(data->read_buffer, 0, data->read_index);
	data->read_index = 0;
}

static int explorir_m_buffer_verify(const struct explorir_m_data *data, char type)
{
	char buffer_type = data->read_buffer[EXPLORIR_M_TYPE_INDEX];

	if (data->read_buffer[0] == EXPLORIR_M_NOT_RECOGNISED_CHAR) {
		LOG_WRN("Sensor did not recognise the command");
		return -EIO;
	}

	if (buffer_type != type) {
		LOG_WRN("Expected type %c but got %c", type, buffer_type);
		return -EIO;
	}

	if (data->read_buffer[0] != EXPLORIR_M_BEGIN_CHAR ||
	    data->read_buffer[2] != EXPLORIR_M_SEPARATOR_CHAR ||
	    data->read_buffer[data->read_index - 2] != EXPLORIR_M_PRE_END_CHAR) {
		LOG_HEXDUMP_WRN(data->read_buffer, data->read_index, "Invalid buffer");
		return -EIO;
	}

	return 0;
}

static int explorir_m_buffer_process(struct explorir_m_data *data, char type,
				     struct sensor_value *val)
{
	if (explorir_m_buffer_verify(data, type) != 0) {
		return -EIO;
	}

	switch (type) {
	case EXPLORIR_M_SET_FILTER_CHAR:
	case EXPLORIR_M_MODE_CHAR:
		break;

	case EXPLORIR_M_CO2_FILTERED_CHAR:
		data->scaling = strtol(&data->read_buffer[EXPLORIR_M_VALUE_INDEX], NULL, 10);
		break;

	case EXPLORIR_M_SCALING_CHAR:
		data->filtered = strtol(&data->read_buffer[EXPLORIR_M_VALUE_INDEX], NULL, 10);
		break;

	case EXPLORIR_M_GET_FILTER_CHAR:
		val->val1 = strtol(&data->read_buffer[EXPLORIR_M_VALUE_INDEX], NULL, 10);
		break;

	default:
		LOG_ERR("Unknown type %c/0x%02x", type, type);
		return -EIO;
	}

	return 0;
}

static void explorir_m_uart_isr(const struct device *uart_dev, void *user_data)
{
	const struct device *dev = user_data;
	struct explorir_m_data *data = dev->data;
	int rc, read_len;

	if (!device_is_ready(uart_dev)) {
		LOG_DBG("UART device is not ready");
		return;
	}

	if (!uart_irq_update(uart_dev)) {
		LOG_DBG("Unable to process interrupts");
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		LOG_DBG("No RX data");
		return;
	}

	read_len = EXPLORIR_M_BUFFER_LENGTH - data->read_index;
	rc = uart_fifo_read(uart_dev, &data->read_buffer[data->read_index], read_len);

	if (rc < 0 || rc == read_len) {
		LOG_ERR("UART read failed: %d", rc < 0 ? rc : -ERANGE);
		explorir_m_uart_flush(uart_dev);
		LOG_HEXDUMP_WRN(data->read_buffer, data->read_index, "Discarding");
		explorir_m_buffer_reset(data);
	} else {
		data->read_index += rc;

		if (data->read_buffer[data->read_index - 1] != EXPLORIR_M_END_CHAR) {
			return;
		}
	}

	k_sem_give(&data->uart_rx_sem);
}

static void explorir_m_uart_terminate(const struct device *uart_dev)
{
	uart_poll_out(uart_dev, EXPLORIR_M_PRE_END_CHAR);
	uart_poll_out(uart_dev, EXPLORIR_M_END_CHAR);
}

static int explorir_m_await_receive(struct explorir_m_data *data)
{
	int rc = k_sem_take(&data->uart_rx_sem, K_MSEC(EXPLORIR_M_MAX_RESPONSE_DELAY));

	/* Reset semaphore if sensor did not respond within maximum specified response time */
	if (rc == -EAGAIN) {
		k_sem_reset(&data->uart_rx_sem);
	}

	return rc;
}

static int explorir_m_uart_transceive(const struct device *dev, char type, struct sensor_value *val,
				      enum explorir_m_uart_set_usage set)
{
	const struct explorir_m_cfg *cfg = dev->config;
	struct explorir_m_data *data = dev->data;
	char buf[EXPLORIR_M_BUFFER_LENGTH];
	int rc, len;

	if (val == NULL && set != EXPLORIR_M_SET_NONE) {
		LOG_ERR("val is NULL but set is not NONE");
		return -EINVAL;
	}

	k_mutex_lock(&data->uart_mutex, K_FOREVER);

	explorir_m_buffer_reset(data);

	uart_poll_out(cfg->uart_dev, type);

	if (set == EXPLORIR_M_SET_VAL_ONE) {
		len = snprintf(buf, EXPLORIR_M_BUFFER_LENGTH, "%c%u", EXPLORIR_M_SEPARATOR_CHAR,
			       val->val1);
	} else if (set == EXPLORIR_M_SET_VAL_ONE_TWO) {
		len = snprintf(buf, EXPLORIR_M_BUFFER_LENGTH, "%c%u%c%u", EXPLORIR_M_SEPARATOR_CHAR,
			       val->val1, EXPLORIR_M_SEPARATOR_CHAR, val->val2);
	} else {
		len = 0;
	}

	if (len == EXPLORIR_M_BUFFER_LENGTH) {
		LOG_WRN("Set value truncated");
	}
	for (int i = 0; i != len; i++) {
		uart_poll_out(cfg->uart_dev, buf[i]);
	}

	explorir_m_uart_terminate(cfg->uart_dev);

	rc = explorir_m_await_receive(data);
	if (rc != 0) {
		LOG_WRN("%c did not receive a response: %d", type, rc);
	}

	if (rc == 0) {
		rc = explorir_m_buffer_process(data, type, val);
	}

	k_mutex_unlock(&data->uart_mutex);

	return rc;
}

static int explorir_m_attr_get(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_CO2) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_EXPLORIR_M_FILTER:
		return explorir_m_uart_transceive(dev, EXPLORIR_M_GET_FILTER_CHAR, val,
						  EXPLORIR_M_SET_NONE);
	default:
		return -ENOTSUP;
	}
}

static int explorir_m_attr_set(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_CO2) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_EXPLORIR_M_FILTER:
		if (val->val1 < 0 || val->val1 > 255) {
			return -ERANGE;
		}
		return explorir_m_uart_transceive(dev, EXPLORIR_M_SET_FILTER_CHAR,
						  (struct sensor_value *)val,
						  EXPLORIR_M_SET_VAL_ONE);
	default:
		return -ENOTSUP;
	}
}

static int explorir_m_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	if (chan != SENSOR_CHAN_CO2 && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	return explorir_m_uart_transceive(dev, EXPLORIR_M_CO2_FILTERED_CHAR, NULL,
					  EXPLORIR_M_SET_NONE);
}

static int explorir_m_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct explorir_m_data *data = dev->data;

	if (chan != SENSOR_CHAN_CO2) {
		return -ENOTSUP;
	}

	if (k_uptime_get() < EXPLORIR_M_CO2_VALID_DELAY) {
		return -EAGAIN;
	}

	val->val1 = data->filtered * data->scaling;
	val->val2 = 0;

	return 0;
}

static const struct sensor_driver_api explorir_m_api_funcs = {
	.attr_set = explorir_m_attr_set,
	.attr_get = explorir_m_attr_get,
	.sample_fetch = explorir_m_sample_fetch,
	.channel_get = explorir_m_channel_get,
};

static int explorir_m_init(const struct device *dev)
{
	const struct explorir_m_cfg *cfg = dev->config;
	struct explorir_m_data *data = dev->data;
	struct sensor_value val;
	int rc;

	LOG_DBG("Initializing %s", dev->name);

	if (!device_is_ready(cfg->uart_dev)) {
		return -ENODEV;
	}

	k_mutex_init(&data->uart_mutex);
	k_sem_init(&data->uart_rx_sem, 0, 1);

	uart_irq_rx_disable(cfg->uart_dev);
	uart_irq_tx_disable(cfg->uart_dev);

	rc = uart_irq_callback_user_data_set(cfg->uart_dev, cfg->cb, (void *)dev);
	if (rc != 0) {
		LOG_ERR("UART IRQ setup failed: %d", rc);
		return rc;
	}

	/* Terminate garbled tx due to GPIO setup or crash during unfinished send */
	explorir_m_uart_terminate(cfg->uart_dev);
	explorir_m_uart_flush_until_end(cfg->uart_dev);

	uart_irq_rx_enable(cfg->uart_dev);

	val.val1 = EXPLORIR_M_MODE_POLL;
	explorir_m_uart_transceive(dev, EXPLORIR_M_MODE_CHAR, &val, EXPLORIR_M_SET_VAL_ONE);
	explorir_m_uart_transceive(dev, EXPLORIR_M_SCALING_CHAR, NULL, EXPLORIR_M_SET_NONE);

	return rc;
}

#define EXPLORIR_M_INIT(n)                                                                         \
                                                                                                   \
	static struct explorir_m_data explorir_m_data_##n;                                         \
                                                                                                   \
	static const struct explorir_m_cfg explorir_m_cfg_##n = {                                  \
		.uart_dev = DEVICE_DT_GET(DT_INST_BUS(n)),                                         \
		.cb = explorir_m_uart_isr,                                                         \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, explorir_m_init, NULL, &explorir_m_data_##n,               \
				     &explorir_m_cfg_##n, POST_KERNEL,                             \
				     CONFIG_SENSOR_INIT_PRIORITY, &explorir_m_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(EXPLORIR_M_INIT)
