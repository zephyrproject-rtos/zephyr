/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usbd_msc.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/drivers/usb/udc.h>

#include "usbd_msc_scsi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_msc, CONFIG_USBD_MSC_LOG_LEVEL);

/* Subclass and Protocol codes */
#define SCSI_TRANSPARENT_COMMAND_SET	0x06
#define BULK_ONLY_TRANSPORT		0x50

/* Control requests */
#define GET_MAX_LUN			0xFE
#define BULK_ONLY_MASS_STORAGE_RESET	0xFF

/* Command wrapper */
#define CBW_SIGNATURE			0x43425355

#define CBW_FLAGS_DIRECTION_IN		0x80
#define CBW_FLAGS_RESERVED_MASK		0x3F

struct CBW {
	uint32_t dCBWSignature;
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[16];
} __packed;

/* Status wrapper */
#define CSW_SIGNATURE			0x53425355

#define CSW_STATUS_COMMAND_PASSED	0x00
#define CSW_STATUS_COMMAND_FAILED	0x01
#define CSW_STATUS_PHASE_ERROR		0x02

struct CSW {
	uint32_t dCSWSignature;
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
} __packed;

/* Single instance is likely enough because it can support multiple LUNs */
#define MSC_NUM_INSTANCES CONFIG_USBD_MSC_INSTANCES_COUNT

/* TODO: Bulk wMaxPacketSize is ultimately determined by connection speed
 * and can be 8/16/32/64 on Full-Speed and 512 on High-Speed. USBD handler
 * will update the value based on first connected speed which is wrong,
 * because it has to be set to valid value at connected speed on every
 * connection.
 */
#define MSC_DEFAULT_BULK_EP_MPS 0

/* Can be 64 if device is not High-Speed capable */
#define MSC_BUF_SIZE 512

NET_BUF_POOL_FIXED_DEFINE(msc_ep_pool,
			  MSC_NUM_INSTANCES * 2, MSC_BUF_SIZE,
			  sizeof(struct udc_buf_info), NULL);

struct msc_event {
	struct usbd_class_node *node;
	/* NULL to request Bulk-Only Mass Storage Reset
	 * Otherwise must point to previously enqueued endpoint buffer
	 */
	struct net_buf *buf;
	int err;
};

/* Each instance has 2 endpoints and can receive bulk only reset command */
K_MSGQ_DEFINE(msc_msgq, sizeof(struct msc_event), MSC_NUM_INSTANCES * 3, 4);

/* Make supported vendor request visible for the device stack */
static const struct usbd_cctx_vendor_req msc_bot_vregs =
	USBD_VENDOR_REQ(GET_MAX_LUN, BULK_ONLY_MASS_STORAGE_RESET);

struct msc_bot_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_desc_header nil_desc;
} __packed;

enum {
	MSC_CLASS_ENABLED,
	MSC_BULK_OUT_QUEUED,
	MSC_BULK_IN_QUEUED,
	MSC_BULK_IN_WEDGED,
	MSC_BULK_OUT_WEDGED,
};

enum msc_bot_state {
	MSC_BBB_EXPECT_CBW,
	MSC_BBB_PROCESS_CBW,
	MSC_BBB_PROCESS_READ,
	MSC_BBB_PROCESS_WRITE,
	MSC_BBB_SEND_CSW,
	MSC_BBB_WAIT_FOR_CSW_SENT,
	MSC_BBB_WAIT_FOR_RESET_RECOVERY,
};

struct msc_bot_ctx {
	struct usbd_class_node *class_node;
	atomic_t bits;
	enum msc_bot_state state;
	uint8_t registered_luns;
	struct scsi_ctx luns[CONFIG_USBD_MSC_LUNS_PER_INSTANCE];
	struct CBW cbw;
	struct CSW csw;
	uint8_t scsi_buf[CONFIG_USBD_MSC_SCSI_BUFFER_SIZE];
	uint32_t transferred_data;
	size_t scsi_offset;
	size_t scsi_bytes;
};

static struct net_buf *msc_buf_alloc(const uint8_t ep)
{
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;

