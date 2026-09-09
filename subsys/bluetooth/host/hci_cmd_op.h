/* hci_cmd_op.h - Bluetooth host asynchronous HCI command operation */

/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_HOST_HCI_CMD_OP_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_HOST_HCI_CMD_OP_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/slist.h>

/* This header is kept free of host-only Kconfig dependencies so that
 * conn_internal.h can embed the operation in struct bt_conn in any build.
 */

struct bt_future;

/** @brief Parameter encoder of an asynchronous HCI command.
 *
 *  Called when the command is dispatched, with an empty command buffer whose
 *  headroom for the packet indicator and HCI command header is reserved: add
 *  the command parameters with the net_buf API. The buffer holds up to
 *  BT_BUF_CMD_TX_SIZE bytes of parameters.
 *
 *  Runs on the TX processor with the host lock held, so it must not block,
 *  must not wait for anything produced by the HCI RX path and must not send
 *  HCI commands itself.
 *
 *  @param buf       Command buffer to fill.
 *  @param user_data User data of the operation.
 */
typedef void (*bt_hci_cmd_encode_t)(struct net_buf *buf, void *user_data);

/** @brief Completion callback of bt_hci_cmd_send_cb().
 *
 *  @param status    HCI status of the command completion (0 on success).
 *  @param rsp       Response buffer positioned at the return parameters,
 *                   NULL when the command completed with a non-zero status.
 *                   Ownership is transferred to the callback, which is
 *                   responsible for calling net_buf_unref() on it.
 *  @param user_data User data given to bt_hci_cmd_send_cb().
 */
typedef void (*bt_hci_cmd_cb_t)(uint8_t status, struct net_buf *rsp, void *user_data);

/** @brief Asynchronous HCI command operation.
 *
 *  Caller-owned context of one asynchronous HCI command. It carries the
 *  command opcode and parameter encoder; the command buffer itself is taken
 *  from the command pool only when the command is dispatched, without
 *  blocking, so the sending APIs never block on the pool. The operation must
 *  remain valid until its completion has been delivered, and can be reused
 *  from within that completion.
 *
 *  Initialize with bt_hci_cmd_op_init() before use. Completion is delivered
 *  through a future (bt_hci_cmd_send_async()); for delivery by callback embed
 *  it in a struct bt_hci_cmd_op_cb instead.
 */
struct bt_hci_cmd_op {
	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	uint16_t opcode;
	uint8_t kind;
	/* HCI status awaiting delivery by a callback */
	uint8_t status;
	bt_hci_cmd_encode_t encode;
	void *user_data;
	atomic_t state;
	struct bt_future *fut;
	/** @endcond */
};

/** @brief Asynchronous HCI command operation completed by a callback.
 *
 *  A bt_hci_cmd_op with the state needed to deliver its completion by
 *  invoking a callback on a caller-provided workqueue
 *  (bt_hci_cmd_send_cb()). Initialize with bt_hci_cmd_op_cb_init() before
 *  use; it can then be reused freely, also from within its own callback.
 */
struct bt_hci_cmd_op_cb {
	/** The operation */
	struct bt_hci_cmd_op op;
	/** @cond INTERNAL_HIDDEN */
	struct k_work work;
	struct k_work_q *workq;
	bt_hci_cmd_cb_t cb;
	void *user_data;
	struct net_buf *rsp;
	/** @endcond */
};

/** @brief Initialize an asynchronous HCI command operation.
 *
 *  Must not be called on an operation that is pending.
 *
 *  @param op        Operation to initialize.
 *  @param opcode    Command OpCode.
 *  @param encode    Parameter encoder, or NULL for a command without
 *                   parameters.
 *  @param user_data User data passed to @p encode.
 */
void bt_hci_cmd_op_init(struct bt_hci_cmd_op *op, uint16_t opcode,
			bt_hci_cmd_encode_t encode, void *user_data);

