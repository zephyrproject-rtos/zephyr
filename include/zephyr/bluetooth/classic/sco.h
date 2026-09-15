/** @file
 *  @brief Bluetooth SCO logical transport handling.
 */
/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_SCO_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_SCO_H_

/**
 * @file
 * @brief Synchronous Connection-Oriented (SCO)
 * @defgroup bt_sco Synchronous Connection-Oriented (SCO)
 * @ingroup bluetooth
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Reserved headroom for SCO channel data transmission.
 *
 *  This macro defines the number of bytes that must be reserved at the
 *  beginning of outgoing SCO data buffers for the HCI SCO header.
 *  Applications should account for this headroom when allocating buffers
 *  for SCO data transmission.
 */
#define BT_SCO_CHAN_SEND_RESERVE BT_HCI_SCO_HDR_SIZE

/** @brief Calculate total SCO SDU buffer size including header.
 *
 *  Helper macro to calculate the total buffer size needed for an SCO
 *  Service Data Unit (SDU), including the required headroom for the
 *  HCI SCO header.
 *
 *  @param mtu Maximum Transmission Unit size (payload only).
 *
 *  @return Total buffer size required (MTU + header reserve).
 */
#define BT_SCO_SDU_SIZE(mtu) ((mtu) + BT_HCI_SCO_HDR_SIZE)

/** @brief Life-span states of SCO channel. Used only by internal APIs
 *  dealing with setting channel to proper state depending on operational
 *  context.
 */
enum bt_sco_state {
	/** Channel disconnected */
	BT_SCO_STATE_DISCONNECTED,
	/** Channel is pending ACL encryption before connecting */
	BT_SCO_STATE_ENCRYPT_PENDING,
	/** Channel in connecting state */
	BT_SCO_STATE_CONNECTING,
	/** Channel ready for upper layer traffic on it */
	BT_SCO_STATE_CONNECTED,
	/** Channel in disconnecting state */
	BT_SCO_STATE_DISCONNECTING,
};

struct bt_sco_chan;

/** @brief SCO channel operations structure */
struct bt_sco_chan_ops {
	/** @brief Channel connected callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  connection completes.
	 *
	 *  @param chan The channel that has been connected
	 */
	void (*connected)(struct bt_sco_chan *chan);

	/** @brief Channel disconnected callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  channel is disconnected, including when a connection gets
	 *  rejected or when setting security fails.
	 *
	 *  @param chan   The channel that has been Disconnected
	 *  @param reason BT_HCI_ERR_* reason for the disconnection.
	 */
	void (*disconnected)(struct bt_sco_chan *chan, uint8_t reason);
};

struct bt_sco_stream;

/** @brief SCO channel structure */
struct bt_sco_chan {
	/**  SCO connection reference */
	struct bt_conn *sco;
	/** Channel operations reference */
	const struct bt_sco_chan_ops *ops;

#if defined(CONFIG_BT_VOICE_OVER_HCI)
	/* SCO stream */
	struct bt_sco_stream         *stream;
#endif /* CONFIG_BT_VOICE_OVER_HCI */

	/** Voice setting for the connection */
	uint8_t voice_setting;

	/** SCO Channel state */
	enum bt_sco_state state;
};

/** @brief Initiate an SCO connection to a remote device.
 *
 *  Allows initiate new SCO link to remote peer using its address.
 *
 *  The caller gets a new reference to the connection object which must be
 *  released with bt_conn_unref() once done using the object.
 *
 *  @param peer  Remote address.
 *  @param chan  sco chan object.
 *
 *  @return Valid connection object on success or NULL otherwise.
 */
struct bt_conn *bt_conn_create_sco(const bt_addr_t *peer, struct bt_sco_chan *chan);

/** @brief SCO Accept Info Structure */
struct bt_sco_accept_info {
	/** The ACL connection that is requesting authorization */
	struct bt_conn *acl;