	buf = net_buf_alloc(&msc_ep_pool, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = ep;

	return buf;
}

static uint8_t msc_get_bulk_in(struct usbd_class_node *const node)
{
	struct msc_bot_desc *desc = node->data->desc;

	return desc->if0_in_ep.bEndpointAddress;
}

static uint8_t msc_get_bulk_out(struct usbd_class_node *const node)
{
	struct msc_bot_desc *desc = node->data->desc;

	return desc->if0_out_ep.bEndpointAddress;
}

static void msc_queue_bulk_out_ep(struct usbd_class_node *const node)
{
	struct msc_bot_ctx *ctx = node->data->priv;
	struct net_buf *buf;
	uint8_t ep;
	int ret;

	if (atomic_test_and_set_bit(&ctx->bits, MSC_BULK_OUT_QUEUED)) {
		/* Already queued */
		return;
	}

	LOG_DBG("Queuing OUT");
	ep = msc_get_bulk_out(node);
	buf = msc_buf_alloc(ep);
	/* The pool is large enough to support all allocations. Failing alloc
	 * indicates either a memory leak or logic error.
	 */
	__ASSERT_NO_MSG(buf);

	ret = usbd_ep_enqueue(node, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue net_buf for 0x%02x", ep);
		net_buf_unref(buf);
		atomic_clear_bit(&ctx->bits, MSC_BULK_OUT_QUEUED);
	}
}

static void msc_stall_bulk_out_ep(struct usbd_class_node *const node)
{
	uint8_t ep;

	ep = msc_get_bulk_out(node);
	usbd_ep_set_halt(node->data->uds_ctx, ep);
}

static void msc_stall_bulk_in_ep(struct usbd_class_node *const node)
{
	uint8_t ep;

	ep = msc_get_bulk_in(node);
	usbd_ep_set_halt(node->data->uds_ctx, ep);
}

static void msc_reset_handler(struct usbd_class_node *node)
{
	struct msc_bot_ctx *ctx = node->data->priv;
	int i;

	LOG_INF("Bulk-Only Mass Storage Reset");
	ctx->state = MSC_BBB_EXPECT_CBW;
	for (i = 0; i < ctx->registered_luns; i++) {
		scsi_reset(&ctx->luns[i]);
	}

	atomic_clear_bit(&ctx->bits, MSC_BULK_IN_WEDGED);
	atomic_clear_bit(&ctx->bits, MSC_BULK_OUT_WEDGED);
}

static bool is_cbw_meaningful(struct msc_bot_ctx *const ctx)
{
	if (ctx->cbw.bmCBWFlags & CBW_FLAGS_RESERVED_MASK) {
		/* Reserved bits are set = not meaningful */
		return false;
	}

	if (ctx->cbw.bCBWLUN >= ctx->registered_luns) {
		/* Either not registered LUN or invalid (> 0x0F) */
		return false;
	}

	if (ctx->cbw.bCBWCBLength < 1 || ctx->cbw.bCBWCBLength > 16) {
		/* Only legal values are 1 to 16, other are reserved */
		return false;
	}

	return true;
}

static void msc_process_read(struct msc_bot_ctx *ctx)
{
	struct scsi_ctx *lun = &ctx->luns[ctx->cbw.bCBWLUN];
	int bytes_queued = 0;
	struct net_buf *buf;
	uint8_t ep;
	size_t len;
	int ret;

	/* Fill SCSI Data IN buffer if there is no data available */
	if (ctx->scsi_bytes == 0) {
		ctx->scsi_bytes = scsi_read_data(lun, ctx->scsi_buf);
		ctx->scsi_offset = 0;
	}

	if (atomic_test_and_set_bit(&ctx->bits, MSC_BULK_IN_QUEUED)) {
		__ASSERT_NO_MSG(false);
		LOG_ERR("IN already queued");
		return;
	}

	ep = msc_get_bulk_in(ctx->class_node);
	buf = msc_buf_alloc(ep);
	/* The pool is large enough to support all allocations. Failing alloc
	 * indicates either a memory leak or logic error.
	 */
	__ASSERT_NO_MSG(buf);

	while (ctx->scsi_bytes - ctx->scsi_offset > 0) {
		len = MIN(ctx->scsi_bytes - ctx->scsi_offset,
			  MSC_BUF_SIZE - bytes_queued);
		if (len == 0) {
			/* Either queued as much as possible or there is no more
			 * SCSI IN data available
			 */
			break;
		}

		net_buf_add_mem(buf, &ctx->scsi_buf[ctx->scsi_offset], len);
		bytes_queued += len;
		ctx->scsi_offset += len;

		if (ctx->scsi_bytes == ctx->scsi_offset) {
			/* SCSI buffer can be reused now */
			ctx->scsi_bytes = scsi_read_data(lun, ctx->scsi_buf);
			ctx->scsi_offset = 0;
		}
	}

	/* Either the net buf is full or there is no more SCSI data */
	ctx->csw.dCSWDataResidue -= bytes_queued;
	ret = usbd_ep_enqueue(ctx->class_node, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue net_buf for 0x%02x", ep);
		net_buf_unref(buf);
		atomic_clear_bit(&ctx->bits, MSC_BULK_IN_QUEUED);
	}
}

static void msc_process_cbw(struct msc_bot_ctx *ctx)
{
	struct scsi_ctx *lun = &ctx->luns[ctx->cbw.bCBWLUN];
	bool cmd_is_data_read, cmd_is_data_write;
	size_t data_len;
	int cb_len;

	cb_len = scsi_usb_boot_cmd_len(ctx->cbw.CBWCB, ctx->cbw.bCBWCBLength);
	data_len = scsi_cmd(lun, ctx->cbw.CBWCB, cb_len, ctx->scsi_buf);
	ctx->scsi_bytes = data_len;
	ctx->scsi_offset = 0;
	cmd_is_data_read = scsi_cmd_is_data_read(lun);
	cmd_is_data_write = scsi_cmd_is_data_write(lun);
	data_len += scsi_cmd_remaining_data_len(lun);

	/* Write commands must not return any data to initiator (host) */
	__ASSERT_NO_MSG(cmd_is_data_read || ctx->scsi_bytes == 0);

	if (ctx->cbw.dCBWDataTransferLength == 0) {
		/* 6.7.1 Hn - Host expects no data transfers */
		if (data_len == 0) {
			/* Case (1) Hn = Dn */
			if (scsi_cmd_get_status(lun) == GOOD) {
				ctx->csw.bCSWStatus = CSW_STATUS_COMMAND_PASSED;
			} else {
				ctx->csw.bCSWStatus = CSW_STATUS_COMMAND_FAILED;
			}
		} else {
			/* Case (2) Hn < Di or (3) Hn < Do */
			ctx->csw.bCSWStatus = CSW_STATUS_PHASE_ERROR;
		}

		ctx->state = MSC_BBB_SEND_CSW;
	} else if (data_len == 0) {
		/* SCSI target does not want any data, but host either wants to
		 * send or receive data. Note that SCSI target data direction is
		 * irrelevant, because opcode can simply be not supported. Even
		 * if host maliciously issues 0 sectors read and wants to write
		 * data as indicated in CB it is still Case (9) Ho > Dn.
		 */
		if (ctx->cbw.bmCBWFlags & CBW_FLAGS_DIRECTION_IN) {
			/* Case (4) Hi > Dn */
			msc_stall_bulk_in_ep(ctx->class_node);
		} else {
			/* Case (9) Ho > Dn */
			msc_stall_bulk_out_ep(ctx->class_node);
		}

		if (scsi_cmd_get_status(lun) == GOOD) {
			ctx->csw.bCSWStatus = CSW_STATUS_COMMAND_PASSED;
		} else {
			ctx->csw.bCSWStatus = CSW_STATUS_COMMAND_FAILED;
		}

		ctx->state = MSC_BBB_SEND_CSW;
	} else if (ctx->cbw.bmCBWFlags & CBW_FLAGS_DIRECTION_IN) {
		/* 6.7.2 Hi - Host expects to receive data from device */
		if ((data_len > ctx->cbw.dCBWDataTransferLength) ||
		    !cmd_is_data_read) {
			/* Case (7) Hi < Di or (8) Hi <> Do */
			msc_stall_bulk_in_ep(ctx->class_node);
			ctx->csw.bCSWStatus = CSW_STATUS_PHASE_ERROR;
			ctx->state = MSC_BBB_SEND_CSW;
		} else {
			/* Case (5) Hi > Di or (6) Hi = Di */
			ctx->state = MSC_BBB_PROCESS_READ;
		}
	} else {
		/* 6.7.3 Ho - Host expects to send data to the device */
		if ((data_len > ctx->cbw.dCBWDataTransferLength) ||
		    !cmd_is_data_write) {
			/* Case (10) Ho <> Di or (13) Ho < Do */
			msc_stall_bulk_out_ep(ctx->class_node);
			ctx->csw.bCSWStatus = CSW_STATUS_PHASE_ERROR;
			ctx->state = MSC_BBB_SEND_CSW;
		} else {
			/* Case (11) Ho > Do or (12) Ho = Do */
			ctx->state = MSC_BBB_PROCESS_WRITE;
		}
	}
}

static void msc_process_write(struct msc_bot_ctx *ctx,
			      uint8_t *buf, size_t len)
{
	size_t tmp;
	struct scsi_ctx *lun = &ctx->luns[ctx->cbw.bCBWLUN];

	ctx->transferred_data += len;

	while ((len > 0) && (scsi_cmd_remaining_data_len(lun) > 0)) {
		/* Copy received data to the end of SCSI buffer */
		tmp = MIN(len, sizeof(ctx->scsi_buf) - ctx->scsi_bytes);
		memcpy(&ctx->scsi_buf[ctx->scsi_bytes], buf, tmp);
		ctx->scsi_bytes += tmp;
		buf += tmp;
		len -= tmp;

		/* Pass data to SCSI layer when either all transfer data bytes
		 * have been received or SCSI buffer is full.
		 */
		while ((ctx->scsi_bytes >= scsi_cmd_remaining_data_len(lun)) ||
		       (ctx->scsi_bytes == sizeof(ctx->scsi_buf))) {
			tmp = scsi_write_data(lun, ctx->scsi_buf, ctx->scsi_bytes);
			__ASSERT(tmp <= ctx->scsi_bytes,
				 "Processed more data than requested");
			if (tmp == 0) {
				LOG_WRN("SCSI handler didn't process %d bytes",
					ctx->scsi_bytes);
				ctx->scsi_bytes = 0;
			} else {
				LOG_DBG("SCSI processed %d out of %d bytes",
					tmp, ctx->scsi_bytes);
			}

			ctx->csw.dCSWDataResidue -= tmp;
			if (scsi_cmd_remaining_data_len(lun) == 0) {
				/* Abandon any leftover data */
				ctx->scsi_bytes = 0;
				break;
			}

			/* Move remaining data at the start of SCSI buffer. Note
			 * that the copied length here is zero (and thus no copy
			 * happens) when underlying sector size is equal to SCSI
			 * buffer size.
			 */
			memmove(ctx->scsi_buf, &ctx->scsi_buf[tmp], ctx->scsi_bytes - tmp);
			ctx->scsi_bytes -= tmp;
		}
	}

	if ((ctx->transferred_data >= ctx->cbw.dCBWDataTransferLength) ||
	    (scsi_cmd_remaining_data_len(lun) == 0)) {
		if (ctx->transferred_data < ctx->cbw.dCBWDataTransferLength) {
			/* Case (11) Ho > Do and the transfer is still in
			 * progress. We do not intend to process more data so
			 * stall the Bulk-Out pipe.
			 */
			msc_stall_bulk_out_ep(ctx->class_node);
		}

		if (scsi_cmd_get_status(lun) == GOOD) {
			ctx->csw.bCSWStatus = CSW_STATUS_COMMAND_PASSED;
		} else {
			ctx->csw.bCSWStatus = CSW_STATUS_COMMAND_FAILED;
		}

		ctx->state = MSC_BBB_SEND_CSW;
	}
}

static void msc_handle_bulk_out(struct msc_bot_ctx *ctx,
				uint8_t *buf, size_t len)
{
	if (ctx->state == MSC_BBB_EXPECT_CBW) {
		if (len == sizeof(struct CBW) && sys_get_le32(buf) == CBW_SIGNATURE) {
			memcpy(&ctx->cbw, buf, sizeof(struct CBW));
			/* Convert dCBWDataTransferLength endianness, other
			 * fields are either single byte or not relevant.
			 */
			ctx->cbw.dCBWDataTransferLength =
				sys_le32_to_cpu(ctx->cbw.dCBWDataTransferLength);
			/* Fill CSW with relevant information */
			ctx->csw.dCSWSignature = sys_cpu_to_le32(CSW_SIGNATURE);
			ctx->csw.dCSWTag = ctx->cbw.dCBWTag;
			ctx->csw.dCSWDataResidue = ctx->cbw.dCBWDataTransferLength;
			ctx->transferred_data = 0;
			if (is_cbw_meaningful(ctx)) {
				ctx->csw.bCSWStatus = CSW_STATUS_COMMAND_FAILED;
				ctx->state = MSC_BBB_PROCESS_CBW;
			} else {
				LOG_INF("Not meaningful CBW");
				/* Mass Storage Class - Bulk Only Transport
				 * does not specify response to not meaningful
				 * CBW. Stall Bulk IN and Report Phase Error.
				 */
				msc_stall_bulk_in_ep(ctx->class_node);
				ctx->csw.bCSWStatus = CSW_STATUS_PHASE_ERROR;
				ctx->state = MSC_BBB_SEND_CSW;
			}
		} else {
			/* 6.6.1 CBW Not Valid */
			LOG_INF("Invalid CBW");
			atomic_set_bit(&ctx->bits, MSC_BULK_IN_WEDGED);
			atomic_set_bit(&ctx->bits, MSC_BULK_OUT_WEDGED);
			msc_stall_bulk_in_ep(ctx->class_node);
			msc_stall_bulk_out_ep(ctx->class_node);
			ctx->state = MSC_BBB_WAIT_FOR_RESET_RECOVERY;
		}
	} else if (ctx->state == MSC_BBB_PROCESS_WRITE) {
		msc_process_write(ctx, buf, len);
	}
}

static void msc_handle_bulk_in(struct msc_bot_ctx *ctx,
			       uint8_t *buf, size_t len)
{
	if (ctx->state == MSC_BBB_WAIT_FOR_CSW_SENT) {
		LOG_DBG("CSW sent");
		ctx->state = MSC_BBB_EXPECT_CBW;
	} else if (ctx->state == MSC_BBB_PROCESS_READ) {
		struct scsi_ctx *lun = &ctx->luns[ctx->cbw.bCBWLUN];

		ctx->transferred_data += len;
		if (ctx->scsi_bytes == 0) {
			if (ctx->csw.dCSWDataResidue > 0) {
				/* Case (5) Hi > Di
				 * While we may have sent short packet, device
				 * shall STALL the Bulk-In pipe (if it does not
				 * send padding data).
				 */
				msc_stall_bulk_in_ep(ctx->class_node);
			}
			if (scsi_cmd_get_status(lun) == GOOD) {
				ctx->csw.bCSWStatus = CSW_STATUS_COMMAND_PASSED;
			} else {
				ctx->csw.bCSWStatus = CSW_STATUS_COMMAND_FAILED;
			}
			ctx->state = MSC_BBB_SEND_CSW;
		}
	}
}

static void msc_send_csw(struct msc_bot_ctx *ctx)
{
	struct net_buf *buf;
	uint8_t ep;
	int ret;

	if (atomic_test_and_set_bit(&ctx->bits, MSC_BULK_IN_QUEUED)) {
		__ASSERT_NO_MSG(false);
		LOG_ERR("IN already queued");
		return;
	}

	/* Convert dCSWDataResidue to LE, other fields are already set */
	ctx->csw.dCSWDataResidue = sys_cpu_to_le32(ctx->csw.dCSWDataResidue);
	ep = msc_get_bulk_in(ctx->class_node);
	buf = msc_buf_alloc(ep);
	/* The pool is large enough to support all allocations. Failing alloc
	 * indicates either a memory leak or logic error.
	 */
	__ASSERT_NO_MSG(buf);

	net_buf_add_mem(buf, &ctx->csw, sizeof(ctx->csw));
	ret = usbd_ep_enqueue(ctx->class_node, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue net_buf for 0x%02x", ep);
		net_buf_unref(buf);
		atomic_clear_bit(&ctx->bits, MSC_BULK_IN_QUEUED);
	}
	ctx->state = MSC_BBB_WAIT_FOR_CSW_SENT;
}

static void usbd_msc_handle_request(struct usbd_class_node *node,
				    struct net_buf *buf, int err)
{
	struct usbd_contex *uds_ctx = node->data->uds_ctx;
	struct msc_bot_ctx *ctx = node->data->priv;
	struct udc_buf_info *bi;

