/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/types.h>
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

#include <gatt/dis.h>
#include <gatt/bas.h>

#define DEVICE_NAME				CONFIG_BLUETOOTH_DEVICE_NAME
#define DEVICE_NAME_LEN				(sizeof(DEVICE_NAME) - 1)
#define SENSOR_1_NAME				"Temperature Sensor 1"
#define SENSOR_2_NAME				"Temperature Sensor 2"
#define SENSOR_3_NAME				"Humidity Sensor"

/* Sensor Internal Update Interval [seconds] */
#define SENSOR_1_UPDATE_IVAL			5
#define SENSOR_2_UPDATE_IVAL			12
#define SENSOR_3_UPDATE_IVAL			60

/* ESS error definitions */
#define ESS_ERR_WRITE_REJECT			0x80
#define ESS_ERR_COND_NOT_SUPP			0x81

/* ESS Trigger Setting conditions */
#define ESS_TRIGGER_INACTIVE			0x00
#define ESS_FIXED_TIME_INTERVAL			0x01
#define ESS_NO_LESS_THAN_SPECIFIED_TIME		0x02
#define ESS_VALUE_CHANGED			0x03
#define ESS_LESS_THAN_REF_VALUE			0x04
#define ESS_LESS_OR_EQUAL_TO_REF_VALUE		0x05
#define ESS_GREATER_THAN_REF_VALUE		0x06
#define ESS_GREATER_OR_EQUAL_TO_REF_VALUE	0x07
#define ESS_EQUAL_TO_REF_VALUE			0x08
#define ESS_NOT_EQUAL_TO_REF_VALUE		0x09

static inline void int_to_le24(u32_t value, u8_t *u24)
{
	u24[0] = value & 0xff;
	u24[1] = (value >> 8) & 0xff;
	u24[2] = (value >> 16) & 0xff;
}

static inline u32_t le24_to_int(const u8_t *u24)
{
	return ((u32_t)u24[0] |
		(u32_t)u24[1] << 8 |
		(u32_t)u24[2] << 16);
}

static ssize_t read_u16(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, u16_t len, u16_t offset)
{
	const u16_t *u16 = attr->user_data;
	u16_t value = sys_cpu_to_le16(*u16);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

/* Environmental Sensing Service Declaration */

struct es_measurement {
	u16_t flags; /* Reserved for Future Use */
	u8_t sampling_func;
	u32_t meas_period;
	u32_t update_interval;
	u8_t application;
	u8_t meas_uncertainty;
};

struct temperature_sensor {
	s16_t temp_value;

	/* Valid Range */
	s16_t lower_limit;
	s16_t upper_limit;

	/* ES trigger setting - Value Notification condition */
	u8_t condition;
	union {
		u32_t seconds;
		s16_t ref_val; /* Reference temperature */
	};

	struct bt_gatt_ccc_cfg  ccc_cfg[CONFIG_BLUETOOTH_MAX_PAIRED];
	struct es_measurement meas;
};

struct humidity_sensor {
	s16_t humid_value;

	struct es_measurement meas;
};

static bool simulate_temp;
static struct temperature_sensor sensor_1 = {
		.temp_value = 1200,
		.lower_limit = -10000,
		.upper_limit = 10000,
		.condition = ESS_VALUE_CHANGED,
		.meas.sampling_func = 0x00,
		.meas.meas_period = 0x01,
		.meas.update_interval = SENSOR_1_UPDATE_IVAL,
		.meas.application = 0x1c,
		.meas.meas_uncertainty = 0x04,
};

static struct temperature_sensor sensor_2 = {
		.temp_value = 1800,
		.lower_limit = -1000,
		.upper_limit = 5000,
		.condition = ESS_VALUE_CHANGED,
		.meas.sampling_func = 0x00,
		.meas.meas_period = 0x01,
		.meas.update_interval = SENSOR_2_UPDATE_IVAL,
		.meas.application = 0x1b,
		.meas.meas_uncertainty = 0x04,
};

static struct humidity_sensor sensor_3 = {
		.humid_value = 6233,
		.meas.sampling_func = 0x02,
		.meas.meas_period = 0x0e10,
		.meas.update_interval = SENSOR_3_UPDATE_IVAL,
		.meas.application = 0x1c,
		.meas.meas_uncertainty = 0x01,
};

static void temp_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	simulate_temp = value == BT_GATT_CCC_NOTIFY;
}

