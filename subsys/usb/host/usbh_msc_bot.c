/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/usb/usbh_hcd.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usbh_msc.h>
#include <zephyr/usb/usbh_msc_bot.h>
#include <zephyr/usb/usbh_msc_scsi_cmd.h>

#include "usbh_ch9.h"
#include "usbh_device.h"
#include "usbh_msc_bot_transport.h"
#include "usbh_msc_flow.h"

LOG_MODULE_REGISTER(usbh_msc_bot, LOG_LEVEL_INF);

#define MSC_BOT_RESET_STEP_MS              150U
#define MSC_BBB_STALE_CSW_FULL_CBW_RETRIES 3U

void usbh_msc_bot_reset(struct usb_device *udev, const struct device *uhc_dev);

static uint32_t g_msc_bot_tag = USB_MSC_BOT_CBW_INQUIRY_TAG_LE;
static bool g_msc_bbb_usb_ready;
static uint32_t g_msc_bot_cmd_timeout_ms;

static uint32_t msc_bot_effective_timeout_ms(void)
{
	if (g_msc_bot_cmd_timeout_ms != 0U) {
		return g_msc_bot_cmd_timeout_ms;
	}

	return CONFIG_USBH_MSC_BOT_CBW_TRANSFER_TIMEOUT_MS;
}

void usbh_msc_bot_set_command_timeout_ms(uint32_t timeout_ms)
{
	g_msc_bot_cmd_timeout_ms = timeout_ms;
}

struct msc_cbw_bulk_sync {
	struct k_sem sem;
	int err;
};

struct msc_bulk_in_sync {
	struct k_sem sem;
	int err;
	struct net_buf *buf;
};

struct msc_bulk_out_sync {
	struct k_sem sem;
	int err;
};

static void msc_bbb_discard_data_in(struct usb_device *udev, struct net_buf **slot)
{
	if (slot == NULL || *slot == NULL) {
		return;
	}
	usbh_xfer_buf_free(udev, *slot);
	*slot = NULL;
}

static int msc_bot_bulk_in_one(struct usb_device *udev, uint8_t ep_in, size_t nbytes,
			       const char *phase, struct net_buf **out_buf);

static int msc_bulk_in_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct msc_bulk_in_sync *const s = xfer->priv;

	s->err = xfer->err;
	s->buf = xfer->buf;
	xfer->buf = NULL;

	if (xfer->err == 0) {
		LOG_DBG("msc_bot_in: complete len=%u",
			s->buf != NULL ? (unsigned int)s->buf->len : 0U);
	} else if (xfer->err == -EPIPE) {
		/* Protocol STALL; class layer CLEAR_FEATURE(HALT) — not HCD fault */
		LOG_DBG("msc_bot_in: STALL (-EPIPE) phase=device protocol");
	} else if (IS_ENABLED(CONFIG_USBH_MSC_FLOW_LOG)) {
		LOG_INF("msc_flow: bulk IN completion " USBH_MSC_ERR_FMT,
			USBH_MSC_ERR_ARG(xfer->err));
	} else {
		LOG_WRN("msc_bot_in: err=%d (e.g. -EIO controller COMP≠success, -EPROTO short vs "
			"TD len)",
			xfer->err);
	}

	usbh_xfer_free(udev, xfer);
	k_sem_give(&s->sem);

	return 0;
}

/**
 * After bulk IN protocol STALL (host completion @c -EPIPE): USB 2.0 §9.4.5 CLEAR_FEATURE of
 * ENDPOINT_HALT on the stalled bulk-IN endpoint, then optional UHC bulk ring sync.
 *
 * BOT / MSC class: CSW-phase STALL is non-fatal; clear HALT then retry STATUS once.
 * Integrated DWC3+xHCI needs @ref usbh_ep_sync_after_clear_feature after HALT clear
 * (same pattern as @ref usbh_msc_bot_reset); the HCD must not Reset EP before this runs.
 */
static int msc_bot_recover_bulk_in_protocol_stall(struct usb_device *udev, uint8_t ep_in,
						  uint8_t ep_out, const struct device *uhc_dev,
						  const char *phase, const char *ctx)
{
	int r;

	USBH_MSC_FLOW_INF("msc_flow: %s recover STALL (%s): CLEAR_FEATURE(HALT) IN ep=0x%02x",
			  phase, ctx, ep_in);

	r = usbh_req_clear_sfs_halt(udev, ep_in);
	if (r != 0) {
		LOG_ERR("msc_bot_recover(%s): CLEAR_FEATURE(HALT) ep=0x%02x (%s) " USBH_MSC_ERR_FMT,
			phase, ep_in, ctx, USBH_MSC_ERR_ARG(r));
		return r;
	}

	if (ep_out != 0U) {
		USBH_MSC_FLOW_INF(
			"msc_flow: %s recover STALL (%s): CLEAR_FEATURE(HALT) OUT ep=0x%02x", phase,
			ctx, ep_out);
		r = usbh_req_clear_sfs_halt(udev, ep_out);

		if (r != 0) {
			LOG_WRN("msc_bot_recover(%s): CLEAR_FEATURE(HALT) bulk OUT "
				"ep=0x%02x " USBH_MSC_ERR_FMT,
				phase, ep_out, USBH_MSC_ERR_ARG(r));
		}
	}

	LOG_INF("msc_bot_recover(%s): bulk IN ep=0x%02x HALT cleared (%s)", phase, ep_in, ctx);

	if (uhc_dev != NULL) {
		USBH_MSC_FLOW_INF("msc_flow: %s recover STALL (%s): UHC bulk sync", phase, ctx);
		r = usbh_ep_sync_after_clear_feature(udev);
		if (r != 0) {
			LOG_WRN("msc_bot_recover(%s): UHC bulk sync after HALT "
				"clear " USBH_MSC_ERR_FMT,
				phase, USBH_MSC_ERR_ARG(r));
			return r;
		}

		r = usbh_eps_verify_steady(udev);
		USBH_MSC_FLOW_INF(
			"msc_flow: %s recover STALL (%s): bulk verify steady " USBH_MSC_ERR_FMT,
			phase, ctx, USBH_MSC_ERR_ARG(r));
		if (r != 0) {
			LOG_WRN("msc_bot_recover(%s): bulk not steady after sync — BOT reset",
				phase);
			usbh_msc_bot_reset(udev, uhc_dev);
			r = usbh_eps_verify_steady(udev);
			USBH_MSC_FLOW_INF("msc_flow: %s recover STALL (%s): post BOT reset "
					  "verify " USBH_MSC_ERR_FMT,
					  phase, ctx, USBH_MSC_ERR_ARG(r));
			if (r != 0) {
				LOG_WRN("msc_bot_recover(%s): bulk still not steady after BOT "
					"reset " USBH_MSC_ERR_FMT,
					phase, USBH_MSC_ERR_ARG(r));
				return r;
			}
		}
	}

	USBH_MSC_FLOW_INF("msc_flow: %s recover STALL (%s): done OK", phase, ctx);
	return 0;
}

