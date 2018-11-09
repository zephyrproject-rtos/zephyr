/*
 * Remote processor messaging
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Copyright (C) 2011 Google, Inc.
 * All rights reserved.
 * Copyright (c) 2016 Freescale Semiconductor, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RPMSG_H_
#define _RPMSG_H_

#include <openamp/compiler.h>
#include <metal/mutex.h>
#include <metal/list.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#if defined __cplusplus
extern "C" {
#endif

/* Configurable parameters */
#define RPMSG_NAME_SIZE		(32)
#define RPMSG_ADDR_BMP_SIZE	(4)

#define RPMSG_NS_EPT_ADDR	(0x35)
#define RPMSG_ADDR_ANY		0xFFFFFFFF

/* Error macros. */
#define RPMSG_SUCCESS		0
#define RPMSG_ERROR_BASE	-2000
#define RPMSG_ERR_NO_MEM	(RPMSG_ERROR_BASE - 1)
#define RPMSG_ERR_NO_BUFF	(RPMSG_ERROR_BASE - 2)
#define RPMSG_ERR_PARAM		(RPMSG_ERROR_BASE - 3)
#define RPMSG_ERR_DEV_STATE	(RPMSG_ERROR_BASE - 4)
#define RPMSG_ERR_BUFF_SIZE	(RPMSG_ERROR_BASE - 5)
#define RPMSG_ERR_INIT		(RPMSG_ERROR_BASE - 6)
#define RPMSG_ERR_ADDR		(RPMSG_ERROR_BASE - 7)

struct rpmsg_endpoint;
struct rpmsg_device;

typedef int (*rpmsg_ept_cb)(struct rpmsg_endpoint *ept, void *data,
			    size_t len, uint32_t src, void *priv);
typedef void (*rpmsg_ns_unbind_cb)(struct rpmsg_endpoint *ept);
typedef void (*rpmsg_ns_bind_cb)(struct rpmsg_device *rdev,
				 const char *name, uint32_t dest);

/**
 * struct rpmsg_endpoint - binds a local rpmsg address to its user
 * @name:name of the service supported
 * @rdev: pointer to the rpmsg device
 * @addr: local address of the endpoint
 * @dest_addr: address of the default remote endpoint binded.
 * @cb: user rx callback, return value of this callback is reserved
 *      for future use, for now, only allow RPMSG_SUCCESS as return value.
 * @ns_unbind_cb: end point service service unbind callback, called when remote
 *                ept is destroyed.
 * @node: end point node.
 * @addr: local rpmsg address
 * @priv: private data for the driver's use
 *
 * In essence, an rpmsg endpoint represents a listener on the rpmsg bus, as
 * it binds an rpmsg address with an rx callback handler.
 */
struct rpmsg_endpoint {
	char name[RPMSG_NAME_SIZE];
	struct rpmsg_device *rdev;
	uint32_t addr;
	uint32_t dest_addr;
	rpmsg_ept_cb cb;
	rpmsg_ns_unbind_cb ns_unbind_cb;
	struct metal_list node;
	void *priv;
};

/**
 * struct rpmsg_device_ops - RPMsg device operations
 * @send_offchannel_raw: send RPMsg data
 */
struct rpmsg_device_ops {
	int (*send_offchannel_raw)(struct rpmsg_device *rdev,
				   uint32_t src, uint32_t dst,
				   const void *data, int size, int wait);
};

/**
 * struct rpmsg_device - representation of a RPMsg device
 * @endpoints: list of endpoints
 * @ns_ept: name service endpoint
 * @bitmap: table endpoin address allocation.
 * @lock: mutex lock for rpmsg management
 * @ns_bind_cb: callback handler for name service announcement without local
 *              endpoints waiting to bind.
 * @ops: RPMsg device operations
 */
struct rpmsg_device {
	struct metal_list endpoints;
	struct rpmsg_endpoint ns_ept;
	unsigned long bitmap[RPMSG_ADDR_BMP_SIZE];
	metal_mutex_t lock;
	rpmsg_ns_bind_cb ns_bind_cb;
	struct rpmsg_device_ops ops;
};

/**
 * rpmsg_send_offchannel_raw() - send a message across to the remote processor,
 * specifying source and destination address.
 * @ept: the rpmsg endpoint
 * @data: payload of the message
 * @len: length of the payload
 *
 * This function sends @data of length @len to the remote @dst address from
 * the source @src address.
 * The message will be sent to the remote processor which the channel belongs
 * to.
 * In case there are no TX buffers available, the function will block until
 * one becomes available, or a timeout of 15 seconds elapses. When the latter
 * happens, -ERESTARTSYS is returned.
 *
 * Returns number of bytes it has sent or negative error value on failure.
 */
