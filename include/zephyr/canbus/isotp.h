/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for ISO-TP (ISO 15765-2:2016)
 *
 * ISO-TP is a transport protocol for CAN (Controller Area Network)
 */

#ifndef ZEPHYR_INCLUDE_ISOTP_H_
#define ZEPHYR_INCLUDE_ISOTP_H_

/**
 * @brief CAN ISO-TP Interface
 * @defgroup can_isotp CAN ISO-TP Interface
 * @ingroup CAN
 * @{
 */

#include <zephyr/drivers/can.h>
#include <zephyr/types.h>
#include <zephyr/net/buf.h>

/*
 * Abbreviations
 * BS      Block Size
 * CAN_DL  CAN LL data size
 * CF      Consecutive Frame
 * CTS     Continue to send
 * DLC     Data length code
 * FC      Flow Control
 * FF      First Frame
 * FS      Flow Status
 * AE      Address Extension
 * SA      Source Address
 * TA      Target Address
 */

/*
 * N_Result according to ISO 15765-2:2016
 * ISOTP_ prefix is used to be zephyr conform
 */

/** Completed successfully */
#define ISOTP_N_OK              0

/** Ar/As has timed out */
#define ISOTP_N_TIMEOUT_A      -1

/** Reception of next FC has timed out */
#define ISOTP_N_TIMEOUT_BS     -2

/** Cr has timed out */
#define ISOTP_N_TIMEOUT_CR     -3

/** Unexpected sequence number */
#define ISOTP_N_WRONG_SN       -4

/** Invalid flow status received*/
#define ISOTP_N_INVALID_FS     -5

/** Unexpected PDU received */
#define ISOTP_N_UNEXP_PDU      -6

/** Maximum number of WAIT flowStatus PDUs exceeded */
#define ISOTP_N_WFT_OVRN       -7

/** FlowStatus OVFLW PDU was received */
#define ISOTP_N_BUFFER_OVERFLW -8

/** General error */
#define ISOTP_N_ERROR          -9

/** Implementation specific errors */

/** Can't bind or send because the CAN device has no filter left*/
#define ISOTP_NO_FREE_FILTER    -10

/** No net buffer left to allocate */
#define ISOTP_NO_NET_BUF_LEFT   -11

/** Not sufficient space in the buffer left for the data */
#define ISOTP_NO_BUF_DATA_LEFT  -12

/** No context buffer left to allocate */
#define ISOTP_NO_CTX_LEFT       -13

/** Timeout for recv */
#define ISOTP_RECV_TIMEOUT      -14

/*
 * CAN ID filtering for ISO-TP fixed addressing according to SAE J1939
 *
 * Format of 29-bit CAN identifier:
 * ------------------------------------------------------
 * | 28 .. 26 | 25  | 24 | 23 .. 16 | 15 .. 8  | 7 .. 0 |
 * ------------------------------------------------------
 * | Priority | EDP | DP | N_TAtype |   N_TA   |  N_SA  |
 * ------------------------------------------------------
 */

/** Position of fixed source address (SA) */
#define ISOTP_FIXED_ADDR_SA_POS         (0U)

/** Mask to obtain fixed source address (SA) */
#define ISOTP_FIXED_ADDR_SA_MASK        (0xFF << ISOTP_FIXED_ADDR_SA_POS)

/** Position of fixed target address (TA) */
#define ISOTP_FIXED_ADDR_TA_POS         (8U)

/** Mask to obtain fixed target address (TA) */
#define ISOTP_FIXED_ADDR_TA_MASK        (0xFF << ISOTP_FIXED_ADDR_TA_POS)

/** Position of priority in fixed addressing mode */
#define ISOTP_FIXED_ADDR_PRIO_POS       (26U)

/** Mask for priority in fixed addressing mode */
#define ISOTP_FIXED_ADDR_PRIO_MASK      (0x7 << ISOTP_FIXED_ADDR_PRIO_POS)

/* CAN filter RX mask to match any priority and source address (SA) */
#define ISOTP_FIXED_ADDR_RX_MASK        (0x03FFFF00)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ISO-TP message id struct
 *
 * Used to pass addresses to the bind and send functions.
 */
struct isotp_msg_id {
	/**
	 * CAN identifier
	 *
	 * If ISO-TP fixed addressing is used, isotp_bind ignores SA and
	 * priority sections and modifies TA section in flow control frames.
	 */
	union {
		uint32_t std_id  : 11;
		uint32_t ext_id  : 29;
	};
	/** ISO-TP extended address (if used) */
	uint8_t ext_addr;
	/** Indicates the CAN identifier type (0 for standard or 1 for extended) */
	uint8_t ide : 1;
	/** Indicates if ISO-TP extended addressing is used */
	uint8_t use_ext_addr : 1;
	/** Indicates if ISO-TP fixed addressing (acc. to SAE J1939) is used */
	uint8_t use_fixed_addr : 1;
};