/**
 * One bulk IN (device → host). On success @p *out_buf is set; caller must usbh_xfer_buf_free.
 */
static int msc_bot_bulk_in_one(struct usb_device *udev, uint8_t ep_in, size_t nbytes,
			       const char *phase, struct net_buf **out_buf)
{
	struct msc_bulk_in_sync sync;
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	*out_buf = NULL;

	k_sem_init(&sync.sem, 0, 1);
	sync.err = -EIO;
	sync.buf = NULL;

	xfer = usbh_xfer_alloc(udev, ep_in, msc_bulk_in_cb, &sync);
	if (xfer == NULL) {
		LOG_ERR("msc_bot_in(%s): usbh_xfer_alloc failed", phase);
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(udev, nbytes);
	if (buf == NULL) {
		usbh_xfer_free(udev, xfer);
		LOG_ERR("msc_bot_in(%s): usbh_xfer_buf_alloc failed", phase);
		return -ENOMEM;
	}

	/*
	 * Bulk IN: keep buf->len == 0 before enqueue. UHC uses max usable transfer length;
	 * completion does net_buf_add(received).
	 */

	ret = usbh_xfer_buf_add(udev, xfer, buf);
	if (ret != 0) {
		usbh_xfer_buf_free(udev, buf);
		usbh_xfer_free(udev, xfer);
		LOG_ERR("msc_bot_in(%s): usbh_xfer_buf_add err=%d", phase, ret);
		return ret;
	}

	LOG_DBG("msc_bot_in(%s): queue ep=0x%02x len=%u", phase, ep_in, (unsigned int)nbytes);

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		usbh_xfer_buf_free(udev, buf);
		usbh_xfer_free(udev, xfer);
		LOG_ERR("msc_bot_in(%s): usbh_xfer_enqueue err=%d", phase, ret);
		return ret;
	}

	LOG_DBG("msc_bot_in(%s): waiting (timeout %u ms)", phase,
		(unsigned int)msc_bot_effective_timeout_ms());

	if (k_sem_take(&sync.sem, K_MSEC(msc_bot_effective_timeout_ms())) != 0) {
		(void)usbh_xfer_dequeue(udev, xfer);
		if (sync.buf != NULL) {
			usbh_xfer_buf_free(udev, sync.buf);
		}
		LOG_ERR("msc_bot_in(%s): timeout (no completion)", phase);
		LOG_WRN("msc_bot_in(%s): bulk IN timeout - device often not completing data "
			"(NAK/STALL/medium); shorter BOT DATA IN often succeeds",
			phase);
		return -ETIMEDOUT;
	}

	if (sync.err != 0) {
		if (sync.buf != NULL) {
			usbh_xfer_buf_free(udev, sync.buf);
		}
		USBH_MSC_FLOW_INF("msc_flow: %s bulk IN " USBH_MSC_ERR_FMT, phase,
				  USBH_MSC_ERR_ARG(sync.err));
		return sync.err;
	}

	*out_buf = sync.buf;
	return 0;
}

/*
 * After DATA phase failure the bulk IN endpoint is often HALTED/STOPPED. Do not
 * queue another bulk IN for CSW (Versal: COMP=3 BABBLE). Run BOT reset instead.
 */
static void msc_bbb_recover_after_data_fail(struct usb_device *udev, const struct device *uhc_host,
					    const char *phase)
{
	if (!IS_ENABLED(CONFIG_USBH_MSC_BOT_TRANSPORT_RECOVER_ON_XFER_ERR)) {
		return;
	}

	if (uhc_host == NULL) {
		LOG_WRN("msc_bbb(%s): DATA fail — no uhc_host for BOT reset", phase);
		return;
	}

	LOG_WRN("msc_bbb(%s): DATA fail — BOT transport reset", phase);
	usbh_msc_bot_reset(udev, uhc_host);
	k_msleep(MSC_BOT_RESET_STEP_MS);
}

static int msc_bulk_out_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct msc_bulk_out_sync *const s = xfer->priv;

	s->err = xfer->err;

	if (xfer->err == 0) {
		LOG_DBG("msc_bot_out: OK len=%u",
			xfer->buf != NULL ? (unsigned int)xfer->buf->len : 0U);
	} else {
		LOG_WRN("msc_bot_out: err=%d", xfer->err);
	}

	usbh_xfer_buf_free(udev, xfer->buf);
	usbh_xfer_free(udev, xfer);
	k_sem_give(&s->sem);

	return 0;
}

/**
 * One bulk OUT (host → device). @p payload must remain valid until completion.
 */
