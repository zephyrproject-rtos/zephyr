/* main.c - Bluetooth Cycling Speed and Cadence app main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <gatt/gap.h>
#include <gatt/dis.h>
#include <gatt/bas.h>

#define DEVICE_NAME			CONFIG_BLUETOOTH_DEVICE_NAME
#define DEVICE_NAME_LEN			(sizeof(DEVICE_NAME) - 1)
#define CSC_APPEARANCE			0x0485
#define CSC_SUPPORTED_LOCATIONS		{ CSC_LOC_OTHER, \
					  CSC_LOC_FRONT_WHEEL, \
					  CSC_LOC_REAR_WHEEL, \
					  CSC_LOC_LEFT_CRANK, \
					  CSC_LOC_RIGHT_CRANK }
#define CSC_FEATURE			(CSC_FEAT_WHEEL_REV | \
					 CSC_FEAT_CRANK_REV | \
					 CSC_FEAT_MULTI_SENSORS)

/* CSC Sensor Locations */
#define CSC_LOC_OTHER			0x00
#define CSC_LOC_TOP_OF_SHOE		0x01
#define CSC_LOC_IN_SHOE			0x02
#define CSC_LOC_HIP			0x03
#define CSC_LOC_FRONT_WHEEL		0x04
#define CSC_LOC_LEFT_CRANK		0x05
#define CSC_LOC_RIGHT_CRANK		0x06
#define CSC_LOC_LEFT_PEDAL		0x07
#define CSC_LOC_RIGHT_PEDAL		0x08
#define CSC_LOC_FRONT_HUB		0x09
#define CSC_LOC_REAR_DROPOUT		0x0a
#define CSC_LOC_CHAINSTAY		0x0b
#define CSC_LOC_REAR_WHEEL		0x0c
#define CSC_LOC_REAR_HUB		0x0d
#define CSC_LOC_CHEST			0x0e

/* CSC Application error codes */
#define CSC_ERR_IN_PROGRESS		0x80
#define CSC_ERR_CCC_CONFIG		0x81

/* SC Control Point Opcodes */
#define SC_CP_OP_SET_CWR		0x01
#define SC_CP_OP_CALIBRATION		0x02
#define SC_CP_OP_UPDATE_LOC		0x03
#define SC_CP_OP_REQ_SUPP_LOC		0x04
#define SC_CP_OP_RESPONSE		0x10

/* SC Control Point Response Values */
#define SC_CP_RSP_SUCCESS		0x01
#define SC_CP_RSP_OP_NOT_SUPP		0x02
#define SC_CP_RSP_INVAL_PARAM		0x03
#define SC_CP_RSP_FAILED		0x04

/* CSC Feature */
#define CSC_FEAT_WHEEL_REV		BIT(0)
#define CSC_FEAT_CRANK_REV		BIT(1)
#define CSC_FEAT_MULTI_SENSORS		BIT(2)

/* CSC Measurement Flags */
#define CSC_WHEEL_REV_DATA_PRESENT	BIT(0)
#define CSC_CRANK_REV_DATA_PRESENT	BIT(1)

/* Cycling Speed and Cadence Service declaration */

static struct bt_gatt_ccc_cfg csc_meas_ccc_cfg[CONFIG_BLUETOOTH_MAX_PAIRED];
static struct bt_gatt_ccc_cfg ctrl_point_ccc_cfg[CONFIG_BLUETOOTH_MAX_PAIRED];
static uint32_t cwr; /* Cumulative Wheel Revolutions */
static uint8_t supported_locations[] = CSC_SUPPORTED_LOCATIONS;
static uint8_t sensor_location; /* Current Sensor Location */
static bool csc_simulate;
static bool ctrl_point_configured;

static void csc_meas_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				     uint16_t value)
{
	csc_simulate = value == BT_GATT_CCC_NOTIFY;
}

static void ctrl_point_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				       uint16_t value)
{
	ctrl_point_configured = value == BT_GATT_CCC_INDICATE;
}

