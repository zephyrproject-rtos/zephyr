/** @file
 *  @brief Bluetooth Mesh Protocol APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_MAIN_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_MAIN_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>

/**
 * @brief Provisioning
 * @defgroup bt_mesh_prov Provisioning
 * @ingroup bt_mesh
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Available authentication algorithms. */
enum {
	BT_MESH_PROV_AUTH_CMAC_AES128_AES_CCM,
	BT_MESH_PROV_AUTH_HMAC_SHA256_AES_CCM,
};

/** OOB Type field values. */
enum {
	BT_MESH_STATIC_OOB_AVAILABLE = BIT(0), /**< Static OOB information available */
	BT_MESH_OOB_AUTH_REQUIRED    = BIT(1)  /**< OOB authentication required */
};

/** Available Provisioning output authentication actions. */
typedef enum {
	BT_MESH_NO_OUTPUT       = 0,
	BT_MESH_BLINK           = BIT(0),   /**< Blink */
	BT_MESH_BEEP            = BIT(1),   /**< Beep */
	BT_MESH_VIBRATE         = BIT(2),   /**< Vibrate */
	BT_MESH_DISPLAY_NUMBER  = BIT(3),   /**< Output numeric */
	BT_MESH_DISPLAY_STRING  = BIT(4),   /**< Output alphanumeric */
} bt_mesh_output_action_t;

/** Available Provisioning input authentication actions. */
typedef enum {
	BT_MESH_NO_INPUT      = 0,
	BT_MESH_PUSH          = BIT(0),	   /**< Push */
	BT_MESH_TWIST         = BIT(1),	   /**< Twist */
	BT_MESH_ENTER_NUMBER  = BIT(2),	   /**< Input number */
	BT_MESH_ENTER_STRING  = BIT(3),	   /**< Input alphanumeric */
} bt_mesh_input_action_t;

/** Available Provisioning bearers. */
typedef enum {
	BT_MESH_PROV_ADV    = BIT(0),	/**< PB-ADV bearer */
	BT_MESH_PROV_GATT   = BIT(1),	/**< PB-GATT bearer */
	BT_MESH_PROV_REMOTE = BIT(2),	/**< PB-Remote bearer */
} bt_mesh_prov_bearer_t;

/** Out of Band information location. */
typedef enum {
	BT_MESH_PROV_OOB_OTHER       = BIT(0),   /**< Other */
	BT_MESH_PROV_OOB_URI         = BIT(1),   /**< Electronic / URI */
	BT_MESH_PROV_OOB_2D_CODE     = BIT(2),   /**< 2D machine-readable code */
	BT_MESH_PROV_OOB_BAR_CODE    = BIT(3),   /**< Bar Code */
	BT_MESH_PROV_OOB_NFC         = BIT(4),   /**< Near Field Communication (NFC) */
	BT_MESH_PROV_OOB_NUMBER      = BIT(5),   /**< Number */
	BT_MESH_PROV_OOB_STRING      = BIT(6),   /**< String */
	BT_MESH_PROV_OOB_CERTIFICATE = BIT(7),   /**< Support for certificate-based provisioning */
	BT_MESH_PROV_OOB_RECORDS     = BIT(8),   /**< Support for provisioning records */
	/* 9 - 10 are reserved */
	BT_MESH_PROV_OOB_ON_BOX      = BIT(11),  /**< On box */
	BT_MESH_PROV_OOB_IN_BOX      = BIT(12),  /**< Inside box */
	BT_MESH_PROV_OOB_ON_PAPER    = BIT(13),  /**< On piece of paper */
	BT_MESH_PROV_OOB_IN_MANUAL   = BIT(14),  /**< Inside manual */
	BT_MESH_PROV_OOB_ON_DEV      = BIT(15),  /**< On device */
} bt_mesh_prov_oob_info_t;

/** Device Capabilities. */
struct bt_mesh_dev_capabilities {
	/** Number of elements supported by the device */
	uint8_t elem_count;

	/** Supported algorithms and other capabilities */
	uint16_t algorithms;

	/** Supported public key types */
	uint8_t pub_key_type;

