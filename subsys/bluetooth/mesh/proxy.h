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

#if defined(CONFIG_BT_MESH_PROXY_USE_DEVICE_NAME)
#define ADV_OPT_USE_NAME BT_LE_ADV_OPT_USE_NAME
#else
#define ADV_OPT_USE_NAME 0
#endif

#define ADV_OPT_PROXY                                                          \
	(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_SCANNABLE |                 \
	 BT_LE_ADV_OPT_ONE_TIME | ADV_OPT_USE_IDENTITY |                       \
	 ADV_OPT_USE_NAME)

#define ADV_OPT_PROV                                                           \
	(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_SCANNABLE |                 \
	 BT_LE_ADV_OPT_ONE_TIME | ADV_OPT_USE_IDENTITY |                       \
	 BT_LE_ADV_OPT_USE_NAME)

#define ADV_SLOW_INT                                                           \
	.interval_min = BT_GAP_ADV_SLOW_INT_MIN,                               \
	.interval_max = BT_GAP_ADV_SLOW_INT_MAX

#define ADV_FAST_INT                                                           \
	.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,                             \
	.interval_max = BT_GAP_ADV_FAST_INT_MAX_2

int bt_mesh_proxy_gatt_enable(void);
int bt_mesh_proxy_gatt_disable(void);
void bt_mesh_proxy_gatt_disconnect(void);

void bt_mesh_proxy_beacon_send(struct bt_mesh_subnet *sub);

int bt_mesh_proxy_adv_start(void);

void bt_mesh_proxy_identity_start(struct bt_mesh_subnet *sub);
void bt_mesh_proxy_identity_stop(struct bt_mesh_subnet *sub);

bool bt_mesh_proxy_relay(struct net_buf *buf, uint16_t dst);
void bt_mesh_proxy_addr_add(struct net_buf_simple *buf, uint16_t addr);

int bt_mesh_proxy_init(void);
