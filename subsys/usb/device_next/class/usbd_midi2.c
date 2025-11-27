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
#include <zephyr/audio/midi.h>
#include <errno.h>

#include "usbd_uac2_macros.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_midi2, CONFIG_USBD_MIDI2_LOG_LEVEL);

#define MIDI1_ALTERNATE 0x00
#define MIDI2_ALTERNATE 0x01

UDC_BUF_POOL_DEFINE(usbd_midi_buf_pool, DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * 2, 512U,
		    sizeof(struct udc_buf_info), NULL);

#define MIDI_QUEUE_SIZE CONFIG_USBD_MIDI2_TX_QUEUE_SIZE

BUILD_ASSERT(IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1) ||
		     IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2),
	     "At least one USB-MIDI alternate setting must be enabled");

enum usbd_midi_mode_index {
#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)
	USBD_MIDI_MODE_INDEX_MIDI1_ONLY,
#endif
#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2)
	USBD_MIDI_MODE_INDEX_MIDI2_ONLY,
#endif
#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1) && IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2)
	USBD_MIDI_MODE_INDEX_BOTH,
#endif
	USBD_MIDI_MODE_INDEX_COUNT,
};

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

#define MIDI1_IN_JACK         0x02
#define MIDI1_OUT_JACK        0x03
#define MIDI1_JACK_EMBEDDED   0x01
#define MIDI1_JACK_EXTERNAL   0x02
#define MIDI1_EMB_IN_JACK_ID  0x01
#define MIDI1_EXT_IN_JACK_ID  0x02
#define MIDI1_EMB_OUT_JACK_ID 0x03
#define MIDI1_EXT_OUT_JACK_ID 0x04
#define MIDI1_MS_TOTAL_LEN                                                                         \
	(sizeof(struct usb_midi_header_descriptor) +                                               \
	 2 * sizeof(struct usb_midi_in_jack_descriptor) +                                          \
	 2 * sizeof(struct usb_midi_out_jack_descriptor) + 2 * sizeof(struct usb_ep_descriptor) +  \
	 2 * sizeof(struct usb_midi1_cs_endpoint_descriptor))
#define MIDI1_EVENT_BYTES 4

#define SYSEX_STATUS_COMPLETE 0x00
#define SYSEX_STATUS_START    0x01
#define SYSEX_STATUS_CONTINUE 0x02
#define SYSEX_STATUS_END      0x03

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
	uint8_t baAssocJackID[1];
} __packed;

/* midi20 5.3.2 Class-Specific MIDI Streaming Data Endpoint Descriptor */
struct usb_midi2_cs_endpoint_descriptor {
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

#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)
	/* MidiStreaming 1.0 on altsetting 0 */
	struct usb_if_descriptor if1_0_std;
	struct usb_midi_header_descriptor if1_0_ms_header;
	struct usb_midi_in_jack_descriptor if1_0_emb_in_jack;
	struct usb_midi_in_jack_descriptor if1_0_ext_in_jack;
	struct usb_midi_out_jack_descriptor if1_0_emb_out_jack;
	struct usb_midi_out_jack_descriptor if1_0_ext_out_jack;
	struct usb_ep_descriptor if1_0_out_ep_fs;
	struct usb_ep_descriptor if1_0_out_ep_hs;
	struct usb_midi1_cs_endpoint_descriptor if1_0_cs_out_ep;
	struct usb_ep_descriptor if1_0_in_ep_fs;
	struct usb_ep_descriptor if1_0_in_ep_hs;
	struct usb_midi1_cs_endpoint_descriptor if1_0_cs_in_ep;
#endif

#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2)
	/* MidiStreaming 2.0 on altsetting 1 */
	struct usb_if_descriptor if1_1_std;
	struct usb_midi_header_descriptor if1_1_ms_header;
	struct usb_ep_descriptor if1_1_out_ep_fs;
	struct usb_ep_descriptor if1_1_out_ep_hs;
	struct usb_midi2_cs_endpoint_descriptor if1_1_cs_out_ep;
	struct usb_ep_descriptor if1_1_in_ep_fs;
	struct usb_ep_descriptor if1_1_in_ep_hs;
	struct usb_midi2_cs_endpoint_descriptor if1_1_cs_in_ep;
#endif

	/* MidiStreaming 2.0 Class-Specific Group Terminal Block Descriptors
	 * Retrievable by a Separate Get Request
	 */
	struct usb_midi_grptrm_header_descriptor grptrm_header;
	struct usb_midi_grptrm_block_descriptor grptrm_blocks[16];
};

/* Device driver configuration */
struct usbd_midi_config {
	struct usbd_midi_descriptors *desc;
	const struct usb_desc_header **fs_descs[USBD_MIDI_MODE_INDEX_COUNT];
	const struct usb_desc_header **hs_descs[USBD_MIDI_MODE_INDEX_COUNT];
};

/* Device driver data */
struct usbd_midi_data {
	struct usbd_class_data *class_data;
	struct k_work rx_work;
	struct k_work tx_work;
	uint8_t tx_queue_buf[MIDI_QUEUE_SIZE];
	struct ring_buf tx_queue;
	uint8_t altsetting;
	bool midi1_enabled;
	bool midi2_enabled;
	struct usbd_midi_ops ops;
	uint8_t sysex_buf[3];
	uint8_t sysex_buf_len;
	bool sysex_transfer_active;
};

static void usbd_midi2_recv(const struct device *dev, struct net_buf *const buf);
#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)
static void usbd_midi1_recv(const struct device *dev, struct net_buf *const buf);
#endif

static inline bool usbd_midi_alt_supported(uint8_t alt)
{
	if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1) && alt == MIDI1_ALTERNATE) {
		return true;
	}
	if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2) && alt == MIDI2_ALTERNATE) {
		return true;
	}

	return false;
}

static uint8_t usbd_midi_default_alt(const struct usbd_midi_data *data)
{
	if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1) && data->midi1_enabled) {
		return MIDI1_ALTERNATE;
	}

	if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2) && data->midi2_enabled) {
		return MIDI2_ALTERNATE;
	}

	return MIDI1_ALTERNATE;
}