	/** Supported OOB Types */
	uint8_t oob_type;

	/** Supported Output OOB Actions */
	bt_mesh_output_action_t output_actions;

	/** Supported Input OOB Actions */
	bt_mesh_input_action_t input_actions;

	/** Maximum size of Output OOB supported */
	uint8_t output_size;

	/** Maximum size in octets of Input OOB supported */
	uint8_t input_size;
};

/** Provisioning properties & capabilities. */
struct bt_mesh_prov {
	/** The UUID that's used when advertising as unprovisioned */
	const uint8_t *uuid;

	/** Optional URI. This will be advertised separately from the
	 *  unprovisioned beacon, however the unprovisioned beacon will
	 *  contain a hash of it so the two can be associated by the
	 *  provisioner.
	 */
	const char *uri;

	/** Out of Band information field. */
	bt_mesh_prov_oob_info_t oob_info;

	/** Pointer to Public Key in big-endian for OOB public key type support.
	 *
	 * Remember to enable @kconfig{CONFIG_BT_MESH_PROV_OOB_PUBLIC_KEY}
	 * when initializing this parameter.
	 *
	 * Must be used together with @ref bt_mesh_prov::private_key_be.
	 */
	const uint8_t *public_key_be;
	/** Pointer to Private Key in big-endian for OOB public key type support.
	 *
	 * Remember to enable @kconfig{CONFIG_BT_MESH_PROV_OOB_PUBLIC_KEY}
	 * when initializing this parameter.
	 *
	 * Must be used together with @ref bt_mesh_prov::public_key_be.
	 */
	const uint8_t *private_key_be;

	/** Static OOB value */
	const uint8_t *static_val;
	/** Static OOB value length */
	uint8_t        static_val_len;

	/** Maximum size of Output OOB supported */
	uint8_t        output_size;
	/** Supported Output OOB Actions */
	uint16_t       output_actions;

	/** Maximum size of Input OOB supported */
	uint8_t        input_size;
	/** Supported Input OOB Actions */
	uint16_t       input_actions;

	/** @brief Provisioning Capabilities.
	 *
	 *  This callback notifies the application that the provisioning capabilities
	 *  of the unprovisioned device has been received.
	 *
	 *  The application can consequently call bt_mesh_auth_method_set_<*> to
	 *  select suitable provisioning oob authentication method.
	 *
	 *  When this callback returns, the provisioner will start authentication with
	 *  the chosen method.
	 *
	 *  @param cap capabilities supported by device.
	 */
	void         (*capabilities)(const struct bt_mesh_dev_capabilities *cap);

	/** @brief Output of a number is requested.
	 *
	 *  This callback notifies the application that it should
	 *  output the given number using the given action.
	 *
	 *  @param act Action for outputting the number.
	 *  @param num Number to be outputted.
	 *
	 *  @return Zero on success or negative error code otherwise
	 */
	int         (*output_number)(bt_mesh_output_action_t act, uint32_t num);

	/** @brief Output of a string is requested.
	 *
	 *  This callback notifies the application that it should
	 *  display the given string to the user.
	 *
	 *  @param str String to be displayed.
	 *
	 *  @return Zero on success or negative error code otherwise
	 */
	int         (*output_string)(const char *str);

	/** @brief Input is requested.
	 *
	 *  This callback notifies the application that it should
	 *  request input from the user using the given action. The
	 *  requested input will either be a string or a number, and
	 *  the application needs to consequently call the
	 *  bt_mesh_input_string() or bt_mesh_input_number() functions
	 *  once the data has been acquired from the user.
	 *
	 *  @param act Action for inputting data.
	 *  @param num Maximum size of the inputted data.
	 *
	 *  @return Zero on success or negative error code otherwise
	 */
	int         (*input)(bt_mesh_input_action_t act, uint8_t size);

	/** @brief The other device finished their OOB input.
	 *
	 *  This callback notifies the application that it should stop
	 *  displaying its output OOB value, as the other party finished their
	 *  OOB input.
	 */
	void 	    (*input_complete)(void);

