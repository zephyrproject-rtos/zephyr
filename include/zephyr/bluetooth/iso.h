/**
 * @file
 * @brief Bluetooth ISO handling
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2021-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_ISO_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_ISO_H_

/**
 * @brief Isochronous channels (ISO)
 * @defgroup bt_iso Isochronous channels (ISO)
 *
 * @since 2.3
 * @version 0.8.0
 *
 * @ingroup bluetooth
 * @{
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief Headroom needed for outgoing ISO SDUs
 */
#define BT_ISO_CHAN_SEND_RESERVE BT_BUF_ISO_SIZE(0)

/**
 *  @brief Helper to calculate needed buffer size for ISO SDUs.
 *         Useful for creating buffer pools.
 *
 *  @param mtu Required ISO SDU size
 *
 *  @return Needed buffer size to match the requested ISO SDU MTU.
 */
#define BT_ISO_SDU_BUF_SIZE(mtu) BT_BUF_ISO_SIZE(mtu)

/**
 * Convert BIS index to bit
 *
 * The BIS indexes start from 0x01, so the lowest allowed bit is
 * BIT(0) that represents index 0x01. To synchronize to e.g. BIS
 * indexes 0x01 and 0x02, the bitfield value should be BIT(0) | BIT(1).
 * As a general notation, to sync to BIS index N use BIT(N - 1).
 */
#define BT_ISO_BIS_INDEX_BIT(x) (BIT((x) - 1))

/** Value to set the ISO data path over HCi. */
#define BT_ISO_DATA_PATH_HCI        0x00

/** Minimum interval value in microseconds */
#define BT_ISO_SDU_INTERVAL_MIN     0x0000FFU
/** Maximum interval value in microseconds */
#define BT_ISO_SDU_INTERVAL_MAX     0x0FFFFFU
/** Minimum ISO interval (N * 1.25 ms) */
#define BT_ISO_ISO_INTERVAL_MIN     0x0004U
/** Maximum ISO interval (N * 1.25 ms) */
#define BT_ISO_ISO_INTERVAL_MAX     0x0C80U
/** Minimum latency value in milliseconds */
#define BT_ISO_LATENCY_MIN          0x0005
/** Maximum latency value in milliseconds */
#define BT_ISO_LATENCY_MAX          0x0FA0
/** Packets will be sent sequentially between the channels in the group */
#define BT_ISO_PACKING_SEQUENTIAL   0x00
/** Packets will be sent interleaved between the channels in the group */
#define BT_ISO_PACKING_INTERLEAVED  0x01
/** Packets may be framed or unframed */
#define BT_ISO_FRAMING_UNFRAMED     0x00
/** Packets are always framed */
#define BT_ISO_FRAMING_FRAMED       0x01
/** Maximum number of isochronous channels in a single group */
#define BT_ISO_MAX_GROUP_ISO_COUNT  0x1F
/** Minimum SDU size */
#define BT_ISO_MIN_SDU              0x0001
/** Maximum SDU size */
#define BT_ISO_MAX_SDU              0x0FFF
/** Minimum PDU size */
#define BT_ISO_CONNECTED_PDU_MIN    0x0000U
/** Minimum PDU size */
#define BT_ISO_BROADCAST_PDU_MIN    0x0001U
/** Maximum PDU size */
#define BT_ISO_PDU_MAX              0x00FBU
/** Minimum burst number */
#define BT_ISO_BN_MIN               0x01U
/** Maximum burst number */
#define BT_ISO_BN_MAX               0x0FU
/** Minimum flush timeout */
#define BT_ISO_FT_MIN               0x01U
/** Maximum flush timeout */
#define BT_ISO_FT_MAX               0xFFU
/** Minimum number of subevents */
#define BT_ISO_NSE_MIN              0x01U
/** Maximum number of subevents */
#define BT_ISO_NSE_MAX              0x1FU
/** Minimum BIG sync timeout value (N * 10 ms) */
#define BT_ISO_SYNC_TIMEOUT_MIN     0x000A
/** Maximum BIG sync timeout value (N * 10 ms) */
#define BT_ISO_SYNC_TIMEOUT_MAX     0x4000
/** Controller controlled maximum subevent count value */
#define BT_ISO_SYNC_MSE_ANY         0x00
/** Minimum BIG sync maximum subevent count value */
#define BT_ISO_SYNC_MSE_MIN         0x01
/** Maximum BIG sync maximum subevent count value */
#define BT_ISO_SYNC_MSE_MAX         0x1F
/** Maximum connected ISO retransmission value */
#define BT_ISO_CONNECTED_RTN_MAX    0xFF
/** Maximum broadcast ISO retransmission value */
#define BT_ISO_BROADCAST_RTN_MAX    0x1E
/** Broadcast code size */
#define BT_ISO_BROADCAST_CODE_SIZE  16
/** Lowest BIS index */
#define BT_ISO_BIS_INDEX_MIN        0x01
/** Highest BIS index */
#define BT_ISO_BIS_INDEX_MAX        0x1F
/** Minimum Immediate Repetition Count */
#define BT_ISO_IRC_MIN              0x01U
/** Maximum Immediate Repetition Count */
#define BT_ISO_IRC_MAX              0x0FU
/** Minimum pre-transmission offset */
#define BT_ISO_PTO_MIN              0x00U
/** Maximum pre-transmission offset */
#define BT_ISO_PTO_MAX              0x0FU

