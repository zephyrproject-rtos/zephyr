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

/** Value to set the ISO data path over HCi. */
#define BT_ISO_DATA_PATH_HCI     0x00

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
	sys_snode_t			node;
	uint8_t				state;
	bt_security_t			required_sec_level;
};

/** @brief ISO Channel IO QoS structure. */
struct bt_iso_chan_io_qos {
	/** Channel interval in us. Value range 0x0000FF - 0x0FFFFFF. */
	uint32_t			interval;
	/** Channel Latency in ms. Value range 0x0005 - 0x0FA0. */
	uint16_t			latency;
	/** Channel SDU. Value range 0x0000 - 0x0FFF. */
	uint16_t			sdu;
	/** Channel PHY - See BT_GAP_LE_PHY for values.
	 *  Setting BT_GAP_LE_PHY_NONE is invalid.
	 */
	uint8_t				phy;
	/** Channel Retransmission Number. Value range 0x00 - 0x0F. */
	uint8_t				rtn;
	/** @brief Channel data path reference
	 *
	 *  Setting to NULL default to HCI data path (same as setting path.pid
	 *  to BT_ISO_DATA_PATH_HCI).
	 */
	struct bt_iso_chan_path		*path;
};

/** @brief ISO Channel QoS structure. */
struct bt_iso_chan_qos {
	/** @brief Channel peripherals sleep clock accuracy Only for CIS
	 *
	 * Shall be worst case sleep clock accuracy of all the peripherals.
	 * If unknown for the peripherals, this should be set to BT_GAP_SCA_UNKNOWN.
	 */
	uint8_t				sca;
	/** Channel packing mode. 0 for unpacked, 1 for packed. */
	uint8_t				packing;
	/** Channel framing mode. 0 for unframed, 1 for framed. */
	uint8_t				framing;
	/** @brief Channel Receiving QoS.
	 *
	 *  Setting NULL disables data path BT_HCI_DATAPATH_DIR_CTLR_TO_HOST.
	 *
	 *  Can only be set for a connected isochronous channel, or a broadcast
	 *  isochronous receiver.
	 */
	struct bt_iso_chan_io_qos	*rx;
	/** @brief Channel Transmission QoS
	 *
	 *  Setting NULL disables data path BT_HCI_DATAPATH_DIR_HOST_TO_CTRL.
	 *
	 *  Can only be set for a connected isochronous channel, or a broadcast
	 *  isochronous transmitter.
	 */
	struct bt_iso_chan_io_qos	*tx;
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

/** ISO packet status flags */
enum {
	/** The ISO packet is valid. */
	BT_ISO_FLAGS_VALID,
	/** @brief The ISO packet may possibly contain errors.
	 *
	 * May be caused by a failed CRC check or if missing a part of the SDU.
	 */
	BT_ISO_FLAGS_ERROR,
	/** The ISO packet was lost. */
	BT_ISO_FLAGS_LOST
};

/** @brief ISO Meta Data structure for received ISO packets. */
struct bt_iso_recv_info {
	/** ISO timestamp - valid only if the Bluetooth controller includes it
	 *  If time stamp is not pressent this value will be 0 on all iso packets
	 */
	uint32_t ts;

	/** ISO packet sequence number of the first fragment in the SDU */
	uint16_t sn;

	/** ISO packet flags (BT_ISO_FLAGS_*) */
	uint8_t flags;
};

/** Opaque type representing an Broadcast Isochronous Group (BIG). */
struct bt_iso_big;

struct bt_iso_big_create_param {
	/** Array of pointers to BIS channels */
	struct bt_iso_chan **bis_channels;

	/** Number channels in @p bis_channels */
	uint8_t num_bis;

	/** Whether or not to encrypt the streams. */
	bool  encryption;

	/** @brief Broadcast code
	 *
	 *  The code used to derive the session key that is used to encrypt and
	 *  decrypt BIS payloads.
	 */
	uint8_t  bcode[16];
};

struct bt_iso_big_sync_param {
	/** Array of pointers to BIS channels */
	struct bt_iso_chan **bis_channels;

	/** Number channels in @p bis_channels */
	uint8_t num_bis;

	/** Bitfield of the BISes to sync to */
	uint32_t bis_bitfield;