	/** @brief Unprovisioned beacon has been received.
	 *
	 *  This callback notifies the application that an unprovisioned
	 *  beacon has been received.
	 *
	 *  @param uuid     UUID
	 *  @param oob_info OOB Information
	 *  @param uri_hash Pointer to URI Hash value. NULL if no hash was
	 *                  present in the beacon.
	 */
	void        (*unprovisioned_beacon)(uint8_t uuid[16],
					    bt_mesh_prov_oob_info_t oob_info,
					    uint32_t *uri_hash);

	/** @brief PB-GATT Unprovisioned Advertising has been received.
	 *
	 *  This callback notifies the application that an PB-GATT
	 *  unprovisioned Advertising has been received.
	 *
	 *  @param uuid     UUID
	 *  @param oob_info OOB Information
	 */
	void        (*unprovisioned_beacon_gatt)(uint8_t uuid[16],
						 bt_mesh_prov_oob_info_t oob_info);

	/** @brief Provisioning link has been opened.
	 *
	 *  This callback notifies the application that a provisioning
	 *  link has been opened on the given provisioning bearer.
	 *
	 *  @param bearer Provisioning bearer.
	 */
	void        (*link_open)(bt_mesh_prov_bearer_t bearer);

	/** @brief Provisioning link has been closed.
	 *
	 *  This callback notifies the application that a provisioning
	 *  link has been closed on the given provisioning bearer.
	 *
	 *  @param bearer Provisioning bearer.
	 */
	void        (*link_close)(bt_mesh_prov_bearer_t bearer);

	/** @brief Provisioning is complete.
	 *
	 *  This callback notifies the application that provisioning has
	 *  been successfully completed, and that the local node has been
	 *  assigned the specified NetKeyIndex and primary element address.
	 *
	 *  @param net_idx NetKeyIndex given during provisioning.
	 *  @param addr    Primary element address.
	 */
	void        (*complete)(uint16_t net_idx, uint16_t addr);

	/** @brief Local node has been reprovisioned.
	 *
	 *  This callback notifies the application that reprovisioning has
	 *  been successfully completed.
	 *
	 *  @param addr    New primary element address.
	 */
	void        (*reprovisioned)(uint16_t addr);

	/** @brief A new node has been added to the provisioning database.
	 *
	 *  This callback notifies the application that provisioning has
	 *  been successfully completed, and that a node has been assigned
	 *  the specified NetKeyIndex and primary element address.
	 *
	 *  @param net_idx  NetKeyIndex given during provisioning.
	 *  @param uuid     UUID of the added node
	 *  @param addr     Primary element address.
	 *  @param num_elem Number of elements that this node has.
	 */
	void        (*node_added)(uint16_t net_idx, uint8_t uuid[16], uint16_t addr,
				  uint8_t num_elem);

	/** @brief Node has been reset.
	 *
	 *  This callback notifies the application that the local node
	 *  has been reset and needs to be provisioned again. The node will
	 *  not automatically advertise as unprovisioned, rather the
	 *  bt_mesh_prov_enable() API needs to be called to enable
	 *  unprovisioned advertising on one or more provisioning bearers.
	 */
	void        (*reset)(void);
};

struct bt_mesh_rpr_cli;
struct bt_mesh_rpr_node;

