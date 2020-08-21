/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(usbd_ch9, CONFIG_USBD_LOG_LEVEL);

#include <zephyr.h>
#include <errno.h>
#include <usb/usbd.h>
#include <usbd_internal.h>
#include <sys/byteorder.h>
#include <net/buf.h>

extern struct usbd_contex usbd_ctx;

static int usb_set_configuration(const struct usb_setup_packet *setup)
{
	struct usbd_class_ctx *cctx = NULL;
	int ret;

	LOG_DBG("Set Configuration Request value %u", setup->wValue);

	if (setup->wValue == 0 || setup->wValue == usbd_ctx.configuration) {
		/*
		 * Action depends on device state and
		 * because device state is not tracked
		 * zero value can not be handled for now.
		 */
		LOG_WRN("Current configuration %u", usbd_ctx.configuration);
		return 0;
	}

	if (setup->wValue != 1) {
		LOG_ERR("Configuration %u not supported", setup->wValue);
		return -EINVAL;
	}

	for (uint8_t i = 0; i < usbd_ctx.cfg_desc.bNumInterfaces; i++) {
		if (usbd_ctx.alternate[i] != 0) {
			ret = usbd_cctx_cfg_eps(i, false);
			if (ret != 0) {
				return ret;
			}

			usbd_ctx.alternate[i] = 0;
		}

		ret = usbd_cctx_cfg_eps(i, true);
		if (ret != 0) {
			return ret;
		}
	}

	if (usbd_ctx.status_cb != NULL) {
		usbd_ctx.status_cb(USB_DC_CONFIGURED,
				   (uint8_t *)&setup->wValue);
	}

	LOG_WRN("Set up configuration %u", setup->wValue);
	usbd_ctx.configuration = setup->wValue;
	atomic_set_bit(&usbd_ctx.state, USBD_STATE_CONFIGURED);

	SYS_SLIST_FOR_EACH_CONTAINER(&usbd_ctx.class_list, cctx, node) {
		if (cctx->ops.cfg_update != NULL) {
			cctx->ops.cfg_update(cctx,
					     (struct usb_setup_packet *)setup);
		}
	}

	return 0;
}

static int usb_set_interface(const struct usb_setup_packet *setup)
{
	uint8_t cur_alternate = usbd_ctx.alternate[setup->wIndex];
	struct usbd_class_ctx *cctx = usbd_cctx_get_by_iface(setup->wIndex);
	int ret;

	LOG_INF("Set Interfaces %u, alternate %u -> %u",
		setup->wIndex, cur_alternate, setup->wValue);

	ret = usbd_cctx_cfg_eps(setup->wIndex, false);
	if (ret != 0) {
		return ret;
	}

	usbd_ctx.alternate[setup->wIndex] = setup->wValue;

	ret = usbd_cctx_cfg_eps(setup->wIndex, true);
	if (ret != 0) {
		return ret;
	}

	if (cctx == NULL) {
		return -ENOTSUP;
	}

	if (cctx->ops.cfg_update != NULL) {
		cctx->ops.cfg_update(cctx, (struct usb_setup_packet *)setup);
	}

	return 0;
}

/* Prepare IN transfer with smallest length */
static int usbd_trim_submit(const struct usb_setup_packet *setup,
			    struct net_buf *buf)
{
	size_t min_len = MIN(setup->wLength, buf->len);
	bool handle_zlp = true;


	if (min_len > 0) {
		LOG_DBG("Prepare Data IN Stage, %u bytes", min_len);
	} else {
		LOG_ERR("Unnecessary trim for Status IN Stage!");
		net_buf_pull(buf, buf->len);
		usbd_tbuf_submit(buf, false);

		return 0;
	}

