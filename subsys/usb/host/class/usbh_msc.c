/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
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

#define MSC_SC_SCSI         0x06U
#define MSC_PROTO_BULK_ONLY 0x50U

#define MSC_CBW_SIGNATURE     0x43425355UL
#define MSC_CSW_SIGNATURE     0x53425355UL
#define MSC_CBW_FLAGS_DATA_IN 0x80U

#define MSC_CSW_STATUS_PASS        0x00U
#define MSC_CSW_STATUS_FAIL        0x01U
#define MSC_CSW_STATUS_PHASE_ERROR 0x02U

#define MSC_REQ_BOT_RESET 0xFFU
#define MSC_REQ_TYPE_OUT  0x21U

#define MSC_CBW_CB_LEN SCSI_MAX_CDB_SIZE

/* Pool space left for control transfers and the command and status wrappers */
#define MSC_POOL_MARGIN 1024U

/* Largest data stage of one command, the pool is shared by all instances */
#define MSC_MAX_DATA_LEN                                                                           \
	(((size_t)CONFIG_UHC_BUF_POOL_SIZE - MSC_POOL_MARGIN) / CONFIG_USBH_MSC_DEVICE_COUNT)

#define MSC_MIN_BLOCK_SIZE 512U

BUILD_ASSERT(MSC_MAX_DATA_LEN >= CONFIG_USBH_MSC_MAX_BLOCK_SIZE,
	     "CONFIG_UHC_BUF_POOL_SIZE must hold one block per device plus 1024 bytes");

#define MSC_XFER_TIMEOUT             K_MSEC(CONFIG_USBH_MSC_XFER_TIMEOUT_MS)
#define MSC_UNIT_READY_INTERVAL      K_MSEC(CONFIG_USBH_MSC_UNIT_READY_INTERVAL_MS)
#define MSC_MEDIUM_POLL_INTERVAL     K_MSEC(CONFIG_USBH_MSC_MEDIUM_POLL_INTERVAL_MS)
#define MSC_TRANSPORT_RETRY_INTERVAL K_SECONDS(1)

/* Returned by msc_scsi_cmd() when the device reports a failed command */
#define MSC_CHECK_CONDITION 1

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
	/* Serializes commands and protects everything below the spinlock */
	struct k_mutex mutex;
	/* Protects attached and active against the completion and removal handlers */
	struct k_spinlock lock;
	struct k_sem xfer_sem;
	struct k_work_delayable poll_work;
	const struct device *uhc_dev;
	struct usb_device *udev;
	struct uhc_transfer *active;
	int xfer_status;
	uint32_t tag;
	uint32_t block_count;
	uint32_t block_size;
	uint32_t generation;
	uint8_t ep_in;
	uint8_t ep_out;
	uint8_t iface;
	uint8_t sense_key;
	uint8_t sense_asc;
	uint8_t sense_ascq;
	bool attached;
	bool identified;
	bool removable;
	bool ready;
	bool write_protected;
	bool no_sync_cache;
};

static struct usbh_msc_lun msc_luns[CONFIG_USBH_MSC_DEVICE_COUNT];

static K_THREAD_STACK_DEFINE(msc_work_stack, CONFIG_USBH_MSC_STACK_SIZE);
static struct k_work_q msc_work_q;

static int msc_xfer_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct usbh_msc_lun *const data = xfer->priv;
	k_spinlock_key_t key;

	ARG_UNUSED(udev);

	key = k_spin_lock(&data->lock);

	if (data->active == xfer) {
		data->active = NULL;
		data->xfer_status = xfer->err;
		k_spin_unlock(&data->lock, key);
		k_sem_give(&data->xfer_sem);

		return 0;
	}

	k_spin_unlock(&data->lock, key);

	/* The waiting command gave up on this transfer and left it to us */
	uhc_xfer_buf_free(data->uhc_dev, xfer->buf);
	(void)uhc_xfer_unref(xfer);

	return 0;
}

static bool msc_is_attached(struct usbh_msc_lun *const data)
{
	k_spinlock_key_t key;
	bool attached;

	key = k_spin_lock(&data->lock);
	attached = data->attached;
	k_spin_unlock(&data->lock, key);

	return attached;
}

