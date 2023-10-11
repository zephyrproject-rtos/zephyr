/* mesh.h - Bluetooth tester headers */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/addr.h>

/* MESH Service */
/* commands */
#define MESH_READ_SUPPORTED_COMMANDS	0x01
struct mesh_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define MESH_OUT_BLINK			BIT(0)
#define MESH_OUT_BEEP			BIT(1)
#define MESH_OUT_VIBRATE		BIT(2)
#define MESH_OUT_DISPLAY_NUMBER		BIT(3)
#define MESH_OUT_DISPLAY_STRING		BIT(4)

#define MESH_IN_PUSH			BIT(0)
#define MESH_IN_TWIST			BIT(1)
#define MESH_IN_ENTER_NUMBER		BIT(2)
#define MESH_IN_ENTER_STRING		BIT(3)

#define MESH_CONFIG_PROVISIONING	0x02

struct set_keys {
	uint8_t pub_key[64];
	uint8_t priv_key[32];
} __packed;

struct mesh_config_provisioning_cmd {
	uint8_t uuid[16];
	uint8_t static_auth[16];
	uint8_t out_size;
	uint16_t out_actions;
	uint8_t in_size;
	uint16_t in_actions;
	uint8_t auth_method;
	struct set_keys set_keys[0];
} __packed;

#define MESH_PROVISION_NODE		0x03
struct mesh_provision_node_cmd {
	uint8_t net_key[16];
	uint16_t net_key_idx;
	uint8_t flags;
	uint32_t iv_index;
	uint32_t seq_num;
	uint16_t addr;
	uint8_t dev_key[16];
	uint8_t pub_key[0];
} __packed;

#define MESH_INIT			0x04
#define MESH_RESET			0x05
#define MESH_INPUT_NUMBER		0x06
struct mesh_input_number_cmd {
	uint32_t number;
} __packed;

#define MESH_INPUT_STRING		0x07
struct mesh_input_string_cmd {
	uint8_t string_len;
	uint8_t string[];
} __packed;

#define MESH_IVU_TEST_MODE		0x08
struct mesh_ivu_test_mode_cmd {
	uint8_t enable;
} __packed;

#define MESH_IVU_TOGGLE_STATE			0x09

#define MESH_NET_SEND			0x0a
struct mesh_net_send_cmd {
	uint8_t ttl;
	uint16_t src;
	uint16_t dst;
	uint8_t payload_len;
	uint8_t payload[];
} __packed;

#define MESH_HEALTH_GENERATE_FAULTS	0x0b
struct mesh_health_generate_faults_rp {
	uint8_t test_id;
	uint8_t cur_faults_count;
	uint8_t reg_faults_count;
	uint8_t current_faults[0];
	uint8_t registered_faults[0];
} __packed;

#define MESH_HEALTH_CLEAR_FAULTS	0x0c

#define MESH_LPN			0x0d
struct mesh_lpn_set_cmd {
	uint8_t enable;
} __packed;

#define MESH_LPN_POLL			0x0e

#define MESH_MODEL_SEND			0x0f
struct mesh_model_send_cmd {
	uint16_t src;
	uint16_t dst;
	uint8_t payload_len;
	uint8_t payload[];
} __packed;

#define MESH_LPN_SUBSCRIBE		0x10
struct mesh_lpn_subscribe_cmd {
	uint16_t address;
} __packed;

#define MESH_LPN_UNSUBSCRIBE		0x11
struct mesh_lpn_unsubscribe_cmd {
	uint16_t address;
} __packed;

#define MESH_RPL_CLEAR			0x12
#define MESH_PROXY_IDENTITY		0x13
#define MESH_COMP_DATA_GET		0x14
struct mesh_comp_data_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t page;
} __packed;

#define MESH_CFG_BEACON_GET		0x15
struct mesh_cfg_val_get_cmd {
	uint16_t net_idx;
	uint16_t address;
} __packed;

#define MESH_CFG_BEACON_SET		0x16
struct mesh_cfg_beacon_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t val;
} __packed;

#define MESH_CFG_DEFAULT_TTL_GET		0x18
#define MESH_CFG_DEFAULT_TTL_SET		0x19
struct mesh_cfg_default_ttl_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t val;
} __packed;

#define MESH_CFG_GATT_PROXY_GET		0x1a
#define MESH_CFG_GATT_PROXY_SET		0x1b
struct mesh_cfg_gatt_proxy_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t val;
} __packed;

#define MESH_CFG_FRIEND_GET		0x1c
#define MESH_CFG_FRIEND_SET		0x1d
struct mesh_cfg_friend_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t val;
} __packed;

#define MESH_CFG_RELAY_GET		0x1e
#define MESH_CFG_RELAY_SET		0x1f
struct mesh_cfg_relay_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t new_relay;
	uint8_t new_transmit;
} __packed;