static enum usbd_midi_mode_index usbd_midi_mode_index(const struct usbd_midi_data *data)
{
	if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1) &&
	    IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2)) {
		if (data->midi1_enabled && data->midi2_enabled) {
			return USBD_MIDI_MODE_INDEX_BOTH;
		}

		if (data->midi1_enabled) {
			return USBD_MIDI_MODE_INDEX_MIDI1_ONLY;
		}

		return USBD_MIDI_MODE_INDEX_MIDI2_ONLY;
	} else if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)) {
		ARG_UNUSED(data);
		return USBD_MIDI_MODE_INDEX_MIDI1_ONLY;
	} else if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2)) {
		ARG_UNUSED(data);
		return USBD_MIDI_MODE_INDEX_MIDI2_ONLY;
	}

	return USBD_MIDI_MODE_INDEX_MIDI1_ONLY;
}

static bool usbd_midi_alt_enabled(const struct usbd_midi_data *data, uint8_t alt)
{
	if (alt == MIDI1_ALTERNATE) {
		return IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1) && data->midi1_enabled;
	}

	if (alt == MIDI2_ALTERNATE) {
		return IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2) && data->midi2_enabled;
	}

	return false;
}

static bool usbd_midi_resolve_alt(const struct usbd_midi_data *data, uint8_t alternate,
				  uint8_t *resolved)
{
	/* When both altsettings are available at runtime, use the requested one directly */
	if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1) &&
	    IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2) &&
	    data->midi1_enabled && data->midi2_enabled) {
		if (usbd_midi_alt_supported(alternate)) {
			*resolved = alternate;
			return true;
		}
		return false;
	}

	/* Only one altsetting available - host must request alternate 0 */
	if (alternate != MIDI1_ALTERNATE) {
		return false;
	}

	/* Resolve to whichever single altsetting is enabled */
	if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1) && data->midi1_enabled) {
		*resolved = MIDI1_ALTERNATE;
		return true;
	}

	if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2) && data->midi2_enabled) {
		*resolved = MIDI2_ALTERNATE;
		return true;
	}

	return false;
}

static void usbd_midi_update_alt_descriptor(const struct usbd_midi_config *cfg,
					    const struct usbd_midi_data *data)
{
#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2)
	if ((cfg == NULL) || (cfg->desc == NULL)) {
		return;
	}

	if (data->midi2_enabled && !data->midi1_enabled) {
		cfg->desc->if1_1_std.bAlternateSetting = MIDI1_ALTERNATE;
	} else {
		cfg->desc->if1_1_std.bAlternateSetting = MIDI2_ALTERNATE;
	}
#else
	ARG_UNUSED(cfg);
	ARG_UNUSED(data);
#endif
}

static const struct usb_desc_header **
usbd_midi_select_desc_array(const struct usbd_midi_config *cfg, const struct usbd_midi_data *data,
			    const enum usbd_speed speed)
{
	enum usbd_midi_mode_index mode = usbd_midi_mode_index(data);

	if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		return cfg->hs_descs[mode];
	}

	return cfg->fs_descs[mode];
}

static void usbd_midi_reset(struct usbd_midi_data *data)
{
	data->altsetting = usbd_midi_default_alt(data);
	ring_buf_reset(&data->tx_queue);
	data->sysex_transfer_active = false;
	data->sysex_buf_len = 0;
}

#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)
static int midi1_cin_payload_len(uint8_t cin)
{
	switch (cin) {
	case MIDI_CIN_SYS_COMMON_2BYTE:
	case MIDI_CIN_PROGRAM_CHANGE:
	case MIDI_CIN_CHANNEL_PRESSURE:
	case MIDI_CIN_SYSEX_END_2BYTE:
		return 2;
	case MIDI_CIN_SYS_COMMON_3BYTE:
	case MIDI_CIN_SYSEX_START:
	case MIDI_CIN_SYSEX_END_3BYTE:
	case MIDI_CIN_NOTE_OFF:
	case MIDI_CIN_NOTE_ON:
	case MIDI_CIN_POLY_KEYPRESS:
	case MIDI_CIN_CONTROL_CHANGE:
	case MIDI_CIN_PITCH_BEND_CHANGE:
		return 3;
	case MIDI_CIN_SYSEX_END_1BYTE:
	case MIDI_CIN_SINGLE_BYTE:
		return 1;
	default:
		return 0;
	}
}

static int midi1_status_payload_len(uint8_t status, uint8_t *cin)
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
		if (cin != NULL) {
			*cin = MIDI_CIN_SYSEX_END_1BYTE;
		}
		return 1;
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

static int midi1_event_to_ump(uint32_t event_le, struct midi_ump *ump)
{
	uint8_t header = event_le & 0xFF;
	uint8_t cable = (header >> 4) & 0x0F;
	uint8_t cin = header & 0x0F;
	uint8_t byte0 = (event_le >> 8) & 0xFF;
	uint8_t byte1 = (event_le >> 16) & 0xFF;
	uint8_t byte2 = (event_le >> 24) & 0xFF;

	if (cin == MIDI_CIN_SYSEX_START || cin == MIDI_CIN_SYSEX_END_1BYTE ||
	    cin == MIDI_CIN_SYSEX_END_2BYTE || cin == MIDI_CIN_SYSEX_END_3BYTE) {
		uint8_t status;
		uint8_t len;

		if (cin == MIDI_CIN_SYSEX_START) {
			/* 0x1 = System Exclusive Start, 0x2 = System Exclusive Continue */
			status = (byte0 == MIDI_STATUS_SYSEX_START) ? SYSEX_STATUS_START
								    : SYSEX_STATUS_CONTINUE;
			len = 3;
		} else if (cin == MIDI_CIN_SYSEX_END_1BYTE) {
			if (byte0 == MIDI_STATUS_TUNE_REQUEST ||
			    byte0 >= MIDI_STATUS_TIMING_CLOCK) {
				*ump = UMP_SYS_RT_COMMON(cable, byte0, 0, 0);
				return 0;
			}
			/* 0x3 = System Exclusive End */
			status = SYSEX_STATUS_END;
			len = 1;
		} else if (cin == MIDI_CIN_SYSEX_END_2BYTE) {
			status = SYSEX_STATUS_END;
			len = 2;
		} else {
			status = SYSEX_STATUS_END;
			len = 3;
		}

		if (byte0 == MIDI_STATUS_SYSEX_START) {
			/* 0x0 = Complete System Exclusive Message */
			status = SYSEX_STATUS_COMPLETE;
		}

		ump->data[0] = (UMP_MT_DATA_64 << 28) | ((cable & 0xF) << 24) |
			       ((status & 0xF) << 20) | ((len & 0xF) << 16) | (byte0 << 8) | byte1;
		ump->data[1] = byte2 << 24;

		return 0;
	}

	uint8_t expected_cin;
	int payload_len = midi1_status_payload_len(byte0, &expected_cin);

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
		uint8_t command = (byte0 >> 4) & 0x0F;
		uint8_t channel = byte0 & 0x0F;

		*ump = UMP_MIDI1_CHANNEL_VOICE(cable, command, channel, byte1, byte2);
	} else {
		*ump = UMP_SYS_RT_COMMON(cable, byte0, byte1, byte2);
	}

	return 0;
}

