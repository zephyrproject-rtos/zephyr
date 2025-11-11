/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#define NUMBER_OF_SLOTS 1
#define EDS_VERSION 0x00
#define EDS_URL_READ_OFFSET 2
#define EDS_URL_WRITE_OFFSET 4
#define EDS_IDLE_TIMEOUT K_SECONDS(30)

/* Idle timer */
struct k_work_delayable idle_work;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	/* Eddystone Service UUID a3c87500-8ed3-4bdf-8a39-a01bebede295 */
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
		      0xdf, 0x4b, 0xd3, 0x8e, 0x00, 0x75, 0xc8, 0xa3),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

/* Eddystone Service Variables */
/* Service UUID a3c87500-8ed3-4bdf-8a39-a01bebede295 */
static const struct bt_uuid_128 eds_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xa3c87500, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87501-8ed3-4bdf-8a39-a01bebede295 */
static const struct bt_uuid_128 eds_caps_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xa3c87501, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87502-8ed3-4bdf-8a39-a01bebede295 */
static const struct bt_uuid_128 eds_slot_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xa3c87502, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87503-8ed3-4bdf-8a39-a01bebede295 */
static const struct bt_uuid_128 eds_intv_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xa3c87503, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87504-8ed3-4bdf-8a39-a01bebede295 */
static const struct bt_uuid_128 eds_tx_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xa3c87504, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87505-8ed3-4bdf-8a39-a01bebede295 */
static const struct bt_uuid_128 eds_adv_tx_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xa3c87505, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87506-8ed3-4bdf-8a39-a01bebede295 */
static const struct bt_uuid_128 eds_lock_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xa3c87506, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87507-8ed3-4bdf-8a39-a01bebede295 */
static const struct bt_uuid_128 eds_unlock_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xa3c87507, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87508-8ed3-4bdf-8a39-a01bebede295 */
static const struct bt_uuid_128 eds_ecdh_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xa3c87508, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87509-8ed3-4bdf-8a39-a01bebede295 */
static const struct bt_uuid_128 eds_eid_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xa3c87509, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c8750a-8ed3-4bdf-8a39-a01bebede295 */
static const struct bt_uuid_128 eds_data_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xa3c8750a, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c8750b-8ed3-4bdf-8a39-a01bebede295 */
static const struct bt_uuid_128 eds_reset_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xa3c8750b, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c8750c-8ed3-4bdf-8a39-a01bebede295 */
static const struct bt_uuid_128 eds_connectable_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xa3c8750c, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

enum {
	EDS_TYPE_UID = 0x00,
	EDS_TYPE_URL = 0x10,
	EDS_TYPE_TLM = 0x20,
	EDS_TYPE_EID = 0x30,
	EDS_TYPE_NONE = 0xff,
};

enum {
	EDS_SLOT_UID = sys_cpu_to_be16(BIT(0)),
	EDS_SLOT_URL = sys_cpu_to_be16(BIT(1)),
	EDS_SLOT_TLM = sys_cpu_to_be16(BIT(2)),
	EDS_SLOT_EID = sys_cpu_to_be16(BIT(3)),
};

struct eds_capabilities {
	uint8_t version;
	uint8_t slots;
	uint8_t uids;
	uint8_t adv_types;
	uint16_t slot_types;
	uint8_t tx_power;
} __packed;

static struct eds_capabilities eds_caps = {
	.version = EDS_VERSION,
	.slots = NUMBER_OF_SLOTS,
	.slot_types = EDS_SLOT_URL, /* TODO: Add support for other slot types */
};

uint8_t eds_active_slot;

enum {
	EDS_LOCKED = 0x00,
	EDS_UNLOCKED = 0x01,
	EDS_UNLOCKED_NO_RELOCKING = 0x02,
};

struct eds_slot {
	uint8_t type;
	uint8_t state;
	uint8_t connectable;
	uint16_t interval;
	uint8_t tx_power;
	uint8_t adv_tx_power;
	uint8_t lock[16];
	uint8_t challenge[16];
	struct bt_data ad[3];
};