	/*
	 * Set ZLP flag when host asks for a bigger length and the
	 * last chunk is wMaxPacketSize long, to indicate the last
	 * packet.
	 */
	if (setup->wLength > min_len && !(min_len % USB_CONTROL_EP_MPS)) {
		/*
		 * Transfer length is less as requested by wLength and
		 * is multiple of wMaxPacketSize.
		 */
		LOG_INF("ZLP, wLength %u , response length %u ",
			setup->wLength, min_len);
		handle_zlp = true;
		if (IS_ENABLED(CONFIG_USBD_DEVICE_DISABLE_ZLP_EPIN_HANDLING)) {
			handle_zlp = false;
		}

	} else if (setup->wLength) {
		if (!(min_len % USB_CONTROL_EP_MPS)) {
			LOG_INF("no ZLP, requested %u , length %u ",
				setup->wLength, min_len);
			handle_zlp = false;
		}
	}

	/* FIXME: */
	buf->len = min_len;

	usbd_tbuf_submit(buf, handle_zlp);

	return 0;
}

static int usb_get_cfg_desc(const struct usb_setup_packet *setup)
{
	struct usbd_class_ctx *cctx;
	struct net_buf *buf;

	buf = usbd_tbuf_alloc(USB_CONTROL_EP_IN,
			     usbd_ctx.cfg_desc.wTotalLength);
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, &usbd_ctx.cfg_desc, usbd_ctx.cfg_desc.bLength);

	SYS_SLIST_FOR_EACH_CONTAINER(&usbd_ctx.class_list, cctx, node) {
		if (cctx->class_desc == NULL) {
			continue;
		}

		struct usb_desc_header *head = cctx->class_desc;
		size_t desc_len = usbd_cctx_desc_len(cctx);

		net_buf_add_mem(buf, head, desc_len);
	}

	usbd_trim_submit(setup, buf);

	return 0;
}

static int usb_get_descriptor(const struct usb_setup_packet *setup)
{
	uint8_t desc_type = USB_GET_DESCRIPTOR_TYPE(setup->wValue);
	uint8_t desc_idx = USB_GET_DESCRIPTOR_INDEX(setup->wValue);
	struct usb_desc_header *head;
	struct net_buf *buf;

	LOG_DBG("Get Descriptor Request type %u index %u",
		desc_type, desc_idx);

	switch (desc_type) {
	case USB_DESC_DEVICE:
		head = (struct usb_desc_header *)&usbd_ctx.dev_desc;

		buf = usbd_tbuf_alloc(USB_CONTROL_EP_IN, head->bLength);
		if (buf == NULL) {
			return -ENOMEM;
		}

		net_buf_add_mem(buf, head, head->bLength);
		usbd_tbuf_submit(buf, true);

		break;
	case USB_DESC_CONFIGURATION:
		if (usb_get_cfg_desc(setup) != 0) {
			return -ENOMEM;
		}

		break;
	case USB_DESC_STRING:
		if (desc_idx == 0) {
			head = (struct usb_desc_header *)&usbd_ctx.lang_desc;
		} else if (desc_idx == 1) {
			head = (struct usb_desc_header *)&usbd_ctx.mfr_desc;
		} else if (desc_idx == 2) {
			head = (struct usb_desc_header *)&usbd_ctx.product_desc;
		} else if (desc_idx == 3) {
			head = (struct usb_desc_header *)&usbd_ctx.sn_desc;
		} else {
			return -ENOTSUP;
		}

		buf = usbd_tbuf_alloc(USB_CONTROL_EP_IN, head->bLength);
		if (buf == NULL) {
			return -ENOMEM;
		}

		net_buf_add_mem(buf, head, head->bLength);
		usbd_tbuf_submit(buf, true);

		break;
	case USB_DESC_INTERFACE:
	case USB_DESC_ENDPOINT:
	case USB_DESC_OTHER_SPEED:
	default:
		LOG_DBG("Invalid descriptor type");
		return -EINVAL;
	}

	return 0;
}

