/*
 * Copyright (c) 2024 Titouan Christophe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_midi2_device

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/class/usbd_midi2.h>
#include <zephyr/usb/usbd.h>
#include <errno.h>

#include "usbd_uac2_macros.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_midi2, CONFIG_USBD_MIDI2_LOG_LEVEL);

#define MIDI1_ALTERNATE 0x00
#define MIDI2_ALTERNATE 0x01

UDC_BUF_POOL_DEFINE(usbd_midi_buf_pool, DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * 2, 512U,
		    sizeof(struct udc_buf_info), NULL);

/* midi20 A.1 MS Class-Specific Interface Descriptor Types */
#define CS_GR_TRM_BLOCK	0x26
/* midi20 A.1 MS Class-Specific Interface Descriptor Subtypes */
#define MS_HEADER	0x01

/* midi20 A.2 MS Class-Specific Endpoint Descriptor Subtypes */
#define MS_GENERAL	0x01
#define MS_GENERAL_2_0	0x02

/* midi20 A.3 MS Class-Specific Group Terminal Block Descriptor Subtypes */
#define GR_TRM_BLOCK_HEADER	0x01
#define GR_TRM_BLOCK		0x02

/* midi20 A.6 Group Terminal Block Type */
#define GR_TRM_BIDIRECTIONAL	0x00
#define GR_TRM_INPUT_ONLY	0x01
#define GR_TRM_OUTPUT_ONLY	0x02

/* midi20 A.7 Group Terminal Default MIDI Protocol */
#define USE_MIDI_CI			0x00
#define MIDI_1_0_UP_TO_64_BITS		0x01
#define MIDI_1_0_UP_TO_64_BITS_JRTS	0x02
#define MIDI_1_0_UP_TO_128_BITS		0x03
#define MIDI_1_0_UP_TO_128_BITS_JRTS	0x04
#define MIDI_2_0			0x11
#define MIDI_2_0_JRTS			0x12

/* midi10 Table 4-1 Code Index Number values */
#define MIDI_CIN_MISC			0x0
#define MIDI_CIN_CABLE_EVENT		0x1
#define MIDI_CIN_SYS_COMMON_2BYTE	0x2
#define MIDI_CIN_SYS_COMMON_3BYTE	0x3
#define MIDI_CIN_SYSEX_START		0x4
#define MIDI_CIN_SYSEX_END_1BYTE	0x5
#define MIDI_CIN_SYSEX_END_2BYTE	0x6
#define MIDI_CIN_SYSEX_END_3BYTE	0x7
#define MIDI_CIN_NOTE_OFF		0x8
#define MIDI_CIN_NOTE_ON		0x9
#define MIDI_CIN_POLY_KEYPRESS		0xA
#define MIDI_CIN_CONTROL_CHANGE		0xB
#define MIDI_CIN_PROGRAM_CHANGE		0xC
#define MIDI_CIN_CHANNEL_PRESSURE	0xD
#define MIDI_CIN_PITCH_BEND_CHANGE	0xE
#define MIDI_CIN_SINGLE_BYTE		0xF

/* MIDI 1.0 status bytes used internally to classify USB-MIDI 1.0 events.
 * see midi10 Appendix 1.1 Table of MIDI 1.0 Messages
 */
#define MIDI_STATUS_NOTE_OFF         0x80
#define MIDI_STATUS_NOTE_ON          0x90
#define MIDI_STATUS_POLY_KEYPRESS    0xA0
#define MIDI_STATUS_CONTROL_CHANGE   0xB0
#define MIDI_STATUS_PROGRAM_CHANGE   0xC0
#define MIDI_STATUS_CHANNEL_PRESSURE 0xD0
#define MIDI_STATUS_PITCH_BEND       0xE0
#define MIDI_STATUS_SYSEX_START      0xF0
#define MIDI_STATUS_TIME_CODE        0xF1
#define MIDI_STATUS_SONG_POS         0xF2
#define MIDI_STATUS_SONG_SELECT      0xF3
#define MIDI_STATUS_TUNE_REQUEST     0xF6
#define MIDI_STATUS_SYSEX_END        0xF7
#define MIDI_STATUS_TIMING_CLOCK     0xF8

/* midi10 6.1 MIDI Streaming Class-Specific Descriptor constants */
#define MIDI1_IN_JACK		0x02
#define MIDI1_OUT_JACK		0x03
#define MIDI1_JACK_EMBEDDED	0x01
#define MIDI1_JACK_EXTERNAL	0x02

/* Maximum number of USB-MIDI 1.0 cables (and therefore MIDI 2.0 groups)
 * exposed by a single class instance.
 */
#define USBD_MIDI_MAX_CABLES 16

/* USB-MIDI 1.0 jack ID allocation: each MIDI 2.0 Group Terminal Block (the DT
 * child node) is mapped to up to four jacks on the MIDI 1.0 alternate. The IDs
 * are deterministic but reserved unconditionally so that DT child indexing is
 * stable regardless of which jacks each block actually exposes.
 */
#define MIDI1_EMB_IN_JACK_ID(node)  (4 * DT_NODE_CHILD_IDX(node) + 1)
#define MIDI1_EXT_OUT_JACK_ID(node) (4 * DT_NODE_CHILD_IDX(node) + 2)
#define MIDI1_EXT_IN_JACK_ID(node)  (4 * DT_NODE_CHILD_IDX(node) + 3)
#define MIDI1_EMB_OUT_JACK_ID(node) (4 * DT_NODE_CHILD_IDX(node) + 4)

#define MIDI1_EVENT_BYTES 4

/* ump128 7.7.2 System Exclusive 7-bit Data Messages */
#define SYSEX_STATUS_COMPLETE	0x00
#define SYSEX_STATUS_START	0x01
#define SYSEX_STATUS_CONTINUE	0x02
#define SYSEX_STATUS_END	0x03

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

/* midi10 6.1.2 Class-Specific Bulk Endpoint Descriptor */
struct usb_midi1_cs_endpoint_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bNumEmbMIDIJack;
	uint8_t baAssocJackID[USBD_MIDI_MAX_CABLES];
} __packed;

/* midi20 5.3.2 Class-Specific MIDI Streaming Data Endpoint Descriptor */
struct usb_midi_cs_endpoint_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bNumGrpTrmBlock;
	uint8_t baAssoGrpTrmBlkID[16];
} __packed;

/* midi10 6.1.1 MIDI IN Jack Descriptor */
struct usb_midi_in_jack_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bJackType;
	uint8_t bJackID;
	uint8_t iJack;
} __packed;