	bi = udc_get_buf_info(buf);
	if (err) {
		if (err == -ECONNABORTED) {
			LOG_WRN("request ep 0x%02x, len %u cancelled",
				bi->ep, buf->len);
		} else {
			LOG_ERR("request ep 0x%02x, len %u failed",
				bi->ep, buf->len);
		}

		goto ep_request_error;
	}

	if (bi->ep == msc_get_bulk_out(node)) {
		msc_handle_bulk_out(ctx, buf->data, buf->len);
	} else if (bi->ep == msc_get_bulk_in(node)) {
		msc_handle_bulk_in(ctx, buf->data, buf->len);
	}

ep_request_error:
	if (bi->ep == msc_get_bulk_out(node)) {
		atomic_clear_bit(&ctx->bits, MSC_BULK_OUT_QUEUED);
	} else if (bi->ep == msc_get_bulk_in(node)) {
		atomic_clear_bit(&ctx->bits, MSC_BULK_IN_QUEUED);
	}
	usbd_ep_buf_free(uds_ctx, buf);
}

static void usbd_msc_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	struct msc_event evt;
	struct msc_bot_ctx *ctx;

	while (1) {
		k_msgq_get(&msc_msgq, &evt, K_FOREVER);

		ctx = evt.node->data->priv;
		if (evt.buf == NULL) {
			msc_reset_handler(evt.node);
		} else {
			usbd_msc_handle_request(evt.node, evt.buf, evt.err);
		}

		if (!atomic_test_bit(&ctx->bits, MSC_CLASS_ENABLED)) {
			continue;
		}

		switch (ctx->state) {
		case MSC_BBB_EXPECT_CBW:
		case MSC_BBB_PROCESS_WRITE:
			/* Ensure we can accept next OUT packet */
			msc_queue_bulk_out_ep(evt.node);
			break;
		default:
			break;
		}

		/* Skip (potentially) response generating code if there is
		 * IN data already available for the host to pick up.
		 */
		if (atomic_test_bit(&ctx->bits, MSC_BULK_IN_QUEUED)) {
			continue;
		}

		if (ctx->state == MSC_BBB_PROCESS_CBW) {
			msc_process_cbw(ctx);
		}

		if (ctx->state == MSC_BBB_PROCESS_READ) {
			msc_process_read(ctx);
		} else if (ctx->state == MSC_BBB_PROCESS_WRITE) {
			msc_queue_bulk_out_ep(evt.node);
		} else if (ctx->state == MSC_BBB_SEND_CSW) {
			msc_send_csw(ctx);
		}
	}
}

