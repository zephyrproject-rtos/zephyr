/*
 * Copyright (c) 2024, Vitrolife A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://sensorsandpower.angst-pfister.com/fileadmin/products/datasheets/272/Manual-FCX-MLD_1620-21914-0033-E-0821.pdf
 *
 */

#define DT_DRV_COMPAT ap_fcx_mldx5
#include <ctype.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/fcx_mldx5.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(fcx_mldx5_sensor, CONFIG_SENSOR_LOG_LEVEL);

#define FCX_MLDX5_STX 0x2
#define FCX_MLDX5_ETX 0x3

#define FCX_MLDX5_STX_LEN      1
#define FCX_MLDX5_CMD_LEN      2
/* Data length depends on command type thus defined in array */
#define FCX_MLDX5_CHECKSUM_LEN 2
#define FCX_MLDX5_ETX_LEN      1
#define FCX_MLDX5_HEADER_LEN                                                                       \
	(FCX_MLDX5_STX_LEN + FCX_MLDX5_CMD_LEN + FCX_MLDX5_CHECKSUM_LEN + FCX_MLDX5_ETX_LEN)

#define FCX_MLDX5_STX_INDEX                 0
#define FCX_MLDX5_CMD_INDEX                 (FCX_MLDX5_STX_INDEX + FCX_MLDX5_STX_LEN)
#define FCX_MLDX5_DATA_INDEX                (FCX_MLDX5_CMD_INDEX + FCX_MLDX5_CMD_LEN)
#define FCX_MLDX5_CHECKSUM_INDEX(frame_len) ((frame_len)-FCX_MLDX5_CHECKSUM_LEN - FCX_MLDX5_ETX_LEN)
#define FCX_MLDX5_ETX_INDEX(frame_len)      ((frame_len)-FCX_MLDX5_ETX_LEN)

#define FCX_MLDX5_MAX_FRAME_LEN      11
#define FCX_MLDX5_MAX_RESPONSE_DELAY 200 /* Not specified in datasheet */
#define FCX_MLDX5_MAX_HEAT_UP_TIME   180000

struct fcx_mldx5_data {
	struct k_mutex uart_mutex;
	struct k_sem uart_rx_sem;
	uint32_t o2_ppm;
	uint8_t status;
	uint8_t frame[FCX_MLDX5_MAX_FRAME_LEN];
	uint8_t frame_len;
};

struct fcx_mldx5_cfg {
	const struct device *uart_dev;
	uart_irq_callback_user_data_t cb;
};

enum fcx_mldx5_cmd {
	FCX_MLDX5_CMD_READ_STATUS,
	FCX_MLDX5_CMD_READ_O2_VALUE,
	FCX_MLDX5_CMD_SWITCH_SENSOR_ON_OFF,
	FCX_MLDX5_CMD_RESET,
	FCX_MLDX5_CMD_ERROR,
};

enum fcx_mldx5_errors {
	FCX_MLDX5_ERROR_CHECKSUM,
	FCX_MLDX5_ERROR_UNKNOWN_COMMAND,
	FCX_MLDX5_ERROR_PARAMETER,
	FCX_MLDX5_ERROR_EEPROM,
};

static const char *const fcx_mldx5_cmds[] = {
	[FCX_MLDX5_CMD_READ_STATUS] = "01",
	[FCX_MLDX5_CMD_READ_O2_VALUE] = "02",
	[FCX_MLDX5_CMD_SWITCH_SENSOR_ON_OFF] = "04",
	[FCX_MLDX5_CMD_RESET] = "11",
	[FCX_MLDX5_CMD_ERROR] = "EE",
};

static const uint8_t fcx_mldx5_cmds_data_len[] = {
	[FCX_MLDX5_CMD_READ_STATUS] = 2,
	[FCX_MLDX5_CMD_READ_O2_VALUE] = 5,
	[FCX_MLDX5_CMD_SWITCH_SENSOR_ON_OFF] = 1,
	[FCX_MLDX5_CMD_RESET] = 0,
	[FCX_MLDX5_CMD_ERROR] = 2,
};

static const char *const fcx_mldx5_errors[] = {
	[FCX_MLDX5_ERROR_CHECKSUM] = "checksum",
	[FCX_MLDX5_ERROR_UNKNOWN_COMMAND] = "command",
	[FCX_MLDX5_ERROR_PARAMETER] = "parameter",
	[FCX_MLDX5_ERROR_EEPROM] = "eeprom",
};

static void fcx_mldx5_uart_flush(const struct device *uart_dev)
{
	uint8_t tmp;

	while (uart_fifo_read(uart_dev, &tmp, 1) > 0) {
		continue;
	}
}

