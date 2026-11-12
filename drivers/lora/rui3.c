/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * AT command reference:
 * https://docs.rakwireless.com/RUI3/Serial-Operating-Modes/AT-Command-Manual/
 */

#define DT_DRV_COMPAT rakwireless_rui3

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(rui3, CONFIG_LORA_LOG_LEVEL);

/*
 * RUI3 carries a P2P payload as hexadecimal. Release note RUI-1054 of 4.1.1
 * puts the limit at 478 hexadecimal digits, so 239 bytes, which is lower than
 * the 256 the AT command manual states.
 */
#define RUI3_MAX_PAYLOAD 239U

/*
 * AT+PRECV takes three open-ended windows. 65535 listens until one packet
 * arrives and then stops, which is what a bounded receive needs; 65534 keeps
 * listening afterwards, which is what an asynchronous receiver needs. Both
 * apply the receive parameters to the radio, unlike a window given in
 * milliseconds.
 */
#define RUI3_PRECV_ONE_SHOT   65535U
#define RUI3_PRECV_CONTINUOUS 65534U

/* AT+PRECV accepts a window of at most 65532 ms. */
#define RUI3_PRECV_MAX_MS 65532U

#define RUI3_NWM_P2P 0

#define RUI3_ARGV_SIZE 8U

#define RUI3_RESET_PULSE_MS 10U

/* The measurement never transmits, so the power only has to be in range. */
#define RUI3_LBT_TEST_POWER 14U

/* Measured at 0.3 s on RUI3 4.3.0; leave room for a slower release. */
#define RUI3_REBOOT_WAIT_MS 2000U

struct rui3_config {
	const struct device *uart;
	const struct gpio_dt_spec reset;
};

struct rui3_rx_packet {
	int16_t rssi;
	int8_t snr;
	uint8_t len;
	uint8_t data[RUI3_MAX_PAYLOAD];
};

/*
 * The six radio parameters AT+P2P carries. A trailing 1 keeps them in RAM and
 * applies them in one go, where each per-parameter command writes the whole
 * configuration page to flash instead, around 50 ms a time.
 */
struct rui3_p2p_params {
	uint32_t frequency;
	uint32_t datarate;
	uint32_t bandwidth;
	uint32_t coding_rate;
	uint32_t preamble_len;
	uint32_t tx_power;
};

struct rui3_data {
	const struct device *dev;

	struct modem_pipe *pipe;
	struct modem_backend_uart backend;
	struct modem_chat chat;
	uint8_t backend_rx_buf[CONFIG_LORA_RUI3_MODEM_BUF_SIZE];
	uint8_t backend_tx_buf[CONFIG_LORA_RUI3_MODEM_BUF_SIZE];
	uint8_t chat_rx_buf[CONFIG_LORA_RUI3_MODEM_BUF_SIZE];
	uint8_t *argv[RUI3_ARGV_SIZE];

	char cmd[CONFIG_LORA_RUI3_CMD_BUF_SIZE];
	struct modem_chat_script script;
	struct modem_chat_script_chat script_chat;

	/*
	 * A semaphore rather than a mutex: an asynchronous send returns
	 * holding it and the event handler releases it from the modem
	 * workqueue, which a mutex would not allow.
	 */
	struct k_sem op_sem;
	struct k_sem script_done;
	struct k_sem tx_done;
	int script_result;

	/*
	 * What the module last accepted, so that a command goes out only for
	 * a value that differs and no parameter has to be read back inside a
	 * receive window.
	 */
	struct lora_modem_config applied;
	struct rui3_p2p_params p2p;
	bool applied_valid;

	/* Response of the last AT+<cmd>=? query, without the echoed name. */
	char query[32];
	bool query_valid;

	struct k_msgq rx_msgq;
	char rx_msgq_buf[CONFIG_LORA_RUI3_RX_MSGQ_SIZE * sizeof(struct rui3_rx_packet)];
	struct rui3_rx_packet rx_scratch;

	lora_recv_cb async_rx_cb;
	void *async_rx_user_data;
	/*
	 * Claimed with an atomic swap: the completion event and the
	 * timeout run on different work queues and only one of them may
	 * raise the signal and release op_sem.
	 */
	atomic_ptr_t async_tx_signal;
	struct k_work_delayable tx_timeout_work;
	bool rx_continuous;
};

static int rui3_hex_to_bin(const char *hex, uint8_t *out, size_t out_size, uint8_t *out_len)
{
	size_t len = strlen(hex);

	if ((len % 2U) != 0U) {
		return -EBADMSG;
	}

	len /= 2U;
	if (len > out_size) {
		return -ENOBUFS;
	}

	for (size_t i = 0; i < len; i++) {
		char byte[3] = {hex[2U * i], hex[(2U * i) + 1U], '\0'};
		char *end;
		long val = strtol(byte, &end, 16);

		if (*end != '\0') {
			return -EBADMSG;
		}
		out[i] = (uint8_t)val;
	}

	*out_len = (uint8_t)len;
	return 0;
}

static void rui3_on_ok(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct rui3_data *data = user_data;

	data->script_result = 0;
}

static void rui3_on_error(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct rui3_data *data = user_data;

	LOG_ERR("module rejected the command: %s", argv[0]);
	data->script_result = -EIO;
}

