/*
 * Copyright (c) 2018 Linaro
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <usb/usbd.h>
#include <usbd_internal.h>
#include <net/buf.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(usbd_buf, CONFIG_USBD_LOG_LEVEL);

static K_KERNEL_STACK_DEFINE(usbd_buf_stack, 1024);
static struct k_thread usbd_buf_thread_data;
static struct k_fifo usbd_buf_queue;

static sys_slist_t usbd_buf_slist;

/* TODO: move to Kconfig */
#define USBD_POOL_COUNT		8
#define USBD_POOL_BUF_SIZE	1024

NET_BUF_POOL_DEFINE(usbd_pool, USBD_POOL_COUNT, USBD_POOL_BUF_SIZE,
		    sizeof(struct usbd_buf_ud), NULL);

/*
 * The function is the interface to the legacy USB driver API.
 * It was derived from the usb_transfer but can also serve
 * control endpoints.
 *
 * Note: Must be revised during the change of USB driver API.
 */
static void usbd_tbuf_handler(struct net_buf *buf)
{
	struct usbd_buf_ud *ud = NULL;
	int slist_ret = 0;
	int err = 0;
	uint32_t bytes = 0;
	unsigned int key;

	ud = (struct usbd_buf_ud *)net_buf_user_data(buf);
	LOG_DBG("ep 0x%02x, type 0x%02x, flags 0x%02x, status 0x%02x",
		ud->ep, ud->type, ud->flags, ud->status);
	LOG_DBG("len %u, size %u", buf->len, buf->size);

	if (ud->flags & USBD_TRANS_WR) {
		if (buf->len == 0) {
			if (ud->flags & USBD_TRANS_ZLP) {
				LOG_DBG("Transfer ZLP");
				usb_dc_ep_write(ud->ep, NULL, 0, NULL);
				/* Clear ZLP flag */
				ud->flags &= ~USBD_TRANS_ZLP;
				return;
			}

			err = 0;
			goto done;
		}

		err = usb_dc_ep_write(ud->ep, buf->data, buf->len, &bytes);
		if (err) {
			LOG_ERR("Transfer error %d, ep 0x%02x", err, ud->ep);
			goto done;
		}

		net_buf_pull(buf, bytes);
	} else {
		err = usb_dc_ep_read_wait(ud->ep, net_buf_tail(buf), buf->size,
					  &bytes);
		if (err) {
			LOG_ERR("Transfer error %d, ep 0x%02x", err, ud->ep);
			goto done;
		}

		net_buf_add(buf, bytes);

		/* ZLP, short-pkt or buffer full */
		if (!bytes || (bytes % usb_dc_ep_mps(ud->ep)) ||
		    buf->len >= buf->size) {
			/* transfer complete */
			err = 0;
			goto done;
		}

		/* We expect mote data, clear NAK */
		usb_dc_ep_read_continue(ud->ep);
	}

	return;

done:
	key = irq_lock();
	slist_ret = sys_slist_find_and_remove(&usbd_buf_slist, &ud->node);
	irq_unlock(key);

	if (!slist_ret) {
		LOG_ERR("Could not find buffer instance");
		__asm volatile ("bkpt #0\n");
		k_panic();
	}

	LOG_DBG("Done, ep 0x%02x, status %d, len %zu",
		ud->ep, ud->status, buf->len);

	struct usbd_class_ctx *cctx;
	cctx = usbd_cctx_get_by_ep(ud->ep);
	cctx->ops.ep_cb(cctx, buf, err);
}

static void usbd_buf_thread(void)
{
	while (true) {
		struct net_buf *buf;

		buf = net_buf_get(&usbd_buf_queue, K_FOREVER);
		usbd_tbuf_handler(buf);
		k_yield();
	}
}

uint8_t usbd_get_ep_from_buf(struct net_buf *buf)
{
	struct usbd_buf_ud *ud = NULL;

	ud = (struct usbd_buf_ud *)net_buf_user_data(buf);

	return ud->ep;
}

/* Get buffer corresponding to the endpoint */
static struct net_buf *usbd_tbuf_get_buf(uint8_t ep)
{
	struct net_buf *buf = NULL;
	struct usbd_buf_ud *ud = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&usbd_buf_slist, ud, node) {
		buf = CONTAINER_OF(ud, struct net_buf, user_data);

		if (ep == ud->ep) {
			return buf;
		}
	}

	return NULL;
}

/* This is the common callback for all endpoints */
void usbd_tbuf_ep_cb(uint8_t ep, enum usb_dc_ep_cb_status_code status)
{
	struct net_buf *buf = NULL;
	struct usbd_buf_ud *ud = NULL;
	unsigned int key;

	key = irq_lock();
	buf = usbd_tbuf_get_buf(ep);
	irq_unlock(key);

	if (buf == NULL) {
		LOG_ERR("Unlinked callback for 0x%02x, status 0x%02x",
			ep, status);
		return;
	}

	ud = (struct usbd_buf_ud *)net_buf_user_data(buf);

	ud->status = status;
	net_buf_put(&usbd_buf_queue, buf);
	/*
	 * We have to yield here, otherwise the nRF driver will
	 * continue to run, release "setup stage" and at least one
	 * packet will be lost.
	 */
	k_yield();
}

