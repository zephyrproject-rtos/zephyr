/*
 * Copyright (c) 2019 Tobias Svehagen
 * Copyright (c) 2020 Grinn
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_st67w611m1

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(st67w611m1, CONFIG_WIFI_LOG_LEVEL);

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/types.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_linkaddr.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>

#include <modem_context.h>
#include <modem_cmd_handler.h>

#include "st67w611m1_drv.h"
#include "st67w611m1_spi.h"

K_KERNEL_STACK_DEFINE(st67_workq_stack, CONFIG_ST67W611M1_WORKQ_STACK_SIZE);
K_KERNEL_STACK_DEFINE(st67_at_rx_stack, CONFIG_ST67W611M1_RX_THREAD_STACK_SIZE);

NET_BUF_POOL_DEFINE(at_cmd_net_buf_pool, CONFIG_ST67W611M1_TX_AT_CMD_NET_BUF_POOL_COUNT,
		    CONFIG_ST67W611M1_TX_AT_CMD_NET_BUF_POOL_SIZE, 0, NULL);
NET_PKT_SLAB_DEFINE(at_cmd_net_pkt_slab, CONFIG_ST67W611M1_TX_AT_CMD_NET_PKT_SLAB_COUNT);

RING_BUF_DECLARE(rx_at_cmd_ring_buf, CONFIG_ST67W611M1_RX_AT_CMD_RING_BUF_SIZE);

static struct k_thread st67_at_rx_thread;

static struct st67_driver_data driver_data;

static inline int st67_send_at_cmd(struct st67_driver_data *st67_data,
				   const struct modem_cmd *handler_cmds, size_t handler_cmds_len,
				   const uint8_t *buf, k_timeout_t timeout)
{
	return modem_cmd_send(&st67_data->mctx.iface, &st67_data->mctx.cmd_handler, handler_cmds,
			      handler_cmds_len, buf, &st67_data->sem_cmd_response_wait, timeout);
}

/* modem_cmd_handler read cb */
static int st67_receive_at_cmd_from_bus(struct modem_iface *iface, uint8_t *buf, size_t size,
					size_t *bytes_read)
{
	if (buf == NULL) {
		return -EINVAL;
	}

	size_t available_len = ring_buf_size_get(&rx_at_cmd_ring_buf);

	if (available_len > size) {
		available_len = size;
	}

	*bytes_read = ring_buf_get(&rx_at_cmd_ring_buf, buf, available_len);

	return 0;
}

static struct net_pkt *alloc_net_pkt_and_copy_buf(const uint8_t *buf, size_t len)
{
	struct net_pkt *pkt;
	struct net_buf *new_buf;

	pkt = net_pkt_alloc_from_slab(&at_cmd_net_pkt_slab, ST67W611M1_NET_PKT_ALLOC_TIMEOUT);

	if (pkt == NULL) {
		return NULL;
	}

	new_buf = net_pkt_get_reserve_data(&at_cmd_net_buf_pool, len,
					   ST67W611M1_NET_PKT_ALLOC_TIMEOUT);
	if (new_buf == NULL) {
		net_pkt_unref(pkt);
		return NULL;
	}

	net_buf_add_mem(new_buf, buf, len);
	net_pkt_append_buffer(pkt, new_buf);

	return pkt;
}

/* modem_cmd_handler write cb  */
static int st67_send_at_cmd_to_bus(struct modem_iface *iface, const uint8_t *buf, size_t size)
{
	int ret;

	if (buf == NULL) { /* Empty EOL from cmd_handler */
		return 0;
	}

	struct net_pkt *tx_pkt = alloc_net_pkt_and_copy_buf(buf, size);

	if (tx_pkt == NULL) {
		return -ENOMEM;
	}

	struct st67_spi_pkt *spi_pkt;

	ret = k_mem_slab_alloc(&st67_spi_pkt_mem_slab, (void **)&spi_pkt, K_NO_WAIT);
	if (ret < 0) {
		net_pkt_unref(tx_pkt);
		return ret;
	}

	spi_pkt->net_pkt = tx_pkt;
	spi_pkt->type = AT_CMD;

	ret = st67_spi_send(spi_pkt);
	if (ret < 0) {
		net_pkt_unref(tx_pkt);
		k_mem_slab_free(&st67_spi_pkt_mem_slab, spi_pkt);
		LOG_ERR("can't send to SPI: %d", ret);
		return ret;
	}

	return 0;
}

