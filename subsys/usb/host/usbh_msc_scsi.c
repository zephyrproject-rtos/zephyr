/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/scsi/scsi.h>
#include <zephyr/usb/usb_msc_scsi.h>
#if IS_ENABLED(CONFIG_USBH_MSC_DISK)
#include <zephyr/usb/usb_msc_disk.h>
#endif
#include <zephyr/usb/usbh_msc.h>
#include <zephyr/usb/usbh_msc_scsi_cmd.h>

#include "usbh_device.h"
#include "usbh_msc_flow.h"
#include "usbh_msc_bot_transport.h"

LOG_MODULE_REGISTER(usbh_msc_scsi, CONFIG_USBH_LOG_LEVEL);

static void msc_inquiry_log_summary(const uint8_t *inq, size_t len)
{
	char vendor[9];
	char product[17];
	char rev[5];

	if (len < 36U) {
		LOG_WRN("msc_inquiry: short data len=%u (expect 36)", (unsigned int)len);
		return;
	}

	(void)memcpy(vendor, &inq[8], 8U);
	vendor[8] = '\0';
	(void)memcpy(product, &inq[16], 16U);
	product[16] = '\0';
	(void)memcpy(rev, &inq[32], 4U);
	rev[4] = '\0';

	for (int i = 7; i >= 0 && vendor[i] == ' '; i--) {
		vendor[i] = '\0';
	}
	for (int i = 15; i >= 0 && product[i] == ' '; i--) {
		product[i] = '\0';
	}
	for (int i = 3; i >= 0 && rev[i] == ' '; i--) {
		rev[i] = '\0';
	}

	LOG_DBG("msc INQUIRY bits: b0=0x%02x PQual=%u DevType=%u RMB=%u | b1=0x%02x "
		"ISO_ECMA_ATAPI=%u",
		inq[0], (unsigned int)(inq[0] >> 5) & 7U, (unsigned int)inq[0] & 0x1fU,
		(unsigned int)(inq[1] & 0x80U ? 1U : 0U), inq[1], (unsigned int)(inq[1] & 0x7fU));
	LOG_INF("msc_inquiry: qualifier/type=0x%02x  removable=%u", inq[0],
		(unsigned int)(inq[1] & 0x80U ? 1U : 0U));
	LOG_INF("msc_inquiry: vendor=\"%s\" product=\"%s\" revision=\"%s\"", vendor, product, rev);
}

struct msc_inquiry_ctx {
	uint8_t *buf;
	uint32_t len;
};

static int msc_inquiry_fn(struct scsi_device *sdev, void *arg)
{
	struct msc_inquiry_ctx *ctx = arg;

	return scsi_inquiry(sdev, ctx->buf, ctx->len);
}

int usbh_msc_scsi_inquiry(struct usb_device *udev, const struct usbh_msc_iface *msc_opt,
			  uint8_t lun)
{
	struct usbh_msc_iface msc_local;
	const struct usbh_msc_iface *msc;
	uint8_t inq_buf[USB_SCSI_INQUIRY_DATA_LEN];
	struct msc_inquiry_ctx ctx = {
		.buf = inq_buf,
		.len = sizeof(inq_buf),
	};
	int ret;

	if (udev == NULL) {
		LOG_ERR("msc_inquiry: FAIL - udev is NULL");
		return -EINVAL;
	}

	if (lun > 15U) {
		LOG_ERR("msc_inquiry: invalid LUN %u", lun);
		return -EINVAL;
	}

	if (udev->state != USB_STATE_CONFIGURED) {
		LOG_WRN("msc_inquiry: FAIL - state=%d (need CONFIGURED)", (int)udev->state);
		return -ENOTCONN;
	}

	if (msc_opt == NULL) {
		ret = usbh_msc_prepare(udev, &msc_local);
		if (ret != 0) {
			LOG_WRN("msc_inquiry: FAIL - usbh_msc_prepare err=%d", ret);
			return ret;
		}
		msc = &msc_local;
	} else {
		msc = msc_opt;
	}

	if (lun > msc->max_lun) {
		LOG_WRN("msc_inquiry: LUN %u > max_lun %u (GET_MAX_LUN)", lun, msc->max_lun);
		return -EINVAL;
	}

	LOG_INF("msc_inquiry: LUN %u SCSI INQUIRY", lun);

	ret = usb_msc_scsi_with_lun(NULL, udev, msc, lun, msc_inquiry_fn, &ctx);
	if (ret != 0) {
		LOG_ERR("msc_inquiry: INQUIRY failed: %d", ret);
		return ret;
	}

	msc_inquiry_log_summary(inq_buf, sizeof(inq_buf));
	LOG_DBG("msc_inquiry: complete (LUN %u)", lun);
	return 0;
}

