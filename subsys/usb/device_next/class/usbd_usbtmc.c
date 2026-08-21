/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_usbtmc_device

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_tmc.h>
#include <zephyr/usb/class/usbd_usbtmc.h>

#include <zephyr/drivers/usb/udc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_usbtmc, CONFIG_USBD_USBTMC_LOG_LEVEL);

/*
 * This implements the transport of the USB Test and Measurement Class
 * specification revision 1.0 with bInterfaceProtocol USBTMC. Vendor specific
 * messages, the USB488 subclass, and Bulk-IN termination character support
 * are not implemented. Interpretation of the device dependent message
 * payload, typically IEEE 488.2 or SCPI commands, is left to the
 * application.
 */

#define USBTMC_NUM_INSTANCES		DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)

BUILD_ASSERT((CONFIG_USBD_USBTMC_BUF_POOL_SIZE % USBD_MAX_BULK_MPS) == 0,
	     "USBD_USBTMC_BUF_POOL_SIZE is not a multiple of bulk endpoint MPS");

/* Each instance has at most one Bulk-OUT and one Bulk-IN transfer queued. */
UDC_BUF_POOL_DEFINE(usbtmc_ep_pool, USBTMC_NUM_INSTANCES * 2,
		    CONFIG_USBD_USBTMC_BUF_POOL_SIZE,
		    sizeof(struct udc_buf_info), NULL);

struct usbtmc_tx_meta {
	/* Buffer holds the last chunk of a message */
	bool eom;
};

NET_BUF_POOL_DEFINE(usbtmc_tx_pool,
		    USBTMC_NUM_INSTANCES * CONFIG_USBD_USBTMC_TX_BUF_COUNT,
		    CONFIG_USBD_USBTMC_BUF_POOL_SIZE,
		    sizeof(struct usbtmc_tx_meta), NULL);

struct usbd_usbtmc_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
#if USBD_SUPPORTS_HIGH_SPEED
	struct usb_ep_descriptor if0_hs_out_ep;
	struct usb_ep_descriptor if0_hs_in_ep;
#endif
	struct usb_desc_header nil_desc;
};

enum {
	USBTMC_CLASS_ENABLED,
};

enum usbtmc_out_state {
	/* The next Bulk-OUT transaction starts a new transfer with a header */
	USBTMC_OUT_EXPECT_HEADER,
	/* More transactions of a DEV_DEP_MSG_OUT transfer are expected */
	USBTMC_OUT_DATA,
	/* Bulk-OUT endpoint is halted until the host clears the halt */
	USBTMC_OUT_HALTED,
};

/* State of a class specific request split transaction, USBTMC 1.0, 4.2.1.1 */
enum usbtmc_split_state {
	USBTMC_SPLIT_NONE,
	USBTMC_SPLIT_ABORT_BULK_OUT,
	USBTMC_SPLIT_ABORT_BULK_IN,
	USBTMC_SPLIT_CLEAR,
};

struct usbtmc_config {
	/* Pointer to the associated USBD class node */
	struct usbd_class_data *c_data;
	/* Pointer to the interface description node or NULL */
	struct usbd_desc_node *const if_desc_data;
	/* Pointer to the class interface descriptors */
	struct usbd_usbtmc_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
};

struct usbtmc_data {
	const struct usbd_usbtmc_ops *ops;
	atomic_t state;
	/* Protects the transfer and split transaction state below */
	struct k_spinlock lock;

	/* Bulk-OUT transfer state */
	enum usbtmc_out_state out_state;
	/* A Bulk-OUT transfer buffer is queued */
	bool out_engaged;
	/* The next message data chunk is the first chunk of a new message */
	bool out_begin;
	/* EOM attribute of the current DEV_DEP_MSG_OUT transfer */
	bool out_eom;
	/* bTag of the current or most recent Bulk-OUT transfer */
	uint8_t out_btag;
	/* Message data bytes remaining in the current transfer */
	uint32_t out_data_remaining;
	/* Transfer bytes remaining, including alignment bytes */
	uint32_t out_wire_remaining;
	/* Message data bytes forwarded to the application in the current or
	 * most recent transfer, reported in CHECK_ABORT_BULK_OUT_STATUS
	 */
	uint32_t out_nbytes_rxd;

	/* A Bulk-IN transfer buffer is queued */
	bool in_engaged;
	/* A REQUEST_DEV_DEP_MSG_IN transfer is in progress */
	bool in_req_active;
	/* A short packet terminating an aborted transfer must be queued */
	bool in_zlp_pending;
	/* bTag of the current or most recent REQUEST_DEV_DEP_MSG_IN */
	uint8_t in_btag;
	/* Maximum number of message data bytes the host accepts */
	uint32_t in_req_size;
	/* Message data bytes sent in the current or most recent transfer,
	 * reported in CHECK_ABORT_BULK_IN_STATUS
	 */
	uint32_t in_nbytes_txd;

	/* Message data staged by the application */
	sys_slist_t tx_list;
	uint8_t tx_staged;

	/* Split transaction state */
	enum usbtmc_split_state split;
	bool split_busy;
};

static uint8_t usbtmc_get_bulk_out(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const struct usbtmc_config *cfg = dev->config;
	struct usbd_usbtmc_desc *desc = cfg->desc;

#if USBD_SUPPORTS_HIGH_SPEED
	if (usbd_bus_speed(usbd_class_get_ctx(c_data)) == USBD_SPEED_HS) {
		return desc->if0_hs_out_ep.bEndpointAddress;
	}
#endif

	return desc->if0_out_ep.bEndpointAddress;
}

