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
#define BTP_MESH_READ_SUPPORTED_COMMANDS	0x01
struct btp_mesh_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_MESH_OUT_BLINK			BIT(0)
#define BTP_MESH_OUT_BEEP			BIT(1)
#define BTP_MESH_OUT_VIBRATE			BIT(2)
#define BTP_MESH_OUT_DISPLAY_NUMBER		BIT(3)
#define BTP_MESH_OUT_DISPLAY_STRING		BIT(4)

#define BTP_MESH_IN_PUSH			BIT(0)
#define BTP_MESH_IN_TWIST			BIT(1)
#define BTP_MESH_IN_ENTER_NUMBER		BIT(2)
#define BTP_MESH_IN_ENTER_STRING		BIT(3)

#define BTP_MESH_CONFIG_PROVISIONING		0x02

#define BTP_MESH_PROV_AUTH_MAX_LEN 32

struct btp_mesh_config_provisioning_cmd {
	uint8_t uuid[16];
	uint8_t static_auth[BTP_MESH_PROV_AUTH_MAX_LEN];
	uint8_t static_auth_size;
	uint8_t out_size;
	uint16_t out_actions;
	uint8_t in_size;
	uint16_t in_actions;
	uint8_t auth_method;
} __packed;
struct btp_mesh_config_provisioning_cmd_v2 {
	uint8_t uuid[16];
	uint8_t static_auth[BTP_MESH_PROV_AUTH_MAX_LEN];
	uint8_t static_auth_size;
	uint8_t out_size;
	uint16_t out_actions;
	uint8_t in_size;
	uint16_t in_actions;
	uint8_t auth_method;
	uint8_t set_pub_key[64];
	uint8_t set_priv_key[32];
} __packed;

#define BTP_MESH_PROVISION_NODE			0x03
struct btp_mesh_provision_node_cmd {
	uint8_t net_key[16];
	uint16_t net_key_idx;
	uint8_t flags;
	uint32_t iv_index;
	uint32_t seq_num;
	uint16_t addr;
	uint8_t dev_key[16];
} __packed;
struct btp_mesh_provision_node_cmd_v2 {
	uint8_t net_key[16];
	uint16_t net_key_idx;
	uint8_t flags;
	uint32_t iv_index;
	uint32_t seq_num;
	uint16_t addr;
	uint8_t dev_key[16];
	uint8_t pub_key[64];
} __packed;

#define BTP_MESH_INIT				0x04
struct btp_mesh_init_cmd {
	uint8_t comp;
} __packed;

#define BTP_MESH_RESET				0x05
#define BTP_MESH_INPUT_NUMBER			0x06
struct btp_mesh_input_number_cmd {
	uint32_t number;
} __packed;

#define BTP_MESH_INPUT_STRING			0x07
struct btp_mesh_input_string_cmd {
	uint8_t string_len;
	uint8_t string[];
} __packed;

#define BTP_MESH_IVU_TEST_MODE			0x08
struct btp_mesh_ivu_test_mode_cmd {
	uint8_t enable;
} __packed;

#define BTP_MESH_IVU_TOGGLE_STATE		0x09

#define BTP_MESH_NET_SEND			0x0a
struct btp_mesh_net_send_cmd {
	uint8_t ttl;
	uint16_t src;
	uint16_t dst;
	uint8_t payload_len;
	uint8_t payload[];
} __packed;

#define BTP_MESH_HEALTH_GENERATE_FAULTS	0x0b
struct btp_mesh_health_generate_faults_rp {
	uint8_t test_id;
	uint8_t cur_faults_count;
	uint8_t reg_faults_count;
	uint8_t current_faults[0];
	uint8_t registered_faults[0];
} __packed;

#define BTP_MESH_HEALTH_CLEAR_FAULTS		0x0c

#define BTP_MESH_LPN				0x0d
struct btp_mesh_lpn_set_cmd {
	uint8_t enable;
} __packed;

#define BTP_MESH_LPN_POLL			0x0e

#define BTP_MESH_MODEL_SEND			0x0f
struct btp_mesh_model_send_cmd {
	uint8_t ttl;
	uint16_t src;
	uint16_t dst;
	uint8_t payload_len;
	uint8_t payload[];
} __packed;

#define BTP_MESH_LPN_SUBSCRIBE			0x10
struct btp_mesh_lpn_subscribe_cmd {
	uint16_t address;
} __packed;