static void st67_scan_work(struct k_work *work)
{
	int ret;
	struct st67_driver_data *st67_data = CONTAINER_OF(work, struct st67_driver_data, scan_work);

	ret = st67_send_at_cmd(st67_data, NULL, 0, "AT+CWLAP\r\n", ST67W611M1_SCAN_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Scan failed: err %d", ret);
		goto out;
	}

	ret = k_sem_take(&st67_data->sem_wifi_scan_done_wait, ST67W611M1_SCAN_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Scan timed out: %d", ret);
		goto out;
	}

out:
	if (st67_data->scan_cb != NULL) {
		/* Inform wifi mgmt that scan is over. */
		st67_data->scan_cb(st67_data->net_iface, 0, NULL);
		st67_data->scan_cb = NULL;
	}
}

static int st67_scan(const struct device *dev, struct net_if *iface,
		     struct wifi_scan_params *params, scan_result_cb_t cb)
{
	struct st67_driver_data *st67_data = dev->data;

	ARG_UNUSED(params);

	/* Block wifi scan overlapping. */
	if (st67_data->scan_cb != NULL) {
		return -EINPROGRESS;
	}

	st67_data->scan_cb = cb;

	k_work_submit_to_queue(&st67_data->workq, &st67_data->scan_work);
	return 0;
}

static int conn_cmd_append(struct st67_driver_data *st67_data, size_t *off, const char *chunk,
			   size_t chunk_len)
{
	char *str_end = &st67_data->conn_cmd[sizeof(st67_data->conn_cmd)];
	char *str = &st67_data->conn_cmd[*off];
	const char *chunk_end = chunk + chunk_len;

	for (; chunk < chunk_end; chunk++) {
		if (str_end - str < 1) {
			return -ENOSPC;
		}

		*str = *chunk;
		str++;
	}

	*off = str - st67_data->conn_cmd;

	return 0;
}

#define conn_cmd_append_literal(data, off, chunk)                                                  \
	conn_cmd_append(data, off, chunk, sizeof(chunk) - 1)

static int conn_cmd_escape_and_append(struct st67_driver_data *st67_data, size_t *off,
				      const char *chunk, size_t chunk_len)
{
	char *str_end = &st67_data->conn_cmd[sizeof(st67_data->conn_cmd)];
	char *str = &st67_data->conn_cmd[*off];
	const char *chunk_end = chunk + chunk_len;

	for (; chunk < chunk_end; chunk++) {
		switch (*chunk) {
		case ',':
		case '\\':
		case '"':
			if (str_end - str < 2) {
				return -ENOSPC;
			}

			*str = '\\';
			str++;

			break;
		}

		if (str_end - str < 1) {
			return -ENOSPC;
		}

		*str = *chunk;
		str++;
	}

	*off = str - st67_data->conn_cmd;

	return 0;
}

static void st67_connect_work(struct k_work *work)
{
	int ret;
	struct st67_driver_data *st67_data =
		CONTAINER_OF(work, struct st67_driver_data, connect_work);

	ret = st67_send_at_cmd(st67_data, NULL, 0, st67_data->conn_cmd, ST67W611M1_AT_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Failed to send connect cmd");
	}
	memset(st67_data->conn_cmd, 0, sizeof(st67_data->conn_cmd));
}

static int st67_connect(const struct device *dev, struct net_if *iface,
			struct wifi_connect_req_params *params)
{
	struct st67_driver_data *st67_data = dev->data;

	size_t off = 0;
	int err;

	err = conn_cmd_append_literal(st67_data, &off, "AT+CWJAP=\"");

	if (err) {
		return err;
	}
	err = conn_cmd_escape_and_append(st67_data, &off, params->ssid, params->ssid_length);
	if (err) {
		return err;
	}
	err = conn_cmd_append_literal(st67_data, &off, "\",\"");
	if (err) {
		return err;
	}
	err = conn_cmd_escape_and_append(st67_data, &off, params->psk, params->psk_length);
	if (err) {
		return err;
	}
	err = conn_cmd_append_literal(st67_data, &off, "\"\r\n");
	if (err) {
		return err;
	}

