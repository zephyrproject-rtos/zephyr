/** @file
 *  @brief Bluetooth RFCOMM handling
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_RFCOMM_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_RFCOMM_H_

/**
 * @brief RFCOMM
 * @defgroup bt_rfcomm RFCOMM
 * @since 1.6
 * @version 0.1.0
 * @ingroup bluetooth
 * @{
 */

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/** RFCOMM Maximum Header Size. The length could be 2 bytes, it depends on information length. */
#define BT_RFCOMM_HDR_MAX_SIZE 4
/** RFCOMM FCS Size */
#define BT_RFCOMM_FCS_SIZE     1
/** RFCOMM Credits Size */
#define BT_RFCOMM_CREDITS_SIZE 1

/** @brief RFCOMM Overhead Size
 *
 * The overhead size of RFCOMM includes the maximum header size, FCS size, and credits size.
 *
 * For the field credits size, in the CFC supported case, the space of credits should be discounted
 * from the maximum frame size. It is used to avoid the SDU length exceeding the maximum frame size
 * if the credits field is included.
 */
#define BT_RFCOMM_OVERHEAD_SIZE                                                                    \
	(BT_RFCOMM_HDR_MAX_SIZE + BT_RFCOMM_FCS_SIZE + BT_RFCOMM_CREDITS_SIZE)

/** @brief Helper to calculate needed buffer size for RFCOMM PDUs.
 *         Useful for creating buffer pools.
 *
 *  @param mtu Needed RFCOMM PDU MTU.
 *
 *  @return Needed buffer size to match the requested RFCOMM PDU MTU.
 */
#define BT_RFCOMM_BUF_SIZE(mtu) BT_L2CAP_BUF_SIZE(BT_RFCOMM_OVERHEAD_SIZE + (mtu))

/* RFCOMM channels (1-30): pre-allocated for profiles to avoid conflicts */
enum {
	BT_RFCOMM_CHAN_HFP_HF = 1,
	BT_RFCOMM_CHAN_HFP_AG,
	BT_RFCOMM_CHAN_HSP_AG,
	BT_RFCOMM_CHAN_HSP_HS,
	BT_RFCOMM_CHAN_SPP,
	BT_RFCOMM_CHAN_DYNAMIC_START,
};

struct bt_rfcomm_dlc;

/** @brief RFCOMM DLC operations structure.
 *
 * The object has to stay valid and constant for the lifetime of the DLC.
 *
 *  @note The callbacks are invoked from a thread context, never from an
 *        ISR. Whether a callback is invoked from a context internal to
 *        the stack or synchronously from within the API call that
 *        triggers it, and from which context, is not part of the API and
 *        may change between releases. See
 *        @rstref{Callback execution contexts <bluetooth_callback_contexts>}
 *        for the hazards of blocking in a callback and their mitigations.
 */
struct bt_rfcomm_dlc_ops {
	/** DLC connected callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  connection completes.
	 *
	 *  @param dlc The dlc that has been connected
	 */
	void (*connected)(struct bt_rfcomm_dlc *dlc);

	/** DLC disconnected callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  dlc is disconnected, including when a connection gets
	 *  rejected or cancelled (both incoming and outgoing)
	 *
	 *  @param dlc The dlc that has been Disconnected
	 */
	void (*disconnected)(struct bt_rfcomm_dlc *dlc);

	/** DLC recv callback
	 *
	 *  Called whenever data is received on the DLC.
	 *
	 *  If processing @p buf requires work that cannot complete immediately (e.g. passing data
	 *  to another thread or a work queue), return @c -EINPROGRESS to defer the completion. The
	 *  callback must not block in this case. Then the stack will hold back RX flow-control
	 *  credits (CFC) or assert FC=1 (non-CFC) until @ref bt_rfcomm_dlc_recv_complete is called
	 *  for every outstanding buffer.
	 *
	 *  When @c -EINPROGRESS is returned, the application takes ownership of the @p buf
	 *  reference and must eventually call @ref bt_rfcomm_dlc_recv_complete exactly once,
	 *  passing back the same @p buf pointer. No @ref net_buf_ref or @ref net_buf_unref calls
	 *  are needed around that hand-off.
	 *
	 *  @warning @ref bt_rfcomm_dlc_recv_complete must be called from a thread context
	 *           <b>after this callback has returned</b>. Calling it from inside the recv
	 *           callback (before it returns) will underflow the in-progress counter and
	 *           cause @ref bt_rfcomm_dlc_recv_complete to fail with @c -EINVAL.
	 *
	 *  @param dlc The dlc receiving data.
	 *  @param buf Buffer containing incoming data.
	 *
	 *  @retval 0 Data was fully consumed synchronously; the stack retains ownership of @p buf.
	 *  @retval -EINPROGRESS Data processing is deferred; the application now owns @p buf and
	 *                       must call @ref bt_rfcomm_dlc_recv_complete once processing is done.
	 *  @return Other negative error code; the DLC will be actively disconnected by the stack.
	 */
	int (*recv)(struct bt_rfcomm_dlc *dlc, struct net_buf *buf);