/**
 * @brief Check if ISO BIS bitfield is valid (BT_ISO_BIS_INDEX_BIT(1)|..|BT_ISO_BIS_INDEX_BIT(31))
 *
 * @param _bis_bitfield BIS index bitfield (uint32)
 */
#define BT_ISO_VALID_BIS_BITFIELD(_bis_bitfield)                                                   \
	((_bis_bitfield) != 0U && (_bis_bitfield) <= BIT_MASK(BT_ISO_BIS_INDEX_MAX))

/**
 * @brief Life-span states of ISO channel. Used only by internal APIs dealing with setting channel
 * to proper state depending on operational context.
 */
enum bt_iso_state {
	/** Channel disconnected */
	BT_ISO_STATE_DISCONNECTED,
	/** Channel is pending ACL encryption before connecting */
	BT_ISO_STATE_ENCRYPT_PENDING,
	/** Channel in connecting state */
	BT_ISO_STATE_CONNECTING,
	/** Channel ready for upper layer traffic on it */
	BT_ISO_STATE_CONNECTED,
	/** Channel in disconnecting state */
	BT_ISO_STATE_DISCONNECTING,
};


/**
 * @brief ISO Channel Type.
 */
enum bt_iso_chan_type {
	BT_ISO_CHAN_TYPE_NONE,		/**< No channel type */
	BT_ISO_CHAN_TYPE_CONNECTED,	/**< Connected */
	BT_ISO_CHAN_TYPE_BROADCASTER,	/**< Isochronous broadcaster */
	BT_ISO_CHAN_TYPE_SYNC_RECEIVER	/**< Synchronized receiver */
};

/** @brief ISO Channel structure. */
struct bt_iso_chan {
	/** Channel connection reference */
	struct bt_conn			*iso;
	/** Channel operations reference */
	struct bt_iso_chan_ops		*ops;
	/** Channel QoS reference */
	struct bt_iso_chan_qos		*qos;
	/** Channel state */
	enum bt_iso_state		state;
#if (defined(CONFIG_BT_SMP) && defined(CONFIG_BT_ISO_UNICAST)) || defined(__DOXYGEN__)
	/**
	 * @brief The required security level of the channel
	 *
	 * This value can be set as the central before connecting a CIS
	 * with bt_iso_chan_connect().
	 * The value is overwritten to @ref bt_iso_server::sec_level for the
	 * peripheral once a channel has been accepted.
	 *
	 * Only available when @kconfig{CONFIG_BT_SMP} is enabled.
	 */
	bt_security_t			required_sec_level;
#endif /* CONFIG_BT_SMP && CONFIG_BT_ISO_UNICAST */
	/** @internal Node used internally by the stack */
	sys_snode_t node;
};

/** @brief ISO Channel IO QoS structure. */
struct bt_iso_chan_io_qos {
	/**
	 * @brief Channel SDU.
	 *
	 * Value range is @ref BT_ISO_MIN_SDU to @ref BT_ISO_MAX_SDU.
	 */
	uint16_t			sdu;
	/**
	 * @brief Channel PHY - See the BT_GAP_LE_PHY_* values.
	 *
	 * Setting @ref BT_GAP_LE_PHY_NONE is invalid.
	 */
	uint8_t				phy;
	/**
	 * @brief Channel Retransmission Number.
	 *
	 * This value is ignored if any advanced ISO parameters are set.
	 */
	uint8_t				rtn;
	/**
	 * @brief Channel data path reference
	 *
	 * Setting to NULL default to HCI data path (same as setting path.pid
	 * to @ref BT_ISO_DATA_PATH_HCI).
	 */
	struct bt_iso_chan_path		*path;

#if defined(CONFIG_BT_ISO_TEST_PARAMS) || defined(__DOXYGEN__)
	/**
	 * @brief Maximum PDU size
	 *
	 * Maximum size, in octets, of the payload from link layer to link layer.
	 *
	 * Value range @ref BT_ISO_CONNECTED_PDU_MIN to @ref BT_ISO_PDU_MAX for connected ISO.
	 *
	 * Value range @ref BT_ISO_BROADCAST_PDU_MIN to @ref BT_ISO_PDU_MAX for broadcast ISO.
	 */
	uint16_t max_pdu;

