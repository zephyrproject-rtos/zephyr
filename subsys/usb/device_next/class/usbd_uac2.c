/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usbd_uac2.h>
#include <zephyr/drivers/usb/udc.h>

#include "usbd_uac2_macros.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_uac2, CONFIG_USBD_UAC2_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_uac2

#define COUNT_UAC2_AS_ENDPOINTS(node)						\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_audio_streaming), (	\
		+ AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node) +			\
		AS_HAS_EXPLICIT_FEEDBACK_ENDPOINT(node)))
#define COUNT_UAC2_ENDPOINTS(i)							\
	+ DT_PROP(DT_DRV_INST(i), interrupt_endpoint)				\
	DT_INST_FOREACH_CHILD(i, COUNT_UAC2_AS_ENDPOINTS)
#define UAC2_NUM_ENDPOINTS DT_INST_FOREACH_STATUS_OKAY(COUNT_UAC2_ENDPOINTS)

/* Net buf is used mostly with external data. The main reason behind external
 * data is avoiding unnecessary isochronous data copy operations.
 *
 * Allow up to 6 bytes per item to facilitate optional interrupt endpoint (which
 * requires 6 bytes) and feedback endpoint (4 bytes on High-Speed, 3 bytes on
 * Full-Speed). Because the total number of endpoints is really small (typically
 * there will be just 2 isochronous endpoints; the upper bound originating from
 * the USB specification itself is 30 non-control endpoints). Therefore, the
 * "wasted memory" here is likely to be smaller than the memory overhead for
 * more complex "only as much as needed" schemes (e.g. heap).
 */
NET_BUF_POOL_DEFINE(uac2_pool, UAC2_NUM_ENDPOINTS, 6,
		    sizeof(struct udc_buf_info), NULL);

/* 5.2.2 Control Request Layout */
#define SET_CLASS_REQUEST_TYPE		0x21
#define GET_CLASS_REQUEST_TYPE		0xA1

/* A.14 Audio Class-Specific Request Codes */
#define CUR				0x01
#define RANGE				0x02
#define MEM				0x03

/* A.17.1 Clock Source Control Selectors */
#define CS_SAM_FREQ_CONTROL		0x01
#define CS_CLOCK_VALID_CONTROL		0x02

#define CONTROL_ATTRIBUTE(setup)	(setup->bRequest)
#define CONTROL_ENTITY_ID(setup)	((setup->wIndex & 0xFF00) >> 8)
#define CONTROL_SELECTOR(setup)		((setup->wValue & 0xFF00) >> 8)
#define CONTROL_CHANNEL_NUMBER(setup)	(setup->wValue & 0x00FF)

typedef enum {
	ENTITY_TYPE_INVALID,
	ENTITY_TYPE_CLOCK_SOURCE,
	ENTITY_TYPE_INPUT_TERMINAL,
	ENTITY_TYPE_OUTPUT_TERMINAL,
} entity_type_t;

static size_t clock_frequencies(struct usbd_class_node *const node,
				const uint8_t id, const uint32_t **frequencies);

/* UAC2 device runtime data */
struct uac2_ctx {
	const struct uac2_ops *ops;
	void *user_data;
	/* Bit set indicates the AudioStreaming interface has non-zero bandwidth
	 * alternate setting active.
	 */
	atomic_t as_active;
	atomic_t as_queued;
	uint32_t fb_queued;
};

/* UAC2 device constant data */
struct uac2_cfg {
	struct usbd_class_node *const node;
	const struct usb_desc_header **descriptors;
	/* Entity 1 type is at entity_types[0] */
	const entity_type_t *entity_types;
	/* Array of indexes to data endpoint descriptor in descriptors set.
	 * First AudioStreaming interface is at ep_indexes[0]. Index is 0 if
	 * the interface is external interface (Type IV), i.e. no endpoint.
	 */
	const uint16_t *ep_indexes;
	/* Same as ep_indexes, but for explicit feedback endpoints. */
	const uint16_t *fb_indexes;
	/* First AudioStreaming interface Terminal ID is at as_terminals[0]. */
	const uint8_t *as_terminals;
	/* Number of interfaces (ep_indexes, fb_indexes and as_terminals size) */
	uint8_t num_ifaces;
	/* Number of entities (entity_type array size) */
	uint8_t num_entities;
};

