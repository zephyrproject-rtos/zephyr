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

#include <zephyr/net_buf.h>
#include <zephyr/sys/slist.h>

/* This header is kept free of host-only Kconfig dependencies so that
 * conn_internal.h can embed the operation in struct bt_conn in any build.
 */

struct bt_future;

struct bt_hci_cmd_op;

/** @brief Parameter encoder of an asynchronous HCI command.
 *
 *  Called when the command gets its buffer, with an empty command buffer whose
 *  headroom for the packet indicator and HCI command header is reserved: add
 *  the command parameters with the net_buf API. The buffer holds up to
 *  BT_BUF_CMD_TX_SIZE bytes of parameters.
 *
 *  Runs on the TX processor with the host lock and a spinlock held: it only
 *  writes the parameters and reads the state it needs for that. It must not
 *  block, log at length or call back into the command API.
 *
 *  The context of the encoder is the object that the operation is embedded
 *  in, which CONTAINER_OF() gives. An operation carries no context pointer:
 *  it would be one more word in every object that has an operation.
 *
 *  @param buf Command buffer to fill.
 *  @param op  The operation.
 *
 *  @retval 0 The parameters have been added.
 *  @retval -errno The operation is stale and must not be sent: it completes
 *                 with -ECANCELED.
 */
typedef int (*bt_hci_cmd_encode_t)(struct net_buf *buf, struct bt_hci_cmd_op *op);

/** @brief Completion callback of bt_hci_cmd_send_cb().
 *
 *  Runs on the Bluetooth workqueue. The operation is the caller's again when
 *  the callback is invoked: it may be sent again, and the object that it is
 *  embedded in may be released.
 *
 *  @param op     The operation, see bt_hci_cmd_encode_t for its context.
 *  @param result Result of the command, see bt_hci_cmd_send_async().
 */
typedef void (*bt_hci_cmd_cb_t)(struct bt_hci_cmd_op *op, int result);

/** @brief Asynchronous HCI command operation.
 *
 *  Caller-owned storage of one asynchronous HCI command. It carries the
 *  command opcode and parameter encoder; the command buffer itself is taken
 *  from the command pool only when the command is dispatched, without
 *  blocking, so the sending APIs never block or allocate.
 *
 *  From a successful send call until the completion is handed back (the future
 *  resolved, the callback about to run, or bt_hci_cmd_op_cancel() returning 0)
 *  the operation belongs to the command engine: it, its encoder context and
 *  its response destination must stay valid, and an object that the operation
 *  is embedded in must be kept alive by a reference taken before the send call.
 *
 *  Initialize with bt_hci_cmd_op_init() before the first use.
 */
struct bt_hci_cmd_op {
	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	uint16_t opcode;
	uint8_t state;
	uint8_t kind;
	bt_hci_cmd_encode_t encode;
	struct net_buf_simple *rsp;
	union {
		struct bt_future *fut;
		bt_hci_cmd_cb_t cb;
	};
	uint16_t rsp_len;
	uint8_t status;
	uint8_t err;
	/** @endcond */
};

/** @brief Initialize an asynchronous HCI command operation.
 *
 *  Must not be called on an operation that is pending.
 *
 *  @param op     Operation to initialize.
 *  @param opcode Command OpCode.
 *  @param encode Parameter encoder, or NULL for a command without parameters.
 */
void bt_hci_cmd_op_init(struct bt_hci_cmd_op *op, uint16_t opcode, bt_hci_cmd_encode_t encode);

/** @brief Check whether an asynchronous HCI command operation is pending.
 *
 *  @param op Operation.
 *
 *  @return true from a successful send call until the completion is handed
 *          back. It turns false just before the future is resolved or the
 *          callback is invoked, so a false value does not by itself say that
 *          the result has been collected.
 */
bool bt_hci_cmd_op_is_pending(struct bt_hci_cmd_op *op);