	/**
	 * @brief Burst number
	 *
	 * Value range @ref BT_ISO_BN_MIN to @ref BT_ISO_BN_MAX.
	 */
	uint8_t burst_number;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */
};

/** @brief ISO Channel QoS structure. */
struct bt_iso_chan_qos {
	/**
	 * @brief Channel Receiving QoS.
	 *
	 * Setting NULL disables data path @ref BT_HCI_DATAPATH_DIR_CTLR_TO_HOST.
	 *
	 * Can only be set for a connected isochronous channel, or a broadcast isochronous receiver.
	 */
	struct bt_iso_chan_io_qos	*rx;
	/**
	 * @brief Channel Transmission QoS
	 *
	 * Setting NULL disables data path @ref BT_HCI_DATAPATH_DIR_HOST_TO_CTLR.
	 *
	 * Can only be set for a connected isochronous channel, or a broadcast
	 * isochronous transmitter.
	 */
	struct bt_iso_chan_io_qos	*tx;

#if defined(CONFIG_BT_ISO_TEST_PARAMS) || defined(__DOXYGEN__)
	/**
	 * @brief Number of subevents
	 *
	 * Maximum number of subevents in each CIS or BIS event.
	 *
	 * Value range @ref BT_ISO_NSE_MIN to @ref BT_ISO_NSE_MAX.
	 */
	uint8_t num_subevents;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */
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
	/** Pointer to an array containing the Codec Configuration */
	uint8_t				*cc;
};

/** ISO packet status flag bits */
enum {
	/** The ISO packet is valid. */
	BT_ISO_FLAGS_VALID = BIT(0),

	/**
	 * @brief The ISO packet may possibly contain errors.
	 *
	 * May be caused by a failed CRC check or if missing a part of the SDU.
	 */
	BT_ISO_FLAGS_ERROR = BIT(1),

	/** The ISO packet was lost. */
	BT_ISO_FLAGS_LOST = BIT(2),

	/**
	 * @brief Timestamp is valid
	 *
	 * If not set, then the bt_iso_recv_info.ts value is not valid, and
	 * should not be used.
	 */
	BT_ISO_FLAGS_TS = BIT(3)
};

/** @brief ISO Meta Data structure for received ISO packets. */
struct bt_iso_recv_info {
	/**
	 * @brief ISO timestamp
	 *
	 * Only valid if @p flags has the @ref BT_ISO_FLAGS_TS bit set.
	 */
	uint32_t ts;

	/** ISO packet sequence number of the first fragment in the SDU */
	uint16_t seq_num;

	/** ISO packet flags bitfield (BT_ISO_FLAGS_*) */
	uint8_t flags;
};

/** @brief ISO Meta Data structure for transmitted ISO packets. */
struct bt_iso_tx_info {
	/** CIG reference point or BIG anchor point of a transmitted SDU, in microseconds. */
	uint32_t ts;

	/** Time offset, in microseconds */
	uint32_t offset;

	/** Packet sequence number */
	uint16_t seq_num;
};


/** Opaque type representing an Connected Isochronous Group (CIG). */
struct bt_iso_cig;

/** @brief Connected Isochronous Group (CIG) parameters */
struct bt_iso_cig_param {
	/** @brief Array of pointers to CIS channels */
	struct bt_iso_chan **cis_channels;

	/**
	 * @brief Number of channels in @p cis_channels
	 *
	 * Maximum number of channels in a single group is @ref BT_ISO_MAX_GROUP_ISO_COUNT
	 */
	uint8_t num_cis;

	/**
	 * @brief Channel interval in us for SDUs sent from Central to Peripheral.
	 *
	 * Value range @ref BT_ISO_SDU_INTERVAL_MIN to @ref BT_ISO_SDU_INTERVAL_MAX.
	 */
	uint32_t c_to_p_interval;

	/**
	 * @brief Channel interval in us for SDUs sent from Peripheral to Central.
	 *
	 * Value range @ref BT_ISO_SDU_INTERVAL_MIN to @ref BT_ISO_SDU_INTERVAL_MAX.
	 */
	uint32_t p_to_c_interval;

	/**
	 * @brief Channel Latency in ms for SDUs sent from Central to Peripheral
	 *
	 * Value range @ref BT_ISO_LATENCY_MIN to @ref BT_ISO_LATENCY_MAX.
	 *
	 * This value is ignored if any advanced ISO parameters are set.
	 */
	uint16_t c_to_p_latency;

	/**
	 * @brief Channel Latency in ms for SDUs sent from Peripheral to Central
	 *
	 * Value range @ref BT_ISO_LATENCY_MIN to @ref BT_ISO_LATENCY_MAX.
	 *
	 * This value is ignored if any advanced ISO parameters are set.
	 */
	uint16_t p_to_c_latency;