	/** DLC sent callback
	 *
	 *  @param dlc The dlc which has sent data.
	 *  @param err Sent result.
	 */
	void (*sent)(struct bt_rfcomm_dlc *dlc, int err);
};

/** @brief Role of RFCOMM session and dlc. Used only by internal APIs
 */
typedef enum bt_rfcomm_role {
	BT_RFCOMM_ROLE_ACCEPTOR,
	BT_RFCOMM_ROLE_INITIATOR
} __packed bt_rfcomm_role_t;

/** @brief RFCOMM DLC structure. */
struct bt_rfcomm_dlc {
	/** @cond INTERNAL_HIDDEN */

	/** Response Timeout eXpired (RTX) timer.
	 *
	 *  Used to detect when a peer fails to respond to an RFCOMM command
	 *  (e.g. SABM, DISC) within the allowed time window.
	 */
	struct k_work_delayable rtx_work;

	/** Queue for outgoing data.
	 *
	 *  Holds net_buf fragments waiting to be transmitted over this DLC.
	 *  Frames are dequeued and sent by @p tx_work.
	 */
	struct k_fifo tx_queue;

	/** TX credits semaphore.
	 *
	 *  When Credit-Based Flow Control (CFC) is active this semaphore counts
	 *  the number of frames the remote peer is willing to receive. When CFC
	 *  is not negotiated it is used as a binary semaphore to serialize
	 *  transmissions under aggregate (MSC) flow control.
	 */
	struct k_sem tx_credits;

	/** Worker for RFCOMM TX.
	 *
	 *  Submitted to the bt work queue whenever there are frames in
	 *  @p tx_queue and credits are available, to drain the queue and push
	 *  data to L2CAP.
	 */
	struct k_work tx_work;

	/** Pointer to the RFCOMM session this DLC belongs to. */
	struct bt_rfcomm_session *session;

	/** @endcond */

	/** Pointer to the application callback operations for this DLC. */
	struct bt_rfcomm_dlc_ops *ops;

	/** @cond INTERNAL_HIDDEN */

	/** Internally used field for list handling. */
	sys_snode_t _node;

	/** @endcond */

	/** Minimum security level required before this DLC may be established. */
	bt_security_t required_sec_level;

	/** @cond INTERNAL_HIDDEN */

	/** Role of this DLC: initiator or acceptor. */
	bt_rfcomm_role_t role;

	/** @endcond */

	/** Maximum Transmission Unit for this DLC in bytes.
	 *
	 *  Negotiated during the Parameter Negotiation (PN) procedure.
	 *  Outgoing @ref bt_rfcomm_dlc_send buffers must not exceed this value.
	 */
	uint16_t mtu;

	/** @cond INTERNAL_HIDDEN */

	/** Data Link Connection Identifier assigned to this DLC. */
	uint8_t dlci;

	/** Current connection state of this DLC.
	 *
	 *  One of the BT_RFCOMM_STATE_* values defined in rfcomm_internal.h.
	 */
	uint8_t state;

	/** Number of receive credits remaining for this DLC.
	 *
	 *  Tracks how many additional UIH frames the local side may accept from the remote
	 *  before flow control must be applied.
	 *  Initialized from @p rx_credit_limit at connection setup and decremented as frames are
	 *  received; refilled by sending credit grants to the remote.
	 *
	 *  Only meaningful when CFC is enabled.
	 */
	uint8_t rx_credit;

