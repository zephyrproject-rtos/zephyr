/*
 * Copyright (c) 2026 Zephyr SNMP Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_mib.h"
#include "app_led.h"
#include "app_sensor.h"
#include "app_button.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/snmp/snmp.h>
#include <zephyr/net/snmp/snmp_mib.h>
#include <string.h>
#include <errno.h>

LOG_MODULE_REGISTER(app_mib, LOG_LEVEL_INF);

#ifndef CONFIG_SNMP_SAMPLE_ENTERPRISE_ID
#define CONFIG_SNMP_SAMPLE_ENTERPRISE_ID 54321
#endif

/* -------------------------------------------------------------------------- */
/* Scalar Node Callbacks                                                      */
/* -------------------------------------------------------------------------- */

/* 1. sensorValue (1.3.6.1.4.1.<enterprise>.1.1.1.0) - INTEGER */
static int cb_get_sensor_value(struct snmp_agent *agent, const struct snmp_mib_node *node,
			       struct snmp_varbind *vb)
{
	ARG_UNUSED(agent);
	ARG_UNUSED(node);
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = app_sensor_get_temp();
	return 0;
}

static int cb_set_sensor_value(struct snmp_agent *agent, struct snmp_mib_node *node,
			       const struct snmp_varbind *vb)
{
	ARG_UNUSED(agent);
	ARG_UNUSED(node);
	if (vb->type != ASN1_TAG_INTEGER) {
		return -EINVAL;
	}
	app_sensor_set_temp(vb->val.int_val);
	return 0;
}

/* 2. ledState (1.3.6.1.4.1.<enterprise>.1.1.2.0) - INTEGER (0=OFF, 1=ON) */
static int cb_get_led_state(struct snmp_agent *agent, const struct snmp_mib_node *node,
			    struct snmp_varbind *vb)
{
	ARG_UNUSED(agent);
	ARG_UNUSED(node);
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = app_led_get_state();
	return 0;
}

static int cb_set_led_state(struct snmp_agent *agent, struct snmp_mib_node *node,
			    const struct snmp_varbind *vb)
{
	ARG_UNUSED(agent);
	ARG_UNUSED(node);
	if (vb->type != ASN1_TAG_INTEGER) {
		LOG_WRN("SNMP SET ledState rejected: invalid ASN.1 type 0x%02X", vb->type);
		return -EINVAL;
	}
	int ret = app_led_set_state(vb->val.int_val);
	if (ret < 0) {

		LOG_WRN("SNMP SET ledState rejected invalid value: %d", vb->val.int_val);
		return -EINVAL; /* Maps to badValue in SNMP response */
	}
	return 0;
}

/* 3. buttonCounter (1.3.6.1.4.1.<enterprise>.1.1.3.0) - Counter32 */
static int cb_get_button_counter(struct snmp_agent *agent, const struct snmp_mib_node *node,
				 struct snmp_varbind *vb)
{
	ARG_UNUSED(agent);
	ARG_UNUSED(node);
	vb->type = SNMP_TAG_COUNTER32;
	vb->val.uint_val = app_button_get_counter();
	return 0;
}

/* 4. sensorHumidity (1.3.6.1.4.1.<enterprise>.1.1.4.0) - Gauge32 */
static int cb_get_sensor_humidity(struct snmp_agent *agent, const struct snmp_mib_node *node,
				  struct snmp_varbind *vb)
{
	ARG_UNUSED(agent);
	ARG_UNUSED(node);
	vb->type = SNMP_TAG_GAUGE32;
	vb->val.uint_val = app_sensor_get_humidity();
	return 0;
}

/* 5. sensorDescription (1.3.6.1.4.1.<enterprise>.1.1.5.0) - OCTET STRING */
static int cb_get_sensor_desc(struct snmp_agent *agent, const struct snmp_mib_node *node,
			      struct snmp_varbind *vb)
{
	ARG_UNUSED(agent);
	ARG_UNUSED(node);
	vb->type = ASN1_TAG_OCTET_STRING;
	const char *desc = app_sensor_get_description();
	size_t len = strlen(desc);
	if (len > sizeof(vb->val.octet_str.data)) {

		len = sizeof(vb->val.octet_str.data);
	}
	memcpy(vb->val.octet_str.data, desc, len);
	vb->val.octet_str.len = (uint16_t)len;
	return 0;
}

