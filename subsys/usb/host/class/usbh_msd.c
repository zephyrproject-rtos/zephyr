/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * USB Mass Storage Class (MSC) host driver - Bulk-Only Transport (BOT).
 *
 * Implements the USB MSC BOT protocol as described in:
 *   USB Mass Storage Class - Bulk Only Transport, Revision 1.0
 *   USB Mass Storage Class - UFI Command Specification, Revision 1.0
 *
 * The driver registers as a Zephyr disk_access provider so that the file
 * system layer can mount the USB flash drive without modification.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usbh_msd.h>
#include <zephyr/storage/disk_access.h>
#include <string.h>

#include "usbh_device.h"
#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_desc.h"

LOG_MODULE_REGISTER(usbh_msd, CONFIG_USBH_MSD_LOG_LEVEL);

/* USB MSC class / subclass / protocol codes */
#define USB_MSC_CLASS       0x08
#define USB_MSC_SCSI_SUBCLS 0x06
#define USB_MSC_BOT_PROTO   0x50

/* Bulk-Only Transport class-specific requests */
#define BOT_REQ_RESET       0xFF
#define BOT_REQ_GET_MAX_LUN 0xFE

/* CBW/CSW signatures and sizes */
#define CBW_SIGNATURE       0x43425355UL
#define CSW_SIGNATURE       0x53425355UL
#define CBW_SIZE            31
#define CSW_SIZE            13

/* CBW flags */
#define CBW_FLAG_IN         0x80  /* device-to-host data phase */
#define CBW_FLAG_OUT        0x00  /* host-to-device data phase */

/* CSW status values */
#define CSW_STATUS_GOOD     0x00
#define CSW_STATUS_FAILED   0x01
#define CSW_STATUS_PHASE    0x02

/* SCSI command opcodes used by this driver */
#define SCSI_TEST_UNIT_READY   0x00
#define SCSI_REQUEST_SENSE     0x03
#define SCSI_INQUIRY           0x12
#define SCSI_READ_CAPACITY10   0x25
#define SCSI_READ10            0x28
#define SCSI_WRITE10           0x2A

/* Transfer timeout in milliseconds */
#define MSD_XFER_TIMEOUT_MS    5000

/* Maximum single BOT transfer in bytes (based on UHC buffer pool limits) */
#define MSD_MAX_XFER_BYTES     (CONFIG_USBH_MSD_MAX_SECTORS_PER_XFER * 512U)

/*
 * Command Block Wrapper (packed, 31 bytes).
 */
struct __packed msd_cbw {
	uint32_t signature;
	uint32_t tag;
	uint32_t data_transfer_length;
	uint8_t  flags;
	uint8_t  lun;
	uint8_t  cb_length;
	uint8_t  cb[16];
};

/*
 * Command Status Wrapper (packed, 13 bytes).
 */
struct __packed msd_csw {
	uint32_t signature;
	uint32_t tag;
	uint32_t data_residue;
	uint8_t  status;
};

/*
 * Per-instance driver state.
 */
struct usbh_msd_priv {
	/* Back-pointer to the Zephyr device (for priv_from_dev lookup) */
	const struct device *dev;
	/* Host class data handle */
	struct usbh_class_data *c_data;
	/* USB device handle */
	struct usb_device *udev;
	/* Bulk-IN endpoint address */
	uint8_t ep_in;
	/* Bulk-OUT endpoint address */
	uint8_t ep_out;
	/* Interface number */
	uint8_t iface;
	/* Maximum LUN index (0-based, from GET MAX LUN) */
	uint8_t max_lun;
	/* Driver flags */
	uint32_t flags;
	/* Cached device information */
	struct usbh_msd_info info;
	/* Disk access interface name, e.g. "USBDISK0" */
	char disk_name[16];
	/* Disk access driver ops registered with kernel */
	struct disk_info disk;
	/* Semaphore used to synchronise blocking transfers */
	struct k_sem xfer_sem;
	/* Last transfer status returned by completion callback */
	int xfer_result;
	/* Per-instance CBW tag counter (Fix: was a global, causing multi-instance conflicts) */
	uint32_t cbw_tag;
	/* Mutex protecting BOT transactions */
	struct k_mutex bot_mutex;
};