	/** class code of peer device */
	uint8_t   dev_class[3];

	/** link type */
	uint8_t   link_type;
};

/** @brief SCO Server structure. */
struct bt_sco_server {
	/** Required minimum security level.
	 * Only available when @kconfig{CONFIG_BT_SMP} is enabled.
	 */
	bt_security_t		sec_level;
	/** @brief Server accept callback
	 *
	 *  This callback is called whenever a new incoming connection requires
	 *  authorization.
	 *
	 *  @param info The SCO accept information structure
	 *  @param chan Pointer to receive the allocated channel
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*accept)(const struct bt_sco_accept_info *info, struct bt_sco_chan **chan);
};

/** @brief Register SCO server.
 *
 *  Register SCO server, each new connection is authorized using the accept()
 *  callback which in case of success shall allocate the channel structure
 *  to be used by the new connection.
 *
 *  @param server Server structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_sco_server_register(struct bt_sco_server *server);

/** @brief Unregister SCO server.
 *
 *  Unregister previously registered SCO server.
 *
 *  @param server Server structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_sco_server_unregister(struct bt_sco_server *server);

/** @brief SCO connection callback structure.
 *
 *  This structure is used for tracking the state of a SCO connection.
 *  It is registered with the help of the bt_sco_conn_cb_register() API.
 *  It's permissible to register multiple instances of this @ref bt_sco_conn_cb
 *  type, in case different modules of an application are interested in
 *  tracking the connection state. If a callback is not of interest for
 *  an instance, it may be set to NULL and will as a consequence not be
 *  used for that instance.
 */
struct bt_sco_conn_cb {
	/** @brief A new SCO connection has been established.
	 *
	 *  This callback notifies the application of a new connection.
	 *  In case the err parameter is non-zero it means that the
	 *  connection establishment failed.
	 *
	 *  @param conn New SCO connection object.
	 *  @param err HCI error. Zero for success, non-zero otherwise.
	 */
	void (*connected)(struct bt_conn *conn, uint8_t err);

	/** @brief A SCO connection has been disconnected.
	 *
	 *  This callback notifies the application that a SCO connection
	 *  has been disconnected.
	 *  When this callback is called the stack still has one reference to
	 *  the connection object.
	 *
	 *  @param conn SCO connection object.
	 *  @param reason BT_HCI_ERR_* reason for the disconnection.
	 */
	void (*disconnected)(struct bt_conn *conn, uint8_t reason);

	/** @internal Internally used field for list handling */
	sys_snode_t _node;
};

/** @brief Register SCO connection callbacks.
 *
 *  Register callbacks to monitor the state of SCO connections.
 *
 *  @param cb Callback struct. Must point to memory that remains valid.
 *
 * @retval 0 Success.
 * @retval -EINVAL If @p cb is NULL.
 * @retval -EEXIST if @p cb was already registered.
 */
int bt_sco_conn_cb_register(struct bt_sco_conn_cb *cb);

/**
 * @brief Unregister SCO connection callbacks.
 *
 * Unregister the state of SCO connections callbacks.
 *
 * @param cb Callback struct point to memory that remains valid.
 *
 * @retval 0 Success.
 * @retval -EINVAL If @p cb is NULL.
 * @retval -ENOENT if @p cb was not registered.
 */
int bt_sco_conn_cb_unregister(struct bt_sco_conn_cb *cb);

/**
 *  @brief Register a callback structure for connection events.
 *
 *  @param _name Name of callback structure.
 */
#define BT_SCO_CONN_CB_DEFINE(_name)								\
	static const STRUCT_SECTION_ITERABLE(bt_sco_conn_cb, _CONCAT(bt_sco_conn_cb_, _name))

/**
 * @brief SCO HCI callback structure for handling SCO connection events
 *
 * This structure defines callback functions that are invoked during SCO
 * (Synchronous Connection-Oriented) connection establishment process.
 * It allows upper layer protocols to customize HCI command parameters
 * before they are sent to the controller.
 *
 * The callbacks are typically used by audio profiles like HFP/HSP to
 * configure codec parameters, packet types, and other connection-specific
 * settings based on negotiated audio codec and quality requirements.
 *
 * @note Callbacks are optional and may be NULL if default behavior is desired
 */
struct bt_sco_hci_cb {
	/**
	 * @brief Setup callback for outgoing SCO connection
	 *
	 * Called before sending HCI_Setup_Synchronous_Connection command to monitor the HCI
	 * activity of SCO connections.
	 *
	 * @param acl_conn Pointer to the underlying ACL connection.
	 * @param cp Pointer to HCI setup synchronous connection command parameters.
	 */
	void (*setup)(struct bt_conn *acl_conn, struct bt_hci_cp_setup_sync_conn *cp);