/** @brief Provide provisioning input OOB string.
 *
 *  This is intended to be called after the bt_mesh_prov input callback
 *  has been called with BT_MESH_ENTER_STRING as the action.
 *
 *  @param str String.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_input_string(const char *str);

/** @brief Provide provisioning input OOB number.
 *
 *  This is intended to be called after the bt_mesh_prov input callback
 *  has been called with BT_MESH_ENTER_NUMBER as the action.
 *
 *  @param num Number.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_input_number(uint32_t num);

/** @brief Provide Device public key.
 *
 *  @param public_key Device public key in big-endian.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_prov_remote_pub_key_set(const uint8_t public_key[64]);

/** @brief Use Input OOB authentication.
 *
 *  Provisioner only.
 *
 *  Instruct the unprovisioned device to use the specified Input OOB
 *  authentication action. When using @ref BT_MESH_PUSH, @ref BT_MESH_TWIST or
 *  @ref BT_MESH_ENTER_NUMBER, the @ref bt_mesh_prov::output_number callback is
 *  called with a random number that has to be entered on the unprovisioned
 *  device.
 *
 *  When using @ref BT_MESH_ENTER_STRING, the @ref bt_mesh_prov::output_string
 *  callback is called with a random string that has to be entered on the
 *  unprovisioned device.
 *
 *  @param action Authentication action used by the unprovisioned device.
 *  @param size Authentication size.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_auth_method_set_input(bt_mesh_input_action_t action, uint8_t size);

/** @brief Use Output OOB authentication.
 *
 *  Provisioner only.
 *
 *  Instruct the unprovisioned device to use the specified Output OOB
 *  authentication action. The @ref bt_mesh_prov::input callback will
 *  be called.
 *
 *  When using @ref BT_MESH_BLINK, @ref BT_MESH_BEEP, @ref BT_MESH_VIBRATE
 *  or @ref BT_MESH_DISPLAY_NUMBER, and the application has to call
 *  @ref bt_mesh_input_number with the random number indicated by
 *  the unprovisioned device.
 *
 *  When using @ref BT_MESH_DISPLAY_STRING, the application has to call
 *  @ref bt_mesh_input_string with the random string displayed by the
 *  unprovisioned device.
 *
 *  @param action Authentication action used by the unprovisioned device.
 *  @param size Authentication size.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_auth_method_set_output(bt_mesh_output_action_t action, uint8_t size);

/** @brief Use static OOB authentication.
 *
 *  Provisioner only.
 *
 *  Instruct the unprovisioned device to use static OOB authentication, and use
 *  the given static authentication value when provisioning.
 *
 *  @param static_val Static OOB value.
 *  @param size Static OOB value size.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_auth_method_set_static(const uint8_t *static_val, uint8_t size);

/** @brief Don't use OOB authentication.
 *
 *  Provisioner only.
 *
 *  Don't use any authentication when provisioning new devices. This is the
 *  default behavior.
 *
 *  @warning Not using any authentication exposes the mesh network to
 *           impersonation attacks, where attackers can pretend to be the
 *           unprovisioned device to gain access to the network. Authentication
 *           is strongly encouraged.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_auth_method_set_none(void);

/** @brief Enable specific provisioning bearers
 *
 *  Enable one or more provisioning bearers.
 *
 *  @param bearers Bit-wise or of provisioning bearers.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_prov_enable(bt_mesh_prov_bearer_t bearers);

/** @brief Disable specific provisioning bearers
 *
 *  Disable one or more provisioning bearers.
 *
 *  @param bearers Bit-wise or of provisioning bearers.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_prov_disable(bt_mesh_prov_bearer_t bearers);

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
 *  @param addr     Primary element address
 *  @param dev_key  Device Key
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_provision(const uint8_t net_key[16], uint16_t net_idx,
		      uint8_t flags, uint32_t iv_index, uint16_t addr,
		      const uint8_t dev_key[16]);

/** @brief Provision a Mesh Node using PB-ADV
 *
 *  @param uuid               UUID
 *  @param net_idx            Network Key Index
 *  @param addr               Address to assign to remote device. If addr is 0,
 *                            the lowest available address will be chosen.
 *  @param attention_duration The attention duration to be send to remote device
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_provision_adv(const uint8_t uuid[16], uint16_t net_idx, uint16_t addr,
			  uint8_t attention_duration);

/** @brief Provision a Mesh Node using PB-GATT
 *
 *  @param uuid               UUID
 *  @param net_idx            Network Key Index
 *  @param addr               Address to assign to remote device. If addr is 0,
 *                            the lowest available address will be chosen.
 *  @param attention_duration The attention duration to be send to remote device
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_provision_gatt(const uint8_t uuid[16], uint16_t net_idx, uint16_t addr,
			   uint8_t attention_duration);

/** @brief Provision a Mesh Node using PB-Remote
 *
 *  @param cli     Remote Provisioning Client Model to provision with.
 *  @param srv     Remote Provisioning Server that should be used to tunnel the
 *                 provisioning.
 *  @param uuid    UUID of the unprovisioned node
 *  @param net_idx Network Key Index to give to the unprovisioned node.
 *  @param addr    Address to assign to remote device. If addr is 0,
 *                 the lowest available address will be chosen.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_provision_remote(struct bt_mesh_rpr_cli *cli,
			     const struct bt_mesh_rpr_node *srv,
			     const uint8_t uuid[16], uint16_t net_idx,
			     uint16_t addr);

/** @brief Reprovision a Mesh Node using PB-Remote
 *
 *  Reprovisioning can be used to change the device key, unicast address and
 *  composition data of another device. The reprovisioning procedure uses the
 *  same protocol as normal provisioning, with the same level of security.
 *
 *  There are three tiers of reprovisioning:
 *  1. Refreshing the device key
 *  2. Refreshing the device key and node address. Composition data may change,
 *     including the number of elements.
 *  3. Refreshing the device key and composition data, in case the composition
 *     data of the target node changed due to a firmware update or a similar
 *     procedure.
 *
 *  The target node indicates that its composition data changed by instantiating
 *  its composition data page 128. If the number of elements have changed, it
 *  may be necessary to move the unicast address of the target node as well, to
 *  avoid overlapping addresses.
 *
 *  @note Changing the unicast addresses of the target node requires changes to
 *        all nodes that publish directly to any of the target node's models.
 *
 *  @param cli         Remote Provisioning Client Model to provision on
 *  @param srv         Remote Provisioning Server to reprovision
 *  @param addr        Address to assign to remote device. If addr is 0, the
 *                     lowest available address will be chosen.
 *  @param comp_change The target node has indicated that its composition
 *                     data has changed. Note that the target node will reject
 *                     the update if this isn't true.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_reprovision_remote(struct bt_mesh_rpr_cli *cli,
			       struct bt_mesh_rpr_node *srv,
			       uint16_t addr, bool comp_change);

/** @brief Check if the local node has been provisioned.
 *
 *  This API can be used to check if the local node has been provisioned
 *  or not. It can e.g. be helpful to determine if there was a stored
 *  network in flash, i.e. if the network was restored after calling
 *  settings_load().
 *
 *  @return True if the node is provisioned. False otherwise.
 */
