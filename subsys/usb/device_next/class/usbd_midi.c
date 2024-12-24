/*
 * Copyright (c) 2024 Titouan Christophe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_usb_midi

#include <zephyr/drivers/usb/udc.h>
#include <zephyr/init.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/usb/class/midi.h>
#include <zephyr/usb/class/usbd_midi.h>
#include <zephyr/usb/usbd.h>

#include "usbd_uac2_macros.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_midi, CONFIG_USBD_MIDI_LOG_LEVEL);

#define ALT_USB_MIDI_1 0x00
#define ALT_USB_MIDI_2 0x01

UDC_BUF_POOL_DEFINE(usbd_midi_buf_pool, DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * 2, 512U,
		    sizeof(struct udc_buf_info), NULL);

#define MIDI_QUEUE_SIZE 64

/* midi20 A.1 MS Class-Specific Interface Descriptor Subtypes */
#define MS_DESCRIPTOR_UNDEFINED 0x00
#define MS_HEADER               0x01
#define MIDI_IN_JACK            0x02
#define MIDI_OUT_JACK           0x03
#define ELEMENT                 0x04

/* midi20 A.2 MS Class-Specific Endpoint Descriptor Subtypes */
#define DESCRIPTOR_UNDEFINED 0x00
#define MS_GENERAL           0x01
#define MS_GENERAL_2_0       0x02

/* midi20 A.3 MS Class-Specific Group Terminal Block Descriptor Subtypes */
#define GR_TRM_BLOCK_UNDEFINED 0x00
#define GR_TRM_BLOCK_HEADER    0x01
#define GR_TRM_BLOCK           0x02

/* midi20 A.6 Group Terminal Block Type */
#define GR_TRM_BIDIRECTIONAL 0x00
#define GR_TRM_INPUT_ONLY    0x01
#define GR_TRM_OUTPUT_ONLY   0x02

/* midi20 A.7 Group Terminal Default MIDI Protocol */
#define USE_MIDI_CI                      0x00
#define MIDI_1_0_UP_TO_64_BITS           0x01
#define MIDI_1_0_UP_TO_64_BITS_AND_JRTS  0x02
#define MIDI_1_0_UP_TO_128_BITS          0x03
#define MIDI_1_0_UP_TO_128_BITS_AND_JRTS 0x04
#define MIDI_2_0                         0x11
#define MIDI_2_0_AND_JRTS                0x12

/* midi20: B.2.2 Class-specific AC Interface Descriptor */
struct usb_midi_cs_ac_header_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint16_t bcdADC;
	uint16_t wTotalLength;
	uint8_t bInCollection;
	uint8_t baInterfaceNr1;
} __packed;

/* midi20 5.2.2.1 Class-Specific MS Interface Header Descriptor */
struct usb_midi_header_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint16_t bcdMSC;
	uint16_t wTotalLength;
} __packed;

/* midi20 5.3.2 Class-Specific MIDI Streaming Data Endpoint Descriptor */
struct usb_midi_cs_endpoint_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bNumGrpTrmBlock;
	uint8_t baAssoGrpTrmBlkID[16];
} __packed;

/* midi20 5.4.1 Class Specific Group Terminal Block Header Descriptor */
struct usb_midi_grptrm_header_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint16_t wTotalLength;
} __packed;

/* midi20 5.4.2.1 Group Terminal Block Descriptor */
struct usb_midi_grptrm_block_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bGrpTrmBlkID;
	uint8_t bGrpTrmBlkType;
	uint8_t nGroupTrm;
	uint8_t nNumGroupTrm;
	uint8_t iBlockItem;
	uint8_t bMIDIProtocol;
	uint16_t wMaxInputBandwidth;
	uint16_t wMaxOutputBandwidth;
} __packed;

struct usbd_midi_descriptors {
	struct usb_association_descriptor iad;

	/* Standard AudioControl (AC) Interface Descriptor */
	struct usb_if_descriptor if0_std;
	struct usb_midi_cs_ac_header_descriptor if0_cs;