static int usb_req_get_status(struct usb_setup_packet *setup)
{
	struct net_buf *buf;
	uint16_t response = 0;
	uint8_t ep_addr = setup->wIndex;
	uint8_t stalled;

	switch (setup->RequestType.recipient) {
	case USB_REQTYPE_RECIPIENT_DEVICE:
		if (IS_ENABLED(CONFIG_USBD_DEVICE_REMOTE_WAKEUP)) {
			response = (usbd_ctx.remote_wakeup ?
				    USB_GET_STATUS_REMOTE_WAKEUP : 0);
		}

		break;
	case USB_REQTYPE_RECIPIENT_ENDPOINT:
		usb_dc_ep_is_stalled(ep_addr, &stalled);
		if (stalled) {
			response = BIT(0);
		}

		break;
	case USB_REQTYPE_RECIPIENT_INTERFACE:
	default:
		break;
	}


	buf = usbd_tbuf_alloc(USB_CONTROL_EP_IN, sizeof(response));
	if (buf == NULL) {
		return -ENOMEM;
	}

	LOG_DBG("Get Status response 0x%04x", response);

	net_buf_add_le16(buf, response);
	usbd_tbuf_submit(buf, true);

	return 0;
}

static int usb_req_get_iface(struct usb_setup_packet *setup)

{
	/* TODO: add range check for wIndex */
	uint8_t cur_alternate = usbd_ctx.alternate[setup->wIndex];
	struct net_buf *buf;

	LOG_DBG("Get Interfaces %u, alternate %u",
		setup->wIndex, cur_alternate);

	buf = usbd_tbuf_alloc(USB_CONTROL_EP_IN, sizeof(cur_alternate));
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, cur_alternate);
	usbd_tbuf_submit(buf, true);

	return 0;
}

static int usb_req_get_cfg(struct usb_setup_packet *setup)

{
	uint8_t cfg = usbd_ctx.configuration;
	struct net_buf *buf;

	LOG_DBG("Get Configuration request");

	buf = usbd_tbuf_alloc(USB_CONTROL_EP_IN, sizeof(cfg));
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, cfg);
	usbd_tbuf_submit(buf, true);

	return 0;
}

static int usb_ep_feature_request(struct usb_setup_packet *setup)
{
	uint8_t ep_addr = (uint8_t)setup->wIndex;

	if (setup->bRequest == USB_SREQ_SET_FEATURE) {
		LOG_INF("Set Feature Halt for 0x%02x", ep_addr);
		usb_dc_ep_set_stall(ep_addr);
		if (usbd_ctx.status_cb) {
			usbd_ctx.status_cb(USB_DC_SET_HALT, &ep_addr);
		}
	} else if (setup->bRequest == USB_SREQ_CLEAR_FEATURE) {
		LOG_INF("Clear Feature Halt 0x%02x", ep_addr);
		usb_dc_ep_clear_stall(ep_addr);
		if (usbd_ctx.status_cb) {
			usbd_ctx.status_cb(USB_DC_CLEAR_HALT, &ep_addr);
		}
	}

	return 0;
}

static int usbd_request_to_device(struct usb_setup_packet *setup,
				  struct net_buf *buf)
{
	int ret;