static int midi1_ump_to_event(const struct midi_ump *ump, uint32_t *event_le)
{
	/* Note: This function only handles single packet messages */
	if (UMP_MT(*ump) != UMP_MT_MIDI1_CHANNEL_VOICE && UMP_MT(*ump) != UMP_MT_SYS_RT_COMMON) {
		return -ENOTSUP;
	}

	uint8_t status = UMP_MIDI_STATUS(*ump);
	uint8_t cin;
	int payload_len = midi1_status_payload_len(status, &cin);

	if (payload_len < 0) {
		return payload_len;
	}

	uint8_t bytes[MIDI1_EVENT_BYTES] = {
		(uint8_t)((UMP_GROUP(*ump) << 4) | cin),
		status,
		UMP_MIDI1_P1(*ump),
		(payload_len == 2) ? 0x00 : UMP_MIDI1_P2(*ump),
	};

	*event_le = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16) |
		    ((uint32_t)bytes[3] << 24);

	return 0;
}

static void usbd_midi1_recv(const struct device *dev, struct net_buf *const buf)
{
	struct usbd_midi_data *data = dev->data;
	struct midi_ump ump;
	int ret;
	uint8_t cable;
	uint8_t midi_bytes[3];
	int payload_len;

	LOG_HEXDUMP_DBG(buf->data, buf->len, "MIDI1 - Rx DATA");
	while (buf->len >= MIDI1_EVENT_BYTES) {
		uint32_t packet = net_buf_pull_le32(buf);

		if (data->ops.rx_midi1_cb) {
			uint8_t cin = packet & 0x0F;

			cable = (packet >> 4) & 0x0F;
			payload_len = midi1_cin_payload_len(cin);

			if (payload_len > 0) {
				midi_bytes[0] = (packet >> 8) & 0xFF;
				midi_bytes[1] = (packet >> 16) & 0xFF;
				midi_bytes[2] = (packet >> 24) & 0xFF;

				data->ops.rx_midi1_cb(dev, cable, midi_bytes, payload_len);
				continue;
			}
		}

		ret = midi1_event_to_ump(packet, &ump);
		if (ret == 0 && data->ops.rx_packet_cb) {
			data->ops.rx_packet_cb(dev, ump);
		}
	}

	if (buf->len) {
		LOG_HEXDUMP_WRN(buf->data, buf->len, "Trailing data in Rx buffer");
	}
}
#endif /* CONFIG_USBD_MIDI2_ALTSETTING_MIDI1 */

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

	LOG_DBG("MIDI2 request for %s ep=%02X len=%d err=%d", dev->name, info->ep, buf->len, err);

	if (err && err != -ECONNABORTED) {
		LOG_ERR("Transfer error %d", err);
	}
	if (USB_EP_DIR_IS_OUT(info->ep)) {
		if (data->altsetting == MIDI1_ALTERNATE) {
			if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)) {
				usbd_midi1_recv(dev, buf);
			} else {
				LOG_WRN("Legacy altsetting selected but MIDI 1.0 support is "
					"disabled");
				net_buf_pull(buf, buf->len);
			}
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

static void usbd_midi_class_update(struct usbd_class_data *const class_data, const uint8_t iface,
				   const uint8_t alternate)
{
	const struct device *dev = usbd_class_get_private(class_data);
	bool ready = false;
	struct usbd_midi_data *data = dev->data;
	uint8_t resolved_alt;

	LOG_DBG("update for %s: if=%u, alt=%u", dev->name, iface, alternate);

	if (!usbd_midi_resolve_alt(data, alternate, &resolved_alt)) {
		LOG_WRN("%s requested unsupported altsetting %u", dev->name, alternate);
	} else if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1) &&
		   resolved_alt == MIDI1_ALTERNATE) {
		data->altsetting = MIDI1_ALTERNATE;
		ready = true;
		LOG_INF("%s set USB-MIDI1.0 altsetting", dev->name);
	} else if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2) &&
		   resolved_alt == MIDI2_ALTERNATE) {
		data->altsetting = MIDI2_ALTERNATE;
		ready = true;
		LOG_INF("%s set USB-MIDI2.0 altsetting", dev->name);
	}

	if (ready) {
		ring_buf_reset(&data->tx_queue);
		k_work_submit(&data->rx_work);
	}

	if (data->ops.ready_cb) {
		data->ops.ready_cb(dev, ready);
	}
}

static void usbd_midi_class_enable(struct usbd_class_data *const class_data)
{
	const struct device *dev = usbd_class_get_private(class_data);
	struct usbd_midi_data *data = dev->data;

	usbd_midi_reset(data);

	if (data->ops.ready_cb) {
		data->ops.ready_cb(dev, true);
	}

	LOG_DBG("Enable %s", dev->name);
	k_work_submit(&data->rx_work);
}

static void usbd_midi_class_disable(struct usbd_class_data *const class_data)
{
	const struct device *dev = usbd_class_get_private(class_data);
	struct usbd_midi_data *data = dev->data;
	struct k_work_sync sync;

	if (data->ops.ready_cb) {
		data->ops.ready_cb(dev, false);
	}

	LOG_DBG("Disable %s", dev->name);
	k_work_cancel_sync(&data->rx_work, &sync);
	usbd_midi_reset(data);
}

static void usbd_midi_class_suspended(struct usbd_class_data *const class_data)
{
	const struct device *dev = usbd_class_get_private(class_data);
	struct usbd_midi_data *data = dev->data;
	struct k_work_sync sync;

	if (data->ops.ready_cb) {
		data->ops.ready_cb(dev, false);
	}

	LOG_DBG("Suspend %s", dev->name);
	k_work_cancel_sync(&data->rx_work, &sync);
	usbd_midi_reset(data);
}