	/* Empty MidiStreaming 1.0 on altsetting 0 */
	struct usb_if_descriptor if1_0_std;
	struct usb_midi_header_descriptor if1_0_ms_header;
	struct usb_ep_descriptor if1_0_out_ep_fs;
	struct usb_ep_descriptor if1_0_out_ep_hs;
	struct usb_midi_cs_endpoint_descriptor if1_0_cs_out_ep;
	struct usb_ep_descriptor if1_0_in_ep_fs;
	struct usb_ep_descriptor if1_0_in_ep_hs;
	struct usb_midi_cs_endpoint_descriptor if1_0_cs_in_ep;

	/* MidiStreaming 2.0 on altsetting 1 */
	struct usb_if_descriptor if1_1_std;
	struct usb_midi_header_descriptor if1_1_ms_header;
	struct usb_ep_descriptor if1_1_out_ep_fs;
	struct usb_ep_descriptor if1_1_out_ep_hs;
	struct usb_midi_cs_endpoint_descriptor if1_1_cs_out_ep;
	struct usb_ep_descriptor if1_1_in_ep_fs;
	struct usb_ep_descriptor if1_1_in_ep_hs;
	struct usb_midi_cs_endpoint_descriptor if1_1_cs_in_ep;

	/* MidiStreaming 2.0 Class-Specific Group Terminal Block Descriptors
	 * Retrievable by a Separate Get Request
	 */
	struct usb_midi_grptrm_header_descriptor grptrm_header;
	struct usb_midi_grptrm_block_descriptor grptrm_blocks[16];
};

/* Device driver configuration */
struct usbd_midi_config {
	struct usbd_midi_descriptors *desc;
	struct usb_desc_header const **fs_descs;
	struct usb_desc_header const **hs_descs;
};

/* Device driver data */
struct usbd_midi_data {
	struct usbd_class_data *class_data;
	struct k_work rx_work;
	struct k_work tx_work;
	uint8_t tx_queue_buf[MIDI_QUEUE_SIZE];
	struct ring_buf tx_queue;
	uint8_t altsetting;
	usbd_midi_callback cb;
};

static void usbd_midi2_recv(const struct device *dev, struct net_buf *const buf)
{
	struct usbd_midi_data *data = dev->data;
	uint32_t ump[4];

	LOG_HEXDUMP_DBG(buf->data, buf->len, "MIDI2 - Rx DATA");
	while (buf->len >= 4) {
		ump[0] = net_buf_pull_le32(buf);
		for (size_t i = 1; i < UMP_WORDS(ump); i++) {
			ump[i] = net_buf_pull_le32(buf);
		}

		if (data->cb) {
			data->cb(dev, ump);
		}
	}

	if (buf->len) {
		LOG_HEXDUMP_WRN(buf->data, buf->len, "Trailing data in Rx buffer");
	}
}

static int usbd_midi_class_request(struct usbd_class_data *const class_data,
				   struct net_buf *const buf, const int err)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(class_data);
	const struct device *dev = usbd_class_get_private(class_data);
	struct usbd_midi_data *data = dev->data;
	struct udc_buf_info *info = udc_get_buf_info(buf);

	LOG_DBG("USB-MIDI request for %s ep=%d len=%d err=%d", dev->name, info->ep, buf->len, err);

	if (err) {
		LOG_ERR("Class request error: %d", err);
	}
	if (USB_EP_DIR_IS_OUT(info->ep)) {
		usbd_midi2_recv(dev, buf);
		k_work_submit(&data->rx_work);
	} else {
		LOG_HEXDUMP_DBG(buf->data, buf->len, "Tx DATA complete");
		if (ring_buf_size_get(&data->tx_queue)) {
			k_work_submit(&data->tx_work);
		}
	}

	return usbd_ep_buf_free(uds_ctx, buf);
}

static void usbd_midi_class_update(struct usbd_class_data *const class_data,
				   const uint8_t iface, const uint8_t alternate)
{
	const struct device *dev = usbd_class_get_private(class_data);
	struct usbd_midi_data *data = dev->data;

	LOG_DBG("update for %s: if=%u, alt=%u", dev->name, iface, alternate);

	switch (alternate) {
	case ALT_USB_MIDI_1:
		data->altsetting = ALT_USB_MIDI_1;
		LOG_WRN("%s set USB-MIDI1.0 altsetting (not implemented !)", dev->name);
		break;
	case ALT_USB_MIDI_2:
		data->altsetting = ALT_USB_MIDI_2;
		LOG_INF("%s set USB-MIDI2.0 altsetting", dev->name);
		break;
	}
}

