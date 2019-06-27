/** @file
 *  @brief NRF BLE DFU Service (Legacy)
 */

/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <zephyr/types.h>
#include <misc/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "nrf_dfu.h"

/* References:
 * https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk51.v10.0.0/bledfu_transport_bleservice.html
 * https://developer.nordicsemi.com/nRF51_SDK/nRF51_SDK_v5.x.x/doc/5.2.0/html/a00029.html
 */

#define DFU_OP_RFU		0
#define DFU_OP_START		1
#define DFU_OP_INIT		2
#define DFU_OP_RECV_IMG		3
#define DFU_OP_VALIDATE_IMG	4
#define DFU_OP_ACTIVATE_IMG	5
#define DFU_OP_RESET_SYS	6
#define DFU_OP_IMG_SIZE		7
#define DFU_OP_PKT_NOTIF_REQ	8
#define DFU_OP_RSP_CODE		16
#define DFU_OP_PKT_NOTIF	17

enum {
	RSP_RFU,
	RSP_SUCCESS,
	RSP_INVSTATE,
	RSP_NOTSUP,
	RSP_DATA_SIZE,
	RSP_CRC_ERROR,
	RSP_OP_FAILED
};

enum {
	DFU_STATE_NONE,
	DFU_STATE_BOOTLOADER,
	DFU_STATE_WAIT_START_PKT,
	DFU_STATE_STARTED,
	DFU_STATE_WAIT_INIT_PKT, /* Optional */
	DFU_STATE_RECV,
	DFU_STATE_RECV_DONE,
	DFU_STATE_VALIDATE,
};

struct dfu_start_data {
	u32_t softdevice_len;
	u32_t bootloader_len;
	u32_t application_len;
} __attribute__((packed));

struct dfu_init_data {
	u16_t device_type;
	u16_t device_revision;
	u32_t application_version;
	u16_t softdevice_id_count;
	u16_t softdevice_id[0];
} __attribute__((packed));

static struct bt_uuid_128 nrf_dfu_uuid = BT_UUID_INIT_128(
	0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15,
	0xDE, 0xEF, 0x12, 0x12, 0x30, 0x15, 0x00, 0x00
);

static struct bt_uuid_128 nrf_dfu_control = BT_UUID_INIT_128(
	0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15,
	0xDE, 0xEF, 0x12, 0x12, 0x31, 0x15, 0x00, 0x00
);

static struct bt_uuid_128 nrf_dfu_packet = BT_UUID_INIT_128(
	0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15,
	0xDE, 0xEF, 0x12, 0x12, 0x32, 0x15, 0x00, 0x00
);

static struct bt_uuid_128 nrf_dfu_version = BT_UUID_INIT_128(
	0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15,
	0xDE, 0xEF, 0x12, 0x12, 0x34, 0x15, 0x00, 0x00
);

static int state = DFU_STATE_NONE;
static struct bt_gatt_attr nrf_dfu_attrs[];
static unsigned int exp_size, img_offset;
static u16_t device_type, device_revision;
static u16_t pkt_notif, pkt_count;
static u32_t crc16 = -1;
static struct nrf_dfu_cb *cb;

static void reset_state(struct bt_conn *conn)
{
	state = DFU_STATE_NONE;
	if (conn) {
		bt_conn_disconnect(conn, 0);
	}
}

static ssize_t write_control(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     const void *buf, u16_t len, u16_t offset,
			     u8_t flags)
{
	u8_t opcode = ((u8_t *)buf)[0];
	u8_t rsp[3];

	/* prepare notification response */
	rsp[0] = DFU_OP_RSP_CODE;
	rsp[1] = opcode;
	rsp[2] = RSP_INVSTATE;

	switch (opcode) {
	case DFU_OP_START:
	{
		int err = 0;

		if (len != 2) {
			rsp[2] = RSP_OP_FAILED;
			bt_gatt_notify(conn, attr, &rsp, sizeof(rsp));
			break;
		}

		if (cb && cb->start) {
			err = cb->start(((u8_t *)buf)[1]);
		}

		if (state != DFU_STATE_BOOTLOADER
		    && cb && cb->reset_to_boot && !err) {
			/* Switch to bootloader mode */
			bt_conn_disconnect(conn, 0);
			state = DFU_STATE_BOOTLOADER;
			cb->reset_to_boot();
			break;
		}

		if (err) {
			rsp[2] = RSP_OP_FAILED;
			bt_gatt_notify(conn, attr, &rsp, sizeof(rsp));
			reset_state(conn);
		} else {
			pkt_notif = pkt_count = exp_size = img_offset = 0;
			state = DFU_STATE_WAIT_START_PKT;
		}

		break;
	}
	case DFU_OP_INIT:
	{
		if (state != DFU_STATE_STARTED) {
			bt_gatt_notify(conn, attr, &rsp, sizeof(rsp));
			reset_state(conn);
		} else {
			state = DFU_STATE_WAIT_INIT_PKT;
		}

		break;
	}
	case DFU_OP_RECV_IMG:
	{
		if (state != DFU_STATE_STARTED) {
			bt_gatt_notify(conn, attr, &rsp, sizeof(rsp));
			reset_state(conn);
		} else {
			state = DFU_STATE_RECV;
		}

		break;
	}
	case DFU_OP_VALIDATE_IMG:
	{
		if (state != DFU_STATE_RECV_DONE) {
			reset_state(conn);
		} else if (cb && cb->validate && cb->validate(crc16)) {
			rsp[2] = RSP_CRC_ERROR;
		} else {
			state = DFU_STATE_VALIDATE;
			rsp[2] = RSP_SUCCESS;
		}

		bt_gatt_notify(conn, attr, &rsp, sizeof(rsp));

		if (rsp[2] != RSP_SUCCESS) {
			reset_state(conn);
		}

		break;
	}
	case DFU_OP_ACTIVATE_IMG:
	{
		if (state != DFU_STATE_VALIDATE) {
			bt_gatt_notify(conn, attr, &rsp, sizeof(rsp));
			reset_state(conn);
		} else {
			reset_state(conn);

			if (cb && cb->reset_to_app) {
				/* Boot to app */
				cb->reset_to_app();
			}
		}
		break;
	}
	case DFU_OP_PKT_NOTIF_REQ:
	{
		/* pkt notification request */
		if (len != 3) {
			break;
		}

		pkt_notif = sys_le16_to_cpu(*(u16_t *)((u8_t *)buf + 1));

		break;
	}
	default: /* Unknown OPCODE */
		bt_gatt_notify(conn, attr, &rsp, sizeof(rsp));
	}

	return len;
}