static uint8_t fcx_mldx5_calculate_checksum(const uint8_t *buf, size_t len)
{
	uint8_t checksum;
	size_t i;

	if (buf == NULL || len == 0) {
		return 0;
	}

	checksum = buf[0];
	for (i = 1; i < len; ++i) {
		checksum ^= buf[i];
	}

	return checksum;
}

static int fcx_mldx5_frame_check_error(const struct fcx_mldx5_data *data, const char *command_sent)
{
	const uint8_t len = FCX_MLDX5_HEADER_LEN + fcx_mldx5_cmds_data_len[FCX_MLDX5_CMD_ERROR];
	const char *command_error = fcx_mldx5_cmds[FCX_MLDX5_CMD_ERROR];
	const char *command_received = &data->frame[FCX_MLDX5_CMD_INDEX];
	const char *data_received = &data->frame[FCX_MLDX5_DATA_INDEX];
	uint8_t error;

	if (data->frame_len != len ||
	    strncmp(command_error, command_received, FCX_MLDX5_CMD_LEN) != 0) {
		return 0;
	}

	if (data_received[0] != 'E' || char2hex(data_received[1], &error) != 0 ||
	    error >= ARRAY_SIZE(fcx_mldx5_errors)) {
		LOG_ERR("Could not parse error value %.*s",
			fcx_mldx5_cmds_data_len[FCX_MLDX5_CMD_ERROR], data_received);
	} else {
		LOG_ERR("Command '%s' received error '%s'", command_sent, fcx_mldx5_errors[error]);
	}

	return -EIO;
}

static int fcx_mldx5_frame_verify(const struct fcx_mldx5_data *data, enum fcx_mldx5_cmd cmd)
{
	const uint8_t frame_len = FCX_MLDX5_HEADER_LEN + fcx_mldx5_cmds_data_len[cmd];
	const char *command = fcx_mldx5_cmds[cmd];
	const char *command_received = &data->frame[FCX_MLDX5_CMD_INDEX];
	uint8_t checksum;
	uint8_t checksum_received;

	if (fcx_mldx5_frame_check_error(data, command) != 0) {
		return -EIO;
	} else if (data->frame_len != frame_len) {
		LOG_ERR("Expected command %s frame length %u not %u", command, frame_len,
			data->frame_len);
		return -EIO;
	} else if (data->frame[FCX_MLDX5_STX_INDEX] != FCX_MLDX5_STX) {
		LOG_ERR("No STX");
		return -EIO;
	} else if (strncmp(command, command_received, FCX_MLDX5_CMD_LEN) != 0) {
		LOG_ERR("Expected command %s not %.*s", command, FCX_MLDX5_CMD_LEN,
			command_received);
		return -EIO;
	} else if (data->frame[FCX_MLDX5_ETX_INDEX(data->frame_len)] != FCX_MLDX5_ETX) {
		LOG_ERR("No ETX");
		return -EIO;
	}

	/* cmd and data bytes are used to calculate checksum */
	checksum = fcx_mldx5_calculate_checksum(command_received,
						FCX_MLDX5_CMD_LEN + fcx_mldx5_cmds_data_len[cmd]);
	checksum_received =
		strtol(&data->frame[FCX_MLDX5_CHECKSUM_INDEX(data->frame_len)], NULL, 16);
	if (checksum != checksum_received) {
		LOG_ERR("Expected checksum 0x%02x not 0x%02x", checksum, checksum_received);
		return -EIO;
	}

	return 0;
}

static void fcx_mldx5_uart_isr(const struct device *uart_dev, void *user_data)
{
	const struct device *dev = user_data;
	struct fcx_mldx5_data *data = dev->data;
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

	read_len = FCX_MLDX5_MAX_FRAME_LEN - data->frame_len;
	rc = read_len > 0 ? uart_fifo_read(uart_dev, &data->frame[data->frame_len], read_len)
			  : -ENOMEM;

	if (rc < 0) {
		LOG_ERR("UART read failed: %d", rc < 0 ? rc : -ERANGE);
		fcx_mldx5_uart_flush(uart_dev);
		LOG_HEXDUMP_ERR(data->frame, data->frame_len, "Discarding");
	} else {
		data->frame_len += rc;
		if (data->frame[FCX_MLDX5_ETX_INDEX(data->frame_len)] != FCX_MLDX5_ETX) {
			return;
		}
		LOG_HEXDUMP_DBG(data->frame, data->frame_len, "Frame received");
	}

	k_sem_give(&data->uart_rx_sem);
}