	switch (setup->bRequest) {
	case USB_SREQ_GET_STATUS:
		ret = usb_req_get_status(setup);
		break;
	case USB_SREQ_SET_ADDRESS:
		LOG_DBG("Set Address request, addr 0x%x", setup->wValue);
		usb_dc_set_address(setup->wValue);
		ret = 0;
		break;
	case USB_SREQ_GET_DESCRIPTOR:
		ret = usb_get_descriptor(setup);
		break;
	case USB_SREQ_GET_CONFIGURATION:
		ret = usb_req_get_cfg(setup);
		break;
	case USB_SREQ_SET_CONFIGURATION:
		ret = usb_set_configuration(setup);
		break;
	case USB_SREQ_CLEAR_FEATURE:
		LOG_DBG("Clear Feature request");

		if (IS_ENABLED(CONFIG_USBD_DEVICE_REMOTE_WAKEUP) &&
		    (setup->wValue == USB_SFS_REMOTE_WAKEUP)) {
			usbd_ctx.remote_wakeup = false;
			ret = 0;
		} else {
			ret = -ENOTSUP;
		}

		break;
	case USB_SREQ_SET_FEATURE:
		LOG_DBG("Set Feature request");

		if (IS_ENABLED(CONFIG_USBD_DEVICE_REMOTE_WAKEUP) &&
		    (setup->wValue == USB_SFS_REMOTE_WAKEUP)) {
			usbd_ctx.remote_wakeup = true;
			ret = 0;
		} else {
			ret = -ENOTSUP;
		}

		break;
	case USB_SREQ_SET_DESCRIPTOR:
	default:
		LOG_DBG("Request 0x%02x not supported", setup->bRequest);
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int usbd_requst_to_iface(struct usb_setup_packet *setup,
				struct net_buf *buf)
{
	int ret;

	switch (setup->bRequest) {
	case USB_SREQ_GET_STATUS:
		ret = usb_req_get_status(setup);
		break;

	case USB_SREQ_GET_INTERFACE:
		ret = usb_req_get_iface(setup);
		break;

	case USB_SREQ_SET_INTERFACE:
		ret = usb_set_interface(setup);
		break;

	case USB_SREQ_CLEAR_FEATURE:
	case USB_SREQ_SET_FEATURE:
	default:
		LOG_DBG("Request 0x%02x not supported", setup->bRequest);
		ret = -ENOTSUP;
	}

	return ret;
}

static int usbd_requst_to_ep(struct usb_setup_packet *setup,
			     struct net_buf *buf)
{
	int ret;

	switch (setup->bRequest) {
	case USB_SREQ_GET_STATUS:
		ret = usb_req_get_status(setup);
		break;

	case USB_SREQ_SET_FEATURE:
	case USB_SREQ_CLEAR_FEATURE:
		if (setup->wValue == USB_SFS_ENDPOINT_HALT) {
			ret = usb_ep_feature_request(setup);
		} else {
			ret = -ENOTSUP;
		}
		break;

	case USB_SREQ_SYNCH_FRAME:
	default:
		LOG_DBG("Request 0x%02x not supported", setup->bRequest);
		ret = -ENOTSUP;
	}

	return ret;
}

static int usbd_nonstd_request(struct usb_setup_packet *setup,
			       struct net_buf *buf)
{
	struct usbd_class_ctx *cctx;
	int ret;

	switch (setup->RequestType.recipient) {
	case USB_REQTYPE_RECIPIENT_ENDPOINT:
		cctx = usbd_cctx_get_by_ep(setup->wIndex);
		if (cctx != NULL && cctx->ops.req_handler != NULL) {
			ret = cctx->ops.req_handler(cctx, setup, buf);
		} else {
			ret = -ENOTSUP;
		}
		break;
	case USB_REQTYPE_RECIPIENT_INTERFACE:
		cctx = usbd_cctx_get_by_iface(setup->wIndex);
		if (cctx != NULL && cctx->ops.req_handler != NULL) {
			ret = cctx->ops.req_handler(cctx, setup, buf);
		} else {
			ret = -ENOTSUP;
		}
		break;
	case USB_REQTYPE_RECIPIENT_DEVICE:
		cctx = usbd_cctx_get_by_req(setup->bRequest);
		if (cctx != NULL && cctx->ops.req_handler != NULL) {
			ret = cctx->ops.req_handler(cctx, setup, buf);
		} else {
			ret = -ENOTSUP;
		}
		break;
	default:
		LOG_ERR("Wrong request type");
		ret = -EINVAL;
	}

	return ret;
}

static int usbd_handle_setup(struct usb_setup_packet *setup,
			     struct net_buf *buf)
{
	int ret;

	switch (setup->RequestType.type) {
	case USB_REQTYPE_TYPE_STANDARD:
		switch (setup->RequestType.recipient) {
		case USB_REQTYPE_RECIPIENT_DEVICE:
			ret = usbd_request_to_device(setup, buf);
			break;
		case USB_REQTYPE_RECIPIENT_INTERFACE:
			ret = usbd_requst_to_iface(setup, buf);
			break;
		case USB_REQTYPE_RECIPIENT_ENDPOINT:
			ret = usbd_requst_to_ep(setup, buf);
			break;
		default:
			ret = -EINVAL;
		}

		break;
	case USB_REQTYPE_TYPE_CLASS:
	case USB_REQTYPE_TYPE_VENDOR:
		ret = usbd_nonstd_request(setup, buf);
		break;
	default:
		LOG_ERR("Wrong request type");
		ret = -EINVAL;
	}

	if (ret != 0) {
		LOG_WRN("USB request unsupported or erroneous");
	}

	if (usb_reqtype_is_to_device(setup) && buf != NULL) {
		net_buf_unref(buf);
	}

	return ret;
}

static void usbd_setup_stage_restart(void)
{
	struct net_buf *buf;

	LOG_DBG("Restart Setup OUT transfer");

	buf = usbd_tbuf_alloc(USB_CONTROL_EP_OUT, sizeof(usbd_ctx.setup));

	if (buf == NULL) {
		LOG_ERR("Failed to restart transfer for setup stage");
		__asm volatile ("bkpt #0\n");
		k_panic();
	}

	usbd_tbuf_submit(buf, true);
}

static int usbd_ack_out_stage(void)
{
	struct net_buf *buf;

	buf = usbd_tbuf_alloc(USB_CONTROL_EP_IN, 0);

	if (buf == NULL) {
		return -ENOMEM;
	}

	usbd_tbuf_submit(buf, true);

	return 0;
}

static int usbd_get_spkt_from_buf(struct net_buf *buf,
				  struct usb_setup_packet *spkt)
{
	if (buf->len < sizeof(struct usb_setup_packet)) {
		return -EINVAL;
	}

	memcpy(spkt, buf->data, sizeof(struct usb_setup_packet));

	/* Take care of endianness */
	spkt->wValue = sys_le16_to_cpu(spkt->wValue);
	spkt->wIndex = sys_le16_to_cpu(spkt->wIndex);
	spkt->wLength = sys_le16_to_cpu(spkt->wLength);

	return 0;
}

static void usbd_control_transfer(struct usbd_class_ctx *cctx,
				  struct net_buf *buf, int err)
{
	struct usb_setup_packet *setup = &usbd_ctx.setup;
	struct usbd_buf_ud *ud = NULL;
	uint8_t ctrl_seq_next = USBD_CTRL_SEQ_ERROR;

	ud = (struct usbd_buf_ud *)net_buf_user_data(buf);
	LOG_WRN("EP 0x%02x, len %u", ud->ep, buf->len);

	if (ud->status == USB_DC_EP_SETUP && ud->ep != USB_CONTROL_EP_OUT) {
		LOG_ERR("Setup Packet from wrong endpoint 0x%02x", ud->ep);
		net_buf_unref(buf);
		goto fatal_error;
	}

	if (ud->status == USB_DC_EP_SETUP) {
		if (usbd_get_spkt_from_buf(buf, setup)) {
			LOG_ERR("Setup Packet Error");
			net_buf_unref(buf);
			goto fatal_error;
		}

		net_buf_unref(buf);
	}

	if (ud->ep == USB_CONTROL_EP_OUT && ud->status == USB_DC_EP_SETUP) {
		if (usbd_ctx.ctrl_stage != USBD_CTRL_SEQ_SETUP) {
			LOG_WRN("Previous sequence %u not completed",
				usbd_ctx.ctrl_stage);
			ctrl_seq_next = USBD_CTRL_SEQ_SETUP;
		}

		/*
		 * Setup Stage has been completed (setup packet received).
		 * Next state depends on the direction and
		 * the value of wLength.
		 */
		if (setup->wLength && usb_reqtype_is_to_device(setup)) {
			/* Prepare Data Stage (OUT) */
			LOG_WRN("s->(out)");
			buf = usbd_tbuf_alloc(USB_CONTROL_EP_OUT,
					      setup->wLength);
			if (buf == NULL) {
				goto fatal_error;
			}

			usbd_tbuf_submit(buf, false);
			ctrl_seq_next = USBD_CTRL_SEQ_DATA_OUT;
		} else if (setup->wLength && usb_reqtype_is_to_host(setup)) {
			LOG_WRN("s->(in)");
			/*
			 * Prepare and start the Data Stage (IN)
			 *
			 * Re-submit endpoint OUT buffer,
			 * IN endpoint buffer will be submited by
			 * usbd_handle_setup()
			 */
			usbd_setup_stage_restart();
			if (usbd_handle_setup(setup, NULL) != 0) {
				/* Fixme, should stall_in */
				goto stall_out;
			}

			ctrl_seq_next = USBD_CTRL_SEQ_DATA_IN;
		} else if (usb_reqtype_is_to_device(setup)) {
			LOG_WRN("s->(ack)");
			/*
			 * No Data Stage
			 *
			 * Re-submit endpoint OUT buffer.
			 */
			usbd_setup_stage_restart();
			if (usbd_handle_setup(setup, NULL) != 0) {
				goto stall_in;
			} else if (usbd_ack_out_stage() != 0) {
				goto fatal_error;
			}

			ctrl_seq_next = USBD_CTRL_SEQ_NO_DATA;
		} else {
			LOG_ERR("Cannot determine the next stage");
			goto fatal_error;
		}

	} else if (ud->ep == USB_CONTROL_EP_OUT) {
		if (usbd_ctx.ctrl_stage == USBD_CTRL_SEQ_DATA_OUT) {
			LOG_WRN("s-out->(ack)");
			/*
			 * Data Stage has been completed,
			 * process setup packet and associated OUT data.
			 * Next sequence is Status Stage (IN ZLP ACK).
			 *
			 * Re-submit endpoint OUT buffer.
			 */
			usbd_setup_stage_restart();
			if (usbd_handle_setup(setup, buf) != 0) {
				goto stall_in;
			} else if (usbd_ack_out_stage() != 0) {
				goto fatal_error;
			}

			ctrl_seq_next = USBD_CTRL_SEQ_STATUS_IN;
		} else if (usbd_ctx.ctrl_stage == USBD_CTRL_SEQ_STATUS_OUT) {
			/*
			 * End of a sequence (setup->in->out), reset state.
			 * Previous Data IN stage was completed and
			 * the host confirmed it with an OUT ZLP.
			 */
			LOG_WRN("s-in-ack");
			if (setup->wLength == 0) {
				ctrl_seq_next = USBD_CTRL_SEQ_SETUP;
			} else {
				LOG_ERR("ZLP expected");
				goto stall_out;
			}
			net_buf_unref(buf);
		} else {
			LOG_ERR("Cannot determine the next stage");
			goto fatal_error;
		}

	} else if (ud->ep == USB_CONTROL_EP_IN) {
		if (usbd_ctx.ctrl_stage == USBD_CTRL_SEQ_STATUS_IN) {
			/*
			 * End of a sequence (setup->out->in), reset state.
			 * Previous Data OUT stage was completed and
			 * we confirmed it with an IN ZLP.
			 */
			LOG_WRN("s-out-ack");
			ctrl_seq_next = USBD_CTRL_SEQ_SETUP;
			net_buf_unref(buf);
		} else if (usbd_ctx.ctrl_stage == USBD_CTRL_SEQ_DATA_IN) {
			/*
			 * Previous Data IN stage was completed.
			 * Next sequence is Status Stage (OUT ZLP ACK).
			 */
			LOG_WRN("s-in->(ack)");
			ctrl_seq_next = USBD_CTRL_SEQ_STATUS_OUT;
			/* unref IN endpoint buffer */
			net_buf_unref(buf);
		} else if (usbd_ctx.ctrl_stage == USBD_CTRL_SEQ_NO_DATA) {
			/*
			 * End of a sequence (setup->in), reset state.
			 * Previous NO Data stage was completed and
			 * we confirmed it with an IN ZLP.
			 */
			LOG_WRN("s-ack");
			ctrl_seq_next = USBD_CTRL_SEQ_SETUP;
			net_buf_unref(buf);
		} else {
			LOG_ERR("Cannot determine the next stage");
			goto fatal_error;
		}
	}

	usbd_ctx.ctrl_stage = ctrl_seq_next;

	return;

fatal_error:
	usb_dc_ep_set_stall(USB_CONTROL_EP_IN);
	usb_dc_ep_set_stall(USB_CONTROL_EP_OUT);
	usbd_ctx.ctrl_stage = USBD_CTRL_SEQ_ERROR;
	usbd_setup_stage_restart();

	return;

stall_out:
	LOG_ERR("stall OUT");
	usb_dc_ep_set_stall(USB_CONTROL_EP_OUT);
	usbd_ctx.ctrl_stage = USBD_CTRL_SEQ_SETUP;

	return;

stall_in:
	LOG_ERR("stall IN");
	usb_dc_ep_set_stall(USB_CONTROL_EP_IN);
	usbd_ctx.ctrl_stage = USBD_CTRL_SEQ_SETUP;

	return;
}

static struct usbd_class_ctx ctrl_pipe = {
	.class_desc = NULL,
	.ops = {
		.req_handler = NULL,
		.ep_cb = usbd_control_transfer,
		.cfg_update = NULL,
		.pm_event = NULL,
	},
	.ep_bm = BIT(16) | BIT(0),
};

int usbd_init_control_ep(void)
{
	int ret;

	struct usb_dc_ep_cfg_data ep_out = {
		.ep_mps = USB_CONTROL_EP_MPS,
		.ep_type = USB_DC_EP_CONTROL,
		.ep_addr = USB_CONTROL_EP_OUT,
	};
	struct usb_dc_ep_cfg_data ep_in = {
		.ep_mps = USB_CONTROL_EP_MPS,
		.ep_type = USB_DC_EP_CONTROL,
		.ep_addr = USB_CONTROL_EP_IN,
	};

	sys_slist_find_and_remove(&usbd_ctx.class_list, &ctrl_pipe.node);
	sys_slist_append(&usbd_ctx.class_list, &ctrl_pipe.node);

	ret = usb_dc_ep_configure(&ep_out);
	if (ret != 0) {
		LOG_ERR("Failed to configure control OUT endpoint");
		return ret;
	};

	ret = usb_dc_ep_set_callback(USB_CONTROL_EP_OUT,
				     usbd_tbuf_ep_cb);
	if (ret != 0) {
		LOG_ERR("Failed to set control OUT endpoint callback");
		return ret;
	};

	ret = usb_dc_ep_configure(&ep_in);
	if (ret != 0) {
		LOG_ERR("Failed to configure control IN endpoint");
		return ret;
	};

	ret = usb_dc_ep_set_callback(USB_CONTROL_EP_IN,
				     usbd_tbuf_ep_cb);
	if (ret != 0) {
		LOG_ERR("Failed to configure control IN endpoint");
		return ret;
	}

	ret = usb_dc_ep_enable(USB_CONTROL_EP_OUT);
	if (ret != 0) {
		LOG_ERR("Failed to start transfer for OUT endpoint");
	}

	ret = usb_dc_ep_enable(USB_CONTROL_EP_IN);
	if (ret != 0) {
		LOG_ERR("Failed to start transfer for OUT endpoint");
	}

	usbd_setup_stage_restart();

	return 0;
}