/* midi10 6.1.2 MIDI OUT Jack Descriptor (single input pin) */
struct usb_midi_out_jack_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bJackType;
	uint8_t bJackID;
	uint8_t bNrInputPins;
	uint8_t baSourceID;
	uint8_t baSourcePin;
	uint8_t iJack;
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

	/* MidiStreaming 1.0 on altsetting 0.
	 *
	 * The four jack arrays are indexed by the corresponding MIDI 2.0 Group
	 * Terminal Block's DT child index. Slots for blocks that don't have a
	 * given jack type (for example an "input-only" GTB has no embedded IN
	 * jack) are zero-initialised and are not referenced from the descriptor
	 * pointer arrays.
	 */
	struct usb_if_descriptor if1_0_std;
	struct usb_midi_header_descriptor if1_0_ms_header;
	struct usb_midi_in_jack_descriptor if1_0_emb_in_jacks[USBD_MIDI_MAX_CABLES];
	struct usb_midi_out_jack_descriptor if1_0_ext_out_jacks[USBD_MIDI_MAX_CABLES];
	struct usb_midi_in_jack_descriptor if1_0_ext_in_jacks[USBD_MIDI_MAX_CABLES];
	struct usb_midi_out_jack_descriptor if1_0_emb_out_jacks[USBD_MIDI_MAX_CABLES];
	struct usb_ep_descriptor if1_0_out_ep_fs;
	struct usb_ep_descriptor if1_0_out_ep_hs;
	struct usb_midi1_cs_endpoint_descriptor if1_0_cs_out_ep;
	struct usb_ep_descriptor if1_0_in_ep_fs;
	struct usb_ep_descriptor if1_0_in_ep_hs;
	struct usb_midi1_cs_endpoint_descriptor if1_0_cs_in_ep;

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
	const struct usb_desc_header **fs_descs;
	const struct usb_desc_header **hs_descs;
	/* USB-MIDI 1.0 cable-number to MIDI 2.0 group lookup tables. The
	 * entries are listed in the same order as the corresponding
	 * baAssocJackID list in the matching class-specific endpoint
	 * descriptor, so the cable number doubles as the index.
	 */
	const uint8_t *cn_to_group_out;
	uint8_t cn_to_group_out_len;
	const uint8_t *cn_to_group_in;
	uint8_t cn_to_group_in_len;
};

/* Device driver data */
struct usbd_midi_data {
	struct usbd_class_data *class_data;
	struct k_work rx_work;
	struct k_work tx_work;
	uint8_t tx_queue_buf[64];
	struct ring_buf tx_queue;
	uint8_t altsetting;
	/* Last interface status reported to the application via
	 * usbd_midi_ops.status_changed_cb, also returned by usbd_midi_get_status.
	 */
	enum usbd_midi_status status;
	struct usbd_midi_ops ops;
};

static void usbd_midi2_recv(const struct device *dev, struct net_buf *const buf);
static void usbd_midi1_recv(const struct device *dev, struct net_buf *const buf);

static void usbd_midi_reset(struct usbd_midi_data *data)
{
	data->altsetting = MIDI1_ALTERNATE;
	ring_buf_reset(&data->tx_queue);
}

/* Map the active alternate setting to the equivalent enabled interface status. */
static enum usbd_midi_status usbd_midi_alt_status(const struct usbd_midi_data *data)
{
	return (data->altsetting == MIDI2_ALTERNATE) ? USBD_MIDI_STATUS_MIDI2
						     : USBD_MIDI_STATUS_MIDI1;
}

/* Record the new interface status and notify the application, but only when it
 * actually changes so that the redundant transitions reported by the USB stack
 * (for example enable followed by the initial alternate selection) collapse
 * into a single callback.
 */
static void usbd_midi_notify_status(const struct device *dev, enum usbd_midi_status status)
{
	struct usbd_midi_data *data = dev->data;

	if (data->status == status) {
		return;
	}
	data->status = status;

	if (data->ops.status_changed_cb) {
		data->ops.status_changed_cb(dev, status);
	}
}

/* Return the number of payload bytes carried by a MIDI 1.0 message with the
 * given status byte, and optionally its USB-MIDI 1.0 Code Index Number.
 *
 * see midi10 Table 4-1: Code Index Number Classifications
 */
static int midi1_status_payload_len(const uint8_t status, uint8_t *const cin)
{
	/* Check status is valid */
	if (status < MIDI_STATUS_NOTE_OFF) {
		return -EINVAL;
	}

	/* Check status is a channel voice message */
	if (status < MIDI_STATUS_SYSEX_START) {
		switch (status & 0xF0) {
		case MIDI_STATUS_NOTE_OFF:
		case MIDI_STATUS_NOTE_ON:
		case MIDI_STATUS_POLY_KEYPRESS:
		case MIDI_STATUS_CONTROL_CHANGE:
		case MIDI_STATUS_PITCH_BEND:
			if (cin != NULL) {
				*cin = (status >> 4) & 0x0F;
			}
			return 3;
		case MIDI_STATUS_PROGRAM_CHANGE:
		case MIDI_STATUS_CHANNEL_PRESSURE:
			if (cin != NULL) {
				*cin = (status >> 4) & 0x0F;
			}
			return 2;
		default:
			return -ENOTSUP;
		}
	}

	/* Handle system exclusive, system common, and system real-time messages */
	switch (status) {
	case MIDI_STATUS_SYSEX_START:
		if (cin != NULL) {
			*cin = MIDI_CIN_SYSEX_START;
		}
		return 3;
	case MIDI_STATUS_TIME_CODE:
	case MIDI_STATUS_SONG_SELECT:
		if (cin != NULL) {
			*cin = MIDI_CIN_SYS_COMMON_2BYTE;
		}
		return 2;
	case MIDI_STATUS_SONG_POS:
		if (cin != NULL) {
			*cin = MIDI_CIN_SYS_COMMON_3BYTE;
		}
		return 3;
	case MIDI_STATUS_TUNE_REQUEST:
	case MIDI_STATUS_SYSEX_END:
		if (cin != NULL) {
			*cin = MIDI_CIN_SYSEX_END_1BYTE;
		}
		return 1;
	default:
		if (cin != NULL) {
			*cin = MIDI_CIN_SINGLE_BYTE;
		}
		return 1;
	}
}

/* Convert a single USB-MIDI 1.0 SysEx event packet (CIN 0x4-0x7) into a UMP
 * 7-bit SysEx (Data 64) message for the given group.
 *
 * The F0 (start of exclusive) and F7 (end of exclusive) framing bytes are part
 * of the USB-MIDI 1.0 byte stream but are not carried in the UMP payload, so
 * they are stripped here while deriving the UMP status (complete/start/
 * continue/end) from their presence.
 *
 * see ump128 7.7 System Exclusive (7-Bit) Messages
 * see midi10 Table 4-1: Code Index Number Classifications
 */
static int midi1_sysex_event_to_ump(const uint8_t cin, const uint8_t byte0, const uint8_t byte1,
				    const uint8_t byte2, const uint8_t group,
				    struct midi_ump *const ump)
{
	uint8_t data[3] = {0};
	uint8_t len;
	uint8_t status;
	bool is_start;
	bool is_end;

	switch (cin) {
	case MIDI_CIN_SYSEX_START:
		/* CIN=0x4 carries three bytes and indicates that more USB-MIDI
		 * packets follow, so it never ends the SysEx.
		 */
		data[0] = byte0;
		data[1] = byte1;
		data[2] = byte2;
		len = 3;
		is_end = false;
		break;
	case MIDI_CIN_SYSEX_END_1BYTE:
		data[0] = byte0;
		len = 1;
		is_end = true;
		break;
	case MIDI_CIN_SYSEX_END_2BYTE:
		data[0] = byte0;
		data[1] = byte1;
		len = 2;
		is_end = true;
		break;
	case MIDI_CIN_SYSEX_END_3BYTE:
		data[0] = byte0;
		data[1] = byte1;
		data[2] = byte2;
		len = 3;
		is_end = true;
		break;
	default:
		return -EINVAL;
	}

	/* A leading F0 marks the first packet of the SysEx; drop it. */
	is_start = (len > 0 && data[0] == MIDI_STATUS_SYSEX_START);
	if (is_start) {
		data[0] = data[1];
		data[1] = data[2];
		data[2] = 0;
		len--;
	}

	/* A trailing F7 marks the last packet of the SysEx; drop it. */
	if (is_end && len > 0 && data[len - 1] == MIDI_STATUS_SYSEX_END) {
		data[len - 1] = 0;
		len--;
	}