#define BTP_MESH_LPN_UNSUBSCRIBE		0x11
struct btp_mesh_lpn_unsubscribe_cmd {
	uint16_t address;
} __packed;

#define BTP_MESH_RPL_CLEAR			0x12
#define BTP_MESH_PROXY_IDENTITY			0x13
#define BTP_MESH_COMP_DATA_GET			0x14
struct btp_mesh_comp_data_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t page;
} __packed;
struct btp_mesh_comp_data_get_rp {
	uint8_t data[0];
} __packed;

#define BTP_MESH_CFG_BEACON_GET			0x15
struct btp_mesh_cfg_beacon_get_cmd {
	uint16_t net_idx;
	uint16_t address;
} __packed;
struct btp_mesh_cfg_beacon_get_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_BEACON_SET			0x16
struct btp_mesh_cfg_beacon_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t val;
} __packed;
struct btp_mesh_cfg_beacon_set_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_DEFAULT_TTL_GET		0x18
struct btp_mesh_cfg_default_ttl_get_cmd {
	uint16_t net_idx;
	uint16_t address;
} __packed;
struct btp_mesh_cfg_default_ttl_get_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_DEFAULT_TTL_SET		0x19
struct btp_mesh_cfg_default_ttl_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t val;
} __packed;
struct btp_mesh_cfg_default_ttl_set_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_GATT_PROXY_GET		0x1a
struct btp_mesh_cfg_gatt_proxy_get_cmd {
	uint16_t net_idx;
	uint16_t address;
} __packed;
struct btp_mesh_cfg_gatt_proxy_get_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_GATT_PROXY_SET		0x1b
struct btp_mesh_cfg_gatt_proxy_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t val;
} __packed;
struct btp_mesh_cfg_gatt_proxy_set_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_FRIEND_GET			0x1c
struct btp_mesh_cfg_friend_get_cmd {
	uint16_t net_idx;
	uint16_t address;
} __packed;
struct btp_mesh_cfg_friend_get_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_FRIEND_SET			0x1d
struct btp_mesh_cfg_friend_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t val;
} __packed;
struct btp_mesh_cfg_friend_set_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_RELAY_GET			0x1e
struct btp_mesh_cfg_relay_get_cmd {
	uint16_t net_idx;
	uint16_t address;
} __packed;
struct btp_mesh_cfg_relay_get_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_RELAY_SET			0x1f
struct btp_mesh_cfg_relay_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t new_relay;
	uint8_t new_transmit;
} __packed;
struct btp_mesh_cfg_relay_set_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_MODEL_PUB_GET		0x20
struct btp_mesh_cfg_model_pub_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t model_id;
} __packed;
struct btp_mesh_cfg_model_pub_get_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_MODEL_PUB_SET		0x21
struct btp_mesh_cfg_model_pub_set_cmd {
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
struct btp_mesh_cfg_model_pub_set_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_MODEL_SUB_ADD		0x22
struct btp_mesh_cfg_model_sub_add_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t sub_addr;
	uint16_t model_id;
} __packed;
struct btp_mesh_cfg_model_sub_add_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_MODEL_SUB_DEL		0x23
struct btp_mesh_cfg_model_sub_del_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t sub_addr;
	uint16_t model_id;
} __packed;
struct btp_mesh_cfg_model_sub_del_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_NETKEY_ADD			0x24
struct btp_mesh_cfg_netkey_add_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t net_key[16];
	uint16_t net_key_idx;
} __packed;
struct btp_mesh_cfg_netkey_add_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_NETKEY_GET			0x25
struct btp_mesh_cfg_netkey_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
} __packed;
struct btp_mesh_cfg_netkey_get_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_NETKEY_DEL			0x26
struct btp_mesh_cfg_netkey_del_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
} __packed;
struct btp_mesh_cfg_netkey_del_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_APPKEY_ADD			0x27
struct btp_mesh_cfg_appkey_add_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
	uint8_t app_key[16];
	uint16_t app_key_idx;
} __packed;
struct btp_mesh_cfg_appkey_add_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_APPKEY_DEL			0x28
struct btp_mesh_cfg_appkey_del_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
	uint16_t app_key_idx;
} __packed;
struct btp_mesh_cfg_appkey_del_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_APPKEY_GET			0x29
struct btp_mesh_cfg_appkey_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
} __packed;
struct btp_mesh_cfg_appkey_get_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_MODEL_APP_BIND		0x2A
struct btp_mesh_cfg_model_app_bind_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t app_key_idx;
	uint16_t mod_id;
} __packed;
struct btp_mesh_cfg_model_app_bind_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_MODEL_APP_UNBIND		0x2B
struct btp_mesh_cfg_model_app_unbind_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t app_key_idx;
	uint16_t mod_id;
} __packed;
struct btp_mesh_cfg_model_app_unbind_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_MODEL_APP_GET		0x2C
struct btp_mesh_cfg_model_app_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t mod_id;
} __packed;
struct btp_mesh_cfg_model_app_get_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_MODEL_APP_VND_GET		0x2D
struct btp_mesh_cfg_model_app_vnd_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t mod_id;
	uint16_t cid;
} __packed;
struct btp_mesh_cfg_model_app_vnd_get_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_HEARTBEAT_PUB_SET		0x2E
struct btp_mesh_cfg_heartbeat_pub_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
	uint16_t destination;
	uint8_t count_log;
	uint8_t period_log;
	uint8_t ttl;
	uint16_t features;
} __packed;
struct btp_mesh_cfg_heartbeat_pub_set_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_HEARTBEAT_PUB_GET		0x2F
struct btp_mesh_cfg_heartbeat_pub_get_cmd {
	uint16_t net_idx;
	uint16_t address;
} __packed;
struct btp_mesh_cfg_heartbeat_pub_get_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_HEARTBEAT_SUB_SET		0x30
struct btp_mesh_cfg_heartbeat_sub_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t source;
	uint16_t destination;
	uint8_t period_log;
} __packed;
struct btp_mesh_cfg_heartbeat_sub_set_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_HEARTBEAT_SUB_GET		0x31
struct btp_mesh_cfg_heartbeat_sub_get_cmd {
	uint16_t net_idx;
	uint16_t address;
} __packed;
struct btp_mesh_cfg_heartbeat_sub_get_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_NET_TRANS_GET		0x32
struct btp_mesh_cfg_net_trans_get_cmd {
	uint16_t net_idx;
	uint16_t address;
} __packed;
struct btp_mesh_cfg_net_trans_get_rp {
	uint8_t transmit;
} __packed;

