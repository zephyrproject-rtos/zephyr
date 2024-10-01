/* btp_aics.h - Bluetooth tester headers */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/conn.h>

/*AICS service */
struct btp_aics_instance {
	/** Number of Audio Input Control Service instances */
	uint8_t aics_cnt;
	/** Array of pointers to Audio Input Control Service instances */
	struct bt_aics **aics;
};

extern struct bt_aics_cb aics_client_cb;
extern struct btp_aics_instance aics_client_instance;
extern struct btp_aics_instance aics_server_instance;
void btp_send_aics_state_ev(struct bt_conn *conn, uint8_t att_status, int8_t gain, uint8_t mute,
			    uint8_t mode);
void btp_send_gain_setting_properties_ev(struct bt_conn *conn, uint8_t att_status, uint8_t units,
					 int8_t minimum, int8_t maximum);
void btp_send_aics_input_type_event(struct bt_conn *conn, uint8_t att_status, uint8_t input_type);
void btp_send_aics_status_ev(struct bt_conn *conn, uint8_t att_status, bool active);
void btp_send_aics_description_ev(struct bt_conn *conn, uint8_t att_status, uint8_t data_len,
				  char *description);
void btp_send_aics_procedure_ev(struct bt_conn *conn, uint8_t att_status, uint8_t opcode);

#define BTP_AICS_READ_SUPPORTED_COMMANDS	0x01
struct btp_aics_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

/* AICS client/server commands */
#define BTP_AICS_SET_GAIN			0x02
struct btp_aics_set_gain_cmd {
	bt_addr_le_t address;
	int8_t gain;
} __packed;

#define BTP_AICS_MUTE				0x03
struct btp_aics_mute_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_AICS_UNMUTE				0x04
struct btp_aics_unmute_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_AICS_MAN_GAIN_SET			0x05
struct btp_aics_manual_gain_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_AICS_AUTO_GAIN_SET			0x06
struct btp_aics_auto_gain_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_AICS_SET_MAN_GAIN_ONLY		0x07
#define BTP_AICS_SET_AUTO_GAIN_ONLY		0x08
#define BTP_AICS_AUDIO_DESCRIPTION_SET		0x09
struct btp_aics_audio_desc_cmd {
	uint8_t desc_len;
	uint8_t desc[0];
} __packed;

#define BTP_AICS_MUTE_DISABLE			0x0a
#define BTP_AICS_GAIN_SETTING_PROP_GET		0x0b
struct btp_aics_gain_setting_prop_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_AICS_TYPE_GET			0x0c
struct btp_aics_type_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_AICS_STATUS_GET			0x0d
struct btp_aics_status_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_AICS_STATE_GET			0x0e
struct btp_aics_state_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_AICS_DESCRIPTION_GET		0x0f
struct btp_aics_desc_cmd {
	bt_addr_le_t address;
} __packed;

/* AICS events */
#define BTP_AICS_STATE_EV			0x80
struct btp_aics_state_ev {
	bt_addr_le_t address;
	uint8_t att_status;
	int8_t gain;
	uint8_t mute;
	uint8_t mode;
} __packed;

#define BTP_GAIN_SETTING_PROPERTIES_EV		0x81
struct btp_gain_setting_properties_ev {
	bt_addr_le_t address;
	uint8_t att_status;
	uint8_t units;
	int8_t minimum;
	int8_t maximum;
} __packed;

#define BTP_AICS_INPUT_TYPE_EV			0x82
struct btp_aics_input_type_ev {
	bt_addr_le_t address;
	uint8_t att_status;
	uint8_t input_type;
} __packed;

#define BTP_AICS_STATUS_EV			0x83
struct btp_aics_status_ev {
	bt_addr_le_t address;
	uint8_t att_status;
	bool active;
} __packed;

#define BTP_AICS_DESCRIPTION_EV			0x84
struct btp_aics_description_ev {
	bt_addr_le_t address;
	uint8_t att_status;
	uint8_t data_len;
	char data[0];
} __packed;

#define BTP_AICS_PROCEDURE_EV			0x85
struct btp_aics_procedure_ev {
	bt_addr_le_t address;
	uint8_t att_status;
	uint8_t opcode;
} __packed;
