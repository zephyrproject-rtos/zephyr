/** @file
 *  @brief Bluetooth Mesh Profile APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_MESH_H
#define __BT_MESH_H

#include <zephyr/types.h>
#include <stddef.h>
#include <net/buf.h>

/**
 * @brief Bluetooth Mesh Proxy
 * @defgroup bt_mesh_proxy Bluetooth Mesh Proxy
 * @ingroup bt_mesh
 * @{
 */

/**
 * @brief Enable advertising with Node Identity.
 *
 * This API requires that GATT Proxy support has been enabled. Once called
 * each subnet will start advertising using Node Identity for the next
 * 60 seconds.
 *
 * @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_proxy_identity_enable(void);

/**
 * @}
 */

/**
 * @brief Bluetooth Mesh Composition
 * @defgroup bt_comp Bluetooth Mesh Composition
 * @ingroup bt_mesh
 * @{
 */

#define BT_MESH_ADDR_UNASSIGNED   0x0000
#define BT_MESH_ADDR_ALL_NODES    0xffff
#define BT_MESH_ADDR_PROXIES      0xfffc
#define BT_MESH_ADDR_FRIENDS      0xfffd
#define BT_MESH_ADDR_RELAYS       0xfffe

#define BT_MESH_KEY_UNUSED        0xffff
#define BT_MESH_KEY_DEV           0xfffe

/**
 * @brief Bluetooth Mesh Element
 * @defgroup bt_mesh_elem Bluetooth Mesh Element
 * @ingroup bt_comp
 * @{
 */

/** Helper to define a mesh element within an array.
 *
 *  @param _loc       Location Descriptor.
 *  @param _mods      Array of models
 *  @param _vnd_mods  Array of vendor models.
 */
#define BT_MESH_ELEM(_loc, _mods, _vnd_mods)        \
{                                                   \
	.loc              = (_loc),                 \
	.model_count      = ARRAY_SIZE(_mods),      \
	.models           = (_mods),                \
	.vnd_model_count  = ARRAY_SIZE(_vnd_mods),  \
	.vnd_models       = (_vnd_mods),            \
}

/** Abstraction that describes a Mesh Element */
struct bt_mesh_elem {
	/* Unicast Address. Set at runtime during provisioning. */
	u16_t addr;

	/* Location Descriptor (GATT Bluetooth Namespace Descriptors) */
	const u16_t loc;

	const u8_t model_count;
	const u8_t vnd_model_count;

	struct bt_mesh_model * const models;
	struct bt_mesh_model * const vnd_models;
};

/**
 * @}
 */

/**
 * @brief Bluetooth Mesh Model
 * @defgroup bt_mesh_model Bluetooth Mesh Model
 * @ingroup bt_comp
 * @{
 */

/* Foundation Models */
#define BT_MESH_MODEL_ID_CFG_SRV                   0x0000
#define BT_MESH_MODEL_ID_CFG_CLI                   0x0001
#define BT_MESH_MODEL_ID_HEALTH_SRV                0x0002
#define BT_MESH_MODEL_ID_HEALTH_CLI                0x0003