static int msc_bot_bulk_out_one(struct usb_device *udev, uint8_t ep_out, const void *payload,
				size_t nbytes, const char *phase)
{
	struct msc_bulk_out_sync sync;
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	k_sem_init(&sync.sem, 0, 1);
	sync.err = -EIO;

	xfer = usbh_xfer_alloc(udev, ep_out, msc_bulk_out_cb, &sync);
	if (xfer == NULL) {
		LOG_ERR("msc_bot_out(%s): usbh_xfer_alloc failed", phase);
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(udev, nbytes);
	if (buf == NULL) {
		usbh_xfer_free(udev, xfer);
		LOG_ERR("msc_bot_out(%s): usbh_xfer_buf_alloc failed", phase);
		return -ENOMEM;
	}

	(void)memcpy(net_buf_add(buf, nbytes), payload, nbytes);

	ret = usbh_xfer_buf_add(udev, xfer, buf);
	if (ret != 0) {
		usbh_xfer_buf_free(udev, buf);
		usbh_xfer_free(udev, xfer);
		LOG_ERR("msc_bot_out(%s): usbh_xfer_buf_add err=%d", phase, ret);
		return ret;
	}

	LOG_DBG("msc_bot_out(%s): queue ep=0x%02x len=%u", phase, ep_out, (unsigned int)nbytes);

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		usbh_xfer_buf_free(udev, buf);
		usbh_xfer_free(udev, xfer);
		LOG_ERR("msc_bot_out(%s): usbh_xfer_enqueue err=%d", phase, ret);
		return ret;
	}

	if (k_sem_take(&sync.sem, K_MSEC(msc_bot_effective_timeout_ms())) != 0) {
		(void)usbh_xfer_dequeue(udev, xfer);
		LOG_ERR("msc_bot_out(%s): timeout (no completion)", phase);
		return -ETIMEDOUT;
	}

	return sync.err;
}

static int msc_cbw_bulk_out_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct msc_cbw_bulk_sync *const s = xfer->priv;

	s->err = xfer->err;

	if (xfer->err == 0) {
		LOG_DBG("msc_cbw_out: complete err=0");
	} else if (xfer->err == -ETIMEDOUT) {
		LOG_ERR("msc_cbw_out: timeout (no transfer completion)");
	} else if (xfer->err == -EPIPE) {
		LOG_WRN("msc_cbw_out: stall (-EPIPE)");
	} else if (xfer->err == -ECONNRESET) {
		LOG_DBG("msc_cbw_out: cancelled (ECONNRESET)");
	} else {
		LOG_WRN("msc_cbw_out: complete err=%d", xfer->err);
	}

	usbh_xfer_buf_free(udev, xfer->buf);
	usbh_xfer_free(udev, xfer);
	k_sem_give(&s->sem);

	return 0;
}

static void msc_dbg_log_cbw_fields(const struct usb_msc_bot_cbw *cbw, const char *phase)
{
	const uint8_t fl = cbw->bmCBWFlags;

	LOG_DBG("msc CBW[%s]: sig=0x%08x tag=0x%08x d_xfer_len=%u bmCBWFlags=0x%02x "
		"bit7_data_IN=%u low7=0x%02x bCBWLUN=0x%02x lun_nibble=%u bCBWCBLength=%u "
		"CDB[0..9]=%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
		phase, sys_le32_to_cpu(cbw->dCBWSignature), sys_le32_to_cpu(cbw->dCBWTag),
		sys_le32_to_cpu(cbw->dCBWDataTransferLength), fl,
		(unsigned int)((fl & USB_MSC_BOT_CBW_FLAG_DATA_IN) != 0U ? 1U : 0U),
		(unsigned int)(fl & 0x7fU), cbw->bCBWLUN, (unsigned int)(cbw->bCBWLUN & 0x0fU),
		(unsigned int)cbw->bCBWCBLength, cbw->CBWCB[0], cbw->CBWCB[1], cbw->CBWCB[2],
		cbw->CBWCB[3], cbw->CBWCB[4], cbw->CBWCB[5], cbw->CBWCB[6], cbw->CBWCB[7],
		cbw->CBWCB[8], cbw->CBWCB[9]);
}

static void msc_log_cbw_cdb_decode(const uint8_t *cdb, uint8_t cblen, char *out, size_t out_len)
{
	uint32_t lba;
	uint16_t blocks;

	if (out_len == 0U) {
		return;
	}

	out[0] = '\0';

	if (cblen < 1U) {
		(void)snprintk(out, out_len, "CDB empty");
		return;
	}

	switch (cdb[0]) {
	case USB_SCSI_READ10:
	case USB_SCSI_WRITE10:
		if (cblen >= 10U) {
			lba = sys_get_be32(&cdb[2]);
			blocks = sys_get_be16(&cdb[7]);
			(void)snprintk(out, out_len,
				       "%s LBA=0x%08x (%u) blocks=%u CDB10=%02x %02x %02x %02x "
				       "%02x %02x %02x %02x %02x %02x",
				       cdb[0] == USB_SCSI_READ10 ? "READ10" : "WRITE10", lba, lba,
				       (unsigned int)blocks, cdb[0], cdb[1], cdb[2], cdb[3], cdb[4],
				       cdb[5], cdb[6], cdb[7], cdb[8], cdb[9]);
		} else {
			(void)snprintk(out, out_len, "opcode=0x%02x short CDB len=%u", cdb[0],
				       (unsigned int)cblen);
		}
		break;
	case SCSI_OPCODE_READ_CAPACITY_10:
		if (cblen >= 10U) {
			lba = sys_get_be32(&cdb[2]);
			(void)snprintk(out, out_len,
				       "READ_CAPACITY(10) partial_lba=0x%08x CDB10=%02x %02x %02x "
				       "%02x %02x %02x %02x %02x %02x %02x",
				       lba, cdb[0], cdb[1], cdb[2], cdb[3], cdb[4], cdb[5], cdb[6],
				       cdb[7], cdb[8], cdb[9]);
		} else {
			(void)snprintk(out, out_len, "READ_CAPACITY(10) short CDB len=%u",
				       (unsigned int)cblen);
		}
		break;
	default:
		(void)snprintk(out, out_len,
			       "opcode=0x%02x CDB[%u]=%02x %02x %02x %02x %02x %02x "
			       "%02x %02x %02x %02x",
			       cdb[0], (unsigned int)MIN((unsigned int)cblen, 10U), cdb[0],
			       cblen > 1U ? cdb[1] : 0U, cblen > 2U ? cdb[2] : 0U,
			       cblen > 3U ? cdb[3] : 0U, cblen > 4U ? cdb[4] : 0U,
			       cblen > 5U ? cdb[5] : 0U, cblen > 6U ? cdb[6] : 0U,
			       cblen > 7U ? cdb[7] : 0U, cblen > 8U ? cdb[8] : 0U,
			       cblen > 9U ? cdb[9] : 0U);
		break;
	}
}