static void usbd_midi_class_enable(struct usbd_class_data *const class_data)
{
	const struct device *dev = usbd_class_get_private(class_data);
	struct usbd_midi_data *data = dev->data;

	LOG_DBG("USB-MIDI enable for %s", dev->name);
	k_work_submit(&data->rx_work);
}

static void usbd_midi_class_disable(struct usbd_class_data *const class_data)
{
	const struct device *dev = usbd_class_get_private(class_data);
	struct usbd_midi_data *data = dev->data;

	LOG_DBG("USB-MIDI disable for %s", dev->name);
	k_work_cancel(&data->rx_work);
}

static void usbd_midi_class_suspended(struct usbd_class_data *const class_data)
{
	const struct device *dev = usbd_class_get_private(class_data);
	struct usbd_midi_data *data = dev->data;

	LOG_DBG("USB-MIDI suspended for %s", dev->name);
	k_work_cancel(&data->rx_work);
}

static void usbd_midi_class_resumed(struct usbd_class_data *const class_data)
{
	const struct device *dev = usbd_class_get_private(class_data);
	struct usbd_midi_data *data = dev->data;

	LOG_DBG("USB-MIDI resumed for %s", dev->name);
	k_work_submit(&data->rx_work);
}

static int usbd_midi_class_cth(struct usbd_class_data *const class_data,
			       const struct usb_setup_packet *const setup,
			       struct net_buf *const buf)
{
	const struct device *dev = usbd_class_get_private(class_data);
	const struct usbd_midi_config *config = dev->config;
	struct usbd_midi_data *data = dev->data;

	LOG_DBG("USB-MIDI control to host for %s", dev->name);
	LOG_DBG("  bmRequestType=%02X bRequest=%02X wValue=%04X wIndex=%04X wLength=%04X",
		setup->bmRequestType, setup->bRequest, setup->wValue, setup->wIndex,
		setup->wLength);

	/* midi20 6. Class Specific Command: Group Terminal Blocks Descriptors Request */
	if (data->altsetting == ALT_USB_MIDI_2 && setup->bRequest == USB_SREQ_GET_DESCRIPTOR) {
		if (setup->wValue != ((CS_GR_TRM_BLOCK << 8) | ALT_USB_MIDI_2)) {
			errno = -ENOTSUP;
			return 0;
		}

		size_t head_len = config->desc->grptrm_header.bLength;
		size_t total_len = sys_le16_to_cpu(config->desc->grptrm_header.wTotalLength);

		net_buf_add_mem(buf, (void *) &config->desc->grptrm_header,
				MIN(head_len, setup->wLength));

		if (setup->wLength > head_len) {
			net_buf_add_mem(buf, (void *) &config->desc->grptrm_blocks,
					MIN(total_len, setup->wLength) - head_len);
		}
		LOG_HEXDUMP_DBG(buf->data, buf->len, "Control to host");
	}

	return 0;
}

static int usbd_midi_class_init(struct usbd_class_data *const class_data)
{
	const struct device *dev = usbd_class_get_private(class_data);

	LOG_DBG("Init USB-MIDI device class for %s", dev->name);

	return 0;
}

static void *usbd_midi_class_get_desc(struct usbd_class_data *const class_data,
				      const enum usbd_speed speed)
{
	const struct device *dev = usbd_class_get_private(class_data);
	const struct usbd_midi_config *config = dev->config;

	LOG_DBG("Get descriptors for %s", dev->name);

	return (speed == USBD_SPEED_HS) ? config->hs_descs : config->fs_descs;
}


static struct usbd_class_api usbd_midi_class_api = {
	.request = usbd_midi_class_request,
	.update = usbd_midi_class_update,
	.enable = usbd_midi_class_enable,
	.disable = usbd_midi_class_disable,
	.suspended = usbd_midi_class_suspended,
	.resumed = usbd_midi_class_resumed,
	.control_to_host = usbd_midi_class_cth,
	.init = usbd_midi_class_init,
	.get_desc = usbd_midi_class_get_desc,
};

