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
#include <misc/printk.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#define DEVICE_NAME CONFIG_BLUETOOTH_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define NUMBER_OF_SLOTS 1
#define EDS_VERSION 0x00
#define EDS_URL_READ_OFFSET 2
#define EDS_URL_WRITE_OFFSET 4
#define EDS_IDLE_TIMEOUT K_SECONDS(30)

/* Idle timer */
struct k_delayed_work idle_work;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	/* Eddystone Service UUID a3c87500-8ed3-4bdf-8a39-a01bebede295 */
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
		      0xdf, 0x4b, 0xd3, 0x8e, 0x00, 0x75, 0xc8, 0xa3),
};

/* Set Scan Response data */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/* Eddystone Service Variables */
/* Service UUID a3c87500-8ed3-4bdf-8a39-a01bebede295 */
static struct bt_uuid_128 eds_uuid = BT_UUID_INIT_128(
	0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
	0xdf, 0x4b, 0xd3, 0x8e, 0x00, 0x75, 0xc8, 0xa3);

/* Characteristic UUID a3c87501-8ed3-4bdf-8a39-a01bebede295 */
static struct bt_uuid_128 eds_caps_uuid = BT_UUID_INIT_128(
	0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
	0xdf, 0x4b, 0xd3, 0x8e, 0x01, 0x75, 0xc8, 0xa3);

/* Characteristic UUID a3c87502-8ed3-4bdf-8a39-a01bebede295 */
static struct bt_uuid_128 eds_slot_uuid = BT_UUID_INIT_128(
	0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
	0xdf, 0x4b, 0xd3, 0x8e, 0x02, 0x75, 0xc8, 0xa3);

/* Characteristic UUID a3c87503-8ed3-4bdf-8a39-a01bebede295 */
static struct bt_uuid_128 eds_intv_uuid = BT_UUID_INIT_128(
	0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
	0xdf, 0x4b, 0xd3, 0x8e, 0x03, 0x75, 0xc8, 0xa3);

/* Characteristic UUID a3c87504-8ed3-4bdf-8a39-a01bebede295 */
static struct bt_uuid_128 eds_tx_uuid = BT_UUID_INIT_128(
	0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
	0xdf, 0x4b, 0xd3, 0x8e, 0x04, 0x75, 0xc8, 0xa3);

/* Characteristic UUID a3c87505-8ed3-4bdf-8a39-a01bebede295 */
static struct bt_uuid_128 eds_adv_tx_uuid = BT_UUID_INIT_128(
	0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
	0xdf, 0x4b, 0xd3, 0x8e, 0x05, 0x75, 0xc8, 0xa3);

/* Characteristic UUID a3c87506-8ed3-4bdf-8a39-a01bebede295 */
static struct bt_uuid_128 eds_lock_uuid = BT_UUID_INIT_128(
	0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
	0xdf, 0x4b, 0xd3, 0x8e, 0x06, 0x75, 0xc8, 0xa3);

/* Characteristic UUID a3c87507-8ed3-4bdf-8a39-a01bebede295 */
static struct bt_uuid_128 eds_unlock_uuid = BT_UUID_INIT_128(
	0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
	0xdf, 0x4b, 0xd3, 0x8e, 0x07, 0x75, 0xc8, 0xa3);

/* Characteristic UUID a3c87508-8ed3-4bdf-8a39-a01bebede295 */
static struct bt_uuid_128 eds_ecdh_uuid = BT_UUID_INIT_128(
	0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
	0xdf, 0x4b, 0xd3, 0x8e, 0x08, 0x75, 0xc8, 0xa3);

/* Characteristic UUID a3c87509-8ed3-4bdf-8a39-a01bebede295 */
static struct bt_uuid_128 eds_eid_uuid = BT_UUID_INIT_128(
	0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
	0xdf, 0x4b, 0xd3, 0x8e, 0x09, 0x75, 0xc8, 0xa3);

/* Characteristic UUID a3c8750a-8ed3-4bdf-8a39-a01bebede295 */
static struct bt_uuid_128 eds_data_uuid = BT_UUID_INIT_128(
	0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
	0xdf, 0x4b, 0xd3, 0x8e, 0x0a, 0x75, 0xc8, 0xa3);

/* Characteristic UUID a3c8750b-8ed3-4bdf-8a39-a01bebede295 */
static struct bt_uuid_128 eds_reset_uuid = BT_UUID_INIT_128(
	0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
	0xdf, 0x4b, 0xd3, 0x8e, 0x0b, 0x75, 0xc8, 0xa3);