/* Must be called with the mutex held, which keeps udev valid */
static int msc_bulk_xfer(struct usbh_msc_lun *const data, const uint8_t ep, uint8_t *const buf,
			 const size_t len, size_t *const transferred)
{
	struct uhc_transfer *xfer;
	struct net_buf *nbuf;
	k_spinlock_key_t key;
	bool abandoned = false;
	int ret;

	if (data->udev == NULL) {
		return -ENODEV;
	}

	xfer = usbh_xfer_alloc(data->udev, ep, msc_xfer_cb, data);
	if (xfer == NULL) {
		return -ENOMEM;
	}

	nbuf = usbh_xfer_buf_alloc(data->udev, len);
	if (nbuf == NULL) {
		(void)uhc_xfer_unref(xfer);
		return -ENOMEM;
	}

	if (!USB_EP_DIR_IS_IN(ep) && len > 0U) {
		net_buf_add_mem(nbuf, buf, len);
	}

	ret = usbh_xfer_buf_add(data->udev, xfer, nbuf);
	if (ret != 0) {
		goto out;
	}

	k_sem_reset(&data->xfer_sem);

	key = k_spin_lock(&data->lock);
	if (!data->attached) {
		k_spin_unlock(&data->lock, key);
		ret = -ENODEV;
		goto out;
	}

	data->active = xfer;
	k_spin_unlock(&data->lock, key);

	ret = usbh_xfer_enqueue(data->udev, xfer);
	if (ret != 0) {
		key = k_spin_lock(&data->lock);
		data->active = NULL;
		k_spin_unlock(&data->lock, key);
		goto out;
	}

	if (!msc_is_attached(data)) {
		(void)uhc_ep_dequeue(data->uhc_dev, xfer);
	}

	if (k_sem_take(&data->xfer_sem, MSC_XFER_TIMEOUT) != 0) {
		(void)uhc_ep_dequeue(data->uhc_dev, xfer);

		if (k_sem_take(&data->xfer_sem, MSC_XFER_TIMEOUT) != 0) {
			key = k_spin_lock(&data->lock);
			if (data->active == xfer) {
				data->active = NULL;
				abandoned = true;
			}
			k_spin_unlock(&data->lock, key);

			if (abandoned) {
				LOG_ERR("Transfer on ep 0x%02x not returned by the controller", ep);
				return -ETIMEDOUT;
			}

			(void)k_sem_take(&data->xfer_sem, K_NO_WAIT);
		}

		ret = msc_is_attached(data) ? -ETIMEDOUT : -ENODEV;
		goto out;
	}

	ret = data->xfer_status;
	if (ret != 0) {
		if (!msc_is_attached(data)) {
			ret = -ENODEV;
		}

		goto out;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		size_t copy = MIN(nbuf->len, len);

		if (copy > 0U) {
			memcpy(buf, nbuf->data, copy);
		}

		if (transferred != NULL) {
			*transferred = copy;
		}
	} else if (transferred != NULL) {
		*transferred = len;
	}

out:
	uhc_xfer_buf_free(data->uhc_dev, nbuf);
	(void)uhc_xfer_unref(xfer);

	return ret;
}

static int msc_clear_halt(struct usbh_msc_lun *const data, const uint8_t ep)
{
	if (!msc_is_attached(data)) {
		return -ENODEV;
	}

	return usbh_req_clear_sfs_halt(data->udev, ep);
}

/* Bulk-Only Transport reset recovery */
static int msc_bot_reset(struct usbh_msc_lun *const data)
{
	int ret;

	if (!msc_is_attached(data)) {
		return -ENODEV;
	}

	LOG_WRN("Resetting the bulk-only transport");

	ret = usbh_req_setup(data->udev, MSC_REQ_TYPE_OUT, MSC_REQ_BOT_RESET, 0U, data->iface, 0U,
			     NULL);
	if (ret != 0) {
		LOG_ERR("Bulk-only reset failed: %d", ret);
		return ret;
	}

	ret = msc_clear_halt(data, data->ep_in);
	if (ret != 0) {
		LOG_ERR("Failed to clear the IN halt: %d", ret);
		return ret;
	}

	ret = msc_clear_halt(data, data->ep_out);
	if (ret != 0) {
		LOG_ERR("Failed to clear the OUT halt: %d", ret);
	}

	return ret;
}