bool bt_mesh_is_provisioned(void);

/**
 * @}
 */

/**
 * @brief Bluetooth Mesh
 * @defgroup bt_mesh Bluetooth Mesh
 * @ingroup bluetooth
 * @{
 */

/** Primary Network Key index */
#define BT_MESH_NET_PRIMARY                 0x000

/** Relay feature */
#define BT_MESH_FEAT_RELAY                  BIT(0)
/** GATT Proxy feature */
#define BT_MESH_FEAT_PROXY                  BIT(1)
/** Friend feature */
#define BT_MESH_FEAT_FRIEND                 BIT(2)
/** Low Power Node feature */
#define BT_MESH_FEAT_LOW_POWER              BIT(3)
/** Supported heartbeat publication features */
#define BT_MESH_FEAT_SUPPORTED              (BT_MESH_FEAT_RELAY |   \
					     BT_MESH_FEAT_PROXY |   \
					     BT_MESH_FEAT_FRIEND |  \
					     BT_MESH_FEAT_LOW_POWER)

/** @brief Initialize Mesh support
 *
 *  After calling this API, the node will not automatically advertise as
 *  unprovisioned, rather the bt_mesh_prov_enable() API needs to be called
 *  to enable unprovisioned advertising on one or more provisioning bearers.
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
 *  Resets the state of the node, which means that it needs to be
 *  reprovisioned to become an active node in a Mesh network again.
 *
 *  After calling this API, the node will not automatically advertise as
 *  unprovisioned, rather the bt_mesh_prov_enable() API needs to be called
 *  to enable unprovisioned advertising on one or more provisioning bearers.
 *
 */
void bt_mesh_reset(void);

