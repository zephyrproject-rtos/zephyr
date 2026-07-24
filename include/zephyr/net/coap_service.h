/*
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief CoAP Service API
 *
 * An API for applications to respond to CoAP requests
 */

#ifndef ZEPHYR_INCLUDE_NET_COAP_SERVICE_H_
#define ZEPHYR_INCLUDE_NET_COAP_SERVICE_H_

#include <zephyr/net/coap.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/tls_credentials.h>

#if defined(CONFIG_COAP_OSCORE)
#include <zephyr/net/coap_oscore.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CoAP Service API
 * @defgroup coap_service CoAP service API
 * @since 3.6
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

/**
 * @name CoAP Service configuration flags
 * @anchor COAP_SERVICE_FLAGS
 * @{
 */

/** Start the service on boot. */
#define COAP_SERVICE_AUTOSTART		BIT(0)

/** @} */

/** @cond INTERNAL_HIDDEN */

struct coap_service_data {
	int sock_fd;
	struct coap_observer observers[CONFIG_COAP_SERVICE_OBSERVERS];
	struct coap_pending pending[CONFIG_COAP_SERVICE_PENDING_MESSAGES];
#if defined(CONFIG_COAP_OSCORE) || defined(__DOXYGEN__)
	/**
	 * True while the service is processing an OSCORE-verified request.
	 * Used by coap_service_send_internal() to protect synchronous
	 * responses without depending on the (expirable) exchange cache.
	 * Managed internally by the CoAP server during coap_server_process().
	 * @kconfig_dep{CONFIG_COAP_OSCORE}
	 */
	bool oscore_sync_response;
	/**
	 * OSCORE exchange cache tracking which responses need OSCORE protection.
	 * Points to a statically allocated per-service array, or NULL when the
	 * service is not OSCORE-enabled.
	 * @kconfig_dep{CONFIG_COAP_OSCORE}
	 */
	struct coap_oscore_exchange *oscore_exchange_cache;
#endif /* CONFIG_COAP_OSCORE */
};

struct coap_service {
	const char *name;
	const char *host;
	uint16_t *port;
	uint8_t flags;
	struct coap_resource *res_begin;
	struct coap_resource *res_end;
	struct coap_service_data *data;
#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
	const sec_tag_t *sec_tag_list;
	size_t sec_tag_list_size;
#endif
#if defined(CONFIG_COAP_OSCORE) || defined(__DOXYGEN__)
	/**
	 * If true, requests without OSCORE are rejected with 4.01 Unauthorized.
	 * @kconfig_dep{CONFIG_COAP_OSCORE}
	 */
	bool oscore_required;
	/**
	 * Provider returning the OSCORE security context to use for this service,
	 * or NULL if the service is not OSCORE-enabled. Invoked by the CoAP server
	 * when a request/response needs OSCORE processing, so the context can be
	 * derived lazily once its keying material is available.
	 * @kconfig_dep{CONFIG_COAP_OSCORE}
	 */
	struct coap_oscore_context *(*oscore_ctx_provider)(void);
#endif
};

#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
#define __z_coap_service_secure(_sec_tag_list, _sec_tag_list_size)				\
		.sec_tag_list = _sec_tag_list,							\
		.sec_tag_list_size = _sec_tag_list_size,
#else
#define __z_coap_service_secure(...)
#endif

#if defined(CONFIG_COAP_OSCORE)
/* Statically define the per-service OSCORE exchange cache. */
#define __z_coap_oscore_cache_define(_name)                                                        \
	static struct coap_oscore_exchange _CONCAT(coap_oscore_exchange_cache_,                    \
						   _name)[CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE];
#define __z_coap_oscore_cache_ptr(_name) (_CONCAT(coap_oscore_exchange_cache_, _name))
#define __z_coap_service_oscore(_provider, _required)                                              \
	.oscore_ctx_provider = (_provider), .oscore_required = (_required),
#define __z_coap_service_oscore_data(_cache)                                                       \
	.oscore_sync_response = false, .oscore_exchange_cache = (_cache),
#else
#define __z_coap_service_oscore(...)
#define __z_coap_oscore_cache_define(_name)
#define __z_coap_oscore_cache_ptr(_name) NULL
#define __z_coap_service_oscore_data(...)
#endif

