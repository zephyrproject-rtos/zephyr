/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2025, Joakim Andersson
 */

/**
 * @addtogroup t_modem_driver
 * @{
 * @defgroup t_modem_socket test_modem_socket
 * @}
 */

#include <zephyr/ztest.h>
#include "modem_socket.h"

#define MODEM_SOCKETS_MAX 3
#define MODEM_SOCKETS_BASE_NUM 0
struct modem_socket_config socket_config;
struct modem_socket sockets[MODEM_SOCKETS_MAX];

static const struct socket_op_vtable socket_fd_op_vtable = {
};

ZTEST(modem_socket, test_modem_socket_init_fd_zero)
{
	struct modem_socket *sock;
	int ret;

	ret = modem_socket_init(&socket_config, sockets, MODEM_SOCKETS_MAX, MODEM_SOCKETS_BASE_NUM,
				false, &socket_fd_op_vtable);
	zassert_ok(ret);

	/* Check that fd 0 does not return a modem socket object. */
	sock = modem_socket_from_fd(&socket_config, 0);

	zassert_is_null(sock);
}

ZTEST(modem_socket, test_modem_socket_init_not_allocated)
{
	int ret;

	ret = modem_socket_init(&socket_config, sockets, MODEM_SOCKETS_MAX, MODEM_SOCKETS_BASE_NUM,
				false, &socket_fd_op_vtable);
	zassert_ok(ret);

	for (int i = 0; i < ARRAY_SIZE(sockets); i++) {
		bool result;

		result = modem_socket_is_allocated(&socket_config, &sockets[i]);
		zassert_false(result);
	}
}

ZTEST(modem_socket, test_modem_socket_init_not_assigned)
{
	int ret;

	ret = modem_socket_init(&socket_config, sockets, MODEM_SOCKETS_MAX, MODEM_SOCKETS_BASE_NUM,
				false, &socket_fd_op_vtable);
	zassert_ok(ret);

	for (int i = 0; i < ARRAY_SIZE(sockets); i++) {
		bool result;

		result = modem_socket_id_is_assigned(&socket_config, &sockets[i]);
		zassert_false(result);
	}
}

ZTEST(modem_socket, test_modem_socket_init_not_assigned_dynamic)
{
	int ret;

	ret = modem_socket_init(&socket_config, sockets, MODEM_SOCKETS_MAX, MODEM_SOCKETS_BASE_NUM,
				true, &socket_fd_op_vtable);
	zassert_ok(ret);

	for (int i = 0; i < ARRAY_SIZE(sockets); i++) {
		bool result;

		result = modem_socket_id_is_assigned(&socket_config, &sockets[i]);
		zassert_false(result);
	}
}

static void test_modem_get_put_all(void)
{
	int fds[ARRAY_SIZE(sockets)];

	for (int i = 0; i < ARRAY_SIZE(sockets); i++) {
		fds[i] = modem_socket_get(&socket_config, AF_INET, SOCK_DGRAM, IPPROTO_TCP);
		zassert_false(fds[i] < 0);
	}

	for (int i = 0; i < ARRAY_SIZE(sockets); i++) {
		modem_socket_put(&socket_config, fds[i]);
		/* Returned file description is freed by zsock_close, so we need to free it here. */
		zvfs_free_fd(fds[i]);
	}
}

ZTEST(modem_socket, test_modem_socket_get_put_fd_zero)
{
	struct modem_socket *sock;
	int ret;

	ret = modem_socket_init(&socket_config, sockets, MODEM_SOCKETS_MAX, MODEM_SOCKETS_BASE_NUM,
				false, &socket_fd_op_vtable);
	zassert_ok(ret);

	test_modem_get_put_all();

	/* Check that fd 0 does not return a modem socket object. */
	sock = modem_socket_from_fd(&socket_config, 0);
	zassert_is_null(sock);
}

ZTEST(modem_socket, test_modem_socket_get_put_not_allocated)
{
	int ret;

	ret = modem_socket_init(&socket_config, sockets, MODEM_SOCKETS_MAX, MODEM_SOCKETS_BASE_NUM,
				false, &socket_fd_op_vtable);
	zassert_ok(ret);

	test_modem_get_put_all();

	for (int i = 0; i < ARRAY_SIZE(sockets); i++) {
		bool result;

		result = modem_socket_is_allocated(&socket_config, &sockets[i]);
		zassert_false(result);
	}
}

ZTEST(modem_socket, test_modem_socket_get_put_not_assigned)
{
	int ret;

	ret = modem_socket_init(&socket_config, sockets, MODEM_SOCKETS_MAX, MODEM_SOCKETS_BASE_NUM,
				false, &socket_fd_op_vtable);
	zassert_ok(ret);

	test_modem_get_put_all();

	for (int i = 0; i < ARRAY_SIZE(sockets); i++) {
		bool result;

		result = modem_socket_id_is_assigned(&socket_config, &sockets[i]);
		zassert_false(result);
	}
}

ZTEST(modem_socket, test_modem_socket_get_put_not_assigned_dynamic)
{
	int ret;

	test_modem_get_put_all();

	ret = modem_socket_init(&socket_config, sockets, MODEM_SOCKETS_MAX, MODEM_SOCKETS_BASE_NUM,
				true, &socket_fd_op_vtable);
	zassert_ok(ret);

	for (int i = 0; i < ARRAY_SIZE(sockets); i++) {
		bool result;

		result = modem_socket_id_is_assigned(&socket_config, &sockets[i]);
		zassert_false(result);
	}
}

ZTEST_SUITE(modem_socket, NULL, NULL, NULL, NULL, NULL);