/** @brief Initialize an asynchronous HCI command operation completed by a callback.
 *
 *  Must not be called on an operation that is pending.
 *
 *  @param op        Operation to initialize.
 *  @param opcode    Command OpCode.
 *  @param encode    Parameter encoder, or NULL for a command without
 *                   parameters.
 *  @param user_data User data passed to @p encode.
 */
void bt_hci_cmd_op_cb_init(struct bt_hci_cmd_op_cb *op, uint16_t opcode,
			   bt_hci_cmd_encode_t encode, void *user_data);

/** @brief Check whether an asynchronous HCI command operation is pending.
 *
 *  @param op Operation.
 *
 *  @return true from the moment a send call succeeds until the completion
 *          has been delivered (the future resolved, or the callback about to
 *          run).
 */
bool bt_hci_cmd_op_is_pending(const struct bt_hci_cmd_op *op);

/** @brief Send an HCI command and get notified of its completion via a future.
 *
 *  This is the asynchronous counterpart of bt_hci_cmd_send_sync(): instead of
 *  blocking until the command completes, the caller-provided @p fut is
 *  resolved upon completion (Command Complete or Command Status). This allows
 *  awaiting an HCI command completion from contexts where blocking is not
 *  possible or not desirable (such as an HCI event handler), and is a building
 *  block towards fully asynchronous host APIs.
 *
 *  The command is queued without a buffer; the buffer is allocated when the
 *  command is dispatched and the operation waits, queued, while the command
 *  pool is exhausted. Nothing in this call blocks.
 *
 *  @p fut is initialized by this function and must remain valid until the
 *  command completes. Upon completion the future is resolved with its result
 *  set to the HCI status of the command completion (0 on success), and on
 *  success @c fut->data points to the response buffer, ownership of which is
 *  transferred to the caller (which is responsible for calling
 *  net_buf_unref() on it). @c fut->data is left as NULL when the command
 *  completes with a non-zero status. The caller awaits the completion with
 *  bt_future_wait(), which only reports the wait status; once it returns 0,
 *  the HCI status is read from @c fut->result and the response buffer from
 *  @c fut->data.
 *
 *  With @p fut NULL the command is sent fire-and-forget: the response is
 *  released internally and a failure status is only logged.
 *
 *  If this function returns an error the operation is not pending and no
 *  completion is ever delivered.
 *
 *  @param op  Operation, initialized with bt_hci_cmd_op_init().
 *  @param fut Caller-provided completion future, or NULL.
 *
 *  @return 0 on success, negative errno value on failure.
 *  @retval -EBUSY The operation is already pending.
 *  @retval -EHOSTDOWN The HCI transport is not open, i.e. Bluetooth is not enabled.
 */
int bt_hci_cmd_send_async(struct bt_hci_cmd_op *op, struct bt_future *fut);

/** @brief Send an HCI command and get its completion delivered by a callback.
 *
 *  Like bt_hci_cmd_send_async(), but the completion is delivered by invoking
 *  @p cb on the caller-provided @p workq, so that the caller needs no context
 *  to wait in. The callback receives the raw HCI status and, on success, the
 *  response buffer. If @p workq cannot accept work when the command completes
 *  (it is draining or has not been started), the response is released
 *  internally and the callback never runs.
 *
 *  If this function returns an error the operation is not pending and no
 *  completion is ever delivered.
 *
 *  @param op        Operation, initialized with bt_hci_cmd_op_cb_init().
 *  @param workq     Workqueue on which @p cb is invoked.
 *  @param cb        Completion callback.
 *  @param user_data User data passed to @p cb.
 *
 *  @return 0 on success, negative errno value on failure.
 *  @retval -EINVAL @p workq or @p cb is NULL.
 *  @retval -EBUSY The operation is already pending.
 *  @retval -EHOSTDOWN The HCI transport is not open, i.e. Bluetooth is not enabled.
 */
int bt_hci_cmd_send_cb(struct bt_hci_cmd_op_cb *op, struct k_work_q *workq,
		       bt_hci_cmd_cb_t cb, void *user_data);

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_HCI_CMD_OP_H_ */