static struct eds_slot eds_slots[NUMBER_OF_SLOTS] = {
	[0 ... (NUMBER_OF_SLOTS - 1)] = {
		.type = EDS_TYPE_NONE,  /* Start as disabled */
		.state = EDS_UNLOCKED, /* Start unlocked */
		.interval = sys_cpu_to_be16(BT_GAP_ADV_FAST_INT_MIN_2),
		.lock = { 'Z', 'e', 'p', 'h', 'y', 'r', ' ', 'E', 'd', 'd',
			  'y', 's', 't', 'o', 'n', 'e' },
		.challenge = {},
		.ad = {
			BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
			BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
			BT_DATA_BYTES(BT_DATA_SVC_DATA16,
				      0xaa, 0xfe, /* Eddystone UUID */
				      0x10, /* Eddystone-URL frame type */
				      0x00, /* Calibrated Tx power at 0m */
				      0x00, /* URL Scheme Prefix http://www. */
				      'z', 'e', 'p', 'h', 'y', 'r',
				      'p', 'r', 'o', 'j', 'e', 'c', 't',
				      0x08) /* .org */
		},
	},
};

static ssize_t read_caps(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	const struct eds_capabilities *caps = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, caps,
				 sizeof(*caps));
}

static ssize_t read_slot(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &eds_active_slot, sizeof(eds_active_slot));
}

static ssize_t write_slot(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, const void *buf,
			  uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t value;

	if (offset + len > sizeof(value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(&value, buf, len);

	if (value + 1 > NUMBER_OF_SLOTS) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
	}

	eds_active_slot = value;

	return len;
}

static ssize_t read_tx_power(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];

	if (slot->state == EDS_LOCKED) {
		return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &slot->tx_power,
				 sizeof(slot->tx_power));
}

static ssize_t write_tx_power(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset,
			      uint8_t flags)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];

	if (slot->state == EDS_LOCKED) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
	}

	if (offset + len > sizeof(slot->tx_power)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(&slot->tx_power, buf, len);

	return len;
}

static ssize_t read_adv_tx_power(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 void *buf, uint16_t len, uint16_t offset)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];

	if (slot->state == EDS_LOCKED) {
		return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &slot->tx_power,
				 sizeof(slot->tx_power));
}

static ssize_t write_adv_tx_power(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  const void *buf, uint16_t len,
				  uint16_t offset,
				  uint8_t flags)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];

	if (slot->state == EDS_LOCKED) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
	}

	if (offset + len > sizeof(slot->adv_tx_power)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(&slot->adv_tx_power, buf, len);

	return len;
}

static ssize_t read_interval(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];

	if (slot->state == EDS_LOCKED) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &slot->interval,
				 sizeof(slot->interval));
}

static ssize_t read_lock(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &slot->state,
				 sizeof(slot->state));
}

static ssize_t write_lock(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, const void *buf,
			  uint16_t len, uint16_t offset, uint8_t flags)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];
	uint8_t value;

	if (slot->state == EDS_LOCKED) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
	}

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	/* Write 1 byte to lock or 17 bytes to transition to a new lock state */
	if (len != 1U) {
		/* TODO: Allow setting new lock code, using AES-128-ECB to
		 * decrypt with the existing lock code and set the unencrypted
		 * value as the new code.
		 */
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&value, buf, sizeof(value));

	if (value > EDS_UNLOCKED_NO_RELOCKING) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
	}

	slot->state = value;

	return len;
}

static ssize_t read_unlock(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr,
			   void *buf, uint16_t len, uint16_t offset)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];

	if (slot->state != EDS_LOCKED) {
		return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
	}

	/* returns a 128-bit challenge token. This token is for one-time use
	 * and cannot be replayed.
	 */
	if (bt_rand(slot->challenge, sizeof(slot->challenge))) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, slot->challenge,
				 sizeof(slot->challenge));
}