static void fcx_mldx5_uart_send(const struct device *dev, enum fcx_mldx5_cmd cmd,
				const char *cmd_data)
{
	const struct fcx_mldx5_cfg *cfg = dev->config;
	size_t cmd_data_len = cmd_data != NULL ? strlen(cmd_data) : 0;
	size_t frame_len = FCX_MLDX5_HEADER_LEN + cmd_data_len;
	char buf[FCX_MLDX5_MAX_FRAME_LEN];
	uint8_t checksum;
	size_t i;

	buf[FCX_MLDX5_STX_INDEX] = FCX_MLDX5_STX;
	memcpy(&buf[FCX_MLDX5_CMD_INDEX], fcx_mldx5_cmds[cmd], FCX_MLDX5_CMD_LEN);
	if (cmd_data_len != 0 && cmd_data_len == fcx_mldx5_cmds_data_len[cmd]) {
		memcpy(&buf[FCX_MLDX5_DATA_INDEX], cmd_data, strlen(cmd_data));
	}

	checksum = fcx_mldx5_calculate_checksum(&buf[FCX_MLDX5_CMD_INDEX],
						FCX_MLDX5_CMD_LEN + cmd_data_len);
	bin2hex(&checksum, 1, &buf[FCX_MLDX5_CHECKSUM_INDEX(frame_len)],
		FCX_MLDX5_MAX_FRAME_LEN - FCX_MLDX5_CHECKSUM_INDEX(frame_len));
	buf[FCX_MLDX5_ETX_INDEX(frame_len)] = FCX_MLDX5_ETX;

	for (i = 0; i < frame_len; ++i) {
		uart_poll_out(cfg->uart_dev, buf[i]);
	}

	LOG_HEXDUMP_DBG(buf, frame_len, "Frame sent");
}

static int fcx_mldx5_await_receive(const struct device *dev)
{
	int rc;
	const struct fcx_mldx5_cfg *cfg = dev->config;
	struct fcx_mldx5_data *data = dev->data;

	uart_irq_rx_enable(cfg->uart_dev);

	rc = k_sem_take(&data->uart_rx_sem, K_MSEC(FCX_MLDX5_MAX_RESPONSE_DELAY));

	/* Reset semaphore if sensor did not respond within maximum specified response time
	 */
	if (rc == -EAGAIN) {
		k_sem_reset(&data->uart_rx_sem);
	}

	uart_irq_rx_disable(cfg->uart_dev);

	return rc;
}

static int fcx_mldx5_read_status_value(struct fcx_mldx5_data *data, uint8_t data_len)
{
	char *cmd_data_received = &data->frame[FCX_MLDX5_DATA_INDEX];
	uint8_t value;

	if (cmd_data_received[0] != '0' || char2hex(cmd_data_received[1], &value)) {
		LOG_ERR("Could not parse status value %.*s", data_len, cmd_data_received);
		return -EIO;
	}

	switch (value) {
	case FCX_MLDX5_STATUS_STANDBY:
		break;
	case FCX_MLDX5_STATUS_RAMP_UP:
		break;
	case FCX_MLDX5_STATUS_RUN:
		break;
	case FCX_MLDX5_STATUS_ERROR:
		break;
	default:
		LOG_ERR("Status value %u invalid", value);
		return -EIO;
	}

	data->status = value;
	return 0;
}

static int fcx_mldx5_read_o2_value(struct fcx_mldx5_data *data)
{
	const char *o2_data = &data->frame[FCX_MLDX5_DATA_INDEX];
	uint8_t o2_data_len = fcx_mldx5_cmds_data_len[FCX_MLDX5_CMD_READ_O2_VALUE];
	uint32_t value = 0;
	size_t i;

	for (i = 0; i < o2_data_len; ++i) {
		if (i == 2) {
			if (o2_data[i] != '.') {
				goto invalid_data;
			}
		} else if (isdigit((int)o2_data[i]) == 0) {
			goto invalid_data;
		} else {
			value = value * 10 + (o2_data[i] - '0');
		}
	}

	data->o2_ppm = value * 100;
	return 0;

invalid_data:
	LOG_HEXDUMP_ERR(o2_data, o2_data_len, "Invalid O2 data");
	return -EIO;
}