/** @brief Suspend the Mesh network temporarily.
 *
 *  This API can be used for power saving purposes, but the user should be
 *  aware that leaving the local node suspended for a long period of time
 *  may cause it to become permanently disconnected from the Mesh network.
 *  If at all possible, the Friendship feature should be used instead, to
 *  make the node into a Low Power Node.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_suspend(void);

/** @brief Resume a suspended Mesh network.
 *
 *  This API resumes the local node, after it has been suspended using the
 *  bt_mesh_suspend() API.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_resume(void);

/** @brief Toggle the IV Update test mode
 *
 *  This API is only available if the IV Update test mode has been enabled
 *  in Kconfig. It is needed for passing most of the IV Update qualification
 *  test cases.
 *
 *  @param enable true to enable IV Update test mode, false to disable it.
 */
void bt_mesh_iv_update_test(bool enable);

/** @brief Toggle the IV Update state
 *
 *  This API is only available if the IV Update test mode has been enabled
 *  in Kconfig. It is needed for passing most of the IV Update qualification
 *  test cases.
 *
 *  @return true if IV Update In Progress state was entered, false otherwise.
 */
bool bt_mesh_iv_update(void);

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

/** @brief Send out a Friend Poll message.
 *
 *  Send a Friend Poll message to the Friend of this node. If there is no
 *  established Friendship the function will return an error.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_lpn_poll(void);

/** Low Power Node callback functions. */
struct bt_mesh_lpn_cb {
	/** @brief Friendship established.
	 *
	 *  This callback notifies the application that friendship has
	 *  been successfully established.
	 *
	 *  @param net_idx  NetKeyIndex used during friendship establishment.
	 *  @param friend_addr Friend address.
	 *  @param queue_size  Friend queue size.
	 *  @param recv_window Low Power Node's listens duration for
	 *  Friend response.
	 */
	void (*established)(uint16_t net_idx, uint16_t friend_addr,
			    uint8_t queue_size, uint8_t recv_window);

	/** @brief Friendship terminated.
	 *
	 *  This callback notifies the application that friendship has
	 *  been terminated.
	 *
	 *  @param net_idx  NetKeyIndex used during friendship establishment.
	 *  @param friend_addr Friend address.
	 */
	void (*terminated)(uint16_t net_idx, uint16_t friend_addr);

	/** @brief Local Poll Request.
	 *
	 *  This callback notifies the application that the local node has
	 *  polled the friend node.
	 *
	 *  This callback will be called before @ref bt_mesh_lpn_cb::established
	 *  when attempting to establish a friendship.
	 *
	 *  @param net_idx  NetKeyIndex used during friendship establishment.
	 *  @param friend_addr Friend address.
	 *  @param retry Retry or first poll request for each transaction.
	 */
	void (*polled)(uint16_t net_idx, uint16_t friend_addr, bool retry);
};

/**
 *  @brief Register a callback structure for Friendship events.
 *
 *  @param _name Name of callback structure.
 */
#define BT_MESH_LPN_CB_DEFINE(_name)                                  \
	static const STRUCT_SECTION_ITERABLE(bt_mesh_lpn_cb,          \
					     _CONCAT(bt_mesh_lpn_cb_, \
						     _name))

/** Friend Node callback functions. */
struct bt_mesh_friend_cb {
	/** @brief Friendship established.
	 *
	 *  This callback notifies the application that friendship has
	 *  been successfully established.
	 *
	 *  @param net_idx  NetKeyIndex used during friendship establishment.
	 *  @param lpn_addr Low Power Node address.
	 *  @param recv_delay Receive Delay in units of 1 millisecond.
	 *  @param polltimeout PollTimeout in units of 1 millisecond.
	 */
	void (*established)(uint16_t net_idx, uint16_t lpn_addr,
			    uint8_t recv_delay, uint32_t polltimeout);

	/** @brief Friendship terminated.
	 *
	 *  This callback notifies the application that friendship has
	 *  been terminated.
	 *
	 *  @param net_idx  NetKeyIndex used during friendship establishment.
	 *  @param lpn_addr Low Power Node address.
	 */
	void (*terminated)(uint16_t net_idx, uint16_t lpn_addr);