	/** @endcond */

	/** Requested initial number of receive credits for this DLC.
	 *
	 *  Sets how many UIH frames the local side is willing to accept from the remote after the
	 *  DLC is established. The stack limits this value to MIN((BT_BUF_ACL_RX_COUNT - 1), 255)
	 *  before use. If the field is 0, the stack uses the internal default
	 *  MIN((BT_BUF_ACL_RX_COUNT - 1), 255). The result is written back to the field. Also
	 *  the value of field will be limited to 7 when sending in Parameter Negotiation (PN)
	 *  command/response.
	 *
	 *  @note If the value exceeds MIN((BT_BUF_ACL_RX_COUNT - 1), 255), it will be limited to
	 *  MIN((BT_BUF_ACL_RX_COUNT - 1), 255).
	 *  @note This field can only be modified when the DLC is in an unconnected state. This
	 *  means that from the time @ref bt_rfcomm_server::accept returns until @ref
	 *  bt_rfcomm_dlc_ops::disconnected is called, it cannot be modified any more.
	 *
	 *  Only meaningful when CFC is enabled.
	 */
	uint8_t rx_credit_limit;

	/** @cond INTERNAL_HIDDEN */

	/** Number of in-progress receive operations for this DLC.
	 *
	 *  Counts how many @ref bt_rfcomm_dlc_ops::recv callbacks have returned @c -EINPROGRESS
	 *  but have not yet been completed by calling @ref bt_rfcomm_dlc_recv_complete.
	 *
	 *  For CFC-supported sessions, this counter is used to delay RX credit refill:
	 *  RX credits are not refilled until the in-progress bufs are completed, ensuring
	 *  the remote peer cannot send more frames than the application can handle concurrently.
	 *
	 *  For non-CFC sessions, this counter gates the MSC flow-control commands:
	 *  - MSC with FC=1 (pause) is sent only when the count is changing from 0 to 1
	 *  - MSC with FC=0 (resume) is sent only when the count is changing from 1 to 0
	 */
	atomic_t rx_credit_inprogress;

	/** DLC flags */
	atomic_t flags;

	/** @endcond */
};

struct bt_rfcomm_server {
	/** Server Channel
	 *
	 *  Possible values:
	 *  0           A dynamic value will be auto-allocated when bt_rfcomm_server_register() is
	 *              called.
	 *
	 *  0x01 - 0x1e Dynamically allocated. May be pre-set by the application before server
	 *              registration (not recommended however), or auto-allocated by the stack
	 *              if the 0 is passed.
	 */
	uint8_t channel;

	/** Server accept callback
	 *
	 *  This callback is called whenever a new incoming connection requires
	 *  authorization.
	 *
	 *  @param conn The connection that is requesting authorization
	 *  @param server Pointer to the server structure this callback relates to
	 *  @param dlc Pointer to received the allocated dlc
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_rfcomm_server *server,
		      struct bt_rfcomm_dlc **dlc);

	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	/** @endcond */
};

/** @brief RFCOMM RPN baud rate values */
enum {
	BT_RFCOMM_RPN_BAUD_RATE_2400 = 0x0,
	BT_RFCOMM_RPN_BAUD_RATE_4800 = 0x1,
	BT_RFCOMM_RPN_BAUD_RATE_7200 = 0x2,
	BT_RFCOMM_RPN_BAUD_RATE_9600 = 0x3,
	BT_RFCOMM_RPN_BAUD_RATE_19200 = 0x4,
	BT_RFCOMM_RPN_BAUD_RATE_38400 = 0x5,
	BT_RFCOMM_RPN_BAUD_RATE_57600 = 0x6,
	BT_RFCOMM_RPN_BAUD_RATE_115200 = 0x7,
	BT_RFCOMM_RPN_BAUD_RATE_230400 = 0x8
};

/** @brief RFCOMM RPN data bit values */
enum {
	BT_RFCOMM_RPN_DATA_BITS_5 = 0x0,
	BT_RFCOMM_RPN_DATA_BITS_6 = 0x1,
	BT_RFCOMM_RPN_DATA_BITS_7 = 0x2,
	BT_RFCOMM_RPN_DATA_BITS_8 = 0x3
};

