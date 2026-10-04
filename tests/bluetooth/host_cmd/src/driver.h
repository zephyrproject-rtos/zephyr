/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_BLUETOOTH_HOST_CMD_SRC_DRIVER_H_
#define TESTS_BLUETOOTH_HOST_CMD_SRC_DRIVER_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

/* A vendor command that only the tests send */
#define TEST_OPCODE BT_OP(BT_OGF_VS, 0x03f0U)

/* How long a test waits for something that happens without time passing, as
 * a response from within send() or a work item that is due: the bound only
 * turns a hang into a failure.
 */
#define TEST_TIMEOUT K_SECONDS(1)

/* The command deadline of the Host in milliseconds. HCI_CMD_TIMEOUT comes from
 * hci_core.h, which the sources include themselves: it has no include guard.
 */
#define TEST_DEADLINE_MS ((int64_t)k_ticks_to_ms_floor64(HCI_CMD_TIMEOUT.ticks))

/* How far a measured wait for the deadline may be off: more than the few
 * ticks that the Host takes to act on the expiry and the waiter to return.
 */
#define TEST_DEADLINE_SLACK_MS 100U

/* A wait that is to end with the deadline gives up this much later */
#define TEST_PAST_DEADLINE K_MSEC(TEST_DEADLINE_MS + MSEC_PER_SEC)

/* More than the Host has command buffers in any configuration of the suites */
#define TEST_CMD_BUF_MAX 32U

/* How many TEST_OPCODE commands the controller keeps the parameter length of */
#define TEST_CMD_PLEN_MAX 4U

/* The handle of the connection that test_connect() brings up */
#define TEST_CONN_HANDLE 0x0001U

/** What the test controller does with one command. It responds to every
 *  other command with success and one command credit from within send().
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
	/** The return parameters repeat the first parameter byte of the command
	 *  instead of counting up (TEST_OPCODE only)
	 */
	bool echo;
	/** What send() returns instead of taking the command, if not 0 */
	int send_err;
	/** How often the controller has been given the command */
	unsigned int count;
	/** How often the controller has responded to the command */
	unsigned int rsp_count;
};

/** TEST_OPCODE, whose response is a Command Complete event */
extern struct test_cmd_script test_cmd;

/** Parameter length of the first few TEST_OPCODE commands the controller got */
extern uint8_t test_cmd_plen[TEST_CMD_PLEN_MAX];

/** HCI Disconnect, whose response is a Command Status event, followed by a
 *  Disconnection Complete event when the status is success.
 */
extern struct test_cmd_script test_disconnect;

/** How often the controller has been reset and the transport closed */
extern unsigned int test_reset_count;
extern unsigned int test_close_count;

/** Back to a controller that responds at once, counters at zero */
void test_driver_reset(void);

/** As test_driver_reset(), and respond to a command that the controller holds
 *  and grant a command credit: what a failed test leaves for bt_disable(),
 *  whose HCI Reset is to get through.
 */
void test_driver_release(void);

/** Deliver the response of a scripted command now */
void test_cmd_respond(struct test_cmd_script *script);

/** Wait until the controller has been given @p count commands of @p script.
 *
 *  @retval 0 It has.
 *  @retval -EAGAIN It has not within TEST_TIMEOUT of the last one.
 */
int test_cmd_wait(const struct test_cmd_script *script, unsigned int count);

/** Wait until the controller has responded @p count times to the commands of
 *  @p script.
 *
 *  @retval 0 It has.
 *  @retval -EAGAIN It has not within TEST_TIMEOUT of the last response.
 */
int test_rsp_wait(const struct test_cmd_script *script, unsigned int count);

/** Wait until @p queue has run what has been submitted to it */
void test_workq_flush(struct k_work_q *queue);

/** Wait until the Bluetooth workqueue has run what has been submitted to it */
void test_bt_workq_flush(void);

/** Let every thread run until it waits, for a check of something that nothing
 *  signals: a command that must not have been sent, or a thread that is to
 *  have blocked.
 */
void test_settle(void);

/** Command buffers that a test holds, NULL where none is held */
extern struct net_buf *test_cmd_bufs[TEST_CMD_BUF_MAX];

/** Take every free command buffer into test_cmd_bufs.
 *
 *  @return How many there were.
 */
unsigned int test_cmd_bufs_hold(void);

/** Free the command buffers in test_cmd_bufs */
void test_cmd_bufs_release(void);

/** Check that the command pool has all of its buffers, and counts them right */
void test_cmd_pool_check(void);

/** Deliver a Command Complete event for no command that grants a command credit */
void test_credit_grant(void);

/** Deliver a Command Complete event for no command that takes the credit back */
void test_credit_revoke(void);

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
