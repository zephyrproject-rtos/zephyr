/* btp_hap.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>

/* HAP commands */
#define BTP_HAP_READ_SUPPORTED_COMMANDS         0x01U
struct btp_hap_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_HAP_HA_OPT_PRESETS_SYNC             0x01U
#define BTP_HAP_HA_OPT_PRESETS_INDEPENDENT      0x02U
#define BTP_HAP_HA_OPT_PRESETS_DYNAMIC          0x04U
#define BTP_HAP_HA_OPT_PRESETS_WRITABLE         0x08U

#define BTP_HAP_HA_INIT                         0x02U
struct btp_hap_ha_init_cmd {
	uint8_t type;
	uint16_t opts;
} __packed;

#define BTP_HAP_HARC_INIT                       0x03U
#define BTP_HAP_HAUC_INIT                       0x04U
#define BTP_HAP_IAC_INIT                        0x05U

#define BTP_HAP_IAC_DISCOVER			0x06U
struct btp_hap_iac_discover_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HAP_IAC_SET_ALERT			0x07U
struct btp_hap_iac_set_alert_cmd {
	bt_addr_le_t address;
	uint8_t alert;
} __packed;

#define BTP_HAP_HAUC_DISCOVER			0x08U
struct btp_hap_hauc_discover_cmd {
	bt_addr_le_t address;
} __packed;

/* HAP events */
#define BT_HAP_EV_IAC_DISCOVERY_COMPLETE        0x80U
struct btp_hap_iac_discovery_complete_ev {
	bt_addr_le_t address;
	uint8_t status;
} __packed;

#define BT_HAP_EV_HAUC_DISCOVERY_COMPLETE       0x81U
struct btp_hap_hauc_discovery_complete_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint16_t has_hearing_aid_features_handle;
	uint16_t has_control_point_handle;
	uint16_t has_active_preset_index_handle;
} __packed;