static void msc_log_cbw_out(const struct usb_msc_bot_cbw *cbw, const char *phase)
{
	msc_dbg_log_cbw_fields(cbw, phase);

	if (!IS_ENABLED(CONFIG_USBH_MSC_BOT_CBW_CDB_LOG)) {
		return;
	}

	const uint8_t fl = cbw->bmCBWFlags;
	const uint8_t cblen = cbw->bCBWCBLength;
	char cdb_dec[128];

	msc_log_cbw_cdb_decode(cbw->CBWCB, cblen, cdb_dec, sizeof(cdb_dec));

	LOG_INF("msc CBW[%s]: tag=0x%08x d_xfer_len=%u data_IN=%u lun=%u cblen=%u | %s", phase,
		sys_le32_to_cpu(cbw->dCBWTag), sys_le32_to_cpu(cbw->dCBWDataTransferLength),
		(unsigned int)((fl & USB_MSC_BOT_CBW_FLAG_DATA_IN) != 0U ? 1U : 0U),
		(unsigned int)(cbw->bCBWLUN & 0x0fU), (unsigned int)cblen, cdb_dec);
	LOG_HEXDUMP_INF(cbw, sizeof(*cbw), "msc CBW wire");
}

static void msc_dbg_log_csw_fields(const uint8_t *p, const char *phase)
{
	const uint32_t sig = sys_get_le32(&p[0]);
	const uint32_t csw_tag = sys_get_le32(&p[4]);
	const uint32_t residue = sys_get_le32(&p[8]);
	const uint8_t status = p[12];

	LOG_DBG("msc CSW[%s]: sig=0x%08x tag=0x%08x residue=%u bCSWStatus=0x%02x "
		"(00=GOOD 01=FAIL 02=PHASE_ERR)",
		phase, sig, csw_tag, residue, status);
}

/* MSC BOT §5.1 / §5.2 - CBW must be well-formed before the COMMAND transfer. */
static int msc_bbb_precheck_cbw(const struct usb_msc_bot_cbw *cbw, uint32_t cmd_tag, bool data_in,
				uint32_t data_len, const char *phase)
{
	const uint32_t sig = sys_le32_to_cpu(cbw->dCBWSignature);
	const uint32_t tag = sys_le32_to_cpu(cbw->dCBWTag);
	const uint32_t dcbw_len = sys_le32_to_cpu(cbw->dCBWDataTransferLength);
	const uint8_t fl = cbw->bmCBWFlags;
	const uint8_t cblen = cbw->bCBWCBLength;

	if (sig != USB_MSC_BOT_CBW_SIGNATURE) {
		LOG_ERR("msc_bbb_precheck(%s): bad dCBWSignature 0x%08x", phase, sig);
		return -EINVAL;
	}
	if (tag != cmd_tag) {
		LOG_ERR("msc_bbb_precheck(%s): dCBWTag mismatch - expect=0x%08x CBW=0x%08x", phase,
			cmd_tag, tag);
		return -EINVAL;
	}
	if (dcbw_len != data_len) {
		LOG_ERR("msc_bbb_precheck(%s): dCBWDataTransferLength=%u != data_len=%u (DATA must "
			"match CBW)",
			phase, dcbw_len, data_len);
		return -EINVAL;
	}
	if (cblen < 1U || cblen > 16U) {
		LOG_ERR("msc_bbb_precheck(%s): bCBWCBLength=%u (BOT allows 1..16)", phase, cblen);
		return -EINVAL;
	}
	if (data_len != 0U) {
		if (data_in && (fl & USB_MSC_BOT_CBW_FLAG_DATA_IN) == 0U) {
			LOG_ERR("msc_bbb_precheck(%s): DATA-IN phase but bmCBWFlags bit7 clear",
				phase);
			return -EINVAL;
		}
		if (!data_in && (fl & USB_MSC_BOT_CBW_FLAG_DATA_IN) != 0U) {
			LOG_ERR("msc_bbb_precheck(%s): DATA-OUT phase but bmCBWFlags bit7 set",
				phase);
			return -EINVAL;
		}
	} else if ((fl & USB_MSC_BOT_CBW_FLAG_DATA_IN) != 0U) {
		LOG_ERR("msc_bbb_precheck(%s): dCBWDataTransferLength=0 but Data-In flag set",
			phase);
		return -EINVAL;
	}

	LOG_DBG("msc_bbb_precheck(%s): OK tag=0x%08x dXferLen=%u data_in=%u", phase, cmd_tag,
		data_len, data_in ? 1U : 0U);
	return 0;
}

/*
 * CSW checks (BOT §5.2): signature, dCSWTag, bCSWStatus; for GOOD, dCSWDataResidue must equal
 * (dCBWDataTransferLength - actual DATA bytes). Command failed (01): valid CSW only -
 * return USBH_MSC_BOT_ERR_CSW_COMMAND_FAILED for REQUEST SENSE (not on HCD errors).
 */