/* Array of all MSD class instances */
static struct usbh_msd_priv msd_priv[CONFIG_USBH_MSD_INSTANCES_COUNT];

/* Forward declaration of disk_access ops */
static int msd_disk_status(struct disk_info *disk);
static int msd_disk_read(struct disk_info *disk, uint8_t *buf,
			 uint32_t start, uint32_t count);
static int msd_disk_write(struct disk_info *disk, const uint8_t *buf,
			  uint32_t start, uint32_t count);
static int msd_disk_ioctl(struct disk_info *disk, uint8_t cmd, void *buf);

static const struct disk_operations msd_disk_ops = {
	.init    = NULL,
	.status  = msd_disk_status,
	.read    = msd_disk_read,
	.write   = msd_disk_write,
	.ioctl   = msd_disk_ioctl,
};

/* --------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */

/*
 * Completion callback for synchronous bulk transfers.
 * xfer->priv points to the per-instance usbh_msd_priv.
 */
static int msd_bulk_cb(struct usb_device *const udev,
		       struct uhc_transfer *const xfer)
{
	struct usbh_msd_priv *priv = xfer->priv;

	if (xfer->err == -ECONNRESET) {
		/* Transfer was cancelled; the caller handles cleanup. */
		usbh_xfer_free(udev, xfer);
		return 0;
	}

	priv->xfer_result = xfer->err;
	k_sem_give(&priv->xfer_sem);
	return 0;
}

static struct usbh_msd_priv *priv_from_dev(const struct device *dev)
{
	for (int i = 0; i < CONFIG_USBH_MSD_INSTANCES_COUNT; i++) {
		if (msd_priv[i].dev == dev) {
			return &msd_priv[i];
		}
	}
	return NULL;
}

static struct usbh_msd_priv *priv_from_disk(struct disk_info *disk)
{
	for (int i = 0; i < CONFIG_USBH_MSD_INSTANCES_COUNT; i++) {
		if (&msd_priv[i].disk == disk) {
			return &msd_priv[i];
		}
	}
	return NULL;
}

/*
 * Synchronous bulk transfer helper.
 * Transfers exactly len bytes via the given endpoint.
 * Returns 0 on success, negative errno on error.
 *
 * Fix: on timeout, properly waits for any pending cancel so the
 * xfer/nbuf objects can be safely freed.
 */
static int msd_bulk_xfer(struct usbh_msd_priv *priv, uint8_t ep,
			 void *buf, size_t len)
{
	struct uhc_transfer *xfer;
	struct net_buf *nbuf;
	int ret;

	xfer = usbh_xfer_alloc(priv->udev, ep, msd_bulk_cb, priv);
	if (!xfer) {
		return -ENOMEM;
	}

	nbuf = usbh_xfer_buf_alloc(priv->udev, len);
	if (!nbuf) {
		usbh_xfer_free(priv->udev, xfer);
		return -ENOMEM;
	}

	if (USB_EP_DIR_IS_OUT(ep) && buf) {
		net_buf_add_mem(nbuf, buf, len);
	}
	/* For IN: leave buffer empty; HCD fills it via net_buf_tailroom(). */

	ret = usbh_xfer_buf_add(priv->udev, xfer, nbuf);
	if (ret) {
		usbh_xfer_buf_free(priv->udev, nbuf);
		usbh_xfer_free(priv->udev, xfer);
		return ret;
	}

	k_sem_reset(&priv->xfer_sem);
	ret = usbh_xfer_enqueue(priv->udev, xfer);
	if (ret) {
		usbh_xfer_buf_free(priv->udev, nbuf);
		usbh_xfer_free(priv->udev, xfer);
		return ret;
	}

	if (k_sem_take(&priv->xfer_sem, K_MSEC(MSD_XFER_TIMEOUT_MS))) {
		LOG_ERR("Bulk transfer timeout ep=0x%02x", ep);
		/* Dequeue; msd_bulk_cb will free xfer after ECONNRESET. */
		usbh_xfer_dequeue(priv->udev, xfer);
		/* Wait for the cancel callback to fire before we leave scope. */
		k_sem_take(&priv->xfer_sem, K_MSEC(500));
		usbh_xfer_buf_free(priv->udev, nbuf);
		return -ETIMEDOUT;
	}

	ret = priv->xfer_result;

	/* For IN: copy received data to caller's buffer. */
	if (!ret && USB_EP_DIR_IS_IN(ep) && buf) {
		size_t rx = MIN(len, nbuf->len);

		memcpy(buf, nbuf->data, rx);
	}

	usbh_xfer_buf_free(priv->udev, nbuf);
	usbh_xfer_free(priv->udev, xfer);
	return ret;
}