/** @brief RFCOMM RPN stop bit values */
enum {
	BT_RFCOMM_RPN_STOP_BITS_1 = 0,
	BT_RFCOMM_RPN_STOP_BITS_1_5 = 1
};

/** @brief RFCOMM RPN parity bit values */
enum {
	BT_RFCOMM_RPN_PARITY_NONE = 0x0,
	BT_RFCOMM_RPN_PARITY_ODD = 0x1,
	BT_RFCOMM_RPN_PARITY_EVEN = 0x3,
	BT_RFCOMM_RPN_PARITY_MARK = 0x5,
	BT_RFCOMM_RPN_PARITY_SPACE = 0x7
};

/** @brief Combine data bits, stop bits and parity into a single line settings byte
 *
 *  @param data Data bits value (0-3)
 *  @param stop Stop bits value (0-1)
 *  @param parity Parity value (0-7)
 *
 *  @return Combined line settings byte
 */
#define BT_RFCOMM_SET_LINE_SETTINGS(data, stop, parity) ((data & 0x3) | \
							 ((stop & 0x1) << 2) | \
							 ((parity & 0x7) << 3))

#define BT_RFCOMM_RPN_FLOW_NONE         0x00
#define BT_RFCOMM_RPN_XON_CHAR          0x11
#define BT_RFCOMM_RPN_XOFF_CHAR         0x13

/* Set 1 to all the param mask except reserved */
#define BT_RFCOMM_RPN_PARAM_MASK_ALL    0x3f7f

/** @brief RFCOMM Remote Port Negotiation (RPN) structure */
struct bt_rfcomm_rpn {
	uint8_t  dlci;
	uint8_t  baud_rate;
	uint8_t  line_settings;
	uint8_t  flow_control;
	uint8_t  xon_char;
	uint8_t  xoff_char;
	uint16_t param_mask;
} __packed;

/** @brief Register RFCOMM server
 *
 *  Register RFCOMM server for a channel, each new connection is authorized
 *  using the accept() callback which in case of success shall allocate the dlc
 *  structure to be used by the new connection.
 *
 *  @param server Server structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_rfcomm_server_register(struct bt_rfcomm_server *server);

/** @brief Unregister RFCOMM server
 *
 *  Unregister RFCOMM server for a channel.
 *
 *  @param server Server structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_rfcomm_server_unregister(struct bt_rfcomm_server *server);

/** @brief Connect RFCOMM channel
 *
 *  Connect RFCOMM dlc by channel, once the connection is completed dlc
 *  connected() callback will be called. If the connection is rejected
 *  disconnected() callback is called instead.
 *
 *  @param conn Connection object.
 *  @param dlc Dlc object.
 *  @param channel Server channel to connect to.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_rfcomm_dlc_connect(struct bt_conn *conn, struct bt_rfcomm_dlc *dlc,
			  uint8_t channel);

/** @brief Send data to RFCOMM
 *
 *  Send data from buffer to the dlc. Length should be less than or equal to
 *  mtu.
 *
 *  @param dlc Dlc object.
 *  @param buf Data buffer.
 *
 *  @return Bytes sent in case of success or negative value in case of error.
 */
int bt_rfcomm_dlc_send(struct bt_rfcomm_dlc *dlc, struct net_buf *buf);

