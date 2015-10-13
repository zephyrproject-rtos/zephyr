/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

#define DEVICE_NAME		"Test peripheral"
#define HEART_RATE_APPEARANCE	0x0341

static struct bt_uuid gap_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_GAP,
};

static struct bt_uuid device_name_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_GAP_DEVICE_NAME,
};

static struct bt_gatt_chrc name_chrc = {
	.properties = BT_GATT_CHRC_READ,
	.value_handle = 0x0003,
	.uuid = &device_name_uuid,
};

static int read_name(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		     void *buf, uint16_t len, uint16_t offset)
{
	const char *name = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, name,
				 strlen(name));
}

static struct bt_uuid appeareance_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_GAP_APPEARANCE,
};

static struct bt_gatt_chrc appearance_chrc = {
	.properties = BT_GATT_CHRC_READ,
	.value_handle = 0x0005,
	.uuid = &appeareance_uuid,
};

static int read_appearance(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	uint16_t appearance = sys_cpu_to_le16(HEART_RATE_APPEARANCE);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &appearance,
				 sizeof(appearance));
}

static struct bt_uuid hrs_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_HRS,
};

static struct bt_uuid hrmc_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_HRS_MEASUREMENT,
};

static struct bt_gatt_chrc hrmc_chrc = {
	.properties = BT_GATT_CHRC_NOTIFY,
	.value_handle = 0x0008,
	.uuid = &hrmc_uuid,
};

static struct bt_uuid bslc_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_HRS_BODY_SENSOR,
};

static struct bt_gatt_chrc bslc_chrc = {
	.properties = BT_GATT_CHRC_READ,
	.value_handle = 0x000b,
	.uuid = &bslc_uuid,
};

static struct bt_uuid hrcpc_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_HRS_CONTROL_POINT,
};

static struct bt_gatt_chrc hrcpc_chrc = {
	.properties = BT_GATT_CHRC_WRITE,
	.value_handle = 0x000d,
	.uuid = &hrcpc_uuid,
};

static struct bt_gatt_ccc_cfg hrmc_ccc_cfg[CONFIG_BLUETOOTH_MAX_PAIRED] = {};
static uint8_t simulate_hrm = 0;

static void hrmc_ccc_cfg_changed(uint16_t value)
{
	simulate_hrm = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static int read_blsc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		     void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value = 0x01;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

/* Battery Service Variables */
static struct bt_uuid bas_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_BAS,
};

static struct bt_uuid blvl_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_BAS_BATTERY_LEVEL,
};

static struct bt_gatt_chrc blvl_chrc = {
	.properties = BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
	.value_handle = 0x0010,
	.uuid = &blvl_uuid,
};

static struct bt_gatt_ccc_cfg  blvl_ccc_cfg[CONFIG_BLUETOOTH_MAX_PAIRED] = {};
static uint8_t simulate_blvl = 0;
static uint8_t battery = 100;

static void blvl_ccc_cfg_changed(uint16_t value)
{
	simulate_blvl = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static int read_blvl(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		     void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(*value));
}

/* Current Time Service Variables */
static struct bt_uuid cts_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_CTS,
};

static struct bt_uuid ct_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_CTS_CURRENT_TIME,
};

static struct bt_gatt_chrc ct_chrc = {
	.properties =  BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY |
		BT_GATT_CHRC_WRITE,
	.value_handle = 0x0014,
	.uuid = &ct_uuid,
};

static void generate_current_time(uint8_t *buf)
{
	uint16_t year;

	/* 'Exact Time 256' contains 'Day Date Time' which contains
	 * 'Date Time' - characteristic contains fields for:
	 * year, month, day, hours, minutes and seconds.
	 */

	year = sys_cpu_to_le16(2015);
	memcpy(buf,  &year, 2); /* year */
	buf[2] = 5; /* months starting from 1 */
	buf[3] = 30; /* day */
	buf[4] = 12; /* hours */
	buf[5] = 45; /* minutes */
	buf[6] = 30; /* seconds */

	/* 'Day of Week' part of 'Day Date Time' */
	buf[7] = 1; /* day of week starting from 1 */

	/* 'Fractions 256 part of 'Exact Time 256' */
	buf[8] = 0;

	/* Adjust reason */
	buf[9] = 0; /* No update, change, etc */
}