static void usbd_midi_class_resumed(struct usbd_class_data *const class_data)
{
	const struct device *dev = usbd_class_get_private(class_data);
	struct usbd_midi_data *data = dev->data;

	if (usbd_midi_alt_enabled(data, data->altsetting) && data->ops.ready_cb) {
		data->ops.ready_cb(dev, true);
	}

	LOG_DBG("Resume %s", dev->name);
	k_work_submit(&data->rx_work);
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
	if (data->altsetting != MIDI2_ALTERNATE || setup->bRequest != USB_SREQ_GET_DESCRIPTOR ||
	    setup->wValue != ((CS_GR_TRM_BLOCK << 8) | MIDI2_ALTERNATE)) {
		errno = -ENOTSUP;
		return 0;
	}

	/* Group terminal block header */
	net_buf_add_mem(buf, (void *)&config->desc->grptrm_header, MIN(head_len, setup->wLength));

	/* Group terminal blocks */
	if (setup->wLength > head_len && total_len > head_len) {
		net_buf_add_mem(buf, (void *)&config->desc->grptrm_blocks,
				MIN(total_len, setup->wLength) - head_len);
	}
	LOG_HEXDUMP_DBG(buf->data, buf->len, "Control to host");

	return 0;
}

/**
 * @brief Initialize MIDI class
 *
 * Updates the Audio Control header descriptor's baInterfaceNr1 field to
 * reference the actual MIDI Streaming interface number assigned by the USB
 * stack. This is necessary for composite devices where MIDI is not the first
 * interface (e.g., when DFU is interface 0).
 */
static int usbd_midi_class_init(struct usbd_class_data *const class_data)
{
	const struct device *dev = usbd_class_get_private(class_data);
	const struct usbd_midi_config *config = dev->config;
	struct usbd_midi_descriptors *desc = config->desc;

	LOG_DBG("Init %s device class", dev->name);

	/* Update Audio Control header to reference the actual MIDI Streaming interface */
#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)
	desc->if0_cs.baInterfaceNr1 = desc->if1_0_std.bInterfaceNumber;
#else
	desc->if0_cs.baInterfaceNr1 = desc->if1_1_std.bInterfaceNumber;
#endif
	LOG_DBG("Set baInterfaceNr1 to %u", desc->if0_cs.baInterfaceNr1);

	return 0;
}

static void *usbd_midi_class_get_desc(struct usbd_class_data *const class_data,
				      const enum usbd_speed speed)
{
	const struct device *dev = usbd_class_get_private(class_data);
	const struct usbd_midi_config *config = dev->config;
	const struct usbd_midi_data *data = dev->data;

	LOG_DBG("Get descriptors for %s", dev->name);

	usbd_midi_update_alt_descriptor(config, data);

	return (void *)usbd_midi_select_desc_array(config, data, speed);
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
	const struct usb_ep_descriptor *in_ep_fs;
	const struct usb_ep_descriptor *in_ep_hs;

#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2)
	in_ep_fs = &cfg->desc->if1_1_in_ep_fs;
	in_ep_hs = &cfg->desc->if1_1_in_ep_hs;
#else
	in_ep_fs = &cfg->desc->if1_0_in_ep_fs;
	in_ep_hs = &cfg->desc->if1_0_in_ep_hs;
#endif

	if (USBD_SUPPORTS_HIGH_SPEED && usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return in_ep_hs->bEndpointAddress;
	}

	return in_ep_fs->bEndpointAddress;
}

static uint8_t usbd_midi_get_bulk_out(struct usbd_class_data *const class_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(class_data);
	const struct device *dev = usbd_class_get_private(class_data);
	const struct usbd_midi_config *cfg = dev->config;
	const struct usb_ep_descriptor *out_ep_fs;
	const struct usb_ep_descriptor *out_ep_hs;

#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2)
	out_ep_fs = &cfg->desc->if1_1_out_ep_fs;
	out_ep_hs = &cfg->desc->if1_1_out_ep_hs;
#else
	out_ep_fs = &cfg->desc->if1_0_out_ep_fs;
	out_ep_hs = &cfg->desc->if1_0_out_ep_hs;
#endif

	if (USBD_SUPPORTS_HIGH_SPEED && usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return out_ep_hs->bEndpointAddress;
	}

	return out_ep_fs->bEndpointAddress;
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
		if (ret == -ENODEV) {
			LOG_DBG("Rx enqueue requested while device is disabled");
		} else {
			LOG_ERR("Failed to enqueue Rx net_buf -> %d", ret);
		}
		net_buf_unref(buf);
	}
}

static void usbd_midi_tx_work(struct k_work *work)
{
	struct usbd_midi_data *data = CONTAINER_OF(work, struct usbd_midi_data, tx_work);
	struct net_buf *buf = usbd_midi_buf_alloc(usbd_midi_get_bulk_in(data->class_data));
	int ret;

	if (buf == NULL) {
		LOG_ERR("Unable to allocate Tx net_buf");
		return;
	}

	net_buf_add(buf, ring_buf_get(&data->tx_queue, buf->data, buf->size));
	LOG_HEXDUMP_DBG(buf->data, buf->len, "MIDI2 - Tx DATA");

	ret = usbd_ep_enqueue(data->class_data, buf);
	if (ret != 0) {
		LOG_ERR("Failed to enqueue Tx net_buf -> %d", ret);
		net_buf_unref(buf);
	}
}

static int usbd_midi_preinit(const struct device *dev)
{
	struct usbd_midi_data *data = dev->data;

	LOG_DBG("Init device %s", dev->name);
	ring_buf_init(&data->tx_queue, MIDI_QUEUE_SIZE, data->tx_queue_buf);
	k_work_init(&data->rx_work, usbd_midi_rx_work);
	k_work_init(&data->tx_work, usbd_midi_tx_work);

	return 0;
}

#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)
static uint8_t midi1_sysex_cin_from_len(uint8_t chunk_len)
{
	if (chunk_len == 1) {
		return MIDI_CIN_SYSEX_END_1BYTE;
	} else if (chunk_len == 2) {
		return MIDI_CIN_SYSEX_END_2BYTE;
	} else {
		return MIDI_CIN_SYSEX_END_3BYTE;
	}
}
#endif

