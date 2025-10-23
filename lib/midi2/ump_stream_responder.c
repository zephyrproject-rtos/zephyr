/*
 * Copyright (c) 2025 Titouan Christophe
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/byteorder.h>

#include "ump_stream_responder.h"

#define BIT_IF(cond, n) ((cond) ? BIT(n) : 0)

/**
 * @brief  MIDI-CI version identifier for UMP v1.1 devices
 * @see ump112: 7.1.8 FB Info Notification > MIDI-CI Message Version/Format
 */
#define MIDI_CI_VERSION_FORMAT_UMP_1_1 0x01

static inline bool ep_has_midi1(const struct ump_endpoint_dt_spec *ep)
{
	for (size_t i = 0; i < ep->n_blocks; i++) {
		if (ep->blocks[i].is_midi1) {
			return true;
		}
	}
	return false;
}

static inline bool ep_has_midi2(const struct ump_endpoint_dt_spec *ep)
{
	for (size_t i = 0; i < ep->n_blocks; i++) {
		if (!ep->blocks[i].is_midi1) {
			return true;
		}
	}
	return false;
}

/**
 * @brief  Build an Endpoint Info Notification Universal MIDI Packet
 * @see	ump112: 7.1.2 Endpoint Info Notification Message
 */
static inline struct midi_ump make_endpoint_info(const struct ump_endpoint_dt_spec *ep)
{
	struct midi_ump res;

	res.data[0] = (UMP_MT_UMP_STREAM << 28)
		    | (UMP_STREAM_STATUS_EP_INFO << 16)
		    | 0x0101; /* UMP version 1.1 */

	res.data[1] = BIT(31) /* Static function blocks */
		    | ((ep->n_blocks) << 24)
		    | BIT_IF(ep_has_midi2(ep), 9)
		    | BIT_IF(ep_has_midi1(ep), 8);

	return res;
}

/**
 * @brief  Build a Function Block Info Notification Universal MIDI Packet
 * @see	ump112: 7.1.8 Function Block Info Notification
 */
static inline struct midi_ump make_function_block_info(const struct ump_endpoint_dt_spec *ep,
						       size_t block_num)
{
	const struct ump_block_dt_spec *block = &ep->blocks[block_num];
	struct midi_ump res;
	uint8_t midi1_mode = block->is_31250bps ? 2 : block->is_midi1 ? 1 : 0;

	res.data[0] = (UMP_MT_UMP_STREAM << 28)
		    | (UMP_STREAM_STATUS_FB_INFO << 16)
		    | BIT(15)  /* Block is active */
		    | (block_num << 8)
		    | BIT_IF(block->is_output, 5) /* UI hint Sender */
		    | BIT_IF(block->is_input, 4)  /* UI hint Receiver */
		    | (midi1_mode << 2)
		    | BIT_IF(block->is_output, 1) /* Function block is output */
		    | BIT_IF(block->is_input, 0); /* Function block is input */

	res.data[1] = (block->first_group << 24)
		    | (block->groups_spanned << 16)
		    | (MIDI_CI_VERSION_FORMAT_UMP_1_1 << 8)  /* MIDI-CI for UMP v1.1 */
		    | 0xff;  /* At most 255 simultaneous Sysex streams */

	return res;
}

/**
 * @brief      Copy an ASCII string into a Universal MIDI Packet while leaving
 *             some most significant bytes untouched, such that the caller can
 *             set this prefix.
 * @param      ump     The ump into which the string is copied
 * @param[in]  offset  Number of bytes from the most-significant side to leave free
 * @param[in]  src     The source string
 * @param[in]  len     The length of the source string
 * @return     The number of bytes copied
 */
static inline size_t fill_str(struct midi_ump *ump, size_t offset,
			      const char *src, size_t len)
{
	size_t i, j;

	if (offset >= sizeof(struct midi_ump)) {
		return 0;
	}

	for (i = 0; i < len && (j = i + offset) < sizeof(struct midi_ump); i++) {
		ump->data[j / 4] |= src[i] << (8 * (3 - (j % 4)));
	}

	return i;
}

/**
 * @brief  Send a string as UMP Stream, possibly splitting into multiple
 *	   packets if the string length is larger than 1 UMP
 * @param[in]  cfg     The responder configuration
 * @param[in]  string  The string to send
 * @param[in]  prefix  The fixed prefix of UMP packets to send
 * @param[in]  offset  The offset the strings starts in the packet, in bytes
 *
 * @return	 The number of packets sent
 */
static inline int send_string(const struct ump_stream_responder_cfg *cfg,
			      const char *string, uint32_t prefix, size_t offset)
{
	struct midi_ump reply;
	size_t stringlen = strlen(string);
	size_t strwidth = sizeof(reply) - offset;
	uint8_t format;
	size_t i = 0;
	int res = 0;

	while (i < stringlen) {
		memset(&reply, 0, sizeof(reply));
		format = (i == 0)
			? (stringlen - i <= strwidth)
				? UMP_STREAM_FORMAT_COMPLETE
				: UMP_STREAM_FORMAT_START
			: (stringlen - i > strwidth)
				? UMP_STREAM_FORMAT_CONTINUE
				: UMP_STREAM_FORMAT_END;

		reply.data[0] = (UMP_MT_UMP_STREAM << 28)
			      | (format << 26)
			      | prefix;

		i += fill_str(&reply, offset, &string[i], stringlen - i);
		cfg->send(cfg->dev, reply);
		res++;
	}

	return res;
}

