/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/class/usbh_msc.h>

#include "../usbh_device.h"
#include "../usbh_desc.h"
#include "../usbh_class.h"
#include "../usbh_ch9.h"
#include "usbh_scsi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbh_msc, CONFIG_USBH_MSC_LOG_LEVEL);

#define MSC_SC_SCSI         0x06
#define MSC_PROTO_BULK_ONLY 0x50

#define MSC_CBW_SIGNATURE     0x43425355UL
#define MSC_CSW_SIGNATURE     0x53425355UL
#define MSC_CBW_FLAGS_DATA_IN 0x80U

#define MSC_CSW_STATUS_PASS 0x00U
#define MSC_CSW_STATUS_FAIL 0x01U

#define MSC_CBW_CB_LEN SCSI_MAX_CDB_SIZE

/* A data stage is copied through a single UHC buffer taken from a pool that
 * also has to hold the command and status wrappers plus per-buffer overhead,
 * so a command can never carry the whole pool. Keep a margin and split larger
 * requests into several commands.
 */
#define MSC_POOL_MARGIN 1024U

#define MSC_MAX_DATA_LEN   ((size_t)CONFIG_UHC_BUF_POOL_SIZE - MSC_POOL_MARGIN)
#define MSC_MAX_BLOCK_SIZE 4096U

BUILD_ASSERT(CONFIG_UHC_BUF_POOL_SIZE >= MSC_MAX_BLOCK_SIZE + MSC_POOL_MARGIN,
	     "CONFIG_UHC_BUF_POOL_SIZE must hold one block plus the pool margin");

#define MSC_XFER_TIMEOUT        K_MSEC(CONFIG_USBH_MSC_XFER_TIMEOUT_MS)
#define MSC_UNIT_READY_TIMEOUT  K_MSEC(CONFIG_USBH_MSC_UNIT_READY_TIMEOUT_MS)
#define MSC_UNIT_READY_INTERVAL K_MSEC(CONFIG_USBH_MSC_UNIT_READY_INTERVAL_MS)

struct msc_cbw {
	uint32_t dCBWSignature;
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[MSC_CBW_CB_LEN];
} __packed;

struct msc_csw {
	uint32_t dCSWSignature;
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
} __packed;

struct usbh_msc_lun {
	struct usb_device *udev;
	struct k_mutex mutex;
	struct k_sem xfer_sem;
	uint8_t ep_in;
	uint8_t ep_out;
	uint8_t iface;
	uint32_t tag;
	uint32_t block_count;
	uint32_t block_size;
	struct uhc_transfer *active_xfer;
	int xfer_status;
	bool attached;
	bool ready;
	struct k_work start_work;
};

static struct usbh_msc_lun msc_luns[CONFIG_USBH_MSC_LUN_COUNT];

/* Startup runs SCSI commands that block until the controller replies. It can
 * run neither on the host thread that reports the attach nor on the system
 * workqueue, which a controller driver may use to deliver those replies, so
 * it gets a queue of its own.
 */
static void msc_start_work(struct k_work *work);
static int msc_start(struct usbh_msc_lun *data);

static K_THREAD_STACK_DEFINE(msc_work_stack, CONFIG_USBH_MSC_STACK_SIZE);
static struct k_work_q msc_work_q;

static int msc_xfer_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct usbh_msc_lun *const data = xfer->priv;

	ARG_UNUSED(udev);

	if (data == NULL || data->active_xfer != xfer) {
		return 0;
	}

	data->xfer_status = xfer->err;
	k_sem_give(&data->xfer_sem);

	return 0;
}

