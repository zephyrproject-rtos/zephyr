/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * IPC link protocol core: builds the frames to send and processes the frames received, without
 * any I/O. The transport decides when a frame is exchanged. The backend serializes all calls on
 * a core. Internal to the IPC link backend, not a public API.
 *
 * Frame layout, all fields little-endian:
 *
 *   Header, IPC_LINK_HDR_SIZE bytes, covered by its own CRC
 *     0  ver      Protocol version
 *     1  flags    IPC_LINK_FLAG_*
 *     2  seq      Sequence number of the payload, when len > 0
 *     3  ack      Next sequence number expected from the peer
 *     4  credits  Free receive buffers of the sender
 *     5  rsvd
 *     6  len      Payload length, 0 for a frame without payload
 *     8  mtu      Frame size of the sender
 *    10  hcrc     CRC-16 over bytes 0 to 9
 *
 *   Payload, len bytes, followed by a CRC-16: messages back to back, each aligned to
 *   IPC_LINK_MSG_ALIGN bytes
 *     0  addr     Endpoint address, IPC_LINK_CTRL_ADDR for the control channel
 *     1  flags    Reserved
 *     2  len      Data length
 *     4  data
 *
 * Both CRCs are CRC-16/MCRF4XX: polynomial 0x1021 reflected, initial value 0xFFFF, no final
 * XOR. Time is in milliseconds.
 */

#ifndef ZEPHYR_SUBSYS_IPC_IPC_SERVICE_LIB_IPC_LINK_CORE_H_
#define ZEPHYR_SUBSYS_IPC_IPC_SERVICE_LIB_IPC_LINK_CORE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

/* The counters are public */
#include <zephyr/ipc/ipc_link.h>

#define IPC_LINK_VERSION 1U

#define IPC_LINK_HDR_SIZE     12U
#define IPC_LINK_PCRC_SIZE    2U
#define IPC_LINK_MSG_HDR_SIZE 4U
#define IPC_LINK_MSG_ALIGN    4U

#define IPC_LINK_CTRL_ADDR 0U

/* Header flags */
#define IPC_LINK_FLAG_BOOT     BIT(0)
#define IPC_LINK_FLAG_BOOT_ACK BIT(1)
#define IPC_LINK_FLAG_HDR_ERR  BIT(2)
/* The sender waits for credits and asks for a header back */
#define IPC_LINK_FLAG_POLL     BIT(3)

#define IPC_LINK_WINDOW_MAX  8U
#define IPC_LINK_RX_BUFS_MAX 8U

/* Transmit slots: the window and the frame being filled */
#define IPC_LINK_TX_SLOTS(window) ((window) + 1U)
/* The slots and the buffer of frames without a payload */
#define IPC_LINK_TX_BUFS(window)  (IPC_LINK_TX_SLOTS(window) + 1U)

/* Smallest frame holding a header, one message header and a payload CRC, aligned */
#define IPC_LINK_MTU_MIN                                                                           \
	ROUND_UP(IPC_LINK_HDR_SIZE + IPC_LINK_MSG_HDR_SIZE + IPC_LINK_PCRC_SIZE, IPC_LINK_MSG_ALIGN)

/* The messages end at a multiple of the alignment, the CRC follows them */
#define IPC_LINK_PAYLOAD_MAX(mtu)                                                                  \
	ROUND_DOWN((mtu) - IPC_LINK_HDR_SIZE - IPC_LINK_PCRC_SIZE, IPC_LINK_MSG_ALIGN)
#define IPC_LINK_MSG_LEN_MAX(mtu) (IPC_LINK_PAYLOAD_MAX(mtu) - IPC_LINK_MSG_HDR_SIZE)

/* Session state */
/* Announcing a boot, until the peer acknowledges it */
#define IPC_LINK_STATE_BOOTING      BIT(0)
/* An announcement went out, at boot_sent_at */
#define IPC_LINK_STATE_BOOT_SENT    BIT(1)
/* The last header announced the peer's boot */
#define IPC_LINK_STATE_PEER_BOOTING BIT(2)
#define IPC_LINK_STATE_CONNECTED    BIT(3)