#define BTP_MESH_CFG_NET_TRANS_SET		0x33
struct btp_mesh_cfg_net_trans_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t transmit;
} __packed;
struct btp_mesh_cfg_net_trans_set_rp {
	uint8_t transmit;
} __packed;

#define BTP_MESH_CFG_MODEL_SUB_OVW		0x34
struct btp_mesh_cfg_model_sub_ovw_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t sub_addr;
	uint16_t model_id;
} __packed;
struct btp_mesh_cfg_model_sub_ovw_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_MODEL_SUB_DEL_ALL		0x35
struct btp_mesh_cfg_model_sub_del_all_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t model_id;
} __packed;
struct btp_mesh_cfg_model_sub_del_all_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_MODEL_SUB_GET		0x36
struct btp_mesh_cfg_model_sub_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t model_id;
} __packed;
struct btp_mesh_cfg_model_sub_get_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_MODEL_SUB_GET_VND		0x37
struct btp_mesh_cfg_model_sub_get_vnd_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t model_id;
	uint16_t cid;
} __packed;
struct btp_mesh_cfg_model_sub_get_vnd_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_MODEL_SUB_VA_ADD		0x38
struct btp_mesh_cfg_model_sub_va_add_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t model_id;
	uint8_t uuid[16];
} __packed;
struct btp_mesh_cfg_model_sub_va_add_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_MODEL_SUB_VA_DEL		0x39
struct btp_mesh_cfg_model_sub_va_del_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t model_id;
	uint8_t uuid[16];
} __packed;
struct btp_mesh_cfg_model_sub_va_del_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_MODEL_SUB_VA_OVW		0x3A
struct btp_mesh_cfg_model_sub_va_ovw_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t model_id;
	uint8_t uuid[16];
} __packed;
struct btp_mesh_cfg_model_sub_va_ovw_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_NETKEY_UPDATE		0x3B
struct btp_mesh_cfg_netkey_update_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint8_t net_key[16];
	uint16_t net_key_idx;
} __packed;
struct btp_mesh_cfg_netkey_update_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_APPKEY_UPDATE		0x3C
struct btp_mesh_cfg_appkey_update_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
	uint8_t app_key[16];
	uint16_t app_key_idx;
} __packed;
struct btp_mesh_cfg_appkey_update_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_NODE_IDT_SET		0x3D
struct btp_mesh_cfg_node_idt_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
	uint8_t new_identity;
} __packed;
struct btp_mesh_cfg_node_idt_set_rp {
	uint8_t status;
	uint8_t identity;
} __packed;