	if (is_start && is_end) {
		status = SYSEX_STATUS_COMPLETE;
	} else if (is_start) {
		status = SYSEX_STATUS_START;
	} else if (is_end) {
		status = SYSEX_STATUS_END;
	} else {
		status = SYSEX_STATUS_CONTINUE;
	}

	ump->data[0] = (UMP_MT_DATA_64 << 28) | ((group & 0xF) << 24) | ((status & 0xF) << 20) |
		       ((len & 0xF) << 16) | (data[0] << 8) | data[1];
	ump->data[1] = (uint32_t)data[2] << 24;

	return 0;
}

static int midi1_event_to_ump(const uint32_t event_le, const uint8_t group,
			      struct midi_ump *const ump)
{
	uint8_t header = event_le & 0xFF;
	uint8_t cin = header & 0x0F;
	uint8_t byte0 = (event_le >> 8) & 0xFF;
	uint8_t byte1 = (event_le >> 16) & 0xFF;
	uint8_t byte2 = (event_le >> 24) & 0xFF;
	uint8_t expected_cin;
	int payload_len;
	uint8_t command;
	uint8_t channel;

	/* Single-byte System Common / Real-Time messages are encoded with
	 * CIN=0x5 but are not SysEx fragments, so handle them before the SysEx
	 * code indexes below.
	 */
	if (cin == MIDI_CIN_SYSEX_END_1BYTE &&
	    (byte0 == MIDI_STATUS_TUNE_REQUEST || byte0 >= MIDI_STATUS_TIMING_CLOCK)) {
		*ump = UMP_SYS_RT_COMMON(group, byte0, 0, 0);
		return 0;
	}

	if (cin == MIDI_CIN_SYSEX_START || cin == MIDI_CIN_SYSEX_END_1BYTE ||
	    cin == MIDI_CIN_SYSEX_END_2BYTE || cin == MIDI_CIN_SYSEX_END_3BYTE) {
		return midi1_sysex_event_to_ump(cin, byte0, byte1, byte2, group, ump);
	}

	payload_len = midi1_status_payload_len(byte0, &expected_cin);

	if (payload_len < 0) {
		return payload_len;
	}

	if (cin != expected_cin) {
		return -EINVAL;
	}

	if (payload_len == 2) {
		byte2 = 0x00;
	}

	if (byte0 < MIDI_STATUS_SYSEX_START) {
		command = (byte0 >> 4) & 0x0F;
		channel = byte0 & 0x0F;

		*ump = UMP_MIDI1_CHANNEL_VOICE(group, command, channel, byte1, byte2);
	} else {
		*ump = UMP_SYS_RT_COMMON(group, byte0, byte1, byte2);
	}

	return 0;
}

static int midi1_ump_to_event(const struct midi_ump *const ump, const uint8_t cable_number,
			      uint32_t *const event_le)
{
	uint8_t status;
	uint8_t cin;
	int payload_len;
	uint8_t bytes[MIDI1_EVENT_BYTES];

	/* Note: This function only handles single packet messages */
	if (UMP_MT(*ump) != UMP_MT_MIDI1_CHANNEL_VOICE && UMP_MT(*ump) != UMP_MT_SYS_RT_COMMON) {
		return -ENOTSUP;
	}

	status = UMP_MIDI_STATUS(*ump);
	payload_len = midi1_status_payload_len(status, &cin);

	if (payload_len < 0) {
		return payload_len;
	}

	bytes[0] = (uint8_t)((cable_number << 4) | cin);
	bytes[1] = status;
	bytes[2] = UMP_MIDI1_P1(*ump);
	bytes[3] = (payload_len == 2) ? 0x00 : UMP_MIDI1_P2(*ump);

	*event_le = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16) |
		    ((uint32_t)bytes[3] << 24);

	return 0;
}

static void usbd_midi1_recv(const struct device *dev, struct net_buf *const buf)
{
	const struct usbd_midi_config *config = dev->config;
	struct usbd_midi_data *data = dev->data;
	struct midi_ump ump;
	int ret;
	uint32_t packet;
	uint8_t cn;
	uint8_t group;

	LOG_HEXDUMP_DBG(buf->data, buf->len, "MIDI1 - Rx DATA");
	while (buf->len >= MIDI1_EVENT_BYTES) {
		packet = net_buf_pull_le32(buf);

		cn = (packet >> 4) & 0x0F;
		if (cn >= config->cn_to_group_out_len) {
			LOG_DBG("Drop USB-MIDI 1.0 event with unmapped CN=%u", cn);
			continue;
		}
		group = config->cn_to_group_out[cn];

		ret = midi1_event_to_ump(packet, group, &ump);
		if (ret < 0) {
			LOG_DBG("Drop unsupported USB-MIDI 1.0 event %08x", packet);
			continue;
		}

		if (data->ops.rx_packet_cb) {
			data->ops.rx_packet_cb(dev, ump);
		}
	}

	if (buf->len) {
		LOG_HEXDUMP_WRN(buf->data, buf->len, "Trailing data in Rx buffer");
	}
}