static int msc_tur_fn(struct scsi_device *sdev, void *arg)
{
	int ret;

	ARG_UNUSED(arg);

	ret = scsi_test_unit_ready(sdev);
	if (ret == 0) {
		usbh_msc_bot_transport_after_tur_ok();
	}

	return ret;
}

int usbh_msc_scsi_test_unit_ready(struct usb_device *udev, const struct usbh_msc_iface *msc,
				  uint8_t lun, const struct device *uhc_host)
{
	if (udev == NULL || msc == NULL) {
		return -EINVAL;
	}

	if (lun > msc->max_lun) {
		return -EINVAL;
	}

	return usb_msc_scsi_with_lun(uhc_host, udev, msc, lun, msc_tur_fn, NULL);
}

struct msc_mode_sense6_ctx {
	uint8_t *buf;
	uint32_t len;
	uint8_t pc;
	uint8_t page_code;
	uint8_t subpage;
	bool disable_block_descriptors;
};

static int msc_mode_sense6_fn(struct scsi_device *sdev, void *arg)
{
	struct msc_mode_sense6_ctx *ctx = arg;

	return scsi_mode_sense_6_params(sdev, ctx->buf, ctx->len, ctx->pc, ctx->page_code,
					ctx->subpage, ctx->disable_block_descriptors);
}

int usbh_msc_scsi_mode_sense6(struct usb_device *udev, const struct usbh_msc_iface *msc_opt,
			      uint8_t lun, uint8_t pc, uint8_t page_code, uint8_t subpage,
			      uint8_t alloc_len, bool disable_block_descriptors,
			      const struct device *uhc_host)
{
	struct usbh_msc_iface msc_local;
	const struct usbh_msc_iface *msc;
	uint8_t mode_buf[192];
	struct msc_mode_sense6_ctx ctx;
	int ret;

	if (udev == NULL || alloc_len == 0U || lun > 15U) {
		return -EINVAL;
	}

	if (udev->state != USB_STATE_CONFIGURED) {
		return -ENOTCONN;
	}

	if (msc_opt == NULL) {
		ret = usbh_msc_prepare(udev, &msc_local);
		if (ret != 0) {
			return ret;
		}
		msc = &msc_local;
	} else {
		msc = msc_opt;
	}

	if (lun > msc->max_lun || alloc_len > sizeof(mode_buf)) {
		return -EINVAL;
	}

	ctx.buf = mode_buf;
	ctx.len = alloc_len;
	ctx.pc = pc;
	ctx.page_code = page_code;
	ctx.subpage = subpage;
	ctx.disable_block_descriptors = disable_block_descriptors;

	LOG_INF("msc MODE SENSE(6): LUN%u pc=%u page=0x%02x sub=0x%02x alloc=%u dbd=%d", lun,
		(unsigned int)(pc & 3U), page_code, subpage, (unsigned int)alloc_len,
		disable_block_descriptors ? 1 : 0);

	ret = usb_msc_scsi_with_lun(uhc_host, udev, msc, lun, msc_mode_sense6_fn, &ctx);
	if (ret != 0) {
		return ret;
	}

	LOG_HEXDUMP_DBG(mode_buf, alloc_len, "MODE_SENSE(6) data");
	return 0;
}

