/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB virtual bus service
 */

#ifndef ZEPHYR_INCLUDE_UVB
#define ZEPHYR_INCLUDE_UVB

#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/dlist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Virtual bus event types
 */
enum uvb_event_type {
	/** VBUS ready event */
	UVB_EVT_VBUS_READY,
	/** VBUS removed event */
	UVB_EVT_VBUS_REMOVED,
	/** Device resume event */
	UVB_EVT_RESUME,
	/** Device suspended event */
	UVB_EVT_SUSPEND,
	/** Port reset detected */
	UVB_EVT_RESET,
	/** Endpoint request event */
	UVB_EVT_REQUEST,
	/** Endpoint request reply event */
	UVB_EVT_REPLY,
	/** Device activity event */
	UVB_EVT_DEVICE_ACT,
};

/**
 * @brief Virtual bus device activity type
 */
enum uvb_device_act {
	/** Device issue remote wakeup */
	UVB_DEVICE_ACT_RWUP,
	/** Low speed connection detected */
	UVB_DEVICE_ACT_LS,
	/** Full speed connection detected */
	UVB_DEVICE_ACT_FS,
	/** High speed connection detected */
	UVB_DEVICE_ACT_HS,
	/** Super speed connection detected */
	UVB_DEVICE_ACT_SS,
	/** Connection removed, issued when a device is disabled */
	UVB_DEVICE_ACT_REMOVED,
};

/**
 * @brief Virtual bus host request type
 */
enum uvb_request {
	/** Setup request */
	UVB_REQUEST_SETUP,
	/** Data request */
	UVB_REQUEST_DATA,
};

/**
 * @brief Virtual bus device reply type
 */
enum uvb_reply {
	/** Default value */
	UVB_REPLY_TIMEOUT,
	/** Reply ACK handshake to a request */
	UVB_REPLY_ACK,
	/** Reply NACK handshake to a request */
	UVB_REPLY_NACK,
	/** Reply STALL handshake to a request */
	UVB_REPLY_STALL,
};

/**
 * USB virtual bus packet
 */
struct uvb_packet {
	/** slist node (TBD) */
	sys_snode_t node;
	/** Consecutive packet sequence number */
	uint32_t seq;

	/** Request type */
	enum uvb_request request: 8;
	/** Reply handshake */
	enum uvb_reply reply : 8;
	/** Device (peripheral) address */
	unsigned int addr : 8;
	/** Endpoint address */
	unsigned int ep : 8;

	/** Pointer to a data chunk */
	uint8_t *data;
	/** Length of the data chunk */
	size_t length;
};

/**
 * USB virtual bus node
 */
struct uvb_node {
	union {
		/** dlist device node */
		sys_dnode_t node;
		/** dlist host list */
		sys_dlist_t list;
	};
	/** Name of the UVB node */
	const char *name;
	/** Pointer to the notify callback of the UVB node */
	void (*notify)(const void *const priv,
		       const enum uvb_event_type type,
		       const void *data);
	/** Internally used atomic value */
	atomic_t subscribed;
	/** True for a host node */
	bool head;
	/** Pointer to the node's private data */
	const void *priv;
};

/**
 * @brief Allocate UVB packet for the request or reply.
 *
 * @param[in] request Request type
 * @param[in] addr    Device (peripheral) address
 * @param[in] ep      Endpoint address
 * @param[in] data    Pointer to a chunk of the net_buf data
 * @param[in] length  Data chunk length
 *
 * @return pointer to allocated packet or NULL on error.
 */
struct uvb_packet *uvb_alloc_pkt(const enum uvb_request request,
				 const uint8_t addr, const uint8_t ep,
				 uint8_t *const data,
				 const size_t length);

/**
 * @brief Free UVB packet
 *
 * @param[in] pkt    Pointer to UVB packet
 */
void uvb_free_pkt(struct uvb_packet *const pkt);

/**
 * @brief Advert UVB event on virtual bus
 *
 * All devices subscribed to a controller are advertised.
 * Events like UVB_EVT_REQUEST are to be filtered by using device address.
 *
 * @param[in] host_node Pointer to host controller UVB node
 * @param[in] type      UVB event type
 * @param[in] pkt       Pointer to UVB packet or NULL
 *
 * @return 0 on success, all other values should be treated as error.
 */
int uvb_advert(const struct uvb_node *const host_node,
	       const enum uvb_event_type type,
	       const struct uvb_packet *const pkt);

/**
 * @brief Submit UVB event to host controller node
 *
 * Intended for use by virtual device for the request reply
 * UVB_EVT_REPLY and device activity event UVB_EVT_DEVICE_ACT
 *
 * @param[in] dev_node  Pointer to device controller UVB node
 * @param[in] type      UVB event type
 * @param[in] pkt       Pointer to UVB packet or NULL
 *
 * @return 0 on success, all other values should be treated as error.
 */
int uvb_to_host(const struct uvb_node *const dev_node,
		const enum uvb_event_type type,
		const struct uvb_packet *const pkt);

/**
 * @brief Subscribe to the adverts of the specific host node.
 *
 * Intended for use by virtual device during UDC API init call.
 *
 * @param[in] name     Name of the host node.
 * @param[in] dev_node Pointer to device controller UVB node
 *
 * @return 0 on success, all other values should be treated as error.
 */
int uvb_subscribe(const char *name, struct uvb_node *const dev_node);

/**
 * @brief Unsubsribe from the adverts of the specific host node.
 *
 * Intended for use by virtual device during UDC API shutdown call.
 *
 * @param[in] name     Name of the host node.
 * @param[in] dev_node Pointer to device controller UVB node
 *
 * @return 0 on success, all other values should be treated as error.
 */
int uvb_unsubscribe(const char *name, struct uvb_node *const dev_node);

/**
 * @brief Advert request UVB event on virtual bus
 *
 * @param[in] host_node Pointer to host controller UVB node
 * @param[in] pkt       Pointer to UVB packet
 *
 * @return 0 on success, all other values should be treated as error.
 */
static inline int uvb_advert_pkt(const struct uvb_node *const host_node,
				 const struct uvb_packet *const pkt)
{
	return uvb_advert(host_node, UVB_EVT_REQUEST, pkt);
}

/**
 * @brief Reply to UVB request
 *
 * @param[in] dev_node Pointer to host controller UVB node
 * @param[in] pkt      Pointer to UVB packet
 *
 * @return 0 on success, all other values should be treated as error.
 */
static inline int uvb_reply_pkt(const struct uvb_node *const dev_node,
				const struct uvb_packet *const pkt)
{
	return uvb_to_host(dev_node, UVB_EVT_REPLY, pkt);
}

/** @brief Helper to define UVB host controller node
 *
 *  @param host        UVB host node structure name
 *  @param host_name   UVB host node name
 *  @param host_notify Pointer to host notify callback
 */
#define UVB_HOST_NODE_DEFINE(host, host_name, host_notify)			\
	STRUCT_SECTION_ITERABLE(uvb_node, host) = {				\
		.name = host_name,						\
		.head = true,							\
		.notify = host_notify,						\
	}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_UVB */