#define BTP_MESH_CFG_NODE_IDT_GET		0x3E
struct btp_mesh_cfg_node_idt_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t net_key_idx;
} __packed;
struct btp_mesh_cfg_node_idt_get_rp {
	uint8_t status;
	uint8_t identity;
} __packed;

#define BTP_MESH_CFG_NODE_RESET			0x3F
struct btp_mesh_cfg_node_reset_cmd {
	uint16_t net_idx;
	uint16_t address;
} __packed;
struct btp_mesh_cfg_node_reset_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_LPN_TIMEOUT_GET		0x40
struct btp_mesh_cfg_lpn_timeout_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t unicast_addr;
} __packed;
struct btp_mesh_cfg_lpn_timeout_rp {
	int32_t timeout;
} __packed;

#define BTP_MESH_CFG_MODEL_PUB_VA_SET		0x41
struct btp_mesh_cfg_model_pub_va_set_cmd {
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
struct btp_mesh_cfg_model_pub_va_set_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_CFG_MODEL_APP_BIND_VND		0x42
struct btp_mesh_cfg_model_app_bind_vnd_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t elem_address;
	uint16_t app_key_idx;
	uint16_t mod_id;
	uint16_t cid;
} __packed;
struct btp_mesh_cfg_model_app_bind_vnd_rp {
	uint8_t status;
} __packed;

#define BTP_MESH_HEALTH_FAULT_GET		0x43
struct btp_mesh_health_fault_get_cmd {
	uint16_t address;
	uint16_t app_idx;
	uint16_t cid;
} __packed;

#define BTP_MESH_HEALTH_FAULT_CLEAR		0x44
struct btp_mesh_health_fault_clear_cmd {
	uint16_t address;
	uint16_t app_idx;
	uint16_t cid;
	uint8_t ack;
} __packed;
struct btp_mesh_health_fault_clear_rp {
	uint8_t test_id;
} __packed;

#define BTP_MESH_HEALTH_FAULT_TEST		0x45
struct btp_mesh_health_fault_test_cmd {
	uint16_t address;
	uint16_t app_idx;
	uint16_t cid;
	uint8_t test_id;
	uint8_t ack;
} __packed;
struct btp_mesh_health_fault_test_rp {
	uint8_t test_id;
	uint16_t cid;
	uint8_t faults[];
} __packed;

#define BTP_MESH_HEALTH_PERIOD_GET		0x46
struct btp_mesh_health_period_get_cmd {
	uint16_t address;
	uint16_t app_idx;
} __packed;

#define BTP_MESH_HEALTH_PERIOD_SET		0x47
struct btp_mesh_health_period_set_cmd {
	uint16_t address;
	uint16_t app_idx;
	uint8_t divisor;
	uint8_t ack;
} __packed;
struct btp_mesh_health_period_set_rp {
	uint8_t divisor;
} __packed;

#define BTP_MESH_HEALTH_ATTENTION_GET		0x48
struct btp_mesh_health_attention_get_cmd {
	uint16_t address;
	uint16_t app_idx;
} __packed;

#define BTP_MESH_HEALTH_ATTENTION_SET		0x49
struct btp_mesh_health_attention_set_cmd {
	uint16_t address;
	uint16_t app_idx;
	uint8_t attention;
	uint8_t ack;
} __packed;
struct btp_mesh_health_attention_set_rp {
	uint8_t attention;
} __packed;

#define BTP_MESH_PROVISION_ADV			0x4A
struct btp_mesh_provision_adv_cmd {
	uint8_t uuid[16];
	uint16_t net_idx;
	uint16_t address;
	uint8_t attention_duration;
	uint8_t net_key[16];
} __packed;

#define BTP_MESH_CFG_KRP_GET			0x4B
struct btp_mesh_cfg_krp_get_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t key_net_idx;
} __packed;
struct btp_mesh_cfg_krp_get_rp {
	uint8_t status;
	uint8_t phase;
} __packed;

