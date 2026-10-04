/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_BLUETOOTH_HOST_CMD_SRC_DRIVER_H_
#define TESTS_BLUETOOTH_HOST_CMD_SRC_DRIVER_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

/* A vendor command that only the tests send */
#define TEST_OPCODE BT_OP(BT_OGF_VS, 0x03f0)

/* The handle of the connection that test_connect() brings up */
#define TEST_CONN_HANDLE 0x0001U

/** What the test controller does with one command. Every other command is
 *  answered with success and one command credit from within send().
 */
struct test_cmd_script {
	/** When the response comes: K_NO_WAIT from within send(), K_FOREVER never */
	k_timeout_t delay;
	/** Status of the response */
	uint8_t status;
	/** Num_HCI_Command_Packets of the response */
	uint8_t ncmd;
	/** Length of the return parameters after the status (TEST_OPCODE only) */
	uint8_t rp_len;
	/** What send() returns instead of taking the command, if not 0 */
	int send_err;
	/** How often the controller has been given the command */
	unsigned int count;
};

/** TEST_OPCODE, answered with a Command Complete event */
extern struct test_cmd_script test_cmd;

/** Parameter length of the first few TEST_OPCODE commands the controller got */
extern uint8_t test_cmd_plen[4];

/** HCI Disconnect, answered with a Command Status event and, when the status
 *  is success, a Disconnection Complete event.
 */
extern struct test_cmd_script test_disconnect;

/** How often the controller has been reset and the transport closed */
extern unsigned int test_reset_count;
extern unsigned int test_close_count;

/** Back to a controller that answers everything at once, counters at zero */
void test_driver_reset(void);

/** Deliver the response of a scripted command now */
void test_cmd_respond(const struct test_cmd_script *script);

/** Deliver a Command Complete event for no command that grants a command credit */
void test_credit_grant(void);

/** Whether the Host resets the test controller, which it does unless the
 *  devicetree gives the controller the "no-reset" quirk.
 */
#define TEST_DRIVER_RESETS (!DT_NODE_HAS_PROP(DT_NODELABEL(bt_hci_test), bt_hci_quirks))

/** Suite predicates: most suites need the reset, one cannot do with it */
bool test_with_reset(const void *state);
bool test_without_reset(const void *state);

/** Deliver an LE Connection Complete event for TEST_CONN_HANDLE, peripheral role */
void test_connect(void);

/** Deliver a Disconnection Complete event for TEST_CONN_HANDLE */
void test_disconnect_complete(uint8_t reason);

#endif /* TESTS_BLUETOOTH_HOST_CMD_SRC_DRIVER_H_ */
