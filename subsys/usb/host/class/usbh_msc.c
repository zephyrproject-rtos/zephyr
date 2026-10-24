/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_msc_host

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/logging/log.h>

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_device.h"
#include "usbh_msc.h"
#include "usbh_msc_ufi.h"

LOG_MODULE_REGISTER(usbh_msc, CONFIG_USBH_MSC_CLASS_LOG_LEVEL);

struct usbh_msc_data {
	struct usb_device *udev;
	struct disk_info disk_info;
	struct k_mutex lock;

	const struct usb_if_descriptor *ifaces[CONFIG_USBH_MSC_MAX_INTERFACE];
	uint8_t num_ifaces;
	uint8_t current_iface_idx;

	const struct usb_ep_descriptor *bulk_in_ep_desc;
	const struct usb_ep_descriptor *bulk_out_ep_desc;

	struct cbw cbw;
	struct csw csw;

	enum msc_device_state state;
	bool initialized;
	bool connected;

	uint32_t tag_counter;

	uint32_t sector_count;
	uint32_t sector_size;

	char vendor_id[9];
	char product_id[17];
	char product_rev[5];
	char serial_number[17];
	uint8_t device_type;
	uint8_t removable;

	struct k_sem xfer_sem;
	struct net_buf *xfer_buf;
	int xfer_status;
	bool last_xfer_stall;

	uint8_t cmd_buffer[512];
	uint8_t data_buffer[4096];
	struct {
		uint32_t read_count;
		uint32_t write_count;
		uint32_t error_count;
		uint32_t retry_count;
	} stats;
};

static int msc_disk_init(struct disk_info *disk);
static int msc_disk_status(struct disk_info *disk);
static int msc_disk_read(struct disk_info *disk, uint8_t *data_buf,
			 uint32_t start_sector, uint32_t num_sectors);
static int msc_disk_write(struct disk_info *disk, const uint8_t *data_buf,
			  uint32_t start_sector, uint32_t num_sectors);
static int msc_disk_ioctl(struct disk_info *disk, uint8_t cmd, void *buff);

static const struct disk_operations msc_disk_ops = {
	.init = msc_disk_init,
	.status = msc_disk_status,
	.read = msc_disk_read,
	.write = msc_disk_write,
	.ioctl = msc_disk_ioctl,
};

static void msc_handle_optional_command_stall(struct usbh_msc_data *msc,
					      const char *cmd_name)
{
	int ret;

	LOG_WRN("%s: Device returned STALL (may not support this command)", cmd_name);

	ret = usbh_req_clear_sfs_halt(msc->udev, msc->bulk_in_ep_desc->bEndpointAddress);
	if (ret != 0) {
		LOG_WRN("Failed to clear Bulk-IN HALT (continuing)");
	}

	k_sleep(K_MSEC(50));

	ret = usbh_req_clear_sfs_halt(msc->udev, msc->bulk_out_ep_desc->bEndpointAddress);
	if (ret != 0) {
		LOG_WRN("Failed to clear Bulk-OUT HALT (continuing)");
	}

	k_sleep(K_MSEC(100));

	LOG_INF("%s: HALT cleared, continuing with next command", cmd_name);
}

static void msc_state_change(struct usbh_msc_data *msc, enum msc_device_state new_state)
{
	if (msc->state != new_state) {
		LOG_DBG("MSC[%s] state: %d -> %d", msc->disk_info.name, msc->state, new_state);
		msc->state = new_state;
	}
}

static int msc_bulk_only_reset(struct usbh_msc_data *msc)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE |
				      USB_REQTYPE_TYPE_CLASS |
				      USB_REQTYPE_RECIPIENT_INTERFACE;
	const uint8_t bRequest = 0xFF;
	const uint16_t wValue = 0;
	const uint16_t wIndex = 0;
	const uint16_t wLength = 0;
	int ret;

	LOG_WRN("Performing Bulk-Only Mass Storage Reset");

	ret = usbh_req_setup(msc->udev, bmRequestType, bRequest,
			     wValue, wIndex, wLength, NULL);
	if (ret != 0) {
		LOG_ERR("MSC reset failed: %d", ret);
		return ret;
	}

	k_sleep(K_MSEC(100));

	return 0;
}

