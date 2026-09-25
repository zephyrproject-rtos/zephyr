/** @file
 *  @brief Bluetooth HCI lockstep command helper.
 */

/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_HCI_LOCKSTEP_H_
#define ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_HCI_LOCKSTEP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/hci_pkt.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bluetooth HCI lockstep command helper
 * @defgroup bt_hci_lockstep Bluetooth HCI lockstep command helper
 * @ingroup bt_hci_api
 * @since 4.5
 * @version 0.1.0
 *
 * Helper for sending HCI commands one at a time and waiting for their
 * responses, for HCI drivers that exchange HCI commands with the controller
 * over their own transport — typically for vendor-specific controller
 * initialization — without the Bluetooth Host's command machinery and
 * regardless of whether a Host is present in the build.
 *
 * The helper relies on the HCI traffic being in lockstep: while a command is
 * outstanding no other command is sent, so the next command response received
 * is the response to that command. This holds during controller
 * initialization, before the HCI transport is handed over to its user, and
 * removes the need for an opcode correlation state machine. Command flow
 * control still applies: a command is sent only once the controller allows
 * one, which it does initially and through the Num_HCI_Command_Packets field
 * of every command response, including the HCI_Command_Complete event with
 * the NOP opcode that a controller sends to allow commands on its own.
 *
 * The driver provides the send function of its transport, and feeds the
 * packets it receives from the controller to bt_hci_lockstep_feed(), which
 * consumes the awaited response and leaves every other packet to the driver.
 * Commands are built with the helpers of hci_pkt.h, which this header
 * includes.
 *
 * The helper is initialized once and keeps its view of the controller's
 * command allowance from one opening of the transport to the next. A driver
 * that resets its controller by other means than an HCI command, for example
 * with a reset line while opening, calls bt_hci_lockstep_reset() after that
 * reset and before its first command.
 *
 * The helper is part of every build with @kconfig{CONFIG_BT} enabled.
 *
 * @note This is not an application API: the intended users are HCI drivers.
 *       Applications that need to send HCI commands alongside a running Host
 *       use the higher-level bt_hci_cmd_alloc(), bt_hci_cmd_send() and
 *       bt_hci_cmd_send_sync() APIs of hci.h, which cooperate with the
 *       Host's command flow control.
 *
 * @{
 */

/** @brief Transport send function of a lockstep helper.
 *
 *  Transmits a complete HCI packet to the controller.
 *
 *  A negative return means that the command was not submitted to the
 *  controller: the helper keeps the command allowance it would have used. A
 *  transport that cannot tell whether a command reached the controller before
 *  it failed returns 0 and leaves the outcome to the response wait.
 *
 *  @param dev HCI device given to bt_hci_lockstep_init().
 *  @param pkt Packet to send, starting with its packet indicator.
 *  @param len Length of @p pkt in bytes.
 *
 *  @retval 0 The packet has been submitted to the controller.
 *  @return Negative errno value on failure, the packet not having been
 *          submitted; propagated to the caller of
 *          bt_hci_lockstep_cmd_send_sync().
 */
typedef int (*bt_hci_lockstep_send_t)(const struct device *dev, const uint8_t *pkt, size_t len);

/** @brief Lockstep helper.
 *
 *  Initialize with bt_hci_lockstep_init() before use.
 */
struct bt_hci_lockstep {
	/** @cond INTERNAL_HIDDEN */
	const struct device *dev;
	bt_hci_lockstep_send_t send;
	struct k_sem changed;
	struct k_spinlock lock;
	struct net_buf_simple *rsp;
	uint16_t opcode;
	uint8_t state;
	uint8_t status;
	/** @endcond */

	/** @brief Command timeout.
	 *
	 *  Total time bt_hci_lockstep_cmd_send_sync() waits for the controller
	 *  to allow the command and then to respond to it.
	 *  bt_hci_lockstep_init() sets it to 10 seconds, the Bluetooth Host's
	 *  own command timeout; a driver may change it afterwards.
	 */
	k_timeout_t timeout;
};

/** @brief Initialize a lockstep helper.
 *
 *  To be called once, typically from the driver's device initialization
 *  function: the helper can then be used every time the transport is opened,
 *  without its semaphore being re-initialized. A controller that has been
 *  reset by other means than an HCI command is followed with
 *  bt_hci_lockstep_reset(), not with another call to this function.
 *
 *  @param ls   Lockstep helper.
 *  @param dev  HCI device, passed to @p send.
 *  @param send Transport send function.
 */
void bt_hci_lockstep_init(struct bt_hci_lockstep *ls, const struct device *dev,
			  bt_hci_lockstep_send_t send);

/** @brief Restore the initial state of a lockstep helper.
 *
 *  Restores the initial allowance of one command, for a controller that has
 *  been reset by other means than an HCI command, so that the helper no longer
 *  waits for an allowance the reset controller will not announce. The transport
 *  send function and @ref bt_hci_lockstep.timeout are left as they are.
 *
 *  Can be called from any context, including ISRs, and at the same time as
 *  bt_hci_lockstep_feed(), so a driver that is fed from a callback it cannot
 *  stop can use this as well. A bt_hci_lockstep_cmd_send_sync() call that is
 *  waiting for the controller to allow its command goes ahead and sends it,
 *  as when a recovery path resets a controller that has stopped responding.
 *
 *  A command that is outstanding, its sender having obtained the allowance
 *  and not yet its response, is a different matter: whether it reached the
 *  controller before or after the reset cannot be known, so the driver is
 *  expected not to reset its controller at that point. If it does, the
 *  command is abandoned. Its response is no longer consumed, and its sender
 *  fails: with the transport's error if the command could not be submitted,
 *  otherwise with -EAGAIN when its timeout has run out and not earlier, so
 *  that a response which still arrives cannot be taken for that of a later
 *  command with the same opcode.
 *
 *  Packets the controller sent before the reset are not to be fed afterwards:
 *  the helper cannot tell them from the responses to what follows, so a stale
 *  response to the same opcode would pass for the new one, and a stale command
 *  response allowing no command would revoke the restored allowance.
 *
 *  @isr_ok
 *
 *  @param ls Lockstep helper.
 */
void bt_hci_lockstep_reset(struct bt_hci_lockstep *ls);

/** @brief Feed a received HCI packet to a lockstep helper.
 *
 *  To be called by the driver for every packet received from the controller
 *  while the helper is in use. When the packet is the response to the command
 *  that bt_hci_lockstep_cmd_send_sync() is waiting for, the helper consumes it
 *  and wakes the waiter; any other packet is left to the driver to process as
 *  usual. The packet is not modified.
 *
 *  Every command response fed, consumed or not, also updates the number of
 *  commands the controller allows, which bt_hci_lockstep_cmd_send_sync()
 *  waits for; the HCI_Command_Complete event with the NOP opcode exists for
 *  that alone.
 *
 *  Can be called from any context, including ISRs. Calls are serialized by
 *  the driver's single receive path and come in the order the packets were
 *  received.
 *
 *  @isr_ok
 *
 *  @param ls  Lockstep helper.
 *  @param pkt Received packet, starting with its packet indicator.
 *  @param len Length of @p pkt in bytes.
 *
 *  @retval true  The packet was the awaited response and has been consumed.
 *  @retval false The packet is not the awaited response.
 */
bool bt_hci_lockstep_feed(struct bt_hci_lockstep *ls, const uint8_t *pkt, size_t len);

/** @brief Send an HCI command and wait for its response.
 *
 *  Frames the command parameters in @p cmd into a complete HCI command packet
 *  with bt_hci_pkt_push_cmd_hdr(), waits for the controller to allow a
 *  command, transmits the packet with the transport send function and waits
 *  for the controller's HCI_Command_Complete or HCI_Command_Status response
 *  to arrive through bt_hci_lockstep_feed(). The call waits at most
 *  @ref bt_hci_lockstep.timeout in total, for the controller to allow a
 *  command and then to respond to it.
 *
 *  The response is stored in @p rsp, which is emptied first: its status,
 *  followed by the return parameters of an HCI_Command_Complete, which is
 *  the layout of the @c bt_hci_rp_* structures of hci_types.h. Bytes beyond
 *  the capacity of @p rsp are discarded, so size it for the expected return
 *  parameters. An HCI_Command_Status response carries no return parameters,
 *  so @p rsp holds its status alone, which only means that the controller
 *  has accepted the command, with completion reported later through the
 *  command's own event.
 *
 *  Must be called from thread context, one call at a time: the driver
 *  serializes its callers. On -EINVAL and -EMSGSIZE @p cmd is unchanged; on
 *  every other return it holds the complete command packet, so use
 *  bt_hci_pkt_reset_cmd() before reusing it. Every failure is logged
 *  together with the opcode, so the caller need not log it again. A
 *  transport send failure reported after the response has already arrived
 *  is only logged: the command reached the controller, and its response
 *  counts.
 *
 *  -EAGAIN means an unresponsive controller, to be treated as such rather
 *  than retried: either it allowed no command within the timeout, or it did
 *  not respond to the command, after which it allows no further command and
 *  a late response could not be told apart from the response to a
 *  subsequent command with the same opcode.
 *
 *  @param ls      Lockstep helper.
 *  @param opcode  HCI command opcode.
 *  @param cmd     Buffer holding the command parameters, set up with
 *                 BT_HCI_PKT_CMD_DEFINE() or bt_hci_pkt_reset_cmd(), or NULL
 *                 for a command without parameters.
 *  @param rsp     Buffer for the return parameters, or NULL to discard them.
 *
 *  @retval 0          The command completed successfully.
 *  @retval -EIO       The controller responded with an error status, logged
 *                     and stored in @p rsp.
 *  @retval -EAGAIN    The controller allowed no command, or did not respond,
 *                     within @ref bt_hci_lockstep.timeout.
 *  @retval -EINVAL    @p cmd has insufficient headroom for the packet prefix.
 *  @retval -EMSGSIZE  @p cmd holds more parameter bytes than an HCI command
 *                     can carry.
 *  @return Negative errno value on a transport send failure, as returned
 *          by the send function; the command was not submitted.
 */
int bt_hci_lockstep_cmd_send_sync(struct bt_hci_lockstep *ls, uint16_t opcode,
				  struct net_buf_simple *cmd, struct net_buf_simple *rsp);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_HCI_LOCKSTEP_H_ */