#define MESH_CFG_MODEL_PUB_GET		0x20
struct mesh_cfg_model_pub_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t model_id;
} __packed;

#define MESH_CFG_MODEL_PUB_SET		0x21
struct mesh_cfg_model_pub_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t model_id;
	uint16_t pub_addr;
	uint16_t app_idx;
	uint8_t cred_flag;
	uint8_t ttl;
	uint8_t period;
	uint8_t transmit;
} __packed;

#define MESH_CFG_MODEL_SUB_ADD		0x22
#define MESH_CFG_MODEL_SUB_DEL		0x23
struct mesh_cfg_model_sub_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t sub_addr;
	uint16_t model_id;
} __packed;

#define MESH_CFG_NETKEY_ADD		0x24
struct mesh_cfg_netkey_add_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t net_key[16];
	uint16_t net_key_idx;
} __packed;

#define MESH_CFG_NETKEY_GET		0x25
#define MESH_CFG_NETKEY_DEL		0x26
struct mesh_cfg_netkey_del_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
} __packed;

#define MESH_CFG_APPKEY_ADD		0x27
struct mesh_cfg_appkey_add_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
	uint8_t app_key[16];
	uint16_t app_key_idx;
} __packed;

#define MESH_CFG_APPKEY_DEL		0x28
struct mesh_cfg_appkey_del_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
	uint16_t app_key_idx;
} __packed;

#define MESH_CFG_APPKEY_GET		0x29
struct mesh_cfg_appkey_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
} __packed;

#define MESH_CFG_MODEL_APP_BIND		0x2A
#define MESH_CFG_MODEL_APP_UNBIND		0x2B
struct mesh_cfg_model_app_bind_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t app_key_idx;
	uint16_t mod_id;
} __packed;

#define MESH_CFG_MODEL_APP_GET		0x2C
#define MESH_CFG_MODEL_APP_VND_GET		0x2D
struct mesh_cfg_model_app_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t mod_id;
	uint16_t cid;
} __packed;

#define MESH_CFG_HEARTBEAT_PUB_SET		0x2E
struct mesh_cfg_heartbeat_pub_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
	uint16_t destination;
	uint8_t count_log;
	uint8_t period_log;
	uint8_t ttl;
	uint16_t features;
} __packed;

#define MESH_CFG_HEARTBEAT_PUB_GET		0x2F
#define MESH_CFG_HEARTBEAT_SUB_SET		0x30
struct mesh_cfg_heartbeat_sub_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t source;
	uint16_t destination;
	uint8_t period_log;
} __packed;

#define MESH_CFG_HEARTBEAT_SUB_GET		0x31
#define MESH_CFG_NET_TRANS_GET		0x32
#define MESH_CFG_NET_TRANS_SET		0x33
struct mesh_cfg_net_trans_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t transmit;
} __packed;

#define MESH_CFG_MODEL_SUB_OVW		0x34
#define MESH_CFG_MODEL_SUB_DEL_ALL		0x35
struct mesh_cfg_model_sub_del_all_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t model_id;
} __packed;

#define MESH_CFG_MODEL_SUB_GET		0x36
struct mesh_cfg_model_sub_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t model_id;
} __packed;

#define MESH_CFG_MODEL_SUB_GET_VND		0x37
struct mesh_cfg_model_sub_get_vnd_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t model_id;
	uint16_t cid;
} __packed;

#define MESH_CFG_MODEL_SUB_VA_ADD		0x38
#define MESH_CFG_MODEL_SUB_VA_DEL		0x39
#define MESH_CFG_MODEL_SUB_VA_OVW		0x3A
struct mesh_cfg_model_sub_va_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t model_id;
	uint8_t uuid[16];
} __packed;

#define MESH_CFG_NETKEY_UPDATE		0x3B
#define MESH_CFG_APPKEY_UPDATE		0x3C
#define MESH_CFG_NODE_IDT_SET		0x3D
struct mesh_cfg_node_idt_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
	uint8_t new_identity;
} __packed;

#define MESH_CFG_NODE_IDT_GET		0x3E
struct mesh_cfg_node_idt_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
} __packed;

#define MESH_CFG_NODE_RESET		0x3F
struct mesh_cfg_node_reset_cmd {
	uint16_t net_idx;
	uint16_t address;
} __packed;

#define MESH_CFG_LPN_TIMEOUT_GET		0x40
struct mesh_cfg_lpn_timeout_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t unicast_addr;
} __packed;

#define MESH_CFG_MODEL_PUB_VA_SET		0x41
struct mesh_cfg_model_pub_va_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t model_id;
	uint16_t app_idx;
	uint8_t cred_flag;
	uint8_t ttl;
	uint8_t period;
	uint8_t transmit;
	uint8_t uuid[16];
} __packed;

#define MESH_CFG_MODEL_APP_BIND_VND		0x42
struct mesh_cfg_model_app_bind_vnd_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t app_key_idx;
	uint16_t mod_id;
	uint16_t cid;
} __packed;