static int msc_transport_error(struct usbh_msc_lun *const data, const int err)
{
	if (err != -ENODEV) {
		(void)msc_bot_reset(data);
	}

	return err;
}

static int msc_scsi_cmd(struct usbh_msc_lun *const data, const uint8_t *const cb,
			const uint8_t cb_len, uint8_t *const buf, const size_t len,
			const bool dir_in, const bool allow_short)
{
	const uint8_t data_ep = dir_in ? data->ep_in : data->ep_out;
	struct msc_cbw cbw;
	struct msc_csw csw;
	size_t transferred = 0U;
	size_t data_len = 0U;
	uint32_t residue;
	uint32_t tag;
	int ret;

	if (len > MSC_MAX_DATA_LEN) {
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
		return msc_transport_error(data, ret);
	}

	if (len > 0U) {
		ret = msc_bulk_xfer(data, data_ep, buf, len, &data_len);
		if (ret == -EPIPE) {
			data_len = 0U;
			ret = msc_clear_halt(data, data_ep);
		}

		if (ret != 0) {
			LOG_ERR("Data stage failed: %d", ret);
			return msc_transport_error(data, ret);
		}
	}

	ret = msc_bulk_xfer(data, data->ep_in, (uint8_t *)&csw, sizeof(csw), &transferred);
	if (ret == -EPIPE) {
		ret = msc_clear_halt(data, data->ep_in);
		if (ret == 0) {
			ret = msc_bulk_xfer(data, data->ep_in, (uint8_t *)&csw, sizeof(csw),
					    &transferred);
		}
	}

	if (ret != 0) {
		LOG_ERR("CSW transfer failed: %d", ret);
		return msc_transport_error(data, ret);
	}

	residue = sys_le32_to_cpu(csw.dCSWDataResidue);

	if (transferred != sizeof(csw) || sys_le32_to_cpu(csw.dCSWSignature) != MSC_CSW_SIGNATURE ||
	    sys_le32_to_cpu(csw.dCSWTag) != tag || csw.bCSWStatus > MSC_CSW_STATUS_PHASE_ERROR ||
	    residue > len) {
		LOG_ERR("Invalid CSW");
		return msc_transport_error(data, -EIO);
	}

	if (csw.bCSWStatus == MSC_CSW_STATUS_PHASE_ERROR) {
		LOG_ERR("Phase error");
		return msc_transport_error(data, -EIO);
	}

	if (csw.bCSWStatus == MSC_CSW_STATUS_FAIL) {
		return MSC_CHECK_CONDITION;
	}

	if (!allow_short && (data_len != len || residue != 0U)) {
		LOG_ERR("Short data stage, %zu of %zu, residue %u", data_len, len, residue);
		return -EIO;
	}

	return 0;
}

static int msc_request_sense(struct usbh_msc_lun *const data)
{
	uint8_t cb[SCSI_REQUEST_SENSE_CDB_LEN] = {SCSI_REQUEST_SENSE};
	uint8_t sense[SCSI_REQUEST_SENSE_RESP_LEN] = {0};
	uint8_t code;
	int ret;

	cb[4] = sizeof(sense);

	data->sense_key = 0U;
	data->sense_asc = 0U;
	data->sense_ascq = 0U;

	ret = msc_scsi_cmd(data, cb, sizeof(cb), sense, sizeof(sense), true, true);
	if (ret != 0) {
		return (ret == MSC_CHECK_CONDITION) ? -EIO : ret;
	}

	code = sense[0] & SCSI_SENSE_RESP_CODE_MASK;
	if (code != SCSI_SENSE_RESP_CURRENT && code != SCSI_SENSE_RESP_DEFERRED) {
		return -EIO;
	}

	data->sense_key = sense[2] & SCSI_SENSE_KEY_MASK;
	data->sense_asc = sense[12];
	data->sense_ascq = sense[13];

	return 0;
}