	/**
	 * @brief Accept callback for incoming SCO connection
	 *
	 * Called before sending HCI_Accept_Synchronous_Connection_Request command to monitor the
	 * HCI activity of SCO connections.
	 *
	 * @param cp Pointer to HCI accept synchronous connection request command parameters.
	 */
	void (*accept)(struct bt_hci_cp_accept_sync_conn_req *cp);

	/** @internal Internally used field for list handling */
	sys_snode_t _node;
};

/** @brief Register SCO HCI activity callbacks.
 *
 *  Register callbacks to monitor the HCI activity of SCO.
 *
 *  @param cb Callback struct. Must point to memory that remains valid.
 *
 * @retval 0 Success.
 * @retval -EINVAL If @p cb is NULL.
 * @retval -EEXIST if @p cb was already registered.
 */
int bt_sco_hci_cb_register(struct bt_sco_hci_cb *cb);

/**
 * @brief Unregister SCO HCI activity callbacks.
 *
 * Unregister the HCI activity monitor of SCO callbacks.
 *
 * @param cb Callback struct point to memory that remains valid.
 *
 * @retval 0 Success.
 * @retval -EINVAL If @p cb is NULL.
 * @retval -ENOENT if @p cb was not registered.
 */
int bt_sco_hci_cb_unregister(struct bt_sco_hci_cb *cb);

/**
 *  @brief Register a callback structure for SCO HCI activity.
 *
 *  @param _name Name of callback structure.
 */
#define BT_SCO_HCI_CB_DEFINE(_name) \
	static const STRUCT_SECTION_ITERABLE(bt_sco_hci_cb, _CONCAT(bt_sco_hci_cb_, _name))

/**
 * @brief SCO stream callback operations structure.
 *
 * @kconfig_dep{CONFIG_BT_VOICE_OVER_HCI}
 *
 * This structure defines callback functions for handling SCO stream events.
 * These callbacks are invoked when data is received or transmitted over the
 * SCO stream.
 */
struct bt_sco_stream_ops {
	/** @brief Data received callback
	 *
	 *  Called when data is received on the SCO stream.
	 *
	 *  @param stream Pointer to the SCO stream that received data.
	 *  @param flag Status flag for the received data.
	 *  @param buf  The buffer containing the received data.
	 */
	void (*recv)(struct bt_sco_stream *stream, uint8_t flag, struct net_buf *buf);