	/**
	 * @brief Channel peripherals sleep clock accuracy Only for CIS
	 *
	 * Shall be worst case sleep clock accuracy of all the peripherals.
	 * For possible values see the BT_GAP_SCA_* values.
	 * If unknown for the peripherals, this should be set to @ref BT_GAP_SCA_UNKNOWN.
	 */
	uint8_t sca;

	/**
	 * @brief Channel packing mode.
	 *
	 * @ref BT_ISO_PACKING_SEQUENTIAL or @ref BT_ISO_PACKING_INTERLEAVED
	 */
	uint8_t packing;

	/**
	 * @brief Channel framing mode.
	 *
	 * @ref BT_ISO_FRAMING_UNFRAMED for unframed and @ref BT_ISO_FRAMING_FRAMED for framed.
	 */
	uint8_t framing;

#if defined(CONFIG_BT_ISO_TEST_PARAMS) || defined(__DOXYGEN__)
	/**
	 * @brief Central to Peripheral flush timeout
	 *
	 * The flush timeout in multiples of ISO_Interval for each payload sent
	 * from the Central to Peripheral.
	 *
	 * Value range from @ref BT_ISO_FT_MIN to @ref BT_ISO_FT_MAX
	 */
	uint8_t c_to_p_ft;

	/**
	 * @brief Peripheral to Central flush timeout
	 *
	 * The flush timeout in multiples of ISO_Interval for each payload sent
	 * from the Peripheral to Central.
	 *
	 * Value range from @ref BT_ISO_FT_MIN to @ref BT_ISO_FT_MAX.
	 */
	uint8_t p_to_c_ft;

	/**
	 * @brief ISO interval
	 *
	 * Time between consecutive CIS anchor points.
	 *
	 * Value range from @ref BT_ISO_ISO_INTERVAL_MIN to @ref BT_ISO_ISO_INTERVAL_MAX.
	 */
	uint16_t iso_interval;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */
};

/** ISO connection parameters structure */
struct bt_iso_connect_param {
	/** The ISO channel to connect */
	struct bt_iso_chan *iso_chan;

	/** The ACL connection */
	struct bt_conn *acl;
};

/** Opaque type representing a Broadcast Isochronous Group (BIG). */
struct bt_iso_big;

/** @brief Broadcast Isochronous Group (BIG) creation parameters */
struct bt_iso_big_create_param {
	/** Array of pointers to BIS channels */
	struct bt_iso_chan **bis_channels;

	/**
	 * @brief Number of channels in @p bis_channels
	 *
	 * Maximum number of channels in a single group is @ref BT_ISO_MAX_GROUP_ISO_COUNT
	 */
	uint8_t num_bis;

	/**
	 * @brief Channel interval in us.
	 *
	 *  Value range @ref BT_ISO_SDU_INTERVAL_MIN to @ref BT_ISO_SDU_INTERVAL_MAX.
	 */
	uint32_t interval;

	/**
	 * @brief Channel Latency in ms.
	 *
	 * Value range @ref BT_ISO_LATENCY_MIN to @ref BT_ISO_LATENCY_MAX.
	 *
	 * This value is ignored if any advanced ISO parameters are set.
	 */
	uint16_t latency;

	/**
	 * @brief Channel packing mode.
	 *
	 * @ref BT_ISO_PACKING_SEQUENTIAL or @ref BT_ISO_PACKING_INTERLEAVED
	 */
	uint8_t packing;

	/**
	 * @brief Channel framing mode.
	 *
	 * @ref BT_ISO_FRAMING_UNFRAMED for unframed and @ref BT_ISO_FRAMING_FRAMED for framed.
	 */
	uint8_t framing;

	/** Whether or not to encrypt the streams. */
	bool encryption;

	/**
	 * @brief Broadcast code
	 *
	 * The code used to derive the session key that is used to encrypt and decrypt BIS payloads.
	 *
	 * If the value is a string or the value is less than 16 octets,
	 * the remaining octets shall be 0.
	 *
	 * Example:
	 *   The string "Broadcast Code" shall be
	 *   [42 72 6F 61 64 63 61 73 74 20 43 6F 64 65 00 00]
	 */
	uint8_t bcode[BT_ISO_BROADCAST_CODE_SIZE];

#if defined(CONFIG_BT_ISO_TEST_PARAMS) || defined(__DOXYGEN__)
	/**
	 * @brief Immediate Repetition Count
	 *
	 * The number of times the scheduled payloads are transmitted in a given event.
	 *
	 * Value range from @ref BT_ISO_IRC_MIN to @ref BT_ISO_IRC_MAX.
	 */
	uint8_t irc;

	/**
	 * @brief Pre-transmission offset
	 *
	 * Offset used for pre-transmissions.
	 *
	 * Value range from @ref BT_ISO_PTO_MIN to @ref BT_ISO_PTO_MAX.
	 */
	uint8_t pto;

