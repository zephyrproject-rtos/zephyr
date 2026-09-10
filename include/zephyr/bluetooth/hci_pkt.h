/** @file
 *  @brief Bluetooth HCI packet helpers.
 */

/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HCI_PKT_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HCI_PKT_H_

#include <stdint.h>

#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/net_buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bluetooth HCI packet helpers
 * @defgroup bt_hci_pkt Bluetooth HCI packet helpers
 * @since 4.5
 * @version 0.1.0
 * @ingroup bluetooth
 *
 * Stateless helpers for framing HCI command packets and for parsing the
 * HCI events that carry command responses, operating on @ref net_buf_simple
 * buffers and on packet bytes. They depend on neither the Bluetooth Host nor the HCI driver
 * interface and can be used in any build type: by the Host, by HCI drivers
 * (for example to perform vendor-specific controller initialization over the
 * driver's own transport) and by controller-only applications.
 *
 * @note These are not general application APIs, even though the header lives
 *       in the application-visible include directory: the intended users are
 *       HCI drivers and Bluetooth stack internals. Applications that need to
 *       send HCI commands alongside a running Host use the higher-level
 *       bt_hci_cmd_alloc(), bt_hci_cmd_send() and bt_hci_cmd_send_sync() APIs
 *       of hci.h, which cooperate with the Host's command flow control.
 *
 * Packets are handled in the form used throughout Zephyr: the HCI packet
 * indicator (@c BT_HCI_H4_CMD, @c BT_HCI_H4_EVT, ...) is the first byte,
 * followed by the packet header and the parameters. Parameters are encoded
 * and decoded by the caller with the packed @c bt_hci_cp_* and @c bt_hci_rp_*
 * structures and the net_buf_simple API, exactly as in the Bluetooth Host.
 *
 * @{
 */

/** @brief Size of the prefix of an HCI command packet.
 *
 *  The packet indicator followed by the HCI command header, which precede the
 *  command parameters. Equals the reservation the Bluetooth Host's command
 *  buffers use (see @ref BT_BUF_CMD_SIZE); kept separate so that this header
 *  has no dependency on the Host's buffer API.
 */
#define BT_HCI_PKT_CMD_HDR_SIZE (sizeof(uint8_t) + BT_HCI_CMD_HDR_SIZE)

/** @brief Size of an HCI command packet with @p param_len bytes of parameters. */
#define BT_HCI_PKT_CMD_SIZE(param_len) (BT_HCI_PKT_CMD_HDR_SIZE + (param_len))

/** @cond INTERNAL_HIDDEN */
#define Z_BT_HCI_PKT_CMD_DEFINE(_name, _max_param_len, _storage)                                  \
	_storage uint8_t net_buf_data_##_name[BT_HCI_PKT_CMD_SIZE(_max_param_len)];                \
	_storage struct net_buf_simple _name = {                                                   \
		.data = net_buf_data_##_name + BT_HCI_PKT_CMD_HDR_SIZE,                            \
		.len = 0,                                                                          \
		.size = BT_HCI_PKT_CMD_SIZE(_max_param_len),                                       \
		.__buf = net_buf_data_##_name,                                                     \
	}
/** @endcond */

/** @brief Define a buffer for an HCI command packet.
 *
 *  Defines and initializes a @ref net_buf_simple with storage for an HCI
 *  command packet carrying up to @p _max_param_len bytes of parameters. The
 *  buffer starts out empty, with headroom reserved for
 *  bt_hci_pkt_push_cmd_hdr(): parameters are added with the net_buf_simple
 *  API, after which the header is pushed in front of them.
 *
 *  To reuse the buffer for another command, or to prepare a buffer over
 *  other storage, use bt_hci_pkt_reset_cmd().
 *
 *  @param _name          Name of the net_buf_simple object.
 *  @param _max_param_len Maximum number of parameter bytes the buffer holds.
 */
#define BT_HCI_PKT_CMD_DEFINE(_name, _max_param_len)                                               \
	Z_BT_HCI_PKT_CMD_DEFINE(_name, _max_param_len,)

/** @brief Define a static buffer for an HCI command packet.
 *
 *  Same as BT_HCI_PKT_CMD_DEFINE(), with static storage duration.
 *
 *  @param _name          Name of the net_buf_simple object.
 *  @param _max_param_len Maximum number of parameter bytes the buffer holds.
 */
#define BT_HCI_PKT_CMD_DEFINE_STATIC(_name, _max_param_len)                                        \
	Z_BT_HCI_PKT_CMD_DEFINE(_name, _max_param_len, static)

/** @brief Reset a buffer for a new HCI command packet.
 *
 *  Empties @p buf and reserves headroom for bt_hci_pkt_push_cmd_hdr(), so
 *  that command parameters can be added with the net_buf_simple API. Use it
 *  to reuse a buffer defined with BT_HCI_PKT_CMD_DEFINE(), or to prepare a
 *  net_buf_simple initialized over caller-provided storage (for example with
 *  net_buf_simple_init_with_data()). The storage must hold at least
 *  @ref BT_HCI_PKT_CMD_HDR_SIZE bytes.
 *
 *  @param buf Buffer to reset.
 */
void bt_hci_pkt_reset_cmd(struct net_buf_simple *buf);

/** @brief Push the packet indicator and command header of an HCI command packet.
 *
 *  Prepends @c BT_HCI_H4_CMD and an HCI command header with @p opcode to the
 *  contents of @p buf, which must hold the command parameters (if any). The
 *  parameter length in the header is derived from the length of @p buf, and
 *  the buffer then contains the complete command packet.
 *
 *  @param buf    Buffer holding the command parameters, with at least
 *                @ref BT_HCI_PKT_CMD_HDR_SIZE bytes of headroom, as set up by
 *                BT_HCI_PKT_CMD_DEFINE() or bt_hci_pkt_reset_cmd().
 *  @param opcode HCI command opcode.
 *
 *  @retval 0         Success; @p buf holds the complete command packet.
 *  @retval -EINVAL   @p buf has insufficient headroom for the prefix.
 *  @retval -EMSGSIZE @p buf holds more parameter bytes than an HCI command can
 *                    carry.
 */
int bt_hci_pkt_push_cmd_hdr(struct net_buf_simple *buf, uint16_t opcode);

/** @brief Decoded HCI command response.
 *
 *  Filled by bt_hci_pkt_pull_cmd_complete(), bt_hci_pkt_pull_cmd_status() and
 *  bt_hci_pkt_parse_cmd_rsp() from an HCI_Command_Complete or
 *  HCI_Command_Status event.
 */
struct bt_hci_pkt_cmd_rsp {
	/** @brief Opcode of the command the event responds to.
	 *
	 *  @c BT_OP_NOP when the event responds to no command and only updates
	 *  the number of command packets the controller accepts.
	 */
	uint16_t opcode;
	/** @brief Num_HCI_Command_Packets.
	 *
	 *  The number of further HCI command packets the controller accepts.
	 */
	uint8_t ncmd;
	/** @brief Status code (@c BT_HCI_ERR_*).
	 *
	 *  For HCI_Command_Complete this is the first return parameter. It is
	 *  meaningful only when @ref opcode is not @c BT_OP_NOP: that event
	 *  responds to no command and carries no status, and the field is then
	 *  @c BT_HCI_ERR_SUCCESS. Match @ref opcode before acting on it. When
	 *  an HCI_Command_Complete for a command lacks the status its return
	 *  parameters start with, reported as @c -ENODATA by the decoders, the
	 *  field is @c BT_HCI_ERR_UNSPECIFIED.
	 */
	uint8_t status;
	/** @brief Return parameters of the command.
	 *
	 *  Point into the decoded event and start with the status, as the
	 *  @c bt_hci_rp_* structures do. HCI_Command_Status carries no return
	 *  parameters: @ref rp_len is then 0 and @ref rp points just past the
	 *  event parameters, so that a zero-length copy stays well defined.
	 */
	const uint8_t *rp;
	/** @brief Length of @ref rp in bytes. */
	size_t rp_len;
};

/** @brief Pull an HCI_Command_Complete event.
 *
 *  Decodes the event parameters of an HCI_Command_Complete event from @p buf,
 *  which must be positioned at the start of the event parameters, i.e. after
 *  the HCI event header. On success the event parameters preceding the
 *  return parameters have been pulled, leaving @p buf positioned at the
 *  return parameters of the command, which start with the status (as the
 *  @c bt_hci_rp_* structures do).
 *
 *  @param buf Buffer positioned at the event parameters.
 *  @param rsp Decoded response.
 *
 *  @retval 0        Success; @p buf is positioned at the return parameters.
 *  @retval -ENODATA The event responds to a command but carries no return
 *                   parameters, not even the status; @p rsp is filled in
 *                   with @c BT_HCI_ERR_UNSPECIFIED as status and @p buf is
 *                   positioned as on success.
 *  @retval -EINVAL  The event is malformed; @p buf and @p rsp are unchanged.
 */
int bt_hci_pkt_pull_cmd_complete(struct net_buf_simple *buf, struct bt_hci_pkt_cmd_rsp *rsp);

/** @brief Pull an HCI_Command_Status event.
 *
 *  Decodes the event parameters of an HCI_Command_Status event from @p buf,
 *  which must be positioned at the start of the event parameters, i.e. after
 *  the HCI event header. On success the event parameters have been pulled.
 *
 *  @param buf Buffer positioned at the event parameters.
 *  @param rsp Decoded response.
 *
 *  @retval 0       Success; the event parameters have been pulled from @p buf.
 *  @retval -EINVAL The event is malformed; @p buf and @p rsp are unchanged.
 */
int bt_hci_pkt_pull_cmd_status(struct net_buf_simple *buf, struct bt_hci_pkt_cmd_rsp *rsp);

/** @brief Parse an HCI command response from a complete HCI packet.
 *
 *  Examines a complete HCI packet, starting with its packet indicator, and
 *  decodes it when it is an HCI_Command_Complete or HCI_Command_Status
 *  event, leaving the return parameters of the command in @ref
 *  bt_hci_pkt_cmd_rsp.rp. Bytes beyond the length given in the event header
 *  are ignored, and the packet is not modified, so any received packet can be
 *  examined in place.
 *
 *  @param pkt Complete HCI packet.
 *  @param len Length of @p pkt in bytes.
 *  @param rsp Decoded response.
 *
 *  @retval 0        The packet is a command response.
 *  @retval -ENODATA The packet is an HCI_Command_Complete for a command that
 *                   carries no return parameters, not even the status; @p rsp
 *                   is filled in with @c BT_HCI_ERR_UNSPECIFIED as status.
 *  @retval -ENOMSG  The packet is not a command response; @p rsp is unchanged.
 *  @retval -EINVAL  The packet is a malformed HCI event; @p rsp is unchanged.
 */
int bt_hci_pkt_parse_cmd_rsp(const uint8_t *pkt, size_t len, struct bt_hci_pkt_cmd_rsp *rsp);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HCI_PKT_H_ */