static ssize_t write_unlock(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, const void *buf,
			    uint16_t len, uint16_t offset, uint8_t flags)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];

	if (slot->state != EDS_LOCKED) {
		return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
	}

	/* TODO: accepts a 128-bit encrypted value that verifies the client
	 * knows the beacon's lock code.
	 */

	return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
}

static uint8_t eds_ecdh[32] = {}; /* TODO: Add ECDH key */

static ssize_t read_ecdh(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(eds_ecdh));
}

static uint8_t eds_eid[16] = {}; /* TODO: Add EID key */

static ssize_t read_eid(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(eds_eid));
}

static ssize_t read_adv_data(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];

	if (slot->state == EDS_LOCKED) {
		return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
	}

	/* If the slot is currently not broadcasting, reading the slot data
	 * shall return either an empty array or a single byte of 0x00.
	 */
	if (slot->type == EDS_TYPE_NONE) {
		return 0;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 slot->ad[2].data + EDS_URL_READ_OFFSET,
				 slot->ad[2].data_len - EDS_URL_READ_OFFSET);
}

static int eds_slot_restart(struct eds_slot *slot, uint8_t type)
{
	int err;
	char addr_s[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr = {0};

	/* Restart advertising */
	bt_le_adv_stop();

	if (type == EDS_TYPE_NONE) {
		struct bt_le_oob oob;

		/* Restore connectable if slot */
		if (bt_le_oob_get_local(BT_ID_DEFAULT, &oob) == 0) {
			addr = oob.addr;
		}

		err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	} else {
		size_t count = 1;

		bt_id_get(&addr, &count);
		err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, slot->ad,
				      ARRAY_SIZE(slot->ad), NULL, 0);
	}

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return err;
	}

	bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));
	printk("Advertising as %s\n", addr_s);

	slot->type = type;

	return 0;
}

static ssize_t write_adv_data(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset,
			      uint8_t flags)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];
	uint8_t type;

	if (slot->state == EDS_LOCKED) {
		return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
	}

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	/* Writing an empty array, clears the slot and stops Tx. */
	if (!len) {
		eds_slot_restart(slot, EDS_TYPE_NONE);
		return len;
	}

	/* Write length: 17 bytes (UID), 19 bytes (URL), 1 byte (TLM), 34 or
	 * 18 bytes (EID)
	 */
	if (len > 19) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&type, buf, sizeof(type));

	switch (type) {
	case EDS_TYPE_URL:
		/* written data is just the frame type and any ID-related
		 * information, and doesn't include the Tx power since that is
		 * controlled by characteristics 4 (Radio Tx Power) and
		 * 5 (Advertised Tx Power).
		 */
		slot->ad[2].data_len = MIN(slot->ad[2].data_len,
					   len + EDS_URL_WRITE_OFFSET);
		memcpy((uint8_t *) slot->ad[2].data + EDS_URL_WRITE_OFFSET, buf,
		       slot->ad[2].data_len - EDS_URL_WRITE_OFFSET);

		/* Restart slot */
		if (eds_slot_restart(slot, type) < 0) {
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		return len;
	case EDS_TYPE_UID:
	case EDS_TYPE_TLM:
	case EDS_TYPE_EID:
	default:
		/* TODO: Add support for other types. */
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
	}
}

static ssize_t write_reset(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr,
			   const void *buf, uint16_t len, uint16_t offset,
			   uint8_t flags)
{
	/* TODO: Power cycle or reload for storage the values */
	return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
}

static ssize_t read_connectable(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	uint8_t connectable = 0x01;

	/* Returning a non-zero value indicates that the beacon is capable
	 * of becoming non-connectable
	 */
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &connectable, sizeof(connectable));
}