#define BTP_MESH_CFG_KRP_SET			0x4C
struct btp_mesh_cfg_krp_set_cmd {
	uint16_t net_idx;
	uint16_t address;
	uint16_t key_net_idx;
	uint8_t transition;
} __packed;
struct btp_mesh_cfg_krp_set_rp {
	uint8_t status;
	uint8_t phase;
} __packed;

#define BTP_MESH_VA_ADD				0x4D
struct btp_mesh_va_add_cmd {
	uint8_t label_uuid[16];
} __packed;
struct btp_mesh_va_add_rp {
	uint16_t addr;
} __packed;

#define BTP_MESH_VA_DEL				0x4E
struct btp_mesh_va_del_cmd {
	uint8_t label_uuid[16];
} __packed;

#define BTP_MESH_PROXY_CONNECT			0x77

struct btp_proxy_connect_cmd {
	uint16_t net_idx;
} __packed;

struct sar_transmitter {
	uint8_t seg_int_step;
	uint8_t unicast_retrans_count;
	uint8_t unicast_retrans_without_prog_count;
	uint8_t unicast_retrans_int_step;
	uint8_t unicast_retrans_int_inc;
	uint8_t multicast_retrans_count;
	uint8_t multicast_retrans_int;
} __packed;

struct sar_receiver {
	uint8_t seg_thresh;
	uint8_t ack_delay_inc;
	uint8_t ack_retrans_count;
	uint8_t discard_timeout;
	uint8_t rx_seg_int_step;
} __packed;

#define BTP_MESH_SAR_TRANSMITTER_GET		0x4f
struct btp_mesh_sar_transmitter_get_cmd {
	uint16_t dst;
} __packed;

#define BTP_MESH_SAR_TRANSMITTER_SET		0x50
struct btp_mesh_sar_transmitter_set_cmd {
	uint16_t dst;
	struct sar_transmitter tx;
} __packed;

#define BTP_MESH_SAR_RECEIVER_GET		0x51
struct btp_mesh_sar_receiver_get_cmd {
	uint16_t dst;
} __packed;

#define BTP_MESH_SAR_RECEIVER_SET		0x52
struct btp_mesh_sar_receiver_set_cmd {
	uint16_t dst;
	struct sar_receiver rx;
} __packed;

#define BTP_MESH_LARGE_COMP_DATA_GET		0x53
struct btp_mesh_large_comp_data_get_cmd {
	uint16_t net_idx;
	uint16_t addr;
	uint8_t page;
	uint16_t offset;
} __packed;
struct btp_mesh_large_comp_data_get_rp {
	uint8_t data[0];
} __packed;

#define BTP_MESH_MODELS_METADATA_GET		0x54
struct btp_mesh_models_metadata_get_cmd {
	uint16_t net_idx;
	uint16_t addr;
	uint8_t page;
	uint16_t offset;
} __packed;
struct btp_mesh_models_metadata_get_rp {
	uint8_t data[0];
} __packed;

#define BTP_MESH_OPCODES_AGGREGATOR_INIT	0x55
struct btp_mesh_opcodes_aggregator_init_cmd {
	uint16_t net_idx;
	uint16_t app_idx;
	uint16_t dst;
	uint16_t elem_addr;
} __packed;

#define BTP_MESH_OPCODES_AGGREGATOR_SEND	0x56

#define BTP_MESH_COMP_CHANGE_PREPARE		0x57

#define BTP_MESH_RPR_SCAN_START			0x59
struct btp_rpr_scan_start_cmd {
	uint16_t dst;
	uint8_t timeout;
	uint8_t uuid[16];
} __packed;

#define BTP_MESH_RPR_EXT_SCAN_START		0x5a
struct btp_rpr_ext_scan_start_cmd {
	uint16_t dst;
	uint8_t timeout;
	uint8_t uuid[16];
	uint8_t ad_count;
	uint8_t ad_types[];
} __packed;

#define BTP_MESH_RPR_SCAN_CAPS_GET		0x5b
struct btp_rpr_scan_caps_get_cmd {
	uint16_t dst;
} __packed;

#define BTP_MESH_RPR_SCAN_GET			0x5c
struct btp_rpr_scan_get_cmd {
	uint16_t dst;
} __packed;

#define BTP_MESH_RPR_SCAN_STOP			0x5d
struct btp_rpr_scan_stop_cmd {
	uint16_t dst;
} __packed;