	/**
	 * @brief ISO interval
	 *
	 * Time between consecutive BIS anchor points.
	 *
	 * Value range from @ref BT_ISO_ISO_INTERVAL_MIN to  @ref BT_ISO_ISO_INTERVAL_MAX.
	 */
	uint16_t iso_interval;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */
};

/** @brief Broadcast Isochronous Group (BIG) Sync Parameters */
struct bt_iso_big_sync_param {
	/** Array of pointers to BIS channels */
	struct bt_iso_chan **bis_channels;

	/**
	 * @brief Number channels in @p bis_channels
	 *
	 * Maximum number of channels in a single group is @ref BT_ISO_MAX_GROUP_ISO_COUNT
	 */
	uint8_t num_bis;

	/**
	 * @brief Bitfield of the BISes to sync to
	 *
	 * Use @ref BT_ISO_BIS_INDEX_BIT to convert BIS indexes to a bitfield.
	 *
	 * To synchronize to e.g. BIS indexes 0x01 and 0x02, this can be set to
	 * BT_ISO_BIS_INDEX_BIT(0x01) | BT_ISO_BIS_INDEX_BIT(0x02).
	 */
	uint32_t bis_bitfield;

	/**
	 * @brief Maximum subevents
	 *
	 * The MSE (Maximum Subevents) parameter is the maximum number of
	 * subevents that a  Controller should use to receive data payloads
	 * in each interval for a BIS.
	 *
	 * Value range is @ref BT_ISO_SYNC_MSE_MIN to @ref BT_ISO_SYNC_MSE_MAX, or
	 * @ref BT_ISO_SYNC_MSE_ANY to let the controller choose.
	 */
	uint32_t mse;

	/**
	 * @brief Synchronization timeout for the BIG (N * 10 MS)
	 *
	 * Value range is @ref BT_ISO_SYNC_TIMEOUT_MIN to @ref BT_ISO_SYNC_TIMEOUT_MAX.
	 */
	uint16_t sync_timeout;

	/** Whether or not the streams of the BIG are encrypted */
	bool  encryption;

	/**
	 * @brief Broadcast code
	 *
	 * The code used to derive the session key that is used to encrypt and decrypt BIS payloads.
	 *
	 * If the value is a string or a the value is less than 16 octets,
	 * the remaining octets shall be 0.
	 *
	 * Example:
	 *   The string "Broadcast Code" shall be
	 *   [42 72 6F 61 64 63 61 73 74 20 43 6F 64 65 00 00]
	 */
	uint8_t bcode[BT_ISO_BROADCAST_CODE_SIZE];
};

/** @brief Broadcast Isochronous Group (BIG) information */
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
	/**
	 * @brief Channel connected callback
	 *
	 * If this callback is provided it will be called whenever the connection completes.
	 *
	 * For a peripheral, the QoS values (see @ref bt_iso_chan_io_qos)
	 * are set when this is called. The peripheral does not have any
	 * information about the RTN though.
	 *
	 * @param chan The channel that has been connected
	 */
	void (*connected)(struct bt_iso_chan *chan);

	/**
	 * @brief Channel disconnected callback
	 *
	 * If this callback is provided it will be called whenever the
	 * channel is disconnected, including when a connection gets
	 * rejected or when setting security fails.
	 *
	 * @param chan   The channel that has been Disconnected
	 * @param reason BT_HCI_ERR_* reason for the disconnection.
	 */
	void (*disconnected)(struct bt_iso_chan *chan, uint8_t reason);

	/**
	 * @brief Channel alloc_buf callback
	 *
	 * If this callback is provided the channel will use it to allocate
	 * buffers to store incoming data.
	 *
	 * @param chan The channel requesting a buffer.
	 *
	 * @return Allocated buffer.
	 */
	struct net_buf *(*alloc_buf)(struct bt_iso_chan *chan);

	/**
	 * @brief Channel recv callback
	 *
	 * @param chan The channel receiving data.
	 * @param buf Buffer containing incoming data.
	 * @param info Pointer to the metadata for the buffer. The lifetime of the
	 *             pointer is linked to the lifetime of the net_buf.
	 *             Metadata such as sequence number and timestamp can be
	 *             provided by the bluetooth controller.
	 */
	void (*recv)(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
		     struct net_buf *buf);

	/**
	 * @brief Channel sent callback
	 *
	 * This callback will be called once the controller marks the SDU
	 * as completed. When the controller does so is implementation
	 * dependent. It could be after the SDU is enqueued for transmission,
	 * or after it is sent on air or flushed.
	 *
	 * @param chan The channel which has sent data.
	 */
	void (*sent)(struct bt_iso_chan *chan);
};

