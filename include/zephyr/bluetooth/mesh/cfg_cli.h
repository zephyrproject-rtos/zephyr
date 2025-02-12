/** @file
 *  @brief Configuration Client Model APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_CFG_CLI_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_CFG_CLI_H_

/**
 * @brief Configuration Client Model
 * @defgroup bt_mesh_cfg_cli Configuration Client Model
 * @ingroup bt_mesh
 * @{
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_cfg_cli;
struct bt_mesh_cfg_cli_hb_pub;
struct bt_mesh_cfg_cli_hb_sub;
struct bt_mesh_cfg_cli_mod_pub;

/** Mesh Configuration Client Status messages callback */
struct bt_mesh_cfg_cli_cb {

	/** @brief Optional callback for Composition data messages.
	 *
	 *  Handles received Composition data messages from a server.
	 *
	 *  @note For decoding @c buf, please refer to
	 *        @ref bt_mesh_comp_p0_get and
	 *        @ref bt_mesh_comp_p1_elem_pull.
	 *
	 *  @param cli       Client that received the status message.
	 *  @param addr      Address of the sender.
	 *  @param page      Composition data page.
	 *  @param buf       Composition data buffer.
	 */
	void (*comp_data)(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t page,
			  struct net_buf_simple *buf);

	/** @brief Optional callback for Model Pub status messages.
	 *
	 *  Handles received Model Pub status messages from a server.
	 *
	 *  @param cli       Client that received the status message.
	 *  @param addr      Address of the sender.
	 *  @param status    Status code for the message.
	 *  @param elem_addr Address of the element.
	 *  @param mod_id    Model ID.
	 *  @param cid       Company ID.
	 *  @param pub       Publication configuration parameters.
	 */
	void (*mod_pub_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
			       uint16_t elem_addr, uint16_t mod_id, uint16_t cid,
			       struct bt_mesh_cfg_cli_mod_pub *pub);

	/** @brief Optional callback for Model Sub Status messages.
	 *
	 *  Handles received Model Sub Status messages from a server.
	 *
	 *  @param cli         Client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param status      Status Code for requesting message.
	 *  @param elem_addr   The unicast address of the element.
	 *  @param sub_addr    The sub address.
	 *  @param mod_id      The model ID within the element.
	 */
	void (*mod_sub_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr,
			     uint8_t status, uint16_t elem_addr,
			     uint16_t sub_addr, uint32_t mod_id);

	/** @brief Optional callback for Model Sub list messages.
	 *
	 *  Handles received Model Sub list messages from a server.
	 *
	 *  @note The @c buf parameter should be decoded using
	 *        @ref net_buf_simple_pull_le16 in iteration, as long
	 *        as @c buf->len is greater than or equal to  2.
	 *
	 *  @param cli       Client that received the status message.
	 *  @param addr      Address of the sender.
	 *  @param status    Status code for the message.
	 *  @param elem_addr Address of the element.
	 *  @param mod_id    Model ID.
	 *  @param cid       Company ID.
	 *  @param buf       Message buffer containing subscription addresses.
	 */
	void (*mod_sub_list)(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
			     uint16_t elem_addr, uint16_t mod_id, uint16_t cid,
			     struct net_buf_simple *buf);

	/** @brief Optional callback for Node Reset Status messages.
	 *
	 *  Handles received Node Reset Status messages from a server.
	 *
	 *  @param cli         Client that received the status message.
	 *  @param addr        Address of the sender.
	 */
	void (*node_reset_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr);

	/** @brief Optional callback for Beacon Status messages.
	 *
	 *  Handles received Beacon Status messages from a server.
	 *
	 *  @param cli         Client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param status      Status Code for requesting message.
	 */
	void (*beacon_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr,
				uint8_t status);

	/** @brief Optional callback for Default TTL Status messages.
	 *
	 *  Handles received Default TTL Status messages from a server.
	 *
	 *  @param cli         Client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param status      Status Code for requesting message.
	 */
	void (*ttl_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr,
			   uint8_t status);

	/** @brief Optional callback for Friend Status messages.
	 *
	 *  Handles received Friend Status messages from a server.
	 *
	 *  @param cli         Client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param status      Status Code for requesting message.
	 */
	void (*friend_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr,
			      uint8_t status);

	/** @brief Optional callback for GATT Proxy Status messages.
	 *
	 *  Handles received GATT Proxy Status messages from a server.
	 *
	 *  @param cli         Client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param status      Status Code for requesting message.
	 */
	void (*gatt_proxy_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr,
				  uint8_t status);

	/** @brief Optional callback for Network Transmit Status messages.
	 *
	 *  Handles received Network Transmit Status messages from a server.
	 *
	 *  @param cli         Client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param status      Status Code for requesting message.
	 */
	void (*network_transmit_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr,
					uint8_t status);

	/** @brief Optional callback for Relay Status messages.
	 *
	 *  Handles received Relay Status messages from a server.
	 *
	 *  @param cli         Client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param status      Status Code for requesting message.
	 *  @param transmit    The relay retransmit count and interval steps.
	 */
	void (*relay_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr,
			     uint8_t status, uint8_t transmit);

	/** @brief Optional callback for NetKey Status messages.
	 *
	 *  Handles received NetKey Status messages from a server.
	 *
	 *  @param cli         Client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param status      Status Code for requesting message.
	 *  @param net_idx     The index of the NetKey.
	 */
	void (*net_key_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr,
			       uint8_t status, uint16_t net_idx);

	/** @brief Optional callback for Netkey list messages.
	 *
	 *  Handles received Netkey list messages from a server.
	 *
	 *  @note The @c buf parameter should be decoded using the
	 *        @ref bt_mesh_key_idx_unpack_list helper function.
	 *
	 *  @param cli	Client that received the status message.
	 *  @param addr	Address of the sender.
	 *  @param buf	Message buffer containing key indexes.
	 */
	void (*net_key_list)(struct bt_mesh_cfg_cli *cli, uint16_t addr,
			     struct net_buf_simple *buf);

	/** @brief Optional callback for AppKey Status messages.
	 *
	 *  Handles received AppKey Status messages from a server.
	 *
	 *  @param cli         Client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param status      Status Code for requesting message.
	 *  @param net_idx     The index of the NetKey.
	 *  @param app_idx     The index of the AppKey.
	 */
	void (*app_key_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr,
			       uint8_t status, uint16_t net_idx,
			       uint16_t app_idx);

