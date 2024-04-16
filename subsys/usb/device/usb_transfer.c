/*
 * Copyright (c) 2018 Linaro
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/logging/log.h>

#include "usb_transfer.h"
#include "usb_work_q.h"

LOG_MODULE_REGISTER(usb_transfer, CONFIG_USB_DEVICE_LOG_LEVEL);

struct usb_transfer_sync_priv {
	int tsize;
	struct k_sem sem;
};

struct usb_transfer_data {
	/** endpoint associated to the transfer */
	uint8_t ep;
	/** Transfer status */
	int status;
	/** Transfer read/write buffer */
	uint8_t *buffer;
	/** Transfer buffer size */
	size_t bsize;
	/** Transferred size */
	size_t tsize;
	/** Transfer callback */
	usb_transfer_callback cb;
	/** Transfer caller private data */
	void *priv;
	/** Transfer synchronization semaphore */
	struct k_sem sem;
	/** Transfer read/write work */
	struct k_work work;
	/** Transfer flags */
	unsigned int flags;
};

/** Max number of parallel transfers */
static struct usb_transfer_data ut_data[CONFIG_USB_MAX_NUM_TRANSFERS];

/* Transfer management */
static struct usb_transfer_data *usb_ep_get_transfer(uint8_t ep)
{
	for (size_t i = 0; i < ARRAY_SIZE(ut_data); i++) {
		if (ut_data[i].ep == ep && ut_data[i].status != 0) {
			return &ut_data[i];
		}
	}

	return NULL;
}

bool usb_transfer_is_busy(uint8_t ep)
{
	struct usb_transfer_data *trans = usb_ep_get_transfer(ep);

	if (trans && trans->status == -EBUSY) {
		return true;
	}

	return false;
}

static void usb_transfer_work(struct k_work *item)
{
	struct usb_transfer_data *trans;
	int ret = 0;
	uint32_t bytes;
	uint8_t ep;

	trans = CONTAINER_OF(item, struct usb_transfer_data, work);
	ep = trans->ep;

	if (trans->status != -EBUSY) {
		/* transfer cancelled or already completed */
		LOG_DBG("Transfer cancelled or completed, ep 0x%02x", ep);
		goto done;
	}

	if (trans->flags & USB_TRANS_WRITE) {
		if (!trans->bsize) {
			if (!(trans->flags & USB_TRANS_NO_ZLP)) {
				LOG_DBG("Transfer ZLP");
				usb_write(ep, NULL, 0, NULL);
			}
			trans->status = 0;
			goto done;
		}

		ret = usb_write(ep, trans->buffer, trans->bsize, &bytes);
		if (ret) {
			LOG_ERR("Transfer error %d, ep 0x%02x", ret, ep);
			/* transfer error */
			trans->status = -EINVAL;
			goto done;
		}

		trans->buffer += bytes;
		trans->bsize -= bytes;
		trans->tsize += bytes;
	} else {
		ret = usb_dc_ep_read_wait(ep, trans->buffer, trans->bsize,
					  &bytes);
		if (ret) {
			LOG_ERR("Transfer error %d, ep 0x%02x", ret, ep);
			/* transfer error */
			trans->status = -EINVAL;
			goto done;
		}

		trans->buffer += bytes;
		trans->bsize -= bytes;
		trans->tsize += bytes;

		/* ZLP, short-pkt or buffer full */
		if (!bytes || (bytes % usb_dc_ep_mps(ep)) || !trans->bsize) {
			/* transfer complete */
			trans->status = 0;
			goto done;
		}

		/* we expect mote data, clear NAK */
		usb_dc_ep_read_continue(ep);
	}

done:
	if (trans->status != -EBUSY) { /* Transfer complete */
		usb_transfer_callback cb = trans->cb;
		int tsize = trans->tsize;
		void *priv = trans->priv;

		if (k_is_in_isr()) {
			/* reschedule completion in thread context */
			k_work_submit_to_queue(&USB_WORK_Q, &trans->work);
			return;
		}

		LOG_DBG("Transfer done, ep 0x%02x, status %d, size %zu",
			trans->ep, trans->status, trans->tsize);

		trans->cb = NULL;
		k_sem_give(&trans->sem);

		/* Transfer completion callback */
		if (cb) {
			cb(ep, tsize, priv);
		}
	}
}