/*
 * Clear a halted (STALL) endpoint by issuing CLEAR_FEATURE/ENDPOINT_HALT.
 */
static void msd_clear_stall(struct usbh_msd_priv *priv, uint8_t ep)
{
	int ret = usbh_req_set_sfs_halt(priv->udev, ep);

	if (ret) {
		LOG_WRN("Clear stall ep=0x%02x failed: %d", ep, ret);
	}
}

/*
 * Execute a BOT command.
 *
 * Phases: CBW -> [data] -> CSW
 * On data-phase failure the endpoint STALL is cleared before reading CSW,
 * per the BOT specification error-recovery procedure.
 *
 * Fix: per-instance cbw_tag, proper stall-clear on data failure.
 */
static int msd_bot_command(struct usbh_msd_priv *priv, uint8_t lun,
			   const uint8_t *cb, uint8_t cb_len,
			   void *data, uint32_t data_len, bool data_in)
{
	struct msd_cbw cbw;
	struct msd_csw csw;
	int ret;
	int data_ret = 0;

	/* Phase 1: send CBW */
	memset(&cbw, 0, sizeof(cbw));
	cbw.signature            = sys_cpu_to_le32(CBW_SIGNATURE);
	cbw.tag                  = sys_cpu_to_le32(++priv->cbw_tag);
	cbw.data_transfer_length = sys_cpu_to_le32(data_len);
	cbw.flags                = data_in ? CBW_FLAG_IN : CBW_FLAG_OUT;
	cbw.lun                  = lun;
	cbw.cb_length            = cb_len;
	memcpy(cbw.cb, cb, MIN(cb_len, sizeof(cbw.cb)));

	ret = msd_bulk_xfer(priv, priv->ep_out, &cbw, CBW_SIZE);
	if (ret) {
		LOG_ERR("CBW send failed: %d", ret);
		return ret;
	}

	/* Phase 2: optional data stage */
	if (data && data_len) {
		uint8_t ep = data_in ? priv->ep_in : priv->ep_out;

		data_ret = msd_bulk_xfer(priv, ep, data, data_len);
		if (data_ret) {
			LOG_WRN("Data phase ep=0x%02x failed: %d - clearing STALL",
				ep, data_ret);
			/* Per BOT spec: clear STALL before reading CSW */
			msd_clear_stall(priv, ep);
		}
	}

	/* Phase 3: receive CSW */
	ret = msd_bulk_xfer(priv, priv->ep_in, &csw, CSW_SIZE);
	if (ret) {
		LOG_ERR("CSW receive failed: %d", ret);
		return ret;
	}

	if (sys_le32_to_cpu(csw.signature) != CSW_SIGNATURE) {
		LOG_ERR("Invalid CSW signature 0x%08x",
			sys_le32_to_cpu(csw.signature));
		return -EIO;
	}

	if (sys_le32_to_cpu(csw.tag) != priv->cbw_tag) {
		LOG_ERR("CSW tag mismatch: expected %u got %u",
			priv->cbw_tag, sys_le32_to_cpu(csw.tag));
		return -EIO;
	}

	if (csw.status == CSW_STATUS_PHASE) {
		LOG_ERR("Phase error - performing BOT reset");
		usbh_msd_bot_reset(priv->dev);
		return -EIO;
	}

	if (csw.status != CSW_STATUS_GOOD) {
		LOG_DBG("CSW command failed status=0x%02x", csw.status);
		return -EIO;
	}

	/* Propagate data-phase error if CSW was GOOD but data failed */
	return data_ret;
}