static void usbd_midi2_recv(const struct device *dev, struct net_buf *const buf)
{
	struct usbd_midi_data *data = dev->data;
	struct midi_ump ump;

	LOG_HEXDUMP_DBG(buf->data, buf->len, "MIDI2 - Rx DATA");
	while (buf->len >= 4) {
		ump.data[0] = net_buf_pull_le32(buf);
		for (size_t i = 1; i < UMP_NUM_WORDS(ump); i++) {
			if (buf->len < 4) {
				LOG_ERR("Incomplete UMP");
				return;
			}
			ump.data[i] = net_buf_pull_le32(buf);
		}

		if (data->ops.rx_packet_cb) {
			data->ops.rx_packet_cb(dev, ump);
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

	LOG_DBG("MIDI2 request for %s ep=%02X len=%d err=%d",
		dev->name, info->ep, buf->len, err);

	if (err && err != -ECONNABORTED) {
		LOG_ERR("Transfer error %d", err);
	}
	if (USB_EP_DIR_IS_OUT(info->ep)) {
		if (data->altsetting == MIDI1_ALTERNATE) {
			usbd_midi1_recv(dev, buf);
		} else {
			usbd_midi2_recv(dev, buf);
		}
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
	bool ready = false;
	struct usbd_midi_data *data = dev->data;

	LOG_DBG("update for %s: if=%u, alt=%u", dev->name, iface, alternate);

	switch (alternate) {
	case MIDI1_ALTERNATE:
		data->altsetting = MIDI1_ALTERNATE;
		ready = true;
		LOG_INF("%s set USB-MIDI1.0 altsetting", dev->name);
		break;
	case MIDI2_ALTERNATE:
		data->altsetting = MIDI2_ALTERNATE;
		ready = true;
		LOG_INF("%s set USB-MIDI2.0 altsetting", dev->name);
		break;
	default:
		LOG_WRN("%s requested unsupported altsetting %u", dev->name, alternate);
		break;
	}

	if (ready) {
		ring_buf_reset(&data->tx_queue);
		k_work_submit(&data->rx_work);
		usbd_midi_notify_status(dev, usbd_midi_alt_status(data));
	}
}

static void usbd_midi_class_enable(struct usbd_class_data *const class_data)
{
	const struct device *dev = usbd_class_get_private(class_data);
	struct usbd_midi_data *data = dev->data;

	usbd_midi_reset(data);

	LOG_DBG("Enable %s", dev->name);
	k_work_submit(&data->rx_work);
	usbd_midi_notify_status(dev, usbd_midi_alt_status(data));
}

static void usbd_midi_class_disable(struct usbd_class_data *const class_data)
{
	const struct device *dev = usbd_class_get_private(class_data);
	struct usbd_midi_data *data = dev->data;
	struct k_work_sync sync;

	LOG_DBG("Disable %s", dev->name);
	k_work_cancel_sync(&data->rx_work, &sync);
	usbd_midi_reset(data);
	usbd_midi_notify_status(dev, USBD_MIDI_STATUS_DISABLED);
}

static void usbd_midi_class_suspended(struct usbd_class_data *const class_data)
{
	const struct device *dev = usbd_class_get_private(class_data);
	struct usbd_midi_data *data = dev->data;
	struct k_work_sync sync;

	LOG_DBG("Suspend %s", dev->name);
	k_work_cancel_sync(&data->rx_work, &sync);
	usbd_midi_reset(data);
	usbd_midi_notify_status(dev, USBD_MIDI_STATUS_DISABLED);
}

static void usbd_midi_class_resumed(struct usbd_class_data *const class_data)
{
	const struct device *dev = usbd_class_get_private(class_data);
	struct usbd_midi_data *data = dev->data;

	LOG_DBG("Resume %s", dev->name);
	k_work_submit(&data->rx_work);
	usbd_midi_notify_status(dev, usbd_midi_alt_status(data));
}

static int usbd_midi_class_cth(struct usbd_class_data *const class_data,
			       const struct usb_setup_packet *const setup,
			       struct net_buf *const buf)
{
	const struct device *dev = usbd_class_get_private(class_data);
	const struct usbd_midi_config *config = dev->config;
	struct usbd_midi_data *data = dev->data;

	size_t head_len = config->desc->grptrm_header.bLength;
	size_t total_len = sys_le16_to_cpu(config->desc->grptrm_header.wTotalLength);

	LOG_DBG("Control to host for %s", dev->name);
	LOG_DBG("  bmRequestType=%02X bRequest=%02X wValue=%04X wIndex=%04X wLength=%04X",
		setup->bmRequestType, setup->bRequest, setup->wValue, setup->wIndex,
		setup->wLength);

	/* Only support Group Terminal blocks retrieved with
	 * midi20 6. Class Specific Command: Group Terminal Blocks Descriptors Request
	 */
	if (data->altsetting != MIDI2_ALTERNATE ||
	    setup->bRequest != USB_SREQ_GET_DESCRIPTOR ||
	    setup->wValue != ((CS_GR_TRM_BLOCK << 8) | MIDI2_ALTERNATE)) {
		errno = -ENOTSUP;
		return 0;
	}

	/* Group terminal block header */
	net_buf_add_mem(buf, (void *) &config->desc->grptrm_header,
			MIN(head_len, setup->wLength));

	/* Group terminal blocks */
	if (setup->wLength > head_len && total_len > head_len) {
		net_buf_add_mem(buf, (void *) &config->desc->grptrm_blocks,
				MIN(total_len, setup->wLength) - head_len);
	}
	LOG_HEXDUMP_DBG(buf->data, buf->len, "Control to host");

	return 0;
}

/* Update the Audio Control header to reference the MIDI Streaming interface
 * number assigned by the USB stack. This is needed for composite devices where
 * MIDI is not the first interface.
 */
static int usbd_midi_class_init(struct usbd_class_data *const class_data)
{
	const struct device *dev = usbd_class_get_private(class_data);
	const struct usbd_midi_config *config = dev->config;
	struct usbd_midi_descriptors *desc = config->desc;

	LOG_DBG("Init %s device class", dev->name);

	desc->if0_cs.baInterfaceNr1 = desc->if1_0_std.bInterfaceNumber;
	LOG_DBG("Set baInterfaceNr1 to %u", desc->if0_cs.baInterfaceNr1);

	return 0;
}

static void *usbd_midi_class_get_desc(struct usbd_class_data *const class_data,
				      const enum usbd_speed speed)
{
	const struct device *dev = usbd_class_get_private(class_data);
	const struct usbd_midi_config *config = dev->config;

	LOG_DBG("Get descriptors for %s", dev->name);

	if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		return (void *)config->hs_descs;
	}

	return (void *)config->fs_descs;
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

static uint8_t usbd_midi_get_bulk_in(struct usbd_class_data *const class_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(class_data);
	const struct device *dev = usbd_class_get_private(class_data);
	const struct usbd_midi_config *cfg = dev->config;

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return cfg->desc->if1_1_in_ep_hs.bEndpointAddress;
	}

	return cfg->desc->if1_1_in_ep_fs.bEndpointAddress;
}

static uint8_t usbd_midi_get_bulk_out(struct usbd_class_data *const class_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(class_data);
	const struct device *dev = usbd_class_get_private(class_data);
	const struct usbd_midi_config *cfg = dev->config;

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return cfg->desc->if1_1_out_ep_hs.bEndpointAddress;
	}

	return cfg->desc->if1_1_out_ep_fs.bEndpointAddress;
}

static void usbd_midi_rx_work(struct k_work *work)
{
	struct usbd_midi_data *data = CONTAINER_OF(work, struct usbd_midi_data, rx_work);
	struct net_buf *buf;
	int ret;

	buf = usbd_midi_buf_alloc(usbd_midi_get_bulk_out(data->class_data));
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
	struct usbd_midi_data *data = CONTAINER_OF(work, struct usbd_midi_data, tx_work);
	struct net_buf *buf;
	int ret;

	buf = usbd_midi_buf_alloc(usbd_midi_get_bulk_in(data->class_data));
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

	LOG_DBG("Init device %s", dev->name);
	ring_buf_init(&data->tx_queue, 64, data->tx_queue_buf);
	k_work_init(&data->rx_work, usbd_midi_rx_work);
	k_work_init(&data->tx_work, usbd_midi_tx_work);

	return 0;
}

static uint8_t midi1_sysex_cin_from_len(const uint8_t chunk_len)
{
	if (chunk_len == 1) {
		return MIDI_CIN_SYSEX_END_1BYTE;
	} else if (chunk_len == 2) {
		return MIDI_CIN_SYSEX_END_2BYTE;
	} else {
		return MIDI_CIN_SYSEX_END_3BYTE;
	}
}

/* Convert a UMP 7-bit SysEx (Data 64) message into up to three USB-MIDI 1.0
 * event packets for the given cable number.
 *
 * The UMP payload does not carry the F0/F7 framing bytes, so they are inserted
 * around the data of the start/end fragments while splitting it into the 3-byte
 * USB-MIDI 1.0 event packets.
 *
 * @param ump           The UMP Data 64 message to convert.
 * @param cable_number  The destination USB-MIDI 1.0 cable number.
 * @param events        Receives the little-endian USB-MIDI 1.0 event packets.
 *
 * @return The number of event packets written to @p events (0 to 3), or a
 *         negative errno on error.
 *
 * see ump128 7.7 System Exclusive (7-Bit) Messages
 */
static int midi1_ump_to_sysex_events(const struct midi_ump *const ump, const uint8_t cable_number,
				     uint32_t events[3])
{
	uint8_t status = (ump->data[0] >> 20) & 0xF;
	uint8_t len = (ump->data[0] >> 16) & 0xF;
	uint8_t payload[6];
	uint8_t sysex[8];
	uint8_t total = 0;
	uint8_t processed = 0;
	int n_events = 0;

	if (len > ARRAY_SIZE(payload)) {
		return -EINVAL;
	}

	payload[0] = (ump->data[0] >> 8) & 0xFF;
	payload[1] = ump->data[0] & 0xFF;
	payload[2] = (ump->data[1] >> 24) & 0xFF;
	payload[3] = (ump->data[1] >> 16) & 0xFF;
	payload[4] = (ump->data[1] >> 8) & 0xFF;
	payload[5] = ump->data[1] & 0xFF;

	/* Re-insert the F0/F7 framing the UMP form omits. */
	if (status == SYSEX_STATUS_START || status == SYSEX_STATUS_COMPLETE) {
		sysex[total++] = MIDI_STATUS_SYSEX_START;
	}
	for (uint8_t i = 0; i < len; i++) {
		sysex[total++] = payload[i];
	}
	if (status == SYSEX_STATUS_END || status == SYSEX_STATUS_COMPLETE) {
		sysex[total++] = MIDI_STATUS_SYSEX_END;
	}

	while (processed < total) {
		uint8_t chunk_len = MIN(3, total - processed);
		uint8_t cin;
		uint32_t event_le;

		if (processed + chunk_len < total) {
			/* More fragments of this UMP follow. */
			cin = MIDI_CIN_SYSEX_START;
		} else if (status == SYSEX_STATUS_START || status == SYSEX_STATUS_CONTINUE) {
			/* The SysEx continues in a later UMP. */
			cin = MIDI_CIN_SYSEX_START;
		} else {
			cin = midi1_sysex_cin_from_len(chunk_len);
		}

		event_le = (uint32_t)((cable_number << 4) | cin);
		event_le |= (uint32_t)sysex[processed] << 8;
		if (chunk_len > 1) {
			event_le |= (uint32_t)sysex[processed + 1] << 16;
		}
		if (chunk_len > 2) {
			event_le |= (uint32_t)sysex[processed + 2] << 24;
		}

		events[n_events++] = sys_cpu_to_le32(event_le);
		processed += chunk_len;
	}

	return n_events;
}

/**
 * @brief Resolve a MIDI 2.0 group number to a USB-MIDI 1.0 cable number on the
 *        IN endpoint (device-to-host direction).
 *
 * @param config  Driver configuration containing the IN endpoint lookup table.
 * @param group   The MIDI 2.0 group requested by the caller.
 * @param cable_number  Set to the corresponding USB-MIDI 1.0 cable number on
 *                      success.
 *
 * @return 0 on success, or -ENOTSUP if @p group has no embedded OUT jack
 *         (i.e. is not exposed on the IN endpoint).
 */
static int midi1_in_ep_cable_for_group(const struct usbd_midi_config *config,
				       const uint8_t group, uint8_t *const cable_number)
{
	for (uint8_t i = 0; i < config->cn_to_group_in_len; i++) {
		if (config->cn_to_group_in[i] == group) {
			*cable_number = i;
			return 0;
		}
	}

	return -ENOTSUP;
}

int usbd_midi_send(const struct device *dev, const struct midi_ump ump)
{
	const struct usbd_midi_config *config = dev->config;
	struct usbd_midi_data *data = dev->data;
	size_t words = UMP_NUM_WORDS(ump);
	size_t buflen = 4 * words;
	size_t needed = buflen;
	uint32_t word;
	uint8_t cable_number = 0;
	int ret;

	LOG_DBG("Send MT=%X group=%X", UMP_MT(ump), UMP_GROUP(ump));

	if (data->altsetting == MIDI1_ALTERNATE) {
		if (UMP_MT(ump) == UMP_MT_DATA_64) {
			/* Worst case: 6 payload bytes plus F0/F7 framing -> 3
			 * USB-MIDI 1.0 packets (12 bytes).
			 */
			needed = 12;
		} else {
			needed = MIDI1_EVENT_BYTES;
		}

		ret = midi1_in_ep_cable_for_group(config, UMP_GROUP(ump), &cable_number);
		if (ret) {
			LOG_DBG("Group %u has no USB-MIDI 1.0 IN cable", UMP_GROUP(ump));
			return ret;
		}
	}

	if (needed > ring_buf_space_get(&data->tx_queue)) {
		LOG_WRN("Not enough space in tx queue");
		return -ENOBUFS;
	}

	if (data->altsetting == MIDI2_ALTERNATE) {
		for (size_t i = 0; i < words; i++) {
			word = sys_cpu_to_le32(ump.data[i]);
			ring_buf_put(&data->tx_queue, (const uint8_t *)&word, sizeof(word));
		}
	} else if (data->altsetting == MIDI1_ALTERNATE) {
		if (UMP_MT(ump) == UMP_MT_DATA_64) {
			uint32_t events[3];
			int n_events;

			n_events = midi1_ump_to_sysex_events(&ump, cable_number, events);
			if (n_events < 0) {
				return n_events;
			}

			for (int e = 0; e < n_events; e++) {
				ring_buf_put(&data->tx_queue, (const uint8_t *)&events[e],
					     MIDI1_EVENT_BYTES);
			}
		} else {
			ret = midi1_ump_to_event(&ump, cable_number, &word);
			if (ret) {
				return ret;
			}
			word = sys_cpu_to_le32(word);
			ring_buf_put(&data->tx_queue, (const uint8_t *)&word, MIDI1_EVENT_BYTES);
		}
	} else {
		return -EIO;
	}
	k_work_submit(&data->tx_work);

	return 0;
}

void usbd_midi_set_ops(const struct device *dev, const struct usbd_midi_ops *ops)
{
	struct usbd_midi_data *data = dev->data;

	if (ops == NULL) {
		memset(&data->ops, 0, sizeof(struct usbd_midi_ops));
	} else {
		memcpy(&data->ops, ops, sizeof(struct usbd_midi_ops));
	}

	LOG_DBG("Set ops for %s to %p", dev->name, ops);
}

enum usbd_midi_status usbd_midi_get_status(const struct device *dev)
{
	const struct usbd_midi_data *data = dev->data;

	return data->status;
}

/* Group Terminal Block unique identification number, type and protocol
 * see midi20 5.4.2 Group Terminal Block Descriptor
 */
#define GRPTRM_BLOCK_ID(node) UTIL_INC(DT_NODE_CHILD_IDX(node))
#define GRPTRM_BLOCK_TYPE(node)                                                                   \
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

/* True (1) if the GTB exposes data flowing host-to-device (a USB-MIDI 1.0
 * embedded IN jack), false (0) otherwise.
 */
#define GRPTRM_IS_SINK(node)                                              \
	UTIL_OR(DT_ENUM_HAS_VALUE(node, terminal_type, bidirectional),    \
		DT_ENUM_HAS_VALUE(node, terminal_type, output_only))

/* True (1) if the GTB exposes data flowing device-to-host (a USB-MIDI 1.0
 * embedded OUT jack), false (0) otherwise.
 */
#define GRPTRM_IS_SOURCE(node)                                            \
	UTIL_OR(DT_ENUM_HAS_VALUE(node, terminal_type, bidirectional),    \
		DT_ENUM_HAS_VALUE(node, terminal_type, input_only))

/* Group Terminal Block unique identification number with a trailing comma
 * if that block is bidirectional or of given terminal type; otherwise empty
 */
#define GRPTRM_BLOCK_ID_SEP_IF(node, ttype)                                    \
	IF_ENABLED(                                                            \
		UTIL_OR(DT_ENUM_HAS_VALUE(node, terminal_type, bidirectional), \
			DT_ENUM_HAS_VALUE(node, terminal_type, ttype)),        \
		(GRPTRM_BLOCK_ID(node),))

/* All unique identification numbers of output+bidir group terminal blocks */
#define GRPTRM_OUTPUT_BLOCK_IDS(n) \
	DT_INST_FOREACH_CHILD_VARGS(n, GRPTRM_BLOCK_ID_SEP_IF, output_only)

/* All unique identification numbers of input+bidir group terminal blocks */
#define GRPTRM_INPUT_BLOCK_IDS(n) \
	DT_INST_FOREACH_CHILD_VARGS(n, GRPTRM_BLOCK_ID_SEP_IF, input_only)

#define N_INPUTS(n)  sizeof((uint8_t[]){GRPTRM_INPUT_BLOCK_IDS(n)})
#define N_OUTPUTS(n) sizeof((uint8_t[]){GRPTRM_OUTPUT_BLOCK_IDS(n)})

/* CN-to-group lookup table contributors: emit "DT_REG_ADDR(node)," (the
 * group number) for sinks/sources respectively, in DT child order so the
 * resulting list lines up with the corresponding baAssocJackID entries.
 */
#define GRPTRM_GROUP_IF_SINK(node)                                  \
	IF_ENABLED(GRPTRM_IS_SINK(node), (DT_REG_ADDR(node),))
#define GRPTRM_GROUP_IF_SOURCE(node)                                \
	IF_ENABLED(GRPTRM_IS_SOURCE(node), (DT_REG_ADDR(node),))

#define USBD_MIDI_VALIDATE_GRPTRM_BLOCK(node)                              \
	BUILD_ASSERT(DT_REG_ADDR(node) < USBD_MIDI_MAX_CABLES,             \
		     "Group Terminal Block address must be within 0..15"); \
	BUILD_ASSERT(DT_REG_ADDR(node) + DT_REG_SIZE(node) <=              \
			     USBD_MIDI_MAX_CABLES,                         \
		     "Too many Group Terminals in this Block");           \
	BUILD_ASSERT(DT_REG_SIZE(node) == 1,                               \
		     "MIDI 1.0 compatibility currently supports one group per block");

#define USBD_MIDI_VALIDATE_INSTANCE(n)                                     \
	DT_INST_FOREACH_CHILD(n, USBD_MIDI_VALIDATE_GRPTRM_BLOCK);          \
	BUILD_ASSERT(DT_INST_CHILD_NUM_STATUS_OKAY(n) >= 1,                 \
		     "At least one Group Terminal Block is required");     \
	BUILD_ASSERT(DT_INST_CHILD_NUM_STATUS_OKAY(n) <=                    \
			     USBD_MIDI_MAX_CABLES,                         \
		     "Too many Group Terminal Blocks for USB-MIDI 1.0");

#define USBD_MIDI2_INIT_GRPTRM_BLOCK_DESCRIPTOR(node)                                   \
	{                                                                               \
		.bLength = sizeof(struct usb_midi_grptrm_block_descriptor),             \
		.bDescriptorType = CS_GR_TRM_BLOCK,                                     \
		.bDescriptorSubtype = GR_TRM_BLOCK,                                     \
		.bGrpTrmBlkID = GRPTRM_BLOCK_ID(node),                                  \
		.bGrpTrmBlkType = GRPTRM_BLOCK_TYPE(node),                              \
		.nGroupTrm = DT_REG_ADDR(node),                                         \
		.nNumGroupTrm = DT_REG_SIZE(node),                                      \
		.iBlockItem = 0,                                                        \
		.bMIDIProtocol = GRPTRM_PROTOCOL(node),                                 \
		.wMaxInputBandwidth = sys_cpu_to_le16(DT_PROP(node, serial_31250bps)),  \
		.wMaxOutputBandwidth = sys_cpu_to_le16(DT_PROP(node, serial_31250bps)), \
	}

#define USBD_MIDI2_BUILD_GRPTRM_BLOCK(node) USBD_MIDI2_INIT_GRPTRM_BLOCK_DESCRIPTOR(node),

#define USBD_MIDI2_GRPTRM_TOTAL_LEN(n)                    \
	sizeof(struct usb_midi_grptrm_header_descriptor)  \
	+ DT_INST_CHILD_NUM_STATUS_OKAY(n)                \
	* sizeof(struct usb_midi_grptrm_block_descriptor)

/* USB-MIDI 1.0 jack descriptor initializers, emitted with a trailing
 * designated initializer index so unused slots stay zero-initialised.
 */
#define USBD_MIDI1_EMB_IN_JACK_INIT(node)                                       \
	IF_ENABLED(GRPTRM_IS_SINK(node), (                                      \
		[DT_NODE_CHILD_IDX(node)] = {                                   \
			.bLength = sizeof(struct usb_midi_in_jack_descriptor),  \
			.bDescriptorType = USB_DESC_CS_INTERFACE,               \
			.bDescriptorSubtype = MIDI1_IN_JACK,                    \
			.bJackType = MIDI1_JACK_EMBEDDED,                       \
			.bJackID = MIDI1_EMB_IN_JACK_ID(node),                  \
		},))

#define USBD_MIDI1_EXT_OUT_JACK_INIT(node)                                       \
	IF_ENABLED(GRPTRM_IS_SINK(node), (                                       \
		[DT_NODE_CHILD_IDX(node)] = {                                    \
			.bLength = sizeof(struct usb_midi_out_jack_descriptor),  \
			.bDescriptorType = USB_DESC_CS_INTERFACE,                \
			.bDescriptorSubtype = MIDI1_OUT_JACK,                    \
			.bJackType = MIDI1_JACK_EXTERNAL,                        \
			.bJackID = MIDI1_EXT_OUT_JACK_ID(node),                  \
			.bNrInputPins = 1,                                       \
			.baSourceID = MIDI1_EMB_IN_JACK_ID(node),                \
			.baSourcePin = 0x01,                                     \
		},))

#define USBD_MIDI1_EXT_IN_JACK_INIT(node)                                       \
	IF_ENABLED(GRPTRM_IS_SOURCE(node), (                                    \
		[DT_NODE_CHILD_IDX(node)] = {                                   \
			.bLength = sizeof(struct usb_midi_in_jack_descriptor),  \
			.bDescriptorType = USB_DESC_CS_INTERFACE,               \
			.bDescriptorSubtype = MIDI1_IN_JACK,                    \
			.bJackType = MIDI1_JACK_EXTERNAL,                       \
			.bJackID = MIDI1_EXT_IN_JACK_ID(node),                  \
		},))

#define USBD_MIDI1_EMB_OUT_JACK_INIT(node)                                       \
	IF_ENABLED(GRPTRM_IS_SOURCE(node), (                                     \
		[DT_NODE_CHILD_IDX(node)] = {                                    \
			.bLength = sizeof(struct usb_midi_out_jack_descriptor),  \
			.bDescriptorType = USB_DESC_CS_INTERFACE,                \
			.bDescriptorSubtype = MIDI1_OUT_JACK,                    \
			.bJackType = MIDI1_JACK_EMBEDDED,                        \
			.bJackID = MIDI1_EMB_OUT_JACK_ID(node),                  \
			.bNrInputPins = 1,                                       \
			.baSourceID = MIDI1_EXT_IN_JACK_ID(node),                \
			.baSourcePin = 0x01,                                     \
		},))

/* baAssocJackID list contributors. Sinks emit their embedded IN jack IDs in
 * DT order (used by the OUT EP's baAssocJackID); sources emit their embedded
 * OUT jack IDs in DT order (used by the IN EP's baAssocJackID). Cable numbers
 * are the resulting positions in these lists.
 */
#define USBD_MIDI1_EMB_IN_JACK_ID_IF_SINK(node)                                  \
	IF_ENABLED(GRPTRM_IS_SINK(node), (MIDI1_EMB_IN_JACK_ID(node),))
#define USBD_MIDI1_EMB_OUT_JACK_ID_IF_SOURCE(node)                               \
	IF_ENABLED(GRPTRM_IS_SOURCE(node), (MIDI1_EMB_OUT_JACK_ID(node),))

/* Total length of the USB-MIDI 1.0 alternate setting class-specific
 * descriptors: the MS interface header, one embedded + one external jack per
 * sink, one embedded + one external jack per source, and the two endpoint
 * descriptors with their N-jack class-specific endpoint descriptors.
 */
#define MIDI1_MS_TOTAL_LEN(n)                                                  \
	(sizeof(struct usb_midi_header_descriptor) +                           \
	 N_OUTPUTS(n) * sizeof(struct usb_midi_in_jack_descriptor) +           \
	 N_OUTPUTS(n) * sizeof(struct usb_midi_out_jack_descriptor) +          \
	 N_INPUTS(n) * sizeof(struct usb_midi_in_jack_descriptor) +            \
	 N_INPUTS(n) * sizeof(struct usb_midi_out_jack_descriptor) +           \
	 2 * sizeof(struct usb_ep_descriptor) +                                \
	 (4 + N_OUTPUTS(n)) +                                                  \
	 (4 + N_INPUTS(n)))

/* Per-child descriptor pointer emitters for the FS and HS descriptor pointer
 * arrays. Reference the jack array slots only for children that actually
 * provide a jack of the given type.
 */
#define USBD_MIDI1_JACK_PTR(field, n, node)                                                  \
	(struct usb_desc_header *)&usbd_midi_desc_##n.field[DT_NODE_CHILD_IDX(node)]

#define USBD_MIDI1_JACK_PTRS_PER_CHILD(node, n)                                              \
	IF_ENABLED(GRPTRM_IS_SINK(node), (                                                   \
		USBD_MIDI1_JACK_PTR(if1_0_emb_in_jacks, n, node),                            \
		USBD_MIDI1_JACK_PTR(if1_0_ext_out_jacks, n, node),))                         \
	IF_ENABLED(GRPTRM_IS_SOURCE(node), (                                                 \
		USBD_MIDI1_JACK_PTR(if1_0_ext_in_jacks, n, node),                            \
		USBD_MIDI1_JACK_PTR(if1_0_emb_out_jacks, n, node),))

#define USBD_MIDI1_JACK_PTRS(n)                                                              \
	DT_INST_FOREACH_CHILD_VARGS(n, USBD_MIDI1_JACK_PTRS_PER_CHILD, n)

/* clang-format off */
#define USBD_MIDI_DEFINE_DESCRIPTORS(n)                                                            \
	static struct usbd_midi_descriptors usbd_midi_desc_##n = {                                 \
		.iad =                                                                             \
			{                                                                          \
				.bLength = sizeof(struct usb_association_descriptor),              \
				.bDescriptorType = USB_DESC_INTERFACE_ASSOC,                       \
				.bFirstInterface = 0,                                              \
				.bInterfaceCount = 2,                                              \
				.bFunctionClass = AUDIO,                                           \
				.bFunctionSubClass = MIDISTREAMING,                                \
			},                                                                         \
		.if0_std =                                                                         \
			{                                                                          \
				.bLength = sizeof(struct usb_if_descriptor),                       \
				.bDescriptorType = USB_DESC_INTERFACE,                             \
				.bInterfaceNumber = 0,                                             \
				.bAlternateSetting = 0,                                            \
				.bNumEndpoints = 0,                                                \
				.bInterfaceClass = AUDIO,                                          \
				.bInterfaceSubClass = AUDIOCONTROL,                                \
			},                                                                         \
		.if0_cs =                                                                          \
			{                                                                          \
				.bLength = sizeof(struct usb_midi_cs_ac_header_descriptor),        \
				.bDescriptorType = USB_DESC_CS_INTERFACE,                          \
				.bDescriptorSubtype = MS_HEADER,                                   \
				.bcdADC = sys_cpu_to_le16(0x0100),                                 \
				.wTotalLength = sizeof(struct usb_midi_cs_ac_header_descriptor),   \
				.bInCollection = 1,                                                \
				.baInterfaceNr1 = 1,                                               \
			},                                                                         \
		.if1_0_std =                                                                       \
			{                                                                          \
				.bLength = sizeof(struct usb_if_descriptor),                       \
				.bDescriptorType = USB_DESC_INTERFACE,                             \
				.bInterfaceNumber = 1,                                             \
				.bAlternateSetting = MIDI1_ALTERNATE,                              \
				.bNumEndpoints = 2,                                                \
				.bInterfaceClass = AUDIO,                                          \
				.bInterfaceSubClass = MIDISTREAMING,                               \
			},                                                                         \
		.if1_0_ms_header =                                                                 \
			{                                                                          \
				.bLength = sizeof(struct usb_midi_header_descriptor),              \
				.bDescriptorType = USB_DESC_CS_INTERFACE,                          \
				.bDescriptorSubtype = MS_HEADER,                                   \
				.bcdMSC = sys_cpu_to_le16(0x0100),                                 \
				.wTotalLength = sys_cpu_to_le16(MIDI1_MS_TOTAL_LEN(n)),            \
			},                                                                         \
		.if1_0_emb_in_jacks = {[0] = {0},                                                  \
			DT_INST_FOREACH_CHILD(n, USBD_MIDI1_EMB_IN_JACK_INIT)},                    \
		.if1_0_ext_out_jacks = {[0] = {0},                                                 \
			DT_INST_FOREACH_CHILD(n, USBD_MIDI1_EXT_OUT_JACK_INIT)},                   \
		.if1_0_ext_in_jacks = {[0] = {0},                                                  \
			DT_INST_FOREACH_CHILD(n, USBD_MIDI1_EXT_IN_JACK_INIT)},                    \
		.if1_0_emb_out_jacks = {[0] = {0},                                                 \
			DT_INST_FOREACH_CHILD(n, USBD_MIDI1_EMB_OUT_JACK_INIT)},                   \
		.if1_0_out_ep_fs =                                                                 \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = n + FIRST_OUT_EP_ADDR,                         \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(64U),                            \
			},                                                                         \
		.if1_0_out_ep_hs =                                                                 \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = n + FIRST_OUT_EP_ADDR,                         \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(512U),                           \
			},                                                                         \
		.if1_0_cs_out_ep =                                                                 \
			{                                                                          \
				.bLength = 4 + N_OUTPUTS(n),                                       \
				.bDescriptorType = USB_DESC_CS_ENDPOINT,                           \
				.bDescriptorSubtype = MS_GENERAL,                                  \
				.bNumEmbMIDIJack = N_OUTPUTS(n),                                   \
				.baAssocJackID = {DT_INST_FOREACH_CHILD(                           \
					n, USBD_MIDI1_EMB_IN_JACK_ID_IF_SINK) 0},                  \
			},                                                                         \
		.if1_0_in_ep_fs =                                                                  \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = n + FIRST_IN_EP_ADDR,                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(64U),                            \
			},                                                                         \
		.if1_0_in_ep_hs =                                                                  \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = n + FIRST_IN_EP_ADDR,                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(512U),                           \
			},                                                                         \
		.if1_0_cs_in_ep =                                                                  \
			{                                                                          \
				.bLength = 4 + N_INPUTS(n),                                        \
				.bDescriptorType = USB_DESC_CS_ENDPOINT,                           \
				.bDescriptorSubtype = MS_GENERAL,                                  \
				.bNumEmbMIDIJack = N_INPUTS(n),                                    \
				.baAssocJackID = {DT_INST_FOREACH_CHILD(                           \
					n, USBD_MIDI1_EMB_OUT_JACK_ID_IF_SOURCE) 0},               \
			},                                                                         \
		.if1_1_std =                                                                       \
			{                                                                          \
				.bLength = sizeof(struct usb_if_descriptor),                       \
				.bDescriptorType = USB_DESC_INTERFACE,                             \
				.bInterfaceNumber = 1,                                             \
				.bAlternateSetting = MIDI2_ALTERNATE,                              \
				.bNumEndpoints = 2,                                                \
				.bInterfaceClass = AUDIO,                                          \
				.bInterfaceSubClass = MIDISTREAMING,                               \
			},                                                                         \
		.if1_1_ms_header =                                                                 \
			{                                                                          \
				.bLength = sizeof(struct usb_midi_header_descriptor),              \
				.bDescriptorType = USB_DESC_CS_INTERFACE,                          \
				.bDescriptorSubtype = MS_HEADER,                                   \
				.bcdMSC = sys_cpu_to_le16(0x0200),                                 \
				.wTotalLength = sys_cpu_to_le16(                                   \
					sizeof(struct usb_midi_header_descriptor)),                \
			},                                                                         \
		.if1_1_out_ep_fs =                                                                 \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = n + FIRST_OUT_EP_ADDR,                         \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(64U),                            \
			},                                                                         \
		.if1_1_out_ep_hs =                                                                 \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = n + FIRST_OUT_EP_ADDR,                         \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(512U),                           \
			},                                                                         \
		.if1_1_cs_out_ep =                                                                 \
			{                                                                          \
				.bLength = 4 + N_OUTPUTS(n),                                       \
				.bDescriptorType = USB_DESC_CS_ENDPOINT,                           \
				.bDescriptorSubtype = MS_GENERAL_2_0,                              \
				.bNumGrpTrmBlock = N_OUTPUTS(n),                                   \
				.baAssoGrpTrmBlkID = {GRPTRM_OUTPUT_BLOCK_IDS(n) 0},               \
			},                                                                         \
		.if1_1_in_ep_fs =                                                                  \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = n + FIRST_IN_EP_ADDR,                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(64U),                            \
			},                                                                         \
		.if1_1_in_ep_hs =                                                                  \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = n + FIRST_IN_EP_ADDR,                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(512U),                           \
			},                                                                         \
		.if1_1_cs_in_ep =                                                                  \
			{                                                                          \
				.bLength = 4 + N_INPUTS(n),                                        \
				.bDescriptorType = USB_DESC_CS_ENDPOINT,                           \
				.bDescriptorSubtype = MS_GENERAL_2_0,                              \
				.bNumGrpTrmBlock = N_INPUTS(n),                                    \
				.baAssoGrpTrmBlkID = {GRPTRM_INPUT_BLOCK_IDS(n) 0},                \
			},                                                                         \
		.grptrm_header =                                                                   \
			{                                                                          \
				.bLength = sizeof(struct usb_midi_grptrm_header_descriptor),       \
				.bDescriptorType = CS_GR_TRM_BLOCK,                                \
				.bDescriptorSubtype = GR_TRM_BLOCK_HEADER,                         \
				.wTotalLength = sys_cpu_to_le16(USBD_MIDI2_GRPTRM_TOTAL_LEN(n)),   \
			},                                                                         \
		.grptrm_blocks = {DT_INST_FOREACH_CHILD(n, USBD_MIDI2_BUILD_GRPTRM_BLOCK)},        \
	};                                                                                         \
	static const struct usb_desc_header						\
		*usbd_midi_desc_array_fs_##n[] = {					\
		(struct usb_desc_header *)&usbd_midi_desc_##n.iad,                                 \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_std,                             \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_cs,                              \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_std,                           \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_ms_header,                     \
		USBD_MIDI1_JACK_PTRS(n)                                                            \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_out_ep_fs,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_cs_out_ep,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_in_ep_fs,                      \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_cs_in_ep,                      \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_std,                           \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_ms_header,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_out_ep_fs,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_cs_out_ep,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_in_ep_fs,                      \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_cs_in_ep,                      \
		NULL,                                                                              \
	};                                                                                         \
	static const struct usb_desc_header						\
		*usbd_midi_desc_array_hs_##n[] = {					\
		(struct usb_desc_header *)&usbd_midi_desc_##n.iad,                                 \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_std,                             \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_cs,                              \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_std,                           \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_ms_header,                     \
		USBD_MIDI1_JACK_PTRS(n)                                                            \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_out_ep_hs,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_cs_out_ep,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_in_ep_hs,                      \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_cs_in_ep,                      \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_std,                           \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_ms_header,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_out_ep_hs,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_cs_out_ep,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_in_ep_hs,                      \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_cs_in_ep,                      \
		NULL,                                                                              \
	};