	/** @brief Optional callback for Appkey list messages.
	 *
	 *  Handles received Appkey list messages from a server.
	 *
	 *  @note The @c buf parameter should be decoded using the
	 *        @ref bt_mesh_key_idx_unpack_list helper function.
	 *
	 *  @param cli     Client that received the status message.
	 *  @param addr    Address of the sender.
	 *  @param status  Status code for the message.
	 *  @param net_idx The index of the NetKey.
	 *  @param buf     Message buffer containing key indexes.
	 */
	void (*app_key_list)(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
			     uint16_t net_idx, struct net_buf_simple *buf);

	/** @brief Optional callback for Model App Status messages.
	 *
	 *  Handles received Model App Status messages from a server.
	 *
	 *  @param cli         Client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param status      Status Code for requesting message.
	 *  @param elem_addr   The unicast address of the element.
	 *  @param app_idx     The sub address.
	 *  @param mod_id      The model ID within the element.
	 */
	void (*mod_app_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr,
			       uint8_t status, uint16_t elem_addr,
			       uint16_t app_idx, uint32_t mod_id);

	/** @brief Optional callback for Model App list messages.
	 *
	 *  Handles received Model App list messages from a server.
	 *
	 *  @note The @c buf parameter should be decoded using the
	 *        @ref bt_mesh_key_idx_unpack_list helper function.
	 *
	 *  @param cli       Client that received the status message.
	 *  @param addr      Address of the sender.
	 *  @param status    Status code for the message.
	 *  @param elem_addr Address of the element.
	 *  @param mod_id    Model ID.
	 *  @param cid       Company ID.
	 *  @param buf       Message buffer containing key indexes.
	 */
	void (*mod_app_list)(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
			     uint16_t elem_addr, uint16_t mod_id, uint16_t cid,
			     struct net_buf_simple *buf);

	/** @brief Optional callback for Node Identity Status messages.
	 *
	 *  Handles received Node Identity Status messages from a server.
	 *
	 *  @param cli         Client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param status      Status Code for requesting message.
	 *  @param net_idx     The index of the NetKey.
	 *  @param identity    The node identity state.
	 */
	void (*node_identity_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr,
				     uint8_t status, uint16_t net_idx,
				     uint8_t identity);

	/** @brief Optional callback for LPN PollTimeout Status messages.
	 *
	 *  Handles received LPN PollTimeout Status messages from a server.
	 *
	 *  @param cli         Client that received the status message.
	 *  @param addr        Address of the sender.
	 *  @param elem_addr   The unicast address of the LPN.
	 *  @param timeout     Current value of PollTimeout timer of the LPN.
	 */
	void (*lpn_timeout_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr,
				   uint16_t elem_addr, uint32_t timeout);

	/** @brief Optional callback for Key Refresh Phase status messages.
	 *
	 *  Handles received Key Refresh Phase status messages from a server.
	 *
	 *  @param cli     Client that received the status message.
	 *  @param addr    Address of the sender.
	 *  @param status  Status code for the message.
	 *  @param net_idx The index of the NetKey.
	 *  @param phase   Phase of the KRP.
	 */
	void (*krp_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
			   uint16_t net_idx, uint8_t phase);

	/** @brief Optional callback for Heartbeat pub status messages.
	 *
	 *  Handles received Heartbeat pub status messages from a server.
	 *
	 *  @param cli    Client that received the status message.
	 *  @param addr   Address of the sender.
	 *  @param status Status code for the message.
	 *  @param pub    HB publication configuration parameters.
	 */
	void (*hb_pub_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
			      struct bt_mesh_cfg_cli_hb_pub *pub);

	/** @brief Optional callback for Heartbeat Sub status messages.
	 *
	 *  Handles received Heartbeat Sub status messages from a server.
	 *
	 *  @param cli    Client that received the status message.
	 *  @param addr   Address of the sender.
	 *  @param status Status code for the message.
	 *  @param sub    HB subscription configuration parameters.
	 */
	void (*hb_sub_status)(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
			      struct bt_mesh_cfg_cli_hb_sub *sub);
};

/** Mesh Configuration Client Model Context */
struct bt_mesh_cfg_cli {
	/** Composition data model entry pointer. */
	const struct bt_mesh_model *model;

	/** Optional callback for Mesh Configuration Client Status messages. */
	const struct bt_mesh_cfg_cli_cb *cb;