	/** @brief Data sent callback
	 *
	 *  Called when data has been successfully sent on the SCO stream.
	 *  This callback can be used to track transmission completion and
	 *  manage flow control.
	 *
	 *  @param stream Pointer to the SCO stream that sent the data.
	 */
	void (*sent)(struct bt_sco_stream *stream);
};

/**
 * @brief SCO stream structure.
 *
 * @kconfig_dep{CONFIG_BT_VOICE_OVER_HCI}
 *
 * This structure represents an SCO audio/data stream and contains
 * references to the underlying SCO connection and stream operation callbacks.
 */
struct bt_sco_stream {
	/** Pointer to the SCO connection object */
	struct bt_conn *sco;
	/** Pointer to stream operation callbacks */
	struct bt_sco_stream_ops *ops;
};

/** @brief Register SCO stream callbacks.
 *
 *  @kconfig_dep{CONFIG_BT_VOICE_OVER_HCI}
 *
 *  Register callback operations for an SCO stream. These callbacks will be
 *  invoked for data reception and transmission events on the stream.
 *
 *  @param stream Pointer to the SCO stream.
 *  @param ops    Pointer to the stream operations structure.
 *                Must point to memory that remains valid.
 *
 *  @retval 0 Success.
 *  @retval -EINVAL If @p stream or @p ops is NULL.
 */
int bt_sco_stream_cb_register(struct bt_sco_stream *stream, struct bt_sco_stream_ops *ops);

/** @brief Unregister SCO stream callbacks.
 *
 *  @kconfig_dep{CONFIG_BT_VOICE_OVER_HCI}
 *
 *  Unregister previously registered stream callbacks.
 *
 *  @param stream Pointer to the SCO stream.
 *
 *  @retval 0 Success.
 *  @retval -EINVAL If @p stream is NULL.
 */
int bt_sco_stream_cb_unregister(struct bt_sco_stream *stream);

/** @brief Connect an SCO stream to an SCO connection.
 *
 *  @kconfig_dep{CONFIG_BT_VOICE_OVER_HCI}
 *
 *  Associates an SCO stream with an established SCO connection, enabling
 *  data transmission and reception through the stream interface.
 *
 *  @param sco    Pointer to an established SCO connection object.
 *  @param stream Pointer to the SCO stream to connect.
 *
 *  @retval 0 Success.
 *  @retval -EINVAL If @p sco or @p stream is NULL.
 *  @retval -EALREADY If the stream is already connected.
 *  @retval -EBUSY If there is any other stream has been connected.
 */
int bt_sco_stream_connect(struct bt_conn *sco, struct bt_sco_stream *stream);

/** @brief Disconnect an SCO stream.
 *
 *  @kconfig_dep{CONFIG_BT_VOICE_OVER_HCI}
 *
 *  Disconnects an SCO stream from its associated SCO connection.
 *  After disconnection, the stream can no longer be used for data
 *  transmission or reception until reconnected.
 *
 *  @param stream Pointer to the SCO stream to disconnect.
 *
 *  @retval 0 Success.
 *  @retval -EINVAL If @p stream is NULL.
 *  @retval -ENOTCONN If the stream is not connected.
 */
int bt_sco_stream_disconnect(struct bt_sco_stream *stream);

/** @brief Send data over an SCO stream.
 *
 *  @kconfig_dep{CONFIG_BT_VOICE_OVER_HCI}
 *
 *  Send audio or data packets over the SCO stream. The buffer should contain
 *  properly formatted SCO data according to the negotiated voice settings.
 *
 *  @note The buffer must have sufficient headroom (at least
 *        @ref BT_SCO_CHAN_SEND_RESERVE bytes) for the HCI SCO header.
 *  @note The buffer's user_data_size must be at least
 *        CONFIG_BT_CONN_TX_USER_DATA_SIZE bytes.
 *  @note The data length must not exceed the SCO MTU.
 *
 *  @param stream Pointer to the connected SCO stream.
 *  @param buf    Network buffer containing the data to send.
 *                Ownership of the buffer is transferred to the stack.
 *
 *  @retval 0 Success. Buffer queued for transmission.
 *  @retval -EINVAL If @p stream or @p buf is NULL, insufficient headroom,
 *                  or buffer user_data_size is too small.
 *  @retval -ENOTCONN If the stream or underlying SCO connection is not connected.
 *  @retval -EMSGSIZE If the data length exceeds the SCO MTU.
 */
int bt_sco_stream_send(struct bt_sco_stream *stream, struct net_buf *buf);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_SCO_H_ */