#define BTP_MESH_RPR_LINK_GET			0x5e
struct btp_rpr_link_get_cmd {
	uint16_t dst;
} __packed;

#define BTP_MESH_RPR_LINK_CLOSE			0x5f
struct btp_rpr_link_close_cmd {
	uint16_t dst;
} __packed;

#define BTP_MESH_RPR_PROV_REMOTE		0x60
struct btp_rpr_prov_remote_cmd {
	uint16_t dst;
	uint8_t uuid[16];
	uint16_t net_idx;
	uint16_t addr;
} __packed;

#define BTP_MESH_RPR_REPROV_REMOTE		0x61
struct btp_rpr_reprov_remote_cmd {
	uint16_t dst;
	uint16_t addr;
	bool comp_change;
} __packed;

#define BTP_MMDL_DFU_INFO_GET			0x5f
struct btp_mmdl_dfu_info_get_cmd {
	uint8_t limit;
} __packed;

#define BTP_MMDL_BLOB_INFO_GET			0x60
struct btp_mmdl_blob_info_get_cmd {
	uint8_t addr_cnt;
	uint8_t addr[];
} __packed;

#define BTP_MMDL_DFU_UPDATE_METADATA_CHECK	0x61
struct btp_mmdl_dfu_metadata_check_cmd {
	uint8_t index;
	uint8_t slot_idx;
	uint8_t slot_size;
	uint8_t fwid_len;
	uint8_t metadata_len;
	uint8_t data[];
} __packed;

struct btp_mmdl_dfu_metadata_check_rp {
	uint8_t idx;
	uint8_t status;
	uint8_t effect;
} __packed;

#define BTP_MMDL_DFU_FIRMWARE_UPDATE_GET	0x62
#define BTP_MMDL_DFU_FIRMWARE_UPDATE_CANCEL	0x63
#define BTP_MMDL_DFU_FIRMWARE_UPDATE_START	0x64
struct btp_mmdl_dfu_firmware_update_cmd {
	uint8_t addr_cnt;
	uint8_t slot_idx;
	uint8_t slot_size;
	uint8_t fwid_len;
	uint8_t metadata_len;
	uint8_t block_size;
	uint16_t chunk_size;
	uint8_t data[];
} __packed;

struct btp_mmdl_dfu_firmware_update_rp {
	uint8_t status;
} __packed;

#define BTP_MMDL_BLOB_SRV_RECV			0x65
struct btp_mmdl_blob_srv_recv_cmd {
	uint64_t id;
	uint16_t timeout;
	uint8_t ttl;
} __packed;

#define BTP_MMDL_BLOB_TRANSFER_START		0x66
struct btp_mmdl_blob_transfer_start_cmd {
	uint64_t id;
	uint16_t size;
	uint8_t block_size;
	uint16_t chunk_size;
	uint16_t timeout;
	uint8_t ttl;
} __packed;

#define BTP_MMDL_BLOB_TRANSFER_CANCEL		0x67
#define BTP_MMDL_BLOB_TRANSFER_GET		0x68
#define BTP_MMDL_BLOB_SRV_CANCEL		0x69
#define BTP_MMDL_DFU_FIRMWARE_UPDATE_APPLY	0x6A
#define BTP_MMDL_DFU_SRV_APPLY			0x6B

#define BTP_MESH_PRIV_BEACON_GET		0x6c
struct btp_priv_beacon_get_cmd {
	uint16_t dst;
} __packed;

#define BTP_MESH_PRIV_BEACON_SET		0x6d
struct btp_priv_beacon_set_cmd {
	uint16_t dst;
	uint8_t enabled;
	uint8_t rand_interval;
} __packed;

#define BTP_MESH_PRIV_GATT_PROXY_GET		0x6e
struct btp_priv_gatt_proxy_get_cmd {
	uint16_t dst;
} __packed;

#define BTP_MESH_PRIV_GATT_PROXY_SET		0x6f
struct btp_priv_gatt_proxy_set_cmd {
	uint16_t dst;
	uint8_t state;
} __packed;

#define BTP_MESH_PRIV_NODE_ID_GET		0x70
struct btp_priv_node_id_get_cmd {
	uint16_t dst;
	uint16_t key_net_idx;
} __packed;

#define BTP_MESH_PRIV_NODE_ID_SET		0x71
struct btp_priv_node_id_set_cmd {
	uint16_t dst;
	uint16_t net_idx;
	uint8_t state;
} __packed;