static entity_type_t id_type(struct usbd_class_node *const node, uint8_t id)
{
	const struct device *dev = node->data->priv;
	const struct uac2_cfg *cfg = dev->config;

	if ((id - 1) < cfg->num_entities) {
		return cfg->entity_types[id - 1];
	}

	return ENTITY_TYPE_INVALID;
}

static const struct usb_ep_descriptor *
get_as_data_ep(struct usbd_class_node *const node, int as_idx)
{
	const struct device *dev = node->data->priv;
	const struct uac2_cfg *cfg = dev->config;
	const struct usb_desc_header *desc = NULL;

	if ((as_idx < cfg->num_ifaces) && cfg->ep_indexes[as_idx]) {
		desc = cfg->descriptors[cfg->ep_indexes[as_idx]];
	}

	return (const struct usb_ep_descriptor *)desc;
}

static const struct usb_ep_descriptor *
get_as_feedback_ep(struct usbd_class_node *const node, int as_idx)
{
	const struct device *dev = node->data->priv;
	const struct uac2_cfg *cfg = dev->config;
	const struct usb_desc_header *desc = NULL;

	if ((as_idx < cfg->num_ifaces) && cfg->fb_indexes[as_idx]) {
		desc = cfg->descriptors[cfg->fb_indexes[as_idx]];
	}

	return (const struct usb_ep_descriptor *)desc;
}

static int ep_to_as_interface(const struct device *dev, uint8_t ep, bool *fb)
{
	const struct uac2_cfg *cfg = dev->config;
	const struct usb_ep_descriptor *desc;

	for (int i = 0; i < cfg->num_ifaces; i++) {
		if (!cfg->ep_indexes[i]) {
			/* If there is no data endpoint there cannot be feedback
			 * endpoint. Simply skip external interfaces.
			 */
			continue;
		}

		desc = get_as_data_ep(cfg->node, i);
		if (desc && (ep == desc->bEndpointAddress)) {
			*fb = false;
			return i;
		}

		desc = get_as_feedback_ep(cfg->node, i);
		if (desc && (ep == desc->bEndpointAddress)) {
			*fb = true;
			return i;
		}
	}

	*fb = false;
	return -ENOENT;
}

static int terminal_to_as_interface(const struct device *dev, uint8_t terminal)
{
	const struct uac2_cfg *cfg = dev->config;

	for (int as_idx = 0; as_idx < cfg->num_ifaces; as_idx++) {
		if (terminal == cfg->as_terminals[as_idx]) {
			return as_idx;
		}
	}

	return -ENOENT;
}

void usbd_uac2_set_ops(const struct device *dev,
		       const struct uac2_ops *ops, void *user_data)
{
	struct uac2_ctx *ctx = dev->data;

	__ASSERT(ops->sof_cb, "SOF callback is mandatory");

	ctx->ops = ops;
	ctx->user_data = user_data;
}