static uint8_t usbtmc_get_bulk_in(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const struct usbtmc_config *cfg = dev->config;
	struct usbd_usbtmc_desc *desc = cfg->desc;

#if USBD_SUPPORTS_HIGH_SPEED
	if (usbd_bus_speed(usbd_class_get_ctx(c_data)) == USBD_SPEED_HS) {
		return desc->if0_hs_in_ep.bEndpointAddress;
	}
#endif

	return desc->if0_in_ep.bEndpointAddress;
}

static size_t usbtmc_get_bulk_mps(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return 512U;
	}

	return 64U;
}

static struct net_buf *usbtmc_buf_alloc(const uint8_t ep, const size_t size)
{
	struct udc_buf_info *bi;
	struct net_buf *buf;

	buf = net_buf_alloc(&usbtmc_ep_pool, K_NO_WAIT);
	if (buf == NULL) {
		return NULL;
	}

	/* Limit the length of an OUT transfer to the expected size */
	buf->size = MIN(size, buf->size);
	bi = udc_get_buf_info(buf);
	bi->ep = ep;

	return buf;
}

/* Update the split transaction busy state, requires the instance lock */
static void usbtmc_split_update(struct usbtmc_data *const data)
{
	switch (data->split) {
	case USBTMC_SPLIT_ABORT_BULK_OUT:
		data->split_busy = data->out_engaged;
		break;
	case USBTMC_SPLIT_ABORT_BULK_IN:
		data->split_busy = data->in_engaged || data->in_zlp_pending;
		break;
	case USBTMC_SPLIT_CLEAR:
		data->split_busy = data->out_engaged || data->in_engaged;
		break;
	case USBTMC_SPLIT_NONE:
	default:
		data->split_busy = false;
		break;
	}
}

/* Discard all message data staged by the application, requires the lock */
static void usbtmc_flush_tx(struct usbtmc_data *const data)
{
	struct net_buf *buf;

	while ((buf = (void *)sys_slist_get(&data->tx_list)) != NULL) {
		net_buf_unref(buf);
	}

	data->tx_staged = 0U;
}

/* Queue a Bulk-OUT transfer buffer of the given size */
static int usbtmc_submit_out(struct usbd_class_data *const c_data,
			     const size_t size)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_data *data = dev->data;
	k_spinlock_key_t key;
	struct net_buf *buf;
	int err;

	buf = usbtmc_buf_alloc(usbtmc_get_bulk_out(c_data), size);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate Bulk-OUT buffer");
		return -ENOMEM;
	}

	key = k_spin_lock(&data->lock);
	data->out_engaged = true;
	k_spin_unlock(&data->lock, key);

	err = usbd_ep_enqueue(c_data, buf);
	if (err) {
		LOG_ERR("Failed to enqueue Bulk-OUT buffer");
		net_buf_unref(buf);
		key = k_spin_lock(&data->lock);
		data->out_engaged = false;
		k_spin_unlock(&data->lock, key);
	}

	return err;
}

/*
 * Halt the Bulk-OUT endpoint because of a transfer error, USBTMC 1.0,
 * Table 7. The host clears the halt to recover, and the device then
 * interprets the first transaction of the next transfer as a new header.
 */
static void usbtmc_out_error_halt(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	data->out_state = USBTMC_OUT_HALTED;
	k_spin_unlock(&data->lock, key);

	usbd_ep_set_halt(usbd_class_get_ctx(c_data), usbtmc_get_bulk_out(c_data));
}

/* Forward message data to the application */
static void usbtmc_deliver_msg_out(struct usbd_class_data *const c_data,
				   const uint8_t *const chunk, const size_t len,
				   const bool eom)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_data *data = dev->data;
	bool begin = data->out_begin;

	if (len == 0U) {
		return;
	}

	data->out_nbytes_rxd += len;
	data->out_begin = eom;

	data->ops->msg_out(dev, chunk, len, begin, eom);
}

/* Queue the next Bulk-IN transfer if possible, called from any context */
static void usbtmc_in_kick(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_data *data = dev->data;
	struct usbtmc_msg_header *hdr;
	struct net_buf *staged = NULL;
	struct usbtmc_tx_meta *meta;
	k_spinlock_key_t key;
	struct net_buf *buf;
	bool zlp = false;
	uint8_t btag;
	size_t size = 0;
	bool eom;
	int err;

	key = k_spin_lock(&data->lock);
	if (data->in_engaged) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	if (data->in_zlp_pending) {
		/* Queue a short packet to terminate an aborted transfer,
		 * USBTMC 1.0, 4.2.1.4
		 */
		data->in_zlp_pending = false;
		zlp = true;
	} else if (data->in_req_active) {
		staged = (void *)sys_slist_get(&data->tx_list);
		if (staged == NULL) {
			k_spin_unlock(&data->lock, key);
			return;
		}

		size = MIN(data->in_req_size,
			   CONFIG_USBD_USBTMC_BUF_POOL_SIZE -
			   sizeof(struct usbtmc_msg_header));
		size = MIN(size, staged->len);
	} else {
		k_spin_unlock(&data->lock, key);
		return;
	}

	data->in_engaged = true;
	btag = data->in_btag;
	k_spin_unlock(&data->lock, key);

	buf = usbtmc_buf_alloc(usbtmc_get_bulk_in(c_data),
			       sizeof(struct usbtmc_msg_header) + size);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate Bulk-IN buffer");
		goto restore_state;
	}

	if (!zlp) {
		meta = net_buf_user_data(staged);
		eom = meta->eom && size == staged->len;

		hdr = net_buf_add(buf, sizeof(struct usbtmc_msg_header));
		hdr->MsgID = USBTMC_MSGID_DEV_DEP_MSG_IN;
		hdr->bTag = btag;
		hdr->bTagInverse = (uint8_t)~btag;
		hdr->reserved = 0U;
		hdr->dev_dep_msg_in.TransferSize = sys_cpu_to_le32(size);
		hdr->dev_dep_msg_in.bmTransferAttributes =
			eom ? USBTMC_TRANSFER_ATTRIB_EOM : 0U;
		memset(hdr->dev_dep_msg_in.reserved, 0,
		       sizeof(hdr->dev_dep_msg_in.reserved));
		net_buf_add_mem(buf, staged->data, size);

		/* The device must always terminate a Bulk-IN transfer with a
		 * short packet, if necessary a zero-length packet,
		 * USBTMC 1.0, 3.3
		 */
		if (buf->len % usbtmc_get_bulk_mps(c_data) == 0U) {
			udc_ep_buf_set_zlp(buf);
		}
	}

	err = usbd_ep_enqueue(c_data, buf);
	if (err) {
		LOG_ERR("Failed to enqueue Bulk-IN buffer");
		net_buf_unref(buf);
		goto restore_state;
	}

	if (staged != NULL) {
		key = k_spin_lock(&data->lock);
		data->in_nbytes_txd = size;
		if (size == staged->len) {
			data->tx_staged--;
			k_spin_unlock(&data->lock, key);
			net_buf_unref(staged);
		} else {
			net_buf_pull(staged, size);
			sys_slist_prepend(&data->tx_list, &staged->node);
			k_spin_unlock(&data->lock, key);
		}
	}

	return;