	st67_data->conn_cmd[off] = '\0';

	k_work_submit_to_queue(&st67_data->workq, &st67_data->connect_work);
	return 0;
}

static void st67_disconnect_work(struct k_work *work)
{
	struct st67_driver_data *st67_data =
		CONTAINER_OF(work, struct st67_driver_data, disconnect_work);

	st67_send_at_cmd(st67_data, NULL, 0, "AT+CWQAP\r\n", ST67W611M1_AT_CMD_TIMEOUT);
}

static int st67_disconnect(const struct device *dev, struct net_if *iface)
{
	struct st67_driver_data *st67_data = dev->data;

	k_work_submit_to_queue(&st67_data->workq, &st67_data->disconnect_work);
	return 0;
}

static void st67_at_rx(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct st67_driver_data *st67_data = p1;

	while (true) {
		k_sem_take(&st67_data->sem_rx_wait, K_FOREVER);
		modem_cmd_handler_process(&st67_data->mctx.cmd_handler, &st67_data->mctx.iface);

		k_yield();
	}
}

MODEM_CMD_DEFINE(on_cmd_ok)
{
	struct st67_driver_data *st67_data =
		CONTAINER_OF(data, struct st67_driver_data, cmd_handler_data);

	modem_cmd_handler_set_error(data, 0);
	k_sem_give(&st67_data->sem_cmd_response_wait);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_error)
{
	struct st67_driver_data *st67_data =
		CONTAINER_OF(data, struct st67_driver_data, cmd_handler_data);

	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&st67_data->sem_cmd_response_wait);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_ready)
{
	struct st67_driver_data *st67_data =
		CONTAINER_OF(data, struct st67_driver_data, cmd_handler_data);

	k_work_submit_to_queue(&st67_data->workq, &st67_data->init_work);
	return 0;
}

static char *str_unquote(char *str)
{
	char *end;

	if (str[0] != '"') {
		return str;
	}

	str++;

	end = strrchr(str, '"');
	if (end != NULL) {
		*end = 0;
	}

	return str;
}

MODEM_CMD_DEFINE(on_cmd_cipstamac)
{
	int ret = 0;
	struct st67_driver_data *st67_data =
		CONTAINER_OF(data, struct st67_driver_data, cmd_handler_data);

	char *mac = str_unquote(argv[0]);

	ret = net_bytes_from_str(st67_data->sta_mac_addr, ARRAY_SIZE(st67_data->sta_mac_addr), mac);
	return ret;
}

MODEM_CMD_DEFINE(on_cmd_cwnetmode)
{
	struct st67_driver_data *st67_data =
		CONTAINER_OF(data, struct st67_driver_data, cmd_handler_data);
	st67_data->is_supported_st67_firmware_detected = false;

	if (argc < 1) {
		LOG_ERR("Cannot determine firmware version");
		return -EINVAL;
	}
	int val = strtol(argv[0], NULL, 10);

	switch (val) {
	case ST67W611M1_CWNETMODE_T02_VAL:
		st67_data->is_supported_st67_firmware_detected = true;
		return 0;
	case ST67W611M1_CWNETMODE_T01_VAL:
		LOG_ERR("ST67W611M1 T01 firmware not supported. Please flash the T02 firmware.");
		return -ENODEV;
	default:
		LOG_ERR("Unknown ST67W611M1 firmware.");

		return -ENODEV;
	}
}

MODEM_CMD_DEFINE(on_cmd_wifi_scan_done)
{
	struct st67_driver_data *st67_data =
		CONTAINER_OF(data, struct st67_driver_data, cmd_handler_data);

	k_sem_give(&st67_data->sem_wifi_scan_done_wait);
	return 0;
}

static int get_next_int_token(int *dest, char **cursor, int max_len)
{
	char token[max_len + 1];
	int token_len = 0;
	bool comma_found = false;

	while (token_len < max_len) {
		if (**cursor == ',' || (**cursor == ')' && (*cursor)[1] == '\0')) {
			comma_found = true;
			(*cursor)++; /* Skip delimiter. */
			break;
		}
		token[token_len] = **cursor;
		(*cursor)++;
		token_len++;
	}

	if (!comma_found) {
		return -EINVAL;
	}

	token[token_len] = '\0';

	*dest = strtol(token, NULL, 10);
	return 0;
}