static int fcx_mldx5_buffer_process(struct fcx_mldx5_data *data, enum fcx_mldx5_cmd cmd,
				    const char *cmd_data)
{
	if (fcx_mldx5_frame_verify(data, cmd) != 0) {
		return -EIO;
	}

	switch (cmd) {
	case FCX_MLDX5_CMD_READ_STATUS:
		return fcx_mldx5_read_status_value(data, fcx_mldx5_cmds_data_len[cmd]);
	case FCX_MLDX5_CMD_READ_O2_VALUE:
		return fcx_mldx5_read_o2_value(data);
	case FCX_MLDX5_CMD_SWITCH_SENSOR_ON_OFF:
		return cmd_data != NULL && data->frame[FCX_MLDX5_DATA_INDEX] == cmd_data[0];
	case FCX_MLDX5_CMD_RESET:
		return 0;
	default:
		LOG_ERR("Unknown command 0x%02x", cmd);
		return -EIO;
	}
}

static int fcx_mldx5_uart_transceive(const struct device *dev, enum fcx_mldx5_cmd cmd,
				     const char *cmd_data)
{
	struct fcx_mldx5_data *data = dev->data;
	int rc;

	k_mutex_lock(&data->uart_mutex, K_FOREVER);

	data->frame_len = 0;
	fcx_mldx5_uart_send(dev, cmd, cmd_data);

	rc = fcx_mldx5_await_receive(dev);
	if (rc != 0) {
		LOG_ERR("%s did not receive a response: %d", fcx_mldx5_cmds[cmd], rc);
	} else {
		rc = fcx_mldx5_buffer_process(data, cmd, cmd_data);
	}

	k_mutex_unlock(&data->uart_mutex);

	return rc;
}

static int fcx_mldx5_attr_get(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, struct sensor_value *val)
{
	struct fcx_mldx5_data *data = dev->data;
	int rc;

	if (chan != SENSOR_CHAN_O2) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_FCX_MLDX5_STATUS:
		rc = fcx_mldx5_uart_transceive(dev, FCX_MLDX5_CMD_READ_STATUS, NULL);
		val->val1 = data->status;
		return rc;
	default:
		return -ENOTSUP;
	}
}

static int fcx_mldx5_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	if (chan != SENSOR_CHAN_O2 && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	return fcx_mldx5_uart_transceive(dev, FCX_MLDX5_CMD_READ_O2_VALUE, NULL);
}

static int fcx_mldx5_channel_get(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct fcx_mldx5_data *data = dev->data;

	if (chan != SENSOR_CHAN_O2) {
		return -ENOTSUP;
	}

	val->val1 = data->o2_ppm;
	val->val2 = 0;

	return 0;
}

static const struct sensor_driver_api fcx_mldx5_api_funcs = {
	.attr_get = fcx_mldx5_attr_get,
	.sample_fetch = fcx_mldx5_sample_fetch,
	.channel_get = fcx_mldx5_channel_get,
};

#ifdef CONFIG_PM_DEVICE
static int pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return fcx_mldx5_uart_transceive(dev, FCX_MLDX5_CMD_SWITCH_SENSOR_ON_OFF, "1");
	case PM_DEVICE_ACTION_SUSPEND:
		/* Standby with 20 % heating output */
		return fcx_mldx5_uart_transceive(dev, FCX_MLDX5_CMD_SWITCH_SENSOR_ON_OFF, "0");
	default:
		return -ENOTSUP;
	}
}
#endif

static int fcx_mldx5_init(const struct device *dev)
{
	int rc;
	const struct fcx_mldx5_cfg *cfg = dev->config;
	struct fcx_mldx5_data *data = dev->data;

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

	/* Retry in case of garbled tx due to GPIO setup, crash during unfinished send or sensor
	 * start up time
	 */
	if (!WAIT_FOR(fcx_mldx5_uart_transceive(dev, FCX_MLDX5_CMD_READ_STATUS, NULL) == 0,
		      1000 * USEC_PER_MSEC, k_msleep(10))) {
		LOG_ERR("Read status failed");
		return -EIO;
	}

	LOG_INF("%s status 0x%x", dev->name, data->status);

	return 0;
}

#define FCX_MLDX5_INIT(n)                                                                          \
                                                                                                   \
	static struct fcx_mldx5_data fcx_mldx5_data_##n = {                                        \
		.status = FCX_MLDX5_STATUS_UNKNOWN,                                                \
	};                                                                                         \
                                                                                                   \
	static const struct fcx_mldx5_cfg fcx_mldx5_cfg_##n = {                                    \
		.uart_dev = DEVICE_DT_GET(DT_INST_BUS(n)),                                         \
		.cb = fcx_mldx5_uart_isr,                                                          \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, pm_action);                                                    \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, fcx_mldx5_init, PM_DEVICE_DT_INST_GET(n),                  \
				     &fcx_mldx5_data_##n, &fcx_mldx5_cfg_##n, POST_KERNEL,         \
				     CONFIG_SENSOR_INIT_PRIORITY, &fcx_mldx5_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(FCX_MLDX5_INIT)