static ssize_t write_connectable(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset,
				 uint8_t flags)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];

	if (slot->state == EDS_LOCKED) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
	}

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len > sizeof(slot->connectable)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	/* If any non-zero value is written, the beacon shall remain in its
	 * connectable state until any other value is written.
	 */
	memcpy(&slot->connectable, buf, len);

	return len;
}

/* Eddystone Configuration Service Declaration */
BT_GATT_SERVICE_DEFINE(eds_svc,
	BT_GATT_PRIMARY_SERVICE(&eds_uuid),
	/* Capabilities: Readable only when unlocked. Never writable. */
	BT_GATT_CHARACTERISTIC(&eds_caps_uuid.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_caps, NULL, &eds_caps),
	/* Active slot: Must be unlocked for both read and write. */
	BT_GATT_CHARACTERISTIC(&eds_slot_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_slot, write_slot, NULL),
	/* Advertising Interval: Must be unlocked for both read and write. */
	BT_GATT_CHARACTERISTIC(&eds_intv_uuid.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_interval, NULL, NULL),
	/* Radio TX Power: Must be unlocked for both read and write. */
	BT_GATT_CHARACTERISTIC(&eds_tx_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_tx_power, write_tx_power, NULL),
	/* Advertised TX Power: Must be unlocked for both read and write. */
	BT_GATT_CHARACTERISTIC(&eds_adv_tx_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_adv_tx_power, write_adv_tx_power, NULL),
	/* Lock State:
	 * Readable in locked or unlocked state.
	 * Writeable only in unlocked state.
	 */
	BT_GATT_CHARACTERISTIC(&eds_lock_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_lock, write_lock, NULL),
	/* Unlock:
	 * Readable only in locked state.
	 * Writeable only in locked state.
	 */
	BT_GATT_CHARACTERISTIC(&eds_unlock_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_unlock, write_unlock, NULL),
	/* Public ECDH Key: Readable only in unlocked state. Never writable. */
	BT_GATT_CHARACTERISTIC(&eds_ecdh_uuid.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_ecdh, NULL, &eds_ecdh),
	/* EID Identity Key:Readable only in unlocked state. Never writable. */
	BT_GATT_CHARACTERISTIC(&eds_eid_uuid.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_eid, NULL, eds_eid),
	/* ADV Slot Data: Must be unlocked for both read and write. */
	BT_GATT_CHARACTERISTIC(&eds_data_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_adv_data, write_adv_data, NULL),
	/* ADV Factory Reset: Must be unlocked for write. */
	BT_GATT_CHARACTERISTIC(&eds_reset_uuid.uuid,  BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE, NULL, write_reset, NULL),
	/* ADV Remain Connectable: Must be unlocked for write. */
	BT_GATT_CHARACTERISTIC(&eds_connectable_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_connectable, write_connectable, NULL),
);

static void bt_ready(int err)
{
	char addr_s[BT_ADDR_LE_STR_LEN];
	struct bt_le_oob oob;

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	/* Restore connectable if slot */
	bt_le_oob_get_local(BT_ID_DEFAULT, &oob);
	bt_addr_le_to_str(&oob.addr, addr_s, sizeof(addr_s));
	printk("Initial advertising as %s\n", addr_s);

	k_work_schedule(&idle_work, EDS_IDLE_TIMEOUT);

	printk("Configuration mode: waiting connections...\n");
}

static void idle_timeout(struct k_work *work)
{
	if (eds_slots[eds_active_slot].type == EDS_TYPE_NONE) {
		printk("Switching to Beacon mode %u.\n", eds_active_slot);
		eds_slot_restart(&eds_slots[eds_active_slot], EDS_TYPE_URL);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed err 0x%02x %s\n", err, bt_hci_err_to_str(err));
	} else {
		printk("Connected\n");
		k_work_cancel_delayable(&idle_work);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];

	printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));

	if (!slot->connectable) {
		k_work_reschedule(&idle_work, K_NO_WAIT);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

int main(void)
{
	int err;

	k_work_init_delayable(&idle_work, idle_timeout);

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
	return 0;
}