static int get_next_quoted_token(char *dest, char **cursor, int max_len)
{
	int token_len = 0;
	bool ending_quote_found = false;

	if ((dest == NULL) || (cursor == NULL)) {
		return -EINVAL;
	}

	if (**cursor != '"') {
		return -EINVAL;
	}

	(*cursor)++;

	while (token_len < max_len) {
		if (**cursor == '"') {
			if (((*cursor)[1] == ',') ||
			    (((*cursor)[1] == ')' && (*cursor)[2] == '\0'))) {
				ending_quote_found = true;
				*cursor += 2; /* Skip both quote and delimiter. */
				break;
			}
		}
		dest[token_len] = **cursor;
		(*cursor)++;
		token_len++;
	}

	if (!ending_quote_found) {
		return -EINVAL;
	}

	dest[token_len] = '\0';

	return token_len;
}

/* Depends on ST67W611M1_CWLAPOPT_CMD */
/* +CWLAP:(ecn,ssid,rssi,mac,channel) */
MODEM_CMD_DEFINE(on_cmd_cwlap)
{
	struct st67_driver_data *st67_data =
		CONTAINER_OF(data, struct st67_driver_data, cmd_handler_data);
	struct wifi_scan_result result = {0};
	int ret;
	char *cursor;

	int security_index;
	int ssid_length;
	int rssi;
	char mac[ST67W611M1_CWLAP_MAC_ADDR_STR_LEN];
	int channel;

	if (argc < 1) {
		return -EBADMSG;
	}
	cursor = argv[0];

	ret = get_next_int_token(&security_index, &cursor, 2);
	if (ret < 0) {
		return ret;
	}
	if (security_index > ARRAY_SIZE(st67w611m1_ap_security_type)) {
		return -EBADMSG;
	}
	result.security = st67w611m1_ap_security_type[security_index];

	/* Scan result won't be correct if it contains ",
	 * This is a known limitation of the AT cmd syntax on the ST67's firmware.
	 * There could also be some issues if the SSID contains \r\n
	 */
	ssid_length = get_next_quoted_token(result.ssid, &cursor, ARRAY_SIZE(result.ssid));
	if (ssid_length < 0) {
		return -EBADMSG;
	}
	result.ssid_length = ssid_length;

	ret = get_next_int_token(&rssi, &cursor, 4);
	if (ret < 0) {
		return ret;
	}
	result.rssi = rssi;

	ret = get_next_quoted_token(mac, &cursor, ARRAY_SIZE(mac));
	if (ret < 0) {
		return ret;
	}
	if (net_bytes_from_str(result.mac, ARRAY_SIZE(result.mac), mac) < 0) {
		return -EBADMSG;
	}

	result.mac_length = WIFI_MAC_ADDR_LEN;

	ret = get_next_int_token(&channel, &cursor, 4);
	if (ret < 0) {
		return ret;
	}
	result.channel = channel;

	if (st67_data->scan_cb) {
		st67_data->scan_cb(st67_data->net_iface, 0, &result);
	}
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_wifi_connecting)
{
	struct st67_driver_data *st67_data =
		CONTAINER_OF(data, struct st67_driver_data, cmd_handler_data);

	if (st67_data->sta_current_state != ST67_DISCONNECTED) {
		LOG_WRN("Unexpected sta state %d", st67_data->sta_current_state);
	}

	st67_data->sta_current_state = ST67_CONNECTING;
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_wifi_connected)
{
	struct st67_driver_data *st67_data =
		CONTAINER_OF(data, struct st67_driver_data, cmd_handler_data);

	if (st67_data->sta_current_state != ST67_CONNECTING) {
		LOG_WRN("Unexpected sta state %d", st67_data->sta_current_state);
	}

	wifi_mgmt_raise_connect_result_event(st67_data->net_iface, WIFI_STATUS_CONN_SUCCESS);
	net_if_dormant_off(st67_data->net_iface);

	st67_data->sta_current_state = ST67_CONNECTED;
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_wifi_disconnected)
{
	struct st67_driver_data *st67_data =
		CONTAINER_OF(data, struct st67_driver_data, cmd_handler_data);

	if (st67_data->sta_current_state != ST67_CONNECTED) {
		LOG_WRN("Unexpected sta state %d", st67_data->sta_current_state);
	}

	wifi_mgmt_raise_disconnect_result_event(st67_data->net_iface, WIFI_REASON_DISCONN_SUCCESS);
	net_if_dormant_on(st67_data->net_iface);

	st67_data->sta_current_state = ST67_DISCONNECTED;
	return 0;
}

