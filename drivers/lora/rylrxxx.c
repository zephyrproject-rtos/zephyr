/*
 * Copyright (C) 2024 David Ullmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define DT_DRV_COMPAT reyax_rylrxxx

#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/modem/chat.h>
#include <zephyr/sys/atomic.h>
#include <errno.h>

LOG_MODULE_REGISTER(rylr, CONFIG_LORA_LOG_LEVEL);

#define RYLR_CMD_BAND_FORMAT                "AT+BAND=%u\r\n"
#define RYLR_CMD_BAND_PARM_CHARS            9U
#define RYLR_CMD_BAND_FORMAT_NUM_WILDCARDS  1U
#define RYLR_CMD_BAND_FORMAT_WILDCARD_CHARS (RYLR_CMD_BAND_FORMAT_NUM_WILDCARDS * 2)
#define RYLR_CMD_BAND_FORMAT_LEN_WITHOUT_WILDCARDS                                                 \
	(sizeof(RYLR_CMD_BAND_FORMAT) - RYLR_CMD_BAND_FORMAT_WILDCARD_CHARS - 1)
#define RYLR_CMD_BAND_LENGTH (RYLR_CMD_BAND_FORMAT_LEN_WITHOUT_WILDCARDS + RYLR_CMD_BAND_PARM_CHARS)

#define RYLR_CMD_SEND_FORMAT                "AT+SEND=0,%u,%s\r\n"
#define RYLR_CMD_SEND_FORMAT_NUM_WILDCARDS  2U
#define RYLR_CMD_SEND_FORMAT_WILDCARD_CHARS (RYLR_CMD_SEND_FORMAT_NUM_WILDCARDS * 2)
#define RYLR_CMD_SEND_FORMAT_LEN_WITHOUT_WILDCARDS                                                 \
	(sizeof(RYLR_CMD_SEND_FORMAT) - RYLR_CMD_SEND_FORMAT_WILDCARD_CHARS - 1)
#define RYLR_PAYLOAD_LENGTH_FIELD_CHARS(payload_len)                                               \
	(payload_len >= 100 ? 3 : (payload_len >= 10 ? 2 : 1))
#define RYLR_CMD_SEND_LENGTH(payload_len)                                                          \
	(RYLR_CMD_SEND_FORMAT_LEN_WITHOUT_WILDCARDS +                                              \
	 RYLR_PAYLOAD_LENGTH_FIELD_CHARS(payload_len) + payload_len)

#define RYLR_CMD_RF_SETTINGS_FORMAT                "AT+PARAMETER=%u,%u,%u,%u\r\n"
#define RYLR_CMD_RF_SETTINGS_FORMAT_NUM_WILDCARDS  4U
#define RYLR_CMD_RF_SETTINGS_FORMAT_WILDCARD_CHARS (RYLR_CMD_RF_SETTINGS_FORMAT_NUM_WILDCARDS * 2U)
#define RYLR_CMD_RF_SETTINGS_FORMAT_LEN_WITHOUT_WILDCARDS                                          \
	(sizeof(RYLR_CMD_RF_SETTINGS_FORMAT) - RYLR_CMD_RF_SETTINGS_FORMAT_WILDCARD_CHARS - 1)
#define RYLR_CMD_RF_SETTINGS_FORMAT_PARAM_CHARS(spread_factor)                                     \
	(RYLR_CMD_RF_SETTINGS_FORMAT_NUM_WILDCARDS - 1) + (spread_factor >= 10U ? 2U : 1U)
#define RYLR_CMD_RF_SETTINGS_LEN(spread_factor)                                                    \
	(RYLR_CMD_RF_SETTINGS_FORMAT_LEN_WITHOUT_WILDCARDS +                                       \
	 RYLR_CMD_RF_SETTINGS_FORMAT_PARAM_CHARS(spread_factor))

#define RYLR_CMD_POWER_FORMAT                "AT+CRFOP=%u\r\n"
#define RYLR_CMD_POWER_FORMAT_NUM_WILDCARDS  1U
#define RYLR_CMD_POWER_FORMAT_WILDCARD_CHARS (RYLR_CMD_POWER_FORMAT_NUM_WILDCARDS * 2U)
#define RYLR_CMD_POWER_FORMAT_LEN_WITHOUT_WILDCARDS                                                \
	(sizeof(RYLR_CMD_POWER_FORMAT) - RYLR_CMD_POWER_FORMAT_WILDCARD_CHARS - 1)
#define RYLR_CMD_POWER_FORMAT_PARAM_CHARS(power) (power >= 10U ? 2U : 1U)
#define RYLR_CMD_POWER_LEN(power)                                                                  \
	(RYLR_CMD_POWER_FORMAT_LEN_WITHOUT_WILDCARDS + RYLR_CMD_POWER_FORMAT_PARAM_CHARS(power))

#define RYLR_MAX_RESPONSE  256
#define RYLR_MAX_MSG_BYTES 256

#define RYLR_ALLOC_TIMEOUT K_SECONDS(1)

#define RYLR_TX_PENDING_FLAG_POS (0U)
#define RYLR_RX_PENDING_FLAG_POS (1U)

#define RYLR_IS_TX_PENDING(flags) (flags & (0x01 << RYLR_TX_PENDING_FLAG_POS))
#define RYLR_IS_RX_PENDING(flags) (flags & (0x01 << RYLR_RX_PENDING_FLAG_POS))

#define RYLR_SET_TX_PENDING(flags) (flags |= (0x01 << RYLR_TX_PENDING_FLAG_POS))
#define RYLR_SET_RX_PENDING(flags) (flags |= (0x01 << RYLR_RX_PENDING_FLAG_POS))

#define RYLR_CLEAR_TX_PENDING(flags) (flags &= ~(0x01 << RYLR_TX_PENDING_FLAG_POS))
#define RYLR_CLEAR_RX_PENDING(flags) (flags &= ~(0x01 << RYLR_RX_PENDING_FLAG_POS))

#define RYLR_IS_ASYNC_OP_PENDING(flags) (RYLR_IS_RX_PENDING(flags) || RYLR_IS_TX_PENDING(flags))

#define RYLR_MAX_RESPONSE_ARGS 6U

#define RYLR_MIN_RESET_MSECS  (100U)
#define RYLR_RESET_WAIT_MSECS (RYLR_MIN_RESET_MSECS + 10U)

struct rylr_config {
	const struct device *uart;
	const struct gpio_dt_spec reset;
};

struct rylr_data {
	uint8_t cmd_buffer[CONFIG_LORA_RYLRXX_CMD_BUF_SIZE];
	size_t curr_cmd_len;
	bool is_tx;
	int handler_error;
	struct k_msgq rx_msgq;
	struct k_sem script_sem;
	struct k_sem operation_sem;
	uint8_t pending_async_flags;
	struct k_poll_signal *async_tx_signal;
	lora_recv_cb async_rx_cb;
	const struct device *dev;
	uint8_t msgq_buffer[CONFIG_RYLRXXX_UNSOLICITED_RX_MSGQ_SIZE];
	struct modem_pipe *modem_pipe;
	uint8_t uart_backend_rx_buff[CONFIG_RYLRXXX_MODEM_BUFFERS_SIZE];
	uint8_t uart_backend_tx_buff[CONFIG_RYLRXXX_MODEM_BUFFERS_SIZE];
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t chat_rx_buf[CONFIG_RYLRXXX_MODEM_BUFFERS_SIZE];
	uint8_t chat_tx_buf[CONFIG_RYLRXXX_MODEM_BUFFERS_SIZE];
	uint8_t *chat_argv[RYLR_MAX_RESPONSE_ARGS];
	struct modem_chat chat;
	struct modem_chat_script dynamic_script;
	struct modem_chat_script_chat dynamic_chat;
};

struct rylr_recv_msg {
	uint16_t addr;
	uint8_t length;
	uint8_t *data;
	uint8_t rssi;
	uint8_t snr;
};

static void on_ok(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	int err = 0;
	struct rylr_data *driver_data = user_data;

	driver_data->handler_error = err;
}

static void on_err(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	int radio_err = 0;
	struct rylr_data *driver_data = user_data;

	driver_data->handler_error = -EIO;

	if (argc != 2) {
		driver_data->handler_error = -EBADMSG;
		LOG_ERR("malformed error message from radio");
		return;
	}

	radio_err = atoi(argv[1]);
	LOG_ERR("error from rylr: %d", radio_err);
}

static void on_rx(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct rylr_data *driver_data = user_data;
	struct rylr_recv_msg msg;
	int err = 0;

	driver_data->handler_error = 0;

	if (argc != 6) {
		driver_data->handler_error = -EBADMSG;
		return;
	}

	msg.addr = atoi(argv[1]);
	msg.length = atoi(argv[2]);
	msg.data = argv[3];
	msg.rssi = atoi(argv[4]);
	msg.snr = atoi(argv[5]);

	if (RYLR_IS_RX_PENDING(driver_data->pending_async_flags)) {
		driver_data->async_rx_cb(driver_data->dev, msg.data, msg.length, msg.rssi, msg.snr);
	} else {
		err = k_msgq_put(&driver_data->rx_msgq, &msg, K_NO_WAIT);
		if (err != 0) {
			LOG_ERR("error adding messgae to queue: %d", err);
			driver_data->handler_error = err;
		}
	}
}

static void on_script_finished(struct modem_chat *chat, enum modem_chat_script_result result,
			       void *user_data)
{
	struct rylr_data *driver_data = user_data;

	if (RYLR_IS_TX_PENDING(driver_data->pending_async_flags)) {
		RYLR_CLEAR_TX_PENDING(driver_data->pending_async_flags);
		k_poll_signal_raise(driver_data->async_tx_signal, driver_data->handler_error);
		k_sem_give(&driver_data->operation_sem);
	}

	k_sem_give(&driver_data->script_sem);
}

MODEM_CHAT_MATCH_DEFINE(ok_match, "+OK", "", on_ok);

MODEM_CHAT_MATCHES_DEFINE(abort_matches, MODEM_CHAT_MATCH("+ERR=", "", on_err));

MODEM_CHAT_MATCHES_DEFINE(unsol_matches, MODEM_CHAT_MATCH("+RCV=", ",", on_rx));

MODEM_CHAT_SCRIPT_CMDS_DEFINE(ping_cmd, MODEM_CHAT_SCRIPT_CMD_RESP("AT\r\n", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(ping_script, ping_cmd, abort_matches, on_script_finished,
			 CONFIG_RYLRXXX_RADIO_CMD_RESPONSE_TIMEOUT_MS);

static void rylr_reset_dynamic_script(struct rylr_data *data)
{
	data->dynamic_chat.response_matches = &ok_match;
	data->dynamic_chat.response_matches_size = 1;
	data->dynamic_chat.timeout = 0;

	data->dynamic_script.script_chats = &data->dynamic_chat;
	data->dynamic_script.script_chats_size = 1;
	data->dynamic_script.abort_matches = abort_matches;
	data->dynamic_script.abort_matches_size = 1;
	data->dynamic_script.callback = on_script_finished;
	data->dynamic_script.timeout = CONFIG_RYLRXXX_RADIO_CMD_RESPONSE_TIMEOUT_MS;
}

static uint32_t rylr_get_bandwidth_index(enum lora_signal_bandwidth bw)
{
	switch (bw) {
	case BW_125_KHZ:
		return 7;
	case BW_250_KHZ:
		return 8;
	case BW_500_KHZ:
		return 9;
	default:
		return 7;
	}
}

static int rylr_send_cmd_buffer(const struct device *dev)
{
	int err = 0;
	struct rylr_data *data = dev->data;

	rylr_reset_dynamic_script(data);

	data->dynamic_chat.request = data->cmd_buffer;
	data->dynamic_chat.request_size = data->curr_cmd_len;

	err = modem_chat_run_script(&data->chat, &data->dynamic_script);
	if (err != 0) {
		LOG_ERR("could not send cmd: %s. err: %d", data->cmd_buffer, err);
		return err;
	}
	err = k_sem_take(&data->script_sem, K_MSEC(CONFIG_RYLRXXX_RADIO_CMD_RESPONSE_TIMEOUT_MS));
	if (err) {
		LOG_ERR("error waiting for response: %d", err);
		return err;
	}
	return data->handler_error;
}

static int rylr_set_rf_band(const struct device *dev, uint32_t frequency)
{
	struct rylr_data *data = dev->data;

	if (sprintf(data->cmd_buffer, RYLR_CMD_BAND_FORMAT, frequency) != RYLR_CMD_BAND_LENGTH) {
		LOG_ERR("could not create frequency string");
		return -EINVAL;
	}

	data->curr_cmd_len = RYLR_CMD_BAND_LENGTH;
	return rylr_send_cmd_buffer(dev);
}

static int rylr_set_rf_parameters(const struct device *dev, uint32_t datarate, uint32_t bandwidth,
				  uint32_t coding_rate, uint32_t preamble_length)
{
	struct rylr_data *data = dev->data;
	size_t cmd_len;

	if (datarate < 7 || datarate > 12) {
		LOG_ERR("datarate/spread factor must be between 7 and 12 inclusive");
		return -EINVAL;
	}

	if (coding_rate < 1 || coding_rate > 4) {
		LOG_ERR("coding rate must be between 1 and 4 inclusive");
		return -EINVAL;
	}

	if (preamble_length < 4 || preamble_length > 7) {
		LOG_ERR("preamble length must be between 4 and 7 inclusive");
		return -EINVAL;
	}

	cmd_len = sprintf(data->cmd_buffer, RYLR_CMD_RF_SETTINGS_FORMAT, datarate,
			  rylr_get_bandwidth_index(bandwidth), coding_rate, preamble_length);
	if (cmd_len != RYLR_CMD_RF_SETTINGS_LEN(datarate)) {
		LOG_ERR("could not create rf settings string");
		return -EINVAL;
	}

	data->curr_cmd_len = cmd_len;
	return rylr_send_cmd_buffer(dev);
}

static int rylr_set_power(const struct device *dev, uint32_t power)
{
	struct rylr_data *data = dev->data;
	size_t cmd_len;

	if (power > 15) {
		LOG_ERR("power cannot be greater than 15");
		return -EINVAL;
	}

	cmd_len = RYLR_CMD_POWER_LEN(power);
	if (sprintf(data->cmd_buffer, RYLR_CMD_POWER_FORMAT, power) != cmd_len) {
		LOG_ERR("could not create power string");
		return -EINVAL;
	}

	data->curr_cmd_len = cmd_len;
	return rylr_send_cmd_buffer(dev);
}

static int rylr_config(const struct device *dev, struct lora_modem_config *config)
{
	int err = 0;
	struct rylr_data *data = dev->data;

	err = k_sem_take(&data->operation_sem, K_NO_WAIT);
	if (err != 0) {
		LOG_ERR("error taking operation semaphore: %d", err);
		return err;
	}

	if (RYLR_IS_ASYNC_OP_PENDING(data->pending_async_flags)) {
		LOG_ERR("pending async opperation");
		err = -EBUSY;
		goto exit;
	}

	err = rylr_set_rf_band(dev, config->frequency);
	if (err != 0) {
		LOG_ERR("could not send frequency cmd: %d", err);
		goto exit;
	}

	err = rylr_set_rf_parameters(dev, config->datarate, config->bandwidth, config->coding_rate,
				     config->preamble_len);
	if (err != 0) {
		LOG_ERR("could not send rf params cmd: %d", err);
		goto exit;
	}

	err = rylr_set_power(dev, config->tx_power);
	if (err != 0) {
		LOG_ERR("could not send power cmd: %d", err);
		goto exit;
	}

	data->is_tx = config->tx;

exit:
	k_sem_give(&data->operation_sem);
	return err;
}

int rylr_send(const struct device *dev, uint8_t *payload, uint32_t payload_len)
{
	int err = 0;
	struct rylr_data *data = dev->data;
	int cmd_len = RYLR_CMD_SEND_LENGTH(payload_len);

	err = k_sem_take(&data->operation_sem, K_NO_WAIT);
	if (err != 0) {
		LOG_ERR("error taking operation semaphore: %d", err);
		return err;
	}

	if (RYLR_IS_ASYNC_OP_PENDING(data->pending_async_flags)) {
		LOG_ERR("pending async opperation");
		err = -EBUSY;
		goto exit;
	}

	if (!data->is_tx) {
		LOG_ERR("radio not configured in tx mode");
		err = -EOPNOTSUPP;
		goto exit;
	}

	if (cmd_len > CONFIG_LORA_RYLRXX_CMD_BUF_SIZE) {
		LOG_ERR("payload too long");
		err = -EINVAL;
		goto exit;
	}

	snprintf(data->cmd_buffer, cmd_len + 1, RYLR_CMD_SEND_FORMAT, payload_len, payload);
	data->curr_cmd_len = cmd_len;
	err = rylr_send_cmd_buffer(dev);
	if (err != 0) {
		LOG_ERR("error sending data: %d", err);
		goto exit;
	}

exit:
	k_sem_give(&data->operation_sem);
	return err;
}

int rylr_send_async(const struct device *dev, uint8_t *payload, uint32_t payload_len,
		    struct k_poll_signal *async)
{
	int err = 0;
	struct rylr_data *data = dev->data;
	int cmd_len;

	err = k_sem_take(&data->operation_sem, K_NO_WAIT);
	if (err != 0) {
		LOG_ERR("error taking operation sem: %d", err);
		return err;
	}

	if (RYLR_IS_ASYNC_OP_PENDING(data->pending_async_flags)) {
		LOG_ERR("pending async opperation");
		err = -EBUSY;
		goto bail;
	}

	RYLR_SET_TX_PENDING(data->pending_async_flags);

	if (!data->is_tx) {
		LOG_ERR("radio not configured in tx mode");
		err = -EOPNOTSUPP;
		goto bail;
	}

	cmd_len = RYLR_CMD_SEND_LENGTH(payload_len);
	if (cmd_len > CONFIG_LORA_RYLRXX_CMD_BUF_SIZE) {
		LOG_ERR("payload too long");
		err = -EINVAL;
		goto bail;
	}

	if (async == NULL) {
		LOG_ERR("async signal cannot be null");
		err = -EINVAL;
		goto bail;
	}

	data->async_tx_signal = async;
	data->curr_cmd_len =
		snprintf(data->cmd_buffer, cmd_len + 1, RYLR_CMD_SEND_FORMAT, payload_len, payload);
	rylr_reset_dynamic_script(data);
	data->dynamic_chat.request = data->cmd_buffer;
	data->dynamic_chat.request_size = data->curr_cmd_len;

	return modem_chat_run_script_async(&data->chat, &data->dynamic_script);
bail:
	RYLR_CLEAR_TX_PENDING(data->pending_async_flags);
	k_sem_give(&data->operation_sem);
	return err;
}

int rylr_recv(const struct device *dev, uint8_t *ret_msg, uint8_t size, k_timeout_t timeout,
	      int16_t *rssi, int8_t *snr)
{

	int ret = 0;
	struct rylr_data *data = dev->data;
	struct rylr_recv_msg msg;

	ret = k_sem_take(&data->operation_sem, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("error taking operation semaphore: %d", ret);
		return ret;
	}

	if (data->is_tx) {
		LOG_ERR("radio is configured for tx");
		ret = -EOPNOTSUPP;
		goto exit;
	}

	if (RYLR_IS_ASYNC_OP_PENDING(data->pending_async_flags)) {
		LOG_ERR("pending async opperation");
		ret = -EBUSY;
		goto exit;
	}

	ret = k_msgq_get(&data->rx_msgq, &msg, timeout);
	if (ret != 0) {
		LOG_ERR("error getting msg from queue: %d", ret);
		goto exit;
	}

	ret = data->handler_error;
	if (ret != 0) {
		LOG_ERR("error in recv cb: %d", ret);
		goto exit;
	}

	if (msg.length > size) {
		LOG_ERR("buf len of %u too small for message len of %u", size, msg.length);
		ret = -ENOBUFS;
		goto exit;
	}

	*rssi = msg.rssi;
	*snr = msg.snr;
	memcpy(ret_msg, msg.data, msg.length);
	ret = msg.length;

exit:
	k_sem_give(&data->operation_sem);
	return ret;
}

int rylr_recv_async(const struct device *dev, lora_recv_cb cb)
{
	int err = 0;
	struct rylr_data *data = dev->data;

	err = k_sem_take(&data->operation_sem, K_NO_WAIT);
	if (err != 0) {
		LOG_ERR("error taking operation semaphore: %d", err);
		return err;
	}

	/* This is not a user error but the documeted way to cancel async reception in lora api*/
	if (cb == NULL) {
		goto bail;
	}

	if (data->is_tx) {
		LOG_ERR("radio is configured for tx");
		err = -EOPNOTSUPP;
		goto bail;
	}

	data->async_rx_cb = cb;
	if (RYLR_IS_ASYNC_OP_PENDING(data->pending_async_flags)) {
		LOG_ERR("pending async opperation");
		err = -EBUSY;
		goto bail;
	}
	RYLR_SET_RX_PENDING(data->pending_async_flags);

	return err;