static void msc_bot_schedule_reset(struct usbd_class_node *node)
{
	struct msc_event request = {
		.node = node,
		.buf = NULL, /* Bulk-Only Mass Storage Reset */
	};

	k_msgq_put(&msc_msgq, &request, K_FOREVER);
}

/* Feature endpoint halt state handler */
static void msc_bot_feature_halt(struct usbd_class_node *const node,
				 const uint8_t ep, const bool halted)
{
	struct msc_bot_ctx *ctx = node->data->priv;

	if (ep == msc_get_bulk_in(node) && !halted &&
	    atomic_test_bit(&ctx->bits, MSC_BULK_IN_WEDGED)) {
		/* Endpoint shall remain halted until Reset Recovery */
		usbd_ep_set_halt(node->data->uds_ctx, ep);
	} else if (ep == msc_get_bulk_out(node) && !halted &&
	    atomic_test_bit(&ctx->bits, MSC_BULK_OUT_WEDGED)) {
		/* Endpoint shall remain halted until Reset Recovery */
		usbd_ep_set_halt(node->data->uds_ctx, ep);
	}
}

/* USB control request handler to device */
static int msc_bot_control_to_dev(struct usbd_class_node *const node,
				  const struct usb_setup_packet *const setup,
				  const struct net_buf *const buf)
{
	if (setup->bRequest == BULK_ONLY_MASS_STORAGE_RESET &&
	    setup->wValue == 0 && setup->wLength == 0) {
		msc_bot_schedule_reset(node);
	} else {
		errno = -ENOTSUP;
	}

	return 0;
}