/**
 * @brief      Handle Endpoint Discovery messages
 * @param[in]  cfg   The responder configuration
 * @param[in]  pkt   The discovery packet to handle
 * @return     The number of UMP sent as reply
 */
static inline int ump_ep_discover(const struct ump_stream_responder_cfg *cfg,
				  const struct midi_ump pkt)
{
	int res = 0;
	uint8_t filter = UMP_STREAM_EP_DISCOVERY_FILTER(pkt);

	/* Request for Endpoint Info Notification */
	if ((filter & UMP_EP_DISC_FILTER_EP_INFO) != 0U) {
		cfg->send(cfg->dev, make_endpoint_info(cfg->ep_spec));
		res++;
	}

	/* Request for Endpoint Name Notification */
	if ((filter & UMP_EP_DISC_FILTER_EP_NAME) != 0U && cfg->ep_spec->name != NULL) {
		res += send_string(cfg, cfg->ep_spec->name,
				   UMP_STREAM_STATUS_EP_NAME << 16, 2);
	}

	/* Request for Product Instance ID */
	if ((filter & UMP_EP_DISC_FILTER_PRODUCT_ID) != 0U && IS_ENABLED(CONFIG_HWINFO)) {
		res += send_string(cfg, ump_product_instance_id(),
				   UMP_STREAM_STATUS_PROD_ID << 16, 2);
	}

	return res;
}

static inline int ump_fb_discover_block(const struct ump_stream_responder_cfg *cfg,
					size_t block_num, uint8_t filter)
{
	int res = 0;
	const struct ump_block_dt_spec *blk = &cfg->ep_spec->blocks[block_num];

	if ((filter & UMP_FB_DISC_FILTER_INFO) != 0U) {
		cfg->send(cfg->dev, make_function_block_info(cfg->ep_spec, block_num));
		res++;
	}

	if ((filter & UMP_FB_DISC_FILTER_NAME) != 0U && blk->name != NULL) {
		res += send_string(cfg, blk->name,
				   (UMP_STREAM_STATUS_FB_NAME << 16) | (block_num << 8), 3);
	}

	return res;
}

/**
 * @brief      Handle Function Block Discovery messages
 * @param[in]  cfg   The responder configuration
 * @param[in]  pkt   The discovery packet to handle
 * @return     The number of UMP sent as reply
 */
static inline int ump_fb_discover(const struct ump_stream_responder_cfg *cfg,
				  const struct midi_ump pkt)
{
	int res = 0;
	uint8_t block_num = UMP_STREAM_FB_DISCOVERY_NUM(pkt);
	uint8_t filter = UMP_STREAM_FB_DISCOVERY_FILTER(pkt);

	if (block_num < cfg->ep_spec->n_blocks) {
		res += ump_fb_discover_block(cfg, block_num, filter);
	} else if (block_num == 0xff) {
		/* Requesting information for all blocks at once */
		for (block_num = 0; block_num < cfg->ep_spec->n_blocks; block_num++) {
			res += ump_fb_discover_block(cfg, block_num, filter);
		}
	}

	return res;
}

const char *ump_product_instance_id(void)
{
	static char product_id[43] = "";
	static const char hex[] = "0123456789ABCDEF";

	if (IS_ENABLED(CONFIG_HWINFO) && product_id[0] == '\0') {
		uint8_t devid[sizeof(product_id) / 2];
		ssize_t len = hwinfo_get_device_id(devid, sizeof(devid));

		if (len == -ENOSYS && hwinfo_get_device_eui64(devid) == 0) {
			/* device id unavailable, but there is an eui64,
			 * which has a fixed length of 8
			 */
			len = 8;
		} else if (len < 0) {
			/* Other hwinfo driver error; mark as empty */
			len = 0;
		}

		/* Convert to hex string */
		for (ssize_t i = 0; i < len; i++) {
			product_id[2 * i] = hex[devid[i] >> 4];
			product_id[2 * i + 1] = hex[devid[i] & 0xf];
		}
		product_id[2*len] = '\0';
	}

	return product_id;
}

int ump_stream_respond(const struct ump_stream_responder_cfg *cfg,
		       const struct midi_ump pkt)
{
	if (cfg->send == NULL) {
		return -EINVAL;
	}

	if (UMP_MT(pkt) != UMP_MT_UMP_STREAM) {
		return 0;
	}

	switch (UMP_STREAM_STATUS(pkt)) {
	case UMP_STREAM_STATUS_EP_DISCOVERY:
		return ump_ep_discover(cfg, pkt);
	case UMP_STREAM_STATUS_FB_DISCOVERY:
		return ump_fb_discover(cfg, pkt);
	}

	return 0;
}