/*
 * STmin is split in two valid ranges:
 *   0-127: 0ms-127ms
 * 128-240: Reserved
 * 241-249: 100us-900us (multiples of 100us)
 * 250-   : Reserved
 */

/**
 * @brief ISO-TP frame control options struct
 *
 * Used to pass the options to the bind and send functions.
 */
struct isotp_fc_opts {
	uint8_t bs;    /**< Block size. Number of CF PDUs before next CF is sent */
	uint8_t stmin; /**< Minimum separation time. Min time between frames */
};

typedef void (*isotp_tx_callback_t)(int error_nr, void *arg);

struct isotp_send_ctx;
struct isotp_recv_ctx;

/**
 * @brief Bind an address to a receiving context.
 *
 * This function binds an RX and TX address combination to an RX context.
 * When data arrives from the specified address, it is buffered and can be read
 * by calling isotp_recv.
 * When calling this routine, a filter is applied in the CAN device, and the
 * context is initialized. The context must be valid until calling unbind.
 *
 * @param ctx     Context to store the internal states.
 * @param can_dev The CAN device to be used for sending and receiving.
 * @param rx_addr Identifier for incoming data.
 * @param tx_addr Identifier for FC frames.
 * @param opts    Flow control options.
 * @param timeout Timeout for FF SF buffer allocation.
 *
 * @retval ISOTP_N_OK on success
 * @retval ISOTP_NO_FREE_FILTER if CAN device has no filters left.
 */
int isotp_bind(struct isotp_recv_ctx *ctx, const struct device *can_dev,
	       const struct isotp_msg_id *rx_addr,
	       const struct isotp_msg_id *tx_addr,
	       const struct isotp_fc_opts *opts,
	       k_timeout_t timeout);

/**
 * @brief Unbind a context from the interface
 *
 * This function removes the binding from isotp_bind.
 * The filter is detached from the CAN device, and if a transmission is ongoing,
 * buffers are freed.
 * The context can be discarded safely after calling this function.
 *
 * @param ctx     Context that should be unbound.
 */
void isotp_unbind(struct isotp_recv_ctx *ctx);

/**
 * @brief Read out received data from fifo.
 *
 * This function reads the data from the receive FIFO of the context.
 * It blocks if the FIFO is empty.
 * If an error occurs, the function returns a negative number and leaves the
 * data buffer unchanged.
 *
 * @param ctx     Context that is already bound.
 * @param data    Pointer to a buffer where the data is copied to.
 * @param len     Size of the buffer.
 * @param timeout Timeout for incoming data.
 *
 * @retval Number of bytes copied on success
 * @retval ISOTP_WAIT_TIMEOUT when "timeout" timed out
 * @retval ISOTP_N_* on error
 */
int isotp_recv(struct isotp_recv_ctx *ctx, uint8_t *data, size_t len,
	       k_timeout_t timeout);

/**
 * @brief Get the net buffer on data reception
 *
 * This function reads incoming data into net-buffers.
 * It blocks until the entire packet is received, BS is reached, or an error
 * occurred. If BS was zero, the data is in a single net_buf. Otherwise,
 * the data is fragmented in chunks of BS size.
 * The net-buffers are referenced and must be freed with net_buf_unref after the
 * data is processed.
 *
 * @param ctx     Context that is already bound.
 * @param buffer  Pointer where the net_buf pointer is written to.
 * @param timeout Timeout for incoming data.
 *
 * @retval Remaining data length for this transfer if BS > 0, 0 for BS = 0
 * @retval ISOTP_WAIT_TIMEOUT when "timeout" timed out
 * @retval ISOTP_N_* on error
 */
int isotp_recv_net(struct isotp_recv_ctx *ctx, struct net_buf **buffer,
		   k_timeout_t timeout);

/**
 * @brief Send data
 *
 * This function is used to send data to a peer that listens to the tx_addr.
 * An internal work-queue is used to transfer the segmented data.
 * Data and context must be valid until the transmission has finished.
 * If a complete_cb is given, this function is non-blocking, and the callback
 * is called on completion with the return value as a parameter.
 *
 * @param ctx         Context to store the internal states.
 * @param can_dev     The CAN device to be used for sending and receiving.
 * @param data        Data to be sent.
 * @param len         Length of the data to be sent.
 * @param rx_addr     Identifier for FC frames.
 * @param tx_addr     Identifier for outgoing frames the receiver listens on.
 * @param complete_cb Function called on completion or NULL.
 * @param cb_arg      Argument passed to the complete callback.
 *
 * @retval ISOTP_N_OK on success
 * @retval ISOTP_N_* on error
 */
int isotp_send(struct isotp_send_ctx *ctx, const struct device *can_dev,
	       const uint8_t *data, size_t len,
	       const struct isotp_msg_id *tx_addr,
	       const struct isotp_msg_id *rx_addr,
	       isotp_tx_callback_t complete_cb, void *cb_arg);