/* What the next header carries */
#define IPC_LINK_OWED_BOOT_ACK BIT(0)
/* The last frame received had no valid header: the peer sends its header again */
#define IPC_LINK_OWED_HDR_ERR  BIT(1)
#define IPC_LINK_OWED_ACK      BIT(2)
#define IPC_LINK_OWED_CREDITS  BIT(3)
#define IPC_LINK_OWED_POLL     BIT(4)

/* Results of ipc_link_core_rx_check() */
#define IPC_LINK_RX_HDR_OK     BIT(0)
/* The payload fits the frame and its CRC matches */
#define IPC_LINK_RX_PAYLOAD_OK BIT(1)
/* The messages fill the payload exactly */
#define IPC_LINK_RX_LAYOUT_OK  BIT(2)

/* Events returned by ipc_link_core_rx() and ipc_link_core_check() */
#define IPC_LINK_CORE_EVT_RX            BIT(0)
#define IPC_LINK_CORE_EVT_CONNECTED     BIT(1)
/* The peer runs another protocol version or frame size */
#define IPC_LINK_CORE_EVT_PEER_MISMATCH BIT(3)
/* The frame had no valid header */
#define IPC_LINK_CORE_EVT_HDR_ERR       BIT(4)
/* The session ended: too many retransmissions, or the peer came back incompatible */
#define IPC_LINK_CORE_EVT_DISCONNECTED  BIT(5)

struct ipc_link_msg {
	uint8_t addr;
	uint16_t len;
	const uint8_t *data;
};

struct ipc_link_core_cfg {
	/* IPC_LINK_TX_BUFS(window) buffers of mtu bytes */
	uint8_t *tx_bufs;
	/* rx_count buffers of mtu bytes */
	uint8_t *rx_bufs;
	/* Frame size, at least IPC_LINK_MTU_MIN and a multiple of IPC_LINK_MSG_ALIGN, so that the
	 * frame buffers and the messages in them stay aligned
	 */
	uint16_t mtu;
	/* Number of receive buffers, 2 to IPC_LINK_RX_BUFS_MAX */
	uint8_t rx_count;
	/* Payloads in flight, 1 to IPC_LINK_WINDOW_MAX */
	uint8_t window;
	/* Time without acknowledgment before a payload is sent again, also the interval of the
	 * boot announcements and of the polls for credits
	 */
	uint32_t retx_timeout;
	/* Timeouts in a row without an answer from the peer, while a payload waits for its
	 * acknowledgment or for credits, before the session ends. 0 for no limit.
	 */
	uint8_t max_retx;
};

struct ipc_link_core {
	struct ipc_link_core_cfg cfg;

	/* Transmit window: base is the oldest unacknowledged sequence number, next the one to send
	 * and head the one being filled
	 */
	uint8_t base;
	uint8_t next;
	uint8_t head;
	uint8_t base_slot;
	uint16_t fill_len;
	/* CRC of the payload being filled, so far */
	uint16_t fill_crc;
	uint16_t tx_len[IPC_LINK_TX_SLOTS(IPC_LINK_WINDOW_MAX)];
	/* When the slot was last sent, or found lost */
	uint32_t sent_at[IPC_LINK_TX_SLOTS(IPC_LINK_WINDOW_MAX)];
	/* Slots never sent */
	uint16_t slot_fresh;
	uint8_t peer_credits;
	/* Timeouts in a row without an answer from the peer */
	uint8_t retx_count;
	/* Since when a payload waits for the transport to send it, while send_timed */
	uint32_t send_since;
	bool send_timed;

	uint8_t expected;
	uint8_t rx_armed;
	uint8_t rx_free;
	uint8_t credits_sent;

	/* IPC_LINK_STATE_* */
	uint8_t state;
	/* IPC_LINK_OWED_* */
	uint8_t owed;

	uint32_t session;
	uint32_t boot_sent_at;
	uint32_t poll_sent_at;

#ifdef CONFIG_IPC_SERVICE_LINK_STATS
	struct ipc_link_stats stats;
#endif
};

/*
 * Initialize a core, which starts by announcing its boot.
 *
 * @param core Core
 * @param cfg Configuration, copied
 */
void ipc_link_core_init(struct ipc_link_core *core, const struct ipc_link_core_cfg *cfg);

/*
 * Whether a frame is to be sent: data, an acknowledgment or credits to give, a boot
 * announcement due, or a header to receive again after a bad one.
 *
 * @param core Core
 * @param now Current time
 */
