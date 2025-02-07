/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_sock_svc, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/zvfs/eventfd.h>

static int init_socket_service(void);

enum SOCKET_SERVICE_THREAD_STATUS {
	SOCKET_SERVICE_THREAD_UNINITIALIZED = 0,
	SOCKET_SERVICE_THREAD_FAILED,
	SOCKET_SERVICE_THREAD_STOPPED,
	SOCKET_SERVICE_THREAD_RUNNING,
};
static enum SOCKET_SERVICE_THREAD_STATUS thread_status;

static K_MUTEX_DEFINE(lock);
static K_CONDVAR_DEFINE(wait_start);

STRUCT_SECTION_START_EXTERN(net_socket_service_desc);
STRUCT_SECTION_END_EXTERN(net_socket_service_desc);

static struct service {
	struct zsock_pollfd events[CONFIG_ZVFS_POLL_MAX];
	int count;
} ctx;

#define get_idx(svc) (*(svc->idx))

void net_socket_service_foreach(net_socket_service_cb_t cb, void *user_data)
{
	STRUCT_SECTION_FOREACH(net_socket_service_desc, svc) {
		cb(svc, user_data);
	}
}

static void cleanup_svc_events(const struct net_socket_service_desc *svc)
{
	for (int i = 0; i < svc->pev_len; i++) {
		svc->pev[i].event.fd = -1;
		svc->pev[i].event.events = 0;
	}
}

int z_impl_net_socket_service_register(const struct net_socket_service_desc *svc,
				       struct zsock_pollfd *fds, int len,
				       void *user_data)
{
	int i, ret = -ENOENT;

	k_mutex_lock(&lock, K_FOREVER);

	if (thread_status == SOCKET_SERVICE_THREAD_UNINITIALIZED) {
		(void)k_condvar_wait(&wait_start, &lock, K_FOREVER);
	} else if (thread_status != SOCKET_SERVICE_THREAD_RUNNING) {
		NET_ERR("Socket service thread not running, service %p register fails.", svc);
		ret = -EIO;
		goto out;
	}

	if (STRUCT_SECTION_START(net_socket_service_desc) > svc ||
	    STRUCT_SECTION_END(net_socket_service_desc) <= svc) {
		goto out;
	}

	if (fds == NULL) {
		cleanup_svc_events(svc);
	} else {
		if (len > svc->pev_len) {
			NET_DBG("Too many file descriptors, "
				"max is %d for service %p",
				svc->pev_len, svc);
			ret = -ENOMEM;
			goto out;
		}

		for (i = 0; i < len; i++) {
			svc->pev[i].event = fds[i];
			svc->pev[i].user_data = user_data;
		}
	}

	/* Tell the thread to re-read the variables */
	zvfs_eventfd_write(ctx.events[0].fd, 1);
	ret = 0;

out:
	k_mutex_unlock(&lock);

	return ret;
}

static struct net_socket_service_desc *find_svc_and_event(
	struct zsock_pollfd *pev,
	struct net_socket_service_event **event)
{
	STRUCT_SECTION_FOREACH(net_socket_service_desc, svc) {
		for (int i = 0; i < svc->pev_len; i++) {
			if (svc->pev[i].event.fd == pev->fd) {
				*event = &svc->pev[i];
				return svc;
			}
		}
	}

	return NULL;
}

/* We do not set the user callback to our work struct because we need to
 * hook into the flow and restore the global poll array so that the next poll
 * round will not notice it and call the callback again while we are
 * servicing the callback.
 */
void net_socket_service_callback(struct net_socket_service_event *pev)
{
	struct net_socket_service_event ev = *pev;

	ev.callback(&ev);
}

static int call_work(struct zsock_pollfd *pev, struct net_socket_service_event *event)
{
	int ret = 0;
	int fd = pev->fd;

	/* Mark the global fd non pollable so that we do not
	 * call the callback second time.
	 */
	pev->fd = -1;

	/* Synchronous call */
	net_socket_service_callback(event);

	/* Restore the fd so that new data can be re-triggered */
	pev->fd = fd;

	return ret;
}

static int trigger_work(struct zsock_pollfd *pev)
{
	struct net_socket_service_event *event;
	struct net_socket_service_desc *svc;

	svc = find_svc_and_event(pev, &event);
	if (svc == NULL) {
		return -ENOENT;
	}

	event->svc = svc;

	/* Copy the triggered event to our event so that we know what
	 * was actually causing the event.
	 */
	event->event = *pev;

	return call_work(pev, event);
}

