/** @file
 *  @brief Bluetooth Mesh Profile APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_MESH_MAIN_H
#define __BT_MESH_MAIN_H

/**
 * @brief Bluetooth Mesh Provisioning
 * @defgroup bt_mesh_prov Bluetooth Mesh Provisioning
 * @ingroup bt_mesh
 * @{
 */

typedef enum {
	BT_MESH_NO_OUTPUT       = 0,
	BT_MESH_BLINK           = BIT(0),
	BT_MESH_BEEP            = BIT(1),
	BT_MESH_VIBRATE         = BIT(2),
	BT_MESH_DISPLAY_NUMBER  = BIT(3),
	BT_MESH_DISPLAY_STRING  = BIT(4),
} bt_mesh_output_action_t;

typedef enum {
	BT_MESH_NO_INPUT      = 0,
	BT_MESH_PUSH          = BIT(0),
	BT_MESH_TWIST         = BIT(1),
	BT_MESH_ENTER_NUMBER  = BIT(2),
	BT_MESH_ENTER_STRING  = BIT(3),
} bt_mesh_input_action_t;

struct bt_mesh_prov {
	const u8_t *uuid;

	const u8_t *static_val;
	u8_t        static_val_len;

	u8_t        output_size;
	u16_t       output_actions;

	u8_t        input_size;
	u16_t       input_actions;

	int         (*output_number)(bt_mesh_output_action_t act, u32_t num);
	int         (*output_string)(const char *str);

	int         (*input)(bt_mesh_input_action_t act, u8_t size);

	void        (*link_open)(bt);
	void        (*link_close)(void);
	void        (*complete)(u16_t addr);
};

int bt_mesh_input_string(const char *str);
int bt_mesh_input_number(u32_t num);

/**
 * @}
 */

/**
 * @brief Bluetooth Mesh
 * @defgroup bt_mesh Bluetooth Mesh
 * @ingroup bluetooth
 * @{
 */

/* Primary Network Key index */
#define BT_MESH_NET_PRIMARY                 0x000

#define BT_MESH_RELAY_DISABLED              0x00
#define BT_MESH_RELAY_ENABLED               0x01
#define BT_MESH_RELAY_NOT_SUPPORTED         0x02

#define BT_MESH_BEACON_DISABLED             0x00
#define BT_MESH_BEACON_ENABLED              0x01

#define BT_MESH_GATT_PROXY_DISABLED         0x00
#define BT_MESH_GATT_PROXY_ENABLED          0x01
#define BT_MESH_GATT_PROXY_NOT_SUPPORTED    0x02

#define BT_MESH_FRIEND_DISABLED             0x00
#define BT_MESH_FRIEND_ENABLED              0x01
#define BT_MESH_FRIEND_NOT_SUPPORTED        0x02

#define BT_MESH_NODE_IDENTITY_STOPPED       0x00
#define BT_MESH_NODE_IDENTITY_RUNNING       0x01
#define BT_MESH_NODE_IDENTITY_NOT_SUPPORTED 0x02

/* Features */
#define BT_MESH_FEAT_RELAY                  BIT(0)
#define BT_MESH_FEAT_PROXY                  BIT(1)
#define BT_MESH_FEAT_FRIEND                 BIT(2)
#define BT_MESH_FEAT_LOW_POWER              BIT(3)

/** @brief Initialize Mesh support
 *
 *  @param prov Node provisioning information.
 *  @param comp Node Composition.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_init(const struct bt_mesh_prov *prov,
		 const struct bt_mesh_comp *comp);

/** @brief Reset the state of the local Mesh node.
 *
 *  Resets the state of the node, which in turn causes it to start sending
 *  out unprovisioned beacons.
 */
void bt_mesh_reset(void);

/** @brief Provision the local Mesh Node.
 *
 *  This API should normally not be used directly by the application. The
 *  only exception is for testing purposes where manual provisioning is
 *  desired without an actual external provisioner.
 *
 *  @param net_key  Network Key
 *  @param net_idx  Network Key Index
 *  @param flags    Provisioning Flags
 *  @param iv_index IV Index
 *  @param seq      Sequence Number (0 if newly provisioned).
 *  @param addr     Primary element address
 *  @param dev_key  Device Key
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_provision(const u8_t net_key[16], u16_t net_idx,
		      u8_t flags, u32_t iv_index, u32_t seq,
		      u16_t addr, const u8_t dev_key[16]);

/** @brief Toggle the Low Power feature of the local device
 *
 *  Enables or disables the Low Power feature of the local device. This is
 *  exposed as a run-time feature, since the device might want to change
 *  this e.g. based on being plugged into a stable power source or running
 *  from a battery power source.
 *
 *  @param enable  true to enable LPN functionality, false to disable it.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_lpn_set(bool enable);

/**
 * @}
 */

#endif /* __BT_MESH_MAIN_H */