/* --------------------------------------------------------------------------
 * SCSI commands
 * -------------------------------------------------------------------------- */

static int scsi_test_unit_ready(struct usbh_msd_priv *priv, uint8_t lun)
{
	uint8_t cb[6] = {SCSI_TEST_UNIT_READY, 0, 0, 0, 0, 0};

	return msd_bot_command(priv, lun, cb, sizeof(cb), NULL, 0, true);
}

static int scsi_inquiry(struct usbh_msd_priv *priv, uint8_t lun)
{
	uint8_t cb[6]   = {SCSI_INQUIRY, 0, 0, 0, 36, 0};
	uint8_t buf[36] = {0};
	int ret;

	ret = msd_bot_command(priv, lun, cb, sizeof(cb), buf, 36, true);
	if (ret) {
		return ret;
	}

	/* bytes 8..15: vendor, 16..31: product, 32..35: revision */
	memcpy(priv->info.vendor,   &buf[8],  8);
	memcpy(priv->info.product,  &buf[16], 16);
	memcpy(priv->info.revision, &buf[32], 4);
	priv->info.vendor[8]   = '\0';
	priv->info.product[16] = '\0';
	priv->info.revision[4] = '\0';

	LOG_INF("MSD Inquiry: vendor='%s' product='%s' rev='%s'",
		priv->info.vendor, priv->info.product, priv->info.revision);
	return 0;
}