/* Characteristic UUID a3c8750c-8ed3-4bdf-8a39-a01bebede295 */
static struct bt_uuid_128 eds_connectable_uuid = BT_UUID_INIT_128(
	0x95, 0xe2, 0xed, 0xeb, 0x1b, 0xa0, 0x39, 0x8a,
	0xdf, 0x4b, 0xd3, 0x8e, 0x0c, 0x75, 0xc8, 0xa3);

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
	u8_t version;
	u8_t slots;
	u8_t uids;
	u8_t adv_types;
	u16_t slot_types;
	u8_t tx_power;
} __packed;

static struct eds_capabilities eds_caps = {
	.version = EDS_VERSION,
	.slots = NUMBER_OF_SLOTS,
	.slot_types = EDS_SLOT_URL, /* TODO: Add support for other slot types */
};

u8_t eds_active_slot;

enum {
	EDS_LOCKED = 0x00,
	EDS_UNLOCKED = 0x01,
	EDS_UNLOCKED_NO_RELOCKING = 0x02,
};

struct eds_slot {
	u8_t type;
	u8_t state;
	u8_t connectable;
	u16_t interval;
	u8_t tx_power;
	u8_t adv_tx_power;
	u8_t lock[16];
	u8_t challenge[16];
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
			 void *buf, u16_t len, u16_t offset)
{
	const struct eds_capabilities *caps = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, caps,
				 sizeof(*caps));
}

static ssize_t read_slot(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &eds_active_slot, sizeof(eds_active_slot));
}

static ssize_t write_slot(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, const void *buf,
			  u16_t len, u16_t offset, u8_t flags)
{
	u8_t value;

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
			     void *buf, u16_t len, u16_t offset)
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
			      const void *buf, u16_t len, u16_t offset,
			      u8_t flags)
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
				 void *buf, u16_t len, u16_t offset)
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
				  const void *buf, u16_t len,
				  u16_t offset,
				  u8_t flags)
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
			     void *buf, u16_t len, u16_t offset)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];

	if (slot->state == EDS_LOCKED) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &slot->interval,
				 sizeof(slot->interval));
}

static ssize_t read_lock(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &slot->state,
				 sizeof(slot->state));
}

static ssize_t write_lock(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, const void *buf,
			  u16_t len, u16_t offset, u8_t flags)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];
	u8_t value;

	if (slot->state == EDS_LOCKED) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
	}

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	/* Write 1 byte to lock or 17 bytes to transition to a new lock state */
	if (len != 1) {
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
			   void *buf, u16_t len, u16_t offset)
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
			    u16_t len, u16_t offset, u8_t flags)
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

static u8_t eds_ecdh[32] = {}; /* TODO: Add ECDH key */

static ssize_t read_ecdh(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	u8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(eds_ecdh));
}

static u8_t eds_eid[16] = {}; /* TODO: Add EID key */

static ssize_t read_eid(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, u16_t len, u16_t offset)
{
	u8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(eds_eid));
}

static ssize_t read_adv_data(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     u16_t len, u16_t offset)
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

static int eds_slot_restart(struct eds_slot *slot, u8_t type)
{
	int err;

	/* Restart advertising */
	bt_le_adv_stop();

	if (type == EDS_TYPE_NONE) {
		/* Restore connectable if slot */
		err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
				      sd, ARRAY_SIZE(sd));
	} else {
		err = bt_le_adv_start(BT_LE_ADV_NCONN, slot->ad,
				      ARRAY_SIZE(slot->ad), sd, ARRAY_SIZE(sd));
	}

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return err;
	}

	slot->type = type;

	return 0;
}

static ssize_t write_adv_data(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, u16_t len, u16_t offset,
			      u8_t flags)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];
	u8_t type;

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
		slot->ad[2].data_len = min(slot->ad[2].data_len,
					   len + EDS_URL_WRITE_OFFSET);
		memcpy(&slot->ad[2].data + EDS_URL_WRITE_OFFSET, buf,
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
			   const void *buf, u16_t len, u16_t offset,
			   u8_t flags)
{
	/* TODO: Power cycle or reload for storage the values */
	return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
}

static ssize_t read_connectable(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     u16_t len, u16_t offset)
{
	u8_t connectable = 0x01;

	/* Returning a non-zero value indicates that the beacon is capable
	 * of becoming non-connectable
	 */
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &connectable, sizeof(connectable));
}