	/* Internal parameters for tracking message responses. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
};

/**
 *  @brief Generic Configuration Client model composition data entry.
 *
 *  @param cli_data Pointer to a @ref bt_mesh_cfg_cli instance.
 */
#define BT_MESH_MODEL_CFG_CLI(cli_data)			\
	BT_MESH_MODEL_CNT_CB(BT_MESH_MODEL_ID_CFG_CLI,	\
			     bt_mesh_cfg_cli_op, NULL,	\
			     cli_data, 1, 0, &bt_mesh_cfg_cli_cb)

/**
 *
 *  @brief Helper macro to encode model publication period in units of 100ms
 *
 *  @param steps Number of 100ms steps.
 *
 *  @return Encoded value that can be assigned to bt_mesh_cfg_cli_mod_pub.period
 */
#define BT_MESH_PUB_PERIOD_100MS(steps)  ((steps) & BIT_MASK(6))

/**
 *  @brief Helper macro to encode model publication period in units of 1 second
 *
 *  @param steps Number of 1 second steps.
 *
 *  @return Encoded value that can be assigned to bt_mesh_cfg_cli_mod_pub.period
 */
#define BT_MESH_PUB_PERIOD_SEC(steps)   (((steps) & BIT_MASK(6)) | (1 << 6))

/**
 *
 *  @brief Helper macro to encode model publication period in units of 10
 *  seconds
 *
 *  @param steps Number of 10 second steps.
 *
 *  @return Encoded value that can be assigned to bt_mesh_cfg_cli_mod_pub.period
 */
#define BT_MESH_PUB_PERIOD_10SEC(steps) (((steps) & BIT_MASK(6)) | (2 << 6))

/**
 *
 *  @brief Helper macro to encode model publication period in units of 10
 *  minutes
 *
 *  @param steps Number of 10 minute steps.
 *
 *  @return Encoded value that can be assigned to bt_mesh_cfg_cli_mod_pub.period
 */
#define BT_MESH_PUB_PERIOD_10MIN(steps) (((steps) & BIT_MASK(6)) | (3 << 6))

/** Model publication configuration parameters. */
struct bt_mesh_cfg_cli_mod_pub {
	/** Publication destination address. */
	uint16_t addr;
	/** Virtual address UUID, or NULL if this is not a virtual address. */
	const uint8_t *uuid;
	/** Application index to publish with. */
	uint16_t  app_idx;
	/** Friendship credential flag. */
	bool   cred_flag;
	/** Time To Live to publish with. */
	uint8_t   ttl;
	/**
	 * Encoded publish period.
	 * @see BT_MESH_PUB_PERIOD_100MS, BT_MESH_PUB_PERIOD_SEC,
	 * BT_MESH_PUB_PERIOD_10SEC,
	 * BT_MESH_PUB_PERIOD_10MIN
	 */
	uint8_t   period;
	/**
	 * Encoded transmit parameters.
	 * @see BT_MESH_TRANSMIT
	 */
	uint8_t   transmit;
};

/** Heartbeat subscription configuration parameters. */
struct bt_mesh_cfg_cli_hb_sub {
	/** Source address to receive Heartbeat messages from. */
	uint16_t src;
	/** Destination address to receive Heartbeat messages on. */
	uint16_t dst;
	/**
	 * Logarithmic subscription period to keep listening for.
	 * The decoded subscription period is (1 << (period - 1)) seconds, or 0
	 * seconds if period is 0.
	 */
	uint8_t  period;
	/**
	 * Logarithmic Heartbeat subscription receive count.
	 * The decoded Heartbeat count is (1 << (count - 1)) if count is
	 * between 1 and 0xfe, 0 if count is 0 and 0xffff if count is 0xff.
	 *
	 * Ignored in Heartbeat subscription set.
	 */
	uint8_t  count;
	/**
	 * Minimum hops in received messages, ie the shortest registered path
	 * from the publishing node to the subscribing node. A Heartbeat
	 * received from an immediate neighbor has hop count = 1.
	 *
	 * Ignored in Heartbeat subscription set.
	 */
	uint8_t  min;
	/**
	 * Maximum hops in received messages, ie the longest registered path
	 * from the publishing node to the subscribing node. A Heartbeat
	 * received from an immediate neighbor has hop count = 1.
	 *
	 * Ignored in Heartbeat subscription set.
	 */
	uint8_t  max;
};

/** Heartbeat publication configuration parameters. */
struct bt_mesh_cfg_cli_hb_pub {
	/** Heartbeat destination address. */
	uint16_t dst;
	/**
	 * Logarithmic Heartbeat count. Decoded as (1 << (count - 1)) if count
	 * is between 1 and 0x11, 0 if count is 0, or "indefinitely" if count is
	 * 0xff.
	 *
	 * When used in Heartbeat publication set, this parameter denotes the
	 * number of Heartbeat messages to send.
	 *
	 * When returned from Heartbeat publication get, this parameter denotes
	 * the number of Heartbeat messages remaining to be sent.
	 */
	uint8_t  count;
	/**
	 * Logarithmic Heartbeat publication transmit interval in seconds.
	 * Decoded as (1 << (period - 1)) if period is between 1 and 0x11.
	 * If period is 0, Heartbeat publication is disabled.
	 */
	uint8_t  period;
	/** Publication message Time To Live value. */
	uint8_t  ttl;
	/**
	 * Bitmap of features that trigger Heartbeat publications.
	 * Legal values are @ref BT_MESH_FEAT_RELAY,
	 * @ref BT_MESH_FEAT_PROXY, @ref BT_MESH_FEAT_FRIEND and
	 * @ref BT_MESH_FEAT_LOW_POWER
	 */
	uint16_t feat;
	/** Network index to publish with. */
	uint16_t net_idx;
};

/** @brief Reset the target node and remove it from the network.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param status  Status response parameter
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_node_reset(uint16_t net_idx, uint16_t addr, bool *status);

/** @brief Get the target node's composition data.
 *
 *  If the other device does not have the given composition data page, it will
 *  return the largest page number it supports that is less than the requested
 *  page index. The actual page the device responds with is returned in @c rsp.
 *
 *  This method can be used asynchronously by setting @p rsp and @p comp
 *  as NULL. This way the method will not wait for response and will return
 *  immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param page    Composition data page, or 0xff to request the first available
 *                 page.
 *  @param rsp     Return parameter for the returned page number, or NULL.
 *  @param comp    Composition data buffer to fill.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_comp_data_get(uint16_t net_idx, uint16_t addr, uint8_t page, uint8_t *rsp,
				  struct net_buf_simple *comp);

/** @brief Get the target node's network beacon state.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param status  Status response parameter, returns one of
 *                 @ref BT_MESH_BEACON_DISABLED or @ref BT_MESH_BEACON_ENABLED
 *                 on success.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_beacon_get(uint16_t net_idx, uint16_t addr, uint8_t *status);

/** @brief             Get the target node's network key refresh phase state.
 *
 *  This method can be used asynchronously by setting @p status and @p phase
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node address.
 *  @param key_net_idx Network key index.
 *  @param status      Status response parameter.
 *  @param phase       Pointer to the Key Refresh variable to fill.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_krp_get(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx, uint8_t *status,
			    uint8_t *phase);

/** @brief             Set the target node's network key refresh phase parameters.
 *
 *  This method can be used asynchronously by setting @p status and @p phase
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node address.
 *  @param key_net_idx Network key index.
 *  @param transition  Transition parameter.
 *  @param status      Status response parameter.
 *  @param phase       Pointer to the new Key Refresh phase. Will return the actual
 *                     Key Refresh phase after updating.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_krp_set(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
			    uint8_t transition, uint8_t *status, uint8_t *phase);

/** @brief Set the target node's network beacon state.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param val     New network beacon state, should be one of
 *                 @ref BT_MESH_BEACON_DISABLED or @ref BT_MESH_BEACON_ENABLED.
 *  @param status  Status response parameter. Returns one of
 *                 @ref BT_MESH_BEACON_DISABLED or @ref BT_MESH_BEACON_ENABLED
 *                 on success.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_beacon_set(uint16_t net_idx, uint16_t addr, uint8_t val, uint8_t *status);

/** @brief Get the target node's Time To Live value.
 *
 *  This method can be used asynchronously by setting @p ttl
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param ttl     TTL response buffer.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_ttl_get(uint16_t net_idx, uint16_t addr, uint8_t *ttl);

/** @brief Set the target node's Time To Live value.
 *
 *  This method can be used asynchronously by setting @p ttl
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param val     New Time To Live value.
 *  @param ttl     TTL response buffer.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_ttl_set(uint16_t net_idx, uint16_t addr, uint8_t val, uint8_t *ttl);

/** @brief Get the target node's Friend feature status.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param status  Status response parameter. Returns one of
 *                 @ref BT_MESH_FRIEND_DISABLED, @ref BT_MESH_FRIEND_ENABLED or
 *                 @ref BT_MESH_FRIEND_NOT_SUPPORTED on success.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_friend_get(uint16_t net_idx, uint16_t addr, uint8_t *status);

/** @brief Set the target node's Friend feature state.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param val     New Friend feature state. Should be one of
 *                 @ref BT_MESH_FRIEND_DISABLED or
 *                 @ref BT_MESH_FRIEND_ENABLED.
 *  @param status  Status response parameter. Returns one of
 *                 @ref BT_MESH_FRIEND_DISABLED, @ref BT_MESH_FRIEND_ENABLED or
 *                 @ref BT_MESH_FRIEND_NOT_SUPPORTED on success.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_friend_set(uint16_t net_idx, uint16_t addr, uint8_t val, uint8_t *status);

/** @brief Get the target node's Proxy feature state.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param status  Status response parameter. Returns one of
 *                 @ref BT_MESH_GATT_PROXY_DISABLED,
 *                 @ref BT_MESH_GATT_PROXY_ENABLED or
 *                 @ref BT_MESH_GATT_PROXY_NOT_SUPPORTED on success.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_gatt_proxy_get(uint16_t net_idx, uint16_t addr, uint8_t *status);

/** @brief Set the target node's Proxy feature state.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param val     New Proxy feature state. Must be one of
 *                 @ref BT_MESH_GATT_PROXY_DISABLED or
 *                 @ref BT_MESH_GATT_PROXY_ENABLED.
 *  @param status  Status response parameter. Returns one of
 *                 @ref BT_MESH_GATT_PROXY_DISABLED,
 *                 @ref BT_MESH_GATT_PROXY_ENABLED or
 *                 @ref BT_MESH_GATT_PROXY_NOT_SUPPORTED on success.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_gatt_proxy_set(uint16_t net_idx, uint16_t addr, uint8_t val, uint8_t *status);

/** @brief Get the target node's network_transmit state.
 *
 *  This method can be used asynchronously by setting @p transmit
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx  Network index to encrypt with.
 *  @param addr     Target node address.
 *  @param transmit Network transmit response parameter. Returns the encoded
 *                  network transmission parameters on success. Decoded with
 *                  @ref BT_MESH_TRANSMIT_COUNT and @ref BT_MESH_TRANSMIT_INT.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_net_transmit_get(uint16_t net_idx, uint16_t addr, uint8_t *transmit);

/** @brief Set the target node's network transmit parameters.
 *
 *  This method can be used asynchronously by setting @p transmit
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx    Network index to encrypt with.
 *  @param addr       Target node address.
 *  @param val        New encoded network transmit parameters.
 *                    @see BT_MESH_TRANSMIT.
 *  @param transmit   Network transmit response parameter. Returns the encoded
 *                    network transmission parameters on success. Decoded with
 *                    @ref BT_MESH_TRANSMIT_COUNT and @ref BT_MESH_TRANSMIT_INT.
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_net_transmit_set(uint16_t net_idx, uint16_t addr, uint8_t val,
				     uint8_t *transmit);

/** @brief Get the target node's Relay feature state.
 *
 *  This method can be used asynchronously by setting @p status and @p transmit
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx  Network index to encrypt with.
 *  @param addr     Target node address.
 *  @param status   Status response parameter. Returns one of
 *                  @ref BT_MESH_RELAY_DISABLED, @ref BT_MESH_RELAY_ENABLED or
 *                  @ref BT_MESH_RELAY_NOT_SUPPORTED on success.
 *  @param transmit Transmit response parameter. Returns the encoded relay
 *                  transmission parameters on success. Decoded with
 *                  @ref BT_MESH_TRANSMIT_COUNT and @ref BT_MESH_TRANSMIT_INT.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_relay_get(uint16_t net_idx, uint16_t addr, uint8_t *status, uint8_t *transmit);

/** @brief Set the target node's Relay parameters.
 *
 *  This method can be used asynchronously by setting @p status
 *  and @p transmit as NULL. This way the method will not wait for
 *  response and will return immediately after sending the command.
 *
 *  @param net_idx      Network index to encrypt with.
 *  @param addr         Target node address.
 *  @param new_relay    New relay state. Must be one of
 *                      @ref BT_MESH_RELAY_DISABLED or
 *                      @ref BT_MESH_RELAY_ENABLED.
 *  @param new_transmit New encoded relay transmit parameters.
 *                      @see BT_MESH_TRANSMIT.
 *  @param status       Status response parameter. Returns one of
 *                      @ref BT_MESH_RELAY_DISABLED, @ref BT_MESH_RELAY_ENABLED
 *                      or @ref BT_MESH_RELAY_NOT_SUPPORTED on success.
 *  @param transmit     Transmit response parameter. Returns the encoded relay
 *                      transmission parameters on success. Decoded with
 *                      @ref BT_MESH_TRANSMIT_COUNT and
 *                      @ref BT_MESH_TRANSMIT_INT.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_relay_set(uint16_t net_idx, uint16_t addr, uint8_t new_relay,
			      uint8_t new_transmit, uint8_t *status, uint8_t *transmit);

/** @brief Add a network key to the target node.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node address.
 *  @param key_net_idx Network key index.
 *  @param net_key     Network key.
 *  @param status      Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_net_key_add(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
				const uint8_t net_key[16], uint8_t *status);

/** @brief Get a list of the target node's network key indexes.
 *
 *  This method can be used asynchronously by setting @p keys
 *  or @p key_cnt as NULL. This way the method will not wait
 *  for response and will return immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param keys    Net key index list response parameter. Will be filled with
 *                 all the returned network key indexes it can fill.
 *  @param key_cnt Net key index list length. Should be set to the
 *                 capacity of the @c keys list when calling. Will return the
 *                 number of returned network key indexes upon success.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_net_key_get(uint16_t net_idx, uint16_t addr, uint16_t *keys, size_t *key_cnt);

/** @brief Delete a network key from the target node.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node address.
 *  @param key_net_idx Network key index.
 *  @param status      Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_net_key_del(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
				uint8_t *status);

/** @brief Add an application key to the target node.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node address.
 *  @param key_net_idx Network key index the application key belongs to.
 *  @param key_app_idx Application key index.
 *  @param app_key     Application key.
 *  @param status      Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_app_key_add(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
				uint16_t key_app_idx, const uint8_t app_key[16], uint8_t *status);

/** @brief Get a list of the target node's application key indexes for a
 *         specific network key.
 *
 *  This method can be used asynchronously by setting @p status and
 *  ( @p keys or @p key_cnt ) as NULL. This way the method will not wait
 *  for response and will return immediately after sending the command.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node address.
 *  @param key_net_idx Network key index to request the app key indexes of.
 *  @param status      Status response parameter.
 *  @param keys        App key index list response parameter. Will be filled
 *                     with all the returned application key indexes it can
 *                     fill.
 *  @param key_cnt     App key index list length. Should be set to the
 *                     capacity of the @c keys list when calling. Will return
 *                     the number of returned application key indexes upon
 *                     success.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_app_key_get(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
				uint8_t *status, uint16_t *keys, size_t *key_cnt);

/** @brief Delete an application key from the target node.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node address.
 *  @param key_net_idx Network key index the application key belongs to.
 *  @param key_app_idx Application key index.
 *  @param status      Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_app_key_del(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
				uint16_t key_app_idx, uint8_t *status);

/** @brief Bind an application to a SIG model on the target node.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node address.
 *  @param elem_addr   Element address the model is in.
 *  @param mod_app_idx Application index to bind.
 *  @param mod_id      Model ID.
 *  @param status      Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_app_bind(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				 uint16_t mod_app_idx, uint16_t mod_id, uint8_t *status);

/** @brief Unbind an application from a SIG model on the target node.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node address.
 *  @param elem_addr   Element address the model is in.
 *  @param mod_app_idx Application index to unbind.
 *  @param mod_id      Model ID.
 *  @param status      Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_app_unbind(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				   uint16_t mod_app_idx, uint16_t mod_id, uint8_t *status);

/** @brief Bind an application to a vendor model on the target node.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node address.
 *  @param elem_addr   Element address the model is in.
 *  @param mod_app_idx Application index to bind.
 *  @param mod_id      Model ID.
 *  @param cid         Company ID of the model.
 *  @param status      Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_app_bind_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				     uint16_t mod_app_idx, uint16_t mod_id, uint16_t cid,
				     uint8_t *status);

/** @brief Unbind an application from a vendor model on the target node.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response and will
 *  return immediately after sending the command.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node address.
 *  @param elem_addr   Element address the model is in.
 *  @param mod_app_idx Application index to unbind.
 *  @param mod_id      Model ID.
 *  @param cid         Company ID of the model.
 *  @param status      Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_app_unbind_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				       uint16_t mod_app_idx, uint16_t mod_id, uint16_t cid,
				       uint8_t *status);

/** @brief Get a list of all applications bound to a SIG model on the target
 *         node.
 *
 *  This method can be used asynchronously by setting @p status
 *  and ( @p apps or @p app_cnt ) as NULL. This way the method will
 *  not wait for response and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param mod_id    Model ID.
 *  @param status    Status response parameter.
 *  @param apps      App index list response parameter. Will be filled with all
 *                   the returned application key indexes it can fill.
 *  @param app_cnt   App index list length. Should be set to the capacity of the
 *                   @c apps list when calling. Will return the number of
 *                   returned application key indexes upon success.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_app_get(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				uint16_t mod_id, uint8_t *status, uint16_t *apps, size_t *app_cnt);

/** @brief Get a list of all applications bound to a vendor model on the target
 *         node.
 *
 *  This method can be used asynchronously by setting @p status
 *  and ( @p apps or @p app_cnt ) as NULL. This way the method will
 *  not wait for response and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param mod_id    Model ID.
 *  @param cid       Company ID of the model.
 *  @param status    Status response parameter.
 *  @param apps      App index list response parameter. Will be filled with all
 *                   the returned application key indexes it can fill.
 *  @param app_cnt   App index list length. Should be set to the capacity of the
 *                   @c apps list when calling. Will return the number of
 *                   returned application key indexes upon success.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_app_get_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				    uint16_t mod_id, uint16_t cid, uint8_t *status, uint16_t *apps,
				    size_t *app_cnt);

/** @brief Get publish parameters for a SIG model on the target node.
 *
 *  This method can be used asynchronously by setting @p status and
 *  @p pub as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param mod_id    Model ID.
 *  @param pub       Publication parameter return buffer.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_pub_get(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				uint16_t mod_id, struct bt_mesh_cfg_cli_mod_pub *pub,
				uint8_t *status);

/** @brief Get publish parameters for a vendor model on the target node.
 *
 *  This method can be used asynchronously by setting @p status
 *  and @p pub as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param mod_id    Model ID.
 *  @param cid       Company ID of the model.
 *  @param pub       Publication parameter return buffer.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_pub_get_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				    uint16_t mod_id, uint16_t cid,
				    struct bt_mesh_cfg_cli_mod_pub *pub, uint8_t *status);

/** @brief Set publish parameters for a SIG model on the target node.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @p pub shall not be NULL.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param mod_id    Model ID.
 *  @param pub       Publication parameters.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_pub_set(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				uint16_t mod_id, struct bt_mesh_cfg_cli_mod_pub *pub,
				uint8_t *status);

/** @brief Set publish parameters for a vendor model on the target node.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @p pub shall not be NULL.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param mod_id    Model ID.
 *  @param cid       Company ID of the model.
 *  @param pub       Publication parameters.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_pub_set_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				    uint16_t mod_id, uint16_t cid,
				    struct bt_mesh_cfg_cli_mod_pub *pub, uint8_t *status);

/** @brief Add a group address to a SIG model's subscription list.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param sub_addr  Group address to add to the subscription list.
 *  @param mod_id    Model ID.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_add(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				uint16_t sub_addr, uint16_t mod_id, uint8_t *status);

/** @brief Add a group address to a vendor model's subscription list.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param sub_addr  Group address to add to the subscription list.
 *  @param mod_id    Model ID.
 *  @param cid       Company ID of the model.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_add_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				    uint16_t sub_addr, uint16_t mod_id, uint16_t cid,
				    uint8_t *status);

/** @brief Delete a group address in a SIG model's subscription list.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param sub_addr  Group address to add to the subscription list.
 *  @param mod_id    Model ID.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_del(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				uint16_t sub_addr, uint16_t mod_id, uint8_t *status);

/** @brief Delete a group address in a vendor model's subscription list.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param sub_addr  Group address to add to the subscription list.
 *  @param mod_id    Model ID.
 *  @param cid       Company ID of the model.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_del_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				    uint16_t sub_addr, uint16_t mod_id, uint16_t cid,
				    uint8_t *status);

/** @brief Overwrite all addresses in a SIG model's subscription list with a
 * group address.
 *
 * Deletes all subscriptions in the model's subscription list, and adds a
 * single group address instead.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param sub_addr  Group address to add to the subscription list.
 *  @param mod_id    Model ID.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_overwrite(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				      uint16_t sub_addr, uint16_t mod_id, uint8_t *status);

/** @brief Overwrite all addresses in a vendor model's subscription list with a
 * group address.
 *
 * Deletes all subscriptions in the model's subscription list, and adds a
 * single group address instead.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param sub_addr  Group address to add to the subscription list.
 *  @param mod_id    Model ID.
 *  @param cid       Company ID of the model.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_overwrite_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
					  uint16_t sub_addr, uint16_t mod_id, uint16_t cid,
					  uint8_t *status);

/** @brief Add a virtual address to a SIG model's subscription list.
 *
 *  This method can be used asynchronously by setting @p status
 *  and @p virt_addr as NULL. This way the method will not wait
 *  for response and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param label     Virtual address label to add to the subscription list.
 *  @param mod_id    Model ID.
 *  @param virt_addr Virtual address response parameter.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_va_add(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				   const uint8_t label[16], uint16_t mod_id, uint16_t *virt_addr,
				   uint8_t *status);

/** @brief Add a virtual address to a vendor model's subscription list.
 *
 *  This method can be used asynchronously by setting @p status
 *  and @p virt_addr as NULL. This way the method will not wait
 *  for response and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param label     Virtual address label to add to the subscription list.
 *  @param mod_id    Model ID.
 *  @param cid       Company ID of the model.
 *  @param virt_addr Virtual address response parameter.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_va_add_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				       const uint8_t label[16], uint16_t mod_id, uint16_t cid,
				       uint16_t *virt_addr, uint8_t *status);

/** @brief Delete a virtual address in a SIG model's subscription list.
 *
 *  This method can be used asynchronously by setting @p status
 *  and @p virt_addr as NULL. This way the method will not wait
 *  for response and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param label     Virtual address parameter to add to the subscription list.
 *  @param mod_id    Model ID.
 *  @param virt_addr Virtual address response parameter.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_va_del(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				   const uint8_t label[16], uint16_t mod_id, uint16_t *virt_addr,
				   uint8_t *status);

/** @brief Delete a virtual address in a vendor model's subscription list.
 *
 *  This method can be used asynchronously by setting @p status
 *  and @p virt_addr as NULL. This way the method will not wait
 *  for response and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param label     Virtual address label to add to the subscription list.
 *  @param mod_id    Model ID.
 *  @param cid       Company ID of the model.
 *  @param virt_addr Virtual address response parameter.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_va_del_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				       const uint8_t label[16], uint16_t mod_id, uint16_t cid,
				       uint16_t *virt_addr, uint8_t *status);

/** @brief Overwrite all addresses in a SIG model's subscription list with a
 *  virtual address.
 *
 *  Deletes all subscriptions in the model's subscription list, and adds a
 *  single group address instead.
 *
 *  This method can be used asynchronously by setting @p status
 *  and @p virt_addr as NULL. This way the method will not wait
 *  for response and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param label     Virtual address label to add to the subscription list.
 *  @param mod_id    Model ID.
 *  @param virt_addr Virtual address response parameter.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_va_overwrite(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
					 const uint8_t label[16], uint16_t mod_id,
					 uint16_t *virt_addr, uint8_t *status);

/** @brief Overwrite all addresses in a vendor model's subscription list with a
 *  virtual address.
 *
 *  Deletes all subscriptions in the model's subscription list, and adds a
 *  single group address instead.
 *
 *  This method can be used asynchronously by setting @p status
 *  and @p virt_addr as NULL. This way the method will not wait
 *  for response and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param label     Virtual address label to add to the subscription list.
 *  @param mod_id    Model ID.
 *  @param cid       Company ID of the model.
 *  @param virt_addr Virtual address response parameter.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_va_overwrite_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
					     const uint8_t label[16], uint16_t mod_id, uint16_t cid,
					     uint16_t *virt_addr, uint8_t *status);

/** @brief Get the subscription list of a SIG model on the target node.
 *
 *  This method can be used asynchronously by setting @p status and
 *  ( @p subs or @p sub_cnt ) as NULL. This way the method will
 *  not wait for response and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param mod_id    Model ID.
 *  @param status    Status response parameter.
 *  @param subs      Subscription list response parameter. Will be filled with
 *                   all the returned subscriptions it can fill.
 *  @param sub_cnt   Subscription list element count. Should be set to the
 *                   capacity of the @c subs list when calling. Will return the
 *                   number of returned subscriptions upon success.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_get(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				uint16_t mod_id, uint8_t *status, uint16_t *subs, size_t *sub_cnt);

/** @brief Get the subscription list of a vendor model on the target node.
 *
 *  This method can be used asynchronously by setting @p status and
 *  ( @p subs or @p sub_cnt ) as NULL. This way the method will
 *  not wait for response and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param mod_id    Model ID.
 *  @param cid       Company ID of the model.
 *  @param status    Status response parameter.
 *  @param subs      Subscription list response parameter. Will be filled with
 *                   all the returned subscriptions it can fill.
 *  @param sub_cnt   Subscription list element count. Should be set to the
 *                   capacity of the @c subs list when calling. Will return the
 *                   number of returned subscriptions upon success.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_get_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				    uint16_t mod_id, uint16_t cid, uint8_t *status, uint16_t *subs,
				    size_t *sub_cnt);

/** @brief Set the target node's Heartbeat subscription parameters.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @p sub shall not be null.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param sub     New Heartbeat subscription parameters.
 *  @param status  Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_hb_sub_set(uint16_t net_idx, uint16_t addr, struct bt_mesh_cfg_cli_hb_sub *sub,
			       uint8_t *status);

/** @brief Get the target node's Heartbeat subscription parameters.
 *
 *  This method can be used asynchronously by setting @p status
 *  and @p sub as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param sub     Heartbeat subscription parameter return buffer.
 *  @param status  Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_hb_sub_get(uint16_t net_idx, uint16_t addr, struct bt_mesh_cfg_cli_hb_sub *sub,
			       uint8_t *status);

/** @brief Set the target node's Heartbeat publication parameters.
 *
 *  @note The target node must already have received the specified network key.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @p pub shall not be NULL;
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param pub     New Heartbeat publication parameters.
 *  @param status  Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_hb_pub_set(uint16_t net_idx, uint16_t addr,
			       const struct bt_mesh_cfg_cli_hb_pub *pub, uint8_t *status);

/** @brief Get the target node's Heartbeat publication parameters.
 *
 *  This method can be used asynchronously by setting @p status
 *  and @p pub as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param pub     Heartbeat publication parameter return buffer.
 *  @param status  Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_hb_pub_get(uint16_t net_idx, uint16_t addr, struct bt_mesh_cfg_cli_hb_pub *pub,
			       uint8_t *status);

/** @brief Delete all group addresses in a SIG model's subscription list.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param mod_id    Model ID.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_del_all(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				    uint16_t mod_id, uint8_t *status);

/** @brief Delete all group addresses in a vendor model's subscription list.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @param net_idx   Network index to encrypt with.
 *  @param addr      Target node address.
 *  @param elem_addr Element address the model is in.
 *  @param mod_id    Model ID.
 *  @param cid       Company ID of the model.
 *  @param status    Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_mod_sub_del_all_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
					uint16_t mod_id, uint16_t cid, uint8_t *status);

/** @brief Update a network key to the target node.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node address.
 *  @param key_net_idx Network key index.
 *  @param net_key     Network key.
 *  @param status      Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_net_key_update(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
				   const uint8_t net_key[16], uint8_t *status);

/** @brief Update an application key to the target node.
 *
 *  This method can be used asynchronously by setting @p status
 *  as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @param net_idx     Network index to encrypt with.
 *  @param addr        Target node address.
 *  @param key_net_idx Network key index the application key belongs to.
 *  @param key_app_idx Application key index.
 *  @param app_key     Application key.
 *  @param status      Status response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_app_key_update(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
				   uint16_t key_app_idx, const uint8_t app_key[16],
				   uint8_t *status);

/** @brief Set the Node Identity parameters.
 *
 *  This method can be used asynchronously by setting @p status
 *  and @p identity as NULL. This way the method will not wait
 *  for response and will return immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param new_identity  New identity state. Must be one of
 *                      @ref BT_MESH_NODE_IDENTITY_STOPPED or
 *                      @ref BT_MESH_NODE_IDENTITY_RUNNING
 *  @param key_net_idx Network key index the application key belongs to.
 *  @param status  Status response parameter.
 *  @param identity Identity response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_node_identity_set(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
				      uint8_t new_identity, uint8_t *status, uint8_t *identity);

/** @brief Get the Node Identity parameters.
 *
 *  This method can be used asynchronously by setting @p status
 *  and @p identity as NULL. This way the method will not wait
 *  for response and will return immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param key_net_idx Network key index the application key belongs to.
 *  @param status  Status response parameter.
 *  @param identity Identity response parameter. Must be one of
 *                      @ref BT_MESH_NODE_IDENTITY_STOPPED or
 *                      @ref BT_MESH_NODE_IDENTITY_RUNNING
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_node_identity_get(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
				      uint8_t *status, uint8_t *identity);

/** @brief Get the Low Power Node Polltimeout parameters.
 *
 *  This method can be used asynchronously by setting @p polltimeout
 *  as NULL. This way the method will not wait for response
 *  and will return immediately after sending the command.
 *
 *  @param net_idx Network index to encrypt with.
 *  @param addr    Target node address.
 *  @param unicast_addr LPN unicast address.
 *  @param polltimeout Poll timeout response parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_cfg_cli_lpn_timeout_get(uint16_t net_idx, uint16_t addr, uint16_t unicast_addr,
				    int32_t *polltimeout);

/** @brief Get the current transmission timeout value.
 *
 *  @return The configured transmission timeout in milliseconds.
 */
int32_t bt_mesh_cfg_cli_timeout_get(void);

/** @brief Set the transmission timeout value.
 *
 *  @param timeout The new transmission timeout.
 */
void bt_mesh_cfg_cli_timeout_set(int32_t timeout);

/** Parsed Composition data page 0 representation.
 *
 *  Should be pulled from the return buffer passed to
 *  @ref bt_mesh_cfg_cli_comp_data_get using
 *  @ref bt_mesh_comp_p0_get.
 */
struct bt_mesh_comp_p0 {
	/** Company ID */
	uint16_t cid;
	/** Product ID */
	uint16_t pid;
	/** Version ID */
	uint16_t vid;
	/** Replay protection list size */
	uint16_t crpl;
	/** Supported features, see @ref BT_MESH_FEAT_SUPPORTED. */
	uint16_t feat;