restore_state:
	key = k_spin_lock(&data->lock);
	data->in_engaged = false;
	if (zlp) {
		data->in_zlp_pending = true;
	} else {
		sys_slist_prepend(&data->tx_list, &staged->node);
	}

	k_spin_unlock(&data->lock, key);
}

/*
 * Handle a REQUEST_DEV_DEP_MSG_IN header, USBTMC 1.0, 3.2.1.2.
 * Returns false if the Bulk-OUT endpoint has been halted.
 */
static bool usbtmc_handle_request_msg_in(struct usbd_class_data *const c_data,
					 const struct usbtmc_msg_header *const hdr,
					 const size_t len)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_data *data = dev->data;
	bool dequeue = false;
	k_spinlock_key_t key;
	uint32_t size;
	bool halt_in;

	size = sys_le32_to_cpu(hdr->request_dev_dep_msg_in.TransferSize);

	/* A REQUEST_DEV_DEP_MSG_IN transfer is only a header, the device
	 * does not support the termination character, and EOM must be zero,
	 * USBTMC 1.0, Table 4
	 */
	if (len != sizeof(struct usbtmc_msg_header) || size == 0U ||
	    hdr->request_dev_dep_msg_in.bmTransferAttributes != 0U) {
		LOG_WRN("Invalid REQUEST_DEV_DEP_MSG_IN header");
		usbtmc_out_error_halt(c_data);
		return false;
	}

	key = k_spin_lock(&data->lock);
	halt_in = data->in_req_active || data->in_engaged;
	if (halt_in) {
		/* The transfer is not resumed after the host clears the
		 * halt, the device queues nothing until a new command
		 * message expecting a response is received.
		 */
		dequeue = data->in_engaged;
		data->in_req_active = false;
		data->in_zlp_pending = false;
	} else {
		data->in_req_active = true;
		data->in_btag = hdr->bTag;
		data->in_req_size = size;
		data->in_nbytes_txd = 0U;
	}

	k_spin_unlock(&data->lock, key);

	if (halt_in) {
		/* Command message expecting a response while a Bulk-IN
		 * transfer is in progress, USBTMC 1.0, Table 12
		 */
		LOG_WRN("REQUEST_DEV_DEP_MSG_IN while a transfer is in progress");
		usbd_ep_set_halt(usbd_class_get_ctx(c_data),
				 usbtmc_get_bulk_in(c_data));

		if (dequeue) {
			usbd_ep_dequeue(usbd_class_get_ctx(c_data),
					usbtmc_get_bulk_in(c_data));
		}
	} else {
		usbtmc_in_kick(c_data);
	}

	return true;
}

/*
 * Handle the first transaction of a Bulk-OUT transfer, which starts with a
 * message header, USBTMC 1.0, 3.2. Returns the size of the next Bulk-OUT
 * transfer buffer to queue, or zero if the endpoint has been halted.
 */