/** @brief Disconnect RFCOMM dlc
 *
 *  Disconnect RFCOMM dlc, if the connection is pending it will be
 *  canceled and as a result the dlc disconnected() callback is called.
 *
 *  @param dlc Dlc object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_rfcomm_dlc_disconnect(struct bt_rfcomm_dlc *dlc);

/**
 * @brief Complete receiving RFCOMM channel data
 *
 * Must be called exactly once for each invocation of @ref bt_rfcomm_dlc_ops::recv that returned
 * @c -EINPROGRESS. Pass back the same @p buf pointer that was received in that callback; no
 * additional @ref net_buf_ref or @ref net_buf_unref calls are needed.
 *
 * @note This function must be called from a thread context only, never from an ISR.
 *
 * @note This function must be called only after @ref bt_rfcomm_dlc_ops::recv has returned.
 *       Calling it from within the recv callback (before it returns) underflows the in-progress
 *       counter and causes this function to return @c -EINVAL.
 *
 * When this function returns, the stack releases its reference to @p buf regardless of the return
 * value, unless @p dlc or @p buf is @c NULL (in which case @c -EINVAL is returned immediately and
 * @p buf is not touched).
 *
 * Flow-control behavior after a successful call:
 * - CFC sessions: the stack may now send additional RX credits to the remote peer, allowing it
 *   to send more frames.
 * - Non-CFC sessions: an MSC command with FC=0 (resume) is sent to the remote peer only when
 *   all in-progress buffers are completed.
 *
 * @param dlc Pointer to the RFCOMM DLC whose receive operation is being completed.
 * @param buf The buffer that was passed to @ref bt_rfcomm_dlc_ops::recv when it returned
 *            @c -EINPROGRESS.
 *
 * @return 0 in case of success or negative value in case of error.
 * @retval -EINVAL @p dlc or @p buf is @c NULL, the DLC's CFC state is unknown, or the
 *                 in-progress counter would underflow (more completions than in-progress calls).
 * @retval -ENOTCONN The DLC has no associated session or is not in the connected state.
 */
int bt_rfcomm_dlc_recv_complete(struct bt_rfcomm_dlc *dlc, struct net_buf *buf);

/** @brief Allocate the buffer from pool after reserving head room for RFCOMM,
 *  L2CAP and ACL headers.
 *
 *  @param pool Which pool to take the buffer from.
 *
 *  @return New buffer.
 */
struct net_buf *bt_rfcomm_create_pdu(struct net_buf_pool *pool);

/**
 * @brief Send Remote Port Negotiation command
 *
 * @param dlc Pointer to the RFCOMM DLC
 * @param rpn Pointer to the RPN parameters to send
 *
 * @return 0 on success, negative error code on failure
 */
int bt_rfcomm_send_rpn_cmd(struct bt_rfcomm_dlc *dlc, struct bt_rfcomm_rpn *rpn);

/** @brief Remote Line Status value: No error */
#define BT_RFCOMM_RLS_NO_ERR (0x00U)

/** @brief Remote Line Status value: error occurred
 *
 *  @param err Error code to be set in the RLS value; must be one of the following values:
 *             @ref BT_RFCOMM_RLS_ERR_OVERRUN_ERROR, @ref BT_RFCOMM_RLS_ERR_PARITY_ERROR, or
 *             @ref BT_RFCOMM_RLS_ERR_FRAMING_ERROR.
 *
 *  @return RLS value with error code set.
 */
#define BT_RFCOMM_RLS_ERR(err) (BIT(0) | (err))

/** @brief Overrun Error - Received character overwrote an unread character */
#define BT_RFCOMM_RLS_ERR_OVERRUN_ERROR BIT(1)

/** @brief Parity Error - Received character's parity was incorrect */
#define BT_RFCOMM_RLS_ERR_PARITY_ERROR BIT(2)

/** @brief Framing Error - a character did not terminate with a stop bit */
#define BT_RFCOMM_RLS_ERR_FRAMING_ERROR BIT(3)

/**
 * @brief Send Remote Line Status Command
 *
 * Send remote line status with specific rls value @p line_status to the RFCOMM DLC.
 * For @p line_status, the BIT(4-7) are reserved and must be set to 0.
 * The BIT(0-3) indicate the Line Status.
 * If the BIT(0) is set to 0, there is no error occurred.
 * If the BIT(0) is set to 1, the error is indicated by BIT(1-3) with the following values:
 * @ref BT_RFCOMM_RLS_ERR_OVERRUN_ERROR - Received character overwrote an unread
 * character.
 * @ref BT_RFCOMM_RLS_ERR_PARITY_ERROR - Received character's parity was incorrect.
 * @ref BT_RFCOMM_RLS_ERR_FRAMING_ERROR - a character did not terminate with a stop bit.
 *
 * @p line_status can be created using @ref BT_RFCOMM_RLS_NO_ERR and @ref BT_RFCOMM_RLS_ERR macros.
 *
 * @param dlc Pointer to the RFCOMM DLC
 * @param line_status Line Status value
 *
 * @return 0 on success, negative error code on failure
 */
int bt_rfcomm_send_rls_cmd(struct bt_rfcomm_dlc *dlc, uint8_t line_status);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_RFCOMM_H_ */