/* 6. sensorStatusOid (1.3.6.1.4.1.<enterprise>.1.1.6.0) - OBJECT IDENTIFIER */
static int cb_get_sensor_status_oid(struct snmp_agent *agent, const struct snmp_mib_node *node,
				    struct snmp_varbind *vb)
{
	ARG_UNUSED(agent);
	ARG_UNUSED(node);
	vb->type = ASN1_TAG_OID;
	/* Points to sensorValue: 1.3.6.1.4.1.<enterprise>.1.1.1.0 */
	vb->val.oid.len = 11;
	vb->val.oid.ids[0] = 1;
	vb->val.oid.ids[1] = 3;
	vb->val.oid.ids[2] = 6;
	vb->val.oid.ids[3] = 1;
	vb->val.oid.ids[4] = 4;
	vb->val.oid.ids[5] = 1;
	vb->val.oid.ids[6] = CONFIG_SNMP_SAMPLE_ENTERPRISE_ID;
	vb->val.oid.ids[7] = 1;
	vb->val.oid.ids[8] = 1;
	vb->val.oid.ids[9] = 1;
	vb->val.oid.ids[10] = 0;
	return 0;
}

/* -------------------------------------------------------------------------- */
/* Table Node Callbacks: sensorTable (1.3.6.1.4.1.<enterprise>.1.3.1)         */
/* -------------------------------------------------------------------------- */

static int cb_table_col1_get(struct snmp_agent *agent, const struct snmp_table_row *row,
			     uint8_t column, struct snmp_varbind *vb)
{
	ARG_UNUSED(agent);
	ARG_UNUSED(column);
	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = (int32_t)row->index;
	return 0;
}

static int cb_table_col2_get(struct snmp_agent *agent, const struct snmp_table_row *row,
			     uint8_t column, struct snmp_varbind *vb)
{
	ARG_UNUSED(agent);
	ARG_UNUSED(column);
	const char *name = NULL;
	int ret = app_sensor_get_table_entry(row->index, &name, NULL);
	if (ret < 0 || !name) {

		return -ENOENT;
	}

	vb->type = ASN1_TAG_OCTET_STRING;
	size_t len = strlen(name);
	if (len > sizeof(vb->val.octet_str.data)) {

		len = sizeof(vb->val.octet_str.data);
	}
	memcpy(vb->val.octet_str.data, name, len);
	vb->val.octet_str.len = (uint16_t)len;
	return 0;
}

static int cb_table_col3_get(struct snmp_agent *agent, const struct snmp_table_row *row,
			     uint8_t column, struct snmp_varbind *vb)
{
	ARG_UNUSED(agent);
	ARG_UNUSED(column);
	int32_t val = 0;
	int ret = app_sensor_get_table_entry(row->index, NULL, &val);
	if (ret < 0) {

		return -ENOENT;
	}

	vb->type = ASN1_TAG_INTEGER;
	vb->val.int_val = val;
	return 0;
}

static const uint8_t s_sensor_cols[] = {1, 2, 3};
static const uint8_t s_sensor_types[] = {ASN1_TAG_INTEGER, ASN1_TAG_OCTET_STRING, ASN1_TAG_INTEGER};
static snmp_table_get_cb_t s_sensor_get_cbs[] = {cb_table_col1_get, cb_table_col2_get,
						 cb_table_col3_get};

static struct snmp_table_row s_sensor_rows[3] = {{.index = 1}, {.index = 2}, {.index = 3}};

static struct snmp_mib_table s_sensor_table = {
	.table_oid = {.len = 10,
		      .ids = {1, 3, 6, 1, 4, 1, CONFIG_SNMP_SAMPLE_ENTERPRISE_ID, 1, 3, 1}},
	.column_cnt = 3,
	.column_subids = s_sensor_cols,
	.column_types = s_sensor_types,
	.get_cbs = s_sensor_get_cbs,
	.set_cbs = NULL,
};

