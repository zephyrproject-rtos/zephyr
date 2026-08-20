/** @file
 *  @brief Bluetooth HCI lockstep command helper.
 */

/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HCI_LOCKSTEP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HCI_LOCKSTEP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/hci_pkt.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bluetooth HCI lockstep command helper
 * @defgroup bt_hci_lockstep Bluetooth HCI lockstep command helper
 * @ingroup bluetooth
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
 * removes the need for Num_HCI_Command_Packets flow control and for an opcode
 * correlation state machine.
 *
 * The driver provides the send function of its transport, and feeds the
 * packets it receives from the controller to bt_hci_lockstep_feed(), which
 * consumes the awaited response and leaves every other packet to the driver.
 * Commands are built with the helpers of hci_pkt.h, which this header
 * includes.
 *
 * Enabled with @kconfig{CONFIG_BT_HCI_LOCKSTEP}, which the HCI drivers using
 * the helper select in their Kconfig.
 *
 * @note This is not an application API, even though the header lives in the
 *       application-visible include directory: the intended users are HCI
 *       drivers. Applications that need to send HCI commands alongside a
 *       running Host use the higher-level bt_hci_cmd_alloc(),
 *       bt_hci_cmd_send() and bt_hci_cmd_send_sync() APIs of hci.h, which
 *       cooperate with the Host's command flow control.
 *
 * @{
 */

/** @brief Transport send function of a lockstep helper.
 *
 *  Transmits a complete HCI packet to the controller.
 *
 *  @param dev HCI device given to bt_hci_lockstep_init().
 *  @param pkt Packet to send, starting with its packet indicator.
 *  @param len Length of @p pkt in bytes.
 *
 *  @return 0 on success, or a negative error code, which is propagated to the
 *          caller of bt_hci_lockstep_cmd_send_sync().
 */
typedef int (*bt_hci_lockstep_send_t)(const struct device *dev, const uint8_t *pkt, size_t len);

/** @brief Lockstep helper.
 *
 *  Initialize with bt_hci_lockstep_init() before use.
 */
struct bt_hci_lockstep {
	/** Status code (@c BT_HCI_ERR_*) of the most recently received
	 *  response, valid after bt_hci_lockstep_cmd_send_sync() has returned
	 *  0 or -EIO.
	 */
	uint8_t status;

	/** How long bt_hci_lockstep_cmd_send_sync() waits for a response.
	 *  bt_hci_lockstep_init() sets it to 10 seconds, the Bluetooth Host's
	 *  own command timeout; a driver may change it afterwards.
	 */
	k_timeout_t timeout;

	/** @cond INTERNAL_HIDDEN */
	const struct device *dev;
	bt_hci_lockstep_send_t send;
	struct k_sem rsp_sem;
	struct net_buf_simple *rsp;
	atomic_t waiting;
	uint16_t opcode;
	/** @endcond */
};

/** @brief Initialize a lockstep helper.
 *
 *  To be called once, typically from the driver's device initialization
 *  function: the helper can then be used every time the transport is opened,
 *  without its semaphore being re-initialized.
 *
 *  @param ls   Lockstep helper.
 *  @param dev  HCI device, passed to @p send.
 *  @param send Transport send function.
 */
void bt_hci_lockstep_init(struct bt_hci_lockstep *ls, const struct device *dev,
			  bt_hci_lockstep_send_t send);

/** @brief Feed a received HCI packet to a lockstep helper.
 *
 *  To be called by the driver for every packet received from the controller
 *  while the helper is in use. When the packet is the response to the command
 *  that bt_hci_lockstep_cmd_send_sync() is waiting for, the helper consumes it
 *  and wakes the waiter; any other packet is left to the driver to process as
 *  usual. The packet is not modified.
 *
 *  Can be called from any context, including ISRs.
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
 *  with bt_hci_pkt_push_cmd_hdr(), transmits it with the transport send
 *  function and waits, for at most @ref bt_hci_lockstep.timeout, for the
 *  controller's HCI_Command_Complete or HCI_Command_Status response to arrive
 *  through bt_hci_lockstep_feed().
 *
 *  The return parameters of an HCI_Command_Complete response, starting with
 *  the status (as the @c bt_hci_rp_* structures of hci_types.h do), are
 *  stored in @p rsp, which is emptied first. Return parameters beyond the
 *  capacity of @p rsp are discarded, so size it for the expected return
 *  parameters. An HCI_Command_Status response carries no return parameters,
 *  so @p rsp stays empty; its status only means that the controller has
 *  accepted the command, with completion reported later through the
 *  command's own event. The status of the response is also available in
 *  @ref bt_hci_lockstep.status.
 *
 *  Must be called from thread context. Once the packet has been handed to
 *  the send function, @p cmd holds the complete command packet; use
 *  bt_hci_pkt_reset_cmd() before reusing it. On -EINVAL and -EMSGSIZE
 *  @p cmd is unchanged. Every failure is logged together with the opcode,
 *  so the caller need not log it again.
 *
 *  After -EAGAIN the helper cannot tell a late response apart from the
 *  response to a subsequent command with the same opcode, so the caller
 *  should treat the controller as unresponsive rather than retry.
 *
 *  @param ls      Lockstep helper.
 *  @param opcode  HCI command opcode.
 *  @param cmd     Buffer holding the command parameters, set up with
 *                 BT_HCI_PKT_CMD_DEFINE() or bt_hci_pkt_reset_cmd(), or NULL
 *                 for a command without parameters.
 *  @param rsp     Buffer for the return parameters, or NULL to discard them.
 *
 *  @retval 0          The command completed successfully.
 *  @retval -EIO       The controller responded with an error status, available
 *                     in @ref bt_hci_lockstep.status.
 *  @retval -EAGAIN    No response arrived within @ref bt_hci_lockstep.timeout.
 *  @retval -EINVAL    @p cmd has insufficient headroom for the packet prefix.
 *  @retval -EMSGSIZE  @p cmd holds more parameter bytes than an HCI command
 *                     can carry.
 *  @return Any negative error code returned by the transport send function.
 */
int bt_hci_lockstep_cmd_send_sync(struct bt_hci_lockstep *ls, uint16_t opcode,
				  struct net_buf_simple *cmd, struct net_buf_simple *rsp);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HCI_LOCKSTEP_H_ */