	struct net_buf_simple *_buf;
};

/** Composition data page 0 element representation */
struct bt_mesh_comp_p0_elem {
	/** Element location */
	uint16_t loc;
	/** The number of SIG models in this element */
	size_t nsig;
	/** The number of vendor models in this element */
	size_t nvnd;

	uint8_t *_buf;
};

/** @brief Create a composition data page 0 representation from a buffer.
 *
 *  The composition data page object will take ownership over the buffer, which
 *  should not be manipulated directly after this call.
 *
 *  This function can be used in combination with @ref bt_mesh_cfg_cli_comp_data_get
 *  to read out composition data page 0 from other devices:
 *
 *  @code
 *  NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_RX_SDU_MAX);
 *  struct bt_mesh_comp_p0 comp;
 *
 *  err = bt_mesh_cfg_cli_comp_data_get(net_idx, addr, 0, &page, &buf);
 *  if (!err) {
 *          bt_mesh_comp_p0_get(&comp, &buf);
 *  }
 *  @endcode
 *
 *  @param buf  Network buffer containing composition data.
 *  @param comp Composition data structure to fill.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_comp_p0_get(struct bt_mesh_comp_p0 *comp,
			struct net_buf_simple *buf);

/** @brief Pull a composition data page 0 element from a composition data page 0
 *         instance.
 *
 *  Each call to this function will pull out a new element from the composition
 *  data page, until all elements have been pulled.
 *
 *  @param comp Composition data page
 *  @param elem Element to fill.
 *
 *  @return A pointer to @c elem on success, or NULL if no more elements could
 *          be pulled.
 */
struct bt_mesh_comp_p0_elem *bt_mesh_comp_p0_elem_pull(const struct bt_mesh_comp_p0 *comp,
						       struct bt_mesh_comp_p0_elem *elem);

/** @brief Get a SIG model from the given composition data page 0 element.
 *
 *  @param elem Element to read the model from.
 *  @param idx  Index of the SIG model to read.
 *
 *  @return The Model ID of the SIG model at the given index, or 0xffff if the
 *          index is out of bounds.
 */
uint16_t bt_mesh_comp_p0_elem_mod(struct bt_mesh_comp_p0_elem *elem, int idx);

/** @brief Get a vendor model from the given composition data page 0 element.
 *
 *  @param elem Element to read the model from.
 *  @param idx  Index of the vendor model to read.
 *
 *  @return The model ID of the vendor model at the given index, or
 *          {0xffff, 0xffff} if the index is out of bounds.
 */
struct bt_mesh_mod_id_vnd bt_mesh_comp_p0_elem_mod_vnd(struct bt_mesh_comp_p0_elem *elem, int idx);

/** Composition data page 1 element representation */
struct bt_mesh_comp_p1_elem {
	/** The number of SIG models in this element */
	size_t nsig;
	/** The number of vendor models in this element */
	size_t nvnd;
	/** Buffer containing SIG and Vendor Model Items */
	struct net_buf_simple *_buf;
};

/** Composition data page 1 model item representation */
struct bt_mesh_comp_p1_model_item {
	/** Corresponding_Group_ID field indicator */
	bool cor_present;
	/** Determines the format of Extended Model Item */
	bool format;
	/** Number of items in Extended Model Items*/
	uint8_t ext_item_cnt : 6;
	/** Buffer containing Extended Model Items.
	 *  If cor_present is set to 1 it starts with
	 *  Corresponding_Group_ID
	 */
	uint8_t cor_id;
	struct net_buf_simple *_buf;
};

/** Extended Model Item in short representation */
struct bt_mesh_comp_p1_item_short {
	/** Element address modifier */
	uint8_t elem_offset : 3;
	/** Model Index */
	uint8_t mod_item_idx : 5;
};

/** Extended Model Item in long representation */
struct bt_mesh_comp_p1_item_long {
	/** Element address modifier */
	uint8_t elem_offset;
	/** Model Index */
	uint8_t mod_item_idx;
};

/** Extended Model Item */
struct bt_mesh_comp_p1_ext_item {
	enum { SHORT, LONG } type;