	/** @brief Friend Poll Request.
	 *
	 *  This callback notifies the application that the low power node has
	 *  polled the friend node.
	 *
	 *  This callback will be called before @ref bt_mesh_friend_cb::established
	 *  when attempting to establish a friendship.
	 *
	 *  @param net_idx  NetKeyIndex used during friendship establishment.
	 *  @param lpn_addr LPN address.
	 */
	void (*polled)(uint16_t net_idx, uint16_t lpn_addr);
};

/**
 *  @brief Register a callback structure for Friendship events.
 *
 *  Registers a callback structure that will be called whenever Friendship
 *  gets established or terminated.
 *
 *  @param _name Name of callback structure.
 */
#define BT_MESH_FRIEND_CB_DEFINE(_name)                                  \
	static const STRUCT_SECTION_ITERABLE(bt_mesh_friend_cb,          \
					     _CONCAT(bt_mesh_friend_cb_, \
						     _name))
#if defined(CONFIG_BT_TESTING)
struct bt_mesh_snb {
	/** Flags */
	uint8_t flags;

	/** Network ID */
	uint64_t net_id;

	/** IV Index */
	uint32_t iv_idx;

	/** Authentication Value */
	uint64_t auth_val;
};

struct bt_mesh_prb {
	/** Random */
	uint8_t random[13];

	/** Flags */
	uint8_t flags;

	/** IV Index */
	uint32_t iv_idx;

	/** Authentication tag */
	uint64_t auth_tag;
};

/** Beacon callback functions. */
struct bt_mesh_beacon_cb {
	/** @brief Secure Network Beacon received.
	 *
	 *  This callback notifies the application that Secure Network Beacon
	 *  was received.
	 *
	 *  @param snb  Structure describing received Secure Network Beacon
	 */
	void (*snb_received)(const struct bt_mesh_snb *snb);

	/** @brief Private Beacon received.
	 *
	 *  This callback notifies the application that Private Beacon
	 *  was received and successfully decrypted.
	 *
	 *  @param prb  Structure describing received Private Beacon
	 */
	void (*priv_received)(const struct bt_mesh_prb *prb);
};

/**
 *  @brief Register a callback structure for beacon events.
 *
 *  Registers a callback structure that will be called whenever beacon advertisement
 *  is received.
 *
 *  @param _name Name of callback structure.
 */
#define BT_MESH_BEACON_CB_DEFINE(_name)                                  \
	static const STRUCT_SECTION_ITERABLE(bt_mesh_beacon_cb,          \
					     _CONCAT(bt_mesh_beacon_cb_, \
						     _name))
#endif

/** @brief Terminate Friendship.
 *
 *  Terminated Friendship for given LPN.
 *
 *  @param lpn_addr Low Power Node address.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_mesh_friend_terminate(uint16_t lpn_addr);

/** @brief Store pending RPL entry(ies) in the persistent storage.
 *
 * This API allows the user to store pending RPL entry(ies) in the persistent
 * storage without waiting for the timeout.
 *
 * @note When flash is used as the persistent storage, calling this API too
 *       frequently may wear it out.
 *
 * @param addr Address of the node which RPL entry needs to be stored or
 * @ref BT_MESH_ADDR_ALL_NODES to store all pending RPL entries.
 */
void bt_mesh_rpl_pending_store(uint16_t addr);

/** @brief Iterate stored Label UUIDs.
 *
 * When @c addr is @ref BT_MESH_ADDR_UNASSIGNED, this function iterates over all available addresses
 * starting with @c uuid. In this case, use @c retaddr to get virtual address representation of
 * the returned Label UUID. When @c addr is a virtual address, this function returns next Label
 * UUID corresponding to the @c addr. When @c uuid is NULL, this function returns the first
 * available UUID. If @c uuid is previously returned uuid, this function returns following uuid.
 *
 * @param addr    Virtual address to search for, or @ref BT_MESH_ADDR_UNASSIGNED.
 * @param uuid    Pointer to the previously returned Label UUID or NULL.
 * @param retaddr Pointer to a memory where virtual address representation of the returning UUID is
 *                to be stored to.
 *
 * @return Pointer to Label UUID, or NULL if no more entries found.
 */
const uint8_t *bt_mesh_va_uuid_get(uint16_t addr, const uint8_t *uuid, uint16_t *retaddr);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_MAIN_H_ */
