/** @file
 *  @brief Internal APIs for Bluetooth SCO handling.
 */
/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

struct bt_sco_chan {
	struct bt_conn *sco;
	/** Channel operations reference */
	struct bt_sco_chan_ops		*ops;

	enum bt_sco_state		state;
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
	int (*accept)(const struct bt_sco_accept_info *info,
			  struct bt_sco_chan **chan);
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

/** @brief sco channel connected.
 *
 *  sco channel connected
 *
 *  @param sco SCO connection object.
 */
void bt_sco_connected(struct bt_conn *sco);

/** @brief sco channel disconnected.
 *
 *  sco channel disconnected
 *
 *  @param sco SCO connection object.
 */
void bt_sco_disconnected(struct bt_conn *sco);

uint8_t bt_esco_conn_req(struct bt_hci_evt_conn_request *evt);

#if defined(CONFIG_BT_CONN_LOG_LEVEL_DBG)
void bt_sco_chan_set_state_debug(struct bt_sco_chan *chan,
				 enum bt_sco_state state,
				 const char *func, int line);
#define bt_sco_chan_set_state(_chan, _state) \
	bt_sco_chan_set_state_debug(_chan, _state, __func__, __LINE__)
#else
void bt_sco_chan_set_state(struct bt_sco_chan *chan, enum bt_sco_state state);
#endif /* CONFIG_BT_CONN_LOG_LEVEL_DBG */