static struct net_buf *usbd_midi_buf_alloc(uint8_t ep)
{
	struct udc_buf_info *info;
	struct net_buf *buf;

	buf = net_buf_alloc(&usbd_midi_buf_pool, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	info = udc_get_buf_info(buf);
	info->ep = ep;

	return buf;
}

static inline uint8_t usbd_midi_get_ep(struct usbd_class_data *const class_data,
				       uint8_t direction)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(class_data);
	const struct device *dev = usbd_class_get_private(class_data);
	const struct usbd_midi_config *cfg = dev->config;

	return (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS)
		? (direction == USB_EP_DIR_IN)
			? cfg->desc->if1_1_in_ep_hs.bEndpointAddress
			: cfg->desc->if1_1_out_ep_hs.bEndpointAddress
		: (direction == USB_EP_DIR_IN)
			? cfg->desc->if1_1_in_ep_fs.bEndpointAddress
			: cfg->desc->if1_1_out_ep_fs.bEndpointAddress;
}

static void usbd_midi_rx_work(struct k_work *work)
{
	int ret;
	struct net_buf *buf;
	struct usbd_midi_data *data = CONTAINER_OF(work, struct usbd_midi_data, rx_work);

	buf = usbd_midi_buf_alloc(usbd_midi_get_ep(data->class_data, USB_EP_DIR_OUT));
	if (buf == NULL) {
		LOG_WRN("Unable to allocate Rx net_buf");
		return;
	}

	LOG_DBG("Enqueue Rx...");
	ret = usbd_ep_enqueue(data->class_data, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue Rx net_buf -> %d", ret);
		net_buf_unref(buf);
	}
}

static void usbd_midi_tx_work(struct k_work *work)
{
	int ret;
	struct net_buf *buf;
	struct usbd_midi_data *data = CONTAINER_OF(work, struct usbd_midi_data, tx_work);

	buf = usbd_midi_buf_alloc(usbd_midi_get_ep(data->class_data, USB_EP_DIR_IN));
	if (buf == NULL) {
		LOG_ERR("Unable to allocate Tx net_buf");
		return;
	}

	net_buf_add(buf, ring_buf_get(&data->tx_queue, buf->data, buf->size));
	LOG_HEXDUMP_DBG(buf->data, buf->len, "MIDI2 - Tx DATA");

	ret = usbd_ep_enqueue(data->class_data, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue Tx net_buf -> %d", ret);
		net_buf_unref(buf);
	}
}

static int usbd_midi_preinit(const struct device *dev)
{
	struct usbd_midi_data *data = dev->data;

	LOG_DBG("Init USB-MIDI device %s", dev->name);
	ring_buf_init(&data->tx_queue, MIDI_QUEUE_SIZE, data->tx_queue_buf);
	k_work_init(&data->rx_work, usbd_midi_rx_work);
	k_work_init(&data->tx_work, usbd_midi_tx_work);

	return 0;
}

int usbd_midi_send(const struct device *dev, const uint32_t ump[static 4])
{
	struct usbd_midi_data *data = dev->data;
	size_t words = UMP_WORDS(ump);
	size_t buflen = 4 * words;
	int ret = 0;
	uint32_t word;

	LOG_DBG("Send MT=%X group=%X", UMP_MT(ump), UMP_GROUP(ump));
	if (data->altsetting != ALT_USB_MIDI_2) {
		LOG_WRN("MIDI2.0 is not enabled");
		return -EIO;
	}

	if (buflen > ring_buf_space_get(&data->tx_queue)) {
		LOG_WRN("Not enough space in tx queue");
		ret = -EAGAIN;
	} else {
		for (size_t i = 0; i < words; i++) {
			word = sys_cpu_to_le32(ump[i]);
			ring_buf_put(&data->tx_queue, (const uint8_t *)&word, 4);
		}
		k_work_submit(&data->tx_work);
	}

	return ret;
}

void usbd_midi_set_callback(const struct device *dev, const usbd_midi_callback cb)
{
	struct usbd_midi_data *data = dev->data;

	LOG_DBG("Set callback for %s to %p", dev->name, cb);
	data->cb = cb;
}