bail:
	RYLR_CLEAR_RX_PENDING(data->pending_async_flags);
	k_sem_give(&data->operation_sem);
	return err;
}

int rylr_test_cw(const struct device *dev, uint32_t frequency, int8_t tx_power, uint16_t duration)
{
	return -ENOSYS;
}

static int rylr_init(const struct device *dev)
{
	int err = 0;
	struct rylr_data *data = dev->data;
	const struct rylr_config *config = dev->config;

	if (!gpio_is_ready_dt(&config->reset)) {
		return -ENODEV;
	}

	if (!device_is_ready(config->uart)) {
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->reset, config->reset.dt_flags);
	if (err != 0) {
		LOG_ERR("error configuring reset gpio: %d", err);
		return err;
	}

	k_msgq_init(&data->rx_msgq, data->msgq_buffer, sizeof(struct rylr_recv_msg),
		    ARRAY_SIZE(data->msgq_buffer));

	err = k_sem_init(&data->script_sem, 0, 1);
	if (err != 0) {
		LOG_ERR("error initializing response semaphore. err=%d", err);
	}

	err = k_sem_init(&data->operation_sem, 1, 1);
	if (err != 0) {
		LOG_ERR("error initializing operation semaphore. err=%d", err);
	}

	const struct modem_backend_uart_config uart_backend_config = {
		.uart = config->uart,
		.receive_buf = data->uart_backend_rx_buff,
		.receive_buf_size = ARRAY_SIZE(data->uart_backend_rx_buff),
		.transmit_buf = data->uart_backend_tx_buff,
		.transmit_buf_size = ARRAY_SIZE(data->uart_backend_tx_buff),
	};

	data->uart_pipe = modem_backend_uart_init(&data->uart_backend, &uart_backend_config);

	const struct modem_chat_config chat_config = {
		.user_data = data,
		.receive_buf = data->chat_rx_buf,
		.receive_buf_size = ARRAY_SIZE(data->chat_rx_buf),
		.delimiter = "\r\n",
		.delimiter_size = sizeof("\r\n") - 1,
		.filter = NULL,
		.filter_size = 0,
		.argv = data->chat_argv,
		.argv_size = ARRAY_SIZE(data->chat_argv),
		.unsol_matches = unsol_matches,
		.unsol_matches_size = ARRAY_SIZE(unsol_matches),
	};

	err = modem_chat_init(&data->chat, &chat_config);
	if (err != 0) {
		LOG_ERR("error initializing chat %d", err);
		return err;
	}

	err = modem_chat_attach(&data->chat, data->uart_pipe);
	if (err != 0) {
		LOG_ERR("error attaching chat %d", err);
		return err;
	}

	err = modem_pipe_open(data->uart_pipe);
	if (err != 0) {
		LOG_ERR("error opening uart pipe %d", err);
		return err;
	}

	err = gpio_pin_set_dt(&config->reset, 1);
	if (err != 0) {
		LOG_ERR("error setting reset line: %d", err);
		return err;
	}

	k_sleep(K_MSEC(RYLR_RESET_WAIT_MSECS));

	err = gpio_pin_set_dt(&config->reset, 0);
	if (err != 0) {
		LOG_ERR("error unsetting reset line: %d", err);
		return err;
	}

	k_sleep(K_MSEC(RYLR_RESET_WAIT_MSECS)); /* wait a bit more for module to boot up*/

	err = modem_chat_run_script(&data->chat, &ping_script);
	if (err != 0) {
		LOG_ERR("error pinging radio: %d", err);
		return err;
	}

	err = k_sem_take(&data->script_sem, K_MSEC(CONFIG_RYLRXXX_RADIO_CMD_RESPONSE_TIMEOUT_MS));
	if (err != 0) {
		LOG_ERR("error waiting for ping response from radio %d", err);
		return err;
	}

	LOG_INF("successfully initialized rylr");
	return err;
}

static const struct lora_driver_api rylr_lora_api = {
	.config = rylr_config,
	.send = rylr_send,
	.send_async = rylr_send_async,
	.recv = rylr_recv,
	.recv_async = rylr_recv_async,
	.test_cw = rylr_test_cw,
};

#define RYLR_DEVICE_INIT(n)                                                                        \
	static struct rylr_data dev_data_##n;                                                      \
	static const struct rylr_config dev_config_##n = {                                         \
		.uart = DEVICE_DT_GET(DT_INST_BUS(n)),                                             \
		.reset = GPIO_DT_SPEC_INST_GET(n, reset_gpios),                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &rylr_init, NULL, &dev_data_##n, &dev_config_##n, POST_KERNEL,    \
			      CONFIG_LORA_INIT_PRIORITY, &rylr_lora_api);

DT_INST_FOREACH_STATUS_OKAY(RYLR_DEVICE_INIT)