static int msc_reset_recovery(struct usbh_msc_data *msc)
{
	int ret;

	LOG_WRN("Starting Mass Storage Reset Recovery");

	ret = msc_bulk_only_reset(msc);
	if (ret != 0) {
		LOG_ERR("Bulk-Only Reset failed: %d", ret);
		return ret;
	}

	k_sleep(K_MSEC(100));

	ret = usbh_req_clear_sfs_halt(msc->udev, msc->bulk_in_ep_desc->bEndpointAddress);
	if (ret != 0) {
		LOG_WRN("Failed to clear Bulk-IN halt: %d (continuing)", ret);
	}

	k_sleep(K_MSEC(50));

	ret = usbh_req_clear_sfs_halt(msc->udev, msc->bulk_out_ep_desc->bEndpointAddress);
	if (ret != 0) {
		LOG_WRN("Failed to clear Bulk-OUT halt: %d (continuing)", ret);
	}

	k_sleep(K_MSEC(100));

	LOG_INF("Mass Storage Reset Recovery complete");

	return 0;
}

static int msc_xfer_cb(struct usb_device *const udev,
			struct uhc_transfer *const xfer)
{
	struct usbh_msc_data *msc = (struct usbh_msc_data *)xfer->priv;

	if (msc == NULL) {
		return -EINVAL;
	}

	if (xfer->err == -EPIPE) {
		LOG_WRN("Endpoint STALL detected on 0x%02x", xfer->ep);
		msc->last_xfer_stall = true;
	}

	msc->xfer_buf = xfer->buf;
	k_sem_give(&msc->xfer_sem);

	return 0;
}