/* Answers to AT+<name>=? arrive as "AT+<name>=<value>". */
static void rui3_on_query(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct rui3_data *data = user_data;
	const char *eq;

	/*
	 * The match consumes the "AT+" prefix into argv[0] and leaves the rest
	 * of the line, "<name>=<value>", in argv[1].
	 */
	if (argc < 2U) {
		return;
	}

	eq = strchr(argv[1], '=');
	if (eq == NULL) {
		return;
	}

	strncpy(data->query, eq + 1, sizeof(data->query) - 1U);
	data->query[sizeof(data->query) - 1U] = '\0';
	data->query_valid = true;

	/* The echoed value is the answer; the OK that follows may be eaten. */
	data->script_result = 0;
}

/*
 * Runs on the modem workqueue. An asynchronous send left the operation
 * semaphore taken and the signal set, so finishing it here is what releases
 * the driver for the next caller.
 */
static bool rui3_finish_async_tx(struct rui3_data *data, int result)
{
	struct k_poll_signal *sig = atomic_ptr_set(&data->async_tx_signal, NULL);

	if (sig == NULL) {
		return false;
	}

	k_poll_signal_raise(sig, result);
	k_sem_give(&data->op_sem);
	return true;
}

static void rui3_tx_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct rui3_data *data = CONTAINER_OF(dwork, struct rui3_data, tx_timeout_work);

	if (rui3_finish_async_tx(data, -ETIMEDOUT)) {
		LOG_ERR("no transmit completion event");
	}
}

static void rui3_on_tx_done(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct rui3_data *data = user_data;

	if (rui3_finish_async_tx(data, 0)) {
		(void)k_work_cancel_delayable(&data->tx_timeout_work);
		return;
	}

	k_sem_give(&data->tx_done);
}

/* "+EVT:RXP2P:<rssi>:<snr>:<payload>" split on ':'. */
static void rui3_on_rx(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct rui3_data *data = user_data;
	struct rui3_rx_packet *pkt = &data->rx_scratch;
	int ret;

	if (argc != 4U) {
		LOG_ERR("malformed receive event, %u fields", argc);
		return;
	}

	ret = rui3_hex_to_bin(argv[3], pkt->data, sizeof(pkt->data), &pkt->len);
	if (ret != 0) {
		LOG_ERR("cannot decode the received payload: %d", ret);
		return;
	}

	pkt->rssi = (int16_t)strtol(argv[1], NULL, 10);
	pkt->snr = (int8_t)strtol(argv[2], NULL, 10);

	if (data->async_rx_cb != NULL) {
		data->async_rx_cb(data->dev, pkt->data, pkt->len, pkt->rssi, pkt->snr,
				  data->async_rx_user_data);
		return;
	}

	ret = k_msgq_put(&data->rx_msgq, pkt, K_NO_WAIT);
	if (ret != 0) {
		LOG_WRN("receive queue full, packet dropped");
	}
}

static void rui3_on_script_done(struct modem_chat *chat, enum modem_chat_script_result result,
				const struct modem_chat_script_completion_info *info,
				void *user_data)
{
	struct rui3_data *data = user_data;

	if (result != MODEM_CHAT_SCRIPT_RESULT_SUCCESS) {
		data->script_result = -EIO;
	}

	/*
	 * An asynchronous send waits for the completion event, not for the
	 * script, which only carries the OK that acknowledges the command. The
	 * caller is finished here only when the command itself was refused.
	 */
	if (data->script_result != 0) {
		if (rui3_finish_async_tx(data, data->script_result)) {
			(void)k_work_cancel_delayable(&data->tx_timeout_work);
		}
	}

	k_sem_give(&data->script_done);
}

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", rui3_on_ok);
/* A query answers with the echoed value and then OK, two lines. */
MODEM_CHAT_MATCHES_DEFINE(query_matches, MODEM_CHAT_MATCH("AT+", "", rui3_on_query),
			  MODEM_CHAT_MATCH("OK", "", rui3_on_ok));

MODEM_CHAT_MATCHES_DEFINE(abort_matches, MODEM_CHAT_MATCH("AT_ERROR", "", rui3_on_error),
			  MODEM_CHAT_MATCH("AT_PARAM_ERROR", "", rui3_on_error),
			  MODEM_CHAT_MATCH("AT_BUSY_ERROR", "", rui3_on_error),
			  MODEM_CHAT_MATCH("AT_MODE_NO_SUPPORT", "", rui3_on_error));

MODEM_CHAT_MATCHES_DEFINE(unsol_matches, MODEM_CHAT_MATCH("+EVT:RXP2P:", ":", rui3_on_rx),
			  MODEM_CHAT_MATCH("+EVT:TXP2P DONE", "", rui3_on_tx_done));

/*
 * Start the script for the command already held in data->cmd without waiting
 * for it. modem_chat appends the delimiter itself, so the command carries
 * none. The caller holds op_sem.
 */