static int msc_bulk_xfer(struct usbh_msc_lun *const data, const uint8_t ep, uint8_t *const buf,
			 const size_t len, size_t *const transferred)
{
	struct usb_device *const udev = data->udev;
	const struct device *uhc_dev;
	struct uhc_transfer *xfer;
	struct net_buf *nbuf;
	int ret;

	if (udev == NULL) {
		return -ENODEV;
	}

	/* The controller outlives the USB device, which the removal handler
	 * frees while a command can still be in flight. Keep it to release the
	 * transfer without going through the device.
	 */
	uhc_dev = ((struct usbh_context *)udev->ctx)->dev;

	xfer = usbh_xfer_alloc(udev, ep, msc_xfer_cb, data);
	if (xfer == NULL) {
		return -ENOMEM;
	}

	nbuf = usbh_xfer_buf_alloc(udev, len);
	if (nbuf == NULL) {
		usbh_xfer_free(udev, xfer);
		return -ENOMEM;
	}

	if (!USB_EP_DIR_IS_IN(ep) && len > 0) {
		net_buf_add_mem(nbuf, buf, len);
	}

	ret = usbh_xfer_buf_add(udev, xfer, nbuf);
	if (ret != 0) {
		goto out;
	}

	k_sem_reset(&data->xfer_sem);
	data->active_xfer = xfer;

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		goto out;
	}

	ret = k_sem_take(&data->xfer_sem, MSC_XFER_TIMEOUT);
	if (ret != 0) {
		usbh_xfer_dequeue(udev, xfer);

		/* The dequeue only schedules the cancellation, so wait for the
		 * completion before the buffer and the transfer are released.
		 */
		(void)k_sem_take(&data->xfer_sem, MSC_XFER_TIMEOUT);
		ret = -ETIMEDOUT;
		goto out;
	}

	ret = data->xfer_status;
	if (ret == -ENODEV) {
		/* The device was removed while this command was queued. The
		 * transfer is still owned by the controller, so cancel it and
		 * wait for the completion before releasing it below.
		 */
		(void)uhc_ep_dequeue(uhc_dev, xfer);
		(void)k_sem_take(&data->xfer_sem, MSC_XFER_TIMEOUT);
		goto out;
	}

	if (ret != 0) {
		goto out;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		size_t copy = MIN(nbuf->len, len);

		if (copy > 0) {
			memcpy(buf, nbuf->data, copy);
		}

		if (transferred != NULL) {
			*transferred = copy;
		}
	} else if (transferred != NULL) {
		*transferred = len;
	}

out:
	data->active_xfer = NULL;

	/* Released through the controller rather than the device: the removal
	 * handler frees the device while a command can still be in flight, and
	 * the transfer and the buffer come from the controller's own pools,
	 * which that teardown does not walk.
	 */
	uhc_xfer_buf_free(uhc_dev, nbuf);
	(void)uhc_xfer_free(uhc_dev, xfer);

	return ret;
}

static int msc_scsi_cmd(struct usbh_msc_lun *const data, const uint8_t *const cb,
			const uint8_t cb_len, uint8_t *const buf, const size_t len,
			const bool dir_in, const bool allow_short)
{
	struct msc_cbw cbw;
	struct msc_csw csw;
	size_t transferred = 0;
	size_t data_len = 0;
	uint32_t tag;
	int ret;

	if (len > MSC_MAX_DATA_LEN) {
		LOG_ERR("Data length %zu exceeds the UHC buffer pool", len);
		return -E2BIG;
	}

	tag = ++data->tag;

	memset(&cbw, 0, sizeof(cbw));
	cbw.dCBWSignature = sys_cpu_to_le32(MSC_CBW_SIGNATURE);
	cbw.dCBWTag = sys_cpu_to_le32(tag);
	cbw.dCBWDataTransferLength = sys_cpu_to_le32((uint32_t)len);
	cbw.bmCBWFlags = dir_in ? MSC_CBW_FLAGS_DATA_IN : 0U;
	cbw.bCBWLUN = 0U;
	cbw.bCBWCBLength = cb_len;
	memcpy(cbw.CBWCB, cb, cb_len);

	ret = msc_bulk_xfer(data, data->ep_out, (uint8_t *)&cbw, sizeof(cbw), NULL);
	if (ret != 0) {
		LOG_ERR("CBW transfer failed: %d", ret);
		return ret;
	}

	if (len > 0) {
		ret = msc_bulk_xfer(data, dir_in ? data->ep_in : data->ep_out, buf, len, &data_len);
		if (ret != 0) {
			LOG_ERR("Data stage failed: %d", ret);

			if (data->udev == NULL) {
				/* The device is gone, there is no halt to
				 * clear and no status to collect.
				 */
				return ret;
			}

			/* Bulk-Only Transport requires the halt to be cleared
			 * before the status stage can be read.
			 */
			(void)usbh_req_clear_sfs_halt(data->udev,
						      dir_in ? data->ep_in : data->ep_out);

			/* Collect the status the device still owes, so the
			 * next command does not read a stale CSW.
			 */
			(void)msc_bulk_xfer(data, data->ep_in, (uint8_t *)&csw, sizeof(csw), NULL);
			return ret;
		}
	}

	ret = msc_bulk_xfer(data, data->ep_in, (uint8_t *)&csw, sizeof(csw), &transferred);
	if (ret != 0) {
		LOG_ERR("CSW transfer failed: %d", ret);
		return ret;
	}

	if (transferred < sizeof(csw) || sys_le32_to_cpu(csw.dCSWSignature) != MSC_CSW_SIGNATURE ||
	    sys_le32_to_cpu(csw.dCSWTag) != tag) {
		LOG_ERR("Malformed CSW");
		return -EIO;
	}

	if (csw.bCSWStatus != MSC_CSW_STATUS_PASS) {
		LOG_DBG("Command failed, CSW status %u", csw.bCSWStatus);
		return -EIO;
	}

	/* Bulk-Only Transport allows a device to return less than requested
	 * and still pass the command. Only the block commands need the whole
	 * data stage, because a partially filled buffer cannot be told apart
	 * from complete data. The residue is reported by the device, so the
	 * count the controller actually moved is checked as well.
	 */
	if (!allow_short && data_len != len) {
		LOG_ERR("Short data stage, %zu of %zu", data_len, len);
		return -EIO;
	}

	if (!allow_short && sys_le32_to_cpu(csw.dCSWDataResidue) != 0U) {
		LOG_ERR("Short transfer, residue %u of %zu", sys_le32_to_cpu(csw.dCSWDataResidue),
			len);
		return -EIO;
	}

	return 0;
}