/* The AT command says "ERROR" but it's rather a return value received after the
 * connection/disconnection command.
 */
MODEM_CMD_DEFINE(on_cmd_wifi_error)
{
	struct st67_driver_data *st67_data =
		CONTAINER_OF(data, struct st67_driver_data, cmd_handler_data);

	/* 1 is the default fail value. */
	int status = 1;

	enum st67_sta_current_state current_state = st67_data->sta_current_state;

	if (argc > 0) {
		int return_code = strtol(argv[0], NULL, 10);

		if (current_state == ST67_DISCONNECTED) {
			switch (return_code) {
			case ST67W611M1_WIFI_REASON_DISCONN_AP_LEAVING:
				status = WIFI_REASON_DISCONN_AP_LEAVING;
				break;
			case ST67W611M1_WIFI_REASON_DISCONN_AP_LEAVING_2:
				status = WIFI_REASON_DISCONN_AP_LEAVING;
				break;
			case ST67W611M1_WIFI_REASON_DISCONN_USER_REQUEST:
				status = WIFI_REASON_DISCONN_USER_REQUEST;
				break;
			default:
				LOG_ERR("got unknown wifi disconnect code %d", return_code);
			}
		} else if (current_state == ST67_CONNECTING) {
			switch (return_code) {
			case ST67W611M1_WIFI_STATUS_CONN_WRONG_PASSWORD:
				status = WIFI_STATUS_CONN_WRONG_PASSWORD;
				break;
			case ST67W611M1_WIFI_STATUS_CONN_WRONG_PASSWORD_2:
				status = WIFI_STATUS_CONN_WRONG_PASSWORD;
				break;
			case ST67W611M1_WIFI_STATUS_CONN_TIMEOUT:
				status = WIFI_STATUS_CONN_TIMEOUT;
				break;
			case ST67W611M1_WIFI_STATUS_CONN_AP_NOT_FOUND:
				status = WIFI_STATUS_CONN_AP_NOT_FOUND;
				break;
			default:
				LOG_ERR("got unknown wifi connect code %d", return_code);
			}
		} else {
			LOG_ERR("got unknown wifi code %d", return_code);
		}
	}

	if (current_state == ST67_DISCONNECTED) {
		wifi_mgmt_raise_disconnect_result_event(st67_data->net_iface, status);
	} else if (current_state == ST67_CONNECTING) {
		st67_data->sta_current_state = ST67_DISCONNECTED;
		wifi_mgmt_raise_connect_result_event(st67_data->net_iface, status);
	} else {
		LOG_WRN("Unexpected sta state %d", current_state);
	}

	return 0;
}

static const struct modem_cmd response_cmds[] = {
	MODEM_CMD("OK", on_cmd_ok, 0U, ""),
	MODEM_CMD("ERROR", on_cmd_error, 0U, ""),
};

static const struct modem_cmd unsol_cmds[] = {
	MODEM_CMD("ready", on_cmd_ready, 0U, ""),
	MODEM_CMD("+CW:SCAN_DONE", on_cmd_wifi_scan_done, 0U, ""),
	MODEM_CMD("+CWLAP:(", on_cmd_cwlap, 1U, ""),
	MODEM_CMD("+CW:CONNECTED", on_cmd_wifi_connected, 0U, ""),
	MODEM_CMD("+CW:CONNECTING", on_cmd_wifi_connecting, 0U, ""),
	MODEM_CMD("+CW:DISCONNECTED", on_cmd_wifi_disconnected, 0U, ""),
	/* Received at both connection and disconnection. */
	MODEM_CMD("+CW:ERROR,", on_cmd_wifi_error, 1U, ""),
};