/* clang-format off */
#define __z_coap_service_define(_name, _host, _port, _flags, _res_begin, _res_end, _sec_tag_list,  \
				_sec_tag_list_size, _oscore_provider, _oscore_required,            \
				_oscore_cache)                                                     \
	static struct coap_service_data _CONCAT(coap_service_data_, _name) = {                     \
		.sock_fd = -1,                                                                     \
		__z_coap_service_oscore_data(_oscore_cache)                                        \
	};                                                                                         \
	const STRUCT_SECTION_ITERABLE(coap_service, _name) = {                                     \
		.name = STRINGIFY(_name),                                                          \
		.host = _host,                                                                     \
		.port = (uint16_t *)(_port),                                                       \
		.flags = _flags,                                                                   \
		.res_begin = (_res_begin),                                                         \
		.res_end = (_res_end),                                                             \
		.data = &_CONCAT(coap_service_data_, _name),                                       \
		__z_coap_service_secure(_sec_tag_list, _sec_tag_list_size)                         \
		__z_coap_service_oscore(_oscore_provider, _oscore_required)                        \
	}
/* clang-format on */

/** @endcond */

/**
 * @brief Define a static CoAP resource owned by the service named @p _service .
 *
 * @note The handlers registered with the resource can return a CoAP response code to reply with
 * an acknowledge without any payload, nothing is sent if the return value is 0 or negative.
 * As seen in the example.
 *
 * @code{.c}
 *     static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
 *
 *     static int led_put(struct coap_resource *resource, struct coap_packet *request,
 *                        struct net_sockaddr *addr, net_socklen_t addr_len)
 *     {
 *             const uint8_t *payload;
 *             uint16_t payload_len;
 *
 *             payload = coap_packet_get_payload(request, &payload_len);
 *             if (payload_len != 1) {
 *                     return COAP_RESPONSE_CODE_BAD_REQUEST;
 *             }
 *
 *             if (gpio_pin_set_dt(&led, payload[0]) < 0) {
 *                     return COAP_RESPONSE_CODE_INTERNAL_ERROR;
 *             }
 *
 *             return COAP_RESPONSE_CODE_CHANGED;
 *     }
 *
 *     COAP_RESOURCE_DEFINE(my_resource, my_service, {
 *             .put = led_put,
 *     });
 * @endcode
 *
 * @param _name Name of the resource.
 * @param _service Name of the associated service.
 */
#define COAP_RESOURCE_DEFINE(_name, _service, ...)						\
	STRUCT_SECTION_ITERABLE_ALTERNATE(_CONCAT(coap_resource_, _service), coap_resource,	\
					  _name) = __VA_ARGS__

/**
 * @brief Define a CoAP service with static resources.
 *
 * @note The @p _host parameter can be `NULL`. If not, it is used to specify an IP address either in
 * IPv4 or IPv6 format a fully-qualified hostname or a virtual host, otherwise the any address is
 * used.
 *
 * @note The @p _port parameter must be non-`NULL`. It points to a location that specifies the port
 * number to use for the service. If the specified port number is zero, then an ephemeral port
 * number will be used and the actual port number assigned will be written back to memory. For
 * ephemeral port numbers, the memory pointed to by @p _port must be writeable.
 *
 * @param _name Name of the service.
 * @param _host IP address or hostname associated with the service.
 * @param[inout] _port Pointer to port associated with the service.
 * @param _flags Configuration flags @see @ref COAP_SERVICE_FLAGS.
 */
#define COAP_SERVICE_DEFINE(_name, _host, _port, _flags)                                           \
	extern struct coap_resource _CONCAT(_CONCAT(_coap_resource_, _name), _list_start)[];       \
	extern struct coap_resource _CONCAT(_CONCAT(_coap_resource_, _name), _list_end)[];         \
	__z_coap_service_define(_name, _host, _port, _flags,                                       \
				&_CONCAT(_CONCAT(_coap_resource_, _name), _list_start)[0],         \
				&_CONCAT(_CONCAT(_coap_resource_, _name), _list_end)[0], NULL, 0,  \
				NULL, false, NULL)