/** @brief ISO Accept Info Structure */
struct bt_iso_accept_info {
	/** The ACL connection that is requesting authorization */
	struct bt_conn *acl;

	/**
	 * @brief The ID of the connected isochronous group (CIG) on the central
	 *
	 * The ID is unique per ACL
	 */
	uint8_t cig_id;

	/**
	 * @brief The ID of the connected isochronous stream (CIS) on the central
	 *
	 * This ID is unique within a CIG
	 */
	uint8_t cis_id;
};

/** @brief ISO Server structure. */
struct bt_iso_server {
#if defined(CONFIG_BT_SMP) || defined(__DOXYGEN__)
	/**
	 * @brief Required minimum security level.
	 *
	 * Only available when @kconfig{CONFIG_BT_SMP} is enabled.
	 */
	bt_security_t		sec_level;
#endif /* CONFIG_BT_SMP */

	/**
	 * @brief Server accept callback
	 *
	 * This callback is called whenever a new incoming connection requires authorization.
	 *
	 * @param info The ISO accept information structure
	 * @param chan Pointer to receive the allocated channel
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*accept)(const struct bt_iso_accept_info *info, struct bt_iso_chan **chan);
};

/**
 * @brief Register ISO server.
 *
 * Register ISO server, each new connection is authorized using the accept()
 * callback which in case of success shall allocate the channel structure
 * to be used by the new connection.
 *
 * @param server Server structure.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_iso_server_register(struct bt_iso_server *server);

/**
 * @brief Unregister ISO server.
 *
 * Unregister previously registered ISO server.
 *
 * @param server Server structure.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_iso_server_unregister(struct bt_iso_server *server);

/**
 * @brief Creates a CIG as a central
 *
 * This can called at any time, even before connecting to a remote device.
 * This must be called before any connected isochronous stream (CIS) channel can be connected.
 *
 * Once a CIG is created, the channels supplied in the @p param can be
 * connected using bt_iso_chan_connect().
 *
 * @param[in]  param    The parameters used to create and enable the CIG.
 * @param[out] out_cig  Connected Isochronous Group object on success.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_iso_cig_create(const struct bt_iso_cig_param *param, struct bt_iso_cig **out_cig);

/**
 * @brief Reconfigure a CIG as a central
 *
 * This function can be used to update a CIG. It will update the group specific
 * parameters, and, if supplied, change the QoS parameters of the individual
 * CIS. If the cis_channels in @p param contains CIS that was not originally
 * in the call to bt_iso_cig_create(), these will be added to the group.
 * It is not possible to remove any CIS from the group after creation.
 *
 * This can be called at any time before connecting an ISO to a remote device.
 * Once any CIS in the group has connected, the group cannot be changed.
 *
 * Once a CIG is created, the channels supplied in the @p param can be
 * connected using bt_iso_chan_connect().
 *
 * @param cig       Connected Isochronous Group object.
 * @param param     The parameters used to reconfigure the CIG.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_iso_cig_reconfigure(struct bt_iso_cig *cig, const struct bt_iso_cig_param *param);

/**
 * @brief Terminates a CIG as a central
 *
 * All the CIS in the CIG shall be disconnected first.
 *
 * @param cig    Pointer to the CIG structure.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_iso_cig_terminate(struct bt_iso_cig *cig);

/**
 * @brief Connect ISO channels on ACL connections
 *
 * Connect ISO channels. The ISO channels must have been initialized in a CIG
 * first by calling bt_iso_cig_create().
 *
 * Once the connection is completed the channels' connected() callback will be
 * called. If the connection is rejected disconnected() callback is called
 * instead.
 *
 * This function will also setup the ISO data path based on the @p path
 * parameter of the @ref bt_iso_chan_io_qos for each channel.
 *
 * @param param Pointer to a connect parameter array with the ISO and ACL pointers.
 * @param count Number of connect parameters.
 *
 * @retval 0 Successfully started the connecting procedure.
 *
 * @retval -EINVAL Invalid parameters were supplied.
 *
 * @retval -EBUSY Some ISO channels are already being connected.
 *         It is not possible to have multiple outstanding connection requests.
 *         May also be returned if @kconfig{CONFIG_BT_SMP} is enabled and a
 *         pairing procedure is already in progress.
 *
 * @retval -ENOBUFS Not buffers available to send request to controller or if
 *         @kconfig{CONFIG_BT_SMP} is enabled and no more keys could be stored.
 *
 * @retval -ENOMEM If @kconfig{CONFIG_BT_SMP} is enabled and no more keys
 *         could be stored.
 *
 * @retval -EIO Controller rejected the request or if @kconfig{CONFIG_BT_SMP}
 *         is enabled and pairing has timed out.
 *
 * @retval -ENOTCONN If @kconfig{CONFIG_BT_SMP} is enabled the ACL is not
 *         connected.
 */
int bt_iso_chan_connect(const struct bt_iso_connect_param *param, size_t count);

/**
 * @brief Disconnect connected ISO channel
 *
 * Disconnect connected ISO channel.
 *
 * If the device is a central and the connection is pending it will be
 * canceled and as a result the channel bt_iso_chan_ops.disconnected() callback is called.
 *
 * If the device is a peripheral and the connection is pending it will be rejected, as a peripheral
 * shall wait for a CIS Established event (which may trigger a bt_iso_chan_ops.disconnected()
 * callback in case of an error).
 *
 * Regarding to input parameter, to get details see reference description
 * to bt_iso_chan_connect() API.
 *
 * @param chan Channel object.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_iso_chan_disconnect(struct bt_iso_chan *chan);

/**
 * @brief Send data to ISO channel without timestamp
 *
 * Send data from buffer to the channel. If credits are not available, buf will
 * be queued and sent as and when credits are received from peer.
 * Regarding to first input parameter, to get details see reference description
 * to bt_iso_chan_connect() API.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in
 * case of an error the caller retains the ownership of the buffer.
 *
 * @param chan     Channel object.
 * @param buf      Buffer containing data to be sent.
 * @param seq_num  Packet Sequence number. This value shall be incremented for
 *                 each call to this function and at least once per SDU
 *                 interval for a specific channel.
 *
 * @return Number of octets sent in case of success or negative value in case of error.
 */
int bt_iso_chan_send(struct bt_iso_chan *chan, struct net_buf *buf, uint16_t seq_num);

/**
 * @brief Send data to ISO channel with timestamp
 *
 * Send data from buffer to the channel. If credits are not available, buf will
 * be queued and sent as and when credits are received from peer.
 * Regarding to first input parameter, to get details see reference description
 * to bt_iso_chan_connect() API.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in
 * case of an error the caller retains the ownership of the buffer.
 *
 * @param chan     Channel object.
 * @param buf      Buffer containing data to be sent.
 * @param seq_num  Packet Sequence number. This value shall be incremented for
 *                 each call to this function and at least once per SDU
 *                 interval for a specific channel.
 * @param ts       Timestamp of the SDU in microseconds (us).
 *                 This value can be used to transmit multiple
 *                 SDUs in the same SDU interval in a CIG or BIG.
 *
 * @return Number of octets sent in case of success or negative value in case of error.
 */
int bt_iso_chan_send_ts(struct bt_iso_chan *chan, struct net_buf *buf, uint16_t seq_num,
			uint32_t ts);

/** @brief ISO Unicast TX Info Structure */
struct bt_iso_unicast_tx_info {
	/** The transport latency in us */
	uint32_t latency;