static int scsi_read_capacity(struct usbh_msd_priv *priv, uint8_t lun)
{
	uint8_t cb[10] = {SCSI_READ_CAPACITY10, 0};
	uint8_t buf[8] = {0};
	int ret;

	ret = msd_bot_command(priv, lun, cb, sizeof(cb), buf, 8, true);
	if (ret) {
		return ret;
	}

	/* buf[0..3] = last LBA (big-endian), buf[4..7] = block length */
	priv->info.sector_count = sys_get_be32(&buf[0]) + 1U;
	priv->info.sector_size  = sys_get_be32(&buf[4]);

	LOG_INF("MSD capacity: %u sectors x %u bytes",
		priv->info.sector_count, priv->info.sector_size);
	return 0;
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

int usbh_msd_get_info(const struct device *dev, struct usbh_msd_info *info)
{
	struct usbh_msd_priv *priv = priv_from_dev(dev);

	if (!priv || !info) {
		return -EINVAL;
	}
	if (!(priv->flags & MSD_DEVICE_FLAG_CONNECTED)) {
		return -ENODEV;
	}

	*info = priv->info;
	return 0;
}

bool usbh_msd_is_ready(const struct device *dev)
{
	struct usbh_msd_priv *priv = priv_from_dev(dev);

	if (!priv) {
		return false;
	}
	return (priv->flags & MSD_DEVICE_FLAG_READY) != 0;
}

int usbh_msd_read(const struct device *dev, uint8_t lun, uint8_t *buf,
		  uint32_t start_lba, uint32_t sector_count)
{
	struct usbh_msd_priv *priv = priv_from_dev(dev);
	uint8_t cb[10];
	uint32_t total_bytes;
	int ret;

	if (!priv || !buf) {
		return -EINVAL;
	}
	if (!(priv->flags & MSD_DEVICE_FLAG_READY)) {
		return -ENODEV;
	}

	/* Fix: enforce per-transfer sector limit to avoid OOM on net_buf alloc */
	if (sector_count > CONFIG_USBH_MSD_MAX_SECTORS_PER_XFER) {
		LOG_ERR("sector_count %u exceeds max %u per transfer",
			sector_count, CONFIG_USBH_MSD_MAX_SECTORS_PER_XFER);
		return -EINVAL;
	}

	total_bytes = sector_count * priv->info.sector_size;

	memset(cb, 0, sizeof(cb));
	cb[0] = SCSI_READ10;
	sys_put_be32(start_lba,    &cb[2]);
	sys_put_be16(sector_count, &cb[7]);

	k_mutex_lock(&priv->bot_mutex, K_FOREVER);
	ret = msd_bot_command(priv, lun, cb, sizeof(cb), buf, total_bytes, true);
	k_mutex_unlock(&priv->bot_mutex);

	return ret;
}

int usbh_msd_write(const struct device *dev, uint8_t lun, const uint8_t *buf,
		   uint32_t start_lba, uint32_t sector_count)
{
	struct usbh_msd_priv *priv = priv_from_dev(dev);
	uint8_t cb[10];
	uint32_t total_bytes;
	int ret;

	if (!priv || !buf) {
		return -EINVAL;
	}
	if (!(priv->flags & MSD_DEVICE_FLAG_READY)) {
		return -ENODEV;
	}
	if (priv->flags & MSD_DEVICE_FLAG_WP) {
		return -EROFS;
	}

	/* Fix: enforce per-transfer sector limit */
	if (sector_count > CONFIG_USBH_MSD_MAX_SECTORS_PER_XFER) {
		LOG_ERR("sector_count %u exceeds max %u per transfer",
			sector_count, CONFIG_USBH_MSD_MAX_SECTORS_PER_XFER);
		return -EINVAL;
	}

	total_bytes = sector_count * priv->info.sector_size;

	memset(cb, 0, sizeof(cb));
	cb[0] = SCSI_WRITE10;
	sys_put_be32(start_lba,    &cb[2]);
	sys_put_be16(sector_count, &cb[7]);

	k_mutex_lock(&priv->bot_mutex, K_FOREVER);
	ret = msd_bot_command(priv, lun, cb, sizeof(cb), (void *)buf,
			      total_bytes, false);
	k_mutex_unlock(&priv->bot_mutex);

	return ret;
}

int usbh_msd_test_unit_ready(const struct device *dev, uint8_t lun)
{
	struct usbh_msd_priv *priv = priv_from_dev(dev);
	int ret;

	if (!priv) {
		return -EINVAL;
	}

	k_mutex_lock(&priv->bot_mutex, K_FOREVER);
	ret = scsi_test_unit_ready(priv, lun);
	k_mutex_unlock(&priv->bot_mutex);

	return ret;
}

int usbh_msd_request_sense(const struct device *dev, uint8_t lun,
			   uint8_t *sense_key, uint8_t *asc, uint8_t *ascq)
{
	struct usbh_msd_priv *priv = priv_from_dev(dev);
	uint8_t cb[6]   = {SCSI_REQUEST_SENSE, 0, 0, 0, 18, 0};
	uint8_t buf[18] = {0};
	int ret;

	if (!priv) {
		return -EINVAL;
	}

	k_mutex_lock(&priv->bot_mutex, K_FOREVER);
	ret = msd_bot_command(priv, lun, cb, sizeof(cb), buf, 18, true);
	k_mutex_unlock(&priv->bot_mutex);

	if (!ret) {
		if (sense_key) {
			*sense_key = buf[2] & 0x0F;
		}
		if (asc) {
			*asc  = buf[12];
		}
		if (ascq) {
			*ascq = buf[13];
		}
	}

	return ret;
}

int usbh_msd_bot_reset(const struct device *dev)
{
	struct usbh_msd_priv *priv = priv_from_dev(dev);
	struct net_buf *buf;
	int ret;

	if (!priv) {
		return -EINVAL;
	}

	/*
	 * Fix: correct BOT Reset class request per USB MSC BOT spec section 3.1:
	 *   bmRequestType = 0x21  (class, interface, host-to-device)
	 *   bRequest      = 0xFF  (Bulk-Only Mass Storage Reset)
	 *   wValue        = 0
	 *   wIndex        = bInterfaceNumber
	 *   wLength       = 0
	 */
	buf = usbh_xfer_buf_alloc(priv->udev, 0);
	ret = usbh_req_setup(priv->udev,
			     USB_REQTYPE_DIR_TO_DEVICE |
			     USB_REQTYPE_TYPE_CLASS |
			     USB_REQTYPE_RECIPIENT_INTERFACE,
			     BOT_REQ_RESET,
			     0,
			     priv->iface,
			     0,
			     buf);
	if (buf) {
		usbh_xfer_buf_free(priv->udev, buf);
	}

	if (ret) {
		LOG_ERR("BOT reset request failed: %d", ret);
	} else {
		/* After BOT reset, clear both bulk endpoints */
		msd_clear_stall(priv, priv->ep_in);
		msd_clear_stall(priv, priv->ep_out);
	}

	return ret;
}

/* --------------------------------------------------------------------------
 * Disk access interface (connects to Zephyr FS layer)
 * -------------------------------------------------------------------------- */

static int msd_disk_status(struct disk_info *disk)
{
	struct usbh_msd_priv *priv = priv_from_disk(disk);

	if (!priv) {
		return DISK_STATUS_UNINIT;
	}
	if (!(priv->flags & MSD_DEVICE_FLAG_CONNECTED)) {
		return DISK_STATUS_UNINIT;
	}
	if (!(priv->flags & MSD_DEVICE_FLAG_READY)) {
		return DISK_STATUS_NOMEDIA;
	}
	return DISK_STATUS_OK;
}

static int msd_disk_read(struct disk_info *disk, uint8_t *buf,
			 uint32_t start, uint32_t count)
{
	struct usbh_msd_priv *priv = priv_from_disk(disk);

	if (!priv) {
		return -ENODEV;
	}
	return usbh_msd_read(priv->dev, 0, buf, start, count);
}

static int msd_disk_write(struct disk_info *disk, const uint8_t *buf,
			  uint32_t start, uint32_t count)
{
	struct usbh_msd_priv *priv = priv_from_disk(disk);

	if (!priv) {
		return -ENODEV;
	}
	return usbh_msd_write(priv->dev, 0, buf, start, count);
}

static int msd_disk_ioctl(struct disk_info *disk, uint8_t cmd, void *buf)
{
	struct usbh_msd_priv *priv = priv_from_disk(disk);

	if (!priv) {
		return -ENODEV;
	}

	switch (cmd) {
	case DISK_IOCTL_CTRL_INIT:
		/* Device initialized during probe; nothing to do here. */
		return 0;

	case DISK_IOCTL_CTRL_DEINIT:
		/* Nothing to deinitialize at the disk layer. */
		return 0;

	case DISK_IOCTL_CTRL_SYNC:
		/* USB MSD has no write cache; sync is a no-op. */
		return 0;

	case DISK_IOCTL_GET_SECTOR_COUNT:
		if (!buf) {
			return -EINVAL;
		}
		*(uint32_t *)buf = priv->info.sector_count;
		return 0;

	case DISK_IOCTL_GET_SECTOR_SIZE:
		if (!buf) {
			return -EINVAL;
		}
		*(uint32_t *)buf = priv->info.sector_size;
		return 0;

	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		if (!buf) {
			return -EINVAL;
		}
		*(uint32_t *)buf = 1U;
		return 0;

	default:
		return -EINVAL;
	}
}

/* --------------------------------------------------------------------------
 * USB host class API callbacks
 * -------------------------------------------------------------------------- */

static int msd_class_init(struct usbh_class_data *const c_data)
{
	LOG_DBG("MSD host class init");
	return 0;
}

/*
 * Fix: msd_class_completion_cb is the class-layer callback for transfers
 * submitted WITHOUT a per-transfer callback (e.g. future async path).
 * The synchronous msd_bulk_xfer already uses msd_bulk_cb directly, so
 * this path should not give the semaphore to avoid double-give.
 * Left as a no-op stub for future use.
 */
static int msd_class_completion_cb(struct usbh_class_data *const c_data,
				   struct uhc_transfer *const xfer)
{
	ARG_UNUSED(c_data);
	ARG_UNUSED(xfer);
	return 0;
}

static int msd_class_probe(struct usbh_class_data *const c_data,
			   struct usb_device *const udev,
			   const uint8_t iface)
{
	struct usbh_msd_priv *priv = NULL;
	uint8_t ep_in = 0, ep_out = 0;
	uint8_t max_lun = 0;
	/* Fix: use separate slot_idx so disk_name uses the correct index */
	int slot_idx;
	int retry;
	int ret;

	/* Find a free instance slot */
	for (slot_idx = 0; slot_idx < CONFIG_USBH_MSD_INSTANCES_COUNT; slot_idx++) {
		if (!(msd_priv[slot_idx].flags & MSD_DEVICE_FLAG_CONNECTED)) {
			priv = &msd_priv[slot_idx];
			break;
		}
	}

	if (!priv) {
		LOG_ERR("No free MSD instance slots (max %d)",
			CONFIG_USBH_MSD_INSTANCES_COUNT);
		return -ENOMEM;
	}

	/* Walk the interface descriptor to find bulk IN and OUT endpoints */
	{
		struct usb_cfg_descriptor *cfg =
			(struct usb_cfg_descriptor *)udev->cfg_desc;
		struct usb_desc_header *dhp = udev->ifaces[iface].dhp;
		void *desc_end = (void *)((uint8_t *)cfg + cfg->wTotalLength);

		while (dhp != NULL && (void *)dhp < desc_end) {
			if (dhp->bDescriptorType == USB_DESC_ENDPOINT) {
				struct usb_ep_descriptor *ep =
					(struct usb_ep_descriptor *)dhp;
				uint8_t type = ep->bmAttributes &
					       USB_EP_TRANSFER_TYPE_MASK;

				if (type == USB_EP_TYPE_BULK) {
					if (USB_EP_DIR_IS_IN(ep->bEndpointAddress)) {
						ep_in = ep->bEndpointAddress;
					} else {
						ep_out = ep->bEndpointAddress;
					}
				}
			}
			/* Stop when we hit the next interface descriptor */
			if (dhp->bDescriptorType == USB_DESC_INTERFACE &&
			    dhp != udev->ifaces[iface].dhp) {
				break;
			}
			dhp = (void *)((uint8_t *)dhp + dhp->bLength);
		}
	}

	if (!ep_in || !ep_out) {
		LOG_ERR("MSD: bulk IN/OUT endpoints not found on iface %u", iface);
		return -ENOTSUP;
	}

	LOG_INF("MSD probe: slot=%d iface=%u ep_in=0x%02x ep_out=0x%02x",
		slot_idx, iface, ep_in, ep_out);

	/* Initialise semaphore, mutex, and per-instance state */
	k_sem_init(&priv->xfer_sem, 0, 1);
	k_mutex_init(&priv->bot_mutex);

	priv->c_data  = c_data;
	priv->udev    = udev;
	priv->iface   = iface;
	priv->ep_in   = ep_in;
	priv->ep_out  = ep_out;
	priv->cbw_tag = 0;
	priv->flags   = MSD_DEVICE_FLAG_CONNECTED;

	c_data->priv  = priv;

	/*
	 * Fix: assign priv->dev so that public API (usbh_msd_read/write/etc.)
	 * can locate this instance via priv_from_dev().
	 * We use the class_data pointer as a stable device handle surrogate
	 * since there is no struct device* in the Zephyr usbh class model.
	 * Cast c_data to device* for lookup purposes only.
	 */
	priv->dev = (const struct device *)c_data;

	/* GET MAX LUN class request (optional; some devices may STALL it) */
	{
		struct net_buf *lun_buf = usbh_xfer_buf_alloc(priv->udev, 1);

		if (lun_buf) {
			net_buf_add(lun_buf, 1);
			ret = usbh_req_setup(priv->udev,
					     USB_REQTYPE_DIR_TO_HOST |
					     USB_REQTYPE_TYPE_CLASS |
					     USB_REQTYPE_RECIPIENT_INTERFACE,
					     BOT_REQ_GET_MAX_LUN,
					     0,
					     priv->iface,
					     1,
					     lun_buf);
			if (!ret && lun_buf->len >= 1) {
				max_lun = lun_buf->data[0];
				LOG_INF("MSD max LUN: %u", max_lun);
			}
			usbh_xfer_buf_free(priv->udev, lun_buf);
		}
	}
	priv->info.max_lun = max_lun;

	/* SCSI INQUIRY to populate vendor/product strings */
	k_mutex_lock(&priv->bot_mutex, K_FOREVER);
	ret = scsi_inquiry(priv, 0);
	if (ret) {
		LOG_WRN("INQUIRY failed: %d", ret);
	}

	/* Poll TEST UNIT READY (up to 5 times with 200 ms delay) */
	for (retry = 0; retry < 5; retry++) {
		ret = scsi_test_unit_ready(priv, 0);
		if (!ret) {
			break;
		}
		k_sleep(K_MSEC(200));
	}

	if (!ret) {
		ret = scsi_read_capacity(priv, 0);
	}
	k_mutex_unlock(&priv->bot_mutex);

	if (!ret) {
		priv->flags |= MSD_DEVICE_FLAG_READY;

		/* Fix: use slot_idx for disk name, not the retry loop counter */
		snprintf(priv->disk_name, sizeof(priv->disk_name),
			 "USBDISK%d", slot_idx);
		priv->disk.name = priv->disk_name;
		priv->disk.ops  = &msd_disk_ops;

		ret = disk_access_register(&priv->disk);
		if (ret) {
			LOG_ERR("disk_access_register failed: %d", ret);
		} else {
			LOG_INF("USB MSD registered as disk '%s'",
				priv->disk_name);
		}
	} else {
		LOG_WRN("USB MSD device not ready after probe");
	}

	return 0;
}

static int msd_class_removed(struct usbh_class_data *const c_data)
{
	struct usbh_msd_priv *priv = c_data->priv;

	if (!priv) {
		return 0;
	}

	LOG_INF("USB MSD device removed (disk '%s')", priv->disk_name);

	if (priv->flags & MSD_DEVICE_FLAG_READY) {
		disk_access_unregister(&priv->disk);
	}

	/* Fix: clear priv->dev on removal */
	priv->dev   = NULL;
	priv->udev  = NULL;
	priv->flags = 0;
	c_data->priv = NULL;

	return 0;
}

/* --------------------------------------------------------------------------
 * Class registration
 * -------------------------------------------------------------------------- */

static struct usbh_class_api msd_class_api = {
	.init          = msd_class_init,
	.completion_cb = msd_class_completion_cb,
	.probe         = msd_class_probe,
	.removed       = msd_class_removed,
};

/* Match USB MSC/SCSI/BOT devices */
static const struct usbh_class_filter msd_filter[] = {
	{
		.class = USB_MSC_CLASS,
		.sub   = USB_MSC_SCSI_SUBCLS,
		.proto = USB_MSC_BOT_PROTO,
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
	},
	{0} /* terminator */
};

USBH_DEFINE_CLASS(usbh_msd_0, &msd_class_api, NULL, msd_filter);