static ssize_t write_packet(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr,
			    const void *buf, u16_t len, u16_t offset,
			    u8_t flags)
{
	switch (state) {
	case DFU_STATE_WAIT_START_PKT:
	{
		u8_t rsp[] = {DFU_OP_RSP_CODE, DFU_OP_START, RSP_SUCCESS};
		const struct dfu_start_data *data = buf;

		if (len != sizeof(struct dfu_start_data)) {
			rsp[2] = RSP_OP_FAILED;
		} else {
			state = DFU_STATE_STARTED;
		}

		/* Only support Application update */
		exp_size = sys_le32_to_cpu(data->application_len);
		img_offset = 0;

		bt_gatt_notify(conn,  &nrf_dfu_attrs[1], &rsp, sizeof(rsp));

		if (rsp[2] != RSP_SUCCESS) {
			reset_state(conn);
		}

		break;
	}
	case DFU_STATE_WAIT_INIT_PKT:
	{
		u8_t rsp[] = {DFU_OP_RSP_CODE, DFU_OP_INIT, RSP_SUCCESS};
		const struct dfu_init_data *init = buf;

		if (len < sizeof(*init) ||
		    len < (sizeof(*init) + init->softdevice_id_count)) {
			rsp[2] = RSP_OP_FAILED;
		} else {
			/* Go back to started state */
			state = DFU_STATE_STARTED;
		}

		/* Check device_type if any */
		if ((device_type != 0) && (device_type != init->device_type)) {
			rsp[2] = RSP_OP_FAILED;
		}

		/* Check device_revision if any */
		if ((device_revision != 0) &&
		    (device_revision != init->device_revision)) {
			rsp[2] = RSP_OP_FAILED;
		}

		/* do not care about softDevice, CRC1-16-CCIT is last field */
		crc16 = sys_le16_to_cpu(init->softdevice_id[init->softdevice_id_count]);

		bt_gatt_notify(conn,  &nrf_dfu_attrs[1], &rsp, sizeof(rsp));

		if (rsp[2] != RSP_SUCCESS) {
			reset_state(conn);
		}

		break;
	}
	case DFU_STATE_RECV:
	{
		int err = 0;

		if (cb && cb->receive) {
			err = cb->receive(img_offset, buf, len);
		}

		exp_size -= len;
		img_offset += len;
		pkt_count++;

		if (pkt_notif && !(pkt_count % pkt_notif)) {
			u8_t rsp[] = { DFU_OP_PKT_NOTIF,
				       0x00, 0x00, 0x00, 0x00 };
			u32_t *bytes = (u32_t *)&rsp[2];

			*bytes = sys_cpu_to_le32(img_offset);

			/* Report Received Image Size */
			bt_gatt_notify(conn,  &nrf_dfu_attrs[1], &rsp,
				       sizeof(rsp));
		}

		if (exp_size == 0 || err) {
			u8_t rsp[] = { DFU_OP_RSP_CODE, DFU_OP_RECV_IMG,
				       err ? RSP_OP_FAILED : RSP_SUCCESS };

			state = DFU_STATE_RECV_DONE;

			bt_gatt_notify(conn,  &nrf_dfu_attrs[1], &rsp,
				       sizeof(rsp));

			if (rsp[2] != RSP_SUCCESS) {
				reset_state(conn);
			}
		}

		break;
	}
	default:
		return -EINVAL;
	}

	return len;
}

static ssize_t read_version(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr,
			    void *buf, u16_t len, u16_t offset)
{
	u16_t version;

	if (cb && cb->reset_to_boot && state != DFU_STATE_BOOTLOADER) {
		version = 0x0001; /* 0.1 (app mode) */
	} else {
		version = 0x0004; /* 0.4 (bootloader mode) */
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &version,
				 sizeof(version));
}

static struct bt_gatt_ccc_cfg control_ccc_cfg[BT_GATT_CCC_MAX] = {};

static struct bt_gatt_attr nrf_dfu_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(&nrf_dfu_uuid),
	BT_GATT_CHARACTERISTIC(&nrf_dfu_control.uuid,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_WRITE,
			       NULL, write_control, NULL),
	BT_GATT_CCC(control_ccc_cfg, NULL),
	BT_GATT_CHARACTERISTIC(&nrf_dfu_packet.uuid,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE,
			       NULL, write_packet, NULL),
	BT_GATT_CHARACTERISTIC(&nrf_dfu_version.uuid,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_version, NULL, NULL),
};

static struct bt_gatt_service nrf_dfu_svc = BT_GATT_SERVICE(nrf_dfu_attrs);

void nrf_dfu_init(struct nrf_dfu_cb *callbacks, u16_t type, u16_t revision)
{
	cb = callbacks;
	device_type = type;
	device_revision = revision;

	bt_gatt_service_register(&nrf_dfu_svc);
}