int app_mib_init(struct snmp_agent *agent)
{
	int ret;

	/* Configure Enterprise OID for sysObjectID and Traps */
	struct snmp_oid ent_oid = {.len = 7,
				   .ids = {1, 3, 6, 1, 4, 1, CONFIG_SNMP_SAMPLE_ENTERPRISE_ID}};
	snmp_set_device_enterprise_oid(agent, &ent_oid);

#if defined(CONFIG_SNMP_SAMPLE_SENSOR_OBJECT)
	/* 1. Register sensorValue (1.3.6.1.4.1.<enterprise>.1.1.1.0) */
	static const struct snmp_mib_node node_sensor_val = {
		.oid = {.len = 11,
			.ids = {1, 3, 6, 1, 4, 1, CONFIG_SNMP_SAMPLE_ENTERPRISE_ID, 1, 1, 1, 0}},
		.type = ASN1_TAG_INTEGER,
		.get_cb = cb_get_sensor_value,
		.set_cb = cb_set_sensor_value,
	};
	ret = snmp_mib_register(agent, &node_sensor_val);
	if (ret < 0) {
		LOG_ERR("Failed to register sensorValue node: %d", ret);
		return ret;
	}

	/* 4. Register sensorHumidity (1.3.6.1.4.1.<enterprise>.1.1.4.0) */
	static const struct snmp_mib_node node_humidity = {
		.oid = {.len = 11,
			.ids = {1, 3, 6, 1, 4, 1, CONFIG_SNMP_SAMPLE_ENTERPRISE_ID, 1, 1, 4, 0}},
		.type = SNMP_TAG_GAUGE32,
		.get_cb = cb_get_sensor_humidity,
		.set_cb = NULL,
	};
	ret = snmp_mib_register(agent, &node_humidity);
	if (ret < 0) {
		LOG_ERR("Failed to register sensorHumidity node: %d", ret);
		return ret;
	}

	/* 5. Register sensorDescription (1.3.6.1.4.1.<enterprise>.1.1.5.0) */
	static const struct snmp_mib_node node_desc = {
		.oid = {.len = 11,
			.ids = {1, 3, 6, 1, 4, 1, CONFIG_SNMP_SAMPLE_ENTERPRISE_ID, 1, 1, 5, 0}},
		.type = ASN1_TAG_OCTET_STRING,
		.get_cb = cb_get_sensor_desc,
		.set_cb = NULL,
	};
	ret = snmp_mib_register(agent, &node_desc);
	if (ret < 0) {
		LOG_ERR("Failed to register sensorDescription node: %d", ret);
		return ret;
	}

	/* 6. Register sensorStatusOid (1.3.6.1.4.1.<enterprise>.1.1.6.0) */
	static const struct snmp_mib_node node_status_oid = {
		.oid = {.len = 11,
			.ids = {1, 3, 6, 1, 4, 1, CONFIG_SNMP_SAMPLE_ENTERPRISE_ID, 1, 1, 6, 0}},
		.type = ASN1_TAG_OID,
		.get_cb = cb_get_sensor_status_oid,
		.set_cb = NULL,
	};
	ret = snmp_mib_register(agent, &node_status_oid);
	if (ret < 0) {
		LOG_ERR("Failed to register sensorStatusOid node: %d", ret);
		return ret;
	}
#endif

#if defined(CONFIG_SNMP_SAMPLE_LED_OBJECT)
	/* 2. Register ledState (1.3.6.1.4.1.<enterprise>.1.1.2.0) */
	static const struct snmp_mib_node node_led = {
		.oid = {.len = 11,
			.ids = {1, 3, 6, 1, 4, 1, CONFIG_SNMP_SAMPLE_ENTERPRISE_ID, 1, 1, 2, 0}},
		.type = ASN1_TAG_INTEGER,
		.get_cb = cb_get_led_state,
		.set_cb = cb_set_led_state,
	};
	ret = snmp_mib_register(agent, &node_led);
	if (ret < 0) {
		LOG_ERR("Failed to register ledState node: %d", ret);
		return ret;
	}
#endif

#if defined(CONFIG_SNMP_SAMPLE_BUTTON_TRAP)
	/* 3. Register buttonCounter (1.3.6.1.4.1.<enterprise>.1.1.3.0) */
	static const struct snmp_mib_node node_btn = {
		.oid = {.len = 11,
			.ids = {1, 3, 6, 1, 4, 1, CONFIG_SNMP_SAMPLE_ENTERPRISE_ID, 1, 1, 3, 0}},
		.type = SNMP_TAG_COUNTER32,
		.get_cb = cb_get_button_counter,
		.set_cb = NULL,
	};
	ret = snmp_mib_register(agent, &node_btn);
	if (ret < 0) {
		LOG_ERR("Failed to register buttonCounter node: %d", ret);
		return ret;
	}
#endif

	/* 7. Register sensorTable (1.3.6.1.4.1.<enterprise>.1.3.1) */
	ret = snmp_mib_register_table(agent, &s_sensor_table);
	if (ret == 0) {
		/*
		 * The row list belongs to the shared, application-owned table, so it
		 * is linked once — by the first agent that registers it — while every
		 * instance is still INITIALIZED and no MIB walk can run concurrently.
		 */
		if (s_sensor_table.rows == NULL) {
			snmp_mib_table_add_row(agent, &s_sensor_table, &s_sensor_rows[0]);
			snmp_mib_table_add_row(agent, &s_sensor_table, &s_sensor_rows[1]);
			snmp_mib_table_add_row(agent, &s_sensor_table, &s_sensor_rows[2]);
		}
	} else {
		LOG_WRN("Failed to register sensorTable: %d", ret);
	}

	LOG_INF("Private MIB registered under 1.3.6.1.4.1.%d.1 (nucleoH743)",
		CONFIG_SNMP_SAMPLE_ENTERPRISE_ID);
	return 0;
}