static size_t usbtmc_handle_header(struct usbd_class_data *const c_data,
				   struct net_buf *const buf)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const size_t mps = usbtmc_get_bulk_mps(c_data);
	struct usbtmc_data *data = dev->data;
	struct usbtmc_msg_header *hdr;
	k_spinlock_key_t key;
	uint32_t wire_total;
	uint32_t payload;
	uint32_t size;
	bool eom;

	if (buf->len < sizeof(struct usbtmc_msg_header)) {
		/* Incomplete header, USBTMC 1.0, Table 7 */
		LOG_WRN("Incomplete Bulk-OUT header");
		usbtmc_out_error_halt(c_data);
		return 0;
	}

	hdr = (struct usbtmc_msg_header *)buf->data;
	if (hdr->bTagInverse != (uint8_t)~hdr->bTag) {
		/* Illegal field combination, USBTMC 1.0, Table 7 */
		LOG_WRN("bTagInverse is not the complement of bTag");
		usbtmc_out_error_halt(c_data);
		return 0;
	}

	data->out_btag = hdr->bTag;

	switch (hdr->MsgID) {
	case USBTMC_MSGID_DEV_DEP_MSG_OUT:
		break;
	case USBTMC_MSGID_REQUEST_DEV_DEP_MSG_IN:
		if (usbtmc_handle_request_msg_in(c_data, hdr, buf->len)) {
			return mps;
		}

		return 0;
	default:
		/* Unsupported MsgID, vendor specific messages are not
		 * supported, USBTMC 1.0, Table 7
		 */
		LOG_WRN("Unsupported MsgID %u", hdr->MsgID);
		usbtmc_out_error_halt(c_data);
		return 0;
	}

	size = sys_le32_to_cpu(hdr->dev_dep_msg_out.TransferSize);
	if (size == 0U) {
		LOG_WRN("Invalid DEV_DEP_MSG_OUT TransferSize");
		usbtmc_out_error_halt(c_data);
		return 0;
	}

	/* Bulk-OUT transactions are a multiple of four bytes long, the host
	 * adds up to three alignment bytes to the last transaction of a
	 * transfer, USBTMC 1.0, 3.2
	 */
	wire_total = sizeof(struct usbtmc_msg_header) +
		     ROUND_UP(size, USBTMC_ALIGNMENT);
	eom = hdr->dev_dep_msg_out.bmTransferAttributes & USBTMC_TRANSFER_ATTRIB_EOM;
	payload = MIN(buf->len - sizeof(struct usbtmc_msg_header), size);

	key = k_spin_lock(&data->lock);
	data->out_eom = eom;
	data->out_nbytes_rxd = 0U;
	data->out_data_remaining = size - payload;
	data->out_wire_remaining = wire_total - MIN(buf->len, wire_total);
	k_spin_unlock(&data->lock, key);

	usbtmc_deliver_msg_out(c_data, buf->data + sizeof(struct usbtmc_msg_header),
			       payload, eom && data->out_data_remaining == 0U);

	if (buf->len > wire_total) {
		/* More bytes than expected, the expected message data bytes
		 * are forwarded and the rest is discarded, USBTMC 1.0, Table 7
		 */
		LOG_WRN("Too many bytes in Bulk-OUT transfer");
		usbtmc_out_error_halt(c_data);
		return 0;
	}

	if (data->out_wire_remaining == 0U) {
		return mps;
	}

	if (buf->len < mps) {
		/* The transfer ended with a short packet but fewer bytes than
		 * expected were received, the received message data bytes are
		 * forwarded and EOM is ignored, USBTMC 1.0, Table 7
		 */
		LOG_WRN("Too few bytes in Bulk-OUT transfer");
		usbtmc_out_error_halt(c_data);
		return 0;
	}

	data->out_state = USBTMC_OUT_DATA;

	return MIN(CONFIG_USBD_USBTMC_BUF_POOL_SIZE,
		   ROUND_UP(data->out_wire_remaining, mps));
}

/*
 * Handle a continuation of a DEV_DEP_MSG_OUT transfer. Returns the size of
 * the next Bulk-OUT transfer buffer to queue, or zero if the endpoint has
 * been halted.
 */
static size_t usbtmc_handle_out_data(struct usbd_class_data *const c_data,
				     struct net_buf *const buf)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const size_t mps = usbtmc_get_bulk_mps(c_data);
	struct usbtmc_data *data = dev->data;
	uint32_t wire_before = data->out_wire_remaining;
	k_spinlock_key_t key;
	uint32_t payload;

	payload = MIN(buf->len, data->out_data_remaining);

	key = k_spin_lock(&data->lock);
	data->out_data_remaining -= payload;
	data->out_wire_remaining -= MIN(buf->len, wire_before);
	k_spin_unlock(&data->lock, key);

	usbtmc_deliver_msg_out(c_data, buf->data, payload,
			       data->out_eom && data->out_data_remaining == 0U);

	if (buf->len > wire_before) {
		/* More bytes than expected, USBTMC 1.0, Table 7 */
		LOG_WRN("Too many bytes in Bulk-OUT transfer");
		usbtmc_out_error_halt(c_data);
		return 0;
	}

	if (data->out_wire_remaining == 0U) {
		data->out_state = USBTMC_OUT_EXPECT_HEADER;
		return mps;
	}

	if (buf->len < buf->size) {
		/* The transfer ended with a short packet but fewer bytes than
		 * expected were received, USBTMC 1.0, Table 7
		 */
		LOG_WRN("Too few bytes in Bulk-OUT transfer");
		usbtmc_out_error_halt(c_data);
		return 0;
	}

	return MIN(CONFIG_USBD_USBTMC_BUF_POOL_SIZE,
		   ROUND_UP(data->out_wire_remaining, mps));
}

static void usbtmc_out_completion(struct usbd_class_data *const c_data,
				  struct net_buf *const buf, const int err)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_data *data = dev->data;
	k_spinlock_key_t key;
	size_t next_size = 0;

	key = k_spin_lock(&data->lock);
	data->out_engaged = false;
	usbtmc_split_update(data);
	k_spin_unlock(&data->lock, key);

	if (err != 0) {
		if (err != -ECONNABORTED) {
			LOG_ERR("Bulk-OUT transfer failed, error %d", err);
		}

		usbd_ep_buf_free(uds_ctx, buf);
		return;
	}

	if (!atomic_test_bit(&data->state, USBTMC_CLASS_ENABLED)) {
		usbd_ep_buf_free(uds_ctx, buf);
		return;
	}

	switch (data->out_state) {
	case USBTMC_OUT_EXPECT_HEADER:
		next_size = usbtmc_handle_header(c_data, buf);
		break;
	case USBTMC_OUT_DATA:
		next_size = usbtmc_handle_out_data(c_data, buf);
		break;
	case USBTMC_OUT_HALTED:
	default:
		/* Discard data received before the endpoint was halted */
		break;
	}

	/* Release the buffer before a new one is allocated */
	usbd_ep_buf_free(uds_ctx, buf);

	if (next_size != 0) {
		usbtmc_submit_out(c_data, next_size);
	}
}