/* clang-format on */

/* clang-format off */
/* The trailing 0xFF sentinel in usbd_midi_cn_to_group_{out,in}_##n[] below
 * guarantees the arrays are never empty (which would be ill-formed in strict
 * C when an instance has no sinks or no sources). cn_to_group_*_len gives the
 * actual usable length, so the sentinel is never read at runtime.
 */
#define USBD_MIDI_DEFINE_DEVICE(n)                                                                 \
	USBD_MIDI_VALIDATE_INSTANCE(n)                                                             \
	USBD_MIDI_DEFINE_DESCRIPTORS(n);                                                           \
	static const uint8_t usbd_midi_cn_to_group_out_##n[] = {                                   \
		DT_INST_FOREACH_CHILD(n, GRPTRM_GROUP_IF_SINK)                                     \
		0xFF,                                                                              \
	};                                                                                         \
	static const uint8_t usbd_midi_cn_to_group_in_##n[] = {                                    \
		DT_INST_FOREACH_CHILD(n, GRPTRM_GROUP_IF_SOURCE)                                   \
		0xFF,                                                                              \
	};                                                                                         \
	USBD_DEFINE_CLASS(midi_##n, &usbd_midi_class_api, (void *)DEVICE_DT_GET(DT_DRV_INST(n)), \
			  NULL);                                                                   \
	static const struct usbd_midi_config usbd_midi_config_##n = {                              \
		.desc = &usbd_midi_desc_##n,                                                       \
		.fs_descs = usbd_midi_desc_array_fs_##n,                                           \
		.hs_descs = usbd_midi_desc_array_hs_##n,                                           \
		.cn_to_group_out = usbd_midi_cn_to_group_out_##n,                                  \
		.cn_to_group_out_len = N_OUTPUTS(n),                                               \
		.cn_to_group_in = usbd_midi_cn_to_group_in_##n,                                    \
		.cn_to_group_in_len = N_INPUTS(n),                                                 \
	};                                                                                         \
	static struct usbd_midi_data usbd_midi_data_##n = {                                        \
		.class_data = &midi_##n,                                                           \
		.altsetting = MIDI1_ALTERNATE,                                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, usbd_midi_preinit, NULL, &usbd_midi_data_##n,                     \
			      &usbd_midi_config_##n, POST_KERNEL,                                  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(USBD_MIDI_DEFINE_DEVICE)