static void handle_at_cmd_frame(const uint8_t *buf, size_t len)
{
	struct st67_driver_data *st67_data = &driver_data;

	if (len > ring_buf_space_get(&rx_at_cmd_ring_buf)) {
		LOG_ERR("Not enough space in RX AT cmd ring buf");
		return;
	}

	ring_buf_put(&rx_at_cmd_ring_buf, buf, len);

	k_sem_give(&st67_data->sem_rx_wait);
}

static void handle_sta_data_frame(const uint8_t *buf, size_t len)
{
	struct st67_driver_data *st67_data = &driver_data;
	struct net_if *iface = st67_data->net_iface;
	struct net_pkt *pkt;

	if (iface == NULL) {
		return;
	}

	pkt = net_pkt_rx_alloc_with_buffer(iface, len, NET_AF_UNSPEC, 0,
					   ST67W611M1_NET_PKT_ALLOC_TIMEOUT);
	if (pkt == NULL) {
		LOG_ERR("Failed to alloc for pkt");
		return;
	}

	if (net_pkt_write(pkt, buf, len) < 0) {
		LOG_ERR("Failed to write in pkt");
		net_pkt_unref(pkt);
		return;
	}

	if (net_recv_data(iface, pkt) < 0) {
		LOG_ERR("Failed to send pkt");
		net_pkt_unref(pkt);
		return;
	}
}

static void handle_sap_data_frame(const uint8_t *buf, size_t len)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	LOG_ERR("SoftAP not yet supported");
}

static int st67_send(const struct device *dev, struct net_pkt *pkt)
{
	int ret;

	if ((pkt == NULL) || (pkt->buffer == NULL)) {
		return -EINVAL;
	}

	struct st67_spi_pkt *spi_pkt;

	ret = k_mem_slab_alloc(&st67_spi_pkt_mem_slab, (void **)&spi_pkt, K_NO_WAIT);
	if (ret < 0) {
		return ret;
	}

	spi_pkt->net_pkt = pkt;
	/* For now, the driver only supports STA.
	 * For SoftAP, the iface will be checked to determine the type.
	 */
	spi_pkt->type = STA_DATA;
	net_pkt_ref(pkt);
	ret = st67_spi_send(spi_pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
		k_mem_slab_free(&st67_spi_pkt_mem_slab, spi_pkt);
		return ret;
	}

	return 0;
}

static void st67_init_work(struct k_work *work)
{
	int ret = 0;
	struct st67_driver_data *st67_data = CONTAINER_OF(work, struct st67_driver_data, init_work);

	static const struct setup_cmd cmds[] = {
		SETUP_CMD("AT+CIPSTAMAC?\r\n", "+CIPSTAMAC:", on_cmd_cipstamac, 1U, ""),
		SETUP_CMD_NOHANDLE(ST67W611M1_CWLAPOPT_CMD),
		SETUP_CMD("AT+CWNETMODE?\r\n", "+CWNETMODE:", on_cmd_cwnetmode, 1U, ""),
	};
	ret = modem_cmd_handler_setup_cmds(
		&st67_data->mctx.iface, &st67_data->mctx.cmd_handler, cmds, ARRAY_SIZE(cmds),
		&st67_data->sem_cmd_response_wait, ST67W611M1_AT_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Init failed: %d", ret);
		return;
	}

	k_sem_give(&st67_data->sem_st67_init_over);
}