static void socket_service_thread(void)
{
	int ret, i, fd, count = 0;
	zvfs_eventfd_t value;

	STRUCT_SECTION_COUNT(net_socket_service_desc, &ret);
	if (ret == 0) {
		NET_INFO("No socket services found, service disabled.");
		goto fail;
	}

	/* Create contiguous poll event array to enable socket polling */
	STRUCT_SECTION_FOREACH(net_socket_service_desc, svc) {
		NET_DBG("Service %s has %d pollable sockets",
			COND_CODE_1(CONFIG_NET_SOCKETS_LOG_LEVEL_DBG,
				    (svc->owner), ("")),
			svc->pev_len);
		get_idx(svc) = count + 1;
		count += svc->pev_len;
	}

	if ((count + 1) > ARRAY_SIZE(ctx.events)) {
		NET_ERR("You have %d services to monitor but "
			"%zd poll entries configured.",
			count + 1, ARRAY_SIZE(ctx.events));
		NET_ERR("Please increase value of %s to at least %d",
			"CONFIG_ZVFS_POLL_MAX", count + 1);
		goto fail;
	}

	NET_DBG("Monitoring %d socket entries", count);

	ctx.count = count + 1;

	/* Create an zvfs_eventfd that can be used to trigger events during polling */
	fd = zvfs_eventfd(0, 0);
	if (fd < 0) {
		fd = -errno;
		NET_ERR("zvfs_eventfd failed (%d)", fd);
		goto out;
	}

	thread_status = SOCKET_SERVICE_THREAD_RUNNING;
	k_condvar_broadcast(&wait_start);

	ctx.events[0].fd = fd;
	ctx.events[0].events = ZSOCK_POLLIN;

restart:
	i = 1;

	k_mutex_lock(&lock, K_FOREVER);

	/* Copy individual events to the big array */
	STRUCT_SECTION_FOREACH(net_socket_service_desc, svc) {
		for (int j = 0; j < svc->pev_len; j++) {
			ctx.events[get_idx(svc) + j] = svc->pev[j].event;
		}
	}

	k_mutex_unlock(&lock);

	while (true) {
		ret = zsock_poll(ctx.events, count + 1, -1);
		if (ret < 0) {
			ret = -errno;
			NET_ERR("poll failed (%d)", ret);
			goto out;
		}

		if (ret == 0) {
			/* should not happen because timeout is -1 */
			break;
		}

		/* Process work here */
		for (i = 1; i < (count + 1); i++) {
			if (ctx.events[i].fd < 0) {
				continue;
			}

			if (ctx.events[i].revents > 0) {
				ret = trigger_work(&ctx.events[i]);
				if (ret < 0) {
					NET_DBG("Triggering work failed (%d)", ret);
					goto restart;
				}
			}
		}

		/* Relocate after trigger work so the work gets done before restarting */
		if (ret > 0 && ctx.events[0].revents) {
			zvfs_eventfd_read(ctx.events[0].fd, &value);
			ctx.events[0].revents = 0;
			NET_DBG("Received restart event.");
			goto restart;
		}
	}

out:
	NET_DBG("Socket service thread stopped");
	thread_status = SOCKET_SERVICE_THREAD_STOPPED;

	return;

fail:
	thread_status = SOCKET_SERVICE_THREAD_FAILED;
	k_condvar_broadcast(&wait_start);
}

static int init_socket_service(void)
{
	k_tid_t ssm;
	static struct k_thread service_thread;

	static K_THREAD_STACK_DEFINE(service_thread_stack,
				     CONFIG_NET_SOCKETS_SERVICE_STACK_SIZE);

	ssm = k_thread_create(&service_thread,
			      service_thread_stack,
			      K_THREAD_STACK_SIZEOF(service_thread_stack),
			      (k_thread_entry_t)socket_service_thread, NULL, NULL, NULL,
			      CLAMP(CONFIG_NET_SOCKETS_SERVICE_THREAD_PRIO,
				    K_HIGHEST_APPLICATION_THREAD_PRIO,
				    K_LOWEST_APPLICATION_THREAD_PRIO), 0, K_NO_WAIT);

	k_thread_name_set(ssm, "net_socket_service");

	return 0;
}

void socket_service_init(void)
{
	(void)init_socket_service();
}