static ssize_t read_location(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(*value));
}

static ssize_t read_csc_feature(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	uint16_t csc_feature = CSC_FEATURE;

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &csc_feature, sizeof(csc_feature));
}

static void ctrl_point_ind(struct bt_conn *conn, uint8_t req_op, uint8_t status,
			   const void *data, uint16_t data_len);

struct write_sc_ctrl_point_req {
	uint8_t op;
	union {
		uint32_t cwr;
		uint8_t location;
	};
} __packed;

static ssize_t write_ctrl_point(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	const struct write_sc_ctrl_point_req *req = buf;
	uint8_t status;
	int i;

	if (!ctrl_point_configured) {
		return BT_GATT_ERR(CSC_ERR_CCC_CONFIG);
	}

	if (!len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	switch (req->op) {
	case SC_CP_OP_SET_CWR:
		if (len != sizeof(req->op) + sizeof(req->cwr)) {
			status = SC_CP_RSP_INVAL_PARAM;
			break;
		}

		cwr = sys_le32_to_cpu(req->cwr);
		status = SC_CP_RSP_SUCCESS;
		break;
	case SC_CP_OP_UPDATE_LOC:
		if (len != sizeof(req->op) + sizeof(req->location)) {
			status = SC_CP_RSP_INVAL_PARAM;
			break;
		}

		/* Break if the requested location is the same as current one */
		if (req->location == sensor_location) {
			status = SC_CP_RSP_SUCCESS;
			break;
		}

		/* Pre-set status */
		status = SC_CP_RSP_INVAL_PARAM;

		/* Check if requested location is supported */
		for (i = 0; i < ARRAY_SIZE(supported_locations); i++) {
			if (supported_locations[i] == req->location) {
				sensor_location = req->location;
				status = SC_CP_RSP_SUCCESS;
				break;
			}
		}

		break;
	case SC_CP_OP_REQ_SUPP_LOC:
		if (len != sizeof(req->op)) {
			status = SC_CP_RSP_INVAL_PARAM;
			break;
		}

		/* Indicate supported locations and return */
		ctrl_point_ind(conn, req->op, SC_CP_RSP_SUCCESS,
			       &supported_locations,
			       sizeof(supported_locations));

		return len;
	default:
		status = SC_CP_RSP_OP_NOT_SUPP;
	}

	ctrl_point_ind(conn, req->op, status, NULL, 0);

	return len;
}

static struct bt_gatt_attr csc_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CSC),
	BT_GATT_CHARACTERISTIC(BT_UUID_CSC_MEASUREMENT, BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_CSC_MEASUREMENT, 0x00, NULL, NULL, NULL),
	BT_GATT_CCC(csc_meas_ccc_cfg, csc_meas_ccc_cfg_changed),
	BT_GATT_CHARACTERISTIC(BT_UUID_SENSOR_LOCATION, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_SENSOR_LOCATION, BT_GATT_PERM_READ,
			   read_location, NULL, &sensor_location),
	BT_GATT_CHARACTERISTIC(BT_UUID_CSC_FEATURE, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_CSC_FEATURE, BT_GATT_PERM_READ,
			   read_csc_feature, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_SC_CONTROL_POINT,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE),
	BT_GATT_DESCRIPTOR(BT_UUID_SC_CONTROL_POINT, BT_GATT_PERM_WRITE, NULL,
			   write_ctrl_point, &sensor_location),
	BT_GATT_CCC(ctrl_point_ccc_cfg, ctrl_point_ccc_cfg_changed),
};

struct sc_ctrl_point_ind {
	uint8_t op;
	uint8_t req_op;
	uint8_t status;
	uint8_t data[];
} __packed;

