/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_BLUETOOTH_CLASSIC_OBEX_SRC_TEST_LINK_H_
#define TESTS_BLUETOOTH_CLASSIC_OBEX_SRC_TEST_LINK_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/classic/obex.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

/* MTU of both sides of the fake transport, also used as MOPL */
#define TEST_MTU 512U

/* One side of the fake OBEX transport */
struct test_link {
	struct bt_obex obex;
	/* Packets sent on this side and not delivered to the peer yet */
	struct k_fifo txq;
	size_t pending;
	/* Error returned by send() instead of queuing the packet, 0 to queue it */
	int send_err;
	/* Whether alloc_buf() fails */
	bool alloc_fail;
};

/* OBEX client with the records of its callbacks */
struct test_client {
	struct bt_obex_client client;
	/* Connection Id added to each request if use_conn_id is set */
	uint32_t conn_id;
	bool use_conn_id;
	size_t connect_count;
	uint8_t connect_rsp;
	size_t action_count;
	uint8_t action_rsp;
	size_t get_count;
	uint8_t get_rsp;
};

/* OBEX server with the records of its callbacks */
struct test_server {
	struct bt_obex_server server;
	size_t connect_count;
	size_t action_count;
	bool action_final;
	uint8_t action_id;
	size_t get_count;
	bool get_final;
};

/* Data pointer and length of a buffer, to check that a failed call left it unchanged */
struct test_buf_state {
	const uint8_t *data;
	uint16_t len;
};

/* Side of the clients */
extern struct test_link cli_link;
/* Side of the servers */
extern struct test_link srv_link;

/* Initialize both sides and save the free buffer counts of the test pools */
void test_link_setup(void);
/* Release the queued packets, for the after hook of a suite */
void test_link_cleanup(void);
/* Check that no packet is queued and that the test pools have no leaked buffer */
void test_link_check_idle(void);

/* Number of packets queued on one side */
size_t test_link_pending(struct test_link *link);
/* Deliver the next queued packet of `from` to `to` */
void test_link_pump(struct test_link *from, struct test_link *to);

/* Buffer with the head room reserved for OBEX, like the GOEP transport buffers */
struct net_buf *test_buf(void);
/* Buffer without any head room */
struct net_buf *test_buf_no_headroom(void);
/* Add the headers of a Copy action request (and the Connection Id of the client) */
void test_add_action_headers(struct net_buf *buf, const struct test_client *c);
/* Buffer holding the headers of a Copy action request */
struct net_buf *test_action_req(const struct test_client *c);

void test_buf_state_save(const struct net_buf *buf, struct test_buf_state *state);
void test_buf_state_check(const struct net_buf *buf, const struct test_buf_state *state);

/* Clear the records and attach the client to cli_link */
void test_client_init(struct test_client *c);
/* Clear the records and attach the server to srv_link */
void test_server_init(struct test_server *s);

/* OBEX connect, with the Target, Who and Connection Id headers if `target` is not NULL */
void test_connect(struct test_client *c, struct test_server *s, const uint8_t *target,
		  uint32_t conn_id);

/* Send an action request, the buffer is released if the call fails */
int test_send_action(struct test_client *c, bool final);
/* Non-final action request answered with Continue */
void test_continue_action(struct test_client *c, struct test_server *s);
/* Final action request answered with Success */
void test_finish_action(struct test_client *c, struct test_server *s);
/* Single packet GET operation with SRM enabled by both sides */
void test_srm_get(struct test_client *c, struct test_server *s);

#endif /* TESTS_BLUETOOTH_CLASSIC_OBEX_SRC_TEST_LINK_H_ */