static int msc_bbb_validate_csw(struct net_buf *buf, uint32_t expect_tag, uint32_t data_len,
				uint32_t data_actlen, const char *phase)
{
	const uint8_t *p;
	uint32_t sig;
	uint32_t csw_tag;
	uint32_t residue;
	uint8_t status;
	const uint32_t exp_residue_good = data_len - data_actlen;

	if (buf->len < sizeof(struct usb_msc_bot_csw)) {
		LOG_ERR("msc_csw(%s): short len=%u (need %u)", phase, (unsigned int)buf->len,
			(unsigned int)sizeof(struct usb_msc_bot_csw));
		return -EIO;
	}

	p = buf->data;
	sig = sys_get_le32(&p[0]);
	csw_tag = sys_get_le32(&p[4]);
	residue = sys_get_le32(&p[8]);
	status = p[12];

	LOG_DBG("msc_csw(%s): tag exp=0x%08x got=0x%08x d_len=%u act=%u residue=%u status=0x%02x",
		phase, expect_tag, csw_tag, data_len, data_actlen, residue, status);
	msc_dbg_log_csw_fields(p, phase);

	if (sig != USB_MSC_BOT_CSW_SIGNATURE) {
		LOG_ERR("msc_csw(%s): bad dCSWSignature 0x%08x", phase, sig);
		return -EIO;
	}

	if (csw_tag != expect_tag) {
		LOG_ERR("msc_csw(%s): dCSWTag mismatch - expect=0x%08x CSW=0x%08x", phase,
			expect_tag, csw_tag);
		return -EIO;
	}

	if (status > USB_MSC_BOT_CSW_STATUS_PHASE_ERROR) {
		LOG_ERR("msc_csw(%s): illegal bCSWStatus0x%02x", phase, status);
		return -EIO;
	}

	if (data_actlen > data_len) {
		LOG_ERR("msc_csw(%s): DATA actual %u B > dCBWDataTransferLength %u B", phase,
			(unsigned int)data_actlen, data_len);
		return -EIO;
	}

	if (status == USB_MSC_BOT_CSW_STATUS_PHASE_ERROR) {
		LOG_ERR("msc_csw(%s): PHASE_ERR (02) - CBW_tag=0x%08x CSW_tag=0x%08x (BOT reset "
			"recovery)",
			phase, expect_tag, csw_tag);
		return USBH_MSC_BOT_ERR_CSW_PHASE_ERROR;
	}

	if (status == USB_MSC_BOT_CSW_STATUS_FAILED) {
		if (residue != exp_residue_good) {
			LOG_WRN("msc_csw(%s): CSW=FAILED (01) residue=%u (strict GOOD would be %u "
				"for "
				"data_len=%u act=%u)",
				phase, residue, exp_residue_good, data_len, data_actlen);
		} else {
			LOG_DBG("msc_csw(%s): FAILED (01) residue=%u", phase, residue);
		}
		LOG_WRN("msc_csw(%s): command failed (REQUEST SENSE path)", phase);
		return USBH_MSC_BOT_ERR_CSW_COMMAND_FAILED;
	}

	if (status != USB_MSC_BOT_CSW_STATUS_GOOD) {
		LOG_ERR("msc_csw(%s): unexpected bCSWStatus 0x%02x", phase, status);
		return -EIO;
	}

	if (residue != exp_residue_good) {
		LOG_ERR("msc_csw(%s): residue=%u BOT requires %u for GOOD (d_len=%u act=%u)", phase,
			residue, exp_residue_good, data_len, data_actlen);
		return -EIO;
	}

	LOG_DBG("msc_csw(%s): good tag=0x%08x residue=%u", phase, csw_tag, residue);
	return 0;
}

/*
 * Bulk-Only Transport: COMMAND (CBW OUT), DATA (one bulk IN or OUT), STATUS (CSW IN).
 *
 * Strict phase sequence (BOT — no overlap): CBW (bulk OUT) → optional DATA (one bulk IN or OUT)
 * → CSW (bulk IN). After DATA completes — including short final packet (xHCI COMP_SHORT_PACKET) —
 * only one CSW bulk IN is issued for this CBW; no second DATA transfer.
 *
 * DATA-IN shorter than dCBWDataTransferLength is valid when CSW residue matches (BOT §5.2), e.g.
 * MODE SENSE; fixed-length commands need residue == d_len − actual. Overrun vs CBW is −EIO.
 * Mis-framed DATA surfaces as PHASE_ERR / residue mismatch.
 *
 * −EPIPE from @c msc_bot_bulk_in_one is handled as CSW STALL recovery only in the STATUS loop below
 * (logger phase @c "CSW"): CLEAR_FEATURE(HALT) only (no @ref usbh_ep_sync_after_clear_feature;
 * see @ref msc_bot_recover_bulk_in_protocol_stall), retry CSW once; second CSW STALL →
 * @ref usbh_msc_bot_reset (class reset §5.3.4 + clear HALT IN/OUT + UHC bulk sync when @a uhc_host)
 * + full CBW re-issue (same tag), bounded by MSC_BBB_STALE_CSW_FULL_CBW_RETRIES.
 * DATA-phase IN stall: clear HALT, discard DATA, actlen 0, then STATUS.
 * DATA-IN timeout: full CBW retry budget as below.
 */
static int msc_bbb_command_with_cbw(struct usb_device *udev, const struct usbh_msc_iface *msc,
				    const struct usb_msc_bot_cbw *cbw, uint32_t cmd_tag,
				    bool data_in, uint32_t data_len,
				    const uint8_t *data_out_payload, const char *phase,
				    struct net_buf **data_in_out, const struct device *uhc_host)
{
	struct msc_cbw_bulk_sync sync;
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	struct net_buf *in_csw = NULL;
	uint32_t data_actlen = 0U;
	unsigned int stale_full_cbw_retries = 0U;
	unsigned int csw_stall_retries;
	int ret;

	if (data_in_out != NULL) {
		*data_in_out = NULL;
	}

	if (udev == NULL || msc == NULL || cbw == NULL || phase == NULL) {
		return -EINVAL;
	}

	ret = msc_bbb_precheck_cbw(cbw, cmd_tag, data_in, data_len, phase);
	if (ret != 0) {
		return ret;
	}

	if (data_len != 0U) {
		if (data_in) {
			if (data_in_out == NULL) {
				LOG_ERR("msc_bbb(%s): data-in but data_in_out is NULL", phase);
				return -EINVAL;
			}
		} else if (data_out_payload == NULL) {
			LOG_ERR("msc_bbb(%s): data-out but payload is NULL", phase);
			return -EINVAL;
		}
	}

retry_full_cbw:
	csw_stall_retries = 0;
	k_sem_init(&sync.sem, 0, 1);
	sync.err = -EIO;

	USBH_MSC_FLOW_INF("msc_flow: %s BOT start tag=0x%08x data_in=%d data_len=%u cbw_try=%u",
			  phase, cmd_tag, data_in ? 1 : 0, data_len, stale_full_cbw_retries);

	if (stale_full_cbw_retries > 0U) {
		LOG_DBG("msc_bbb(%s): full CBW re-issue try=%u/%u tag=0x%08x", phase,
			stale_full_cbw_retries, (unsigned int)MSC_BBB_STALE_CSW_FULL_CBW_RETRIES,
			cmd_tag);
	}