void usb_transfer_ep_callback(uint8_t ep, enum usb_dc_ep_cb_status_code status)
{
	struct usb_transfer_data *trans = usb_ep_get_transfer(ep);

	if (status != USB_DC_EP_DATA_IN && status != USB_DC_EP_DATA_OUT) {
		return;
	}

	if (!trans) {
		if (status == USB_DC_EP_DATA_OUT) {
			uint32_t bytes;
			/* In the unlikely case we receive data while no
			 * transfer is ongoing, we have to consume the data
			 * anyway. This is to prevent stucking reception on
			 * other endpoints (e.g dw driver has only one rx-fifo,
			 * so drain it).
			 */
			do {
				uint8_t data;

				usb_dc_ep_read_wait(ep, &data, 1, &bytes);
			} while (bytes);

			LOG_ERR("RX data lost, no transfer");
		}
		return;
	}

	if (!k_is_in_isr() || (status == USB_DC_EP_DATA_OUT)) {
		/* If we are not in IRQ context, no need to defer work */
		/* Read (out) needs to be done from ep_callback */
		usb_transfer_work(&trans->work);
	} else {
		k_work_submit_to_queue(&USB_WORK_Q, &trans->work);
	}
}

int usb_transfer(uint8_t ep, uint8_t *data, size_t dlen, unsigned int flags,
		 usb_transfer_callback cb, void *cb_data)
{
	struct usb_transfer_data *trans = NULL;
	int key, ret = 0;

	/* Parallel transfer to same endpoint is not supported. */
	if (usb_transfer_is_busy(ep)) {
		return -EBUSY;
	}

	LOG_DBG("Transfer start, ep 0x%02x, data %p, dlen %zd",
		ep, data, dlen);

	key = irq_lock();

	for (size_t i = 0; i < ARRAY_SIZE(ut_data); i++) {
		if (!k_sem_take(&ut_data[i].sem, K_NO_WAIT)) {
			trans = &ut_data[i];
			break;
		}
	}

	if (!trans) {
		LOG_ERR("No transfer slot available");
		ret = -ENOMEM;
		goto done;
	}

	if (trans->status == -EBUSY) {
		/* A transfer is already ongoing and not completed */
		LOG_ERR("A transfer is already ongoing, ep 0x%02x", ep);
		k_sem_give(&trans->sem);
		ret = -EBUSY;
		goto done;
	}

	/* Configure new transfer */
	trans->ep = ep;
	trans->buffer = data;
	trans->bsize = dlen;
	trans->tsize = 0;
	trans->cb = cb;
	trans->flags = flags;
	trans->priv = cb_data;
	trans->status = -EBUSY;

	if (usb_dc_ep_mps(ep) && (dlen % usb_dc_ep_mps(ep))) {
		/* no need to send ZLP since last packet will be a short one */
		trans->flags |= USB_TRANS_NO_ZLP;
	}

	if (flags & USB_TRANS_WRITE) {
		/* start writing first chunk */
		k_work_submit_to_queue(&USB_WORK_Q, &trans->work);
	} else {
		/* ready to read, clear NAK */
		ret = usb_dc_ep_read_continue(ep);
	}

done:
	irq_unlock(key);
	return ret;
}

void usb_cancel_transfer(uint8_t ep)
{
	struct usb_transfer_data *trans;
	unsigned int key;

	key = irq_lock();

	trans = usb_ep_get_transfer(ep);
	if (!trans) {
		goto done;
	}

	if (trans->status != -EBUSY) {
		goto done;
	}

	trans->status = -ECANCELED;
	k_work_submit_to_queue(&USB_WORK_Q, &trans->work);

done:
	irq_unlock(key);
}

void usb_cancel_transfers(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(ut_data); i++) {
		struct usb_transfer_data *trans = &ut_data[i];
		unsigned int key;

		key = irq_lock();

		if (trans->status == -EBUSY) {
			trans->status = -ECANCELED;
			k_work_submit_to_queue(&USB_WORK_Q, &trans->work);
			LOG_DBG("Cancel transfer for ep: 0x%02x", trans->ep);
		}

		irq_unlock(key);
	}
}

static void usb_transfer_sync_cb(uint8_t ep, int size, void *priv)
{
	struct usb_transfer_sync_priv *pdata = priv;

	pdata->tsize = size;
	k_sem_give(&pdata->sem);
}

int usb_transfer_sync(uint8_t ep, uint8_t *data, size_t dlen, unsigned int flags)
{
	struct usb_transfer_sync_priv pdata;
	int ret;

	k_sem_init(&pdata.sem, 0, 1);

	ret = usb_transfer(ep, data, dlen, flags, usb_transfer_sync_cb, &pdata);
	if (ret) {
		return ret;
	}

	/* Semaphore will be released by the transfer completion callback */
	k_sem_take(&pdata.sem, K_FOREVER);

	return pdata.tsize;
}

/* Init transfer slots */
int usb_transfer_init(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(ut_data); i++) {
		k_work_init(&ut_data[i].work, usb_transfer_work);
		k_sem_init(&ut_data[i].sem, 1, 1);
	}

	return 0;
}