int rpmsg_send_offchannel_raw(struct rpmsg_endpoint *ept, uint32_t src,
			      uint32_t dst, const void *data, int size,
			      int wait);

/**
 * rpmsg_send() - send a message across to the remote processor
 * @ept: the rpmsg endpoint
 * @data: payload of the message
 * @len: length of the payload
 *
 * This function sends @data of length @len based on the @ept.
 * The message will be sent to the remote processor which the channel belongs
 * to, using @ept's source and destination addresses.
 * In case there are no TX buffers available, the function will block until
 * one becomes available, or a timeout of 15 seconds elapses. When the latter
 * happens, -ERESTARTSYS is returned.
 *
 * Returns number of bytes it has sent or negative error value on failure.
 */
static inline int rpmsg_send(struct rpmsg_endpoint *ept, const void *data,
			     int len)
{
	if (ept->dest_addr == RPMSG_ADDR_ANY)
		return RPMSG_ERR_ADDR;
	return rpmsg_send_offchannel_raw(ept, ept->addr, ept->dest_addr, data,
					 len, true);
}

/**
 * rpmsg_sendto() - send a message across to the remote processor, specify dst
 * @ept: the rpmsg endpoint
 * @data: payload of message
 * @len: length of payload
 * @dst: destination address
 *
 * This function sends @data of length @len to the remote @dst address.
 * The message will be sent to the remote processor which the @ept
 * channel belongs to, using @ept's source address.
 * In case there are no TX buffers available, the function will block until
 * one becomes available, or a timeout of 15 seconds elapses. When the latter
 * happens, -ERESTARTSYS is returned.
 *
 * Returns number of bytes it has sent or negative error value on failure.
 */
static inline int rpmsg_sendto(struct rpmsg_endpoint *ept, const void *data,
			       int len, uint32_t dst)
{
	return rpmsg_send_offchannel_raw(ept, ept->addr, dst, data, len, true);
}

/**
 * rpmsg_send_offchannel() - send a message using explicit src/dst addresses
 * @ept: the rpmsg endpoint
 * @src: source address
 * @dst: destination address
 * @data: payload of message
 * @len: length of payload
 *
 * This function sends @data of length @len to the remote @dst address,
 * and uses @src as the source address.
 * The message will be sent to the remote processor which the @ept
 * channel belongs to.
 * In case there are no TX buffers available, the function will block until
 * one becomes available, or a timeout of 15 seconds elapses. When the latter
 * happens, -ERESTARTSYS is returned.
 *
 * Returns number of bytes it has sent or negative error value on failure.
 */
static inline int rpmsg_send_offchannel(struct rpmsg_endpoint *ept,
					uint32_t src, uint32_t dst,
					const void *data, int len)
{
	return rpmsg_send_offchannel_raw(ept, src, dst, data, len, true);
}

/**
 * rpmsg_trysend() - send a message across to the remote processor
 * @ept: the rpmsg endpoint
 * @data: payload of message
 * @len: length of payload
 *
 * This function sends @data of length @len on the @ept channel.
 * The message will be sent to the remote processor which the @ept
 * channel belongs to, using @ept's source and destination addresses.
 * In case there are no TX buffers available, the function will immediately
 * return -ENOMEM without waiting until one becomes available.
 *
 * Returns number of bytes it has sent or negative error value on failure.
 */
static inline int rpmsg_trysend(struct rpmsg_endpoint *ept, const void *data,
				int len)
{
	if (ept->dest_addr == RPMSG_ADDR_ANY)
		return RPMSG_ERR_ADDR;
	return rpmsg_send_offchannel_raw(ept, ept->addr, ept->dest_addr, data,
					 len, false);
}

/**
 * rpmsg_trysendto() - send a message across to the remote processor,
 * specify dst
 * @ept: the rpmsg endpoint
 * @data: payload of message
 * @len: length of payload
 * @dst: destination address
 *
 * This function sends @data of length @len to the remote @dst address.
 * The message will be sent to the remote processor which the @ept
 * channel belongs to, using @ept's source address.
 * In case there are no TX buffers available, the function will immediately
 * return -ENOMEM without waiting until one becomes available.
 *
 * Returns number of bytes it has sent or negative error value on failure.
 */