static int st67_init(const struct device *dev)
{
	int ret;
	struct st67_driver_data *st67_data = dev->data;

	ret = st67_spi_init();
	if (ret < 0) {
		LOG_ERR("SPI init error: %d", ret);
		return ret;
	}

	st67_spi_register_drv_rx_iface_cb(handle_at_cmd_frame, AT_CMD);
	st67_spi_register_drv_rx_iface_cb(handle_sta_data_frame, STA_DATA);
	st67_spi_register_drv_rx_iface_cb(handle_sap_data_frame, SAP_DATA);

	k_sem_init(&st67_data->sem_st67_init_over, 0, 1);
	k_sem_init(&st67_data->sem_cmd_response_wait, 0, 1);
	k_sem_init(&st67_data->sem_wifi_scan_done_wait, 0, 1);
	k_sem_init(&st67_data->sem_rx_wait, 0, 1);

	/* Implemented with work items to return the API calls quickly and not wait for semaphore
	 * (modem_cmd_send())
	 */
	k_work_init(&st67_data->init_work, st67_init_work);
	k_work_init(&st67_data->scan_work, st67_scan_work);
	k_work_init(&st67_data->connect_work, st67_connect_work);
	k_work_init(&st67_data->disconnect_work, st67_disconnect_work);

	k_work_queue_start(&st67_data->workq, st67_workq_stack,
			   K_KERNEL_STACK_SIZEOF(st67_workq_stack),
			   CONFIG_ST67W611M1_WORKQ_THREAD_PRIORITY, NULL);
	k_thread_name_set(st67_data->workq.thread_id, "st67_workq");

	/* cmd handler */
	const struct modem_cmd_handler_config cmd_handler_config = {
		.match_buf = st67_data->cmd_match_buf,
		.match_buf_len = sizeof(st67_data->cmd_match_buf),
		.buf_pool = &at_cmd_net_buf_pool,
		.alloc_timeout = K_NO_WAIT,
		.eol = NULL, /* \r\n is handled in the commands directly for SPI efficiency. */
		.user_data = NULL,
		.response_cmds = response_cmds,
		.response_cmds_len = ARRAY_SIZE(response_cmds),
		.unsol_cmds = unsol_cmds,
		.unsol_cmds_len = ARRAY_SIZE(unsol_cmds),
	};
	ret = modem_cmd_handler_init(&st67_data->mctx.cmd_handler, &st67_data->cmd_handler_data,
				     &cmd_handler_config);
	if (ret < 0) {
		LOG_ERR("modem_cmd_handler_init failed: %d", ret);
		return ret;
	}

	ret = modem_context_register(&st67_data->mctx);
	if (ret < 0) {
		LOG_ERR("modem_context_register failed: %d", ret);
		return ret;
	}

	/* cmd handler cb register
	 * Done this way because cmd handler was designed mainly for UART.
	 * Refer to drivers/modem/modem_iface_uart.h
	 */
	st67_data->mctx.iface.read = st67_receive_at_cmd_from_bus;
	st67_data->mctx.iface.write = st67_send_at_cmd_to_bus;

	/* AT RX thread */
	k_thread_create(&st67_at_rx_thread, st67_at_rx_stack,
			K_KERNEL_STACK_SIZEOF(st67_at_rx_stack), st67_at_rx, st67_data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ST67W611M1_RX_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&st67_at_rx_thread, "st67_at_rx_thread");

	ret = k_sem_take(&st67_data->sem_st67_init_over, ST67W611M1_INIT_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Did not receive ST67 init over signal");
		return ret;
	}

	if (!st67_data->is_supported_st67_firmware_detected) {
		return -ENODEV;
	}

	return 0;
}

static void st67_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct st67_driver_data *st67_data = dev->data;

	st67_data->net_iface = iface;

	net_eth_set_if_type_wifi(iface);

	if (net_if_set_link_addr(st67_data->net_iface, st67_data->sta_mac_addr,
				 ARRAY_SIZE(st67_data->sta_mac_addr), NET_LINK_ETHERNET) < 0) {
		LOG_ERR("Failed to set mac address");
	}

	ethernet_init(iface);
	net_if_dormant_on(iface); /* Wifi network association not yet done. */
}

static const struct wifi_mgmt_ops st67_mgmt_ops = {
	.scan = st67_scan,
	.connect = st67_connect,
	.disconnect = st67_disconnect,
};

static const struct net_wifi_mgmt_offload st67_api = {
	.wifi_iface.iface_api.init = st67_iface_init,
	.wifi_iface.send = st67_send,
	.wifi_mgmt_api = &st67_mgmt_ops,
};

ETH_NET_DEVICE_DT_INST_DEFINE(0, st67_init, NULL, &driver_data, NULL, CONFIG_WIFI_INIT_PRIORITY,
			      &st67_api, ST67W611M1_ETH_MTU);