#define MESH_HEALTH_FAULT_GET		0x43
struct mesh_health_fault_get_cmd {
	uint16_t address;
	uint16_t app_idx;
	uint16_t cid;
} __packed;

#define MESH_HEALTH_FAULT_CLEAR		0x44
struct mesh_health_fault_clear_cmd {
	uint16_t address;
	uint16_t app_idx;
	uint16_t cid;
	uint8_t ack;
} __packed;

#define MESH_HEALTH_FAULT_TEST		0x45
struct mesh_health_fault_test_cmd {
	uint16_t address;
	uint16_t app_idx;
	uint16_t cid;
	uint8_t test_id;
	uint8_t ack;
} __packed;

#define MESH_HEALTH_PERIOD_GET		0x46
struct mesh_health_period_get_cmd {
	uint16_t address;
	uint16_t app_idx;
} __packed;

#define MESH_HEALTH_PERIOD_SET		0x47
struct mesh_health_period_set_cmd {
	uint16_t address;
	uint16_t app_idx;
	uint8_t divisor;
	uint8_t ack;
} __packed;

#define MESH_HEALTH_ATTENTION_GET		0x48
struct mesh_health_attention_get_cmd {
	uint16_t address;
	uint16_t app_idx;
} __packed;

#define MESH_HEALTH_ATTENTION_SET		0x49
struct mesh_health_attention_set_cmd {
	uint16_t address;
	uint16_t app_idx;
	uint8_t attention;
	uint8_t ack;
} __packed;

#define MESH_PROVISION_ADV		0x4A
struct mesh_provision_adv_cmd {
	uint8_t uuid[16];
	uint16_t net_idx;
	uint16_t address;
	uint8_t attention_duration;
	uint8_t net_key[16];
} __packed;

#define MESH_CFG_KRP_GET		0x4B
struct mesh_cfg_krp_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t key_net_idx;
} __packed;

#define MESH_CFG_KRP_SET		0x4C
struct mesh_cfg_krp_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t key_net_idx;
	uint8_t transition;
} __packed;

/* events */
#define MESH_EV_OUT_NUMBER_ACTION	0x80
struct mesh_out_number_action_ev {
	uint16_t action;
	uint32_t number;
} __packed;

#define MESH_EV_OUT_STRING_ACTION	0x81
struct mesh_out_string_action_ev {
	uint8_t string_len;
	uint8_t string[];
} __packed;

#define MESH_EV_IN_ACTION		0x82
struct mesh_in_action_ev {
	uint16_t action;
	uint8_t size;
} __packed;

#define MESH_EV_PROVISIONED		0x83

#define MESH_PROV_BEARER_PB_ADV		0x00
#define MESH_PROV_BEARER_PB_GATT	0x01
#define MESH_EV_PROV_LINK_OPEN		0x84
struct mesh_prov_link_open_ev {
	uint8_t bearer;
} __packed;

#define MESH_EV_PROV_LINK_CLOSED	0x85
struct mesh_prov_link_closed_ev {
	uint8_t bearer;
} __packed;

#define MESH_EV_NET_RECV		0x86
struct mesh_net_recv_ev {
	uint8_t ttl;
	uint8_t ctl;
	uint16_t src;
	uint16_t dst;
	uint8_t payload_len;
	uint8_t payload[];
} __packed;

#define MESH_EV_INVALID_BEARER		0x87
struct mesh_invalid_bearer_ev {
	uint8_t opcode;
} __packed;

#define MESH_EV_INCOMP_TIMER_EXP	0x88

#define MESH_EV_FRND_ESTABLISHED	0x89
struct mesh_frnd_established_ev {
	uint16_t net_idx;
	uint16_t lpn_addr;
	uint8_t recv_delay;
	uint32_t polltimeout;
} __packed;

#define MESH_EV_FRND_TERMINATED		0x8a
struct mesh_frnd_terminated_ev {
	uint16_t net_idx;
	uint16_t lpn_addr;
} __packed;

#define MESH_EV_LPN_ESTABLISHED		0x8b
struct mesh_lpn_established_ev {
	uint16_t net_idx;
	uint16_t friend_addr;
	uint8_t queue_size;
	uint8_t recv_win;
} __packed;

#define MESH_EV_LPN_TERMINATED		0x8c
struct mesh_lpn_terminated_ev {
	uint16_t net_idx;
	uint16_t friend_addr;
} __packed;

#define MESH_EV_LPN_POLLED			0x8d
struct mesh_lpn_polled_ev {
	uint16_t net_idx;
	uint16_t friend_addr;
	uint8_t retry;
} __packed;

#define MESH_EV_PROV_NODE_ADDED		0x8e
struct mesh_prov_node_added_ev {
	uint16_t net_idx;
	uint16_t addr;
	uint8_t uuid[16];
	uint8_t num_elems;
} __packed;