static struct bt_gatt_ccc_cfg ct_ccc_cfg[CONFIG_BLUETOOTH_MAX_PAIRED] = {};

static void ct_ccc_cfg_changed(uint16_t value)
{
	/* TODO: Handle value */
}

static uint8_t ct[10];
static uint8_t ct_update = 0;

static int read_ct(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		   void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(ct));
}

static int write_ct(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		    const void *buf, uint16_t len, uint16_t offset)
{
	uint8_t *value = attr->user_data;

	if (offset + len > sizeof(ct)) {
		return -EINVAL;
	}

	memcpy(value + offset, buf, len);
	ct_update = 1;

	return len;
}

/* Device Information Service Variables */
static struct bt_uuid dis_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_DIS,
};

static struct bt_uuid model_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_DIS_MODEL_NUMBER_STRING,
};

static struct bt_gatt_chrc model_chrc = {
	.properties = BT_GATT_CHRC_READ,
	.value_handle = 0x0018,
	.uuid = &model_uuid,
};

static int read_model(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		   void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

static struct bt_uuid manuf_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_DIS_MANUFACTURER_NAME_STRING,
};

static struct bt_gatt_chrc manuf_chrc = {
	.properties = BT_GATT_CHRC_READ,
	.value_handle = 0x001a,
	.uuid = &manuf_uuid,
};

static int read_manuf(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		      void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

/* Custom Service Variables */
static struct bt_uuid vnd_uuid = {
	.type = BT_UUID_128,
	.u128 = { 0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
		  0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12 },
};

static struct bt_uuid vnd_enc_uuid = {
	.type = BT_UUID_128,
	.u128 = { 0xf1, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
		  0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12 },
};

static struct bt_gatt_chrc vnd_enc_chrc = {
	.properties = BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
	.value_handle = 0x001d,
	.uuid = &vnd_enc_uuid,
};

static struct bt_uuid vnd_auth_uuid = {
	.type = BT_UUID_128,
	.u128 = { 0xf2, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
		  0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12 },
};

static struct bt_gatt_chrc vnd_auth_chrc = {
	.properties = BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
	.value_handle = 0x001f,
	.uuid = &vnd_auth_uuid,
};

static uint8_t vnd_value[] = { 'V', 'e', 'n', 'd', 'o', 'r' };

static int read_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		   void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

static int write_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		     const void *buf, uint16_t len, uint16_t offset)
{
	uint8_t *value = attr->user_data;

	if (offset + len > sizeof(vnd_value)) {
		return -EINVAL;
	}

	memcpy(value + offset, buf, len);

	return len;
}

static struct vnd_long_value {
	/* TODO: buffer needs to be per connection */
	uint8_t buf[BT_BUF_MAX_DATA];
	uint8_t data[BT_BUF_MAX_DATA];
} vnd_long_value = {
	.buf = { 'V', 'e', 'n', 'd', 'o', 'r' },
	.data = { 'V', 'e', 'n', 'd', 'o', 'r' },
};

static int read_long_vnd(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	struct vnd_long_value *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value->data,
				 sizeof(value->data));
}

static int write_long_vnd(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, const void *buf,
			  uint16_t len, uint16_t offset)
{
	struct vnd_long_value *value = attr->user_data;

	if (offset + len > sizeof(value->buf)) {
		return -EINVAL;
	}

	/* Copy to buffer */
	memcpy(value->buf + offset, buf, len);

	return len;
}

static int flush_long_vnd(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, uint8_t flags)
{
	struct vnd_long_value *value = attr->user_data;

	switch (flags) {
	case BT_GATT_FLUSH_DISCARD:
		/* Discard buffer reseting it back with data */
		memcpy(value->buf, value->data, sizeof(value->buf));
		return 0;
	case BT_GATT_FLUSH_SYNC:
		/* Sync buffer to data */
		memcpy(value->data, value->buf, sizeof(value->data));
		return 0;
	}

	return -EINVAL;
}

