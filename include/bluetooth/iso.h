/** @file
 *  @brief Bluetooth ISO handling
 */

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_ISO_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_ISO_H_

/**
 * @brief ISO
 * @defgroup bt_iso ISO
 * @ingroup bluetooth
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/atomic.h>
#include <bluetooth/buf.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci.h>

/** @def BT_ISO_CHAN_SEND_RESERVE
 *  @brief Headroom needed for outgoing buffers
 */
#define BT_ISO_CHAN_SEND_RESERVE (CONFIG_BT_HCI_RESERVE + \
				  BT_HCI_ISO_HDR_SIZE + \
				  BT_HCI_ISO_DATA_HDR_SIZE)

struct bt_iso_chan;

/** @brief Life-span states of ISO channel. Used only by internal APIs
 *  dealing with setting channel to proper state depending on operational
 *  context.
 */
enum {
	/** Channel disconnected */
	BT_ISO_DISCONNECTED,
	/** Channel bound to a connection */
	BT_ISO_BOUND,
	/** Channel in connecting state */
	BT_ISO_CONNECT,
	/** Channel ready for upper layer traffic on it */
	BT_ISO_CONNECTED,
	/** Channel in disconnecting state */
	BT_ISO_DISCONNECT,
};

/** @brief ISO Channel structure. */
struct bt_iso_chan {
	/** Channel connection reference */
	struct bt_conn			*conn;
	/** Channel operations reference */
	struct bt_iso_chan_ops		*ops;
	/** Channel QoS reference */
	struct bt_iso_chan_qos		*qos;
	/** Channel data path reference*/
	struct bt_iso_chan_path		*path;
	sys_snode_t			node;
	uint8_t				state;
	bt_security_t			required_sec_level;
};

/** @brief Audio QoS direction */
enum {
	BT_ISO_CHAN_QOS_IN,
	BT_ISO_CHAN_QOS_OUT,
	BT_ISO_CHAN_QOS_INOUT
};

/** @brief ISO Channel QoS structure. */
struct bt_iso_chan_qos {
	/** @brief Channel direction
	 *
	 *  Possible values: BT_ISO_CHAN_QOS_IN, BT_ISO_CHAN_QOS_OUT or
	 *  BT_ISO_CHAN_QOS_INOUT.
	 */
	uint8_t				dir;
	/** Channel interval */
	uint32_t			interval;
	/** Channel SCA */
	uint8_t				sca;
	/** Channel packing mode */
	uint8_t				packing;
	/** Channel framing mode */
	uint8_t				framing;
	/** Channel Latency */
	uint16_t			latency;
	/** Channel SDU */
	uint8_t				sdu;
	/** Channel PHY */
	uint8_t				phy;
	/** Channel Retransmission Number */
	uint8_t				rtn;
};

/** @brief ISO Channel Data Path structure. */
struct bt_iso_chan_path {
	/** Default path ID */
	uint8_t				pid;
	/** Coding Format */
	uint8_t				format;
	/** Company ID */
	uint16_t			cid;
	/** Vendor-defined Codec ID */
	uint16_t			vid;
	/** Controller Delay */
	uint32_t			delay;
	/** Codec Configuration length*/
	uint8_t				cc_len;
	/** Codec Configuration */
	uint8_t				cc[0];
};

/** @brief ISO Channel operations structure. */
struct bt_iso_chan_ops {
	/** @brief Channel connected callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  connection completes.
	 *
	 *  @param chan The channel that has been connected
	 */
	void (*connected)(struct bt_iso_chan *chan);

	/** @brief Channel disconnected callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  channel is disconnected, including when a connection gets
	 *  rejected.
	 *
	 *  @param chan The channel that has been Disconnected
	 */
	void (*disconnected)(struct bt_iso_chan *chan);

	/** @brief Channel alloc_buf callback
	 *
	 *  If this callback is provided the channel will use it to allocate
	 *  buffers to store incoming data.
	 *
	 *  @param chan The channel requesting a buffer.
	 *
	 *  @return Allocated buffer.
	 */
	struct net_buf *(*alloc_buf)(struct bt_iso_chan *chan);

	/** @brief Channel recv callback
	 *
	 *  @param chan The channel receiving data.
	 *  @param buf Buffer containing incoming data.
	 */
	void (*recv)(struct bt_iso_chan *chan, struct net_buf *buf);
};

/** @brief ISO Server structure. */
struct bt_iso_server {
	/** Required minimim security level */
	bt_security_t		sec_level;

	/** @brief Server accept callback
	 *
	 *  This callback is called whenever a new incoming connection requires
	 *  authorization.
	 *
	 *  @param conn The connection that is requesting authorization
	 *  @param chan Pointer to receive the allocated channel
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_iso_chan **chan);
};

/** @brief Register ISO server.
 *
 *  Register ISO server, each new connection is authorized using the accept()
 *  callback which in case of success shall allocate the channel structure
 *  to be used by the new connection.
 *
 *  @param server Server structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_iso_server_register(struct bt_iso_server *server);

/** @brief Bind ISO channels
 *
 *  Bind ISO channels with existing ACL connections, Channel objects passed
 *  (over an address of it) shouldn't be instantiated in application as
 *  standalone.
 *
 *  @param conns Array of ACL connection objects
 *  @param num_conns Number of connection objects
 *  @param chans Array of ISO Channel objects to be created
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_iso_chan_bind(struct bt_conn **conns, uint8_t num_conns,
		     struct bt_iso_chan **chans);

/** @brief Connect ISO channels
 *
 *  Connect ISO channels, once the connection is completed each channel
 *  connected() callback will be called. If the connection is rejected
 *  disconnected() callback is called instead.
 *  Channel object passed (over an address of it) as second parameter shouldn't
 *  be instantiated in application as standalone.
 *
 *  @param chans Array of ISO channel objects
 *  @param num_chans Number of channel objects
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_iso_chan_connect(struct bt_iso_chan **chans, uint8_t num_chans);

/** @brief Disconnect ISO channel
 *
 *  Disconnect ISO channel, if the connection is pending it will be
 *  canceled and as a result the channel disconnected() callback is called.
 *  Regarding to input parameter, to get details see reference description
 *  to bt_iso_chan_connect() API above.
 *
 *  @param chan Channel object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_iso_chan_disconnect(struct bt_iso_chan *chan);

/** @brief Send data to ISO channel
 *
 *  Send data from buffer to the channel. If credits are not available, buf will
 *  be queued and sent as and when credits are received from peer.
 *  Regarding to first input parameter, to get details see reference description
 *  to bt_iso_chan_connect() API above.
 *
 *  @param chan Channel object.
 *  @param buf Buffer containing data to be sent.
 *
 *  @return Bytes sent in case of success or negative value in case of error.
 */
int bt_iso_chan_send(struct bt_iso_chan *chan, struct net_buf *buf);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_ISO_H_ */