static ssize_t write_connectable(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf, u16_t len, u16_t offset,
				 u8_t flags)
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
static struct bt_gatt_attr eds_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(&eds_uuid),
	/* Capabilities */
	BT_GATT_CHARACTERISTIC(&eds_caps_uuid.uuid, BT_GATT_CHRC_READ),
	/* Readable only when unlocked. Never writable. */
	BT_GATT_DESCRIPTOR(&eds_caps_uuid.uuid, BT_GATT_PERM_READ,
			   read_caps, NULL, &eds_caps),
	/* Active slot */
	BT_GATT_CHARACTERISTIC(&eds_slot_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
	/* Must be unlocked for both read and write. */
	BT_GATT_DESCRIPTOR(&eds_slot_uuid.uuid,
			   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   read_slot, write_slot, NULL),
	/* Advertising Interval */
	BT_GATT_CHARACTERISTIC(&eds_intv_uuid.uuid, BT_GATT_CHRC_READ),
	/* Must be unlocked for both read and write. */
	BT_GATT_DESCRIPTOR(&eds_intv_uuid.uuid, BT_GATT_PERM_READ,
			   read_interval, NULL, NULL),
	/* Radio TX Power */
	BT_GATT_CHARACTERISTIC(&eds_tx_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
	/* Must be unlocked for both read and write. */
	BT_GATT_DESCRIPTOR(&eds_tx_uuid.uuid,
			   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   read_tx_power, write_tx_power, NULL),
	/* Advertised TX Power */
	BT_GATT_CHARACTERISTIC(&eds_adv_tx_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
	/* Must be unlocked for both read and write. */
	BT_GATT_DESCRIPTOR(&eds_adv_tx_uuid.uuid,
			   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   read_adv_tx_power, write_adv_tx_power, NULL),
	/* Lock State */
	BT_GATT_CHARACTERISTIC(&eds_lock_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
	/* Readable in locked or unlocked state.
	 * Writeable only in unlocked state.
	 */
	BT_GATT_DESCRIPTOR(&eds_lock_uuid.uuid,
			   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   read_lock, write_lock, NULL),
	/* Unlock */
	BT_GATT_CHARACTERISTIC(&eds_unlock_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
	/* Readable only in locked state.
	 * Writeable only in locked state.
	 */
	BT_GATT_DESCRIPTOR(&eds_unlock_uuid.uuid,
			   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   read_unlock, write_unlock, NULL),
	/* Public ECDH Key */
	BT_GATT_CHARACTERISTIC(&eds_ecdh_uuid.uuid, BT_GATT_CHRC_READ),
	/* Readable only in unlocked state. Never writable. */
	BT_GATT_DESCRIPTOR(&eds_ecdh_uuid.uuid, BT_GATT_PERM_READ,
			   read_ecdh, NULL, &eds_ecdh),
	/* EID Identity Key */
	BT_GATT_CHARACTERISTIC(&eds_eid_uuid.uuid, BT_GATT_CHRC_READ),
	/* Readable only in unlocked state. Never writable. */
	BT_GATT_DESCRIPTOR(&eds_eid_uuid.uuid, BT_GATT_PERM_READ,
			   read_eid, NULL, eds_eid),
	/* ADV Slot Data */
	BT_GATT_CHARACTERISTIC(&eds_data_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
	/* Must be unlocked for both read and write. */
	BT_GATT_DESCRIPTOR(&eds_eid_uuid.uuid,
			   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   read_adv_data, write_adv_data, NULL),
	/* ADV Factory Reset */
	BT_GATT_CHARACTERISTIC(&eds_reset_uuid.uuid,  BT_GATT_CHRC_WRITE),
	/* Must be unlocked write. */
	BT_GATT_DESCRIPTOR(&eds_reset_uuid.uuid,
			   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   NULL, write_reset, NULL),
	/* ADV Remain Connectable */
	BT_GATT_CHARACTERISTIC(&eds_connectable_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
	/* Must be unlocked for write. */
	BT_GATT_DESCRIPTOR(&eds_connectable_uuid.uuid,
			   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   read_connectable, write_connectable, NULL),
};

static struct bt_gatt_service eds_svc = BT_GATT_SERVICE(eds_attrs);

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bt_gatt_service_register(&eds_svc);

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	k_delayed_work_submit(&idle_work, EDS_IDLE_TIMEOUT);

	printk("Configuration mode: waiting connections...\n");
}

static void idle_timeout(struct k_work *work)
{
	if (eds_slots[eds_active_slot].type == EDS_TYPE_NONE) {
		printk("Switching to Beacon mode.\n");
		eds_slot_restart(&eds_slots[eds_active_slot], EDS_TYPE_URL);
	}
}

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
	} else {
		printk("Connected\n");
		k_delayed_work_cancel(&idle_work);
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	struct eds_slot *slot = &eds_slots[eds_active_slot];

	printk("Disconnected (reason %u)\n", reason);

	if (!slot->connectable) {
		k_delayed_work_submit(&idle_work, 0);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void main(void)
{
	int err;

	bt_conn_cb_register(&conn_callbacks);
	k_delayed_work_init(&idle_work, idle_timeout);

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
}