static const struct bt_uuid vnd_long_uuid = {
	.type = BT_UUID_128,
	.u128 = { 0xf3, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
		  0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12 },
};

static struct bt_gatt_chrc vnd_long_chrc = {
	.properties = BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
		      BT_GATT_CHRC_EXT_PROP,
	.value_handle = 0x0021,
	.uuid = &vnd_long_uuid,
};

static struct bt_gatt_cep vnd_long_cep = {
	.properties = BT_GATT_CEP_RELIABLE_WRITE,
};

static int signed_value;

static int read_signed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		       void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(signed_value));
}

static int write_signed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t offset)
{
	uint8_t *value = attr->user_data;

	if (offset + len > sizeof(signed_value)) {
		return -EINVAL;
	}

	memcpy(value + offset, buf, len);

	return len;
}

static const struct bt_uuid vnd_signed_uuid = {
	.type = BT_UUID_128,
	.u128 = { 0xf3, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x13,
		  0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x13 },
};

static struct bt_gatt_chrc vnd_signed_chrc = {
	.properties = BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
		      BT_GATT_CHRC_AUTH,
	.value_handle = 0x0024,
	.uuid = &vnd_signed_uuid,
};

static const struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(0x0001, &gap_uuid),
	BT_GATT_CHARACTERISTIC(0x0002, &name_chrc),
	BT_GATT_DESCRIPTOR(0x0003, &device_name_uuid, BT_GATT_PERM_READ,
			  read_name, NULL, DEVICE_NAME),
	BT_GATT_CHARACTERISTIC(0x0004, &appearance_chrc),
	BT_GATT_DESCRIPTOR(0x0005, &appeareance_uuid, BT_GATT_PERM_READ,
			   read_appearance, NULL, NULL),
	/* Heart Rate Service Declaration */
	BT_GATT_PRIMARY_SERVICE(0x0006, &hrs_uuid),
	BT_GATT_CHARACTERISTIC(0x0007, &hrmc_chrc),
	BT_GATT_DESCRIPTOR(0x0008, &hrmc_uuid, BT_GATT_PERM_READ, NULL, NULL,
			   NULL),
	BT_GATT_CCC(0x0009, 0x0008, hrmc_ccc_cfg, hrmc_ccc_cfg_changed),
	BT_GATT_CHARACTERISTIC(0x000a, &bslc_chrc),
	BT_GATT_DESCRIPTOR(0x000b, &bslc_uuid, BT_GATT_PERM_READ, read_blsc,
			   NULL, NULL),
	BT_GATT_CHARACTERISTIC(0x000c, &hrcpc_chrc),
	/* TODO: Add write permission and callback */
	BT_GATT_DESCRIPTOR(0x000d, &hrcpc_uuid, BT_GATT_PERM_READ, NULL, NULL,
			   NULL),
	/* Battery Service Declaration */
	BT_GATT_PRIMARY_SERVICE(0x000e, &bas_uuid),
	BT_GATT_CHARACTERISTIC(0x000f, &blvl_chrc),
	BT_GATT_DESCRIPTOR(0x0010, &blvl_uuid, BT_GATT_PERM_READ, read_blvl,
			   NULL, &battery),
	BT_GATT_CCC(0x0011, 0x0010, blvl_ccc_cfg, blvl_ccc_cfg_changed),
	/* Current Time Service Declaration */
	BT_GATT_PRIMARY_SERVICE(0x0012, &cts_uuid),
	BT_GATT_CHARACTERISTIC(0x0013, &ct_chrc),
	BT_GATT_DESCRIPTOR(0x0014, &ct_uuid,
			   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   read_ct, write_ct, ct),
	BT_GATT_CCC(0x0015, 0x0014, ct_ccc_cfg, ct_ccc_cfg_changed),
	/* Device Information Service Declaration */
	BT_GATT_PRIMARY_SERVICE(0x0016, &dis_uuid),
	BT_GATT_CHARACTERISTIC(0x0017, &model_chrc),
	BT_GATT_DESCRIPTOR(0x0018, &model_uuid, BT_GATT_PERM_READ,
			   read_model, NULL, CONFIG_PLATFORM),
	BT_GATT_CHARACTERISTIC(0x0019, &manuf_chrc),
	BT_GATT_DESCRIPTOR(0x001a, &manuf_uuid, BT_GATT_PERM_READ,
			   read_manuf, NULL, "Manufacturer"),
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(0x001b, &vnd_uuid),
	BT_GATT_CHARACTERISTIC(0x001c, &vnd_enc_chrc),
	BT_GATT_DESCRIPTOR(0x001d, &vnd_enc_uuid,
			   BT_GATT_PERM_READ | BT_GATT_PERM_READ_ENCRYPT |
			   BT_GATT_PERM_WRITE | BT_GATT_PERM_WRITE_ENCRYPT,
			   read_vnd, write_vnd, vnd_value),
	BT_GATT_CHARACTERISTIC(0x001e, &vnd_auth_chrc),
	BT_GATT_DESCRIPTOR(0x001f, &vnd_auth_uuid,
			   BT_GATT_PERM_READ | BT_GATT_PERM_READ_AUTHEN |
			   BT_GATT_PERM_WRITE | BT_GATT_PERM_WRITE_AUTHEN,
			   read_vnd, write_vnd, vnd_value),
	BT_GATT_CHARACTERISTIC(0x0020, &vnd_long_chrc),
	BT_GATT_LONG_DESCRIPTOR(0x0021, &vnd_long_uuid,
			   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   read_long_vnd, write_long_vnd, flush_long_vnd,
			   &vnd_long_value),
	BT_GATT_CEP(0x0022, &vnd_long_cep),
	BT_GATT_CHARACTERISTIC(0x0023, &vnd_signed_chrc),
	BT_GATT_DESCRIPTOR(0x0024, &vnd_signed_uuid,
			   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   read_signed, write_signed, &signed_value),
};