int usbd_midi_send(const struct device *dev, const struct midi_ump ump)
{
	struct usbd_midi_data *data = dev->data;
	size_t words = UMP_NUM_WORDS(ump);
	size_t buflen = 4 * words;
	size_t needed = buflen;
	uint32_t word;
	int ret;

	ARG_UNUSED(ret);

	LOG_DBG("Send MT=%X group=%X", UMP_MT(ump), UMP_GROUP(ump));

	if (!usbd_midi_alt_enabled(data, data->altsetting)) {
		return -EIO;
	}
	if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1) && data->altsetting == MIDI1_ALTERNATE) {
		if (UMP_MT(ump) == UMP_MT_DATA_64) {
			needed = 8; /* Worst case: 6 bytes -> 2 packets (8 bytes) */
		} else {
			needed = MIDI1_EVENT_BYTES;
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
	} else if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)) {
		if (UMP_MT(ump) == UMP_MT_DATA_64) {
			uint8_t group = UMP_GROUP(ump);
			uint8_t status = (ump.data[0] >> 20) & 0xF;
			uint8_t len = (ump.data[0] >> 16) & 0xF;
			uint8_t bytes[6];

			bytes[0] = (ump.data[0] >> 8) & 0xFF;
			bytes[1] = ump.data[0] & 0xFF;
			bytes[2] = (ump.data[1] >> 24) & 0xFF;
			bytes[3] = (ump.data[1] >> 16) & 0xFF;
			bytes[4] = (ump.data[1] >> 8) & 0xFF;
			bytes[5] = ump.data[1] & 0xFF;

			size_t processed = 0;

			while (processed < len) {
				uint8_t chunk_len = MIN(3, len - processed);
				uint8_t cin;

				if (processed + chunk_len < len) {
					cin = MIDI_CIN_SYSEX_START;
				} else {
					if (status == SYSEX_STATUS_START ||
					    status == SYSEX_STATUS_CONTINUE) {
						cin = MIDI_CIN_SYSEX_START;
					} else {
						cin = midi1_sysex_cin_from_len(chunk_len);
					}
				}

				uint32_t event_le = (group << 4) | cin;

				event_le |= ((uint32_t)bytes[processed] << 8);
				if (chunk_len > 1) {
					event_le |= ((uint32_t)bytes[processed + 1] << 16);
				}
				if (chunk_len > 2) {
					event_le |= ((uint32_t)bytes[processed + 2] << 24);
				}

				event_le = sys_cpu_to_le32(event_le);
				ring_buf_put(&data->tx_queue, (const uint8_t *)&event_le, 4);
				processed += chunk_len;
			}
		} else {
			ret = midi1_ump_to_event(&ump, &word);
			if (ret) {
				return ret;
			}
			word = sys_cpu_to_le32(word);
			ring_buf_put(&data->tx_queue, (const uint8_t *)&word, MIDI1_EVENT_BYTES);
		}
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

int usbd_midi_set_mode(const struct device *dev, bool enable_midi1, bool enable_midi2)
{
	struct usbd_midi_data *data = dev->data;
	const struct usbd_midi_config *cfg = dev->config;
	struct usbd_context *uds_ctx = NULL;

	if (enable_midi1 && !IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)) {
		return -ENOTSUP;
	}

	if (enable_midi2 && !IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2)) {
		return -ENOTSUP;
	}

	if (!enable_midi1 && !enable_midi2) {
		return -EINVAL;
	}

	if (data->class_data != NULL) {
		uds_ctx = usbd_class_get_ctx(data->class_data);
	}

	if ((uds_ctx != NULL) && uds_ctx->status.enabled) {
		return -EBUSY;
	}

	if (data->midi1_enabled == enable_midi1 && data->midi2_enabled == enable_midi2) {
		return 0;
	}

	data->midi1_enabled = enable_midi1;
	data->midi2_enabled = enable_midi2;

	usbd_midi_reset(data);
	usbd_midi_update_alt_descriptor(cfg, data);

	return 0;
}

#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)
static int usbd_midi_sysex_send_midi1(struct usbd_midi_data *data, uint8_t cable_number,
				      bool is_end)
{
	uint8_t cin;

	/* MIDI 1.0 Protocol */
	if (is_end) {
		cin = midi1_sysex_cin_from_len(data->sysex_buf_len);
	} else {
		cin = MIDI_CIN_SYSEX_START;
	}

	uint32_t event_le = (cable_number << 4) | cin;

	event_le |= ((uint32_t)data->sysex_buf[0] << 8);
	if (data->sysex_buf_len > 1) {
		event_le |= ((uint32_t)data->sysex_buf[1] << 16);
	}
	if (data->sysex_buf_len > 2) {
		event_le |= ((uint32_t)data->sysex_buf[2] << 24);
	}

	event_le = sys_cpu_to_le32(event_le);
	if (ring_buf_space_get(&data->tx_queue) < 4) {
		/* Restore state on error? Difficult. Return
		 * error.
		 */
		return -ENOBUFS;
	}
	ring_buf_put(&data->tx_queue, (const uint8_t *)&event_le, 4);
	k_work_submit(&data->tx_work);

	return 0;
}
#endif /* CONFIG_USBD_MIDI2_ALTSETTING_MIDI1 */

#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1) &&                                           \
	IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2)
static int usbd_midi_sysex_send_midi2(const struct device *dev, uint8_t cable_number, bool is_end)
{
	struct usbd_midi_data *data = dev->data;
	/* MIDI 2.0 Protocol (UMP) */
	struct midi_ump ump;
	uint8_t ump_status;

	if (data->sysex_buf_len == 3 && !is_end && data->sysex_buf[0] == MIDI_STATUS_SYSEX_START) {
		ump_status = SYSEX_STATUS_START;
	} else if (is_end) {
		/* If single packet total? Not tracked
		 * easily here without more state, but
		 * SYSEX_STATUS_END is valid for single
		 * packet messages too if we assume previous
		 * was Start/Continue.
		 */
		ump_status = SYSEX_STATUS_END;
	} else {
		ump_status = SYSEX_STATUS_CONTINUE;
	}

	/* Special case correction for first packet */
	if (data->sysex_buf[0] == MIDI_STATUS_SYSEX_START && !is_end) {
		ump_status = SYSEX_STATUS_START;
	} else if (data->sysex_buf[0] == MIDI_STATUS_SYSEX_START && is_end) {
		ump_status = SYSEX_STATUS_COMPLETE;
	}

	memset(&ump, 0, sizeof(ump));
	ump.data[0] = (UMP_MT_DATA_64 << 28) | ((cable_number & 0xF) << 24) |
		      ((ump_status & 0xF) << 20) | ((data->sysex_buf_len & 0xF) << 16) |
		      (data->sysex_buf[0] << 8);

	if (data->sysex_buf_len > 1) {
		ump.data[0] |= data->sysex_buf[1];
	}
	if (data->sysex_buf_len > 2) {
		ump.data[1] |= (data->sysex_buf[2] << 24);
	}

	return usbd_midi_send(dev, ump);
}
#endif /* CONFIG_USBD_MIDI2_ALTSETTING_MIDI1 && CONFIG_USBD_MIDI2_ALTSETTING_MIDI2 */