static struct net_buf *
uac2_buf_alloc(const uint8_t ep, void *data, uint16_t size)
{
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;

	buf = net_buf_alloc_with_data(&uac2_pool, data, size, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = ep;

	if (USB_EP_DIR_IS_OUT(ep)) {
		/* Buffer is empty, USB stack will write data from host */
		buf->len = 0;
	}

	return buf;
}

int usbd_uac2_send(const struct device *dev, uint8_t terminal,
		   void *data, uint16_t size)
{
	const struct uac2_cfg *cfg = dev->config;
	struct uac2_ctx *ctx = dev->data;
	struct net_buf *buf;
	const struct usb_ep_descriptor *desc;
	uint8_t ep = 0;
	int as_idx = terminal_to_as_interface(dev, terminal);
	int ret;

	desc = get_as_data_ep(cfg->node, as_idx);
	if (desc) {
		ep = desc->bEndpointAddress;
	}

	if (!ep) {
		LOG_ERR("No endpoint for terminal %d", terminal);
		return -ENOENT;
	}

	if (!atomic_test_bit(&ctx->as_active, as_idx)) {
		/* Host is not interested in the data */
		ctx->ops->buf_release_cb(dev, terminal, data, ctx->user_data);
		return 0;
	}

	if (atomic_test_and_set_bit(&ctx->as_queued, as_idx)) {
		LOG_ERR("Previous send not finished yet on 0x%02x", ep);
		return -EAGAIN;
	}

	buf = uac2_buf_alloc(ep, data, size);
	if (!buf) {
		/* This shouldn't really happen because netbuf should be large
		 * enough, but if it does all we loose is just single packet.
		 */
		LOG_ERR("No netbuf for send");
		atomic_clear_bit(&ctx->as_queued, as_idx);
		ctx->ops->buf_release_cb(dev, terminal, data, ctx->user_data);
		return -ENOMEM;
	}

	ret = usbd_ep_enqueue(cfg->node, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue net_buf for 0x%02x", ep);
		net_buf_unref(buf);
		atomic_clear_bit(&ctx->as_queued, as_idx);
		ctx->ops->buf_release_cb(dev, terminal, data, ctx->user_data);
	}

	return ret;
}

static void schedule_iso_out_read(struct usbd_class_node *const node,
				  uint8_t ep, uint16_t mps, uint8_t terminal)
{
	struct device *dev = node->data->priv;
	const struct uac2_cfg *cfg = dev->config;
	struct uac2_ctx *ctx = dev->data;
	struct net_buf *buf;
	void *data_buf;
	int as_idx = terminal_to_as_interface(dev, terminal);
	int ret;

	/* All calls to this function are internal to class, if terminal is not
	 * associated with interface there is a bug in class implementation.
	 */
	__ASSERT_NO_MSG((as_idx >= 0) && (as_idx < cfg->num_ifaces));
	/* Silence warning if asserts are not enabled */
	ARG_UNUSED(cfg);

	if (!((as_idx >= 0) && atomic_test_bit(&ctx->as_active, as_idx))) {
		/* Host won't send data */
		return;
	}

	if (atomic_test_and_set_bit(&ctx->as_queued, as_idx)) {
		/* Transfer already queued - do not requeue */
		return;
	}

	/* Prepare transfer to read audio OUT data from host */
	data_buf = ctx->ops->get_recv_buf(dev, terminal, mps, ctx->user_data);
	if (!data_buf) {
		LOG_ERR("No data buffer for terminal %d", terminal);
		atomic_clear_bit(&ctx->as_queued, as_idx);
		return;
	}

	buf = uac2_buf_alloc(ep, data_buf, mps);
	if (!buf) {
		LOG_ERR("No netbuf for read");
		/* Netbuf pool should be large enough, but if for some reason
		 * we are out of netbuf, there's nothing better to do than to
		 * pass the buffer back to application.
		 */
		ctx->ops->data_recv_cb(dev, terminal,
				       data_buf, 0, ctx->user_data);
		atomic_clear_bit(&ctx->as_queued, as_idx);
		return;
	}

	ret = usbd_ep_enqueue(node, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue net_buf for 0x%02x", ep);
		net_buf_unref(buf);
		atomic_clear_bit(&ctx->as_queued, as_idx);
	}
}

static void write_explicit_feedback(struct usbd_class_node *const node,
				    uint8_t ep, uint8_t terminal)
{
	struct device *dev = node->data->priv;
	struct uac2_ctx *ctx = dev->data;
	struct net_buf *buf;
	struct udc_buf_info *bi;
	uint32_t fb_value;
	int as_idx = terminal_to_as_interface(dev, terminal);
	int ret;

	buf = net_buf_alloc(&uac2_pool, K_NO_WAIT);
	if (!buf) {
		LOG_ERR("No buf for feedback");
		return;
	}

	bi = udc_get_buf_info(buf);
	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = ep;

	fb_value = ctx->ops->feedback_cb(dev, terminal, ctx->user_data);

	/* REVISE: How to determine operating speed? Should we have separate
	 * class instances for high-speed and full-speed (because high-speed
	 * allows more sampling rates and/or bit depths)?
	 */
	if (udc_device_speed(node->data->uds_ctx->dev) == UDC_BUS_SPEED_FS) {
		net_buf_add_le24(buf, fb_value);
	} else {
		net_buf_add_le32(buf, fb_value);
	}

	ret = usbd_ep_enqueue(node, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue net_buf for 0x%02x", ep);
		net_buf_unref(buf);
	} else {
		ctx->fb_queued |= BIT(as_idx);
	}
}

void uac2_update(struct usbd_class_node *const node,
		 uint8_t iface, uint8_t alternate)
{
	struct device *dev = node->data->priv;
	const struct uac2_cfg *cfg = dev->config;
	struct uac2_ctx *ctx = dev->data;
	const struct usb_association_descriptor *iad;
	const struct usb_ep_descriptor *data_ep, *fb_ep;
	uint8_t as_idx;
	bool microframes;

	LOG_DBG("iface %d alt %d", iface, alternate);

	iad = (const struct usb_association_descriptor *)cfg->descriptors[0];

	/* AudioControl interface (bFirstInterface) doesn't have alternate
	 * configurations, therefore the iface must be AudioStreaming.
	 */
	__ASSERT_NO_MSG((iface > iad->bFirstInterface) &&
			(iface < iad->bFirstInterface + iad->bInterfaceCount));
	as_idx = iface - iad->bFirstInterface - 1;

	/* Audio class is forbidden on Low-Speed, therefore the only possibility
	 * for not using microframes is when device operates at Full-Speed.
	 */
	if (udc_device_speed(node->data->uds_ctx->dev) == UDC_BUS_SPEED_FS) {
		microframes = false;
	} else {
		microframes = true;
	}

	/* Notify application about terminal state change */
	ctx->ops->terminal_update_cb(dev, cfg->as_terminals[as_idx], alternate,
				     microframes, ctx->user_data);

	if (alternate == 0) {
		/* Mark interface as inactive, any pending endpoint transfers
		 * were already cancelled by the USB stack.
		 */
		atomic_clear_bit(&ctx->as_active, as_idx);
		return;
	}

	atomic_set_bit(&ctx->as_active, as_idx);

	data_ep = get_as_data_ep(node, as_idx);
	/* External interfaces (i.e. NULL data_ep) do not have alternate
	 * configuration and therefore data_ep must be valid here.
	 */
	__ASSERT_NO_MSG(data_ep);

	if (USB_EP_DIR_IS_OUT(data_ep->bEndpointAddress)) {
		schedule_iso_out_read(node, data_ep->bEndpointAddress,
				      sys_le16_to_cpu(data_ep->wMaxPacketSize),
				      cfg->as_terminals[as_idx]);

		fb_ep = get_as_feedback_ep(node, as_idx);
		if (fb_ep) {
			write_explicit_feedback(node, fb_ep->bEndpointAddress,
						cfg->as_terminals[as_idx]);
		}
	}
}

/* Table 5-6: 4-byte Control CUR Parameter Block */
static void layout3_cur_response(struct net_buf *const buf, uint16_t length,
				 const uint32_t value)
{
	uint8_t tmp[4];

	/* dCUR */
	sys_put_le32(value, tmp);
	net_buf_add_mem(buf, tmp, MIN(length, 4));
}

/* Table 5-7: 4-byte Control RANGE Parameter Block */
static void layout3_range_response(struct net_buf *const buf, uint16_t length,
				   const uint32_t *min, const uint32_t *max,
				   const uint32_t *res, int n)
{
	uint16_t to_add;
	uint8_t tmp[4];
	int i;
	int item;

	/* wNumSubRanges */
	sys_put_le16(n, tmp);
	to_add = MIN(length, 2);
	net_buf_add_mem(buf, tmp, to_add);
	length -= to_add;

	/* Keep adding dMIN, dMAX, dRES as long as we have entries to add and
	 * we didn't reach wLength response limit.
	 */
	i = item = 0;
	while ((length > 0) && (i < n)) {
		to_add = MIN(length, 4);
		if (item == 0) {
			sys_put_le32(min[i], tmp);
		} else if (item == 1) {
			sys_put_le32(max[i], tmp);
		} else if (item == 2) {
			if (res) {
				sys_put_le32(res[i], tmp);
			} else {
				memset(tmp, 0, 4);
			}
		}
		net_buf_add_mem(buf, tmp, to_add);
		length -= to_add;

		if (++item == 3) {
			item = 0;
			i++;
		}
	}
}

static int get_clock_source_request(struct usbd_class_node *const node,
				    const struct usb_setup_packet *const setup,
				    struct net_buf *const buf)
{
	const uint32_t *frequencies;
	size_t count;

	/* Channel Number must be zero */
	if (CONTROL_CHANNEL_NUMBER(setup) != 0) {
		LOG_DBG("Clock source control with channel %d",
			CONTROL_CHANNEL_NUMBER(setup));
		errno = -EINVAL;
		return 0;
	}

	count = clock_frequencies(node, CONTROL_ENTITY_ID(setup), &frequencies);

	if (CONTROL_SELECTOR(setup) == CS_SAM_FREQ_CONTROL) {
		if (CONTROL_ATTRIBUTE(setup) == CUR) {
			if (count == 1) {
				layout3_cur_response(buf, setup->wLength,
						     frequencies[0]);
				return 0;
			}
			/* TODO: If there is more than one frequency supported,
			 * call registered application API to determine active
			 * sample rate.
			 */
		} else if (CONTROL_ATTRIBUTE(setup) == RANGE) {
			layout3_range_response(buf, setup->wLength, frequencies,
					       frequencies, NULL, count);
			return 0;
		}
	} else {
		LOG_DBG("Unhandled clock control selector 0x%02x",
			CONTROL_SELECTOR(setup));
	}

	errno = -ENOTSUP;
	return 0;
}

static int uac2_control_to_host(struct usbd_class_node *const node,
				const struct usb_setup_packet *const setup,
				struct net_buf *const buf)
{
	entity_type_t entity_type;

	if ((CONTROL_ATTRIBUTE(setup) != CUR) &&
	    (CONTROL_ATTRIBUTE(setup) != RANGE)) {
		errno = -ENOTSUP;
		return 0;
	}

	if (setup->bmRequestType == GET_CLASS_REQUEST_TYPE) {
		entity_type = id_type(node, CONTROL_ENTITY_ID(setup));
		if (entity_type == ENTITY_TYPE_CLOCK_SOURCE) {
			return get_clock_source_request(node, setup, buf);
		}
	}

	errno = -ENOTSUP;
	return 0;
}

static int uac2_request(struct usbd_class_node *const node, struct net_buf *buf,
			int err)
{
	struct device *dev = node->data->priv;
	const struct uac2_cfg *cfg = dev->config;
	struct uac2_ctx *ctx = dev->data;
	struct usbd_contex *uds_ctx = node->data->uds_ctx;
	struct udc_buf_info *bi;
	uint8_t ep, terminal;
	uint16_t mps;
	int as_idx;
	bool is_feedback;

	bi = udc_get_buf_info(buf);
	if (err) {
		if (err == -ECONNABORTED) {
			LOG_WRN("request ep 0x%02x, len %u cancelled",
				bi->ep, buf->len);
		} else {
			LOG_ERR("request ep 0x%02x, len %u failed",
				bi->ep, buf->len);
		}
	}

	mps = buf->size;
	ep = bi->ep;
	as_idx = ep_to_as_interface(dev, ep, &is_feedback);
	__ASSERT_NO_MSG((as_idx >= 0) && (as_idx < cfg->num_ifaces));
	terminal = cfg->as_terminals[as_idx];

	if (is_feedback) {
		ctx->fb_queued &= ~BIT(as_idx);
	} else {
		atomic_clear_bit(&ctx->as_queued, as_idx);
	}

	if (USB_EP_DIR_IS_OUT(ep)) {
		ctx->ops->data_recv_cb(dev, terminal, buf->__buf, buf->len,
				       ctx->user_data);
	} else if (!is_feedback) {
		ctx->ops->buf_release_cb(dev, terminal, buf->__buf, ctx->user_data);
	}

	usbd_ep_buf_free(uds_ctx, buf);
	if (err) {
		return 0;
	}

	/* Reschedule the read or explicit feedback write */
	if (USB_EP_DIR_IS_OUT(ep)) {
		schedule_iso_out_read(node, ep, mps, terminal);
	}

	return 0;
}

static void uac2_sof(struct usbd_class_node *const node)
{
	struct device *dev = node->data->priv;
	const struct usb_ep_descriptor *data_ep;
	const struct usb_ep_descriptor *feedback_ep;
	const struct uac2_cfg *cfg = dev->config;
	struct uac2_ctx *ctx = dev->data;
	int as_idx;

	ctx->ops->sof_cb(dev, ctx->user_data);

	for (as_idx = 0; as_idx < cfg->num_ifaces; as_idx++) {
		/* Make sure OUT endpoint has read request pending. The request
		 * won't be pending only if there was buffer underrun, i.e. the
		 * application failed to supply receive buffer.
		 */
		data_ep = get_as_data_ep(node, as_idx);
		if (data_ep && USB_EP_DIR_IS_OUT(data_ep->bEndpointAddress)) {
			schedule_iso_out_read(node, data_ep->bEndpointAddress,
				sys_le16_to_cpu(data_ep->wMaxPacketSize),
				cfg->as_terminals[as_idx]);
		}

		/* Skip interfaces without explicit feedback endpoint */
		feedback_ep = get_as_feedback_ep(node, as_idx);
		if (feedback_ep == NULL) {
			continue;
		}

		/* We didn't get feedback write request callback yet, skip it
		 * for now to allow faster recovery (i.e. reduce workload to be
		 * done during this frame).
		 */
		if (ctx->fb_queued & BIT(as_idx)) {
			continue;
		}

		/* Only send feedback if host has enabled alternate interface */
		if (!atomic_test_bit(&ctx->as_active, as_idx)) {
			continue;
		}

		/* Make feedback available on every frame (value "sent" in
		 * previous SOF is "gone" even if USB host did not attempt to
		 * read it).
		 */
		write_explicit_feedback(node, feedback_ep->bEndpointAddress,
					cfg->as_terminals[as_idx]);
	}
}

static void *uac2_get_desc(struct usbd_class_node *const node,
			   const enum usbd_speed speed)
{
	struct device *dev = usbd_class_get_private(node);
	const struct uac2_cfg *cfg = dev->config;

	if (speed == USBD_SPEED_FS) {
		return cfg->descriptors;
	}

	return NULL;
}

static int uac2_init(struct usbd_class_node *const node)
{
	struct device *dev = node->data->priv;
	struct uac2_ctx *ctx = dev->data;

	if (ctx->ops == NULL) {
		LOG_ERR("Application did not register UAC2 ops");
		return -EINVAL;
	}

	return 0;
}

struct usbd_class_api uac2_api = {
	.update = uac2_update,
	.control_to_host = uac2_control_to_host,
	.request = uac2_request,
	.sof = uac2_sof,
	.get_desc = uac2_get_desc,
	.init = uac2_init,
};

#define DEFINE_ENTITY_TYPES(node)						\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_clock_source), (	\
		ENTITY_TYPE_CLOCK_SOURCE					\
	))									\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_input_terminal), (	\
		ENTITY_TYPE_INPUT_TERMINAL					\
	))									\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_output_terminal), (	\
		ENTITY_TYPE_OUTPUT_TERMINAL					\
	))									\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_audio_streaming), (	\
		ENTITY_TYPE_INVALID						\
	))									\
	, /* Comma here causes unknown types to fail at compile time */
