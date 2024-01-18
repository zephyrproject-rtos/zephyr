/**
 * @file
 * @brief BSD Socket service API
 *
 * API can be used to install a k_work that is called
 * if there is data received to a socket.
 */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKET_SERVICE_H_
#define ZEPHYR_INCLUDE_NET_SOCKET_SERVICE_H_

/**
 * @brief BSD socket service API
 * @defgroup bsd_socket_service BSD socket service API
 * @ingroup networking
 * @{
 */

#include <sys/types.h>
#include <zephyr/types.h>
#include <zephyr/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This struct contains information which socket triggered
 * calls to the callback function.
 */
struct net_socket_service_event {
	/** k_work that is done when there is desired activity in file descriptor. */
	struct k_work work;
	/** Callback to be called for desired socket activity */
	k_work_handler_t callback;
	/** Socket information that triggered this event. */
	struct zsock_pollfd event;
	/** User data */
	void *user_data;
	/** Service back pointer */
	struct net_socket_service_desc *svc;
};

/**
 * Main structure holding socket service configuration information.
 * The k_work item is created so that when there is data coming
 * to those fds, the k_work callback is then called.
 * The workqueue can be set NULL in which case system workqueue is used.
 * The service descriptor should be created at built time, and then used
 * as a parameter to register the sockets to be monitored.
 * User should create needed sockets and then setup the poll struct and
 * then register the sockets to be monitored at runtime.
 */
struct net_socket_service_desc {
#if CONFIG_NET_SOCKETS_LOG_LEVEL >= LOG_LEVEL_DBG
	/**
	 * Owner name. This can be used in debugging to see who has
	 * registered this service.
	 */
	const char *owner;
#endif
	/** Workqueue where the work is submitted. */
	struct k_work_q *work_q;
	/** Pointer to the list of services that we are listening */
	struct net_socket_service_event *pev;
	/** Length of the pollable socket array for this service. */
	int pev_len;
	/** Where are my pollfd entries in the global list */
	int *idx;
};

#define __z_net_socket_svc_get_name(_svc_id) __z_net_socket_service_##_svc_id
#define __z_net_socket_svc_get_idx(_svc_id) __z_net_socket_service_idx_##_svc_id
#define __z_net_socket_svc_get_owner __FILE__ ":" STRINGIFY(__LINE__)

extern void net_socket_service_callback(struct k_work *work);

#if CONFIG_NET_SOCKETS_LOG_LEVEL >= LOG_LEVEL_DBG
#define NET_SOCKET_SERVICE_OWNER .owner = __z_net_socket_svc_get_owner,
#else
#define NET_SOCKET_SERVICE_OWNER
#endif

#define NET_SOCKET_SERVICE_CALLBACK_MODE(_flag)				\
	IF_ENABLED(_flag,						\
		   (.work = Z_WORK_INITIALIZER(net_socket_service_callback),))

#define __z_net_socket_service_define(_name, _work_q, _cb, _count, _async, ...) \
	static int __z_net_socket_svc_get_idx(_name);			\
	static struct net_socket_service_event				\
			__z_net_socket_svc_get_name(_name)[_count] = {	\
		[0 ... ((_count) - 1)] = {				\
			.event.fd = -1, /* Invalid socket */		\
			NET_SOCKET_SERVICE_CALLBACK_MODE(_async)	\
			.callback = _cb,				\
		}							\
	};								\
	COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__), (), __VA_ARGS__)	\
	const STRUCT_SECTION_ITERABLE(net_socket_service_desc, _name) = { \
		NET_SOCKET_SERVICE_OWNER				\
		.work_q = (_work_q),                                    \
		.pev = __z_net_socket_svc_get_name(_name),		\
		.pev_len = (_count),					\
		.idx = &__z_net_socket_svc_get_idx(_name),		\
	}

/**
 * @brief Statically define a network socket service.
 *        The user callback is called asynchronously for this service meaning that
 *        the service API will not wait until the user callback returns before continuing
 *        with next socket service.
 *
 * The socket service can be accessed outside the module where it is defined using:
 *
 * @code extern struct net_socket_service_desc <name>; @endcode
 *
 * @note This macro cannot be used together with a static keyword.
 *       If such a use-case is desired, use NET_SOCKET_SERVICE_ASYNC_DEFINE_STATIC
 *       instead.
 *
 * @param name Name of the service.
 * @param work_q Pointer to workqueue where the work is done. Can be null in which case
 *        system workqueue is used.
 * @param cb Callback function that is called for socket activity.
 * @param count How many pollable sockets is needed for this service.
 */