static void usbtmc_in_completion(struct usbd_class_data *const c_data,
				 struct net_buf *const buf, const int err)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_data *data = dev->data;
	bool msg_in_done = false;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	data->in_engaged = false;

	if (err == 0) {
		/* The transfer was terminated with a short packet, a pending
		 * request is complete and an aborted transfer does not need
		 * an additional short packet anymore.
		 */
		msg_in_done = data->in_req_active;
		data->in_req_active = false;
		data->in_zlp_pending = false;
	}

	usbtmc_split_update(data);
	k_spin_unlock(&data->lock, key);

	if (err != 0 && err != -ECONNABORTED) {
		LOG_ERR("Bulk-IN transfer failed, error %d", err);
	}

	/* Release the buffer before a new one is allocated */
	usbd_ep_buf_free(uds_ctx, buf);

	if (msg_in_done && data->ops->msg_in_done != NULL) {
		data->ops->msg_in_done(dev);
	}

	/* Queue a short packet termination of an aborted transfer */
	usbtmc_in_kick(c_data);
}

static int usbtmc_request_handler(struct usbd_class_data *const c_data,
				  struct net_buf *const buf, const int err)
{
	struct udc_buf_info *bi;

	bi = udc_get_buf_info(buf);
	LOG_DBG("Transfer ep 0x%02x, len %u, err %d", bi->ep, buf->len, err);

	if (bi->ep == usbtmc_get_bulk_out(c_data)) {
		usbtmc_out_completion(c_data, buf, err);
	} else if (bi->ep == usbtmc_get_bulk_in(c_data)) {
		usbtmc_in_completion(c_data, buf, err);
	} else {
		usbd_ep_buf_free(usbd_class_get_ctx(c_data), buf);
	}

	return 0;
}

static struct net_buf *usbtmc_response(struct usbd_class_data *const c_data,
				       const struct usb_setup_packet *const setup,
				       const void *const rsp, const size_t size)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const uint16_t len = MIN(size, setup->wLength);
	struct net_buf *buf;

	buf = usbd_ep_ctrl_data_in_alloc(uds_ctx, len);
	if (buf == NULL) {
		return NULL;
	}

	net_buf_add_mem(buf, rsp, len);

	return buf;
}

static struct net_buf *
usbtmc_initiate_abort_bulk_out(struct usbd_class_data *const c_data,
			       const struct usb_setup_packet *const setup)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_initiate_abort_response rsp = {0};
	struct usbtmc_data *data = dev->data;
	bool dequeue = false;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	rsp.bTag = data->out_btag;

	if (data->split_busy) {
		rsp.USBTMC_status = USBTMC_STATUS_SPLIT_IN_PROGRESS;
	} else if (data->out_state != USBTMC_OUT_DATA) {
		/* No transfer is in progress and nothing is buffered because
		 * transfers are processed as they are received
		 */
		rsp.USBTMC_status = USBTMC_STATUS_FAILED;
	} else if ((uint8_t)setup->wValue != data->out_btag) {
		rsp.USBTMC_status = USBTMC_STATUS_TRANSFER_NOT_IN_PROGRESS;
	} else {
		rsp.USBTMC_status = USBTMC_STATUS_SUCCESS;
		data->out_state = USBTMC_OUT_HALTED;
		data->split = USBTMC_SPLIT_ABORT_BULK_OUT;
		dequeue = data->out_engaged;
		data->split_busy = dequeue;
	}

	k_spin_unlock(&data->lock, key);

	if (rsp.USBTMC_status == USBTMC_STATUS_SUCCESS) {
		/* The device must halt the Bulk-OUT endpoint before the
		 * response is queued, USBTMC 1.0, 4.2.1.2
		 */
		usbd_ep_set_halt(usbd_class_get_ctx(c_data),
				 usbtmc_get_bulk_out(c_data));

		if (dequeue) {
			usbd_ep_dequeue(usbd_class_get_ctx(c_data),
					usbtmc_get_bulk_out(c_data));
		}
	}

	return usbtmc_response(c_data, setup, &rsp, sizeof(rsp));
}

static struct net_buf *
usbtmc_check_abort_bulk_out(struct usbd_class_data *const c_data,
			    const struct usb_setup_packet *const setup)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_check_abort_bulk_out_response rsp = {0};
	struct usbtmc_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (data->split != USBTMC_SPLIT_ABORT_BULK_OUT) {
		rsp.USBTMC_status = USBTMC_STATUS_SPLIT_NOT_IN_PROGRESS;
	} else if (data->split_busy) {
		rsp.USBTMC_status = USBTMC_STATUS_PENDING;
	} else {
		rsp.USBTMC_status = USBTMC_STATUS_SUCCESS;
		rsp.NBYTES_RXD = sys_cpu_to_le32(data->out_nbytes_rxd);
		data->split = USBTMC_SPLIT_NONE;
	}

	k_spin_unlock(&data->lock, key);

	return usbtmc_response(c_data, setup, &rsp, sizeof(rsp));
}