static void msc_medium_lost(struct usbh_msc_lun *const data)
{
	if (!data->ready) {
		return;
	}

	data->ready = false;
	LOG_INF("Medium removed or changed");

	if (msc_is_attached(data)) {
		(void)k_work_reschedule_for_queue(&msc_work_q, &data->poll_work, K_NO_WAIT);
	}
}

/* Run a command, and on CHECK CONDITION read the sense data and map it to an error */
static int msc_command(struct usbh_msc_lun *const data, const uint8_t *const cb,
		       const uint8_t cb_len, uint8_t *const buf, const size_t len,
		       const bool dir_in, const bool allow_short)
{
	int ret;

	ret = msc_scsi_cmd(data, cb, cb_len, buf, len, dir_in, allow_short);
	if (ret != MSC_CHECK_CONDITION) {
		return ret;
	}

	ret = msc_request_sense(data);
	if (ret != 0) {
		return ret;
	}

	LOG_DBG("Command 0x%02x failed, sense %02x/%02x/%02x", cb[0], data->sense_key,
		data->sense_asc, data->sense_ascq);

	switch (data->sense_key) {
	case SCSI_SENSE_UNIT_ATTENTION:
		if (data->sense_asc == SCSI_ASC_MEDIUM_MAY_CHANGED) {
			msc_medium_lost(data);
			return -ENODEV;
		}

		return -EAGAIN;
	case SCSI_SENSE_NOT_READY:
		if (data->sense_asc == SCSI_ASC_MEDIUM_NOT_PRESENT) {
			msc_medium_lost(data);
			return -ENODEV;
		}

		return -EBUSY;
	case SCSI_SENSE_DATA_PROTECT:
		return -EROFS;
	case SCSI_SENSE_ILLEGAL_REQUEST:
		return (data->sense_asc == SCSI_ASC_INVALID_OPCODE) ? -ENOTSUP : -EINVAL;
	default:
		return -EIO;
	}
}

static int msc_test_unit_ready(struct usbh_msc_lun *const data)
{
	uint8_t cb[SCSI_TST_U_RDY_CDB_LEN] = {SCSI_TST_U_RDY};

	return msc_command(data, cb, sizeof(cb), NULL, 0U, false, false);
}

static int msc_start_unit(struct usbh_msc_lun *const data)
{
	uint8_t cb[SCSI_START_STOP_UNIT_CDB_LEN] = {SCSI_START_STOP_UNIT};

	cb[4] = SCSI_START_STOP_START;

	return msc_command(data, cb, sizeof(cb), NULL, 0U, false, false);
}

static int msc_inquiry(struct usbh_msc_lun *const data)
{
	uint8_t cb[SCSI_INQUIRY_CDB_LEN] = {SCSI_INQUIRY};
	uint8_t resp[SCSI_INQUIRY_RESP_LEN] = {0};
	char vendor[9];
	char product[17];
	int ret;

	cb[4] = sizeof(resp);

	ret = msc_command(data, cb, sizeof(cb), resp, sizeof(resp), true, true);
	if (ret != 0) {
		return ret;
	}

	if ((resp[0] & SCSI_INQUIRY_PDT_MASK) != SCSI_INQUIRY_PDT_DIRECT) {
		LOG_ERR("Unsupported peripheral device type 0x%02x",
			resp[0] & SCSI_INQUIRY_PDT_MASK);
		return -ENOTSUP;
	}

	data->removable = (resp[1] & SCSI_INQUIRY_RMB) != 0U;

	memcpy(vendor, &resp[8], 8);
	vendor[8] = '\0';
	memcpy(product, &resp[16], 16);
	product[16] = '\0';

	LOG_INF("Vendor '%s' Product '%s'%s", vendor, product,
		data->removable ? " (removable)" : "");

	return 0;
}