#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)
int usbd_midi_send_midi1(const struct device *dev, uint8_t cable_number, const uint8_t *midi_bytes,
			 size_t len)
{
	struct usbd_midi_data *data = dev->data;
	size_t offset = 0;
	int ret;

	if (!usbd_midi_alt_enabled(data, data->altsetting)) {
		return -EIO;
	}

	while (offset < len) {
		uint8_t byte = midi_bytes[offset];
		uint8_t cin = 0;
		int msg_len = 0;

		/* Handle System Exclusive Messages (Stateful) */
		if (byte == MIDI_STATUS_SYSEX_START || data->sysex_transfer_active) {
			if (byte == MIDI_STATUS_SYSEX_START) {
				data->sysex_transfer_active = true;
				data->sysex_buf_len = 0;
				/* Skip the F0 byte, we will consume it into the buffer */
			}

			size_t available = len - offset;
			size_t processed = 0;

			while (processed < available) {
				uint8_t b = midi_bytes[offset + processed];

				/* Store byte in temp buffer */
				data->sysex_buf[data->sysex_buf_len++] = b;
				processed++;

				/* Check for End of SysEx */
				bool is_end = (b == MIDI_STATUS_SYSEX_END);

				if (is_end) {
					data->sysex_transfer_active = false;
				}

				/* If buffer full (3 bytes) or End of SysEx, send packet */
				if (data->sysex_buf_len == 3 || is_end) {
					if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1) &&
					    data->altsetting == MIDI1_ALTERNATE) {
						ret = usbd_midi_sysex_send_midi1(data, cable_number,
										 is_end);
					} else if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2) &&
						   data->altsetting == MIDI2_ALTERNATE) {
						ret = usbd_midi_sysex_send_midi2(dev, cable_number,
										 is_end);
					}

					if (ret < 0) {
						return ret;
					}

					data->sysex_buf_len = 0;
				}

				if (is_end) {
					break;
				}
			}
			offset += processed;
			continue;
		}

		if (byte >= MIDI_STATUS_NOTE_OFF) {
			/* Determine payload length for other messages */
			msg_len = midi1_status_payload_len(byte, &cin);
			if (msg_len < 0) {
				return -EINVAL;
			}
		} else {
			/* Running status or invalid data byte */
			return -EINVAL;
		}

		if (offset + msg_len > len) {
			return -EINVAL;
		}

		if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1) &&
		    data->altsetting == MIDI1_ALTERNATE) {
			uint32_t event_le = (cable_number << 4) | cin;

			event_le |= ((uint32_t)midi_bytes[offset] << 8);
			if (msg_len > 1) {
				event_le |= ((uint32_t)midi_bytes[offset + 1] << 16);
			}
			if (msg_len > 2) {
				event_le |= ((uint32_t)midi_bytes[offset + 2] << 24);
			}

			event_le = sys_cpu_to_le32(event_le);
			if (ring_buf_space_get(&data->tx_queue) < 4) {
				return -ENOBUFS;
			}
			ring_buf_put(&data->tx_queue, (const uint8_t *)&event_le, 4);
			k_work_submit(&data->tx_work);
		} else if (IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2) &&
			   data->altsetting == MIDI2_ALTERNATE) {
			struct midi_ump ump;
			uint8_t d1 = (msg_len > 1) ? midi_bytes[offset + 1] : 0;
			uint8_t d2 = (msg_len > 2) ? midi_bytes[offset + 2] : 0;

			if (byte < MIDI_STATUS_SYSEX_START) {
				ump = UMP_MIDI1_CHANNEL_VOICE(cable_number, (byte >> 4),
							      (byte & 0xF), d1, d2);
			} else {
				ump = UMP_SYS_RT_COMMON(cable_number, byte, d1, d2);
			}
			ret = usbd_midi_send(dev, ump);
			if (ret) {
				return ret;
			}
		}
		offset += msg_len;
	}

	return 0;
}
#endif /* CONFIG_USBD_MIDI2_ALTSETTING_MIDI1 */

/* Group Terminal Block unique identification number, type and protocol
 * see midi20 5.4.2 Group Terminal Block Descriptor
 */
#define GRPTRM_BLOCK_ID(node) UTIL_INC(DT_NODE_CHILD_IDX(node))
#define GRPTRM_BLOCK_TYPE(node)                                                                    \
	COND_CODE_1(DT_ENUM_HAS_VALUE(node, terminal_type, input_only),           \
		(GR_TRM_INPUT_ONLY),                                              \
		(COND_CODE_1(DT_ENUM_HAS_VALUE(node, terminal_type, output_only), \
			(GR_TRM_OUTPUT_ONLY),                                     \
			(GR_TRM_BIDIRECTIONAL)                                    \
		))                                                                \
	)
#define GRPTRM_PROTOCOL(node)                                                                      \
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
#define GRPTRM_BLOCK_ID_SEP_IF(node, ttype)                                                        \
	IF_ENABLED(                                                            \
		UTIL_OR(DT_ENUM_HAS_VALUE(node, terminal_type, bidirectional), \
			DT_ENUM_HAS_VALUE(node, terminal_type, ttype)),        \
		(GRPTRM_BLOCK_ID(node),))

/* All unique identification numbers of output+bidir group terminal blocks */
#define GRPTRM_OUTPUT_BLOCK_IDS(n)                                                                 \
	DT_INST_FOREACH_CHILD_VARGS(n, GRPTRM_BLOCK_ID_SEP_IF, output_only)

/* All unique identification numbers of input+bidir group terminal blocks */
#define GRPTRM_INPUT_BLOCK_IDS(n) DT_INST_FOREACH_CHILD_VARGS(n, GRPTRM_BLOCK_ID_SEP_IF, input_only)

#define N_INPUTS(n)  sizeof((uint8_t[]){GRPTRM_INPUT_BLOCK_IDS(n)})
#define N_OUTPUTS(n) sizeof((uint8_t[]){GRPTRM_OUTPUT_BLOCK_IDS(n)})