int msc_bot_command(struct usbh_msc_data *msc,
		    const uint8_t *cmd, uint8_t cmd_len,
		    uint8_t *data, uint32_t data_len,
		    bool data_in)
{
	struct uhc_transfer *xfer = NULL;
	struct net_buf *buf = NULL;
	uint32_t csw_sig;
	uint32_t csw_tag;
	bool data_stall_occurred = false;
	bool need_recovery = false;
	uint8_t csw_status;
	size_t copy_len;
	uint8_t ep;
	int halt_ret;
	int ret;

	if (msc == NULL || msc->udev == NULL || cmd == NULL) {
		return -EINVAL;
	}

	if (!msc->connected) {
		return -ENODEV;
	}

	memset(&msc->cbw, 0, sizeof(msc->cbw));
	msc->cbw.dCBWSignature = sys_cpu_to_le32(CBW_SIGNATURE);
	msc->cbw.dCBWTag = sys_cpu_to_le32(++msc->tag_counter);
	msc->cbw.dCBWDataTransferLength = sys_cpu_to_le32(data_len);
	msc->cbw.bmCBWFlags = data_in ? CBW_FLAGS_DATA_IN : CBW_FLAGS_DATA_OUT;
	msc->cbw.bCBWLUN = 0;
	msc->cbw.bCBWCBLength = cmd_len;
	memcpy(msc->cbw.CBWCB, cmd, cmd_len);

	k_sem_reset(&msc->xfer_sem);
	msc->xfer_buf = NULL;

	xfer = usbh_xfer_alloc(msc->udev, msc->bulk_out_ep_desc->bEndpointAddress,
			       msc_xfer_cb, msc);
	if (xfer == NULL) {
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(msc->udev, sizeof(msc->cbw));
	if (buf == NULL) {
		usbh_xfer_free(msc->udev, xfer);
		return -ENOMEM;
	}

	memcpy(buf->data, &msc->cbw, sizeof(msc->cbw));
	net_buf_add(buf, sizeof(msc->cbw));
	xfer->buf = buf;

	ret = usbh_xfer_enqueue(msc->udev, xfer);
	if (ret != 0) {
		net_buf_unref(buf);
		usbh_xfer_free(msc->udev, xfer);
		return ret;
	}

	ret = k_sem_take(&msc->xfer_sem, K_MSEC(1000));
	if (ret != 0) {
		LOG_ERR("CBW transfer timeout");
		need_recovery = true;
		ret = -ETIMEDOUT;
	}

	if (msc->xfer_buf != NULL) {
		net_buf_unref(msc->xfer_buf);
		msc->xfer_buf = NULL;
	}
	usbh_xfer_free(msc->udev, xfer);

	if (ret != 0) {
		goto error_recovery;
	}

	/* Data phase */
	if (data != NULL && data_len > 0) {
		ep = data_in ? msc->bulk_in_ep_desc->bEndpointAddress :
			       msc->bulk_out_ep_desc->bEndpointAddress;

		k_sem_reset(&msc->xfer_sem);
		msc->xfer_buf = NULL;
		msc->last_xfer_stall = false;

		xfer = usbh_xfer_alloc(msc->udev, ep, msc_xfer_cb, msc);
		if (xfer == NULL) {
			return -ENOMEM;
		}

		buf = usbh_xfer_buf_alloc(msc->udev, data_len);
		if (buf == NULL) {
			usbh_xfer_free(msc->udev, xfer);
			return -ENOMEM;
		}

		if (!data_in) {
			memcpy(buf->data, data, data_len);
			net_buf_add(buf, data_len);
		}

		xfer->buf = buf;

		ret = usbh_xfer_enqueue(msc->udev, xfer);
		if (ret != 0) {
			net_buf_unref(buf);
			usbh_xfer_free(msc->udev, xfer);
			return ret;
		}

		ret = k_sem_take(&msc->xfer_sem, K_MSEC(5000));

		if (msc->last_xfer_stall) {
			LOG_WRN("Data phase STALL detected on endpoint 0x%02x", ep);
			data_stall_occurred = true;

			if (msc->xfer_buf != NULL) {
				net_buf_unref(msc->xfer_buf);
				msc->xfer_buf = NULL;
			}
			usbh_xfer_free(msc->udev, xfer);

			halt_ret = usbh_req_clear_sfs_halt(msc->udev, ep);
			if (halt_ret != 0) {
				LOG_ERR("Failed to clear HALT on ep 0x%02x: %d",
					ep, halt_ret);
				need_recovery = true;
				goto error_recovery;
			}

			k_sleep(K_MSEC(100));

			LOG_DBG("HALT cleared on endpoint 0x%02x, proceeding to CSW", ep);
			/* Some non-conformant devices still send CSW after a data STALL. */
			goto csw_phase;
		}

		if (ret != 0) {
			LOG_ERR("Data transfer timeout on endpoint 0x%02x", ep);

			if (msc->xfer_buf != NULL) {
				net_buf_unref(msc->xfer_buf);
				msc->xfer_buf = NULL;
			}
			usbh_xfer_free(msc->udev, xfer);

			need_recovery = true;
			ret = -ETIMEDOUT;
			goto error_recovery;
		}

		if (data_in && msc->xfer_buf != NULL) {
			copy_len = MIN(data_len, msc->xfer_buf->len);
			memcpy(data, msc->xfer_buf->data, copy_len);
			LOG_DBG("Data phase: received %u bytes", copy_len);
		}

		if (msc->xfer_buf != NULL) {
			net_buf_unref(msc->xfer_buf);
			msc->xfer_buf = NULL;
		}
		usbh_xfer_free(msc->udev, xfer);
	}

	/* CSW phase */
csw_phase:
	k_sem_reset(&msc->xfer_sem);
	msc->xfer_buf = NULL;
	msc->last_xfer_stall = false;

	xfer = usbh_xfer_alloc(msc->udev,
			       msc->bulk_in_ep_desc->bEndpointAddress,
			       msc_xfer_cb, msc);
	if (xfer == NULL) {
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(msc->udev, sizeof(msc->csw));
	if (buf == NULL) {
		usbh_xfer_free(msc->udev, xfer);
		return -ENOMEM;
	}

	xfer->buf = buf;

	ret = usbh_xfer_enqueue(msc->udev, xfer);
	if (ret != 0) {
		net_buf_unref(buf);
		usbh_xfer_free(msc->udev, xfer);
		return ret;
	}

	ret = k_sem_take(&msc->xfer_sem, K_MSEC(1000));
	if (ret != 0) {
		LOG_ERR("CSW transfer timeout");
		if (data_stall_occurred) {
			LOG_WRN("CSW timeout after data phase STALL"
				" - device may not support this command");
		}
		need_recovery = true;
		ret = -ETIMEDOUT;
		goto csw_cleanup;
	}

	if (msc->xfer_buf == NULL) {
		LOG_ERR("No CSW buffer received");
		need_recovery = true;
		ret = -EIO;
		goto csw_cleanup;
	}

	if (msc->xfer_buf->len < sizeof(msc->csw)) {
		LOG_WRN("CSW buffer too small: %u bytes (expected %u)",
			msc->xfer_buf->len, sizeof(msc->csw));
		net_buf_unref(msc->xfer_buf);
		msc->xfer_buf = NULL;
		usbh_xfer_free(msc->udev, xfer);
		need_recovery = true;
		ret = -EIO;
		goto error_recovery;
	}

	memcpy(&msc->csw, msc->xfer_buf->data, sizeof(msc->csw));

	csw_sig = sys_le32_to_cpu(msc->csw.dCSWSignature);
	csw_tag = sys_le32_to_cpu(msc->csw.dCSWTag);
	csw_status = msc->csw.bCSWStatus;

	LOG_DBG("CSW received: signature=0x%08X, tag=%u, status=0x%02x",
		csw_sig, csw_tag, csw_status);

	if (csw_sig != CSW_SIGNATURE) {
		LOG_ERR("CSW signature mismatch: 0x%08X (expected 0x%08X)",
			csw_sig, CSW_SIGNATURE);
		if (data_stall_occurred) {
			LOG_WRN("Signature mismatch may be due to data phase STALL");
		}
		need_recovery = true;
		ret = -EIO;
		goto csw_cleanup;
	}

	if (csw_tag != msc->tag_counter) {
		LOG_ERR("CSW tag mismatch: got %u, expected %u", csw_tag, msc->tag_counter);
		need_recovery = true;
		ret = -EIO;
		goto csw_cleanup;
	}

	if (csw_status == CSW_STATUS_PHASE_ERROR) {
		LOG_WRN("CSW Phase Error detected");
		need_recovery = true;
		ret = -EIO;
		goto csw_cleanup;
	}

	if (csw_status != CSW_STATUS_PASSED) {
		LOG_DBG("CSW status: 0x%02x", csw_status);
		if (data_stall_occurred) {
			LOG_DBG("Data phase STALL occurred,"
				" CSW status may indicate command not supported");
			ret = -EPIPE;
		} else {
			ret = -EIO;
		}
		goto csw_cleanup;
	}

	LOG_DBG("Command completed successfully (tag=%u, status=PASSED)", csw_tag);
	ret = 0;

csw_cleanup:
	if (msc->xfer_buf != NULL) {
		net_buf_unref(msc->xfer_buf);
		msc->xfer_buf = NULL;
	}
	usbh_xfer_free(msc->udev, xfer);

error_recovery:
	if (need_recovery) {
		LOG_WRN("Command failed, performing reset recovery");
		msc_reset_recovery(msc);
	}

	return ret;
}

static void parse_inquiry_data(const uint8_t *buffer, struct usbh_msc_data *msc)
{
	const struct scsi_inquiry_data *inq = (const struct scsi_inquiry_data *)buffer;

	memcpy(msc->vendor_id, inq->vendor_id, 8);
	msc->vendor_id[8] = '\0';
	for (int i = 7; i >= 0 && msc->vendor_id[i] == ' '; i--) {
		msc->vendor_id[i] = '\0';
	}

	memcpy(msc->product_id, inq->product_id, 16);
	msc->product_id[16] = '\0';
	for (int i = 15; i >= 0 && msc->product_id[i] == ' '; i--) {
		msc->product_id[i] = '\0';
	}

	memcpy(msc->product_rev, inq->product_rev, 4);
	msc->product_rev[4] = '\0';
	for (int i = 3; i >= 0 && msc->product_rev[i] == ' '; i--) {
		msc->product_rev[i] = '\0';
	}

	msc->device_type = inq->peripheral_device_type;
	msc->removable = inq->removable;

	LOG_INF("Device Info:");
	LOG_INF("  Vendor ID:    %s", msc->vendor_id);
	LOG_INF("  Product ID:   %s", msc->product_id);
	LOG_INF("  Product Rev:  %s", msc->product_rev);
	LOG_INF("  Device Type:  0x%02x %s", msc->device_type,
		msc->device_type == 0x00 ? "(Direct Access Storage)" : "");
	LOG_INF("  Removable:    %s", msc->removable ? "Yes" : "No");
}

static int msc_device_init(struct usbh_msc_data *msc)
{
	uint8_t *buffer = msc->data_buffer;
	uint32_t block_length;
	uint32_t last_lba;
	int ret;

	LOG_DBG("=== Starting MSC Device Enumeration ===");
	LOG_DBG("Initializing MSC device [%s]", msc->disk_info.name);
	msc_state_change(msc, MSC_STATE_INITIALIZING);

	LOG_DBG("Step 1: TEST UNIT READY");
	ret = usbh_msc_test_unit_ready(msc);
	if (ret != 0) {
		if (ret == -EPIPE) {
			msc_handle_optional_command_stall(msc, "TEST_UNIT_READY");
		} else {
			LOG_WRN("TEST UNIT READY not supported: %d (continuing)", ret);
		}
	}

	LOG_INF("Step 2: INQUIRY - Get device basic information");
	memset(buffer, 0, sizeof(msc->data_buffer));
	ret = usbh_msc_inquiry(msc, buffer, 36);
	if (ret != 0) {
		if (ret == -EPIPE) {
			LOG_ERR("INQUIRY returned STALL, device may be broken");
			msc_reset_recovery(msc);
		}
		LOG_ERR("INQUIRY failed: %d", ret);
		msc_state_change(msc, MSC_STATE_ERROR);
		return ret;
	}
	parse_inquiry_data(buffer, msc);

	LOG_INF("Step 3: READ FORMAT CAPACITIES");
	memset(buffer, 0, sizeof(msc->data_buffer));
	ret = usbh_msc_read_format_capacities(msc, buffer, 12);
	if (ret != 0) {
		if (ret == -EPIPE) {
			LOG_INF("Device does not support READ FORMAT CAPACITIES");
		} else {
			LOG_WRN("READ FORMAT CAPACITIES failed: %d (continuing)", ret);
		}
	} else {
		LOG_HEXDUMP_DBG(buffer, 12, "Format Capacities:");
	}

	LOG_INF("Step 4: READ CAPACITY(10) - Get disk capacity");
	memset(buffer, 0, sizeof(msc->data_buffer));
	ret = usbh_msc_read_capacity(msc, buffer, 8);
	if (ret != 0) {
		if (ret == -EPIPE) {
			LOG_ERR("READ CAPACITY returned STALL");
			msc_reset_recovery(msc);
		}
		LOG_ERR("READ CAPACITY failed: %d", ret);
		msc_state_change(msc, MSC_STATE_ERROR);
		return ret;
	}

	last_lba = sys_get_be32(&buffer[0]);
	block_length = sys_get_be32(&buffer[4]);

	msc->sector_count = last_lba + 1;
	msc->sector_size = block_length;

	LOG_INF("  Last LBA:      0x%08X (%u)", last_lba, last_lba);
	LOG_INF("  Block Length:  0x%08X (%u bytes)", block_length, block_length);
	LOG_INF("  Total Capacity: %u sectors x %u bytes = %.2f MB",
		msc->sector_count, msc->sector_size,
		(double)((uint64_t)msc->sector_count * msc->sector_size) / (1024 * 1024));

	LOG_INF("Step 5: MODE SENSE(6) - Get device mode parameters");
	memset(buffer, 0, sizeof(msc->data_buffer));
	ret = usbh_msc_mode_sense_6(msc, 0x3F, buffer, 24);
	if (ret != 0) {
		if (ret == -EPIPE) {
			msc_handle_optional_command_stall(msc, "MODE_SENSE_6");
		} else {
			LOG_WRN("MODE SENSE(6) failed: %d (continuing)", ret);
		}
	} else {
		LOG_HEXDUMP_DBG(buffer, 24, "Extended MODE SENSE(6):");
	}

	msc->initialized = true;
	msc_state_change(msc, MSC_STATE_READY);

	LOG_INF("=== MSC Device Enumeration Complete ===");
	LOG_INF("Device [%s] ready for file system operations", msc->disk_info.name);

	return 0;
}

static int msc_disk_init(struct disk_info *disk)
{
	struct usbh_msc_data *msc = CONTAINER_OF(disk, struct usbh_msc_data, disk_info);
	int ret;

	k_mutex_lock(&msc->lock, K_FOREVER);

	if (!msc->connected || msc->state == MSC_STATE_DISCONNECTED) {
		k_mutex_unlock(&msc->lock);
		return -ENODEV;
	}

	if (msc->initialized) {
		k_mutex_unlock(&msc->lock);
		return 0;
	}

	k_mutex_unlock(&msc->lock);

	ret = msc_device_init(msc);

	return ret;
}

static int msc_disk_status(struct disk_info *disk)
{
	struct usbh_msc_data *msc = CONTAINER_OF(disk, struct usbh_msc_data, disk_info);

	if (!msc->connected) {
		return DISK_STATUS_NOMEDIA;
	}

	if (msc->initialized && msc->state == MSC_STATE_READY) {
		return DISK_STATUS_OK;
	}

	switch (msc->state) {
	case MSC_STATE_DISCONNECTED:
		return DISK_STATUS_NOMEDIA;
	case MSC_STATE_CONNECTED:
	case MSC_STATE_INITIALIZING:
		return DISK_STATUS_UNINIT;
	case MSC_STATE_READY:
		return DISK_STATUS_OK;
	case MSC_STATE_ERROR:
	default:
		return DISK_STATUS_UNINIT;
	}
}

static int msc_disk_read(struct disk_info *disk, uint8_t *data_buf,
			 uint32_t start_sector, uint32_t num_sectors)
{
	struct usbh_msc_data *msc = CONTAINER_OF(disk, struct usbh_msc_data, disk_info);
	int ret;

	k_mutex_lock(&msc->lock, K_FOREVER);

	if (!msc->initialized || msc->state != MSC_STATE_READY) {
		k_mutex_unlock(&msc->lock);
		return -ENODEV;
	}

	if (num_sectors <= UINT16_MAX) {
		ret = usbh_msc_read10(msc, start_sector, data_buf,
				      num_sectors * msc->sector_size,
				      (uint16_t)num_sectors);
	} else {
		ret = usbh_msc_read12(msc, start_sector, data_buf,
				      num_sectors * msc->sector_size,
				      num_sectors);
	}

	if (ret == 0) {
		msc->stats.read_count++;
	} else {
		msc->stats.error_count++;
	}

	k_mutex_unlock(&msc->lock);
	return ret;
}

static int msc_disk_write(struct disk_info *disk, const uint8_t *data_buf,
			  uint32_t start_sector, uint32_t num_sectors)
{
	struct usbh_msc_data *msc = CONTAINER_OF(disk, struct usbh_msc_data, disk_info);
	int ret;

	k_mutex_lock(&msc->lock, K_FOREVER);

	if (!msc->initialized || msc->state != MSC_STATE_READY) {
		k_mutex_unlock(&msc->lock);
		return -ENODEV;
	}

	if (num_sectors <= UINT16_MAX) {
		ret = usbh_msc_write10(msc, start_sector, data_buf,
				       num_sectors * msc->sector_size,
				       (uint16_t)num_sectors);
	} else {
		ret = usbh_msc_write12(msc, start_sector, data_buf,
				       num_sectors * msc->sector_size,
				       num_sectors);
	}

	if (ret == 0) {
		msc->stats.write_count++;
	} else {
		msc->stats.error_count++;
	}

	k_mutex_unlock(&msc->lock);
	return ret;
}

static int msc_disk_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	struct usbh_msc_data *msc = CONTAINER_OF(disk, struct usbh_msc_data, disk_info);

	switch (cmd) {
	case DISK_IOCTL_CTRL_DEINIT:
		return 0;
	case DISK_IOCTL_CTRL_INIT:
		return 0;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		if (!msc->initialized) {
			return -ENODEV;
		}
		*(uint32_t *)buff = msc->sector_count;
		return 0;

	case DISK_IOCTL_GET_SECTOR_SIZE:
		if (!msc->initialized) {
			return -ENODEV;
		}
		*(uint32_t *)buff = msc->sector_size;
		return 0;

	case DISK_IOCTL_CTRL_SYNC:
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int usbh_msc_init(struct usbh_class_data *c_data)
{
	const struct device *dev = c_data->priv;
	struct usbh_msc_data *msc = dev->data;
	int ret;

	LOG_INF("Initializing MSC host data");

	memset(msc, 0, sizeof(*msc));
	k_mutex_init(&msc->lock);

	msc->disk_info.name = dev->name;
	msc->disk_info.ops = &msc_disk_ops;
	msc->state = MSC_STATE_DISCONNECTED;
	msc->connected = false;

	k_sem_init(&msc->xfer_sem, 0, 1);
	msc->xfer_buf = NULL;

	ret = disk_access_register(&msc->disk_info);
	if (ret != 0) {
		LOG_ERR("Failed to register disk [%s]: %d", msc->disk_info.name, ret);
		return ret;
	}

	LOG_INF("MSC host data initialized successfully");
	LOG_INF("Registered disk: %s", msc->disk_info.name);

	return 0;
}

static int usbh_msc_probe(struct usbh_class_data *const c_data,
			  struct usb_device *const udev,
			  uint8_t iface)
{
	const struct device *dev = c_data->priv;
	struct usbh_msc_data *msc = dev->data;
	const struct usb_if_descriptor *if_desc;
	const struct usb_desc_header *desc;
	uint8_t target_iface;
	int ret;

	LOG_INF("MSC device connected");

	if (udev == NULL || udev->state != USB_STATE_CONFIGURED) {
		LOG_ERR("USB device not properly configured");
		return -ENODEV;
	}

	k_mutex_lock(&msc->lock, K_FOREVER);

	memset(msc->ifaces, 0, sizeof(msc->ifaces));
	msc->num_ifaces = 0;
	msc->current_iface_idx = 0;
	msc->bulk_in_ep_desc = NULL;
	msc->bulk_out_ep_desc = NULL;
	msc->initialized = false;
	memset(&msc->stats, 0, sizeof(msc->stats));

	k_sem_init(&msc->xfer_sem, 0, 1);
	msc->xfer_buf = NULL;

	msc->udev = udev;

	if (iface == USBH_CLASS_IFNUM_DEVICE) {
		target_iface = 0;
	} else {
		target_iface = iface;
	}

	if_desc = (const void *)usbh_desc_get_iface(udev, target_iface);
	if (if_desc == NULL) {
		LOG_ERR("Failed to find interface %u", target_iface);
		ret = -ENODEV;
		goto error_cleanup;
	}

	if (if_desc->bInterfaceClass != USB_CLASS_MASS_STORAGE ||
	    if_desc->bInterfaceSubClass != USB_SUBCLASS_SCSI ||
	    if_desc->bInterfaceProtocol != USB_PROTOCOL_BOT) {
		LOG_ERR("Interface %u is not a valid MSC BOT interface", target_iface);
		ret = -EINVAL;
		goto error_cleanup;
	}

	msc->ifaces[0] = if_desc;
	msc->num_ifaces = 1;
	msc->current_iface_idx = 0;

	LOG_DBG("Found MSC interface %u (alt %u): class=0x%02x, subclass=0x%02x, protocol=0x%02x",
		if_desc->bInterfaceNumber, if_desc->bAlternateSetting,
		if_desc->bInterfaceClass, if_desc->bInterfaceSubClass,
		if_desc->bInterfaceProtocol);

	desc = (const void *)if_desc;
	for (uint8_t i = 0; i < if_desc->bNumEndpoints; i++) {
		const struct usb_ep_descriptor *ep_desc;

		desc = usbh_desc_get_next(desc);
		if (desc == NULL) {
			LOG_ERR("Failed to get endpoint descriptor %u", i);
			ret = -EBADMSG;
			goto error_cleanup;
		}

		if (desc->bDescriptorType != USB_DESC_ENDPOINT) {
			LOG_WRN("Expected endpoint descriptor, got type 0x%02x",
				desc->bDescriptorType);
			continue;
		}

		ep_desc = (const void *)desc;

		if (!usbh_desc_is_valid_endpoint(ep_desc)) {
			LOG_WRN("Invalid endpoint descriptor");
			continue;
		}

		if ((ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_BULK) {
			if (USB_EP_DIR_IS_IN(ep_desc->bEndpointAddress)) {
				msc->bulk_in_ep_desc = ep_desc;
				LOG_DBG("Found Bulk IN endpoint: 0x%02x (maxpkt=%u)",
					ep_desc->bEndpointAddress,
					sys_le16_to_cpu(ep_desc->wMaxPacketSize));
			} else {
				msc->bulk_out_ep_desc = ep_desc;
				LOG_DBG("Found Bulk OUT endpoint: 0x%02x (maxpkt=%u)",
					ep_desc->bEndpointAddress,
					sys_le16_to_cpu(ep_desc->wMaxPacketSize));
			}
		}
	}

	if (msc->bulk_in_ep_desc == NULL || msc->bulk_out_ep_desc == NULL) {
		LOG_ERR("Required bulk endpoints not found (IN=%p, OUT=%p)",
			msc->bulk_in_ep_desc, msc->bulk_out_ep_desc);
		ret = -ENODEV;
		goto error_cleanup;
	}

	if_desc = (const void *)usbh_desc_get_next_alt_setting(if_desc);
	while (if_desc != NULL && msc->num_ifaces < CONFIG_USBH_MSC_MAX_INTERFACE) {
		if (if_desc->bInterfaceNumber == target_iface &&
		    if_desc->bInterfaceClass == USB_CLASS_MASS_STORAGE &&
		    if_desc->bInterfaceSubClass == USB_SUBCLASS_SCSI) {
			msc->ifaces[msc->num_ifaces] = if_desc;
			LOG_DBG("Found alternate setting %u for interface %u",
				if_desc->bAlternateSetting, if_desc->bInterfaceNumber);
			msc->num_ifaces++;
		}

		if_desc = (const void *)usbh_desc_get_next_alt_setting(if_desc);
	}

	LOG_INF("Found %u MSC interface(s) for interface %u",
		msc->num_ifaces, target_iface);

	ret = usbh_device_interface_set(udev, target_iface, 0, false);
	if (ret != 0) {
		LOG_ERR("Failed to set interface %u alt setting 0: %d", target_iface, ret);
		goto error_cleanup;
	}

	LOG_DBG("Interface %u configured successfully", target_iface);

	usbh_req_clear_sfs_halt(msc->udev, msc->bulk_in_ep_desc->bEndpointAddress);
	usbh_req_clear_sfs_halt(msc->udev, msc->bulk_out_ep_desc->bEndpointAddress);

	msc->connected = true;
	msc_state_change(msc, MSC_STATE_CONNECTED);

	k_mutex_unlock(&msc->lock);

	LOG_INF("MSC device [%s] (addr=%d) interface %u initialized successfully",
		msc->disk_info.name, udev->addr, target_iface);
	return 0;

error_cleanup:
	msc->udev = NULL;
	msc->bulk_in_ep_desc = NULL;
	msc->bulk_out_ep_desc = NULL;
	memset(msc->ifaces, 0, sizeof(msc->ifaces));
	msc->num_ifaces = 0;
	msc->current_iface_idx = 0;
	msc->connected = false;
	msc_state_change(msc, MSC_STATE_DISCONNECTED);
	k_mutex_unlock(&msc->lock);
	return ret;
}

static int usbh_msc_removed(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct usbh_msc_data *msc = dev->data;

	LOG_INF("MSC device [%s] disconnected", msc->disk_info.name);

	k_mutex_lock(&msc->lock, K_FOREVER);

	msc->connected = false;
	msc_state_change(msc, MSC_STATE_DISCONNECTED);
	msc->initialized = false;

	msc->bulk_in_ep_desc = NULL;
	msc->bulk_out_ep_desc = NULL;

	memset(msc->ifaces, 0, sizeof(msc->ifaces));
	msc->num_ifaces = 0;
	msc->current_iface_idx = 0;

	msc->sector_count = 0;
	msc->sector_size = 0;

	memset(msc->vendor_id, 0, sizeof(msc->vendor_id));
	memset(msc->product_id, 0, sizeof(msc->product_id));
	memset(msc->product_rev, 0, sizeof(msc->product_rev));
	memset(msc->serial_number, 0, sizeof(msc->serial_number));
	msc->device_type = 0;
	msc->removable = 0;

	memset(&msc->cbw, 0, sizeof(msc->cbw));
	memset(&msc->csw, 0, sizeof(msc->csw));

	msc->tag_counter = 0;

	if (msc->stats.read_count > 0 || msc->stats.write_count > 0) {
		LOG_INF("MSC[%s] statistics: reads=%u, writes=%u, errors=%u, retries=%u",
			msc->disk_info.name,
			msc->stats.read_count, msc->stats.write_count,
			msc->stats.error_count, msc->stats.retry_count);
	}

	memset(&msc->stats, 0, sizeof(msc->stats));

	msc->udev = NULL;

	k_mutex_unlock(&msc->lock);

	LOG_INF("MSC device [%s] removal completed", msc->disk_info.name);
	return 0;
}

static const struct usbh_class_filter usbh_msc_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_CLASS_MASS_STORAGE,
		.sub = USB_SUBCLASS_SCSI,
		.proto = USB_PROTOCOL_BOT,
	},
	{0},
};

static struct usbh_class_api msc_class_api = {
	.init = usbh_msc_init,
	.probe = usbh_msc_probe,
	.removed = usbh_msc_removed,
};

#define USBH_MSC_DEVICE_DEFINE(n, _)						\
	static struct usbh_msc_data usbh_msc_data##n;				\
										\
	DEVICE_DEFINE(usbh_msc_##n, "usbh_msc_" #n, NULL, NULL,		\
		      &usbh_msc_data##n, NULL, POST_KERNEL,			\
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);		\
										\
	USBH_DEFINE_CLASS(usbh_msc_c_data_##n, &msc_class_api,			\
			  (void *)DEVICE_GET(usbh_msc_##n), usbh_msc_filters);

LISTIFY(CONFIG_USBH_MSC_CLASS_INSTANCES_COUNT, USBH_MSC_DEVICE_DEFINE, (;), _)
