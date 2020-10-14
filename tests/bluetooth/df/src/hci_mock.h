/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BLUETOOTH_DF_HCI_MOCK_H_
#define ZEPHYR_TESTS_BLUETOOTH_DF_HCI_MOCK_H_

#include <stdint.h>

/* Command handler structure for cmd_handle(). */
struct hci_cmd_handler {
	uint16_t opcode;
	uint8_t rp_len;
	void (*handl_cb)(uint16_t opcode, struct net_buf *buf,
			 struct net_buf **evt, uint8_t rp_len);
};

/* @brief Function sets HCI command handlers that will be used
 * during unit tests to simulate BLE Controller.
 *
 * @param[in] handlers		Pointer to handlers array.
 * @param[in] num_handlers	Length of handlers array.
 */
void hci_set_handlers(const struct hci_cmd_handler *handlers,
		     uint32_t num_handlers);

/* @brief Function prepares comman complete event.
 *
 * Prepared event is associated with given opcode and has size
 * requested by response length.
 *
 * @param[in] opcode	Opcode of a command to prepare completion even.
 * @param[out] buf	Pointer for an event memory.
 * @param[in] rp_len	Length of response event.
 *
 * @return Pointer to beginning of the response memory.
 */
void *hci_cmd_complete(uint16_t opcode, struct net_buf **buf, uint8_t rp_len);

/* @brief The function is a generic handler for command complete
 *  with success status.
 *
 *  All event parameters are set to zeros.
 *
 * @param[in] opcode	Opcode of request command
 * @param[in] cmd	Pointer to requect command
 * @param[out] evt	Pointer to memory where to store response event
 * @param[in] rp_len	Length of response event
 */
void hci_cmd_complete_success(uint16_t opcode, struct net_buf *cmd,
			      struct net_buf **evt, uint8_t rp_len);

/* @brief Function is a generic test handler for HCI commands.
 *
 * Function handles HCI commands by common definded commands or
 * user defined command handlers.
 * User defined command handler has priority over common handlers.
 *
 * @param[in] cmd	Pointer to command memory.
 *
 * @retval NULL		Null poitner if the message does not provide
 *			reposnse.
 */
struct net_buf *hci_cmd_handle(struct net_buf *cmd);

#endif /* ZEPHYR_TESTS_BLUETOOTH_DF_HCI_MOCK_H_ */