	/** The flush timeout (N * 1.25 ms) */
	uint32_t flush_timeout;

	/** The maximum PDU size in octets */
	uint16_t max_pdu;

	/** The transport PHY  */
	uint8_t  phy;

	/** The burst number */
	uint8_t  bn;
};

/** @brief ISO Unicast Info Structure */
struct bt_iso_unicast_info {
	/** The maximum time in us for all PDUs of all CIS in a CIG event */
	uint32_t cig_sync_delay;

	/** The maximum time in us for all PDUs of this CIS in a CIG event */
	uint32_t cis_sync_delay;

	/** @brief TX information for the central to peripheral data path */
	struct bt_iso_unicast_tx_info central;

	/** TX information for the peripheral to central data */
	struct bt_iso_unicast_tx_info peripheral;
};

/** @brief ISO Broadcaster Info Structure */
struct bt_iso_broadcaster_info {
	/** The maximum time in us for all PDUs of all BIS in a BIG event */
	uint32_t sync_delay;

	/** The transport latency in us */
	uint32_t latency;

	/** Pre-transmission offset (N * 1.25 ms) */
	uint32_t  pto;

	/** The maximum PDU size in octets */
	uint16_t max_pdu;

	/** The transport PHY  */
	uint8_t  phy;

	/** The burst number */
	uint8_t  bn;

	/** Number of times a payload is transmitted in a BIS event */
	uint8_t  irc;
};

/** @brief ISO Synchronized Receiver Info Structure */
struct bt_iso_sync_receiver_info {
	/** The transport latency in us */
	uint32_t latency;

	/** Pre-transmission offset (N * 1.25 ms) */
	uint32_t  pto;

	/** The maximum PDU size in octets */
	uint16_t max_pdu;

	/** The burst number */
	uint8_t  bn;

	/** Number of times a payload is transmitted in a BIS event */
	uint8_t  irc;
};

/** ISO channel Info Structure */
struct bt_iso_info {
	/** Channel Type. */
	enum bt_iso_chan_type type;

	/** The ISO interval (N * 1.25 ms) */
	uint16_t iso_interval;

