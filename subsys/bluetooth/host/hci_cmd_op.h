/* hci_cmd_op.h - Bluetooth host asynchronous HCI command operation */

/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_HOST_HCI_CMD_OP_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_HOST_HCI_CMD_OP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/net_buf.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include "future.h"

/* This header is kept free of host-only Kconfig dependencies so that
 * conn_internal.h can embed the operation in struct bt_conn in any build.
 */

struct bt_hci_cmd_op;

/** @brief Parameter encoder of an asynchronous HCI command.
 *
 *  Called when the command gets its buffer, with an empty command buffer whose
 *  headroom for the packet indicator and HCI command header is reserved: add
 *  the command parameters with the net_buf API. The buffer holds up to
 *  BT_BUF_CMD_TX_SIZE bytes of parameters.
 *
 *  Runs on the TX processor with the host lock held, which is what guards
 *  the state that it reads against writers that hold that lock too. It must
 *  not block or send commands. The operation cannot be handed back while the
 *  encoder runs, see bt_hci_cmd_op_cancel().
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

/** @brief Handler of the return parameters of an asynchronous HCI command.
 *
 *  Called when the controller has responded to the command with success,
 *  before the command buffer is released and the completion is handed back,
 *  with the return parameters of the Command Complete event, the status first.
 *  bt_hci_cmd_op_status() gives the status already.
 *
 *  Runs where the response arrives: in the receive context of the HCI driver,
 *  with the scheduler locked, which can be inside the send() of the driver
 *  while the TX processor holds the host lock. It must not block, send
 *  commands or keep @p rp. It copies what is wanted into the object that the
 *  operation is embedded in, for the owner to read once the completion has
 *  been handed back; state that others read needs its own reason to be safe.
 *
 *  @param op The operation, see bt_hci_cmd_encode_t for its context.
 *  @param rp The return parameters, as many as the controller sent: check
 *            their length before reading them.
 *
 *  @retval 0 The return parameters have been taken.
 *  @retval -errno They are of no use, too short for instance: the operation
 *                 completes with this error.
 */
typedef int (*bt_hci_cmd_rsp_t)(struct bt_hci_cmd_op *op, struct net_buf *rp);

/** @brief Completion callback of an asynchronous HCI command.
 *
 *  Runs on the Bluetooth workqueue. The operation is the caller's again when
 *  the callback is invoked: it may be sent again, and the object that it is
 *  embedded in may be released.
 *
 *  @param op     The operation, see bt_hci_cmd_encode_t for its context.
 *  @param result Result of the command, see bt_hci_cmd_send_cb().
 */
typedef void (*bt_hci_cmd_cb_t)(struct bt_hci_cmd_op *op, int result);

/** @brief Description of an asynchronous HCI command.
 *
 *  What every use of a command has in common. Define it once, as a constant
 *  with static storage duration, next to its encoder, the handler of its
 *  return parameters and its callback:
 *
 *  @code{.c}
 *  static const struct bt_hci_cmd_desc foo_cmd = {
 *          .opcode = BT_HCI_OP_FOO,
 *          .encode = foo_encode,
 *          .rsp = foo_rsp,
 *          .done = foo_done,
 *  };
 *  @endcode
 */
struct bt_hci_cmd_desc {
	/** Command OpCode */
	uint16_t opcode;
	/** Parameter encoder, or NULL for a command without parameters */
	bt_hci_cmd_encode_t encode;
	/** Handler of the return parameters, or NULL when they are not wanted */
	bt_hci_cmd_rsp_t rsp;
	/** Completion callback of bt_hci_cmd_send_cb(), or NULL when nobody is
	 *  to be told: a failure is then only logged, and only an operation
	 *  that is never released can be sent that way.
	 */
	bt_hci_cmd_cb_t done;
};

/** @brief Asynchronous HCI command operation.
 *
 *  The caller-owned state of one command that is on its way. The command
 *  buffer is taken from the command pool only when the command is dispatched,
 *  so the sending functions neither block nor allocate.
 *
 *  An operation that is all zeroes is idle: a static variable, or a member of
 *  an object that is cleared when it is taken into use, needs no
 *  initialization.
 *
 *  From a successful send call until the completion is handed back (the
 *  callback about to run, the future resolved, or bt_hci_cmd_op_cancel()
 *  returning 0) the operation and what its encoder uses belong to the command
 *  engine and must stay valid. That time ends with the hand-back only, never
 *  with a timeout of the caller.
 *
 *  The operation is therefore a member of the object that the command is
 *  about, which a reference taken before the send call keeps alive, or a
 *  static variable when the command is about no object. The encoder and the
 *  callback get to that object with CONTAINER_OF(). A stack frame can hold an
 *  operation only as bt_hci_cmd_send_async() describes.
 */