static int msc_test_unit_ready(struct usbh_msc_lun *const data)
{
	uint8_t cb[SCSI_TST_U_RDY_CDB_LEN] = {SCSI_TST_U_RDY};

	return msc_scsi_cmd(data, cb, sizeof(cb), NULL, 0, false, false);
}

static int msc_request_sense(struct usbh_msc_lun *const data)
{
	uint8_t cb[SCSI_REQUEST_SENSE_CDB_LEN] = {SCSI_REQUEST_SENSE};
	uint8_t sense[18] = {0};

	cb[4] = sizeof(sense);

	return msc_scsi_cmd(data, cb, sizeof(cb), sense, sizeof(sense), true, true);
}

static int msc_inquiry(struct usbh_msc_lun *const data)
{
	uint8_t cb[SCSI_INQUIRY_CDB_LEN] = {SCSI_INQUIRY};
	uint8_t resp[36] = {0};
	char vendor[9];
	char product[17];
	int ret;

	cb[4] = sizeof(resp);

	ret = msc_scsi_cmd(data, cb, sizeof(cb), resp, sizeof(resp), true, true);
	if (ret != 0) {
		return ret;
	}

	memcpy(vendor, &resp[8], 8);
	vendor[8] = '\0';
	memcpy(product, &resp[16], 16);
	product[16] = '\0';

	LOG_INF("Vendor '%s' Product '%s'", vendor, product);

	return 0;
}

static int msc_read_capacity(struct usbh_msc_lun *const data)
{
	uint8_t cb[SCSI_READ_CAPACITY_CDB_LEN] = {SCSI_READ_CAPACITY};
	uint8_t resp[SCSI_READ_CAPACITY_RESP_LEN] = {0};
	uint32_t block_count;
	uint32_t block_size;
	int ret;

	ret = msc_scsi_cmd(data, cb, sizeof(cb), resp, sizeof(resp), true, false);
	if (ret != 0) {
		return ret;
	}

	block_count = sys_get_be32(&resp[0]);
	block_size = sys_get_be32(&resp[4]);

	/* The response holds the last addressable block, not the count. */
	if (block_count == UINT32_MAX) {
		LOG_ERR("Medium too large for READ CAPACITY(10)");
		return -ENOTSUP;
	}

	if (block_size == 0U || block_size > MSC_MAX_BLOCK_SIZE ||
	    (block_size & (block_size - 1U)) != 0U) {
		LOG_ERR("Unsupported block size %u", block_size);
		return -ENOTSUP;
	}

	data->block_count = block_count + 1U;
	data->block_size = block_size;

	LOG_INF("Capacity %u blocks of %u bytes", data->block_count, data->block_size);

	return 0;
}

