/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2016 Freescale Semiconductor, Inc. All rights reserved.
 * Copyright (c) 2018 Linaro, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <openamp/rpmsg.h>
#include <metal/alloc.h>
#include <metal/utilities.h>

#include "rpmsg_internal.h"

/**
 * rpmsg_get_address
 *
 * This function provides unique 32 bit address.
 *
 * @param bitmap - bit map for addresses
 * @param size   - size of bitmap
 *
 * return - a unique address
 */
static uint32_t rpmsg_get_address(unsigned long *bitmap, int size)
{
	unsigned int addr = RPMSG_ADDR_ANY;
	unsigned int nextbit;

	nextbit = metal_bitmap_next_clear_bit(bitmap, 0, size);
	if (nextbit < (uint32_t)size) {
		addr = nextbit;
		metal_bitmap_set_bit(bitmap, nextbit);
	}

	return addr;
}

/**
 * rpmsg_release_address
 *
 * Frees the given address.
 *
 * @param bitmap - bit map for addresses
 * @param size   - size of bitmap
 * @param addr   - address to free
 */
static void rpmsg_release_address(unsigned long *bitmap, int size,
				  int addr)
{
	if (addr < size)
		metal_bitmap_clear_bit(bitmap, addr);
}

/**
 * rpmsg_is_address_set
 *
 * Checks whether address is used or free.
 *
 * @param bitmap - bit map for addresses
 * @param size   - size of bitmap
 * @param addr   - address to free
 *
 * return - TRUE/FALSE
 */
static int rpmsg_is_address_set(unsigned long *bitmap, int size, int addr)
{
	if (addr < size)
		return metal_bitmap_is_bit_set(bitmap, addr);
	else
		return RPMSG_ERR_PARAM;
}

/**
 * rpmsg_set_address
 *
 * Marks the address as consumed.
 *
 * @param bitmap - bit map for addresses
 * @param size   - size of bitmap
 * @param addr   - address to free
 *
 * return - none
 */
static int rpmsg_set_address(unsigned long *bitmap, int size, int addr)
{
	if (addr < size) {
		metal_bitmap_set_bit(bitmap, addr);
		return RPMSG_SUCCESS;
	} else {
		return RPMSG_ERR_PARAM;
	}
}

/**
 * This function sends rpmsg "message" to remote device.
 *
 * @param ept     - pointer to end point
 * @param src     - source address of channel
 * @param dst     - destination address of channel
 * @param data    - data to transmit
 * @param size    - size of data
 * @param wait    - boolean, wait or not for buffer to become
 *                  available
 *
 * @return - size of data sent or negative value for failure.
 *
 */
int rpmsg_send_offchannel_raw(struct rpmsg_endpoint *ept, uint32_t src,
			      uint32_t dst, const void *data, int size,
			      int wait)
{
	struct rpmsg_device *rdev;

	if (!ept || !ept->rdev || !data || dst == RPMSG_ADDR_ANY)
		return RPMSG_ERR_PARAM;

	rdev = ept->rdev;

	if (rdev->ops.send_offchannel_raw)
		return rdev->ops.send_offchannel_raw(rdev, src, dst, data,
						      size, wait);

	return RPMSG_ERR_PARAM;
}

int rpmsg_send_ns_message(struct rpmsg_endpoint *ept, unsigned long flags)
{
	struct rpmsg_ns_msg ns_msg;
	int ret;

	ns_msg.flags = flags;
	ns_msg.addr = ept->addr;
	strncpy(ns_msg.name, ept->name, sizeof(ns_msg.name));
	ret = rpmsg_send_offchannel_raw(ept, ept->addr,
					RPMSG_NS_EPT_ADDR,
					&ns_msg, sizeof(ns_msg), true);
	if (ret < 0)
		return ret;
	else
		return RPMSG_SUCCESS;
}