/** @brief Send an HCI command and get its completion through a future.
 *
 *  The asynchronous counterpart of bt_hci_cmd_send_sync(): the command is
 *  queued without a buffer and the caller-provided @p fut is resolved when it
 *  completes. Nothing in this call blocks or allocates.
 *
 *  The result that the future is resolved with:
 *
 *  - 0 or a positive HCI status: the controller answered with that status.
 *  - -EMSGSIZE: the controller answered, but the return parameters did not fit
 *    @p rsp, which holds as much of them as it has room for.
 *    bt_hci_cmd_op_status() and bt_hci_cmd_op_rsp_len() give the status and
 *    the actual length.
 *  - -ECANCELED: the encoder refused the operation, or it was canceled. The
 *    command was not sent.
 *  - -EIO: the HCI driver failed to send the command, which means that the
 *    controller will not answer it.
 *  - -ENOBUFS: no command buffer became available for the operation in the
 *    time that a command is given. The command was not sent.
 *  - -ETIMEDOUT: the controller did not answer the command.
 *  - -EHOSTDOWN: the HCI transport was closed, or the controller stopped
 *    answering, before the command completed. Unless it was the one command
 *    that was with the controller at that moment, it was not sent.
 *
 *  The return parameters, starting with the status as in the Command Complete
 *  event, are copied into @p rsp, which is emptied first. The command buffer
 *  is never handed out.
 *
 *  With @p fut NULL the command is sent fire-and-forget: a failure is only
 *  logged.
 *
 *  A call that fails changes nothing: no completion is delivered for it, and
 *  an earlier send of the same operation stays pending.
 *
 *  @param op  Operation, initialized with bt_hci_cmd_op_init().
 *  @param rsp Destination of the return parameters, or NULL to discard them.
 *  @param fut Completion future, initialized by this function, or NULL.
 *
 *  @retval 0 The command has been queued.
 *  @retval -EBUSY The operation is pending.
 *  @retval -EHOSTDOWN The HCI transport is not open, or the controller has
 *                     stopped answering commands.
 */
int bt_hci_cmd_send_async(struct bt_hci_cmd_op *op, struct net_buf_simple *rsp,
			  struct bt_future *fut);

/** @brief Send an HCI command and get its completion through a callback.
 *
 *  Like bt_hci_cmd_send_async(), but the completion is delivered by invoking
 *  @p cb on the Bluetooth workqueue.
 *
 *  @param op  Operation, initialized with bt_hci_cmd_op_init().
 *  @param rsp Destination of the return parameters, or NULL to discard them.
 *  @param cb  Completion callback.
 *
 *  @retval 0 The command has been queued.
 *  @retval -EINVAL @p cb is NULL.
 *  @retval -EBUSY The operation is pending.
 *  @retval -EHOSTDOWN The HCI transport is not open, or the controller has
 *                     stopped answering commands.
 */
int bt_hci_cmd_send_cb(struct bt_hci_cmd_op *op, struct net_buf_simple *rsp, bt_hci_cmd_cb_t cb);

/** @brief Cancel an asynchronous HCI command that has not been sent yet.
 *
 *  Callable from any context. HCI cannot recall a command, so one that has
 *  been handed to the controller can only be awaited.
 *
 *  @param op Operation.
 *
 *  @retval 0 The operation was still queued and is the caller's again. The
 *            controller never sees the command. A callback is not invoked; a
 *            future has been resolved with -ECANCELED.
 *  @retval -EINPROGRESS The command is with the controller, or its completion
 *                       is about to be delivered: await the completion.
 *  @retval -EALREADY The operation is not pending.
 */
int bt_hci_cmd_op_cancel(struct bt_hci_cmd_op *op);

/** @brief Controller status of the last completion of an operation.
 *
 *  Valid once a completion with a result of 0 or more, or -EMSGSIZE, has been
 *  handed back, until the operation is sent again.
 *
 *  @param op Operation.
 *
 *  @return HCI status that the controller answered with.
 */
static inline uint8_t bt_hci_cmd_op_status(const struct bt_hci_cmd_op *op)
{
	return op->status;
}

/** @brief Length of the return parameters of the last completion.
 *
 *  Valid under the same conditions as bt_hci_cmd_op_status(). Larger than
 *  what the response destination holds when the result was -EMSGSIZE.
 *
 *  @param op Operation.
 *
 *  @return Length in bytes of the return parameters, the status included.
 */
static inline uint16_t bt_hci_cmd_op_rsp_len(const struct bt_hci_cmd_op *op)
{
	return op->rsp_len;
}

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_HCI_CMD_OP_H_ */