/* Group Terminal Block unique identification number, type and protocol
 * see midi20 5.4.2 Group Terminal Block Descriptor
 */
#define GRPTRM_BLOCK_ID(node) UTIL_INC(DT_NODE_CHILD_IDX(node))
#define GRPTRM_BLOCK_TYPE(node)                                                   \
	COND_CODE_1(DT_ENUM_HAS_VALUE(node, terminal_type, input_only),           \
		(GR_TRM_INPUT_ONLY),                                              \
		(COND_CODE_1(DT_ENUM_HAS_VALUE(node, terminal_type, output_only), \
			(GR_TRM_OUTPUT_ONLY),                                     \
			(GR_TRM_BIDIRECTIONAL)                                    \
		))                                                                \
	)
#define GRPTRM_PROTOCOL(node)                                                                     \
	COND_CODE_1(DT_ENUM_HAS_VALUE(node, protocol, midi2),                                     \
		(MIDI_2_0),                                                                       \
		(COND_CODE_1(DT_ENUM_HAS_VALUE(node, protocol, midi1_up_to_64b),                  \
				(MIDI_1_0_UP_TO_64_BITS),                                         \
				(COND_CODE_1(DT_ENUM_HAS_VALUE(node, protocol, midi1_up_to_128b), \
					(MIDI_1_0_UP_TO_128_BITS),                                \
					(USE_MIDI_CI)                                             \
				))                                                                \
			))                                                                        \
		)

/* Group Terminal Block unique identification number with a trailing comma
 * if that block is bidirectional or of given terminal type; otherwise empty
 */
#define GRPTRM_BLOCK_ID_SEP_IF(node, ttype)                                    \
	COND_CODE_1(                                                           \
		UTIL_OR(DT_ENUM_HAS_VALUE(node, terminal_type, bidirectional), \
			DT_ENUM_HAS_VALUE(node, terminal_type, ttype)),        \
		(GRPTRM_BLOCK_ID(node),),                                      \
		())

/* All unique identification numbers of output+bidir group terminal blocks */
#define GRPTRM_OUTPUT_BLOCK_IDS(inst) \
	DT_INST_FOREACH_CHILD_VARGS(inst, GRPTRM_BLOCK_ID_SEP_IF, output_only)

/* All unique identification numbers of input+bidir group terminal blocks */
#define GRPTRM_INPUT_BLOCK_IDS(inst) \
	DT_INST_FOREACH_CHILD_VARGS(inst, GRPTRM_BLOCK_ID_SEP_IF, input_only)

#define N_INPUTS(inst)  sizeof((uint8_t[]){GRPTRM_INPUT_BLOCK_IDS(inst)})
#define N_OUTPUTS(inst) sizeof((uint8_t[]){GRPTRM_OUTPUT_BLOCK_IDS(inst)})

#define USBD_MIDI_VALIDATE_GRPTRM_BLOCK(node)                              \
	BUILD_ASSERT(DT_REG_ADDR(node) < 16,                               \
		     "Group Terminal Block address must be within 0..15"); \
	BUILD_ASSERT(DT_REG_ADDR(node) + DT_REG_SIZE(node) <= 16,          \
		     "Too many Group Terminals in this Block");

#define USBD_MIDI_VALIDATE_INSTANCE(inst) \
	DT_INST_FOREACH_CHILD(inst, USBD_MIDI_VALIDATE_GRPTRM_BLOCK)

#define USBD_MIDI2_INIT_GRPTRM_BLOCK_DESCRIPTOR(node)                       \
	{                                                                   \
		.bLength = sizeof(struct usb_midi_grptrm_block_descriptor), \
		.bDescriptorType = CS_GR_TRM_BLOCK,                         \
		.bDescriptorSubtype = GR_TRM_BLOCK,                         \
		.bGrpTrmBlkID = GRPTRM_BLOCK_ID(node),                      \
		.bGrpTrmBlkType = GRPTRM_BLOCK_TYPE(node),                  \
		.nGroupTrm = DT_REG_ADDR(node),                             \
		.nNumGroupTrm = DT_REG_SIZE(node),                          \
		.iBlockItem = 0,                                            \
		.bMIDIProtocol = GRPTRM_PROTOCOL(node),                     \
		.wMaxInputBandwidth = 0x0000,                               \
		.wMaxOutputBandwidth = 0x0000,                              \
	}