struct msc_read10_ctx {
	uint32_t lba;
	uint16_t blocks;
	void *data;
	uint32_t total_len;
};

static int msc_read10_fn(struct scsi_device *sdev, void *arg)
{
	struct msc_read10_ctx *ctx = arg;

	return scsi_read_10(sdev, ctx->lba, ctx->blocks, ctx->data, ctx->total_len);
}

int usbh_msc_scsi_read10(struct usb_device *udev, const struct usbh_msc_iface *msc, uint8_t lun,
			 uint32_t lba, uint16_t blocks, uint32_t block_size,
			 const struct device *uhc_host, void *data)
{
	struct msc_read10_ctx ctx;
	uint32_t total_len;

	if (udev == NULL || msc == NULL || data == NULL || blocks == 0U || block_size == 0U) {
		return -EINVAL;
	}

	if (lun > msc->max_lun) {
		return -EINVAL;
	}

	total_len = (uint32_t)blocks * block_size;
	if (total_len / block_size != (uint32_t)blocks) {
		return -EOVERFLOW;
	}

	ctx.lba = lba;
	ctx.blocks = blocks;
	ctx.data = data;
	ctx.total_len = total_len;

	LOG_DBG("READ10 LBA=%u blocks=%u block_size=%u len=%u", lba, blocks, block_size, total_len);

	return usb_msc_scsi_with_lun(uhc_host, udev, msc, lun, msc_read10_fn, &ctx);
}

struct msc_write10_ctx {
	uint32_t lba;
	uint16_t blocks;
	const void *data;
	uint32_t total_len;
};

static int msc_write10_fn(struct scsi_device *sdev, void *arg)
{
	struct msc_write10_ctx *ctx = arg;

	return scsi_write_10(sdev, ctx->lba, ctx->blocks, ctx->data, ctx->total_len);
}

int usbh_msc_scsi_write10(struct usb_device *udev, const struct usbh_msc_iface *msc, uint8_t lun,
			  uint32_t lba, uint16_t blocks, uint32_t block_size,
			  const struct device *uhc_host, const void *data)
{
	struct msc_write10_ctx ctx;
	uint32_t total_len;

	if (udev == NULL || msc == NULL || data == NULL || blocks == 0U || block_size == 0U) {
		return -EINVAL;
	}

	if (lun > msc->max_lun) {
		return -EINVAL;
	}

	total_len = (uint32_t)blocks * block_size;
	if (total_len / block_size != (uint32_t)blocks) {
		return -EOVERFLOW;
	}

	ctx.lba = lba;
	ctx.blocks = blocks;
	ctx.data = data;
	ctx.total_len = total_len;

	LOG_DBG("WRITE10 LBA=%u blocks=%u block_size=%u len=%u", lba, blocks, block_size,
		total_len);

	return usb_msc_scsi_with_lun(uhc_host, udev, msc, lun, msc_write10_fn, &ctx);
}

struct msc_verify10_ctx {
	uint32_t lba;
	uint16_t blocks;
};

static int msc_verify10_fn(struct scsi_device *sdev, void *arg)
{
	struct msc_verify10_ctx *ctx = arg;

	return scsi_verify_10(sdev, ctx->lba, ctx->blocks);
}

int usbh_msc_scsi_verify10(struct usb_device *udev, const struct usbh_msc_iface *msc, uint8_t lun,
			   uint32_t lba, uint16_t blocks, const struct device *uhc_host)
{
	struct msc_verify10_ctx ctx;

	if (udev == NULL || msc == NULL || blocks == 0U) {
		return -EINVAL;
	}

	if (lun > msc->max_lun) {
		return -EINVAL;
	}

	ctx.lba = lba;
	ctx.blocks = blocks;

	LOG_DBG("VERIFY10 LBA=%u blocks=%u", (unsigned int)lba, (unsigned int)blocks);

	return usb_msc_scsi_with_lun(uhc_host, udev, msc, lun, msc_verify10_fn, &ctx);
}