#ifdef CONFIG_ISOTP_ENABLE_CONTEXT_BUFFERS
/**
 * @brief Send data with buffered context
 *
 * This function is similar to isotp_send, but the context is automatically
 * allocated from an internal pool.
 *
 * @param can_dev     The CAN device to be used for sending and receiving.
 * @param data        Data to be sent.
 * @param len         Length of the data to be sent.
 * @param rx_addr     Identifier for FC frames.
 * @param tx_addr     Identifier for outgoing frames the receiver listens on.
 * @param complete_cb Function called on completion or NULL.
 * @param cb_arg      Argument passed to the complete callback.
 * @param timeout     Timeout for buffer allocation.
 *
 * @retval ISOTP_N_OK on success
 * @retval ISOTP_N_* on error
 */
int isotp_send_ctx_buf(const struct device *can_dev,
		       const uint8_t *data, size_t len,
		       const struct isotp_msg_id *tx_addr,
		       const struct isotp_msg_id *rx_addr,
		       isotp_tx_callback_t complete_cb, void *cb_arg,
		       k_timeout_t timeout);

/**
 * @brief Send data with buffered context
 *
 * This function is similar to isotp_send_ctx_buf, but the data is carried in
 * a net_buf. net_buf_unref is called on the net_buf when sending is completed.
 *
 * @param can_dev     The CAN device to be used for sending and receiving.
 * @param data        Data to be sent.
 * @param len         Length of the data to be sent.
 * @param rx_addr     Identifier for FC frames.
 * @param tx_addr     Identifier for outgoing frames the receiver listens on.
 * @param complete_cb Function called on completion or NULL.
 * @param cb_arg      Argument passed to the complete callback.
 * @param timeout     Timeout for buffer allocation.
 *
 * @retval ISOTP_N_OK on success
 * @retval ISOTP_* on error
 */
int isotp_send_net_ctx_buf(const struct device *can_dev,
			   struct net_buf *data,
			   const struct isotp_msg_id *tx_addr,
			   const struct isotp_msg_id *rx_addr,
			   isotp_tx_callback_t complete_cb, void *cb_arg,
			   k_timeout_t timeout);

#endif /*CONFIG_ISOTP_ENABLE_CONTEXT_BUFFERS*/

#if defined(CONFIG_ISOTP_USE_TX_BUF) && \
	defined(CONFIG_ISOTP_ENABLE_CONTEXT_BUFFERS)
/**
 * @brief Send data with buffered context
 *
 * This function is similar to isotp_send, but the context is automatically
 * allocated from an internal pool and the data to be send is buffered in an
 * internal net_buff.
 *
 * @param can_dev     The CAN device to be used for sending and receiving.
 * @param data        Data to be sent.
 * @param len         Length of the data to be sent.
 * @param rx_addr     Identifier for FC frames.
 * @param tx_addr     Identifier for outgoing frames the receiver listens on.
 * @param complete_cb Function called on completion or NULL.
 * @param cb_arg      Argument passed to the complete callback.
 * @param timeout     Timeout for buffer allocation.
 *
 * @retval ISOTP_N_OK on success
 * @retval ISOTP_* on error
 */
int isotp_send_buf(const struct device *can_dev,
		   const uint8_t *data, size_t len,
		   const struct isotp_msg_id *tx_addr,
		   const struct isotp_msg_id *rx_addr,
		   isotp_tx_callback_t complete_cb, void *cb_arg,
		   k_timeout_t timeout);
#endif

/** @cond INTERNAL_HIDDEN */

struct isotp_callback {
	isotp_tx_callback_t cb;
	void *arg;
};

struct isotp_send_ctx {
	int filter_id;
	uint32_t error_nr;
	const struct device *can_dev;
	union {
		struct net_buf *buf;
		struct {
			const uint8_t *data;
			size_t len;
		};
	};
	struct k_work work;
	struct _timeout timeout;
	union {
		struct isotp_callback fin_cb;
		struct k_sem fin_sem;
	};
	struct isotp_fc_opts opts;
	uint8_t state;
	uint8_t tx_backlog;
	struct k_sem tx_sem;
	struct isotp_msg_id rx_addr;
	struct isotp_msg_id tx_addr;
	uint8_t wft;
	uint8_t bs;
	uint8_t sn : 4;
	uint8_t is_net_buf  : 1;
	uint8_t is_ctx_slab : 1;
	uint8_t has_callback: 1;
};

struct isotp_recv_ctx {
	int filter_id;
	const struct device *can_dev;
	struct net_buf *buf;
	struct net_buf *act_frag;
	/* buffer currently processed in isotp_recv */
	struct net_buf *recv_buf;
	sys_snode_t alloc_node;
	uint32_t length;
	int error_nr;
	struct k_work work;
	struct _timeout timeout;
	struct k_fifo fifo;
	struct isotp_msg_id rx_addr;
	struct isotp_msg_id tx_addr;
	struct isotp_fc_opts opts;
	uint8_t state;
	uint8_t bs;
	uint8_t wft;
	uint8_t sn_expected : 4;
};

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ISOTP_H_ */