static int rui3_start(const struct device *dev, const struct modem_chat_match *match,
		      uint16_t match_size, int32_t timeout_ms)
{
	struct rui3_data *data = dev->data;
	size_t len = strlen(data->cmd);
	int ret;

	data->script_result = -ETIMEDOUT;
	data->query_valid = false;

	data->script_chat.request = data->cmd;
	data->script_chat.request_size = (uint16_t)len;
	data->script_chat.response_matches = match;
	data->script_chat.response_matches_size = match_size;
	data->script_chat.timeout = 0;

	data->script.script_chats = &data->script_chat;
	data->script.script_chats_size = 1U;
	data->script.abort_matches = abort_matches;
	data->script.abort_matches_size = ARRAY_SIZE(abort_matches);
	data->script.callback = rui3_on_script_done;
	/* modem_chat takes the script timeout in seconds, rounded up. */
	data->script.timeout = (uint16_t)DIV_ROUND_UP(timeout_ms, MSEC_PER_SEC);

	k_sem_reset(&data->script_done);

	ret = modem_chat_run_script_async(&data->chat, &data->script);
	if (ret != 0) {
		LOG_ERR("cannot start \"%s\": %d", data->cmd, ret);
	}

	return ret;
}

/*
 * Send a command whose reply the driver cannot match on, because the module
 * reboots and answers with a banner. The caller waits the module out and
 * confirms the outcome with a query of its own.
 */
static int rui3_start_raw(const struct device *dev, const char *cmd)
{
	struct rui3_data *data = dev->data;
	int ret;

	if (strlen(cmd) >= sizeof(data->cmd)) {
		return -EINVAL;
	}

	strcpy(data->cmd, cmd);

	/*
	 * No reply will match, so give the script a short deadline and let it
	 * expire. Starting the next command before this one ends is refused.
	 */
	ret = rui3_start(dev, &ok_match, 1U, RUI3_REBOOT_WAIT_MS);
	if (ret != 0) {
		return ret;
	}

	(void)k_sem_take(&data->script_done, K_MSEC(RUI3_REBOOT_WAIT_MS * 2));
	return 0;
}

static int rui3_run_match(const struct device *dev, const char *cmd,
			  const struct modem_chat_match *match, uint16_t match_size,
			  int32_t timeout_ms)
{
	struct rui3_data *data = dev->data;
	int ret;

	/* The send path builds in place, so only copy a command from elsewhere. */
	if (cmd != data->cmd) {
		if (strlen(cmd) >= sizeof(data->cmd)) {
			LOG_ERR("command does not fit in the command buffer");
			return -EINVAL;
		}
		strcpy(data->cmd, cmd);
	}

	ret = rui3_start(dev, match, match_size, timeout_ms);
	if (ret != 0) {
		return ret;
	}

	ret = k_sem_take(&data->script_done, K_MSEC(timeout_ms));
	if (ret != 0) {
		LOG_ERR("no answer to \"%s\"", cmd);
		return -ETIMEDOUT;
	}

	return data->script_result;
}

static int rui3_run(const struct device *dev, const char *cmd, bool expect_query)
{
	if (expect_query) {
		return rui3_run_match(dev, cmd, query_matches, ARRAY_SIZE(query_matches),
				      CONFIG_LORA_RUI3_CMD_TIMEOUT_MS);
	}

	return rui3_run_match(dev, cmd, &ok_match, 1U, CONFIG_LORA_RUI3_CMD_TIMEOUT_MS);
}

static int rui3_query_u32(const struct device *dev, const char *cmd, uint32_t *value)
{
	struct rui3_data *data = dev->data;
	int ret = rui3_run(dev, cmd, true);

	if (ret != 0) {
		return ret;
	}

	if (!data->query_valid) {
		return -EBADMSG;
	}

	*value = (uint32_t)strtoul(data->query, NULL, 10);
	return 0;
}

/*
 * AT+IQINVER stores its value in flash, and a write there measures 50 ms on a
 * 4.2.2 module, so the value the driver last applied is passed as @p cached to
 * keep an unchanged setting off the wire. A caller with no cache reads the
 * module back, which costs a round trip of its own.
 */
static int rui3_set_iq(const struct device *dev, bool inverted, const uint32_t *cached)
{
	uint32_t value = inverted ? 1U : 0U;
	uint32_t current;
	char cmd[24];
	int ret;

	if (cached != NULL) {
		if (*cached == value) {
			return 0;
		}
	} else {
		ret = rui3_query_u32(dev, "AT+IQINVER=?", &current);
		if ((ret == 0) && (current == value)) {
			return 0;
		}
	}

	ret = snprintf(cmd, sizeof(cmd), "AT+IQINVER=%u", value);
	if ((ret < 0) || ((size_t)ret >= sizeof(cmd))) {
		return -EINVAL;
	}

	return rui3_run(dev, cmd, false);
}

