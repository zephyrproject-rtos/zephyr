/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Object core support for sockets */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_sock, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <zephyr/kernel.h>

#include "sockets_internal.h"
#include "../../ip/net_private.h"

static struct k_obj_type sock_obj_type;
static K_MUTEX_DEFINE(sock_obj_mutex);

/* Allocate some extra socket objects so that we can track
 * closed sockets and get some historical statistics.
 */
static struct sock_obj sock_objects[CONFIG_ZVFS_OPEN_MAX * 2] = {
	[0 ... ((CONFIG_ZVFS_OPEN_MAX * 2) - 1)] = {
		.fd = -1,
		.init_done = false,
	}
};

static void sock_obj_core_init_and_link(struct sock_obj *sock);
static int sock_obj_core_stats_reset(struct k_obj_core *obj);
static int sock_obj_stats_raw(struct k_obj_core *obj_core, void *stats);
static int sock_obj_core_get_reg_and_proto(int sock,
					   struct net_socket_register **reg);

struct k_obj_core_stats_desc sock_obj_type_stats_desc = {
	.raw_size = sizeof(struct sock_obj_type_raw_stats),
	.raw = sock_obj_stats_raw,
	.reset = sock_obj_core_stats_reset,
	.disable = NULL,    /* Stats gathering is always on */
	.enable = NULL,     /* Stats gathering is always on */
};

static void set_fields(struct sock_obj *obj, int fd,
		       struct net_socket_register *reg,
		       int family, int type, int proto)
{
	obj->fd = fd;
	obj->socket_family = family;
	obj->socket_type = type;
	obj->socket_proto = proto;
	obj->reg = reg;
	obj->creator = k_current_get();
	obj->create_time = sys_clock_tick_get();
}

static void sock_obj_core_init_and_link(struct sock_obj *sock)
{
	static bool type_init_done;

	if (!type_init_done) {
		z_obj_type_init(&sock_obj_type, K_OBJ_TYPE_SOCK,
				offsetof(struct sock_obj, obj_core));
		k_obj_type_stats_init(&sock_obj_type, &sock_obj_type_stats_desc);

		type_init_done = true;
	}

	/* If the socket was closed and we re-opened it again, then clear
	 * the statistics.
	 */
	if (sock->init_done) {
		k_obj_core_stats_reset(K_OBJ_CORE(sock));
	} else {
		k_obj_core_init_and_link(K_OBJ_CORE(sock), &sock_obj_type);
		k_obj_core_stats_register(K_OBJ_CORE(sock), &sock->stats,
					  sizeof(struct sock_obj_type_raw_stats));
	}

	sock->init_done = true;
}

static int sock_obj_stats_raw(struct k_obj_core *obj_core, void *stats)
{
	memcpy(stats, obj_core->stats, sizeof(struct sock_obj_type_raw_stats));

	return 0;
}

static int sock_obj_core_stats_reset(struct k_obj_core *obj_core)
{
	memset(obj_core->stats, 0, sizeof(struct sock_obj_type_raw_stats));

	return 0;
}

static int sock_obj_core_get_reg_and_proto(int sock, struct net_socket_register **reg)
{
	int i, ret;

	k_mutex_lock(&sock_obj_mutex, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(sock_objects); i++) {
		if (sock_objects[i].fd == sock) {
			*reg = sock_objects[i].reg;
			ret = sock_objects[i].socket_proto;
			goto out;
		}
	}

	ret = -ENOENT;

out:
	k_mutex_unlock(&sock_obj_mutex);

	return ret;
}

int sock_obj_core_alloc(int sock, struct net_socket_register *reg,
			int family, int type, int proto)
{
	struct sock_obj *obj = NULL;
	int ret, i;

	if (sock < 0) {
		return -EINVAL;
	}

	k_mutex_lock(&sock_obj_mutex, K_FOREVER);

	/* Try not to allocate already closed sockets so that we
	 * can see historical data.
	 */
	for (i = 0; i < ARRAY_SIZE(sock_objects); i++) {
		if (sock_objects[i].fd < 0) {
			if (sock_objects[i].init_done == false) {
				obj = &sock_objects[i];
				break;
			} else if (obj == NULL) {
				obj = &sock_objects[i];
			}
		}
	}

	if (obj == NULL) {
		ret = -ENOENT;
		goto out;
	}

	set_fields(obj, sock, reg, family, type, proto);
	sock_obj_core_init_and_link(obj);

	ret = 0;

out:
	k_mutex_unlock(&sock_obj_mutex);

	return ret;
}

int sock_obj_core_alloc_find(int sock, int new_sock, int type)
{
	struct net_socket_register *reg = NULL;
	socklen_t optlen = sizeof(int);
	int family;
	int ret;

	if (new_sock < 0) {
		return -EINVAL;
	}

	ret = sock_obj_core_get_reg_and_proto(sock, &reg);
	if (ret < 0) {
		goto out;
	}

	ret = zsock_getsockopt(sock, SOL_SOCKET, SO_DOMAIN, &family, &optlen);
	if (ret < 0) {
		NET_ERR("Cannot get socket domain (%d)", -errno);
		goto out;
	}

	ret = sock_obj_core_alloc(new_sock, reg, family, type, ret);
	if (ret < 0) {
		NET_ERR("Cannot allocate core object for socket %d (%d)",
			new_sock, ret);
	}

out:
	return ret;
}

int sock_obj_core_dealloc(int fd)
{
	int ret;

	k_mutex_lock(&sock_obj_mutex, K_FOREVER);

	for (int i = 0; i < ARRAY_SIZE(sock_objects); i++) {
		if (sock_objects[i].fd == fd) {
			sock_objects[i].fd = -1;

			/* Calculate the lifetime of the socket so that
			 * net-shell can print it for the closed sockets.
			 */
			sock_objects[i].create_time =
				k_ticks_to_ms_ceil32(sys_clock_tick_get() -
						     sock_objects[i].create_time);
			ret = 0;
			goto out;
		}
	}

	ret = -ENOENT;

out:
	k_mutex_unlock(&sock_obj_mutex);

	return ret;
}

void sock_obj_core_update_send_stats(int fd, int bytes)
{
	if (bytes > 0) {
		k_mutex_lock(&sock_obj_mutex, K_FOREVER);

		for (int i = 0; i < ARRAY_SIZE(sock_objects); i++) {
			if (sock_objects[i].fd == fd) {
				sock_objects[i].stats.sent += bytes;
				break;
			}
		}

		k_mutex_unlock(&sock_obj_mutex);
	}
}

void sock_obj_core_update_recv_stats(int fd, int bytes)
{
	if (bytes > 0) {
		k_mutex_lock(&sock_obj_mutex, K_FOREVER);

		for (int i = 0; i < ARRAY_SIZE(sock_objects); i++) {
			if (sock_objects[i].fd == fd) {
				sock_objects[i].stats.received += bytes;
				break;
			}
		}

		k_mutex_unlock(&sock_obj_mutex);
	}
}