	LOG_DBG("msc_bbb(%s): CBW OUT ep=0x%02x len=%u tag=0x%08x data_len=%u", phase,
		msc->ep_out_addr, (unsigned int)sizeof(*cbw), cmd_tag, data_len);
	msc_log_cbw_out(cbw, phase);
	if (!IS_ENABLED(CONFIG_USBH_MSC_BOT_CBW_CDB_LOG)) {
		if (strcmp(phase, "READ10") == 0) {
			LOG_HEXDUMP_DBG(cbw, sizeof(*cbw), "READ10 CBW");
		} else if (strcmp(phase, "WRITE10") == 0) {
			LOG_HEXDUMP_DBG(cbw, sizeof(*cbw), "WRITE10 CBW");
		}
	}

	xfer = usbh_xfer_alloc(udev, msc->ep_out_addr, msc_cbw_bulk_out_cb, &sync);
	if (xfer == NULL) {
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(udev, sizeof(*cbw));
	if (buf == NULL) {
		usbh_xfer_free(udev, xfer);
		return -ENOMEM;
	}

	(void)memcpy(net_buf_add(buf, sizeof(*cbw)), cbw, sizeof(*cbw));

	ret = usbh_xfer_buf_add(udev, xfer, buf);
	if (ret != 0) {
		usbh_xfer_buf_free(udev, buf);
		usbh_xfer_free(udev, xfer);
		return ret;
	}

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		usbh_xfer_buf_free(udev, buf);
		usbh_xfer_free(udev, xfer);
		return ret;
	}

	if (k_sem_take(&sync.sem, K_MSEC(msc_bot_effective_timeout_ms())) != 0) {
		(void)usbh_xfer_dequeue(udev, xfer);
		LOG_ERR("msc_bbb(%s): CBW timeout", phase);
		return -ETIMEDOUT;
	}

	if (sync.err != 0) {
		USBH_MSC_FLOW_INF("msc_flow: %s CBW OUT " USBH_MSC_ERR_FMT, phase,
				  USBH_MSC_ERR_ARG(sync.err));
		return sync.err;
	}

	USBH_MSC_FLOW_INF("msc_flow: %s CBW OUT OK", phase);

	if (USBH_MSC_BOT_POST_CBW_DELAY_US > 0U) {
		k_usleep(USBH_MSC_BOT_POST_CBW_DELAY_US);
	}

	/* DATA phase */
	if (data_len != 0U) {
		USBH_MSC_FLOW_INF("msc_flow: %s DATA %s len=%u", phase, data_in ? "IN" : "OUT",
				  data_len);
		if (data_in) {
			ret = msc_bot_bulk_in_one(udev, msc->ep_in_addr, data_len, phase,
						  data_in_out);
			if (ret == -EPIPE) {
				LOG_WRN("msc_bbb(%s): DATA: stall - CLEAR_FEATURE(HALT) bulk IN",
					phase);
				ret = msc_bot_recover_bulk_in_protocol_stall(
					udev, msc->ep_in_addr, msc->ep_out_addr, uhc_host, phase,
					"DATA stall bulk IN");
				if (ret != 0) {
					LOG_ERR("msc_bbb(%s): clear HALT bulk IN after DATA stall "
						"err=%d",
						phase, ret);
					msc_bbb_discard_data_in(udev, data_in_out);
					return ret;
				}
				msc_bbb_discard_data_in(udev, data_in_out);
				data_actlen = 0U;
			} else if (ret == -ETIMEDOUT && data_len != 0U) {
				msc_bbb_discard_data_in(udev, data_in_out);
				msc_bbb_recover_after_data_fail(udev, uhc_host, phase);
				return ret;
			} else if (ret != 0) {
				msc_bbb_discard_data_in(udev, data_in_out);
				if (data_in && (ret == -EIO || ret == -ETIMEDOUT)) {
					msc_bbb_recover_after_data_fail(udev, uhc_host, phase);
				}
				return ret;
			} else {
				if (data_in_out == NULL || *data_in_out == NULL) {
					return -EIO;
				}
				data_actlen = (uint32_t)(*data_in_out)->len;
				if (data_actlen > data_len) {
					LOG_ERR("msc_bbb(%s): DATA-IN overrun - got %u B, max %u B "
						"(CBW xfer len)",
						phase, data_actlen, data_len);
					msc_bbb_discard_data_in(udev, data_in_out);
					return -EIO;
				}

				LOG_DBG("msc_bbb(%s): DATA-IN bytes=%u (CBW xfer max=%u)", phase,
					data_actlen, data_len);
				USBH_MSC_FLOW_INF("msc_flow: %s DATA IN OK bytes=%u", phase,
						  data_actlen);
			}
		} else {
			ret = msc_bot_bulk_out_one(udev, msc->ep_out_addr, data_out_payload,
						   data_len, phase);
			if (ret == -EPIPE) {
				LOG_WRN("msc_bbb(%s): DATA: stall - CLEAR_FEATURE(HALT) bulk OUT",
					phase);
				ret = usbh_req_clear_sfs_halt(udev, msc->ep_out_addr);
				if (ret != 0) {
					LOG_ERR("msc_bbb(%s): clear HALT bulk OUT after DATA stall "
						"err=%d",
						phase, ret);
					return ret;
				}
				data_actlen = 0U;
			} else if (ret != 0) {
				if (data_len != 0U && (ret == -EIO || ret == -ETIMEDOUT)) {
					msc_bbb_recover_after_data_fail(udev, uhc_host, phase);
				}
				return ret;
			} else {
				data_actlen = data_len;
				LOG_DBG("msc_bbb(%s): DATA-OUT len=%u", phase, data_actlen);
				USBH_MSC_FLOW_INF("msc_flow: %s DATA OUT OK len=%u", phase,
						  data_actlen);
			}
		}
	}