/* USB control request handler to host */
static int msc_bot_control_to_host(struct usbd_class_node *const node,
				   const struct usb_setup_packet *const setup,
				   struct net_buf *const buf)
{
	struct msc_bot_ctx *ctx = node->data->priv;
	uint8_t max_lun;

	if (setup->bRequest == GET_MAX_LUN &&
	    setup->wValue == 0 && setup->wLength >= 1) {
		/* If there is no LUN registered we cannot really do anything,
		 * because STALLing this request means that device does not
		 * support multiple LUNs and host should only address LUN 0.
		 */
		max_lun = ctx->registered_luns ? ctx->registered_luns - 1 : 0;
		net_buf_add_mem(buf, &max_lun, 1);
	} else {
		errno = -ENOTSUP;
	}

	return 0;
}

/* Endpoint request completion event handler */
static int msc_bot_request_handler(struct usbd_class_node *const node,
				   struct net_buf *buf, int err)
{
	struct msc_event request = {
		.node = node,
		.buf = buf,
		.err = err,
	};

	/* Defer request handling to mass storage thread */
	k_msgq_put(&msc_msgq, &request, K_FOREVER);

	return 0;
}

/* Class associated configuration is selected */
static void msc_bot_enable(struct usbd_class_node *const node)
{
	struct msc_bot_ctx *ctx = node->data->priv;

	LOG_INF("Enable");
	atomic_set_bit(&ctx->bits, MSC_CLASS_ENABLED);
	msc_bot_schedule_reset(node);
}