/* Models from the Mesh Model Specification */
#define BT_MESH_MODEL_ID_GEN_ONOFF_SRV             0x1000
#define BT_MESH_MODEL_ID_GEN_ONOFF_CLI             0x1001
#define BT_MESH_MODEL_ID_GEN_LEVEL_SRV             0x1002
#define BT_MESH_MODEL_ID_GEN_LEVEL_CLI             0x1003
#define BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_SRV    0x1004
#define BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_CLI    0x1005
#define BT_MESH_MODEL_ID_GEN_POWER_ONOFF_SRV       0x1006
#define BT_MESH_MODEL_ID_GEN_POWER_ONOFF_SETUP_SRV 0x1007
#define BT_MESH_MODEL_ID_GEN_POWER_ONOFF_CLI       0x1008
#define BT_MESH_MODEL_ID_GEN_POWER_LEVEL_SRV       0x1009
#define BT_MESH_MODEL_ID_GEN_POWER_LEVEL_SETUP_SRV 0x100a
#define BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI       0x100b
#define BT_MESH_MODEL_ID_GEN_BATTERY_SRV           0x100c
#define BT_MESH_MODEL_ID_GEN_BATTERY_CLI           0x100d
#define BT_MESH_MODEL_ID_GEN_LOCATION_SRV          0x100e
#define BT_MESH_MODEL_ID_GEN_LOCATION_SETUPSRV     0x100f
#define BT_MESH_MODEL_ID_GEN_LOCATION_CLI          0x1010
#define BT_MESH_MODEL_ID_GEN_ADMIN_PROP_SRV        0x1011
#define BT_MESH_MODEL_ID_GEN_MANUFACTURER_PROP_SRV 0x1012
#define BT_MESH_MODEL_ID_GEN_USER_PROP_SRV         0x1013
#define BT_MESH_MODEL_ID_GEN_CLIENT_PROP_SRV       0x1014
#define BT_MESH_MODEL_ID_GEN_PROP_CLI              0x1015
#define BT_MESH_MODEL_ID_SENSOR_SRV                0x1100
#define BT_MESH_MODEL_ID_SENSOR_SETUP_SRV          0x1101
#define BT_MESH_MODEL_ID_SENSOR_CLI                0x1102
#define BT_MESH_MODEL_ID_TIME_SRV                  0x1200
#define BT_MESH_MODEL_ID_TIME_SETUP_SRV            0x1201
#define BT_MESH_MODEL_ID_TIME_CLI                  0x1202
#define BT_MESH_MODEL_ID_SCENE_SRV                 0x1203
#define BT_MESH_MODEL_ID_SCENE_SETUP_SRV           0x1204
#define BT_MESH_MODEL_ID_SCENE_CLI                 0x1205
#define BT_MESH_MODEL_ID_SCHEDULER_SRV             0x1206
#define BT_MESH_MODEL_ID_SCHEDULER_SETUP_SRV       0x1207
#define BT_MESH_MODEL_ID_SCHEDULER_CLI             0x1208
#define BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SRV       0x1300
#define BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SETUP_SRV 0x1301
#define BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI       0x1302
#define BT_MESH_MODEL_ID_LIGHT_CTL_SRV             0x1303
#define BT_MESH_MODEL_ID_LIGHT_CTL_SETUP_SRV       0x1304
#define BT_MESH_MODEL_ID_LIGHT_CTL_CLI             0x1305
#define BT_MESH_MODEL_ID_LIGHT_CTL_TEMP_SRV        0x1306
#define BT_MESH_MODEL_ID_LIGHT_HSL_SRV             0x1307
#define BT_MESH_MODEL_ID_LIGHT_HSL_SETUP_SRV       0x1308
#define BT_MESH_MODEL_ID_LIGHT_HSL_CLI             0x1309
#define BT_MESH_MODEL_ID_LIGHT_HSL_HUE_SRV         0x130a
#define BT_MESH_MODEL_ID_LIGHT_HSL_SAT_SRV         0x130b
#define BT_MESH_MODEL_ID_LIGHT_XYL_SRV             0x130c
#define BT_MESH_MODEL_ID_LIGHT_XYL_SETUP_SRV       0x130d
#define BT_MESH_MODEL_ID_LIGHT_XYL_CLI             0x130e
#define BT_MESH_MODEL_ID_LIGHT_LC_SRV              0x130f
#define BT_MESH_MODEL_ID_LIGHT_LC_SETUPSRV         0x1310
#define BT_MESH_MODEL_ID_LIGHT_LC_CLI              0x1311

/** Message sending context. */
struct bt_mesh_msg_ctx {
	/** NetKey Index of the subnet to send the message on. */
	u16_t net_idx;

	/** AppKey Index to encrypt the message with. */
	u16_t app_idx;

	/** Remote address. */
	u16_t addr;

	/** Received TTL value. Not used for sending. */
	u8_t  recv_ttl:7;

	/** Whether friend credentials are used. */
	u8_t  friend_cred:1;

	/** TTL, or BT_MESH_TTL_DEFAULT for default TTL. */
	u8_t  send_ttl;
};

struct bt_mesh_model_op {
	/* OpCode encoded using the BT_MESH_MODEL_OP_* macros */
	const u32_t  opcode;

	/* Minimum required message length */
	const size_t min_len;

	/* Message handler for the opcode */
	void (*const func)(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf);
};

#define BT_MESH_MODEL_OP_1(b0) (b0)
#define BT_MESH_MODEL_OP_2(b0, b1) (((b0) << 8) | (b1))
#define BT_MESH_MODEL_OP_3(b0, cid) ((((b0) << 16) | 0xc00000) | (cid))