	USBH_MSC_FLOW_INF("msc_flow: %s CSW IN start (stall_retries=%u)", phase, csw_stall_retries);
	/*
	 * STATUS (bulk IN CSW). Variable-length DATA-IN leaves residue in CSW (USB Mass
	 * Storage Bulk-Only specification, section 5.2).
	 * -EPIPE on CSW: first stall -> CLEAR_FEATURE(HALT) + one CSW retry; second stall in
	 * the same BOT round -> @ref usbh_msc_bot_reset (optional @a uhc_host for host bulk
	 * steady-state restore) + full CBW re-issue.
	 */
	for (;;) {
		in_csw = NULL;
		ret = msc_bot_bulk_in_one(udev, msc->ep_in_addr, sizeof(struct usb_msc_bot_csw),
					  "CSW", &in_csw);
		USBH_MSC_FLOW_INF("msc_flow: %s CSW IN try stall_retries=%u " USBH_MSC_ERR_FMT,
				  phase, csw_stall_retries, USBH_MSC_ERR_ARG(ret));
		if (ret != -EPIPE) {
			if (ret != 0 && csw_stall_retries == 1U && uhc_host != NULL) {
				LOG_WRN("msc_bbb(%s): CSW IN err=%d after HALT clear — BOT reset "
					"+ one CSW retry",
					phase, ret);
				usbh_msc_bot_reset(udev, uhc_host);
				csw_stall_retries = 2U;
				continue;
			}
			break;
		}

		if (csw_stall_retries == 0U) {
			LOG_WRN("msc_bbb(%s): STATUS: CSW IN stall — clear HALT and retry CSW once",
				phase);
			ret = msc_bot_recover_bulk_in_protocol_stall(udev, msc->ep_in_addr,
								     msc->ep_out_addr, uhc_host,
								     phase, "CSW STALL (1st)");
			if (ret != 0) {
				msc_bbb_discard_data_in(udev, data_in_out);
				return ret;
			}
			csw_stall_retries = 1U;
			continue;
		}

		LOG_WRN("msc_bbb(%s): STATUS: CSW IN stall after HALT clear — BOT reset", phase);
		msc_bbb_discard_data_in(udev, data_in_out);
		usbh_msc_bot_reset(udev, uhc_host);
		if (stale_full_cbw_retries + 1U >= MSC_BBB_STALE_CSW_FULL_CBW_RETRIES) {
			LOG_ERR("msc_bbb(%s): CSW STALL: exhausted full CBW retries (%u)", phase,
				(unsigned int)MSC_BBB_STALE_CSW_FULL_CBW_RETRIES);
			return -EPIPE;
		}
		stale_full_cbw_retries++;
		goto retry_full_cbw;
	}

	if (ret != 0) {
		USBH_MSC_FLOW_INF("msc_flow: %s BOT failed at CSW/DATA " USBH_MSC_ERR_FMT, phase,
				  USBH_MSC_ERR_ARG(ret));
		msc_bbb_discard_data_in(udev, data_in_out);
		return ret;
	}

	ret = msc_bbb_validate_csw(in_csw, cmd_tag, data_len, data_actlen, phase);
	usbh_xfer_buf_free(udev, in_csw);
	in_csw = NULL;

	if (ret != 0) {
		USBH_MSC_FLOW_INF("msc_flow: %s CSW validate " USBH_MSC_ERR_FMT, phase,
				  USBH_MSC_ERR_ARG(ret));
		msc_bbb_discard_data_in(udev, data_in_out);
		return ret;
	}

	USBH_MSC_FLOW_INF("msc_flow: %s BOT complete OK", phase);
	return ret;
}

int usbh_msc_bot_issue(struct usb_device *udev, const struct usbh_msc_iface *msc, uint8_t lun,
		       const uint8_t *cdb, uint8_t cdb_len, bool data_in, uint32_t data_len,
		       const uint8_t *data_out_payload, const char *phase,
		       struct net_buf **data_in_out, const struct device *uhc_host)
{
	struct usb_msc_bot_cbw cbw;
	uint32_t cmd_tag;

	if (udev == NULL || msc == NULL || cdb == NULL || phase == NULL) {
		return -EINVAL;
	}

	if (cdb_len == 0U || cdb_len > sizeof(cbw.CBWCB)) {
		return -EINVAL;
	}

	if (data_len != 0U) {
		if (data_in) {
			if (data_in_out == NULL) {
				LOG_ERR("msc_bbb(%s): data-in but data_in_out is NULL", phase);
				return -EINVAL;
			}
		} else if (data_out_payload == NULL) {
			LOG_ERR("msc_bbb(%s): data-out but payload is NULL", phase);
			return -EINVAL;
		}
	}

	cmd_tag = g_msc_bot_tag++;

	(void)memset(&cbw, 0, sizeof(cbw));
	cbw.dCBWSignature = sys_cpu_to_le32(USB_MSC_BOT_CBW_SIGNATURE);
	cbw.dCBWTag = sys_cpu_to_le32(cmd_tag);
	cbw.dCBWDataTransferLength = sys_cpu_to_le32(data_len);
	cbw.bmCBWFlags = data_in ? USB_MSC_BOT_CBW_FLAG_DATA_IN : 0U;
	cbw.bCBWLUN = lun & 0x0FU;
	cbw.bCBWCBLength = cdb_len;
	(void)memcpy(cbw.CBWCB, cdb, cdb_len);

	return msc_bbb_command_with_cbw(udev, msc, &cbw, cmd_tag, data_in, data_len,
					data_out_payload, phase, data_in_out, uhc_host);
}

void usbh_msc_bot_transport_session_seed(void)
{
	g_msc_bot_tag = USB_MSC_BOT_CBW_INQUIRY_TAG_LE;
	g_msc_bbb_usb_ready = false;
}

void usbh_msc_bot_transport_recover_after_err(struct usb_device *udev, const struct device *uhc_dev)
{
	if (udev == NULL) {
		return;
	}

	LOG_WRN("msc: full BOT recovery (EP0 0xFF + HALT clear + ring sync) after xfer error");
	usbh_msc_bot_reset(udev, uhc_dev);
	usbh_msc_bot_transport_session_seed();
}

void usbh_msc_bot_transport_after_tur_ok(void)
{
	g_msc_bbb_usb_ready = true;
}