/* Class associated configuration is disabled */
static void msc_bot_disable(struct usbd_class_node *const node)
{
	struct msc_bot_ctx *ctx = node->data->priv;

	LOG_INF("Disable");
	atomic_clear_bit(&ctx->bits, MSC_CLASS_ENABLED);
}

/* Initialization of the class implementation */
static int msc_bot_init(struct usbd_class_node *const node)
{
	struct msc_bot_ctx *ctx = node->data->priv;

	ctx->class_node = node;
	ctx->state = MSC_BBB_EXPECT_CBW;
	ctx->registered_luns = 0;

	STRUCT_SECTION_FOREACH(usbd_msc_lun, lun) {
		if (ctx->registered_luns >= CONFIG_USBD_MSC_LUNS_PER_INSTANCE) {
			LOG_ERR("Cannot register LUN %s", lun->disk);
			return -ENOMEM;
		}

		scsi_init(&ctx->luns[ctx->registered_luns++], lun->disk,
			  lun->vendor, lun->product, lun->revision);
	}

	return 0;
}

#define DEFINE_MSC_BOT_DESCRIPTOR(n, _)						\
static struct msc_bot_desc msc_bot_desc_##n = {					\
	.if0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_MASS_STORAGE,			\
		.bInterfaceSubClass = SCSI_TRANSPARENT_COMMAND_SET,		\
		.bInterfaceProtocol = BULK_ONLY_TRANSPORT,			\
		.iInterface = 0,						\
	},									\
	.if0_in_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(MSC_DEFAULT_BULK_EP_MPS),	\
		.bInterval = 0,							\
	},									\
	.if0_out_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(MSC_DEFAULT_BULK_EP_MPS),	\
		.bInterval = 0,							\
	},									\
										\
	.nil_desc = {								\
		.bLength = 0,							\
		.bDescriptorType = 0,						\
	},									\
};

