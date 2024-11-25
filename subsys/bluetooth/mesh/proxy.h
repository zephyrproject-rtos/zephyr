/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_MESH_DEBUG_USE_ID_ADDR)
#define ADV_OPT_USE_IDENTITY BT_LE_ADV_OPT_USE_IDENTITY
#else
#define ADV_OPT_USE_IDENTITY 0
#endif

#define ADV_SLOW_INT                                                           \
	.interval_min = BT_GAP_ADV_SLOW_INT_MIN,                               \
	.interval_max = BT_GAP_ADV_SLOW_INT_MAX

#define ADV_FAST_INT                                                           \
	.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,                             \
	.interval_max = BT_GAP_ADV_FAST_INT_MAX_2

#define BT_DEVICE_NAME (IS_ENABLED(CONFIG_BT_DEVICE_NAME_DYNAMIC) ? \
			(const uint8_t *)bt_get_name() : \
			(const uint8_t *)CONFIG_BT_DEVICE_NAME)
#define BT_DEVICE_NAME_LEN (IS_ENABLED(CONFIG_BT_DEVICE_NAME_DYNAMIC) ? strlen(bt_get_name()) : \
			    (sizeof(CONFIG_BT_DEVICE_NAME) - 1))

#define BT_MESH_ID_TYPE_NET	  0x00
#define BT_MESH_ID_TYPE_NODE	  0x01
#define BT_MESH_ID_TYPE_PRIV_NET  0x02
#define BT_MESH_ID_TYPE_PRIV_NODE 0x03

int bt_mesh_proxy_gatt_enable(void);
int bt_mesh_proxy_gatt_disable(void);
void bt_mesh_proxy_gatt_disconnect(void);

void bt_mesh_proxy_beacon_send(struct bt_mesh_subnet *sub);

int bt_mesh_proxy_adv_start(void);

void bt_mesh_proxy_identity_start(struct bt_mesh_subnet *sub, bool private);
void bt_mesh_proxy_identity_stop(struct bt_mesh_subnet *sub);

bool bt_mesh_proxy_relay(struct bt_mesh_adv *adv, uint16_t dst);
void bt_mesh_proxy_addr_add(struct net_buf_simple *buf, uint16_t addr);
uint8_t bt_mesh_proxy_srv_connected_cnt(void);