#define BT_MESH_MODEL_OP_END { 0, 0, NULL }
#define BT_MESH_MODEL_NO_OPS ((struct bt_mesh_model_op []) \
			      { BT_MESH_MODEL_OP_END })

/** Helper to define an empty model array */
#define BT_MESH_MODEL_EMPTY ((struct bt_mesh_model []){})

#define BT_MESH_MODEL(_id, _op, _pub, _user_data)                            \
{                                                                            \
	.id = (_id),                                                         \
	.op = _op,                                                           \
	.keys = { [0 ... (CONFIG_BLUETOOTH_MESH_MODEL_KEY_COUNT - 1)] =      \
			BT_MESH_KEY_UNUSED },                                \
	.pub = _pub,                                                         \
	.groups = { [0 ... (CONFIG_BLUETOOTH_MESH_MODEL_GROUP_COUNT - 1)] =  \
			BT_MESH_ADDR_UNASSIGNED },                           \
	.user_data = _user_data,                                             \
}

#define BT_MESH_MODEL_VND(_company, _id, _op, _pub, _user_data)              \
{                                                                            \
	.vnd.company = (_company),                                           \
	.vnd.id = (_id),                                                     \
	.op = _op,                                                           \
	.pub = _pub,                                                         \
	.keys = { [0 ... (CONFIG_BLUETOOTH_MESH_MODEL_KEY_COUNT - 1)] =      \
			BT_MESH_KEY_UNUSED },                                \
	.groups = { [0 ... (CONFIG_BLUETOOTH_MESH_MODEL_GROUP_COUNT - 1)] =  \
			BT_MESH_ADDR_UNASSIGNED },                           \
	.user_data = _user_data,                                             \
}

/** Mesh Configuration Server Model Context */
struct bt_mesh_cfg {
	struct bt_mesh_model *model;

	u8_t net_transmit;         /* Network Transmit state */
	u8_t relay;                /* Relay Mode state */
	u8_t relay_retransmit;     /* Relay Retransmit state */
	u8_t beacon;               /* Secure Network Beacon state */
	u8_t gatt_proxy;           /* GATT Proxy state */
	u8_t frnd;                 /* Friend state */
	u8_t default_ttl;          /* Default TTL */

	/* Heartbeat Publication */
	struct {
		struct k_delayed_work timer;

		u16_t dst;
		u16_t count;
		u8_t  period;
		u8_t  ttl;
		u16_t feat;
		u16_t net_idx;
	} hb_pub;

	/* Heartbeat Subscription */
	struct {
		s64_t  expiry;

		u16_t src;
		u16_t dst;
		u16_t count;
		u8_t  min_hops;
		u8_t  max_hops;

		/* Optional subscription tracking function */
		void (*func)(u8_t hops, u16_t feat);
	} hb_sub;
};

extern const struct bt_mesh_model_op bt_mesh_cfg_op[];

#define BT_MESH_MODEL_CFG_SRV(srv_data)                                      \
		BT_MESH_MODEL(BT_MESH_MODEL_ID_CFG_SRV,                      \
			      bt_mesh_cfg_op, NULL, srv_data)

/** @def BT_MESH_TRANSMIT
 *
 *  @brief Encode transmission count & interval steps.
 *
 *  @param count   Number of retransmissions (first transmission is excluded).
 *  @param int_ms  Interval steps in milliseconds. Must be greater than 0
 *                 and a multiple of 10.
 *
 *  @return Mesh transmit value that can be used e.g. for the default
 *          values of the configuration model data.
 */
#define BT_MESH_TRANSMIT(count, int_ms) ((count) | (((int_ms / 10) - 1) << 3))

/** Mesh Health Server Model Context */
struct bt_mesh_health {
	struct bt_mesh_model *model;

	/* Health Period (divider) */
	u8_t period;

	/* Fetch current faults */
	int (*fault_get_cur)(struct bt_mesh_model *model, u8_t *test_id,
			     u16_t *company_id, u8_t *faults,
			     u8_t *fault_count);

	/* Fetch registered faults */
	int (*fault_get_reg)(struct bt_mesh_model *model, u16_t company_id,
			     u8_t *test_id, u8_t *faults,
			     u8_t *fault_count);

	/* Clear registered faults */
	int (*fault_clear)(struct bt_mesh_model *model, u16_t company_id);