static struct net_buf *
usbtmc_initiate_abort_bulk_in(struct usbd_class_data *const c_data,
			      const struct usb_setup_packet *const setup)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_initiate_abort_response rsp = {0};
	struct usbtmc_data *data = dev->data;
	bool dequeue = false;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	rsp.bTag = data->in_btag;

	if (data->split_busy) {
		rsp.USBTMC_status = USBTMC_STATUS_SPLIT_IN_PROGRESS;
	} else if (!data->in_req_active) {
		rsp.USBTMC_status = USBTMC_STATUS_FAILED;
	} else if ((uint8_t)setup->wValue != data->in_btag) {
		rsp.USBTMC_status = USBTMC_STATUS_TRANSFER_NOT_IN_PROGRESS;
	} else {
		/* The Bulk-IN endpoint is not halted, the device queues a
		 * short packet to terminate the transfer and the host reads
		 * until it receives it, USBTMC 1.0, 4.2.1.4. Message data
		 * staged by the application belongs to the aborted response
		 * and is discarded.
		 */
		rsp.USBTMC_status = USBTMC_STATUS_SUCCESS;
		data->in_req_active = false;
		data->in_zlp_pending = true;
		data->split = USBTMC_SPLIT_ABORT_BULK_IN;
		data->split_busy = true;
		dequeue = data->in_engaged;
		usbtmc_flush_tx(data);
	}

	k_spin_unlock(&data->lock, key);

	if (rsp.USBTMC_status == USBTMC_STATUS_SUCCESS) {
		if (dequeue) {
			usbd_ep_dequeue(usbd_class_get_ctx(c_data),
					usbtmc_get_bulk_in(c_data));
		}

		usbtmc_in_kick(c_data);
	}

	return usbtmc_response(c_data, setup, &rsp, sizeof(rsp));
}

static struct net_buf *
usbtmc_check_abort_bulk_in(struct usbd_class_data *const c_data,
			   const struct usb_setup_packet *const setup)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_check_abort_bulk_in_response rsp = {0};
	struct usbtmc_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (data->split != USBTMC_SPLIT_ABORT_BULK_IN) {
		rsp.USBTMC_status = USBTMC_STATUS_SPLIT_NOT_IN_PROGRESS;
	} else if (data->split_busy) {
		rsp.USBTMC_status = USBTMC_STATUS_PENDING;
		rsp.bmAbortBulkIn = USBTMC_BULK_IN_FIFO_BYTES;
	} else {
		rsp.USBTMC_status = USBTMC_STATUS_SUCCESS;
		rsp.NBYTES_TXD = sys_cpu_to_le32(data->in_nbytes_txd);
		data->split = USBTMC_SPLIT_NONE;
	}

	k_spin_unlock(&data->lock, key);

	return usbtmc_response(c_data, setup, &rsp, sizeof(rsp));
}

static struct net_buf *
usbtmc_initiate_clear(struct usbd_class_data *const c_data,
		      const struct usb_setup_packet *const setup)
{
	const struct device *dev = usbd_class_get_private(c_data);
	uint8_t status = USBTMC_STATUS_SUCCESS;
	struct usbtmc_data *data = dev->data;
	bool dequeue_out;
	bool dequeue_in;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (data->split_busy) {
		k_spin_unlock(&data->lock, key);
		status = USBTMC_STATUS_SPLIT_IN_PROGRESS;
		return usbtmc_response(c_data, setup, &status, sizeof(status));
	}

	usbtmc_flush_tx(data);
	data->out_state = USBTMC_OUT_HALTED;
	data->out_begin = true;
	data->in_req_active = false;
	data->in_zlp_pending = false;
	dequeue_out = data->out_engaged;
	dequeue_in = data->in_engaged;
	data->split = USBTMC_SPLIT_CLEAR;
	data->split_busy = dequeue_out || dequeue_in;
	k_spin_unlock(&data->lock, key);

	/* The device must halt the Bulk-OUT endpoint and clear all input and
	 * output buffers, USBTMC 1.0, 4.2.1.6
	 */
	usbd_ep_set_halt(usbd_class_get_ctx(c_data), usbtmc_get_bulk_out(c_data));

	if (dequeue_out) {
		usbd_ep_dequeue(usbd_class_get_ctx(c_data),
				usbtmc_get_bulk_out(c_data));
	}

	if (dequeue_in) {
		usbd_ep_dequeue(usbd_class_get_ctx(c_data),
				usbtmc_get_bulk_in(c_data));
	}

	if (data->ops->clear != NULL) {
		data->ops->clear(dev);
	}

	return usbtmc_response(c_data, setup, &status, sizeof(status));
}

static struct net_buf *
usbtmc_check_clear(struct usbd_class_data *const c_data,
		   const struct usb_setup_packet *const setup)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_check_clear_response rsp = {0};
	struct usbtmc_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (data->split != USBTMC_SPLIT_CLEAR) {
		rsp.USBTMC_status = USBTMC_STATUS_SPLIT_NOT_IN_PROGRESS;
	} else if (data->split_busy) {
		rsp.USBTMC_status = USBTMC_STATUS_PENDING;
	} else {
		rsp.USBTMC_status = USBTMC_STATUS_SUCCESS;
		data->split = USBTMC_SPLIT_NONE;
	}

	k_spin_unlock(&data->lock, key);

	return usbtmc_response(c_data, setup, &rsp, sizeof(rsp));
}

static struct net_buf *
usbtmc_get_capabilities(struct usbd_class_data *const c_data,
			const struct usb_setup_packet *const setup)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_capabilities caps = {0};
	struct usbtmc_data *data = dev->data;

	caps.USBTMC_status = USBTMC_STATUS_SUCCESS;
	caps.bcdUSBTMC = sys_cpu_to_le16(USBTMC_BCD_1_0);

	if (data->ops->indicator_pulse != NULL) {
		caps.bmInterfaceCapabilities = USBTMC_INTF_CAP_INDICATOR_PULSE;
	}

	return usbtmc_response(c_data, setup, &caps, sizeof(caps));
}

static struct net_buf *
usbtmc_indicator_pulse(struct usbd_class_data *const c_data,
		       const struct usb_setup_packet *const setup)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_data *data = dev->data;
	uint8_t status;

	if (data->ops->indicator_pulse == NULL) {
		return NULL;
	}

	if (data->ops->indicator_pulse(dev) == 0) {
		status = USBTMC_STATUS_SUCCESS;
	} else {
		status = USBTMC_STATUS_FAILED;
	}

	return usbtmc_response(c_data, setup, &status, sizeof(status));
}