static int rui3_bandwidth_code(enum lora_signal_bandwidth bw, uint32_t *code)
{
	switch (bw) {
	case BW_7_KHZ:
		*code = 3U;
		break;
	case BW_10_KHZ:
		*code = 4U;
		break;
	case BW_15_KHZ:
		*code = 5U;
		break;
	case BW_20_KHZ:
		*code = 6U;
		break;
	case BW_31_KHZ:
		*code = 7U;
		break;
	case BW_41_KHZ:
		*code = 8U;
		break;
	case BW_62_KHZ:
		*code = 9U;
		break;
	case BW_125_KHZ:
		*code = 0U;
		break;
	case BW_250_KHZ:
		*code = 1U;
		break;
	case BW_500_KHZ:
		*code = 2U;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int rui3_coding_rate_code(enum lora_coding_rate cr, uint32_t *code)
{
	switch (cr) {
	case CR_4_5:
		*code = 0U;
		break;
	case CR_4_6:
		*code = 1U;
		break;
	case CR_4_7:
		*code = 2U;
		break;
	case CR_4_8:
		*code = 3U;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int rui3_bandwidth_hz(enum lora_signal_bandwidth bw, uint32_t *bw_hz)
{
	switch (bw) {
	case BW_7_KHZ:
		*bw_hz = 7810U;
		break;
	case BW_10_KHZ:
		*bw_hz = 10420U;
		break;
	case BW_15_KHZ:
		*bw_hz = 15630U;
		break;
	case BW_20_KHZ:
		*bw_hz = 20830U;
		break;
	case BW_31_KHZ:
		*bw_hz = 31250U;
		break;
	case BW_41_KHZ:
		*bw_hz = 41670U;
		break;
	case BW_62_KHZ:
		*bw_hz = 62500U;
		break;
	case BW_125_KHZ:
		*bw_hz = 125000U;
		break;
	case BW_250_KHZ:
		*bw_hz = 250000U;
		break;
	case BW_500_KHZ:
		*bw_hz = 500000U;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 * RUI3 reports the airtime only once a frame has left the radio, so the figure
 * a caller needs beforehand is computed here from the applied configuration.
 * The module drives the radio through loramac-node, which turns the low data
 * rate optimization on when a symbol lasts longer than 16.38 ms.
 */
static uint32_t rui3_airtime(const struct device *dev, uint32_t data_len)
{
	struct rui3_data *data = dev->data;
	uint32_t t_sym_us, t_preamble_us, n_payload, bw_hz;
	uint8_t sf, cr, hdr_len, de;
	int32_t bits;
	int ret;

	if (!data->applied_valid) {
		return 0;
	}

	ret = rui3_bandwidth_hz(data->applied.bandwidth, &bw_hz);
	if (ret != 0) {
		return 0;
	}

	sf = (uint8_t)data->applied.datarate;
	t_sym_us = ((1UL << sf) * USEC_PER_SEC) / bw_hz;
	t_preamble_us = (data->applied.preamble_len + 4U) * t_sym_us + (t_sym_us / 4U);

	de = (t_sym_us > 16380U) ? 1U : 0U;
	cr = (uint8_t)data->applied.coding_rate;
	hdr_len = data->applied.explicit_header_disable ? 0U : 20U;

	/* The module always appends the 16 bit payload CRC. */
	bits = (int32_t)(8U * data_len) - (int32_t)(4U * sf) + 8 + hdr_len + 16;
	if (bits < 0) {
		bits = 0;
	}

	n_payload = 8U + (DIV_ROUND_UP((uint32_t)bits, 4U * (sf - 2U * de)) * (cr + 4U));

	return DIV_ROUND_CLOSEST(t_preamble_us + (n_payload * t_sym_us), USEC_PER_MSEC);
}

static int rui3_query_p2p(const struct device *dev, struct rui3_p2p_params *out)
{
	struct rui3_data *data = dev->data;
	uint32_t *fields[] = {&out->frequency,   &out->datarate,     &out->bandwidth,
			      &out->coding_rate, &out->preamble_len, &out->tx_power};
	const char *pos;
	char *end;
	int ret;

	ret = rui3_run(dev, "AT+P2P=?", true);
	if (ret != 0) {
		return ret;
	}

	if (!data->query_valid) {
		return -EBADMSG;
	}

	pos = data->query;
	for (size_t i = 0U; i < ARRAY_SIZE(fields); i++) {
		*fields[i] = (uint32_t)strtoul(pos, &end, 10);
		if (end == pos) {
			return -EBADMSG;
		}

		pos = (*end == ':') ? (end + 1) : end;
	}

	return 0;
}

static int rui3_set_p2p(const struct device *dev, struct rui3_p2p_params *want)
{
	struct rui3_data *data = dev->data;
	struct rui3_p2p_params have;
	char cmd[64];
	int ret;

	if (data->applied_valid) {
		have = data->p2p;
	} else {
		ret = rui3_query_p2p(dev, &have);
		if (ret != 0) {
			return ret;
		}
	}

	/*
	 * A receiver configured before any sender has no power of its own,
	 * and the command carries all six parameters, so keep the module's.
	 */
	if (want->tx_power == 0U) {
		want->tx_power = have.tx_power;
	}

	if (memcmp(&have, want, sizeof(have)) == 0) {
		return 0;
	}

	ret = snprintf(cmd, sizeof(cmd), "AT+P2P=%u:%u:%u:%u:%u:%u:1", want->frequency,
		       want->datarate, want->bandwidth, want->coding_rate, want->preamble_len,
		       want->tx_power);
	if ((ret < 0) || ((size_t)ret >= sizeof(cmd))) {
		return -EINVAL;
	}

	return rui3_run(dev, cmd, false);
}

/*
 * AT+SYNCWORD carries the 16 bit register value of the radio rather than the
 * sync word itself, so 0x12 is written as 1424 and 0x34 as 3444. The module
 * answers in lower case, hence the case insensitive compare.
 */
/*
 * An explicit sync word wins over public_network, which otherwise picks
 * between the public 0x34 and the private 0x12 of the LoRa specification.
 */
static uint8_t rui3_sync_word_of(const struct lora_modem_config *config)
{
	if (config->sync_word != 0U) {
		return config->sync_word;
	}

	return config->public_network ? 0x34U : 0x12U;
}

static uint16_t rui3_sync_word_reg(uint8_t sync_word)
{
	return (uint16_t)(((sync_word & 0xF0U) << 8) | 0x0400U | ((sync_word & 0x0FU) << 4) |
			  0x04U);
}

static int rui3_set_sync_word(const struct device *dev, uint16_t reg, const uint16_t *cached)
{
	struct rui3_data *data = dev->data;
	char cmd[24];
	int ret;

	if (cached != NULL) {
		if (*cached == reg) {
			return 0;
		}
	} else {
		ret = rui3_run(dev, "AT+SYNCWORD=?", true);
		if ((ret == 0) && data->query_valid &&
		    ((uint16_t)strtoul(data->query, NULL, 16) == reg)) {
			return 0;
		}
	}

	ret = snprintf(cmd, sizeof(cmd), "AT+SYNCWORD=%04X", reg);
	if ((ret < 0) || ((size_t)ret >= sizeof(cmd))) {
		return -EINVAL;
	}

	return rui3_run(dev, cmd, false);
}

static int rui3_set_rx_window(const struct device *dev, uint32_t arg)
{
	char cmd[24];
	int ret = snprintf(cmd, sizeof(cmd), "AT+PRECV=%u", arg);

	if ((ret < 0) || ((size_t)ret >= sizeof(cmd))) {
		return -EINVAL;
	}

	return rui3_run(dev, cmd, false);
}

/*
 * AT+TCONF holds the radio the measurement runs on, separately from the P2P
 * settings, as frequency:power:bandwidth:datarate:coderate:lna:paBoost:modem:
 * payloadLen:fdev:lowDrOpt:BTproduct. Only the frequency and the width matter
 * here, and the width is an index rather than a figure in Hz.
 */
static int rui3_lbt_bandwidth_code(uint32_t bandwidth_hz, uint32_t *code)
{
	if (bandwidth_hz <= 125000U) {
		*code = 0U;
	} else if (bandwidth_hz <= 250000U) {
		*code = 1U;
	} else if (bandwidth_hz <= 500000U) {
		*code = 2U;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int rui3_read_rssi(const struct device *dev, int16_t *rssi)
{
	struct rui3_data *data = dev->data;
	int ret;

	/* The reply carries no "=?" form, so it is a bare command. */
	ret = rui3_run(dev, "AT+TRSSI", true);
	if (ret != 0) {
		return ret;
	}

	if (!data->query_valid) {
		return -EBADMSG;
	}

	*rssi = (int16_t)strtol(data->query, NULL, 10);
	return 0;
}

static int rui3_lbt(const struct device *dev, uint32_t frequency, uint32_t bandwidth_hz,
		    int16_t threshold_dbm, k_timeout_t duration)
{
	struct rui3_data *data = dev->data;
	k_timepoint_t expiry;
	char cmd[72];
	uint32_t bw;
	bool busy = false;
	int ret;

	ret = rui3_lbt_bandwidth_code(bandwidth_hz, &bw);
	if (ret != 0) {
		LOG_ERR("no channel filter is %u Hz wide", bandwidth_hz);
		return ret;
	}

	ret = snprintf(cmd, sizeof(cmd), "AT+TCONF=%u:%u:%u:7:1:0:0:1:4:25000:0:0", frequency,
		       RUI3_LBT_TEST_POWER, bw);
	if ((ret < 0) || ((size_t)ret >= sizeof(cmd))) {
		return -EINVAL;
	}

	k_sem_take(&data->op_sem, K_FOREVER);

	ret = rui3_run(dev, cmd, false);
	if (ret != 0) {
		goto out;
	}

	/*
	 * One read already outlasts @p duration, so the loop ends on the
	 * first pass unless the channel is busy. It is a loop so that a
	 * shorter read still covers the time the caller asked for.
	 */
	expiry = sys_timepoint_calc(duration);

	do {
		int16_t rssi;

		ret = rui3_read_rssi(dev, &rssi);
		if (ret != 0) {
			goto out;
		}

		if (rssi >= threshold_dbm) {
			busy = true;
			break;
		}
	} while (!sys_timepoint_expired(expiry));

	ret = busy ? 1 : 0;

out:
	k_sem_give(&data->op_sem);
	return ret;
}

static int rui3_config(const struct device *dev, const struct lora_modem_config *config)
{
	struct rui3_data *data = dev->data;
	struct rui3_p2p_params want;
	uint32_t prev_iq;
	uint16_t sync_reg, prev_sync;
	bool cached = false;
	int8_t tx_power;
	uint32_t bw;
	uint32_t cr;
	int ret;

	if (config->tx && (config->tx_power < 5 || config->tx_power > 22)) {
		LOG_ERR("transmit power must be between 5 and 22 dBm");
		return -EINVAL;
	}

	if (config->preamble_len < 5U) {
		LOG_ERR("preamble length must be 5 symbols or more");
		return -EINVAL;
	}

	/*
	 * RUI3 keeps the payload CRC on and exposes no command for it. On
	 * reception that costs nothing, because an explicit header carries
	 * its own CRC presence bit and the radio follows the header, so a
	 * frame sent without a CRC still decodes. A transmission cannot drop
	 * it, so one that asks to is refused.
	 */
	if (config->packet_crc_disable && config->tx) {
		LOG_ERR("the payload CRC cannot be turned off for transmission");
		return -ENOTSUP;
	}

	if (config->explicit_header_disable) {
		LOG_ERR("the implicit header is not supported");
		return -ENOTSUP;
	}

	ret = rui3_bandwidth_code(config->bandwidth, &bw);
	if (ret != 0) {
		LOG_ERR("unsupported bandwidth");
		return ret;
	}

	ret = rui3_coding_rate_code(config->coding_rate, &cr);
	if (ret != 0) {
		LOG_ERR("unsupported coding rate");
		return ret;
	}

	k_sem_take(&data->op_sem, K_FOREVER);

	/* Leave reception before touching the radio parameters. */
	ret = rui3_set_rx_window(dev, 0U);
	if (ret != 0) {
		goto out;
	}

	data->rx_continuous = false;

	/*
	 * Once a configuration has been applied the driver knows what the
	 * module holds, so the parameters below are compared against that
	 * rather than read back. A receive window leaves no room for the
	 * queries: the caller opens it a fixed margin ahead of the frame.
	 */
	if (data->applied_valid) {
		prev_iq = data->applied.iq_inverted ? 1U : 0U;
		cached = true;
	}

	/* The command carries the power too, so a receiver repeats the old one. */
	want = (struct rui3_p2p_params){
		.frequency = config->frequency,
		.datarate = config->datarate,
		.bandwidth = bw,
		.coding_rate = cr,
		.preamble_len = config->preamble_len,
		.tx_power = config->tx ? (uint32_t)config->tx_power : data->p2p.tx_power,
	};

	ret = rui3_set_p2p(dev, &want);
	if (ret != 0) {
		goto out;
	}

	sync_reg = rui3_sync_word_reg(rui3_sync_word_of(config));
	if (cached) {
		prev_sync = rui3_sync_word_reg(rui3_sync_word_of(&data->applied));
	}

	ret = rui3_set_sync_word(dev, sync_reg, cached ? &prev_sync : NULL);
	if (ret != 0) {
		goto out;
	}

	ret = rui3_set_iq(dev, config->iq_inverted, cached ? &prev_iq : NULL);
	if (ret != 0) {
		goto out;
	}

	tx_power = config->tx ? config->tx_power : data->applied.tx_power;
	data->applied = *config;
	data->applied.tx_power = tx_power;
	data->p2p = want;
	data->applied_valid = true;

out:
	k_sem_give(&data->op_sem);
	return ret;
}

/* Build "AT+PSEND=<hex>" in the command buffer. The caller holds op_sem. */
static int rui3_build_send(const struct device *dev, const uint8_t *payload, uint32_t payload_len)
{
	struct rui3_data *data = dev->data;
	size_t pos;

	if (!data->applied_valid || !data->applied.tx) {
		LOG_ERR("not configured for transmission");
		return -EINVAL;
	}

	pos = (size_t)snprintf(data->cmd, sizeof(data->cmd), "AT+PSEND=");
	if ((pos + (2U * payload_len) + sizeof("\r\n")) >= sizeof(data->cmd)) {
		return -EINVAL;
	}

	for (uint32_t i = 0; i < payload_len; i++) {
		pos += (size_t)snprintf(&data->cmd[pos], sizeof(data->cmd) - pos, "%02X",
					payload[i]);
	}

	return 0;
}

static int rui3_send(const struct device *dev, uint8_t *payload, uint32_t payload_len)
{
	struct rui3_data *data = dev->data;
	int ret;

	if ((payload_len == 0U) || (payload_len > RUI3_MAX_PAYLOAD)) {
		return -EINVAL;
	}

	k_sem_take(&data->op_sem, K_FOREVER);

	ret = rui3_build_send(dev, payload, payload_len);
	if (ret != 0) {
		goto out;
	}

	k_sem_reset(&data->tx_done);

	ret = rui3_run(dev, data->cmd, false);
	if (ret != 0) {
		goto out;
	}

	/* OK only acknowledges the command, the event reports the airtime. */
	ret = k_sem_take(&data->tx_done, K_MSEC(CONFIG_LORA_RUI3_TX_TIMEOUT_MS));
	if (ret != 0) {
		LOG_ERR("no transmit completion event");
		ret = -ETIMEDOUT;
	}

out:
	k_sem_give(&data->op_sem);
	return ret;
}

/*
 * Returns as soon as the command is on its way and leaves op_sem taken. The
 * completion event releases it, which is what the LoRaWAN stack needs to time
 * its receive windows from the end of the transmission.
 */
static int rui3_send_async(const struct device *dev, uint8_t *payload, uint32_t payload_len,
			   struct k_poll_signal *async)
{
	struct rui3_data *data = dev->data;
	int ret;

	if ((payload_len == 0U) || (payload_len > RUI3_MAX_PAYLOAD) || (async == NULL)) {
		return -EINVAL;
	}

	k_sem_take(&data->op_sem, K_FOREVER);

	ret = rui3_build_send(dev, payload, payload_len);
	if (ret != 0) {
		k_sem_give(&data->op_sem);
		return ret;
	}

	/*
	 * Arm the signal and the timeout before starting, because the
	 * completion event can land on the modem workqueue as soon as the
	 * script is submitted.
	 */
	atomic_ptr_set(&data->async_tx_signal, async);
	(void)k_work_schedule(&data->tx_timeout_work, K_MSEC(CONFIG_LORA_RUI3_TX_TIMEOUT_MS));

	ret = rui3_start(dev, &ok_match, 1U, CONFIG_LORA_RUI3_CMD_TIMEOUT_MS);
	if (ret != 0) {
		(void)k_work_cancel_delayable(&data->tx_timeout_work);
		if (rui3_finish_async_tx(data, ret) == false) {
			/* The event beat us to it, so op_sem is already free. */
			return 0;
		}
		return ret;
	}

	return 0;
}

static int rui3_recv(const struct device *dev, uint8_t *data_buf, uint8_t size, k_timeout_t timeout,
		     int16_t *rssi, int8_t *snr)
{
	struct rui3_data *data = dev->data;
	struct rui3_rx_packet pkt;
	int ret;

	k_sem_take(&data->op_sem, K_FOREVER);

	if (!data->applied_valid || data->applied.tx) {
		LOG_ERR("not configured for reception");
		k_sem_give(&data->op_sem);
		return -EINVAL;
	}

	/*
	 * A bounded AT+PRECV window puts the radio into reception without
	 * applying the receive parameters first, so it would listen with the
	 * settings of the last transmission, inverted IQ among them. Only the
	 * open-ended window reconfigures the radio, so reception always uses
	 * that one and the caller's deadline closes it here.
	 */
	ret = rui3_set_rx_window(dev, RUI3_PRECV_ONE_SHOT);
	if (ret != 0) {
		k_sem_give(&data->op_sem);
		return ret;
	}

	data->rx_continuous = false;

	k_sem_give(&data->op_sem);

	ret = k_msgq_get(&data->rx_msgq, &pkt, timeout);
	if (ret != 0) {
		k_sem_take(&data->op_sem, K_FOREVER);
		(void)rui3_set_rx_window(dev, 0U);
		data->rx_continuous = false;
		k_sem_give(&data->op_sem);
		return -EAGAIN;
	}

	if (pkt.len > size) {
		return -ENOBUFS;
	}

	memcpy(data_buf, pkt.data, pkt.len);

	if (rssi != NULL) {
		*rssi = pkt.rssi;
	}
	if (snr != NULL) {
		*snr = pkt.snr;
	}

	return (int)pkt.len;
}

static int rui3_recv_async(const struct device *dev, lora_recv_cb cb, void *user_data)
{
	struct rui3_data *data = dev->data;
	int ret = 0;

	k_sem_take(&data->op_sem, K_FOREVER);

	if (cb == NULL) {
		data->async_rx_cb = NULL;
		data->async_rx_user_data = NULL;
		if (data->rx_continuous) {
			ret = rui3_set_rx_window(dev, 0U);
			data->rx_continuous = false;
		}
		goto out;
	}

	if (!data->applied_valid || data->applied.tx) {
		LOG_ERR("not configured for reception");
		ret = -EINVAL;
		goto out;
	}

	data->async_rx_cb = cb;
	data->async_rx_user_data = user_data;

	ret = rui3_set_rx_window(dev, RUI3_PRECV_CONTINUOUS);
	if (ret == 0) {
		data->rx_continuous = true;
	} else {
		data->async_rx_cb = NULL;
	}

out:
	k_sem_give(&data->op_sem);
	return ret;
}

/*
 * The banner and AT+VER=? both report "RUI_<major>.<minor>.<patch>_<model>".
 * The firmware is flashed independently of this build, so an older module is
 * reported and then used anyway, the way the esp-hosted-mcu driver treats its
 * coprocessor.
 */
static void rui3_check_version(const struct device *dev)
{
	struct rui3_data *data = dev->data;
	unsigned int major;
	unsigned int minor;
	unsigned int patch;
	uint32_t found;
	uint32_t want;
	int ret;

	ret = rui3_run(dev, "AT+VER=?", true);
	if ((ret != 0) || !data->query_valid) {
		LOG_WRN("firmware version unknown, expected RUI3 %u.%u.%u or later",
			CONFIG_LORA_RUI3_FW_VERSION_MAJOR, CONFIG_LORA_RUI3_FW_VERSION_MINOR,
			CONFIG_LORA_RUI3_FW_VERSION_PATCH);
		return;
	}

	LOG_INF("module reports %s", data->query);

	if (sscanf(data->query, "RUI_%u.%u.%u", &major, &minor, &patch) != 3) {
		LOG_WRN("cannot read a version out of \"%s\"", data->query);
		return;
	}

	found = (major * 1000000U) + (minor * 1000U) + patch;
	want = (CONFIG_LORA_RUI3_FW_VERSION_MAJOR * 1000000U) +
	       (CONFIG_LORA_RUI3_FW_VERSION_MINOR * 1000U) + CONFIG_LORA_RUI3_FW_VERSION_PATCH;

	if (found < want) {
		LOG_WRN("RUI3 %u.%u.%u is older than the expected %u.%u.%u, P2P may misbehave",
			major, minor, patch, CONFIG_LORA_RUI3_FW_VERSION_MAJOR,
			CONFIG_LORA_RUI3_FW_VERSION_MINOR, CONFIG_LORA_RUI3_FW_VERSION_PATCH);
	}
}

static int rui3_enter_p2p(const struct device *dev)
{
	uint32_t mode;
	int ret;

	ret = rui3_query_u32(dev, "AT+NWM=?", &mode);
	if (ret != 0) {
		return ret;
	}

	if (mode == RUI3_NWM_P2P) {
		return 0;
	}

	/*
	 * A change of network work mode reboots the module, so the command
	 * answers and the banner follows. Give the reboot time to finish, then
	 * confirm the mode rather than trusting the delay.
	 */
	LOG_INF("switching to LoRa P2P, the module reboots");

	ret = rui3_start_raw(dev, "AT+NWM=0");
	if (ret != 0) {
		return ret;
	}

	ret = rui3_query_u32(dev, "AT+NWM=?", &mode);
	if (ret != 0) {
		return ret;
	}

	if (mode != RUI3_NWM_P2P) {
		LOG_ERR("module stayed in work mode %u", mode);
		return -EIO;
	}

	return 0;
}

static int rui3_reset(const struct device *dev)
{
	const struct rui3_config *config = dev->config;
	int ret;

	if (config->reset.port == NULL) {
		/*
		 * ATZ answers with the boot banner rather than OK, and the
		 * banner text has changed between RUI3 releases, so send the
		 * command without expecting a reply and wait the module out.
		 * rui3_init() pings it afterwards.
		 */
		ret = rui3_start_raw(dev, "ATZ");
		if (ret != 0) {
			return ret;
		}

		return 0;
	}

	ret = gpio_pin_set_dt(&config->reset, 1);
	if (ret != 0) {
		return ret;
	}

	k_msleep(RUI3_RESET_PULSE_MS);

	ret = gpio_pin_set_dt(&config->reset, 0);
	if (ret != 0) {
		return ret;
	}

	k_msleep(CONFIG_LORA_RUI3_RESET_TIMEOUT_MS / 4);
	return 0;
}

static int rui3_init(const struct device *dev)
{
	const struct rui3_config *config = dev->config;
	struct rui3_data *data = dev->data;
	int ret;

	data->dev = dev;

	if (!device_is_ready(config->uart)) {
		LOG_ERR("UART not ready");
		return -ENODEV;
	}

	if (config->reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset)) {
			LOG_ERR("reset GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("cannot configure the reset GPIO: %d", ret);
			return ret;
		}
	}

	k_sem_init(&data->op_sem, 1, 1);
	k_sem_init(&data->script_done, 0, 1);
	k_sem_init(&data->tx_done, 0, 1);
	k_work_init_delayable(&data->tx_timeout_work, rui3_tx_timeout_handler);
	k_msgq_init(&data->rx_msgq, data->rx_msgq_buf, sizeof(struct rui3_rx_packet),
		    CONFIG_LORA_RUI3_RX_MSGQ_SIZE);

	const struct modem_backend_uart_config backend_config = {
		.uart = config->uart,
		.receive_buf = data->backend_rx_buf,
		.receive_buf_size = ARRAY_SIZE(data->backend_rx_buf),
		.transmit_buf = data->backend_tx_buf,
		.transmit_buf_size = ARRAY_SIZE(data->backend_tx_buf),
	};

	data->pipe = modem_backend_uart_init(&data->backend, &backend_config);

	const struct modem_chat_config chat_config = {
		.user_data = data,
		.receive_buf = data->chat_rx_buf,
		.receive_buf_size = ARRAY_SIZE(data->chat_rx_buf),
		.delimiter = "\r\n",
		.delimiter_size = sizeof("\r\n") - 1U,
		.filter = NULL,
		.filter_size = 0U,
		.argv = data->argv,
		.argv_size = ARRAY_SIZE(data->argv),
		.unsol_matches = unsol_matches,
		.unsol_matches_size = ARRAY_SIZE(unsol_matches),
	};

	ret = modem_chat_init(&data->chat, &chat_config);
	if (ret != 0) {
		LOG_ERR("cannot initialize the chat: %d", ret);
		return ret;
	}

	ret = modem_chat_attach(&data->chat, data->pipe);
	if (ret != 0) {
		LOG_ERR("cannot attach the chat: %d", ret);
		return ret;
	}

	ret = modem_pipe_open(data->pipe, K_SECONDS(10));
	if (ret != 0) {
		LOG_ERR("cannot open the UART pipe: %d", ret);
		return ret;
	}

	ret = rui3_reset(dev);
	if (ret != 0) {
		LOG_ERR("cannot reset the module: %d", ret);
		return ret;
	}

	ret = rui3_run(dev, "AT", false);
	if (ret != 0) {
		LOG_ERR("module does not answer");
		return ret;
	}

	rui3_check_version(dev);

	ret = rui3_enter_p2p(dev);
	if (ret != 0) {
		return ret;
	}

	/* Reception is left off until the application configures a direction. */
	ret = rui3_set_rx_window(dev, 0U);
	if (ret != 0) {
		return ret;
	}

	LOG_INF("RUI3 module ready in LoRa P2P mode");
	return 0;
}

static DEVICE_API(lora, rui3_api) = {
	.config = rui3_config,
	.airtime = rui3_airtime,
	.send = rui3_send,
	.send_async = rui3_send_async,
	.recv = rui3_recv,
	.recv_async = rui3_recv_async,
	.lbt = rui3_lbt,
};

#define RUI3_DEVICE(inst)                                                                          \
	static struct rui3_data rui3_data_##inst;                                                  \
                                                                                                   \
	static const struct rui3_config rui3_config_##inst = {                                     \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, rui3_init, NULL, &rui3_data_##inst, &rui3_config_##inst,       \
			      POST_KERNEL, CONFIG_LORA_INIT_PRIORITY, &rui3_api);

DT_INST_FOREACH_STATUS_OKAY(RUI3_DEVICE)