	/** The maximum number of subevents in each ISO event */
	uint8_t  max_subevent;

	/**
	 * @brief True if the channel is able to send data
	 *
	 * This is always true when @p type is @ref BT_ISO_CHAN_TYPE_BROADCASTER,
	 * and never true when @p type is @ref BT_ISO_CHAN_TYPE_SYNC_RECEIVER.
	 */
	bool can_send;

	/**
	 * @brief True if the channel is able to recv data
	 *
	 * This is always true when @p type is @ref BT_ISO_CHAN_TYPE_SYNC_RECEIVER,
	 * and never true when @p type is @ref BT_ISO_CHAN_TYPE_BROADCASTER.
	 */
	bool can_recv;

	/** Connection Type specific Info.*/
	union {
#if defined(CONFIG_BT_ISO_UNICAST) || defined(__DOXYGEN__)
		/** Unicast specific Info.
		 * Only available when @kconfig{CONFIG_BT_ISO_UNICAST} is enabled.
		 */
		struct bt_iso_unicast_info unicast;
#endif /* CONFIG_BT_ISO_UNICAST */
#if defined(CONFIG_BT_ISO_BROADCASTER) || defined(__DOXYGEN__)
		/** Broadcaster specific Info.
		 * Only available when @kconfig{CONFIG_BT_ISO_BROADCASTER} is enabled.
		 */
		struct bt_iso_broadcaster_info broadcaster;
#endif /* CONFIG_BT_ISO_BROADCASTER */
#if defined(CONFIG_BT_ISO_SYNC_RECEIVER) || defined(__DOXYGEN__)
		/** Sync receiver specific Info.
		 * Only available when @kconfig{CONFIG_BT_ISO_SYNC_RECEIVER} is enabled.
		 */
		struct bt_iso_sync_receiver_info sync_receiver;
#endif /* CONFIG_BT_ISO_SYNC_RECEIVER */
	};
};

/**
 * @brief Get ISO channel info
 *
 * @param chan Channel object.
 * @param info Channel info object.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_iso_chan_get_info(const struct bt_iso_chan *chan, struct bt_iso_info *info);

/**
 * @brief Get ISO transmission timing info
 *
 * @details Reads timing information for transmitted ISO packet on an ISO channel.
 *          The HCI_LE_Read_ISO_TX_Sync HCI command is used to retrieve this information
 *          from the controller.
 *
 * @note An SDU must have already been successfully transmitted on the ISO channel
 *       for this function to return successfully.
 *
 * @param[in]  chan Channel object.
 * @param[out] info Transmit info object.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_iso_chan_get_tx_sync(const struct bt_iso_chan *chan, struct bt_iso_tx_info *info);

/**
 * @brief Struct to hold the Broadcast Isochronous Group callbacks
 *
 * These can be registered for usage with bt_iso_big_register_cb().
 */
struct bt_iso_big_cb {
	/**
	 * @brief The BIG has started and all of the streams are ready for data
	 *
	 * @param big The started BIG
	 */
	void (*started)(struct bt_iso_big *big);

	/**
	 * @brief The BIG has stopped and none of the streams are ready for data
	 *
	 * @param big The stopped BIG
	 * @param reason The reason why the BIG stopped (see the BT_HCI_ERR_* values)
	 */
	void (*stopped)(struct bt_iso_big *big, uint8_t reason);

	/** @internal Internally used field for list handling */
	sys_snode_t _node;
};

/**
 * @brief Registers callbacks for Broadcast Sources
 *
 * @param cb Pointer to the callback structure.
 *
 * @retval 0 on success
 * @retval -EINVAL if @p cb is NULL
 * @retval -EEXIST if @p cb is already registered
 */
int bt_iso_big_register_cb(struct bt_iso_big_cb *cb);

/**
 * @brief Creates a BIG as a broadcaster
 *
 * @param[in] padv      Pointer to the periodic advertising object the BIGInfo shall be sent on.
 * @param[in] param     The parameters used to create and enable the BIG. The QOS parameters are
 *                      determined by the QOS field of the first BIS in the BIS list of this
 *                      parameter.
 * @param[out] out_big  Broadcast Isochronous Group object on success.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_iso_big_create(struct bt_le_ext_adv *padv, struct bt_iso_big_create_param *param,
		      struct bt_iso_big **out_big);

/**
 * @brief Terminates a BIG as a broadcaster or receiver
 *
 * @param big    Pointer to the BIG structure.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_iso_big_terminate(struct bt_iso_big *big);

/**
 * @brief Creates a BIG as a receiver
 *
 * @param[in] sync     Pointer to the periodic advertising sync object the BIGInfo was received on.
 * @param[in] param    The parameters used to create and enable the BIG sync.
 * @param[out] out_big Broadcast Isochronous Group object on success.
 *
 * @return 0 in case of success or negative value in case of error.
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