#define USBD_MIDI_VALIDATE_GRPTRM_BLOCK(node)                                                      \
	BUILD_ASSERT(DT_REG_ADDR(node) < 16, "Group Terminal Block address must be within 0..15"); \
	BUILD_ASSERT(DT_REG_ADDR(node) + DT_REG_SIZE(node) <= 16,                                  \
		     "Too many Group Terminals in this Block");                                    \
	BUILD_ASSERT(DT_REG_SIZE(node) == 1,                                                       \
		     "MIDI 1.0 compatibility currently supports one group per block");

#define USBD_MIDI_VALIDATE_INSTANCE(n)                                                             \
	DT_INST_FOREACH_CHILD(n, USBD_MIDI_VALIDATE_GRPTRM_BLOCK);                                 \
	BUILD_ASSERT(DT_INST_CHILD_NUM_STATUS_OKAY(n) >= 1,                                        \
		     "At least one Group Terminal Block is required");

#define USBD_MIDI2_INIT_GRPTRM_BLOCK_DESCRIPTOR(node)                                              \
	{                                                                                          \
		.bLength = sizeof(struct usb_midi_grptrm_block_descriptor),                        \
		.bDescriptorType = CS_GR_TRM_BLOCK,                                                \
		.bDescriptorSubtype = GR_TRM_BLOCK,                                                \
		.bGrpTrmBlkID = GRPTRM_BLOCK_ID(node),                                             \
		.bGrpTrmBlkType = GRPTRM_BLOCK_TYPE(node),                                         \
		.nGroupTrm = DT_REG_ADDR(node),                                                    \
		.nNumGroupTrm = DT_REG_SIZE(node),                                                 \
		.iBlockItem = 0,                                                                   \
		.bMIDIProtocol = GRPTRM_PROTOCOL(node),                                            \
		.wMaxInputBandwidth = 0x0000,                                                      \
		.wMaxOutputBandwidth = 0x0000,                                                     \
	}

#define USBD_MIDI2_BUILD_GRPTRM_BLOCK(node) USBD_MIDI2_INIT_GRPTRM_BLOCK_DESCRIPTOR(node),

#define USBD_MIDI2_GRPTRM_TOTAL_LEN(n)                                                             \
	sizeof(struct usb_midi_grptrm_header_descriptor) +                                         \
		DT_INST_CHILD_NUM_STATUS_OKAY(n) * sizeof(struct usb_midi_grptrm_block_descriptor)

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
		IF_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1, (                                   \
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
				.wTotalLength = sys_cpu_to_le16(MIDI1_MS_TOTAL_LEN),               \
			},                                                                         \
		.if1_0_emb_in_jack =                                                               \
			{                                                                          \
				.bLength = sizeof(struct usb_midi_in_jack_descriptor),             \
				.bDescriptorType = USB_DESC_CS_INTERFACE,                          \
				.bDescriptorSubtype = MIDI1_IN_JACK,                               \
				.bJackType = MIDI1_JACK_EMBEDDED,                                  \
				.bJackID = MIDI1_EMB_IN_JACK_ID,                                   \
			},                                                                         \
		.if1_0_ext_in_jack =                                                               \
			{                                                                          \
				.bLength = sizeof(struct usb_midi_in_jack_descriptor),             \
				.bDescriptorType = USB_DESC_CS_INTERFACE,                          \
				.bDescriptorSubtype = MIDI1_IN_JACK,                               \
				.bJackType = MIDI1_JACK_EXTERNAL,                                  \
				.bJackID = MIDI1_EXT_IN_JACK_ID,                                   \
			},                                                                         \
		.if1_0_emb_out_jack =                                                              \
			{                                                                          \
				.bLength = sizeof(struct usb_midi_out_jack_descriptor),            \
				.bDescriptorType = USB_DESC_CS_INTERFACE,                          \
				.bDescriptorSubtype = MIDI1_OUT_JACK,                              \
				.bJackType = MIDI1_JACK_EMBEDDED,                                  \
				.bJackID = MIDI1_EMB_OUT_JACK_ID,                                  \
				.bNrInputPins = 1,                                                 \
				.baSourceID = MIDI1_EXT_IN_JACK_ID,                                \
				.baSourcePin = 0x01,                                               \
			},                                                                         \
		.if1_0_ext_out_jack =                                                              \
			{                                                                          \
				.bLength = sizeof(struct usb_midi_out_jack_descriptor),            \
				.bDescriptorType = USB_DESC_CS_INTERFACE,                          \
				.bDescriptorSubtype = MIDI1_OUT_JACK,                              \
				.bJackType = MIDI1_JACK_EXTERNAL,                                  \
				.bJackID = MIDI1_EXT_OUT_JACK_ID,                                  \
				.bNrInputPins = 1,                                                 \
				.baSourceID = MIDI1_EMB_IN_JACK_ID,                                \
				.baSourcePin = 0x01,                                               \
			},                                                                         \
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
				.bLength = sizeof(struct usb_midi1_cs_endpoint_descriptor),        \
				.bDescriptorType = USB_DESC_CS_ENDPOINT,                           \
				.bDescriptorSubtype = MS_GENERAL,                                  \
				.bNumEmbMIDIJack = 1,                                              \
				.baAssocJackID = {MIDI1_EMB_IN_JACK_ID},                           \
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
				.bLength = sizeof(struct usb_midi1_cs_endpoint_descriptor),        \
				.bDescriptorType = USB_DESC_CS_ENDPOINT,                           \
				.bDescriptorSubtype = MS_GENERAL,                                  \
				.bNumEmbMIDIJack = 1,                                              \
				.baAssocJackID = {MIDI1_EMB_OUT_JACK_ID},                          \
			},                                                                         \
		))                                                                                 \
		IF_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2, (                                   \
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
				.baAssoGrpTrmBlkID = {GRPTRM_OUTPUT_BLOCK_IDS(n)},                 \
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
				.baAssoGrpTrmBlkID = {GRPTRM_INPUT_BLOCK_IDS(n)},                  \
			},                                                                         \
		))                                                                                 \
		.grptrm_header =                                                                   \
			{                                                                          \
				.bLength = sizeof(struct usb_midi_grptrm_header_descriptor),       \
				.bDescriptorType = CS_GR_TRM_BLOCK,                                \
				.bDescriptorSubtype = GR_TRM_BLOCK_HEADER,                         \
				.wTotalLength = sys_cpu_to_le16(USBD_MIDI2_GRPTRM_TOTAL_LEN(n)),   \
			},                                                                         \
		.grptrm_blocks = {DT_INST_FOREACH_CHILD(n, USBD_MIDI2_BUILD_GRPTRM_BLOCK)},        \
	}; \
	IF_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1, (                                           \
	static const struct usb_desc_header *usbd_midi_desc_array_fs_midi1_##n[] = {               \
		(struct usb_desc_header *)&usbd_midi_desc_##n.iad,                                 \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_std,                             \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_cs,                              \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_std,                           \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_ms_header,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_emb_in_jack,                   \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_ext_in_jack,                   \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_emb_out_jack,                  \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_ext_out_jack,                  \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_out_ep_fs,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_cs_out_ep,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_in_ep_fs,                      \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_cs_in_ep,                      \
		NULL,                                                                              \
	}; \
	)) \
	IF_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2, (                                           \
	static const struct usb_desc_header *usbd_midi_desc_array_fs_midi2_##n[] = {               \
		(struct usb_desc_header *)&usbd_midi_desc_##n.iad,                                 \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_std,                             \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_cs,                              \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_std,                           \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_ms_header,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_out_ep_fs,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_cs_out_ep,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_in_ep_fs,                      \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_cs_in_ep,                      \
		NULL,                                                                              \
	}; \
	)) \
	IF_ENABLED(                                                                                \
		UTIL_AND(                                                                          \
			IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1),                            \
			IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2)                             \
		), (                                                                               \
	static const struct usb_desc_header *usbd_midi_desc_array_fs_both_##n[] = {                \
		(struct usb_desc_header *)&usbd_midi_desc_##n.iad,                                 \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_std,                             \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_cs,                              \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_std,                           \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_ms_header,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_emb_in_jack,                   \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_ext_in_jack,                   \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_emb_out_jack,                  \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_ext_out_jack,                  \
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
	}; \
	)) \
	IF_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1, (                                           \
	static const struct usb_desc_header *usbd_midi_desc_array_hs_midi1_##n[] = {               \
		(struct usb_desc_header *)&usbd_midi_desc_##n.iad,                                 \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_std,                             \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_cs,                              \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_std,                           \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_ms_header,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_emb_in_jack,                   \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_ext_in_jack,                   \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_emb_out_jack,                  \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_ext_out_jack,                  \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_out_ep_hs,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_cs_out_ep,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_in_ep_hs,                      \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_cs_in_ep,                      \
		NULL,                                                                              \
	}; \
	)) \
	IF_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2, (                                           \
	static const struct usb_desc_header *usbd_midi_desc_array_hs_midi2_##n[] = {               \
		(struct usb_desc_header *)&usbd_midi_desc_##n.iad,                                 \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_std,                             \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_cs,                              \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_std,                           \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_ms_header,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_out_ep_hs,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_cs_out_ep,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_in_ep_hs,                      \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_1_cs_in_ep,                      \
		NULL,                                                                              \
	}; \
	)) \
	IF_ENABLED(                                                                                \
		UTIL_AND(                                                                          \
			IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1),                            \
			IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2)                             \
		), (                                                                               \
	static const struct usb_desc_header *usbd_midi_desc_array_hs_both_##n[] = {                \
		(struct usb_desc_header *)&usbd_midi_desc_##n.iad,                                 \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_std,                             \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if0_cs,                              \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_std,                           \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_ms_header,                     \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_emb_in_jack,                   \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_ext_in_jack,                   \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_emb_out_jack,                  \
		(struct usb_desc_header *)&usbd_midi_desc_##n.if1_0_ext_out_jack,                  \
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
	}; \
	))
