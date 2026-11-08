/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* The Controller behind the zephyr,bt-hci-test HCI driver.
 *
 * It answers every command with a Command Complete event: the commands that the Host reads the
 * Controller's capabilities with get the values of fake_hci, any other command gets success and
 * zeroed return parameters. Host Number Of Completed Packets gets no response. A test changes
 * the values before bt_enable() and takes the commands that it is about through
 * fake_hci_cb_set().
 */

#ifndef ZEPHYR_TESTS_BLUETOOTH_COMMON_FAKE_HCI_INCLUDE_FAKE_HCI_H_
#define ZEPHYR_TESTS_BLUETOOTH_COMMON_FAKE_HCI_INCLUDE_FAKE_HCI_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/net_buf.h>

/* What the Controller reports to the Host */
struct fake_hci_ctlr {
	/* Read BD_ADDR. The default, all zeros, is no public address. */
	bt_addr_t bd_addr;
	/* Read Local Supported Features. The default is LE supported, BR/EDR not supported. */
	uint8_t features[8];
	/* LE Read Local Supported Features. The default is none. */
	uint8_t le_features[8];
	/* LE Read Buffer Size. The default is one buffer of 27 bytes. */
	uint16_t le_acl_len;
	uint8_t le_acl_num;
};

extern struct fake_hci_ctlr fake_hci;

struct fake_hci_cb {
	/* A command from the Host, in the context of the driver's send(). buf holds the
	 * parameters. Returns true when the test has answered the command, or answers it later
	 * or never, and false for the Controller to answer it.
	 */
	bool (*cmd)(uint16_t opcode, struct net_buf *buf);
};

/* Set the callbacks of the test, or clear them with NULL */
void fake_hci_cb_set(const struct fake_hci_cb *cb);

/* Each of these allocates an event and adds its headers. The caller adds the len bytes of
 * parameters that follow and passes the event to fake_hci_recv().
 */
struct net_buf *fake_hci_evt(uint8_t evt, uint8_t len);
struct net_buf *fake_hci_le_meta_evt(uint8_t subevent, uint8_t len);
struct net_buf *fake_hci_cmd_complete(uint16_t opcode, uint8_t len);

/* Send a Command Status event, as from the command callback */
void fake_hci_cmd_status(uint16_t opcode, uint8_t status);

/* Pass a packet to the Host. When it returns to the test thread, the Host has handled the
 * packet.
 */
void fake_hci_recv(struct net_buf *buf);

#endif /* ZEPHYR_TESTS_BLUETOOTH_COMMON_FAKE_HCI_INCLUDE_FAKE_HCI_H_ */