#if IS_ENABLED(CONFIG_USBH_MSC_BRINGUP_MODE_SENSE6)
static void msc_bringup_log_mode_sense6(struct scsi_device *sdev)
{
	uint8_t mode_buf[192];
	int ms_ret;

	ms_ret = scsi_mode_sense_6_params(sdev, mode_buf, sizeof(mode_buf),
					  USB_SCSI_MODE_SENSE_PC_CURRENT,
					  USB_SCSI_MODE_SENSE_PAGE_ALL, 0U, false);
	USBH_MSC_FLOW_INF("msc_flow: bringup LUN%u MODE SENSE(6) " USBH_MSC_ERR_FMT, sdev->lun,
			  USBH_MSC_ERR_ARG(ms_ret));
	if (ms_ret != 0) {
		LOG_WRN("msc_storage: LUN%u MODE SENSE(6) failed: %d (non-fatal)", sdev->lun,
			ms_ret);
		return;
	}

	LOG_HEXDUMP_DBG(mode_buf, sizeof(mode_buf), "MODE_SENSE(6) data");
}
#endif

struct msc_bringup_probe_ctx {
	uint32_t block_size;
	uint64_t block_count;
	bool removable;
	bool write_protected;
};

static int msc_bringup_probe_fn(struct scsi_device *sdev, void *arg)
{
	struct msc_bringup_probe_ctx *ctx = arg;
	int ret;

	ret = scsi_device_probe(sdev);
	if (ret != 0) {
		return ret;
	}

	ctx->block_size = sdev->block_size;
	ctx->block_count = sdev->block_count;
	ctx->removable = sdev->removable;
	ctx->write_protected = sdev->write_protected;

#if IS_ENABLED(CONFIG_USBH_MSC_BRINGUP_MODE_SENSE6)
	msc_bringup_log_mode_sense6(sdev);
#endif

	return 0;
}

static int msc_bringup_probe_lun(struct usb_device *udev, const struct device *uhc_dev,
				 const struct usbh_msc_iface *msc, uint8_t lun, bool allow_retry)
{
	struct msc_bringup_probe_ctx ctx = {0};
	uint64_t capacity_bytes;
	int ret;
	int retries = (allow_retry && uhc_dev != NULL) ? 1 : 0;

again:
	USBH_MSC_FLOW_INF("msc_flow: bringup LUN%u scsi_device_probe", lun);
	ret = usb_msc_scsi_with_lun(uhc_dev, udev, msc, lun, msc_bringup_probe_fn, &ctx);
	USBH_MSC_FLOW_INF("msc_flow: bringup LUN%u probe " USBH_MSC_ERR_FMT, lun,
			  USBH_MSC_ERR_ARG(ret));
	if (ret != 0 && retries-- > 0) {
		LOG_WRN("msc_storage: LUN%u probe err=%d - BOT reset (retry)", lun, ret);
		usbh_msc_bot_reset(udev, uhc_dev);
		usbh_msc_bot_transport_session_seed();
		goto again;
	}

	if (ret != 0) {
		return ret;
	}

	capacity_bytes = ctx.block_count * (uint64_t)ctx.block_size;
	LOG_INF("msc_storage: LUN%u probe OK last_lba=0x%08x block_len=%u (~%llu MiB) "
		"removable=%u wp=%u",
		lun, (uint32_t)(ctx.block_count - 1U), ctx.block_size,
		(unsigned long long)(capacity_bytes / (1024U * 1024U)), ctx.removable,
		ctx.write_protected);

	return 0;
}