static inline int rpmsg_trysendto(struct rpmsg_endpoint *ept, const void *data,
				  int len, uint32_t dst)
{
	return rpmsg_send_offchannel_raw(ept, ept->addr, dst, data, len, false);
}

/**
 * rpmsg_trysend_offchannel() - send a message using explicit src/dst addresses
 * @ept: the rpmsg endpoint
 * @src: source address
 * @dst: destination address
 * @data: payload of message
 * @len: length of payload
 *
 * This function sends @data of length @len to the remote @dst address,
 * and uses @src as the source address.
 * The message will be sent to the remote processor which the @ept
 * channel belongs to.
 * In case there are no TX buffers available, the function will immediately
 * return -ENOMEM without waiting until one becomes available.
 *
 * Returns number of bytes it has sent or negative error value on failure.
 */
static inline int rpmsg_trysend_offchannel(struct rpmsg_endpoint *ept,
					   uint32_t src, uint32_t dst,
					   const void *data, int len)
{
	return rpmsg_send_offchannel_raw(ept, src, dst, data, len, false);
}

/**
 * rpmsg_init_ept - initialize rpmsg endpoint
 *
 * Initialize an RPMsg endpoint with a name, source address,
 * remoteproc address, endpoitn callback, and destroy endpoint callback.
 *
 * @ept: pointer to rpmsg endpoint
 * @name: service name associated to the endpoint
 * @src: local address of the endpoint
 * @dest: target address of the endpoint
 * @cb: endpoint callback
 * @ns_unbind_cb: end point service unbind callback, called when remote ept is
 *                destroyed.
 */
static inline void rpmsg_init_ept(struct rpmsg_endpoint *ept,
				  const char *name,
				  uint32_t src, uint32_t dest,
				  rpmsg_ept_cb cb,
				  rpmsg_ns_unbind_cb ns_unbind_cb)
{
	strncpy(ept->name, name, sizeof(ept->name));
	ept->addr = src;
	ept->dest_addr = dest;
	ept->cb = cb;
	ept->ns_unbind_cb = ns_unbind_cb;
}

/**
 * rpmsg_create_ept - create rpmsg endpoint and register it to rpmsg device
 *
 * Create a RPMsg endpoint, initialize it with a name, source address,
 * remoteproc address, endpoitn callback, and destroy endpoint callback,
 * and register it to the RPMsg device.
 *
 * @ept: pointer to rpmsg endpoint
 * @name: service name associated to the endpoint
 * @src: local address of the endpoint
 * @dest: target address of the endpoint
 * @cb: endpoint callback
 * @ns_unbind_cb: end point service unbind callback, called when remote ept is
 *                destroyed.
 *
 * In essence, an rpmsg endpoint represents a listener on the rpmsg bus, as
 * it binds an rpmsg address with an rx callback handler.
 *
 * Rpmsg client should create an endpoint to discuss with remote. rpmsg client
 * provide at least a channel name, a callback for message notification and by
 * default endpoint source address should be set to RPMSG_ADDR_ANY.
 *
 * As an option Some rpmsg clients can specify an endpoint with a specific
 * source address.
 */

int rpmsg_create_ept(struct rpmsg_endpoint *ept, struct rpmsg_device *rdev,
		     const char *name, uint32_t src, uint32_t dest,
		     rpmsg_ept_cb cb, rpmsg_ns_unbind_cb ns_unbind_cb);

/**
 * rpmsg_destroy_ept - destroy rpmsg endpoint and unregister it from rpmsg
 *                     device
 *
 * @ept: pointer to the rpmsg endpoint
 *
 * It unregisters the rpmsg endpoint from the rpmsg device and calls the
 * destroy endpoint callback if it is provided.
 */
void rpmsg_destroy_ept(struct rpmsg_endpoint *ept);

/**
 * is_rpmsg_ept_ready - check if the rpmsg endpoint ready to send
 *
 * @ept: pointer to rpmsg endpoint
 *
 * Returns 1 if the rpmsg endpoint has both local addr and destination
 * addr set, 0 otherwise
 */
static inline unsigned int is_rpmsg_ept_ready(struct rpmsg_endpoint *ept)
{
	return (ept->dest_addr != RPMSG_ADDR_ANY &&
		ept->addr != RPMSG_ADDR_ANY);
}

#if defined __cplusplus
}
#endif

#endif				/* _RPMSG_H_ */