/**
 * @brief Define a CoAP secure service with static resources.
 *
 * @note The @p _host parameter can be `NULL`. If not, it is used to specify an IP address either in
 * IPv4 or IPv6 format a fully-qualified hostname or a virtual host, otherwise the any address is
 * used.
 *
 * @note The @p _port parameter must be non-`NULL`. It points to a location that specifies the port
 * number to use for the service. If the specified port number is zero, then an ephemeral port
 * number will be used and the actual port number assigned will be written back to memory. For
 * ephemeral port numbers, the memory pointed to by @p _port must be writeable.
 *
 * @note @kconfig{CONFIG_NET_SOCKETS_ENABLE_DTLS} has to be enabled for CoAP secure support.
 *
 * @param _name Name of the service.
 * @param _host IP address or hostname associated with the service.
 * @param[inout] _port Pointer to port associated with the service.
 * @param _flags Configuration flags @see @ref COAP_SERVICE_FLAGS.
 * @param _sec_tag_list DTLS security tag list used to setup a COAPS socket.
 * @param _sec_tag_list_size DTLS security tag list size used to setup a COAPS socket.
 */
#define COAPS_SERVICE_DEFINE(_name, _host, _port, _flags, _sec_tag_list, _sec_tag_list_size)       \
	BUILD_ASSERT(IS_ENABLED(CONFIG_NET_SOCKETS_ENABLE_DTLS),                                   \
		     "DTLS is required for CoAP secure (CONFIG_NET_SOCKETS_ENABLE_DTLS)");         \
	extern struct coap_resource _CONCAT(_CONCAT(_coap_resource_, _name), _list_start)[];       \
	extern struct coap_resource _CONCAT(_CONCAT(_coap_resource_, _name), _list_end)[];         \
	__z_coap_service_define(_name, _host, _port, _flags,                                       \
				&_CONCAT(_CONCAT(_coap_resource_, _name), _list_start)[0],         \
				&_CONCAT(_CONCAT(_coap_resource_, _name), _list_end)[0],           \
				_sec_tag_list, _sec_tag_list_size, NULL, false, NULL)

/**
 * @brief Define an OSCORE-enabled CoAP service with static resources.
 *
 * Behaves like @ref COAP_SERVICE_DEFINE but additionally wires OSCORE support
 * (RFC 8613) into the service. A per-service OSCORE exchange cache is allocated
 * statically, and the security context is obtained lazily from
 * @p _oscore_ctx_provider so its keying material can be derived at runtime.
 *
 * @note Requires @kconfig{CONFIG_COAP_OSCORE}. When that option is disabled the
 * service degrades to a plain @ref COAP_SERVICE_DEFINE.
 *
 * @param _name Name of the service.
 * @param _host IP address or hostname associated with the service.
 * @param[inout] _port Pointer to port associated with the service.
 * @param _flags Configuration flags @see @ref COAP_SERVICE_FLAGS.
 * @param _oscore_ctx_provider Function of type
 *        `struct coap_oscore_context *(*)(void)` returning the OSCORE context to
 *        use, or NULL if OSCORE is currently unavailable.
 * @param _oscore_required If true, requests without OSCORE are rejected with
 *        4.01 Unauthorized.
 */
#define COAP_SERVICE_DEFINE_OSCORE(_name, _host, _port, _flags, _oscore_ctx_provider,              \
				   _oscore_required)                                               \
	extern struct coap_resource _CONCAT(_CONCAT(_coap_resource_, _name), _list_start)[];       \
	extern struct coap_resource _CONCAT(_CONCAT(_coap_resource_, _name), _list_end)[];         \
	__z_coap_oscore_cache_define(_name) __z_coap_service_define(                               \
		_name, _host, _port, _flags,                                                       \
		&_CONCAT(_CONCAT(_coap_resource_, _name), _list_start)[0],                         \
		&_CONCAT(_CONCAT(_coap_resource_, _name), _list_end)[0], NULL, 0,                  \
		(_oscore_ctx_provider), (_oscore_required), __z_coap_oscore_cache_ptr(_name))