#define NET_SOCKET_SERVICE_ASYNC_DEFINE(name, work_q, cb, count)	\
	__z_net_socket_service_define(name, work_q, cb, count, 1)

/**
 * @brief Statically define a network socket service in a private (static) scope.
 *        The user callback is called asynchronously for this service meaning that
 *        the service API will not wait until the user callback returns before continuing
 *        with next socket service.
 *
 * @param name Name of the service.
 * @param work_q Pointer to workqueue where the work is done. Can be null in which case
 *        system workqueue is used.
 * @param cb Callback function that is called for socket activity.
 * @param count How many pollable sockets is needed for this service.
 */
#define NET_SOCKET_SERVICE_ASYNC_DEFINE_STATIC(name, work_q, cb, count) \
	__z_net_socket_service_define(name, work_q, cb, count, 1, static)

/**
 * @brief Statically define a network socket service.
 *        The user callback is called synchronously for this service meaning that
 *        the service API will wait until the user callback returns before continuing
 *        with next socket service.
 *
 * The socket service can be accessed outside the module where it is defined using:
 *
 * @code extern struct net_socket_service_desc <name>; @endcode
 *
 * @note This macro cannot be used together with a static keyword.
 *       If such a use-case is desired, use NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC
 *       instead.
 *
 * @param name Name of the service.
 * @param work_q Pointer to workqueue where the work is done. Can be null in which case
 *        system workqueue is used.
 * @param cb Callback function that is called for socket activity.
 * @param count How many pollable sockets is needed for this service.
 */
#define NET_SOCKET_SERVICE_SYNC_DEFINE(name, work_q, cb, count)	\
	__z_net_socket_service_define(name, work_q, cb, count, 0)

/**
 * @brief Statically define a network socket service in a private (static) scope.
 *        The user callback is called synchronously for this service meaning that
 *        the service API will wait until the user callback returns before continuing
 *        with next socket service.
 *
 * @param name Name of the service.
 * @param work_q Pointer to workqueue where the work is done. Can be null in which case
 *        system workqueue is used.
 * @param cb Callback function that is called for socket activity.
 * @param count How many pollable sockets is needed for this service.
 */
#define NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(name, work_q, cb, count)	\
	__z_net_socket_service_define(name, work_q, cb, count, 0, static)

/**
 * @brief Register pollable sockets.
 *
 * @param service Pointer to a service description.
 * @param fds Socket array to poll.
 * @param len Length of the socket array.
 * @param user_data User specific data.
 *
 * @retval 0 No error
 * @retval -ENOENT Service is not found.
 * @retval -ENINVAL Invalid parameter.
 */
__syscall int net_socket_service_register(const struct net_socket_service_desc *service,
					  struct zsock_pollfd *fds, int len, void *user_data);

/**
 * @brief Unregister pollable sockets.
 *
 * @param service Pointer to a service description.
 *
 * @retval 0 No error
 * @retval -ENOENT Service is not found.
 * @retval -ENINVAL Invalid parameter.
 */
static inline int net_socket_service_unregister(const struct net_socket_service_desc *service)
{
	return net_socket_service_register(service, NULL, 0, NULL);
}

/**
 * @typedef net_socket_service_cb_t
 * @brief Callback used while iterating over socket services.
 *
 * @param svc Pointer to current socket service.
 * @param user_data A valid pointer to user data or NULL
 */
typedef void (*net_socket_service_cb_t)(const struct net_socket_service_desc *svc,
					void *user_data);

/**
 * @brief Go through all the socket services and call callback for each service.
 *
 * @param cb User-supplied callback function to call
 * @param user_data User specified data
 */
void net_socket_service_foreach(net_socket_service_cb_t cb, void *user_data);

#ifdef __cplusplus
}
#endif

#include <syscalls/socket_service.h>

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_SERVICE_H_ */