#define USBD_MIDI2_GRPTRM_TOTAL_LEN(inst)                 \
	sizeof(struct usb_midi_grptrm_header_descriptor)  \
	+ DT_INST_CHILD_NUM_STATUS_OKAY(inst)             \
	* sizeof(struct usb_midi_grptrm_block_descriptor)

#define USBD_MIDI_DEFINE_DESCRIPTORS(inst)                                               \
	static struct usbd_midi_descriptors usbd_midi_desc_##inst = {                    \
		.iad = {                                                                 \
			.bLength = sizeof(struct usb_association_descriptor),            \
			.bDescriptorType = USB_DESC_INTERFACE_ASSOC,                     \
			.bFirstInterface = 0,                                            \
			.bInterfaceCount = 2,                                            \
			.bFunctionClass = AUDIO,                                         \
			.bFunctionSubClass = MIDISTREAMING,                              \
		},                                                                       \
		.if0_std = {                                                             \
			.bLength = sizeof(struct usb_if_descriptor),                     \
			.bDescriptorType = USB_DESC_INTERFACE,                           \
			.bInterfaceNumber = 0,                                           \
			.bAlternateSetting = 0,                                          \
			.bNumEndpoints = 0,                                              \
			.bInterfaceClass = AUDIO,                                        \
			.bInterfaceSubClass = AUDIOCONTROL,                              \
		},                                                                       \
		.if0_cs = {                                                              \
			.bLength = sizeof(struct usb_midi_cs_ac_header_descriptor),      \
			.bDescriptorType = CS_INTERFACE,                                 \
			.bDescriptorSubtype = MS_HEADER,                                 \
			.bcdADC = sys_cpu_to_le16(0x0100),                               \
			.wTotalLength = sizeof(struct usb_midi_cs_ac_header_descriptor), \
			.bInCollection = 1,                                              \
			.baInterfaceNr1 = 1,                                             \
		},                                                                       \
		.if1_0_std = {                                                           \
			.bLength = sizeof(struct usb_if_descriptor),                     \
			.bDescriptorType = USB_DESC_INTERFACE,                           \
			.bInterfaceNumber = 1,                                           \
			.bAlternateSetting = ALT_USB_MIDI_1,                             \
			.bNumEndpoints = 2,                                              \
			.bInterfaceClass = AUDIO,                                        \
			.bInterfaceSubClass = MIDISTREAMING,                             \
		},                                                                       \
		.if1_0_ms_header = {                                                     \
			.bLength = sizeof(struct usb_midi_header_descriptor),            \
			.bDescriptorType = CS_INTERFACE,                                 \
			.bDescriptorSubtype = MS_HEADER,                                 \
			.bcdMSC = sys_cpu_to_le16(0x0100),                               \
			.wTotalLength = sys_cpu_to_le16(                                 \
				sizeof(struct usb_midi_header_descriptor)                \
				+ 2 * (sizeof(struct usb_ep_descriptor) + 4)             \
			),                                                               \
		},                                                                       \
		.if1_0_out_ep_fs = {                                                     \
			.bLength = sizeof(struct usb_ep_descriptor),                     \
			.bDescriptorType = USB_DESC_ENDPOINT,                            \
			.bEndpointAddress = inst + FIRST_OUT_EP_ADDR,                    \
			.bmAttributes = USB_EP_TYPE_BULK,                                \
			.wMaxPacketSize = sys_cpu_to_le16(64U),                          \
		},                                                                       \
		.if1_0_out_ep_hs = {                                                     \
			.bLength = sizeof(struct usb_ep_descriptor),                     \
			.bDescriptorType = USB_DESC_ENDPOINT,                            \
			.bEndpointAddress = inst + FIRST_OUT_EP_ADDR,                    \
			.bmAttributes = USB_EP_TYPE_BULK,                                \
			.wMaxPacketSize = sys_cpu_to_le16(512U),                         \
		},                                                                       \
		.if1_0_cs_out_ep = {                                                     \
			.bLength = 4,                                                    \
			.bDescriptorType = CS_ENDPOINT,                                  \
			.bDescriptorSubtype = MS_GENERAL,                                \
			.bNumGrpTrmBlock = 0,                                            \
		},                                                                       \
		.if1_0_in_ep_fs = {                                                      \
			.bLength = sizeof(struct usb_ep_descriptor),                     \
			.bDescriptorType = USB_DESC_ENDPOINT,                            \
			.bEndpointAddress = inst + FIRST_IN_EP_ADDR,                     \
			.bmAttributes = USB_EP_TYPE_BULK,                                \
			.wMaxPacketSize = sys_cpu_to_le16(64U),                          \
		},                                                                       \
		.if1_0_in_ep_hs = {                                                      \
			.bLength = sizeof(struct usb_ep_descriptor),                     \
			.bDescriptorType = USB_DESC_ENDPOINT,                            \
			.bEndpointAddress = inst + FIRST_IN_EP_ADDR,                     \
			.bmAttributes = USB_EP_TYPE_BULK,                                \
			.wMaxPacketSize = sys_cpu_to_le16(512U),                         \
		},                                                                       \
		.if1_0_cs_in_ep = {                                                      \
			.bLength = 4 + N_INPUTS(inst),                                   \
			.bDescriptorType = CS_ENDPOINT,                                  \
			.bDescriptorSubtype = MS_GENERAL,                                \
			.bNumGrpTrmBlock = 0,                                            \
		},                                                                       \
		.if1_1_std = {                                                           \
			.bLength = sizeof(struct usb_if_descriptor),                     \
			.bDescriptorType = USB_DESC_INTERFACE,                           \
			.bInterfaceNumber = 1,                                           \
			.bAlternateSetting = ALT_USB_MIDI_2,                             \
			.bNumEndpoints = 2,                                              \
			.bInterfaceClass = AUDIO,                                        \
			.bInterfaceSubClass = MIDISTREAMING,                             \
		},                                                                       \
		.if1_1_ms_header = {                                                     \
			.bLength = sizeof(struct usb_midi_header_descriptor),            \
			.bDescriptorType = CS_INTERFACE,                                 \
			.bDescriptorSubtype = MS_HEADER,                                 \
			.bcdMSC = sys_cpu_to_le16(0x0200),                               \
			.wTotalLength = sys_cpu_to_le16(                                 \
				sizeof(struct usb_midi_header_descriptor)),              \
		},                                                                       \
		.if1_1_out_ep_fs = {                                                     \
			.bLength = sizeof(struct usb_ep_descriptor),                     \
			.bDescriptorType = USB_DESC_ENDPOINT,                            \
			.bEndpointAddress = inst + FIRST_OUT_EP_ADDR,                    \
			.bmAttributes = USB_EP_TYPE_BULK,                                \
			.wMaxPacketSize = sys_cpu_to_le16(64U),                          \
		},                                                                       \
		.if1_1_out_ep_hs = {                                                     \
			.bLength = sizeof(struct usb_ep_descriptor),                     \
			.bDescriptorType = USB_DESC_ENDPOINT,                            \
			.bEndpointAddress = inst + FIRST_OUT_EP_ADDR,                    \
			.bmAttributes = USB_EP_TYPE_BULK,                                \
			.wMaxPacketSize = sys_cpu_to_le16(512U),                         \
		},                                                                       \
		.if1_1_cs_out_ep = {                                                     \
			.bLength = 4 + N_OUTPUTS(inst),                                  \
			.bDescriptorType = CS_ENDPOINT,                                  \
			.bDescriptorSubtype = MS_GENERAL_2_0,                            \
			.bNumGrpTrmBlock = N_OUTPUTS(inst),                              \
			.baAssoGrpTrmBlkID = {GRPTRM_OUTPUT_BLOCK_IDS(inst)},            \
		},                                                                       \
		.if1_1_in_ep_fs = {                                                      \
			.bLength = sizeof(struct usb_ep_descriptor),                     \
			.bDescriptorType = USB_DESC_ENDPOINT,                            \
			.bEndpointAddress = inst + FIRST_IN_EP_ADDR,                     \
			.bmAttributes = USB_EP_TYPE_BULK,                                \
			.wMaxPacketSize = sys_cpu_to_le16(64U),                          \
		},                                                                       \
		.if1_1_in_ep_hs = {                                                      \
			.bLength = sizeof(struct usb_ep_descriptor),                     \
			.bDescriptorType = USB_DESC_ENDPOINT,                            \
			.bEndpointAddress = inst + FIRST_IN_EP_ADDR,                     \
			.bmAttributes = USB_EP_TYPE_BULK,                                \
			.wMaxPacketSize = sys_cpu_to_le16(512U),                         \
		},                                                                       \
		.if1_1_cs_in_ep = {                                                      \
			.bLength = 4 + N_INPUTS(inst),                                   \
			.bDescriptorType = CS_ENDPOINT,                                  \
			.bDescriptorSubtype = MS_GENERAL_2_0,                            \
			.bNumGrpTrmBlock = N_INPUTS(inst),                               \
			.baAssoGrpTrmBlkID = {GRPTRM_INPUT_BLOCK_IDS(inst)},             \
		},                                                                       \
		.grptrm_header = {                                                       \
			.bLength = sizeof(struct usb_midi_grptrm_header_descriptor),     \
			.bDescriptorType = CS_GR_TRM_BLOCK,                              \
			.bDescriptorSubtype = GR_TRM_BLOCK_HEADER,                       \
			.wTotalLength = sys_cpu_to_le16(                                 \
				USBD_MIDI2_GRPTRM_TOTAL_LEN(inst)                        \
			),                                                               \
		},                                                                       \
		.grptrm_blocks = {                                                       \
			DT_INST_FOREACH_CHILD_SEP(                                       \
				inst, USBD_MIDI2_INIT_GRPTRM_BLOCK_DESCRIPTOR, (,)       \
			)                                                                \
		},                                                                       \
	};                                                                               \
	static const struct usb_desc_header *usbd_midi_desc_array_fs_##inst[] = {        \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.iad,                    \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if0_std,                \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if0_cs,                 \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_0_std,              \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_0_ms_header,        \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_0_out_ep_fs,        \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_0_cs_out_ep,        \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_0_in_ep_fs,         \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_0_cs_in_ep,         \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_1_std,              \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_1_ms_header,        \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_1_out_ep_fs,        \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_1_cs_out_ep,        \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_1_in_ep_fs,         \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_1_cs_in_ep,         \
	};                                                                               \
	static const struct usb_desc_header *usbd_midi_desc_array_hs_##inst[] = {        \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.iad,                    \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if0_std,                \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if0_cs,                 \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_0_std,              \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_0_ms_header,        \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_0_out_ep_hs,        \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_0_cs_out_ep,        \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_0_in_ep_hs,         \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_0_cs_in_ep,         \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_1_std,              \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_1_ms_header,        \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_1_out_ep_hs,        \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_1_cs_out_ep,        \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_1_in_ep_hs,         \
		(struct usb_desc_header *)&usbd_midi_desc_##inst.if1_1_cs_in_ep,         \
	};

#define USBD_MIDI_DEFINE_DEVICE(inst)                                           \
	USBD_MIDI_VALIDATE_INSTANCE(inst)                                       \
	USBD_MIDI_DEFINE_DESCRIPTORS(inst);                                     \
	USBD_DEFINE_CLASS(midi_##inst, &usbd_midi_class_api,                    \
			  (void *)DEVICE_DT_GET(DT_DRV_INST(inst)), NULL);      \
	static const struct usbd_midi_config usbd_midi_config_##inst = {        \
		.desc = &usbd_midi_desc_##inst,                                 \
		.fs_descs = usbd_midi_desc_array_fs_##inst,                     \
		.hs_descs = usbd_midi_desc_array_hs_##inst,                     \
	};                                                                      \
	static struct usbd_midi_data usbd_midi_data_##inst = {                  \
		.class_data = &midi_##inst,                                     \
		.altsetting = ALT_USB_MIDI_1,                                   \
	};                                                                      \
	DEVICE_DT_INST_DEFINE(inst, usbd_midi_preinit, NULL,                    \
			      &usbd_midi_data_##inst, &usbd_midi_config_##inst, \
			      POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(USBD_MIDI_DEFINE_DEVICE)