	/** @brief Maximum subevents
	 *
	 *  The MSE (Maximum Subevents) parameter is the maximum number of subevents that a
	 *  Controller should use to receive data payloads in each interval for a BIS
	 */
	uint32_t mse;

	/** Synchronization timeout for the BIG (N * 10 MS) */
	uint16_t sync_timeout;

	/** Whether or not the streams of the BIG are encrypted */
	bool  encryption;

	/** @brief Broadcast code
	 *
	 *  The code used to derive the session key that is used to encrypt and
	 *  decrypt BIS payloads.
	 */
	uint8_t  bcode[16];
};

struct bt_iso_biginfo {
	/** Address of the advertiser */
	const bt_addr_le_t *addr;

	/** Advertiser SID */
	uint8_t sid;

	/** Number of BISes in the BIG */
	uint8_t  num_bis;

	/** Maximum number of subevents in each isochronous event */
	uint8_t  sub_evt_count;

	/** Interval between two BIG anchor point (N * 1.25 ms) */
	uint16_t iso_interval;

	/** The number of new payloads in each BIS event */
	uint8_t  burst_number;

	/** Offset used for pre-transmissions */
	uint8_t  offset;

	/** The number of times a payload is transmitted in a BIS event */
	uint8_t  rep_count;

	/** Maximum size, in octets, of the payload */
	uint16_t max_pdu;

	/** The interval, in microseconds, of periodic SDUs. */
	uint32_t sdu_interval;

	/** Maximum size of an SDU, in octets. */
	uint16_t max_sdu;

	/** Channel PHY */
	uint8_t  phy;

	/** Channel framing mode */
	uint8_t  framing;

	/** Whether or not the BIG is encrypted */
	bool  encryption;
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
	 *  @param chan   The channel that has been Disconnected
	 *  @param reason HCI reason for the disconnection.
	 */
	void (*disconnected)(struct bt_iso_chan *chan, uint8_t reason);

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
	 *  @param info Pointer to the metadata for the buffer. The lifetime of the
	 *              pointer is linked to the lifetime of the net_buf.
	 *              Metadata such as sequence number and timestamp can be
	 *              provided by the bluetooth controller.
	 */
	void (*recv)(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
			struct net_buf *buf);
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

/** @brief Unbind ISO channel
 *
 *  Unbind ISO channel from ACL connection, channel must be in BT_ISO_BOUND
 *  state.
 *
 *  Note: Channels which the ACL connection has been disconnected are unbind
 *  automatically.
 *
 *  @param chan Channel object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_iso_chan_unbind(struct bt_iso_chan *chan);

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
 *  @note Buffer ownership is transferred to the stack in case of success, in
 *  case of an error the caller retains the ownership of the buffer.
 *
 *  @param chan Channel object.
 *  @param buf Buffer containing data to be sent.
 *
 *  @return Bytes sent in case of success or negative value in case of error.
 */
int bt_iso_chan_send(struct bt_iso_chan *chan, struct net_buf *buf);

/** @brief Creates a BIG as a broadcaster
 *
 *  @param[in] padv      Pointer to the periodic advertising object the BIGInfo shall be sent on.
 *  @param[in] param     The parameters used to create and enable the BIG. The QOS parameters are
 *                       determined by the QOS field of the first BIS in the BIS list of this
 *                       parameter.
 *  @param[out] out_big  Broadcast Isochronous Group object on success.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_iso_big_create(struct bt_le_ext_adv *padv, struct bt_iso_big_create_param *param,
		      struct bt_iso_big **out_big);

/** @brief Terminates a BIG as a broadcaster or receiver
 *
 *  @param big    Pointer to the BIG structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_iso_big_terminate(struct bt_iso_big *big);

/** @brief Creates a BIG as a receiver
 *
 *  @param[in] sync     Pointer to the periodic advertising sync object the BIGInfo was received on.
 *  @param[in] param    The parameters used to create and enable the BIG sync.
 *  @param[out] out_big Broadcast Isochronous Group object on success.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_iso_big_sync(struct bt_le_per_adv_sync *sync, struct bt_iso_big_sync_param *param,
		    struct bt_iso_big **out_big);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_ISO_H_ */