	/* Run a specific test */
	int (*fault_test)(struct bt_mesh_model *model, u8_t test_id,
			  u16_t company_id);

	/* Attention Timer state */
	struct {
		struct k_delayed_work timer;
		void (*on)(struct bt_mesh_model *model);
		void (*off)(struct bt_mesh_model *model);
	} attention;
};

int bt_mesh_fault_update(struct bt_mesh_elem *elem);

extern const struct bt_mesh_model_op bt_mesh_health_op[];
extern struct bt_mesh_model_pub bt_mesh_health_pub;

#define BT_MESH_MODEL_HEALTH_SRV(srv_data)                                   \
		BT_MESH_MODEL(BT_MESH_MODEL_ID_HEALTH_SRV,                   \
			      bt_mesh_health_op, &bt_mesh_health_pub,        \
			      srv_data)

struct bt_mesh_model_pub {
	/* Self-reference for easy lookup */
	struct bt_mesh_model *mod;

	u16_t addr;         /* Publish Address */
	u16_t key;          /* Publish AppKey Index */

	u32_t ttl:8,        /* Publish Time to Live */
	      retransmit:8, /* Retransmit Count & Interval Steps */
	      period:8,     /* Publish Period */
	      period_div:4, /* Divisor for the Period */
	      cred:1;       /* Friendship Credentials Flag */

	/* Publish callback */
	void    (*func)(struct bt_mesh_model *mod);

	/* Publish Period Timer */
	struct k_delayed_work timer;
};

/** Abstraction that describes a Mesh Model instance */
struct bt_mesh_model {
	union {
		const u16_t id;
		struct {
			u16_t company;
			u16_t id;
		} vnd;
	};

	/* The Element this Model belongs to */
	struct bt_mesh_elem *elem;

	/* Model Publication */
	struct bt_mesh_model_pub * const pub;

	/* AppKey List */
	u16_t keys[CONFIG_BLUETOOTH_MESH_MODEL_KEY_COUNT];

	/* Subscription List (group or virtual addresses) */
	u16_t groups[CONFIG_BLUETOOTH_MESH_MODEL_GROUP_COUNT];

	const struct bt_mesh_model_op * const op;

	/* Model-specific user data */
	void *user_data;
};

typedef void (*bt_mesh_cb_t)(int err, void *cb_data);

void bt_mesh_model_msg_init(struct net_buf_simple *msg, u32_t opcode);

/** Special TTL value to request using configured default TTL */
#define BT_MESH_TTL_DEFAULT 0xff

/** Maximum allowed TTL value */
#define BT_MESH_TTL_MAX     0x7f

/**
 * @brief Send an Access Layer message.
 *
 * @param model     Mesh (client) Model that the message belongs to.
 * @param ctx       Message context, includes keys, TTL, etc.
 * @param msg       Access Layer payload (the actual message to be sent).
 * @param cb        Optional "message sent" callback.
 * @param cb_data   User data to be passed to the callback.
 *
 * @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_model_send(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *msg, bt_mesh_cb_t cb,
		       void *cb_data);

/**
 * @brief Send a model publication message.
 *
 * @param model  Mesh (client) Model that's publishing the message.
 * @param msg    Access Layer message to publish.
 *
 * @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_model_publish(struct bt_mesh_model *model,
			  struct net_buf_simple *msg);

/**
 * @}
 */

/** Node Composition */
struct bt_mesh_comp {
	u16_t cid;
	u16_t pid;
	u16_t vid;

	size_t elem_count;
	struct bt_mesh_elem *elem;
};

/**
 * @}
 */

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
} bt_mesh_output_action;

typedef enum {
	BT_MESH_NO_INPUT      = 0,
	BT_MESH_PUSH          = BIT(0),
	BT_MESH_TWIST         = BIT(1),
	BT_MESH_ENTER_NUMBER  = BIT(2),
	BT_MESH_ENTER_STRING  = BIT(3),
} bt_mesh_input_action;

struct bt_mesh_prov {
	const u8_t *uuid;

	u8_t       *static_val;
	u8_t        static_val_len;

	u8_t        output_size;
	u16_t       output_actions;

	u8_t        input_size;
	u16_t       input_actions;

	int         (*output_number)(bt_mesh_output_action act, u32_t num);
	int         (*output_string)(const char *str);

	int         (*input)(bt_mesh_input_action act, u8_t size);

	void        (*complete)(void);
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

#endif /* __BT_MESH_H */