bool ipc_link_core_pending(const struct ipc_link_core *core, uint32_t now);

/*
 * Build the next frame to send.
 *
 * @param core Core
 * @param now Current time
 * @param len Set to the frame length
 *
 * @return The frame, valid until the next call
 */
const uint8_t *ipc_link_core_tx_prepare(struct ipc_link_core *core, uint32_t now, size_t *len);

/* @return The buffer the next frame is received into, mtu bytes */
uint8_t *ipc_link_core_rx_buf(struct ipc_link_core *core);

/*
 * Check the frame in the receive buffer: the CRCs and the lengths. Reads only the buffer, which
 * the transport owns until ipc_link_core_rx(), so it needs no serialization with the other calls.
 *
 * @param core Core
 * @param len Received length
 * @param ok False if the transfer failed
 *
 * @return IPC_LINK_RX_* flags, for ipc_link_core_rx()
 */
uint8_t ipc_link_core_rx_check(const struct ipc_link_core *core, size_t len, bool ok);

/*
 * Process the frame in the receive buffer.
 *
 * @param core Core
 * @param checked The result of ipc_link_core_rx_check()
 * @param now Current time
 * @param rx_idx Set to the delivered receive buffer with IPC_LINK_CORE_EVT_RX, which stays owned
 *               by the caller until ipc_link_core_rx_release()
 *
 * @return IPC_LINK_CORE_EVT_* flags
 */
uint32_t ipc_link_core_rx(struct ipc_link_core *core, uint8_t checked, uint32_t now,
			  uint8_t *rx_idx);

/*
 * Time the link: send again the payloads that waited too long for an acknowledgment, and ask
 * the peer for a header when payloads wait for its credits, as the header carrying them may have
 * been lost. Every timeout without an answer counts towards the limit that ends the session,
 * also one in which the transport could not send what waited.
 *
 * @param core Core
 * @param now Current time
 *
 * @return IPC_LINK_CORE_EVT_* flags
 */
uint32_t ipc_link_core_check(struct ipc_link_core *core, uint32_t now);

/*
 * How long until ipc_link_core_check() has something to do: the core waits for the peer's boot
 * acknowledgment, for an acknowledgment or for credits, or a payload waits for the transport to
 * send it, which this call starts timing.
 *
 * @param core Core
 * @param now Current time
 *
 * @return The time until the next timeout, UINT32_MAX if nothing is waited for. A boot
 *         announcement due is pending rather than timed.
 */
uint32_t ipc_link_core_timeout(struct ipc_link_core *core, uint32_t now);

/*
 * Get the next message of a delivered receive buffer.
 *
 * @param core Core
 * @param rx_idx Receive buffer
 * @param offset Start at 0, advanced past the returned message
 * @param msg Filled with the message
 *
 * @return False when there are no more messages
 */
bool ipc_link_core_msg_next(const struct ipc_link_core *core, uint8_t rx_idx, size_t *offset,
			    struct ipc_link_msg *msg);

/* @return The receive buffer holding a delivered message, or -1 */
int ipc_link_core_rx_idx(const struct ipc_link_core *core, const void *ptr);

/* Give a delivered receive buffer back. */
void ipc_link_core_rx_release(struct ipc_link_core *core, uint8_t rx_idx);

/*
 * Queue a message.
 *
 * @retval 0 on success
 * @retval -ENOTCONN if no session started yet
 * @retval -EMSGSIZE if @p len is too large
 * @retval -EAGAIN if the transmit buffers are full
 */
int ipc_link_core_send(struct ipc_link_core *core, uint8_t addr, const void *data, uint16_t len);

/* @return The largest message length */
uint16_t ipc_link_core_max_msg_len(const struct ipc_link_core *core);

/*
 * Whether ipc_link_core_send() takes a message of @p len bytes now, rather than after an
 * acknowledgment frees the transmit buffers.
 */
bool ipc_link_core_has_room(const struct ipc_link_core *core, uint16_t len);

/* @return Whether a session is established */
bool ipc_link_core_connected(const struct ipc_link_core *core);

/* @return Whether the core announces its boot, waiting for the peer to acknowledge it */
bool ipc_link_core_booting(const struct ipc_link_core *core);

#endif /* ZEPHYR_SUBSYS_IPC_IPC_SERVICE_LIB_IPC_LINK_CORE_H_ */