/**
 * @brief Define an OSCORE-enabled CoAP secure (DTLS) service with static resources.
 *
 * Combines @ref COAPS_SERVICE_DEFINE with OSCORE support as described in
 * @ref COAP_SERVICE_DEFINE_OSCORE.
 *
 * @param _name Name of the service.
 * @param _host IP address or hostname associated with the service.
 * @param[inout] _port Pointer to port associated with the service.
 * @param _flags Configuration flags @see @ref COAP_SERVICE_FLAGS.
 * @param _sec_tag_list DTLS security tag list used to setup a COAPS socket.
 * @param _sec_tag_list_size DTLS security tag list size used to setup a COAPS socket.
 * @param _oscore_ctx_provider Function of type
 *        `struct coap_oscore_context *(*)(void)` returning the OSCORE context to
 *        use, or NULL if OSCORE is currently unavailable.
 * @param _oscore_required If true, requests without OSCORE are rejected with
 *        4.01 Unauthorized.
 */
#define COAPS_SERVICE_DEFINE_OSCORE(_name, _host, _port, _flags, _sec_tag_list,                    \
				    _sec_tag_list_size, _oscore_ctx_provider, _oscore_required)    \
	BUILD_ASSERT(IS_ENABLED(CONFIG_NET_SOCKETS_ENABLE_DTLS),                                   \
		     "DTLS is required for CoAP secure (CONFIG_NET_SOCKETS_ENABLE_DTLS)");         \
	extern struct coap_resource _CONCAT(_CONCAT(_coap_resource_, _name), _list_start)[];       \
	extern struct coap_resource _CONCAT(_CONCAT(_coap_resource_, _name), _list_end)[];         \
	__z_coap_oscore_cache_define(_name)                                                        \
		__z_coap_service_define(_name, _host, _port, _flags,                               \
					&_CONCAT(_CONCAT(_coap_resource_, _name), _list_start)[0], \
					&_CONCAT(_CONCAT(_coap_resource_, _name), _list_end)[0],   \
					_sec_tag_list, _sec_tag_list_size, (_oscore_ctx_provider), \
					(_oscore_required), __z_coap_oscore_cache_ptr(_name))

/**
 * @brief Count the number of CoAP services.
 *
 * @param[out] _dst Pointer to location where result is written.
 */
#define COAP_SERVICE_COUNT(_dst) STRUCT_SECTION_COUNT(coap_service, _dst)

/**
 * @brief Count CoAP service static resources.
 *
 * @param _service Pointer to a service.
 */
#define COAP_SERVICE_RESOURCE_COUNT(_service) ((_service)->res_end - (_service)->res_begin)

/**
 * @brief Check if service has the specified resource.
 *
 * @param _service Pointer to a service.
 * @param _resource Pointer to a resource.
 */
#define COAP_SERVICE_HAS_RESOURCE(_service, _resource)						\
	((_service)->res_begin <= _resource && _resource < (_service)->res_end)

/**
 * @brief Iterate over all CoAP services.
 *
 * @param _it Name of iterator (of type @ref coap_service)
 */
#define COAP_SERVICE_FOREACH(_it) STRUCT_SECTION_FOREACH(coap_service, _it)

/**
 * @brief Iterate over static CoAP resources associated with a given @p _service.
 *
 * @note This macro requires that @p _service is defined with @ref COAP_SERVICE_DEFINE.
 *
 * @param _service Name of CoAP service
 * @param _it Name of iterator (of type @ref coap_resource)
 */
#define COAP_RESOURCE_FOREACH(_service, _it)							\
	STRUCT_SECTION_FOREACH_ALTERNATE(_CONCAT(coap_resource_, _service), coap_resource, _it)

/**
 * @brief Iterate over all static resources associated with @p _service .
 *
 * @note This macro is suitable for a @p _service defined with @ref COAP_SERVICE_DEFINE.
 *
 * @param _service Pointer to COAP service
 * @param _it Name of iterator (of type @ref coap_resource)
 */
#define COAP_SERVICE_FOREACH_RESOURCE(_service, _it)						\
	for (struct coap_resource *_it = (_service)->res_begin; ({				\
		__ASSERT(_it <= (_service)->res_end, "unexpected list end location");		\
		_it < (_service)->res_end;							\
	}); _it++)

