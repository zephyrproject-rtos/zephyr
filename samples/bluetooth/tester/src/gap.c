/* gap.c - Bluetooth GAP Tester */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <string.h>

#include <toolchain.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include "bttester.h"

#define CONTROLLER_INDEX 0
#define CONTROLLER_NAME "btp_tester"

/* TODO add api for reading real address */
#define CONTROLLER_ADDR (&(bt_addr_t) {{1, 2, 3, 4, 5, 6}})

static void le_connected(struct bt_conn *conn)
{
	struct gap_device_connected_ev ev;
	const bt_addr_le_t *addr = bt_conn_get_dst(conn);

	memcpy(ev.address, addr->val, sizeof(ev.address));

	/* Convert addr_type to the type defined by tester */
	switch (addr->type) {
	case BT_ADDR_LE_PUBLIC:
		ev.address_type = BTP_BDADDR_LE_PUBLIC;
		break;
	case BT_ADDR_LE_RANDOM:
		ev.address_type = BTP_BDADDR_LE_RANDOM;
		break;
	default:
		return;
	}

	tester_rsp_full(BTP_SERVICE_ID_GAP, GAP_EV_DEVICE_CONNECTED,
			CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void le_disconnected(struct bt_conn *conn)
{
	struct gap_device_disconnected_ev ev;
	const bt_addr_le_t *addr = bt_conn_get_dst(conn);

	memcpy(ev.address, addr->val, sizeof(ev.address));

	/* Convert addr_type to the type defined by tester */
	switch (addr->type) {
	case BT_ADDR_LE_PUBLIC:
		ev.address_type = BTP_BDADDR_LE_PUBLIC;
		break;
	case BT_ADDR_LE_RANDOM:
		ev.address_type = BTP_BDADDR_LE_RANDOM;
		break;
	default:
		return;
	}

	tester_rsp_full(BTP_SERVICE_ID_GAP, GAP_EV_DEVICE_DISCONNECTED,
			CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static struct bt_conn_cb conn_callbacks = {
		.connected = le_connected,
		.disconnected = le_disconnected,
};

static void supported_commands(uint8_t *data, uint16_t len)
{
	uint16_t cmds;
	struct gap_read_supported_commands_rp *rp = (void *) &cmds;

	cmds = 1 << GAP_READ_SUPPORTED_COMMANDS;
	cmds |= 1 << GAP_READ_CONTROLLER_INDEX_LIST;
	cmds |= 1 << GAP_READ_CONTROLLER_INFO;
	cmds |= 1 << GAP_START_ADVERTISING;
	cmds |= 1 << GAP_STOP_ADVERTISING;
	cmds |= 1 << GAP_START_DISCOVERY;
	cmds |= 1 << GAP_STOP_DISCOVERY;

	tester_rsp_full(BTP_SERVICE_ID_GAP, GAP_READ_SUPPORTED_COMMANDS,
			CONTROLLER_INDEX, (uint8_t *) rp, sizeof(cmds));
}

static void controller_index_list(uint8_t *data,  uint16_t len)
{
	struct gap_read_controller_index_list_rp *rp;
	uint8_t buf[sizeof(*rp) + 1];

	rp = (void *) buf;

	rp->num = 1;
	rp->index[0] = CONTROLLER_INDEX;

	tester_rsp_full(BTP_SERVICE_ID_GAP, GAP_READ_CONTROLLER_INDEX_LIST,
			BTP_INDEX_NONE, (uint8_t *) rp, sizeof(buf));
}

static void controller_info(uint8_t *data, uint16_t len)
{
	struct gap_read_controller_info_rp rp;

	memset(&rp, 0, sizeof(rp));
	memcpy(rp.address, CONTROLLER_ADDR, sizeof(bt_addr_t));

	rp.supported_settings = 1 << GAP_SETTINGS_POWERED;
	rp.supported_settings |= 1 << GAP_SETTINGS_CONNECTABLE;
	rp.supported_settings |= 1 << GAP_SETTINGS_BONDABLE;
	rp.supported_settings |= 1 << GAP_SETTINGS_LE;
	rp.supported_settings |= 1 << GAP_SETTINGS_ADVERTISING;

	rp.current_settings = 1 << GAP_SETTINGS_POWERED;
	rp.current_settings |= 1 << GAP_SETTINGS_CONNECTABLE;
	rp.current_settings |= 1 << GAP_SETTINGS_BONDABLE;
	rp.current_settings |= 1 << GAP_SETTINGS_LE;

	memcpy(rp.name, CONTROLLER_NAME, sizeof(CONTROLLER_NAME));

	tester_rsp_full(BTP_SERVICE_ID_GAP, GAP_READ_CONTROLLER_INFO,
			CONTROLLER_INDEX, (uint8_t *) &rp, sizeof(rp));
}

static void start_advertising(const uint8_t *data, uint16_t len)
{
	const struct gap_start_advertising_cmd *cmd = (void *) data;
	uint8_t status = BTP_STATUS_SUCCESS;

	/* TODO
	 * type should be based on current_settings
	 * convert adv_data and scan_rsp and pass them
	 */
	ARG_UNUSED(cmd);

	if (bt_start_advertising(BT_LE_ADV_IND, NULL, NULL) < 0) {
		status = BTP_STATUS_FAILED;
	}

	tester_rsp(BTP_SERVICE_ID_GAP, GAP_START_ADVERTISING, CONTROLLER_INDEX,
		   status);
}

static void stop_advertising(const uint8_t *data, uint16_t len)
{
	uint8_t status = BTP_STATUS_SUCCESS;

	if (bt_stop_advertising() < 0) {
		status = BTP_STATUS_FAILED;
	}

	tester_rsp(BTP_SERVICE_ID_GAP, GAP_STOP_ADVERTISING, CONTROLLER_INDEX,
		   status);
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t evtype,
			 const uint8_t *ad, uint8_t len)
{
	struct gap_device_found_ev *ev;
	uint8_t buf[sizeof(*ev) + len];

	ev = (void*) buf;

	memcpy(ev->address, addr->val, sizeof(ev->address));
	/* Convert addr_type to the type defined by tester */
	switch (addr->type) {
	case BT_ADDR_LE_PUBLIC:
		ev->address_type = BTP_BDADDR_LE_PUBLIC;
		break;
	case BT_ADDR_LE_RANDOM:
		ev->address_type = BTP_BDADDR_LE_RANDOM;
		break;
	default:
		return;
	}

	ev->flags = GAP_DEVICE_FOUND_FLAG_RSSI;
	ev->rssi = rssi;

	if (evtype == BT_LE_ADV_SCAN_RSP) {
		ev->flags |= GAP_DEVICE_FOUND_FLAG_SD;
	} else {
		ev->flags |= GAP_DEVICE_FOUND_FLAG_AD;
	}

	ev->eir_data_len = len;
	if (len) {
		memcpy(ev->eir_data, ad, len);
	}

	tester_rsp_full(BTP_SERVICE_ID_GAP, GAP_EV_DEVICE_FOUND,
			CONTROLLER_INDEX, buf, sizeof(buf));
}

static void start_discovery(const uint8_t *data, uint16_t len)
{
	const struct gap_start_discovery_cmd *cmd = (void *) data;
	uint8_t status;

	/* only LE scan is supported */
	if (cmd->flags & (~GAP_DISCOVERY_FLAG_LE)) {
		status = BTP_STATUS_FAILED;
		goto reply;
	}

	if (bt_start_scanning(BT_SCAN_FILTER_DUP_ENABLE, device_found) < 0) {
		status = BTP_STATUS_FAILED;
		goto reply;
	}

	status = BTP_STATUS_SUCCESS;
reply:
	tester_rsp(BTP_SERVICE_ID_GAP, GAP_START_DISCOVERY, CONTROLLER_INDEX,
		   status);
}

static void stop_discovery(const uint8_t *data, uint16_t len)
{
	uint8_t status = BTP_STATUS_SUCCESS;

	if (bt_stop_scanning() < 0) {
		status = BTP_STATUS_FAILED;
	}

	tester_rsp(BTP_SERVICE_ID_GAP, GAP_STOP_DISCOVERY, CONTROLLER_INDEX,
		   status);
}

void tester_handle_gap(uint8_t opcode, uint8_t index, uint8_t *data,
		       uint16_t len)
{
	switch (opcode) {
	case GAP_READ_SUPPORTED_COMMANDS:
	case GAP_READ_CONTROLLER_INDEX_LIST:
		if (index != BTP_INDEX_NONE){
			tester_rsp(BTP_SERVICE_ID_GAP, opcode, index,
				   BTP_STATUS_FAILED);
			return;
		}
		break;
	default:
		if (index != CONTROLLER_INDEX){
			tester_rsp(BTP_SERVICE_ID_GAP, opcode, index,
				   BTP_STATUS_FAILED);
			return;
		}
		break;
	}

	switch (opcode) {
	case GAP_READ_SUPPORTED_COMMANDS:
		supported_commands(data, len);
		return;
	case GAP_READ_CONTROLLER_INDEX_LIST:
		controller_index_list(data, len);
		return;
	case GAP_READ_CONTROLLER_INFO:
		controller_info(data, len);
		return;
	case GAP_START_ADVERTISING:
		start_advertising(data, len);
		return;
	case GAP_STOP_ADVERTISING:
		stop_advertising(data, len);
		return;
	case GAP_START_DISCOVERY:
		start_discovery(data, len);
		return;
	case GAP_STOP_DISCOVERY:
		stop_discovery(data, len);
		return;
	default:
		tester_rsp(BTP_SERVICE_ID_GAP, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		return;
	}
}

uint8_t tester_init_gap(void)
{
	if (bt_enable(NULL) < 0) {
		return BTP_STATUS_FAILED;
	}

	bt_conn_cb_register(&conn_callbacks);

	return BTP_STATUS_SUCCESS;
}