void usbh_msc_bot_reset(struct usb_device *udev, const struct device *uhc_dev)
{
	struct usbh_msc_iface msc;
	int err;
	int r_in;
	int r_out;
	uint8_t ep_bulk_in;
	uint8_t ep_bulk_out;
	int bulk_ok_before;
	int bulk_ok_after;
	bool steady;

	LOG_INF("======== MSC BOT RESET: EP0 0xFF + bulk IN/OUT CLEAR_FEATURE(HALT) ========");

	if (udev->cfg_desc == NULL) {
		LOG_WRN("msc_reset: SKIP - no cfg_desc (stack must finish SET_CONFIGURATION)");
		return;
	}

	if (udev->state != USB_STATE_CONFIGURED) {
		LOG_WRN("msc_reset: SKIP - USB state=%d (need USB_STATE_CONFIGURED=%d)",
			(int)udev->state, (int)USB_STATE_CONFIGURED);
		return;
	}

	if (usbh_msc_prepare(udev, &msc) != 0) {
		LOG_WRN("msc_reset: SKIP - MSC prepare failed (probe or SET_INTERFACE alt 0)");
		return;
	}

	LOG_INF("msc_reset: MSC GET_MAX_LUN - max_lun=%u (%u logical unit(s))", msc.max_lun,
		(unsigned int)msc.max_lun + 1U);

	ep_bulk_in = msc.ep_in_addr;
	ep_bulk_out = msc.ep_out_addr;

	bulk_ok_before = usbh_eps_verify_steady(udev);
	steady = usbh_post_configure_steady(udev);
	LOG_DBG("msc_reset: pre MSC RESET: bulk_steady=%d steady_post_cfg=%d", bulk_ok_before,
		steady ? 1 : 0);
	LOG_DBG("msc_reset: issuing SETUP on EP0: "
		"bmRequestType=0x21 bRequest=0xFF wValue=0 wIndex=%u wLength=0 "
		"(MSC BOT §5.3.4: wIndex = MSC bInterfaceNumber; must match bound iface)",
		msc.iface_num);
	LOG_DBG("msc_reset: wIndex MSC iface_num=%u", msc.iface_num);

	err = usbh_req_setup(udev, 0x21, USBH_MSC_BOT_REQ_RESET, 0, msc.iface_num, 0, NULL);

	if (err == -EPIPE) {
		LOG_WRN("msc_reset: verify FAIL - EP0 STALL (completion STALL_ERROR)");
	} else if (err != 0) {
		LOG_WRN("msc_reset: verify FAIL - control transfer err=%d", err);
	} else {
		LOG_DBG("msc_reset: control transfer completed (no STALL)");
	}

	if (err == 0) {
		/*
		 * Wall-time delay between BOT reset steps. k_msleep() depends on the
		 * system clock + scheduler; on some integrated USB hosts the thread
		 * may not wake after the first BOT reset success (no further logs). Busy-wait
		 * matches usbh_device.c POST_SET_ADDRESS and guarantees ~150 ms spacing.
		 */
		LOG_DBG("msc_reset: post-BOT-reset delay %u ms (before HALT clear)",
			(unsigned int)MSC_BOT_RESET_STEP_MS);
		k_msleep(MSC_BOT_RESET_STEP_MS);
		LOG_DBG("msc_reset: post-BOT-reset wait done");

		LOG_DBG("msc_reset: CLEAR_FEATURE(HALT) bulk IN ep=0x%02x", ep_bulk_in);
		r_in = usbh_req_clear_sfs_halt(udev, ep_bulk_in);
		if (r_in != 0) {
			LOG_WRN("msc_reset: verify FAIL - clear HALT bulk IN err=%d", r_in);
		} else {
			LOG_DBG("msc_reset: bulk IN halt clear");
		}

		k_msleep(MSC_BOT_RESET_STEP_MS);

		LOG_DBG("msc_reset: CLEAR_FEATURE(HALT) bulk OUT ep=0x%02x", ep_bulk_out);
		r_out = usbh_req_clear_sfs_halt(udev, ep_bulk_out);
		if (r_out != 0) {
			LOG_WRN("msc_reset: verify FAIL - clear HALT bulk OUT err=%d", r_out);
		} else {
			LOG_DBG("msc_reset: bulk OUT halt clear");
		}

		/* Keep IN error if set when aggregating OUT result. */
		err = r_in;
		if (r_in >= 0) {
			err = r_out;
		}

		k_msleep(MSC_BOT_RESET_STEP_MS);

		if (err == 0 && uhc_dev != NULL) {
			int s = usbh_ep_sync_after_clear_feature(udev);

			if (s != 0) {
				LOG_WRN("msc_reset: UHC bulk pipe sync after HALT clear failed "
					"(%d)",
					s);
				err = s;
			} else {
				LOG_DBG("msc_reset: UHC bulk sync after HALT clear OK");
			}
		}
	}

	bulk_ok_after = usbh_eps_verify_steady(udev);
	steady = usbh_post_configure_steady(udev);
	LOG_DBG("msc_reset: post bulk steady diag=%d steady_post_cfg=%d", bulk_ok_after,
		steady ? 1 : 0);

	if (bulk_ok_after != 0) {
		LOG_WRN("msc_reset: verify FAIL - bulk steady diagnostic reported error");
	} else {
		LOG_DBG("msc_reset: bulk steady diagnostic pass");
	}

	if (!steady) {
		LOG_WRN("msc_reset: verify FAIL - controller lost post-Configure Endpoint steady "
			"flag");
	} else {
		LOG_DBG("msc_reset: steady since Configure Endpoint");
	}

	{
		struct usbh_context *ctx = udev->ctx;
		struct usb_device *again = usbh_device_get(ctx, udev->addr);

		if (again == NULL || again != udev || udev->state != USB_STATE_CONFIGURED) {
			LOG_WRN("msc_reset: verify FAIL - device removed or not CONFIGURED");
		} else {
			LOG_INF("msc_reset: verify OK - device still present addr=%u CONFIGURED",
				udev->addr);
		}
	}

	if (err == 0 && bulk_ok_after == 0 && steady) {
		LOG_INF("msc_reset: === ALL targeted checks PASS (reset + HALT clear + steady) "
			"===");
	} else {
		LOG_WRN("msc_reset: === one or more checks FAILED ===");
	}
}