/**
 * @brief Start the provided @p service .
 *
 * @note This function is suitable for a @p service defined with @ref COAP_SERVICE_DEFINE.
 *
 * @param service Pointer to CoAP service
 * @retval 0 in case of success.
 * @retval -EALREADY in case of an already running service.
 * @retval -ENOTSUP in case the server has no valid host and port configuration.
 */
int coap_service_start(const struct coap_service *service);

/**
 * @brief Stop the provided @p service .
 *
 * @note This function is suitable for a @p service defined with @ref COAP_SERVICE_DEFINE.
 *
 * @param service Pointer to CoAP service
 * @retval 0 in case of success.
 * @retval -EALREADY in case the service isn't running.
 */
int coap_service_stop(const struct coap_service *service);

/**
 * @brief Query the provided @p service running state.
 *
 * @note This function is suitable for a @p service defined with @ref COAP_SERVICE_DEFINE.
 *
 * @param service Pointer to CoAP service
 * @retval 1 if the service is running
 * @retval 0 if the service is stopped
 * @retval <0 negative in case of an error.
 */
int coap_service_is_running(const struct coap_service *service);

/**
 * @brief Send a CoAP message from the provided @p service .
 *
 * @note This function is suitable for a @p service defined with @ref COAP_SERVICE_DEFINE.
 *
 * @param service Pointer to CoAP service
 * @param cpkt CoAP Packet to send
 * @param addr Peer address
 * @param addr_len Peer address length
 * @param params Pointer to transmission parameters structure or NULL to use default values.
 * @return 0 in case of success or negative in case of error.
 */
int coap_service_send(const struct coap_service *service, const struct coap_packet *cpkt,
		      const struct net_sockaddr *addr, net_socklen_t addr_len,
		      const struct coap_transmission_parameters *params);

/**
 * @brief Send a CoAP message from the provided @p resource .
 *
 * @note This function is suitable for a @p resource defined with @ref COAP_RESOURCE_DEFINE.
 *
 * @param resource Pointer to CoAP resource
 * @param cpkt CoAP Packet to send
 * @param addr Peer address
 * @param addr_len Peer address length
 * @param params Pointer to transmission parameters structure or NULL to use default values.
 * @return 0 in case of success or negative in case of error.
 */
int coap_resource_send(const struct coap_resource *resource, const struct coap_packet *cpkt,
		       const struct net_sockaddr *addr, net_socklen_t addr_len,
		       const struct coap_transmission_parameters *params);

/**
 * @brief Parse a CoAP observe request for the provided @p resource .
 *
 * @note This function is suitable for a @p resource defined with @ref COAP_RESOURCE_DEFINE.
 *
 * If the observe option value is equal to 0, an observer will be added, if the value is equal
 * to 1, an existing observer will be removed.
 *
 * @param resource Pointer to CoAP resource
 * @param request CoAP request to parse
 * @param addr Peer address
 * @return the observe option value in case of success or negative in case of error.
 */
int coap_resource_parse_observe(struct coap_resource *resource, const struct coap_packet *request,
				const struct net_sockaddr *addr);

/**
 * @brief Lookup an observer by address and remove it from the @p resource .
 *
 * @note This function is suitable for a @p resource defined with @ref COAP_RESOURCE_DEFINE.
 *
 * @param resource Pointer to CoAP resource
 * @param addr Peer address
 * @return 0 in case of success or negative in case of error.
 */
int coap_resource_remove_observer_by_addr(struct coap_resource *resource,
					  const struct net_sockaddr *addr);

/**
 * @brief Lookup an observer by token and remove it from the @p resource .
 *
 * @note This function is suitable for a @p resource defined with @ref COAP_RESOURCE_DEFINE.
 *
 * @param resource Pointer to CoAP resource
 * @param token Pointer to the token
 * @param token_len Length of valid bytes in the token
 * @return 0 in case of success or negative in case of error.
 */
int coap_resource_remove_observer_by_token(struct coap_resource *resource,
					   const uint8_t *token, uint8_t token_len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_COAP_SERVICE_H_ */