	union {
		/** Item in short representation */
		struct bt_mesh_comp_p1_item_short short_item;
		/** Item in long representation */
		struct bt_mesh_comp_p1_item_long long_item;
	};
};

/** @brief Pull a Composition Data Page 1 Element from a composition data page 1
 *         instance.
 *
 *  Each call to this function will pull out a new element from the composition
 *  data page, until all elements have been pulled.
 *
 *  @param buf Composition data page 1 buffer
 *  @param elem Element to fill.
 *
 *  @return A pointer to @c elem on success, or NULL if no more elements could
 *          be pulled.
 */
struct bt_mesh_comp_p1_elem *bt_mesh_comp_p1_elem_pull(
	struct net_buf_simple *buf, struct bt_mesh_comp_p1_elem *elem);

/** @brief Pull a Composition Data Page 1 Model Item from a Composition Data
 * Page 1 Element
 *
 *  Each call to this function will pull out a new item from the Composition Data
 *  Page 1 Element, until all items have been pulled.
 *
 *  @param elem Composition data page 1 Element
 *  @param item Model Item to fill.
 *
 *  @return A pointer to @c item on success, or NULL if no more elements could
 *          be pulled.
 */
struct bt_mesh_comp_p1_model_item *bt_mesh_comp_p1_item_pull(
	struct bt_mesh_comp_p1_elem *elem, struct bt_mesh_comp_p1_model_item *item);

/** @brief Pull Extended Model Item contained in Model Item
 *
 *  Each call to this function will pull out a new element
 *  from the Extended Model Item, until all elements have been pulled.
 *
 *  @param item Model Item to pull Extended Model Items from
 *  @param ext_item Extended Model Item to fill
 *
 *  @return A pointer to @c ext_item on success, or NULL if item could not be pulled
 */
struct bt_mesh_comp_p1_ext_item *bt_mesh_comp_p1_pull_ext_item(
	struct bt_mesh_comp_p1_model_item *item, struct bt_mesh_comp_p1_ext_item *ext_item);

/** Composition data page 2 record parsing structure. */
struct bt_mesh_comp_p2_record {
	/** Mesh profile ID. */
	uint16_t id;
	/** Mesh Profile Version. */
	struct {
		/** Major version. */
		uint8_t x;
		/** Minor version. */
		uint8_t y;
		/** Z version. */
		uint8_t z;
	} version;
	/** Element offset buffer. */
	struct net_buf_simple *elem_buf;
	/** Additional data buffer. */
	struct net_buf_simple *data_buf;
};

/** @brief Pull a Composition Data Page 2 Record from a composition data page 2
 *         instance.
 *
 *  Each call to this function will pull out a new element from the composition
 *  data page, until all elements have been pulled.
 *
 *  @param buf Composition data page 2 buffer
 *  @param record Record to fill.
 *
 *  @return A pointer to @c record on success, or NULL if no more elements could
 *          be pulled.
 */
struct bt_mesh_comp_p2_record *bt_mesh_comp_p2_record_pull(struct net_buf_simple *buf,
							   struct bt_mesh_comp_p2_record *record);

/** @brief Unpack a list of key index entries from a buffer.
 *
 * On success, @c dst_cnt is set to the amount of unpacked key index entries.
 *
 * @param buf Message buffer containing encoded AppKey or NetKey Indexes.
 * @param dst_arr Destination array for the unpacked list.
 * @param dst_cnt Size of the destination array.
 *
 * @return 0 on success.
 * @return -EMSGSIZE if dst_arr size is to small to parse full message.
 */
int bt_mesh_key_idx_unpack_list(struct net_buf_simple *buf, uint16_t *dst_arr, size_t *dst_cnt);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op bt_mesh_cfg_cli_op[];
extern const struct bt_mesh_model_cb bt_mesh_cfg_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_CFG_CLI_H_ */