struct read_es_measurement_rp {
	u16_t flags; /* Reserved for Future Use */
	u8_t sampling_function;
	u8_t measurement_period[3];
	u8_t update_interval[3];
	u8_t application;
	u8_t measurement_uncertainty;
} __packed;

static ssize_t read_es_measurement(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr, void *buf,
				   u16_t len, u16_t offset)
{
	const struct es_measurement *value = attr->user_data;
	struct read_es_measurement_rp rsp;

	rsp.flags = sys_cpu_to_le16(value->flags);
	rsp.sampling_function = value->sampling_func;
	int_to_le24(value->meas_period, rsp.measurement_period);
	int_to_le24(value->update_interval, rsp.update_interval);
	rsp.application = value->application;
	rsp.measurement_uncertainty = value->meas_uncertainty;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &rsp,
				 sizeof(rsp));
}

static ssize_t read_temp_valid_range(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr, void *buf,
				     u16_t len, u16_t offset)
{
	const struct temperature_sensor *sensor = attr->user_data;
	u16_t tmp[] = {sys_cpu_to_le16(sensor->lower_limit),
			  sys_cpu_to_le16(sensor->upper_limit)};

	return bt_gatt_attr_read(conn, attr, buf, len, offset, tmp,
				 sizeof(tmp));
}

struct es_trigger_setting_seconds {
	u8_t condition;
	u8_t sec[3];
} __packed;

struct es_trigger_setting_reference {
	u8_t condition;
	s16_t ref_val;
} __packed;

static ssize_t read_temp_trigger_setting(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 void *buf, u16_t len,
					 u16_t offset)
{
	const struct temperature_sensor *sensor = attr->user_data;

	switch (sensor->condition) {
	/* Operand N/A */
	case ESS_TRIGGER_INACTIVE:
		/* fallthrough */
	case ESS_VALUE_CHANGED:
		return bt_gatt_attr_read(conn, attr, buf, len, offset,
					 &sensor->condition,
					 sizeof(sensor->condition));
	/* Seconds */
	case ESS_FIXED_TIME_INTERVAL:
		/* fallthrough */
	case ESS_NO_LESS_THAN_SPECIFIED_TIME: {
			struct es_trigger_setting_seconds rp;

			rp.condition = sensor->condition;
			int_to_le24(sensor->seconds, rp.sec);

			return bt_gatt_attr_read(conn, attr, buf, len, offset,
						 &rp, sizeof(rp));
		}
	/* Reference temperature */
	default: {
			struct es_trigger_setting_reference rp;

			rp.condition = sensor->condition;
			rp.ref_val = sys_cpu_to_le16(sensor->ref_val);

			return bt_gatt_attr_read(conn, attr, buf, len, offset,
						 &rp, sizeof(rp));
		}
	}
}

static bool check_condition(u8_t condition, s16_t old_val, s16_t new_val,
			    s16_t ref_val)
{
	switch (condition) {
	case ESS_TRIGGER_INACTIVE:
		return false;
	case ESS_FIXED_TIME_INTERVAL:
	case ESS_NO_LESS_THAN_SPECIFIED_TIME:
		/* TODO: Check time requirements */
		return false;
	case ESS_VALUE_CHANGED:
		return new_val != old_val;
	case ESS_LESS_THAN_REF_VALUE:
		return new_val < ref_val;
	case ESS_LESS_OR_EQUAL_TO_REF_VALUE:
		return new_val <= ref_val;
	case ESS_GREATER_THAN_REF_VALUE:
		return new_val > ref_val;
	case ESS_GREATER_OR_EQUAL_TO_REF_VALUE:
		return new_val >= ref_val;
	case ESS_EQUAL_TO_REF_VALUE:
		return new_val == ref_val;
	case ESS_NOT_EQUAL_TO_REF_VALUE:
		return new_val != ref_val;
	default:
		return false;
	}
}