static int msc_read_capacity(struct usbh_msc_lun *const data)
{
	uint8_t cb[SCSI_READ_CAPACITY_CDB_LEN] = {SCSI_READ_CAPACITY};
	uint8_t resp[SCSI_READ_CAPACITY_RESP_LEN] = {0};
	uint32_t last_lba;
	uint32_t block_size;
	int ret;

	ret = msc_command(data, cb, sizeof(cb), resp, sizeof(resp), true, false);
	if (ret != 0) {
		return ret;
	}

	last_lba = sys_get_be32(&resp[0]);
	block_size = sys_get_be32(&resp[4]);

	if (last_lba == UINT32_MAX) {
		LOG_ERR("Medium too large for 32-bit block addresses");
		return -ENOTSUP;
	}

	if (block_size < MSC_MIN_BLOCK_SIZE || block_size > CONFIG_USBH_MSC_MAX_BLOCK_SIZE ||
	    !IS_POWER_OF_TWO(block_size)) {
		LOG_ERR("Unsupported block size %u", block_size);
		return -ENOTSUP;
	}

	data->block_count = last_lba + 1U;
	data->block_size = block_size;

	return 0;
}

static void msc_read_write_protect(struct usbh_msc_lun *const data)
{
	uint8_t cb[SCSI_MODE_SENSE6_CDB_LEN] = {SCSI_MODE_SENSE6};
	uint8_t resp[SCSI_MODE_SENSE6_HDR_LEN] = {0};

	cb[2] = SCSI_MODE_SENSE_ALL_PAGES;
	cb[4] = sizeof(resp);

	data->write_protected = false;

	if (msc_command(data, cb, sizeof(cb), resp, sizeof(resp), true, true) == 0) {
		data->write_protected = (resp[2] & SCSI_MODE_SENSE_WP) != 0U;
	}
}

static int msc_medium_setup(struct usbh_msc_lun *const data)
{
	int ret;

	ret = msc_read_capacity(data);
	if (ret != 0) {
		return ret;
	}

	msc_read_write_protect(data);

	data->generation++;
	if (data->generation == 0U) {
		data->generation = 1U;
	}

	data->ready = true;

	LOG_INF("Medium ready, %u blocks of %u bytes%s", data->block_count, data->block_size,
		data->write_protected ? ", write protected" : "");

	return 0;
}

static void msc_poll_work(struct k_work *work)
{
	struct k_work_delayable *const dwork = k_work_delayable_from_work(work);
	struct usbh_msc_lun *const data = CONTAINER_OF(dwork, struct usbh_msc_lun, poll_work);
	k_timeout_t next = MSC_UNIT_READY_INTERVAL;
	int ret;

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (!msc_is_attached(data)) {
		goto out;
	}

	if (!data->identified) {
		ret = msc_inquiry(data);
		if (ret == -ENOTSUP) {
			goto out;
		}

		if (ret != 0) {
			goto reschedule;
		}

		data->identified = true;
	}

	ret = msc_test_unit_ready(data);

	if (data->ready) {
		if (!data->removable || CONFIG_USBH_MSC_MEDIUM_POLL_INTERVAL_MS == 0) {
			goto out;
		}

		next = MSC_MEDIUM_POLL_INTERVAL;
		goto reschedule;
	}

	if (ret == 0) {
		ret = msc_medium_setup(data);
		if (ret == -ENOTSUP) {
			goto out;
		}

		if (ret == 0) {
			if (!data->removable || CONFIG_USBH_MSC_MEDIUM_POLL_INTERVAL_MS == 0) {
				goto out;
			}

			next = MSC_MEDIUM_POLL_INTERVAL;
		}
	} else if (ret == -EBUSY && data->sense_asc == SCSI_ASC_LUN_NOT_READY &&
		   data->sense_ascq == SCSI_ASCQ_INIT_CMD_REQUIRED) {
		(void)msc_start_unit(data);
	}

reschedule:
	if (ret == -EIO || ret == -ETIMEDOUT || ret == -EPIPE || ret == -ENOMEM) {
		next = MSC_TRANSPORT_RETRY_INTERVAL;
	} else if (ret == -ENODEV && data->sense_asc == SCSI_ASC_MEDIUM_NOT_PRESENT &&
		   CONFIG_USBH_MSC_MEDIUM_POLL_INTERVAL_MS != 0) {
		next = MSC_MEDIUM_POLL_INTERVAL;
	}

	if (msc_is_attached(data)) {
		(void)k_work_reschedule_for_queue(&msc_work_q, dwork, next);
	}

out:
	k_mutex_unlock(&data->mutex);
}