/**
 * @brief Allocate a new buffer for the transfer
 *
 * @note Must be revised after the change of USB driver API.
 *
 * @param[in] ep Endpoint address
 * @param[in] size Buffer size
 *
 * @return Buffer pointer on success, NULL pointer on fail.
 */
struct net_buf *usbd_tbuf_alloc(uint8_t ep, size_t size)
{
	struct net_buf *buf;
	struct usbd_buf_ud *ud;

	LOG_DBG("Allocate net_buf, ep 0x%02x, size %zd", ep, size);

	buf = net_buf_alloc_len(&usbd_pool, size, K_NO_WAIT);
	if (!buf) {
		LOG_ERR("Cannot get free buffer");
		return NULL;
	}

	if (buf->size < size) {
		LOG_ERR("net buf length too small, wrong configuration?");
		net_buf_unref(buf);
		return NULL;
	}

	ud = (struct usbd_buf_ud *)net_buf_user_data(buf);
	ud->ep = ep;
	ud->type = 0;
	ud->flags = 0;
	ud->status = 0;

	return buf;
}

/**
 * @brief Submit a new transfer
 *
 * The function uses information about the endpoint stored in
 * the net_buf's user data and determines the direction of the
 * transfer. There is also additional slist node entry to find
 * the buffer back based on endpoint address.
 *
 * If one transfer already exists for the endpoint,
 * another cannot be started.
 *
 * @note Must be revised/removed after the change of USB driver API.
 *
 * @param[in] buf Pointer to net_buf structure
 * @param[in] handle_zlp Enable IN endpoint transfer ZLP handling
 *
 * @return 0 on success, other values on fail.
 */
int usbd_tbuf_submit(struct net_buf *buf, bool handle_zlp)
{
	struct usbd_buf_ud *ud = NULL;
	unsigned int key;

	ud = (struct usbd_buf_ud *)net_buf_user_data(buf);

	key = irq_lock();
	if (usbd_tbuf_get_buf(ud->ep) != NULL) {
		irq_unlock(key);
		LOG_ERR("ep busy");
		k_panic();
		return -EBUSY;
	}

	sys_slist_append(&usbd_buf_slist, &ud->node);
	irq_unlock(key);

	ud->flags = 0;

	if (USB_EP_DIR_IS_IN(ud->ep)) {
		ud->flags |= USBD_TRANS_WR;

		if (handle_zlp == true &&
		    !(buf->len % usb_dc_ep_mps(ud->ep))) {
			/* Add ZLP if the buffer length is multiple of MPS */
			LOG_DBG("len %u, ZLP will be added!", buf->len);
			ud->flags |= USBD_TRANS_ZLP;
		}

		/* Start writing first chunk */
		net_buf_put(&usbd_buf_queue, buf);
	} else {
		ud->flags |= USBD_TRANS_RD;
		/* ready to read, clear NAK */
		usb_dc_ep_read_continue(ud->ep);
	}

	LOG_DBG("link %p with 0x%02x len %u flags %x",
		buf, ud->ep, buf->len, buf->flags);

	return 0;
}

/**
 * @brief Cancel transfer
 *
 * This function cancels transfer on specific endpoint.
 *
 * May only be applied to disabled endpoint.
 *
 * @note Must be revised after the change of USB driver API.
 *
 * @param[in] ep Endpoint descriptor
 *
 * @return 0 on success, other values on fail.
 */
int usbd_tbuf_cancel(uint8_t ep)
{
	struct net_buf *buf = NULL;
	struct usbd_buf_ud *ud = NULL;
	bool ret;
	unsigned int key;

	buf = usbd_tbuf_get_buf(ep);

	if (buf == NULL) {
		LOG_WRN("Nothing is linked with 0x%02x", ep);
		return 0;
	}

	ud = (struct usbd_buf_ud *)net_buf_user_data(buf);

	key = irq_lock();
	ret = sys_slist_find_and_remove(&usbd_buf_slist, &ud->node);
	irq_unlock(key);

	if (ret == false) {
		LOG_ERR("Could not find buffer instance");
		return -ESRCH;
	}

	net_buf_unref(buf);

	return 0;
}

/**
 * @brief Initiate net_buf based transfer handling
 *
 * @return 0 on success, other values on fail.
 */
int usbd_tbuf_init(void)
{
	k_fifo_init(&usbd_buf_queue);

	k_thread_create(&usbd_buf_thread_data, usbd_buf_stack,
			K_KERNEL_STACK_SIZEOF(usbd_buf_stack),
			(k_thread_entry_t)usbd_buf_thread,
			NULL, NULL, NULL, K_PRIO_COOP(8), 0, K_NO_WAIT);

	k_thread_name_set(&usbd_buf_thread_data, "usbd_tbuf");

	return 0;
}
