/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OCPP_J_
#define __OCPP_J_

#define JSON_MSG_BUF_128 128
#define JSON_MSG_BUF_256 256
#define JSON_MSG_BUF_512 512

#define BOOTNOTIF_MIN_FIELDS 2
#define BOOTNOTIF_MAX_FIELDS 9

#define STOP_TXN_MIN_FIELDS 3
#define STOP_TXN_MAX_FIELDS 5

#define START_TXN_MIN_FIELDS 4
#define START_TXN_MAX_FIELDS 5

#define GET_CFG_MAX_FIELDS 1

#define SAMPLED_VALUE_MIN_FIELDS 2
#define SAMPLED_VALUE_MAX_FIELDS 3

struct json_common_payload_field {
	int val1;
	char *val2;
};

struct json_common_payload_field_str {
	char *val1;
	char *val2;
};

struct json_ocpp_bootnotif_msg {
	char *charge_point_model;
	char *charge_point_vendor;
	char *charge_box_serial_number;
	char *charge_point_serial_number;
	char *firmware_version;
	char *iccid;
	char *imsi;
	char *meter_serial_number;
	char *meter_type;
};

struct json_ocpp_meter_val_msg {
	int connector_id;
	int transaction_id;

	struct json_ocpp_meter_val {
		char *timestamp;

		struct json_ocpp_sample_val {
			char *measurand;
			char *value;
			char *unit;
		} sampled_value[1];
		size_t sampled_value_len;
	} meter_value[1];

	size_t meter_value_len;
};

struct json_ocpp_stop_txn_msg {
	int transaction_id;
	int meter_stop;
	char *timestamp;
	char *reason;
	char *id_tag;
};

struct json_ocpp_start_txn_msg {
	int connector_id;
	char *id_tag;
	int meter_start;
	char *timestamp;
	int reservation_id;
};

struct json_ocpp_getconfig_msg {
	struct json_ocpp_key_val {
		char *key;
		int readonly;
		char *value;
	} configuration_key[1];

	size_t configuration_key_len;
	char *unknown_key;
};

struct json_idtag_info_root {
	struct {
		char *status;
		char *parent_id_tag;
		char *expiry_date;
	} json_id_tag_info;
};

struct json_bootnotif_payload {
	char *status;
	int interval;
	char *current_time;
};

struct json_getconfig_payload {
	char *key[1];
	size_t key_len;
};

#endif /* __OCPP_J_ */