/* clang-format on */

/* clang-format off */
#define USBD_MIDI_DEFINE_DEVICE(n)                                                                 \
	USBD_MIDI_VALIDATE_INSTANCE(n)                                                             \
	USBD_MIDI_DEFINE_DESCRIPTORS(n);                                                           \
	USBD_DEFINE_CLASS(midi_##n, &usbd_midi_class_api, (void *)DEVICE_DT_GET(DT_DRV_INST(n)),   \
			  NULL);                                                                   \
	static const struct usbd_midi_config usbd_midi_config_##n = {                              \
		.desc = &usbd_midi_desc_##n,                                                       \
		.fs_descs = {                                                                      \
			IF_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1, (                           \
				[USBD_MIDI_MODE_INDEX_MIDI1_ONLY] =                                \
					usbd_midi_desc_array_fs_midi1_##n,                         \
			))                                                                         \
			IF_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2, (                           \
				[USBD_MIDI_MODE_INDEX_MIDI2_ONLY] =                                \
					usbd_midi_desc_array_fs_midi2_##n,                         \
			))                                                                         \
			IF_ENABLED(                                                                \
				UTIL_AND(                                                          \
					IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1),            \
					IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2)             \
				), (                                                               \
				[USBD_MIDI_MODE_INDEX_BOTH] =                                      \
					usbd_midi_desc_array_fs_both_##n,                          \
			)) \
		}, \
		.hs_descs = {                                                                      \
			IF_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1, (                           \
				[USBD_MIDI_MODE_INDEX_MIDI1_ONLY] =                                \
					usbd_midi_desc_array_hs_midi1_##n,                         \
			)) \
			IF_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2, (                           \
				[USBD_MIDI_MODE_INDEX_MIDI2_ONLY] =                                \
					usbd_midi_desc_array_hs_midi2_##n,                         \
			)) \
			IF_ENABLED(                                                                \
				UTIL_AND(                                                          \
					IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1),            \
					IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2)             \
				), (                                                               \
				[USBD_MIDI_MODE_INDEX_BOTH] =                                      \
					usbd_midi_desc_array_hs_both_##n,                          \
			)) \
		}, \
	};                                                                                         \
	static struct usbd_midi_data usbd_midi_data_##n = {                                        \
		.class_data = &midi_##n,                                                           \
		.altsetting = COND_CODE_1(                                                         \
			IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1),                            \
			(MIDI1_ALTERNATE), (MIDI2_ALTERNATE)),                                     \
		.midi1_enabled = COND_CODE_1(                                                      \
			IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1),                            \
			(true), (false)),                                                          \
		.midi2_enabled = COND_CODE_1(                                                      \
			IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI2),                            \
			(true), (false)),                                                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, usbd_midi_preinit, NULL, &usbd_midi_data_##n,                     \
			      &usbd_midi_config_##n, POST_KERNEL,                                  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(USBD_MIDI_DEFINE_DEVICE)