#define DEFINE_AS_EP_INDEXES(node)						\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_audio_streaming), (	\
		COND_CODE_1(AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node),		\
			(UAC2_DESCRIPTOR_AS_DATA_EP_INDEX(node),), (0,))	\
	))
#define DEFINE_AS_FB_INDEXES(node)						\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_audio_streaming), (	\
		COND_CODE_1(AS_HAS_EXPLICIT_FEEDBACK_ENDPOINT(node),		\
			(UAC2_DESCRIPTOR_AS_FEEDBACK_EP_INDEX(node),), (0,))	\
	))
#define DEFINE_AS_TERMINALS(node)						\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_audio_streaming), (	\
		ENTITY_ID(DT_PROP(node, linked_terminal)),			\
	))

#define FREQUENCY_TABLE_NAME(node, i)						\
	UTIL_CAT(frequencies_##i##_, ENTITY_ID(node))
#define DEFINE_CLOCK_SOURCES(node, i)						\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_clock_source), (	\
		static const uint32_t FREQUENCY_TABLE_NAME(node, i)[] =		\
				DT_PROP(node, sampling_frequencies);		\
	))

#define DEFINE_LOOKUP_TABLES(i)							\
	static const entity_type_t entity_types_##i[] = {			\
		DT_INST_FOREACH_CHILD_STATUS_OKAY(i, DEFINE_ENTITY_TYPES)	\
	};									\
	static const uint16_t ep_indexes_##i[] = {				\
		DT_INST_FOREACH_CHILD_STATUS_OKAY(i, DEFINE_AS_EP_INDEXES)	\
	};									\
	static const uint16_t fb_indexes_##i[] = {				\
		DT_INST_FOREACH_CHILD_STATUS_OKAY(i, DEFINE_AS_FB_INDEXES)	\
	};									\
	static const uint8_t as_terminals_##i[] = {				\
		DT_INST_FOREACH_CHILD_STATUS_OKAY(i, DEFINE_AS_TERMINALS)	\
	};									\
	DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(i, DEFINE_CLOCK_SOURCES, i)