static void update_temperature(struct bt_conn *conn,
			       const struct bt_gatt_attr *chrc, s16_t value,
			       struct temperature_sensor *sensor)
{
	bool notify = check_condition(sensor->condition,
				      sensor->temp_value, value,
				      sensor->ref_val);

	/* Update temperature value */
	sensor->temp_value = value;

	/* Trigger notification if conditions are met */
	if (notify) {
		value = sys_cpu_to_le16(sensor->temp_value);

		bt_gatt_notify(conn, chrc, &value, sizeof(value));
	}
}

static struct bt_gatt_attr ess_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),

	/* Temperature Sensor 1 */
	BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_TEMPERATURE, BT_GATT_PERM_READ,
			   read_u16, NULL, &sensor_1.temp_value),
	BT_GATT_DESCRIPTOR(BT_UUID_ES_MEASUREMENT, BT_GATT_PERM_READ,
			   read_es_measurement, NULL, &sensor_1.meas),
	BT_GATT_CUD(SENSOR_1_NAME, BT_GATT_PERM_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE, BT_GATT_PERM_READ,
			   read_temp_valid_range, NULL, &sensor_1),
	BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING,
			   BT_GATT_PERM_READ, read_temp_trigger_setting,
			   NULL, &sensor_1),
	BT_GATT_CCC(sensor_1.ccc_cfg, temp_ccc_cfg_changed),

	/* Temperature Sensor 2 */
	BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_TEMPERATURE, BT_GATT_PERM_READ,
			   read_u16, NULL, &sensor_2.temp_value),
	BT_GATT_DESCRIPTOR(BT_UUID_ES_MEASUREMENT, BT_GATT_PERM_READ,
			   read_es_measurement, NULL, &sensor_2.meas),
	BT_GATT_CUD(SENSOR_2_NAME, BT_GATT_PERM_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE, BT_GATT_PERM_READ,
			   read_temp_valid_range, NULL, &sensor_2),
	BT_GATT_DESCRIPTOR(BT_UUID_ES_TRIGGER_SETTING,
			   BT_GATT_PERM_READ, read_temp_trigger_setting,
			   NULL, &sensor_2),
	BT_GATT_CCC(sensor_2.ccc_cfg, temp_ccc_cfg_changed),

	/* Humidity Sensor */
	BT_GATT_CHARACTERISTIC(BT_UUID_HUMIDITY, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_HUMIDITY, BT_GATT_PERM_READ,
			   read_u16, NULL, &sensor_3.humid_value),
	BT_GATT_CUD(SENSOR_3_NAME, BT_GATT_PERM_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_ES_MEASUREMENT, BT_GATT_PERM_READ,
			   read_es_measurement, NULL, &sensor_3.meas),
};

static struct bt_gatt_service ess_svc = BT_GATT_SERVICE(ess_attrs);

static void ess_simulate(void)
{
	static u8_t i;
	u16_t val;

	if (!(i % SENSOR_1_UPDATE_IVAL)) {
		val = 1200 + i;
		update_temperature(NULL, &ess_attrs[2], val, &sensor_1);
	}

	if (!(i % SENSOR_2_UPDATE_IVAL)) {
		val = 1800 + i;
		update_temperature(NULL, &ess_attrs[9], val, &sensor_2);
	}

	if (!(i % SENSOR_3_UPDATE_IVAL)) {
		sensor_3.humid_value = 6233 + (i % 13);
	}

	if (!(i % INT8_MAX)) {
		i = 0;
	}

	i++;
}

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, 0x00, 0x03),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x1a, 0x18),
	/* TODO: Include Service Data AD */
};

static struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
	} else {
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
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

	bt_gatt_service_register(&ess_svc);
	bas_init();
	dis_init(CONFIG_SOC, "ACME");

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};

void main(void)
{
	int err;

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb_display);

	while (1) {
		k_sleep(MSEC_PER_SEC);

		/* Temperature simulation */
		if (simulate_temp) {
			ess_simulate();
		}

		/* Battery level simulation */
		bas_notify();
	}
}
