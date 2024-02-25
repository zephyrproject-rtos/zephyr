/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include "net_shell_private.h"
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>

#if defined(CONFIG_NET_SOCKETS_OBJ_CORE)
struct socket_info {
	int opened;
	int closed;
};

int walk_sockets(struct k_obj_core *obj_core, void *user_data)
{
#if defined(CONFIG_THREAD_NAME)
#define THREAD_NAME_LEN CONFIG_THREAD_MAX_NAME_LEN
#else
#define THREAD_NAME_LEN 23
#endif
	struct sock_obj_type_raw_stats stats = { 0 };
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct socket_info *count = data->user_data;
	char thread_name[THREAD_NAME_LEN + 1];
	char fd[5] = { 0 };
	struct sock_obj *obj;
	int lifetime;
	int ret;

	obj = CONTAINER_OF(obj_core, struct sock_obj, obj_core);

	if (k_thread_name_copy(obj->creator, thread_name,
			       sizeof(thread_name) - 1) < 0) {
		snprintk(thread_name, sizeof(thread_name) - 1, "%p",
			 obj->creator);
	}

	thread_name[sizeof(thread_name) - 1] = '\0';

	ret = k_obj_core_stats_raw(K_OBJ_CORE(obj),
				   &stats, sizeof(stats));
	if (ret != 0) {
		PR_INFO("Failed to get statistics (%d)\n", ret);
	}

	if (obj->fd < 0) {
		/* Already closed socket. The create time contains the
		 * actual lifetime as calculated in close()
		 */
		lifetime = obj->create_time;
		fd[0] = 'C';
		fd[1] = '\0';
		count->closed++;
	} else {
		lifetime = k_ticks_to_ms_ceil32(sys_clock_tick_get() -
						obj->create_time);
		snprintk(fd, sizeof(fd), "%d", obj->fd);
		count->opened++;
	}

	PR("%25s  %-12s  %c%c%c\t%-5s%-13d   %-10" PRId64 "%-10" PRId64 "\n",
	   thread_name, obj->reg->name,
	   obj->socket_family == AF_INET6 ? '6' :
	   (obj->socket_family == AF_INET ? '4' :
	    (obj->socket_family == AF_NET_MGMT ? 'M' : ' ')),
	   obj->socket_type == SOCK_DGRAM ? 'D' :
	   (obj->socket_type == SOCK_STREAM ? 'S' :
	    (obj->socket_type == SOCK_RAW ? 'R' : ' ')),
	   obj->socket_proto == IPPROTO_UDP ? 'U' :
	   (obj->socket_proto == IPPROTO_TCP ? 'T' : ' '),
	   fd, lifetime, stats.sent, stats.received);

	return 0;
}
#endif /* CONFIG_NET_SOCKETS_OBJ_CORE */

#if defined(CONFIG_NET_SOCKETS_SERVICE)

#if CONFIG_NET_SOCKETS_LOG_LEVEL >= LOG_LEVEL_DBG
#define MAX_OWNER_LEN 32
#else
#define MAX_OWNER_LEN sizeof("<unknown>")
#endif

static void walk_socket_services(const struct net_socket_service_desc *svc,
				 void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	int len = 0;
	static char pev_output[sizeof("xxx,") * CONFIG_NET_SOCKETS_POLL_MAX];
	static char owner[MAX_OWNER_LEN + 1];

	NET_ASSERT(svc->pev != NULL);

	for (int i = 0; i < svc->pev_len; i++) {
		len += snprintk(pev_output + len, sizeof(pev_output) - len,
				"%d,", svc->pev[i].event.fd);
	}

	if (len > 0) {
		pev_output[len - 1] = '\0';
	}

#if CONFIG_NET_SOCKETS_LOG_LEVEL >= LOG_LEVEL_DBG
	len = strlen(svc->owner);

	int offset = len > sizeof(owner) ?
			len -= (sizeof(owner) - 3) : 0;

	snprintk(owner, sizeof(owner), "%s%s",
		 offset == 0 ? "" : "...",
		 svc->owner + offset + 1);
#else
	snprintk(owner, sizeof(owner), "<unknown>");
#endif

	PR("%32s  %-6s  %-5d %s\n",
	   owner,
	   svc->pev->work.handler == NULL ? "SYNC" : "ASYNC",
	   svc->pev_len, pev_output);

	(*count)++;
}
#endif /* CONFIG_NET_SOCKETS_SERVICE */

static int cmd_net_sockets(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_SOCKETS_OBJ_CORE)
	struct net_shell_user_data user_data;
	struct k_obj_type *obj_type;
	struct socket_info count = { 0 };

	user_data.sh = sh;
	user_data.user_data = &count;

	PR("%25s  %-12s  %-5s\t%-5s%-14s  %-10s%-10s\n",
	   "Creator", "Name", "Flags", "FD", "Lifetime (ms)", "Sent",
	   "Received");
	PR("\n");

	obj_type = k_obj_type_find(K_OBJ_TYPE_SOCK);
	if (obj_type != NULL) {
		k_obj_type_walk_unlocked(obj_type, walk_sockets, (void *)&user_data);
	}

	if (count.opened == 0 && count.closed == 0) {
		PR("No sockets found.\n");
	} else {
		if (count.opened > 0) {
			PR("\n%d active socket%s found.\n", count.opened,
			   count.opened == 1 ? "" : "s");
		}

		if (count.closed > 0) {
			if (count.opened == 0) {
				PR("\n");
			}

			PR("%d closed socket%s found.\n", count.closed,
			   count.closed == 1 ? "" : "s");
		}
	}

#if defined(CONFIG_NET_SOCKETS_SERVICE)
	PR("\n");
#endif
#endif

#if defined(CONFIG_NET_SOCKETS_SERVICE)
	struct net_shell_user_data svc_user_data;
	int svc_count = 0;

	svc_user_data.sh = sh;
	svc_user_data.user_data = &svc_count;

	PR("Services:\n");
	PR("%32s  %-6s  %-5s %s\n",
	   "Owner", "Mode", "Count", "FDs");
	PR("\n");

	net_socket_service_foreach(walk_socket_services, (void *)&svc_user_data);

	if (svc_count == 0) {
		PR("No socket services found.\n");
	} else {
		PR("\n%d socket service%s found.\n", svc_count,
		   svc_count == 1 ? "" : "s");
	}

#if !defined(CONFIG_NET_SOCKETS_OBJ_CORE)
	PR("\n");
#endif
#endif

#if !defined(CONFIG_NET_SOCKETS_OBJ_CORE)
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_OBJ_CORE and CONFIG_NET_SOCKETS_OBJ_CORE",
		"socket information");
#endif
#if !defined(CONFIG_NET_SOCKETS_SERVICE)
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Socket service not supported.\n");
#endif

	return 0;
}

SHELL_SUBCMD_ADD((net), sockets, NULL,
		 "Show network sockets.",
		 cmd_net_sockets, 1, 0);