static struct net_buf *
usbtmc_control_to_host(struct usbd_class_data *const c_data,
		       const struct usb_setup_packet *const setup)
{
	const uint8_t recipient = setup->RequestType.recipient;

	if (recipient == USB_REQTYPE_RECIPIENT_ENDPOINT) {
		const uint8_t ep = (uint8_t)setup->wIndex;

		switch (setup->bRequest) {
		case USBTMC_REQ_INITIATE_ABORT_BULK_OUT:
			if (ep == usbtmc_get_bulk_out(c_data)) {
				return usbtmc_initiate_abort_bulk_out(c_data, setup);
			}

			break;
		case USBTMC_REQ_CHECK_ABORT_BULK_OUT_STATUS:
			if (ep == usbtmc_get_bulk_out(c_data)) {
				return usbtmc_check_abort_bulk_out(c_data, setup);
			}

			break;
		case USBTMC_REQ_INITIATE_ABORT_BULK_IN:
			if (ep == usbtmc_get_bulk_in(c_data)) {
				return usbtmc_initiate_abort_bulk_in(c_data, setup);
			}

			break;
		case USBTMC_REQ_CHECK_ABORT_BULK_IN_STATUS:
			if (ep == usbtmc_get_bulk_in(c_data)) {
				return usbtmc_check_abort_bulk_in(c_data, setup);
			}

			break;
		default:
			break;
		}
	}

	if (recipient == USB_REQTYPE_RECIPIENT_INTERFACE) {
		switch (setup->bRequest) {
		case USBTMC_REQ_INITIATE_CLEAR:
			return usbtmc_initiate_clear(c_data, setup);
		case USBTMC_REQ_CHECK_CLEAR_STATUS:
			return usbtmc_check_clear(c_data, setup);
		case USBTMC_REQ_GET_CAPABILITIES:
			return usbtmc_get_capabilities(c_data, setup);
		case USBTMC_REQ_INDICATOR_PULSE:
			return usbtmc_indicator_pulse(c_data, setup);
		default:
			break;
		}
	}

	LOG_DBG("bmRequestType 0x%02x bRequest 0x%02x unsupported",
		setup->bmRequestType, setup->bRequest);

	return NULL;
}

/* Reset the Bulk-OUT transfer state, requires the instance lock */
static void usbtmc_reset_out_state(struct usbtmc_data *const data)
{
	data->out_state = USBTMC_OUT_EXPECT_HEADER;
	data->out_begin = true;
	data->out_data_remaining = 0U;
	data->out_wire_remaining = 0U;
}

static void usbtmc_feature_halt(struct usbd_class_data *const c_data,
				const uint8_t ep, const bool halted)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_data *data = dev->data;
	k_spinlock_key_t key;
	bool resume = false;

	if (ep != usbtmc_get_bulk_out(c_data) || halted) {
		return;
	}

	/* The host cleared the Bulk-OUT halt, the device interprets the
	 * first transaction of the next transfer as a new header,
	 * USBTMC 1.0, 4.1.1.1
	 */
	key = k_spin_lock(&data->lock);
	if (data->out_state == USBTMC_OUT_HALTED && !data->out_engaged &&
	    atomic_test_bit(&data->state, USBTMC_CLASS_ENABLED)) {
		usbtmc_reset_out_state(data);
		resume = true;
	}

	k_spin_unlock(&data->lock, key);

	if (resume) {
		usbtmc_submit_out(c_data, usbtmc_get_bulk_mps(c_data));
	}
}

static void usbtmc_enable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_data *data = dev->data;
	k_spinlock_key_t key;

	LOG_DBG("Enable %s", c_data->name);

	key = k_spin_lock(&data->lock);
	usbtmc_reset_out_state(data);
	data->in_req_active = false;
	data->in_zlp_pending = false;
	data->split = USBTMC_SPLIT_NONE;
	data->split_busy = false;
	usbtmc_flush_tx(data);
	k_spin_unlock(&data->lock, key);

	atomic_set_bit(&data->state, USBTMC_CLASS_ENABLED);
	usbtmc_submit_out(c_data, usbtmc_get_bulk_mps(c_data));

	if (data->ops->ready != NULL) {
		data->ops->ready(dev, true);
	}
}

static void usbtmc_disable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbtmc_data *data = dev->data;

	LOG_DBG("Disable %s", c_data->name);

	atomic_clear_bit(&data->state, USBTMC_CLASS_ENABLED);

	if (data->ops->ready != NULL) {
		data->ops->ready(dev, false);
	}
}

static void *usbtmc_get_desc(struct usbd_class_data *const c_data,
			     const enum usbd_speed speed)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const struct usbtmc_config *cfg = dev->config;

	if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		return cfg->hs_desc;
	}

	return cfg->fs_desc;
}

static int usbtmc_init(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	const struct usbtmc_config *cfg = dev->config;
	struct usbd_usbtmc_desc *desc = cfg->desc;
	struct usbtmc_data *data = dev->data;

	if (data->ops == NULL) {
		LOG_ERR("Application event handlers are not registered");
		return -EINVAL;
	}

	if (cfg->if_desc_data != NULL && desc->if0.iInterface == 0) {
		if (usbd_add_descriptor(uds_ctx, cfg->if_desc_data)) {
			LOG_ERR("Failed to add interface string descriptor");
		} else {
			desc->if0.iInterface =
				usbd_str_desc_get_idx(cfg->if_desc_data);
		}
	}

	return 0;
}