/* Must be called with the mutex held */
static int msc_check_access(const struct usbh_msc_lun *const data, const uint32_t lba,
			    const uint32_t count, const bool write)
{
	if (!data->ready) {
		return -ENODEV;
	}

	if (count == 0U || count > data->block_count || lba > data->block_count - count) {
		return -EINVAL;
	}

	if (write && data->write_protected) {
		return -EROFS;
	}

	return 0;
}

static int msc_rw_blocks(struct usbh_msc_lun *const data, const uint8_t opcode, const uint32_t lba,
			 const uint32_t count, uint8_t *const buf)
{
	const bool dir_in = (opcode == SCSI_READ10);
	uint32_t max_blocks;
	uint32_t done = 0U;
	int ret;

	if (data == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = msc_check_access(data, lba, count, !dir_in);
	if (ret != 0) {
		goto out;
	}

	max_blocks = MIN(MSC_MAX_DATA_LEN / data->block_size, UINT16_MAX);

	while (done < count) {
		const uint32_t chunk = MIN(count - done, max_blocks);
		uint8_t *const chunk_buf = buf + (size_t)done * data->block_size;
		uint8_t cb[SCSI_RW10_CDB_LEN] = {opcode};

		sys_put_be32(lba + done, &cb[2]);
		sys_put_be16((uint16_t)chunk, &cb[7]);

		ret = msc_command(data, cb, sizeof(cb), chunk_buf, (size_t)chunk * data->block_size,
				  dir_in, false);
		if (ret == -EAGAIN) {
			ret = msc_command(data, cb, sizeof(cb), chunk_buf,
					  (size_t)chunk * data->block_size, dir_in, false);
		}

		if (ret != 0) {
			if (ret == -EAGAIN || ret == -EBUSY) {
				ret = -EIO;
			}

			goto out;
		}

		done += chunk;
	}

out:
	k_mutex_unlock(&data->mutex);

	return ret;
}

int usbh_msc_read(struct usbh_msc_lun *const lun, const uint32_t lba, const uint32_t count,
		  uint8_t *const buf)
{
	return msc_rw_blocks(lun, SCSI_READ10, lba, count, buf);
}

int usbh_msc_write(struct usbh_msc_lun *const lun, const uint32_t lba, const uint32_t count,
		   const uint8_t *const buf)
{
	return msc_rw_blocks(lun, SCSI_WRITE10, lba, count, (uint8_t *)buf);
}

int usbh_msc_sync(struct usbh_msc_lun *const lun)
{
	uint8_t cb[SCSI_SYNC_CACHE10_CDB_LEN] = {SCSI_SYNC_CACHE10};
	int ret = 0;

	if (lun == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&lun->mutex, K_FOREVER);

	if (!lun->ready) {
		ret = -ENODEV;
		goto out;
	}

	if (lun->no_sync_cache || lun->write_protected) {
		goto out;
	}

	ret = msc_command(lun, cb, sizeof(cb), NULL, 0U, false, false);
	if (ret == -EAGAIN) {
		ret = msc_command(lun, cb, sizeof(cb), NULL, 0U, false, false);
	}

	if (ret == -ENOTSUP || ret == -EINVAL) {
		LOG_DBG("SYNCHRONIZE CACHE not supported");
		lun->no_sync_cache = true;
		ret = 0;
	} else if (ret == -EAGAIN || ret == -EBUSY) {
		ret = -EIO;
	}

out:
	k_mutex_unlock(&lun->mutex);

	return ret;
}

int usbh_msc_get_capacity(struct usbh_msc_lun *const lun, uint32_t *const block_count,
			  uint32_t *const block_size)
{
	int ret = 0;

	if (lun == NULL) {
		return -ENODEV;
	}

	if (block_count == NULL || block_size == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&lun->mutex, K_FOREVER);

	if (lun->ready) {
		*block_count = lun->block_count;
		*block_size = lun->block_size;
	} else {
		ret = -ENODEV;
	}

	k_mutex_unlock(&lun->mutex);

	return ret;
}

bool usbh_msc_is_ready(struct usbh_msc_lun *const lun)
{
	return (lun != NULL) && lun->ready;
}

bool usbh_msc_is_write_protected(struct usbh_msc_lun *const lun)
{
	return (lun != NULL) && lun->ready && lun->write_protected;
}

uint32_t usbh_msc_get_generation(struct usbh_msc_lun *const lun)
{
	if (lun == NULL || !lun->ready) {
		return 0U;
	}

	return lun->generation;
}

struct usbh_msc_lun *usbh_msc_lun_get(const unsigned int idx)
{
	if (idx >= ARRAY_SIZE(msc_luns) || !msc_is_attached(&msc_luns[idx])) {
		return NULL;
	}

	return &msc_luns[idx];
}

static int usbh_msc_init(struct usbh_class_data *const c_data)
{
	struct usbh_msc_lun *const data = c_data->priv;
	static bool work_q_started;

	k_mutex_init(&data->mutex);
	k_sem_init(&data->xfer_sem, 0, 1);
	k_work_init_delayable(&data->poll_work, msc_poll_work);

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
	k_spinlock_key_t key;
	uint8_t ep_in = 0U;
	uint8_t ep_out = 0U;
	const void *desc;

	if (msc_is_attached(data)) {
		return -ENOTSUP;
	}

	desc = usbh_desc_get_iface(udev, iface);
	if (desc == NULL) {
		return -ENOTSUP;
	}

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
			ep_in = ep_desc->bEndpointAddress;
		} else {
			ep_out = ep_desc->bEndpointAddress;
		}
	}

	if (ep_in == 0U || ep_out == 0U) {
		LOG_ERR("Bulk endpoint pair not found");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	data->uhc_dev = ((struct usbh_context *)udev->ctx)->dev;
	data->udev = udev;
	data->ep_in = ep_in;
	data->ep_out = ep_out;
	data->iface = iface;
	data->tag = 0U;
	data->block_count = 0U;
	data->block_size = 0U;
	data->identified = false;
	data->removable = false;
	data->ready = false;
	data->write_protected = false;
	data->no_sync_cache = false;

	key = k_spin_lock(&data->lock);
	data->active = NULL;
	data->attached = true;
	k_spin_unlock(&data->lock, key);

	k_mutex_unlock(&data->mutex);

	LOG_INF("Mass storage attached, ep-in 0x%02x ep-out 0x%02x", ep_in, ep_out);

	(void)k_work_reschedule_for_queue(&msc_work_q, &data->poll_work, K_NO_WAIT);

	return 0;
}

static int usbh_msc_removed(struct usbh_class_data *const c_data)
{
	struct usbh_msc_lun *const data = c_data->priv;
	struct uhc_transfer *xfer;
	struct k_work_sync sync;
	k_spinlock_key_t key;

	data->ready = false;

	key = k_spin_lock(&data->lock);
	data->attached = false;
	xfer = data->active;
	if (xfer != NULL) {
		uhc_xfer_ref(xfer);
	}
	k_spin_unlock(&data->lock, key);

	if (xfer != NULL) {
		(void)uhc_ep_dequeue(data->uhc_dev, xfer);
		(void)uhc_xfer_unref(xfer);
	}

	(void)k_work_cancel_delayable_sync(&data->poll_work, &sync);

	/* Wait for a command in progress to stop using the device */
	k_mutex_lock(&data->mutex, K_FOREVER);
	data->ready = false;
	data->udev = NULL;
	k_mutex_unlock(&data->mutex);

	LOG_INF("Mass storage removed");

	return 0;
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

LISTIFY(CONFIG_USBH_MSC_DEVICE_COUNT, USBH_MSC_DEFINE, (;), _);