static int scsi_storage_bringup_body(struct usb_device *udev, const struct device *uhc_dev,
				     usbh_msc_lun0_verify_fn lun0_verify)
{
	struct usbh_msc_iface msc;
	int ret;
	bool lun0_ok = false;
	uint8_t lun;

	if (udev == NULL) {
		return -EINVAL;
	}

	usbh_msc_bot_transport_session_seed();

	USBH_MSC_FLOW_INF("msc_flow: bringup start VID=0x%04x PID=0x%04x", udev->dev_desc.idVendor,
			  udev->dev_desc.idProduct);

	ret = usbh_msc_prepare(udev, &msc);
	if (ret != 0) {
		LOG_ERR("msc_storage: usbh_msc_prepare " USBH_MSC_ERR_FMT, USBH_MSC_ERR_ARG(ret));
		return ret;
	}

	USBH_MSC_FLOW_INF("msc_flow: bringup MSC prepare OK bulk IN=0x%02x OUT=0x%02x max_lun=%u",
			  msc.ep_in_addr, msc.ep_out_addr, msc.max_lun);

	if (uhc_dev != NULL && msc.bulk_only) {
		usbh_msc_bot_reset(udev, uhc_dev);
		usbh_msc_bot_transport_session_seed();
	}

	LOG_INF("msc_storage: LUN(s)=%u max_lun=%u", (unsigned int)msc.max_lun + 1U, msc.max_lun);

	for (lun = 0U; lun <= msc.max_lun; lun++) {
		if (lun == 0U && lun0_verify != NULL) {
			USBH_MSC_FLOW_INF("msc_flow: bringup LUN0 via verify hook");
			ret = lun0_verify(udev, &msc, uhc_dev);
			USBH_MSC_FLOW_INF("msc_flow: bringup LUN0 verify " USBH_MSC_ERR_FMT,
					  USBH_MSC_ERR_ARG(ret));
			if (ret != 0) {
				return ret;
			}
			lun0_ok = true;
			continue;
		}

#if IS_ENABLED(CONFIG_USBH_MSC_DISK) && IS_ENABLED(CONFIG_USBH_MSC_DISK_AUTO_ATTACH)
		if (lun == 0U || IS_ENABLED(CONFIG_USBH_MSC_DISK_ATTACH_ALL_LUNS)) {
			USBH_MSC_FLOW_INF("msc_flow: bringup LUN%u scsi_disk attach", lun);
			ret = usb_msc_disk_attach_lun(uhc_dev, udev, &msc, lun);
			USBH_MSC_FLOW_INF("msc_flow: bringup LUN%u attach " USBH_MSC_ERR_FMT, lun,
					  USBH_MSC_ERR_ARG(ret));
			if (ret != 0) {
				if (lun == 0U) {
					return ret;
				}
				LOG_WRN("msc_storage: LUN%u scsi_disk attach failed: %d (skip)",
					lun, ret);
				continue;
			}
			if (lun == 0U) {
				lun0_ok = true;
			}
			continue;
		}
#endif

		ret = msc_bringup_probe_lun(udev, uhc_dev, &msc, lun, lun == 0U);
		if (ret != 0) {
			if (lun == 0U) {
				return ret;
			}
			LOG_WRN("msc_storage: LUN%u probe failed: %d (skip)", lun, ret);
			continue;
		}

		if (lun == 0U) {
			lun0_ok = true;
		}
	}

	if (!lun0_ok) {
		USBH_MSC_FLOW_INF("msc_flow: bringup failed — no LUN0 capacity");
		return -EIO;
	}

	USBH_MSC_FLOW_INF("msc_flow: bringup complete OK");
	LOG_INF("msc_storage: ok (GET_MAX_LUN, mid-layer probe per LUN)");
	return 0;
}

int usbh_msc_storage_bringup(struct usb_device *udev, const struct device *uhc_dev,
			     usbh_msc_lun0_verify_fn lun0_verify)
{
	return scsi_storage_bringup_body(udev, uhc_dev, lun0_verify);
}