static struct usbd_class_api usbtmc_api = {
	.feature_halt = usbtmc_feature_halt,
	.control_to_host = usbtmc_control_to_host,
	.request = usbtmc_request_handler,
	.enable = usbtmc_enable,
	.disable = usbtmc_disable,
	.get_desc = usbtmc_get_desc,
	.init = usbtmc_init,
};

int usbd_usbtmc_register(const struct device *dev,
			 const struct usbd_usbtmc_ops *const ops)
{
	struct usbtmc_data *data = dev->data;

	if (ops == NULL || ops->msg_out == NULL) {
		return -EINVAL;
	}

	data->ops = ops;

	return 0;
}

int usbd_usbtmc_msg_write(const struct device *dev, const uint8_t *const data,
			  const size_t len, const bool end)
{
	const struct usbtmc_config *cfg = dev->config;
	struct usbtmc_data *priv = dev->data;
	struct usbtmc_tx_meta *meta;
	k_spinlock_key_t key;
	struct net_buf *buf;
	size_t written = 0;
	size_t size;

	if (!atomic_test_bit(&priv->state, USBTMC_CLASS_ENABLED)) {
		return -EACCES;
	}

	while (written < len) {
		key = k_spin_lock(&priv->lock);
		if (priv->tx_staged >= CONFIG_USBD_USBTMC_TX_BUF_COUNT) {
			k_spin_unlock(&priv->lock, key);
			break;
		}

		priv->tx_staged++;
		k_spin_unlock(&priv->lock, key);

		buf = net_buf_alloc(&usbtmc_tx_pool, K_NO_WAIT);
		if (buf == NULL) {
			key = k_spin_lock(&priv->lock);
			priv->tx_staged--;
			k_spin_unlock(&priv->lock, key);
			break;
		}

		size = MIN(len - written, net_buf_tailroom(buf));
		net_buf_add_mem(buf, &data[written], size);
		written += size;

		meta = net_buf_user_data(buf);
		meta->eom = end && written == len;

		key = k_spin_lock(&priv->lock);
		sys_slist_append(&priv->tx_list, &buf->node);
		k_spin_unlock(&priv->lock, key);
	}

	if (written != 0) {
		usbtmc_in_kick(cfg->c_data);
	}

	return written;
}

#define USBTMC_DEFINE_DESCRIPTOR(n)						\
static struct usbd_usbtmc_desc usbtmc_desc_##n = {				\
	.if0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_APPLICATION,				\
		.bInterfaceSubClass = USBTMC_SUBCLASS,				\
		.bInterfaceProtocol = USBTMC_PROTOCOL_USBTMC,			\
		.iInterface = 0,						\
	},									\
										\
	.if0_out_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64U),				\
		.bInterval = 0,							\
	},									\
										\
	.if0_in_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64U),				\
		.bInterval = 0,							\
	},									\
										\
	IF_ENABLED(USBD_SUPPORTS_HIGH_SPEED, (					\
	.if0_hs_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512U),			\
		.bInterval = 0,							\
	},									\
										\
	.if0_hs_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512U),			\
		.bInterval = 0,							\
	},									\
	))									\
										\
	.nil_desc = {								\
		.bLength = 0,							\
		.bDescriptorType = 0,						\
	},									\
};										\
										\
const static struct usb_desc_header *usbtmc_fs_desc_##n[] = {			\
	(struct usb_desc_header *) &usbtmc_desc_##n.if0,			\
	(struct usb_desc_header *) &usbtmc_desc_##n.if0_out_ep,			\
	(struct usb_desc_header *) &usbtmc_desc_##n.if0_in_ep,			\
	(struct usb_desc_header *) &usbtmc_desc_##n.nil_desc,			\
};										\
										\
IF_ENABLED(USBD_SUPPORTS_HIGH_SPEED, (						\
const static struct usb_desc_header *usbtmc_hs_desc_##n[] = {			\
	(struct usb_desc_header *) &usbtmc_desc_##n.if0,			\
	(struct usb_desc_header *) &usbtmc_desc_##n.if0_hs_out_ep,		\
	(struct usb_desc_header *) &usbtmc_desc_##n.if0_hs_in_ep,		\
	(struct usb_desc_header *) &usbtmc_desc_##n.nil_desc,			\
};										\
))

#define USBD_USBTMC_DT_DEVICE_DEFINE(n)						\
	BUILD_ASSERT(DT_INST_ON_BUS(n, usb),					\
		     "node " DT_NODE_PATH(DT_DRV_INST(n))			\
		     " is not assigned to a USB device controller");		\
										\
	USBTMC_DEFINE_DESCRIPTOR(n)						\
										\
	USBD_DEFINE_CLASS(usbtmc_##n, &usbtmc_api,				\
			  (void *)DEVICE_DT_GET(DT_DRV_INST(n)), NULL);		\
										\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, label), (				\
	USBD_DESC_STRING_DEFINE(usbtmc_if_desc_data_##n,			\
				DT_INST_PROP(n, label),				\
				USBD_DUT_STRING_INTERFACE);			\
	))									\
										\
	static const struct usbtmc_config usbtmc_config_##n = {			\
		.c_data = &usbtmc_##n,						\
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, label), (			\
		.if_desc_data = &usbtmc_if_desc_data_##n,			\
		))								\
		.desc = &usbtmc_desc_##n,					\
		.fs_desc = usbtmc_fs_desc_##n,					\
		.hs_desc = COND_CODE_1(USBD_SUPPORTS_HIGH_SPEED,		\
				       (usbtmc_hs_desc_##n,), (NULL,))		\
	};									\
										\
	static struct usbtmc_data usbtmc_data_##n;				\
										\
	DEVICE_DT_INST_DEFINE(n, NULL, NULL,					\
		&usbtmc_data_##n, &usbtmc_config_##n,				\
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(USBD_USBTMC_DT_DEVICE_DEFINE);