struct usbd_class_api msc_bot_api = {
	.feature_halt = msc_bot_feature_halt,
	.control_to_dev = msc_bot_control_to_dev,
	.control_to_host = msc_bot_control_to_host,
	.request = msc_bot_request_handler,
	.enable = msc_bot_enable,
	.disable = msc_bot_disable,
	.init = msc_bot_init,
};

#define DEFINE_MSC_BOT_CLASS_DATA(x, _)					\
	static struct msc_bot_ctx msc_bot_ctx_##x;			\
	static struct usbd_class_data msc_bot_class_##x = {		\
		.desc = (struct usb_desc_header *)&msc_bot_desc_##x,	\
		.v_reqs = &msc_bot_vregs,				\
		.priv = &msc_bot_ctx_##x,				\
	};								\
									\
	USBD_DEFINE_CLASS(msc_##x, &msc_bot_api, &msc_bot_class_##x);

LISTIFY(MSC_NUM_INSTANCES, DEFINE_MSC_BOT_DESCRIPTOR, ())
LISTIFY(MSC_NUM_INSTANCES, DEFINE_MSC_BOT_CLASS_DATA, ())

K_THREAD_DEFINE(usbd_msc, CONFIG_USBD_MSC_STACK_SIZE,
		usbd_msc_thread, NULL, NULL, NULL,
		CONFIG_SYSTEM_WORKQUEUE_PRIORITY, 0, 0);