#define BTP_MESH_PROXY_PRIVATE_IDENTITY		0x72

#define BTP_MESH_OD_PRIV_PROXY_GET		0x73
struct btp_od_priv_proxy_get_cmd {
	uint16_t dst;
} __packed;

#define BTP_MESH_OD_PRIV_PROXY_SET		0x74

struct btp_od_priv_proxy_set_cmd {
	uint16_t dst;
	uint8_t val;
} __packed;

#define BTP_MESH_SRPL_CLEAR			0x75

struct btp_srpl_clear_cmd {
	uint16_t dst;
	uint16_t range_start;
	uint8_t range_len;
	uint8_t acked;
} __packed;

#define BTP_MESH_PROXY_SOLICIT			0x76

struct btp_proxy_solicit_cmd {
	uint16_t net_idx;
} __packed;

#define BTP_MESH_START				0x78

/* events */
#define BTP_MESH_EV_OUT_NUMBER_ACTION		0x80
struct btp_mesh_out_number_action_ev {
	uint16_t action;
	uint32_t number;
} __packed;

#define BTP_MESH_EV_OUT_STRING_ACTION		0x81
struct btp_mesh_out_string_action_ev {
	uint8_t string_len;
	uint8_t string[];
} __packed;

#define BTP_MESH_EV_IN_ACTION			0x82
struct btp_mesh_in_action_ev {
	uint16_t action;
	uint8_t size;
} __packed;

#define BTP_MESH_EV_PROVISIONED			0x83

#define BTP_MESH_PROV_BEARER_PB_ADV		0x00
#define BTP_MESH_PROV_BEARER_PB_GATT		0x01
#define BTP_MESH_PROV_BEARER_REMOTE		0x04
#define BTP_MESH_EV_PROV_LINK_OPEN		0x84
struct btp_mesh_prov_link_open_ev {
	uint8_t bearer;
} __packed;

#define BTP_MESH_EV_PROV_LINK_CLOSED		0x85
struct btp_mesh_prov_link_closed_ev {
	uint8_t bearer;
} __packed;

#define BTP_MESH_EV_NET_RECV			0x86
struct btp_mesh_net_recv_ev {
	uint8_t ttl;
	uint8_t ctl;
	uint16_t src;
	uint16_t dst;
	uint8_t payload_len;
	uint8_t payload[];
} __packed;

#define BTP_MESH_EV_INVALID_BEARER		0x87
struct btp_mesh_invalid_bearer_ev {
	uint8_t opcode;
} __packed;

#define BTP_MESH_EV_INCOMP_TIMER_EXP		0x88

#define BTP_MESH_EV_FRND_ESTABLISHED		0x89
struct btp_mesh_frnd_established_ev {
	uint16_t net_idx;
	uint16_t lpn_addr;
	uint8_t recv_delay;
	uint32_t polltimeout;
} __packed;

#define BTP_MESH_EV_FRND_TERMINATED		0x8a
struct btp_mesh_frnd_terminated_ev {
	uint16_t net_idx;
	uint16_t lpn_addr;
} __packed;

#define BTP_MESH_EV_LPN_ESTABLISHED		0x8b
struct btp_mesh_lpn_established_ev {
	uint16_t net_idx;
	uint16_t friend_addr;
	uint8_t queue_size;
	uint8_t recv_win;
} __packed;

#define BTP_MESH_EV_LPN_TERMINATED		0x8c
struct btp_mesh_lpn_terminated_ev {
	uint16_t net_idx;
	uint16_t friend_addr;
} __packed;

#define BTP_MESH_EV_LPN_POLLED			0x8d
struct btp_mesh_lpn_polled_ev {
	uint16_t net_idx;
	uint16_t friend_addr;
	uint8_t retry;
} __packed;

#define BTP_MESH_EV_PROV_NODE_ADDED		0x8e
struct btp_mesh_prov_node_added_ev {
	uint16_t net_idx;
	uint16_t addr;
	uint8_t uuid[16];
	uint8_t num_elems;
} __packed;

#define BTP_MESH_EV_MODEL_RECV			0x8f
struct btp_mesh_model_recv_ev {
	uint16_t src;
	uint16_t dst;
	uint8_t payload_len;
	uint8_t payload[];
} __packed;

#define MESH_EV_BLOB_LOST_TARGET		0x90