static void ctrl_point_ind(struct bt_conn *conn, uint8_t req_op, uint8_t status,
			   const void *data, uint16_t data_len)
{
	struct sc_ctrl_point_ind *ind;
	uint8_t buf[sizeof(*ind) + data_len];

	ind = (void *) buf;
	ind->op = SC_CP_OP_RESPONSE;
	ind->req_op = req_op;
	ind->status = status;

	/* Send data (supported locations) if present */
	if (data && data_len) {
		memcpy(ind->data, data, data_len);
	}

	bt_gatt_notify(conn, &csc_attrs[9], buf, sizeof(buf));
}

struct csc_measurement_nfy {
	uint8_t flags;
	uint8_t data[];
} __packed;

struct wheel_rev_data_nfy {
	uint32_t cwr;
	uint16_t lwet;
} __packed;

struct crank_rev_data_nfy {
	uint16_t ccr;
	uint16_t lcet;
} __packed;

static void measurement_nfy(struct bt_conn *conn, uint32_t cwr, uint16_t lwet,
			    uint16_t ccr, uint16_t lcet)
{
	struct csc_measurement_nfy *nfy;
	uint8_t buf[sizeof(*nfy) +
		    (cwr ? sizeof(struct wheel_rev_data_nfy) : 0) +
		    (ccr ? sizeof(struct crank_rev_data_nfy) : 0)];
	uint16_t len = 0;

	nfy = (void *) buf;
	nfy->flags = 0;

	/* Send Wheel Revolution data is present */
	if (cwr) {
		struct wheel_rev_data_nfy data;

		nfy->flags |= CSC_WHEEL_REV_DATA_PRESENT;
		data.cwr = sys_cpu_to_le32(cwr);
		data.lwet = sys_cpu_to_le16(lwet);

		memcpy(nfy->data, &data, sizeof(data));
		len += sizeof(data);
	}

	/* Send Crank Revolution data is present */
	if (ccr) {
		struct crank_rev_data_nfy data;

		nfy->flags |= CSC_CRANK_REV_DATA_PRESENT;
		data.ccr = sys_cpu_to_le16(ccr);
		data.lcet = sys_cpu_to_le16(lcet);

		memcpy(nfy->data + len, &data, sizeof(data));
	}

	bt_gatt_notify(NULL, &csc_attrs[2], buf, sizeof(buf));
}

static uint16_t lwet; /* Last Wheel Event Time */
static uint16_t ccr;  /* Cumulative Crank Revolutions */
static uint16_t lcet; /* Last Crank Event Time */

static void csc_simulation(void)
{
	static uint8_t i;
	uint32_t rand = sys_rand32_get();
	bool nfy_crank = false, nfy_wheel = false;

	/* Measurements don't have to be updated every second */
	if (!(i % 2)) {
		lwet += 1050 + rand % 50;
		cwr += 2;
		nfy_wheel = true;
	}

	if (!(i % 3)) {
		lcet += 1000 + rand % 50;
		ccr += 1;
		nfy_crank = true;
	}

	/*
	 * In typical applications, the CSC Measurement characteristic is
	 * notified approximately once per second. This interval may vary
	 * and is determined by the Server and not required to be configurable
	 * by the Client.
	 */
	measurement_nfy(NULL, nfy_wheel ? cwr : 0, nfy_wheel ? lwet : 0,
			nfy_crank ? ccr : 0, nfy_crank ? lcet : 0);

	/*
	 * The Last Crank Event Time value and Last Wheel Event Time roll over
	 * every 64 seconds.
	 */
	if (!(i % 64)) {
		lcet = 0;
		lwet = 0;
		i = 0;
	}

	i++;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
	} else {
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x16, 0x18, 0x0f, 0x18),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	gap_init(DEVICE_NAME, CSC_APPEARANCE);
	bas_init();
	dis_init(CONFIG_SOC, "ACME");
	bt_gatt_register(csc_attrs, ARRAY_SIZE(csc_attrs));

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

void main(void)
{
	int err;

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);

	while (1) {
		k_sleep(MSEC_PER_SEC);

		/* CSC simulation */
		if (csc_simulate) {
			csc_simulation();
		}

		/* Battery level simulation */
		bas_notify();
	}
}