static int msc_check_range(const struct usbh_msc_lun *const data, const uint32_t lba,
			   const uint32_t count)
{
	if (data == NULL || !data->ready) {
		return -ENODEV;
	}

	if (count == 0U || count > data->block_count || lba > data->block_count - count) {
		return -EINVAL;
	}

	return 0;
}

static int msc_rw_blocks(struct usbh_msc_lun *const data, const uint8_t opcode, const uint32_t lba,
			 const uint32_t count, uint8_t *const buf, const bool dir_in)
{
	uint32_t max_blocks;
	uint32_t done = 0;
	int ret;

	if (data->block_size == 0U) {
		return -ENOTSUP;
	}

	max_blocks = MSC_MAX_DATA_LEN / data->block_size;
	if (max_blocks == 0U) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	while (done < count) {
		uint32_t chunk = MIN(MIN(count - done, max_blocks), UINT16_MAX);
		uint8_t cb[SCSI_RW10_CDB_LEN] = {opcode};

		sys_put_be32(lba + done, &cb[2]);
		sys_put_be16((uint16_t)chunk, &cb[7]);

		ret = msc_scsi_cmd(data, cb, sizeof(cb), buf + (size_t)done * data->block_size,
				   (size_t)chunk * data->block_size, dir_in, false);
		if (ret != 0) {
			goto out;
		}

		done += chunk;
	}

	ret = 0;

out:
	k_mutex_unlock(&data->mutex);

	return ret;
}

int usbh_msc_read(struct usbh_msc_lun *const data, const uint32_t lba, const uint32_t count,
		  uint8_t *const buf)
{
	int ret;

	ret = msc_check_range(data, lba, count);
	if (ret != 0) {
		return ret;
	}

	return msc_rw_blocks(data, SCSI_READ10, lba, count, buf, true);
}

int usbh_msc_write(struct usbh_msc_lun *const data, const uint32_t lba, const uint32_t count,
		   const uint8_t *const buf)
{
	int ret;

	ret = msc_check_range(data, lba, count);
	if (ret != 0) {
		return ret;
	}

	return msc_rw_blocks(data, SCSI_WRITE10, lba, count, (uint8_t *)buf, false);
}

int usbh_msc_get_capacity(struct usbh_msc_lun *const data, uint32_t *const block_count,
			  uint32_t *const block_size)
{
	if (data == NULL || !data->ready) {
		return -ENODEV;
	}

	if (block_count == NULL || block_size == NULL) {
		return -EINVAL;
	}

	*block_count = data->block_count;
	*block_size = data->block_size;

	return 0;
}

bool usbh_msc_is_ready(struct usbh_msc_lun *const lun)
{
	return (lun != NULL) && lun->ready;
}

/* GET MAX LUN is not issued, so a multi-unit device is accessed through its
 * first logical unit. One index is one attached device, and indices are
 * handed out in the order the devices are probed.
 */
struct usbh_msc_lun *usbh_msc_lun_get(const unsigned int idx)
{
	if (idx >= ARRAY_SIZE(msc_luns) || !msc_luns[idx].attached) {
		return NULL;
	}

	return &msc_luns[idx];
}

static void msc_start_work(struct k_work *work)
{
	struct usbh_msc_lun *const data = CONTAINER_OF(work, struct usbh_msc_lun, start_work);

	if (msc_start(data) != 0) {
		LOG_ERR("Medium did not become ready");
	}
}

static int usbh_msc_init(struct usbh_class_data *const c_data)
{
	struct usbh_msc_lun *const data = c_data->priv;

	static bool work_q_started;

	k_mutex_init(&data->mutex);
	k_sem_init(&data->xfer_sem, 0, 1);
	k_work_init(&data->start_work, msc_start_work);

	if (!work_q_started) {
		const struct k_work_queue_config cfg = {.name = "usbh_msc"};

		k_work_queue_start(&msc_work_q, msc_work_stack,
				   K_THREAD_STACK_SIZEOF(msc_work_stack),
				   CONFIG_USBH_MSC_THREAD_PRIORITY, &cfg);
		work_q_started = true;
	}

	return 0;
}