struct bt_hci_cmd_op {
	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	const struct bt_hci_cmd_desc *desc;
	uint8_t state;
	uint8_t kind;
	uint8_t status;
	uint8_t err;
	/** @endcond */
};

/** @brief Operation whose completion resolves a future.
 *
 *  For bt_hci_cmd_send_async(). The future is part of the object, as it
 *  stays with the command engine for as long as the operation does.
 */
struct bt_hci_cmd_op_fut {
	/** Resolved with the result of the command */
	struct bt_future fut;
	/** The operation */
	struct bt_hci_cmd_op op;
};

/** @brief Send an HCI command and get its completion through a callback.
 *
 *  The command is queued without a buffer, and the callback of @p desc is
 *  invoked on the Bluetooth workqueue when it completes. Nothing in this call
 *  blocks or allocates.
 *
 *  The result that the callback gets:
 *
 *  - 0 or a positive HCI status: the controller responded with that status.
 *  - -errno that the handler of the return parameters of @p desc returned: the
 *    controller responded with success, see bt_hci_cmd_rsp_t.
 *  - -ECANCELED: the encoder refused the operation, or bt_hci_cmd_op_cancel()
 *    was called while the encoder ran. The command was not sent.
 *  - -EIO: the HCI driver failed to send the command, which means that the
 *    controller will not respond to it.
 *  - -ENOBUFS: no command buffer became available for the operation in the
 *    time that a command is given. The command was not sent.
 *  - -ETIMEDOUT: the controller did not respond to the command.
 *  - -EHOSTDOWN: the HCI transport was closed, or the controller stopped
 *    responding, before the command completed. Unless it was the one command
 *    that was with the controller at that moment, it was not sent.
 *
 *  The return parameters of the command go to the handler of @p desc, if it
 *  has one.
 *
 *  A call that fails changes nothing: no completion is delivered for it, and
 *  an earlier send of the same operation stays pending.
 *
 *  @param op   Operation.
 *  @param desc Description of the command. It has to stay valid until the
 *              completion is handed back.
 *
 *  @retval 0 The command has been queued.
 *  @retval -EBUSY The operation is pending.
 *  @retval -EHOSTDOWN The HCI transport is not open, or the controller has
 *                     stopped responding to commands.
 */
int bt_hci_cmd_send_cb(struct bt_hci_cmd_op *op, const struct bt_hci_cmd_desc *desc);

/** @brief Send an HCI command and get its completion through a future.
 *
 *  Like bt_hci_cmd_send_cb(), but the completion resolves the future of
 *  @p req, which this call initializes, with the result. A cancel resolves it
 *  with -ECANCELED. The callback of @p desc is not used.
 *
 *  Whoever collects the result has to be done with it before the operation is
 *  sent again: an operation stops being pending just before its future is
 *  resolved.
 *
 *  A function that has @p req on its stack must not return while the operation
 *  is pending. Every operation is handed back, so the function waits for the
 *  future with K_FOREVER. After a shorter wait that timed out it cancels the
 *  operation and then waits with K_FOREVER, whatever the cancel returned:
 *
 *  @code{.c}
 *  if (bt_future_wait(&req.fut, timeout) != 0) {
 *          (void)bt_hci_cmd_op_cancel(&req.op);
 *          (void)bt_future_wait(&req.fut, K_FOREVER);
 *  }
 *  @endcode
 *
 *  @param req  Operation with its future. On a stack, initialize it to zero.
 *  @param desc Description of the command.
 *
 *  @return As bt_hci_cmd_send_cb().
 */
int bt_hci_cmd_send_async(struct bt_hci_cmd_op_fut *req, const struct bt_hci_cmd_desc *desc);

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
 *                       is about to be delivered: await the completion. The
 *                       same if its encoder is running: the command is not
 *                       sent then, and the operation completes with
 *                       -ECANCELED.
 *  @retval -EALREADY The operation is not pending. Its future may still be
 *                    about to be resolved.
 */
int bt_hci_cmd_op_cancel(struct bt_hci_cmd_op *op);

/** @brief Controller status of the last completion of an operation.
 *
 *  Valid once a completion with a result of 0 or more, or with an error of
 *  the handler of the return parameters, has been handed back, until the
 *  operation is sent again.
 *
 *  @param op Operation.
 *
 *  @return HCI status that the controller responded with.
 */
static inline uint8_t bt_hci_cmd_op_status(const struct bt_hci_cmd_op *op)
{
	return op->status;
}

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_HCI_CMD_OP_H_ */