static const struct bt_eir ad[] = {
	{
		.len = 2,
		.type = BT_EIR_FLAGS,
		.data = { BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR },
	},
	{
		.len = 7,
		.type = BT_EIR_UUID16_ALL,
		.data = { 0x0d, 0x18, 0x0f, 0x18, 0x05, 0x18 },
	},
	{
		.len = 17,
		.type = BT_EIR_UUID128_ALL,
		.data = { 0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
			  0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12 },
	},
	{ }
};

static const struct bt_eir sd[] = {
	{
		.len = 16,
		.type = BT_EIR_NAME_COMPLETE,
		.data = DEVICE_NAME,
	},
	{ }
};

static void connected(struct bt_conn *conn)
{
	printk("Connected\n");
}

static void disconnected(struct bt_conn *conn)
{
	printk("Disconnected\n");
}

static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_start_advertising(BT_LE_ADV_IND, ad, sd);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

#ifdef CONFIG_MICROKERNEL
void mainloop(void)
#else
void main(void)
#endif
{
	int err;

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	/* Simulate current time for Current Time Service */
	generate_current_time(ct);

	bt_gatt_register(attrs, ARRAY_SIZE(attrs));

	bt_conn_cb_register(&conn_callbacks);

	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	while (1) {
		static uint8_t hrm[2];

		task_sleep(sys_clock_ticks_per_sec);

		/* Current Time Service updates only when time is changed */
		if (ct_update) {
			ct_update = 0;
			bt_gatt_notify(0x0014, &ct, sizeof(ct));
		}

		/* Heartrate measurements simulation */
		if (simulate_hrm) {
			hrm[0] = 0x06; /* uint8, sensor contact */
			hrm[1] = 90 + (sys_rand32_get() % 20);

			bt_gatt_notify(0x0008, &hrm, sizeof(hrm));
		}

		/* Battery level simulation */
		if (simulate_blvl) {
			battery -= 1;

			if (!battery) {
				/* Software eco battery charger */
				battery = 100;
			}

			bt_gatt_notify(0x0010, &battery, sizeof(battery));
		}
	}
}