static int usbh_msc_probe(struct usbh_class_data *const c_data, struct usb_device *const udev,
			  const uint8_t iface)
{
	struct usbh_msc_lun *const data = c_data->priv;
	const struct usb_ep_descriptor *ep_desc;
	const void *desc;

	if (data->attached) {
		/* This instance already serves a device, let the core try the
		 * next one.
		 */
		return -ENOTSUP;
	}

	desc = usbh_desc_get_iface(udev, iface);
	if (desc == NULL) {
		return -ENOTSUP;
	}

	data->ep_in = 0U;
	data->ep_out = 0U;

	while ((desc = usbh_desc_get_next(desc)) != NULL) {
		if (usbh_desc_is_valid_interface(desc)) {
			break;
		}

		if (!usbh_desc_is_valid_endpoint(desc)) {
			continue;
		}

		ep_desc = desc;
		if ((ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) != USB_EP_TYPE_BULK) {
			continue;
		}

		if (USB_EP_DIR_IS_IN(ep_desc->bEndpointAddress)) {
			data->ep_in = ep_desc->bEndpointAddress;
		} else {
			data->ep_out = ep_desc->bEndpointAddress;
		}
	}

	if (data->ep_in == 0U || data->ep_out == 0U) {
		LOG_ERR("Bulk endpoint pair not found");
		return -ENOTSUP;
	}

	data->udev = udev;
	data->iface = iface;
	data->tag = 0U;
	data->block_count = 0U;
	data->block_size = 0U;
	data->attached = true;

	LOG_INF("Mass storage attached, ep-in 0x%02x ep-out 0x%02x", data->ep_in, data->ep_out);

	/* Identify the medium and read its geometry outside this handler, so
	 * the disk reports itself ready without the application driving it.
	 */
	k_work_submit_to_queue(&msc_work_q, &data->start_work);

	return 0;
}

static int usbh_msc_removed(struct usbh_class_data *const c_data)
{
	struct usbh_msc_lun *const data = c_data->priv;

	data->attached = false;
	data->ready = false;

	/* The removal handler runs on the host thread and must not wait for an
	 * in-flight command to time out, so it works without the mutex.
	 *
	 * Wake a command that is waiting on the controller. Signalling with no
	 * transfer in flight would leave a count behind for the next one.
	 */
	if (data->active_xfer != NULL) {
		/* Only report the status here. The transfer is still queued in
		 * the controller, and cancelling it is asynchronous, so the
		 * waiting command has to see it through before the transfer is
		 * released.
		 */
		data->xfer_status = -ENODEV;
		k_sem_give(&data->xfer_sem);
	}

	data->udev = NULL;

	LOG_INF("Mass storage removed");

	return 0;
}

static int msc_start(struct usbh_msc_lun *const data)
{
	k_timepoint_t deadline;
	int ret;

	if (data == NULL || !data->attached) {
		return -ENODEV;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = msc_inquiry(data);
	if (ret != 0) {
		goto out;
	}

	/* A freshly attached medium reports Not Ready until it has spun up,
	 * so poll while draining the sense data the device queues up.
	 */
	ret = -EIO;

	deadline = sys_timepoint_calc(MSC_UNIT_READY_TIMEOUT);

	do {
		ret = msc_test_unit_ready(data);
		if (ret == 0) {
			break;
		}

		(void)msc_request_sense(data);
		k_sleep(MSC_UNIT_READY_INTERVAL);
	} while (!sys_timepoint_expired(deadline));

	if (ret != 0) {
		LOG_ERR("Unit did not become ready");
		goto out;
	}

	ret = msc_read_capacity(data);
	if (ret != 0) {
		goto out;
	}

	data->ready = true;

out:
	k_mutex_unlock(&data->mutex);

	return ret;
}

static struct usbh_class_api usbh_msc_class_api = {
	.init = usbh_msc_init,
	.probe = usbh_msc_probe,
	.removed = usbh_msc_removed,
};

static struct usbh_class_filter usbh_msc_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_MASS_STORAGE,
		.sub = MSC_SC_SCSI,
		.proto = MSC_PROTO_BULK_ONLY,
	},
	{0},
};

#define USBH_MSC_DEFINE(n, _)                                                                      \
	USBH_DEFINE_CLASS(usbh_msc_##n, &usbh_msc_class_api, &msc_luns[n], usbh_msc_filters);

LISTIFY(CONFIG_USBH_MSC_LUN_COUNT, USBH_MSC_DEFINE, (;), _);