#define DEFINE_UAC2_CLASS_DATA(inst)						\
	DT_INST_FOREACH_CHILD(inst, VALIDATE_NODE)				\
	static struct uac2_ctx uac2_ctx_##inst;					\
	UAC2_DESCRIPTOR_ARRAYS(DT_DRV_INST(inst))				\
	static const struct usb_desc_header *uac2_descriptors_##inst[] = {	\
		UAC2_DESCRIPTOR_PTRS(DT_DRV_INST(inst))				\
	};									\
	static struct usbd_class_data uac2_class_##inst = {			\
		.priv = (void *)DEVICE_DT_GET(DT_DRV_INST(inst)),		\
	};									\
	USBD_DEFINE_CLASS(uac2_##inst, &uac2_api, &uac2_class_##inst);		\
	DEFINE_LOOKUP_TABLES(inst)						\
	static const struct uac2_cfg uac2_cfg_##inst = {			\
		.node = &uac2_##inst,						\
		.descriptors = uac2_descriptors_##inst,				\
		.entity_types = entity_types_##inst,				\
		.ep_indexes = ep_indexes_##inst,				\
		.fb_indexes = fb_indexes_##inst,				\
		.as_terminals = as_terminals_##inst,				\
		.num_ifaces = ARRAY_SIZE(ep_indexes_##inst),			\
		.num_entities = ARRAY_SIZE(entity_types_##inst),		\
	};									\
	BUILD_ASSERT(ARRAY_SIZE(ep_indexes_##inst) <= 32,			\
		"UAC2 implementation supports up to 32 AS interfaces");		\
	BUILD_ASSERT(ARRAY_SIZE(entity_types_##inst) <= 255,			\
		"UAC2 supports up to 255 entities");				\
	DEVICE_DT_DEFINE(DT_DRV_INST(inst), NULL, NULL,				\
		&uac2_ctx_##inst, &uac2_cfg_##inst,				\
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
		NULL);
DT_INST_FOREACH_STATUS_OKAY(DEFINE_UAC2_CLASS_DATA)

static size_t clock_frequencies(struct usbd_class_node *const node,
				const uint8_t id, const uint32_t **frequencies)
{
	const struct device *dev = node->data->priv;
	size_t count;

#define GET_FREQUENCY_TABLE(node, i)						\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_clock_source), (	\
		} else if (id == ENTITY_ID(node)) {				\
			*frequencies = FREQUENCY_TABLE_NAME(node, i);		\
			count = ARRAY_SIZE(FREQUENCY_TABLE_NAME(node, i));	\
	))

	if (0) {
#define SELECT_FREQUENCY_TABLE(i)						\
	} else if (dev == DEVICE_DT_GET(DT_DRV_INST(i))) {			\
		if (0) {							\
			DT_INST_FOREACH_CHILD_VARGS(i, GET_FREQUENCY_TABLE, i)	\
		} else {							\
			*frequencies = NULL;					\
			count = 0;						\
		}
DT_INST_FOREACH_STATUS_OKAY(SELECT_FREQUENCY_TABLE)
	} else {
		*frequencies = NULL;
		count = 0;
	}

	return count;
}