struct rpmsg_endpoint *rpmsg_get_endpoint(struct rpmsg_device *rdev,
					  const char *name, uint32_t addr,
					  uint32_t dest_addr)
{
	struct metal_list *node;
	struct rpmsg_endpoint *ept;

	metal_list_for_each(&rdev->endpoints, node) {
		int name_match = 0;

		ept = metal_container_of(node, struct rpmsg_endpoint, node);
		/* try to get by local address only */
		if (addr != RPMSG_ADDR_ANY && ept->addr == addr)
			return ept;
		/* try to find match on local end remote address */
		if (addr == ept->addr && dest_addr == ept->dest_addr)
			return ept;
		/* else use name service and destination address */
		if (name)
			name_match = !strncmp(ept->name, name,
					      sizeof(ept->name));
		if (!name || !name_match)
			continue;
		/* destination address is known, equal to ept remote address*/
		if (dest_addr != RPMSG_ADDR_ANY && ept->dest_addr == dest_addr)
			return ept;
		/* ept is registered but not associated to remote ept*/
		if (addr == RPMSG_ADDR_ANY && ept->dest_addr == RPMSG_ADDR_ANY)
			return ept;
	}
	return NULL;
}

static void rpmsg_unregister_endpoint(struct rpmsg_endpoint *ept)
{
	struct rpmsg_device *rdev;

	if (!ept)
		return;

	rdev = ept->rdev;

	if (ept->addr != RPMSG_ADDR_ANY)
		rpmsg_release_address(rdev->bitmap, RPMSG_ADDR_BMP_SIZE,
				      ept->addr);
	metal_list_del(&ept->node);
}

int rpmsg_register_endpoint(struct rpmsg_device *rdev,
			    struct rpmsg_endpoint *ept)
{
	ept->rdev = rdev;

	metal_list_add_tail(&rdev->endpoints, &ept->node);
	return RPMSG_SUCCESS;
}

int rpmsg_create_ept(struct rpmsg_endpoint *ept, struct rpmsg_device *rdev,
		     const char *name, uint32_t src, uint32_t dest,
		     rpmsg_ept_cb cb, rpmsg_ns_unbind_cb unbind_cb)
{
	int status;
	uint32_t addr = src;

	if (!ept)
		return RPMSG_ERR_PARAM;

	metal_mutex_acquire(&rdev->lock);
	if (src != RPMSG_ADDR_ANY) {
		status = rpmsg_is_address_set(rdev->bitmap,
					      RPMSG_ADDR_BMP_SIZE, src);
		if (!status) {
			/* Mark the address as used in the address bitmap. */
			rpmsg_set_address(rdev->bitmap, RPMSG_ADDR_BMP_SIZE,
					  src);
		} else if (status > 0) {
			status = RPMSG_SUCCESS;
			goto ret_status;
		} else {
			goto ret_status;
		}
	} else {
		addr = rpmsg_get_address(rdev->bitmap, RPMSG_ADDR_BMP_SIZE);
	}

	rpmsg_init_ept(ept, name, addr, dest, cb, unbind_cb);

	status = rpmsg_register_endpoint(rdev, ept);
	if (status < 0)
		rpmsg_release_address(rdev->bitmap, RPMSG_ADDR_BMP_SIZE, addr);

	if (!status  && ept->dest_addr == RPMSG_ADDR_ANY) {
		/* Send NS announcement to remote processor */
		metal_mutex_release(&rdev->lock);
		status = rpmsg_send_ns_message(ept, RPMSG_NS_CREATE);
		metal_mutex_acquire(&rdev->lock);
		if (status)
			rpmsg_unregister_endpoint(ept);
	}

ret_status:
	metal_mutex_release(&rdev->lock);
	return status;
}

/**
 * rpmsg_destroy_ept
 *
 * This function deletes rpmsg endpoint and performs cleanup.
 *
 * @param ept - pointer to endpoint to destroy
 *
 */
void rpmsg_destroy_ept(struct rpmsg_endpoint *ept)
{
	struct rpmsg_device *rdev;

	if (!ept)
		return;

	rdev = ept->rdev;
	if (ept->addr != RPMSG_NS_EPT_ADDR)
		(void)rpmsg_send_ns_message(ept, RPMSG_NS_DESTROY);
	metal_mutex_acquire(&rdev->lock);
	rpmsg_unregister_endpoint(ept);
	metal_mutex_release(&rdev->lock);
}
