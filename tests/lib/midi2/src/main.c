/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/audio/midi.h>

#include "ump_stream_responder.h"

/* ---- Mock send infrastructure ---- */

#define MAX_SENT 16

static struct midi_ump sent[MAX_SENT];
static int sent_count;

static void mock_send(const void *dev, const struct midi_ump pkt)
{
	ARG_UNUSED(dev);

	if (sent_count < MAX_SENT) {
		sent[sent_count++] = pkt;
	}
}

/* ---- Test endpoint specs ---- */

/*
 * Wrapper structs to define ump_endpoint_dt_spec instances with a fixed
 * number of blocks.  The memory layout matches ump_endpoint_dt_spec with
 * its flexible array member populated.
 */
struct ep_spec_1blk {
	const char *name;
	size_t n_blocks;
	struct ump_block_dt_spec blocks[1];
};

struct ep_spec_2blk {
	const char *name;
	size_t n_blocks;
	struct ump_block_dt_spec blocks[2];
};

#define EP_SPEC(wrapper) ((const struct ump_endpoint_dt_spec *)&(wrapper))

/* Mixed endpoint: one MIDI2 input + one MIDI1 output */
static const struct ep_spec_2blk mixed_ep = {
	.name = "Test EP",
	.n_blocks = 2,
	.blocks =
		{
			{
				.name = "MIDI2 In",
				.first_group = 0,
				.groups_spanned = 1,
				.is_input = true,
				.is_output = false,
				.is_midi1 = false,
				.is_31250bps = false,
			},
			{
				.name = "MIDI1 Out",
				.first_group = 1,
				.groups_spanned = 2,
				.is_input = false,
				.is_output = true,
				.is_midi1 = true,
				.is_31250bps = false,
			},
		},
};

/* MIDI1-only endpoint */
static const struct ep_spec_1blk midi1_only_ep = {
	.name = "MIDI1 EP",
	.n_blocks = 1,
	.blocks =
		{
			{
				.name = "M1",
				.first_group = 0,
				.groups_spanned = 1,
				.is_input = true,
				.is_output = true,
				.is_midi1 = true,
				.is_31250bps = false,
			},
		},
};

/* MIDI2-only endpoint */
static const struct ep_spec_1blk midi2_only_ep = {
	.name = "MIDI2 EP",
	.n_blocks = 1,
	.blocks =
		{
			{
				.name = "M2",
				.first_group = 0,
				.groups_spanned = 4,
				.is_input = true,
				.is_output = true,
				.is_midi1 = false,
				.is_31250bps = false,
			},
		},
};

/* Endpoint with long name to test multi-packet string splitting.
 * EP name offset=2, so strwidth=14 bytes per packet.
 * "A Very Long Endpoint Name Here" = 30 chars -> 3 packets (14+14+2).
 * FB name offset=3, so strwidth=13 bytes per packet.
 * "Block With A Very Long Name!!" = 29 chars -> 3 packets (13+13+3).
 */
static const struct ep_spec_1blk long_name_ep = {
	.name = "A Very Long Endpoint Name Here",
	.n_blocks = 1,
	.blocks =
		{
			{
				.name = "Block With A Very Long Name!!",
				.first_group = 0,
				.groups_spanned = 1,
				.is_input = true,
				.is_output = false,
				.is_midi1 = false,
				.is_31250bps = false,
			},
		},
};

/* Endpoint with NULL names */
static const struct ep_spec_1blk null_name_ep = {
	.name = NULL,
	.n_blocks = 1,
	.blocks =
		{
			{
				.name = NULL,
				.first_group = 0,
				.groups_spanned = 1,
				.is_input = true,
				.is_output = false,
				.is_midi1 = false,
				.is_31250bps = false,
			},
		},
};

/* Endpoint with 31250bps serial MIDI1 block */
static const struct ep_spec_1blk serial_midi1_ep = {
	.name = "Serial",
	.n_blocks = 1,
	.blocks =
		{
			{
				.name = "DIN",
				.first_group = 0,
				.groups_spanned = 1,
				.is_input = true,
				.is_output = true,
				.is_midi1 = true,
				.is_31250bps = true,
			},
		},
};

/* ---- Helpers ---- */

static struct midi_ump make_ep_discovery(uint8_t filter)
{
	struct midi_ump pkt = {0};

	pkt.data[0] = (UMP_MT_UMP_STREAM << 28) | (UMP_STREAM_STATUS_EP_DISCOVERY << 16);
	pkt.data[1] = filter;
	return pkt;
}

static struct midi_ump make_fb_discovery(uint8_t block_num, uint8_t filter)
{
	struct midi_ump pkt = {0};

	pkt.data[0] = (UMP_MT_UMP_STREAM << 28) | (UMP_STREAM_STATUS_FB_DISCOVERY << 16) |
		      ((uint32_t)block_num << 8) | filter;
	return pkt;
}

static struct ump_stream_responder_cfg make_cfg(const struct ump_endpoint_dt_spec *ep,
						ump_send_func send)
{
	struct ump_stream_responder_cfg cfg = {
		.dev = NULL,
		.send = send,
		.ep_spec = ep,
	};

	return cfg;
}

/* ---- Before hook: reset mock state ---- */

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(sent, 0, sizeof(sent));
	sent_count = 0;
}

/* ---- Error handling tests ---- */

ZTEST(midi2_ump_stream, test_null_send_callback)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(mixed_ep), NULL);
	struct midi_ump pkt = make_ep_discovery(UMP_EP_DISC_FILTER_EP_INFO);

	int ret = ump_stream_respond(&cfg, pkt);

	zassert_equal(ret, -EINVAL, "NULL send should return -EINVAL");
}

ZTEST(midi2_ump_stream, test_ignore_non_stream_mt)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(mixed_ep), mock_send);
	struct midi_ump pkt = {0};

	pkt.data[0] = (UMP_MT_MIDI1_CHANNEL_VOICE << 28);

	int ret = ump_stream_respond(&cfg, pkt);

	zassert_equal(ret, 0, "Non-stream MT should return 0");
	zassert_equal(sent_count, 0, "No packets should be sent");
}

ZTEST(midi2_ump_stream, test_ignore_unknown_status)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(mixed_ep), mock_send);
	struct midi_ump pkt = {0};

	pkt.data[0] = (UMP_MT_UMP_STREAM << 28) | (0xFFU << 16);

	int ret = ump_stream_respond(&cfg, pkt);

	zassert_equal(ret, 0, "Unknown status should return 0");
	zassert_equal(sent_count, 0, "No packets should be sent");
}

/* ---- Endpoint Discovery tests ---- */

ZTEST(midi2_ump_stream, test_ep_discovery_empty_filter)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(mixed_ep), mock_send);
	struct midi_ump pkt = make_ep_discovery(0);

	int ret = ump_stream_respond(&cfg, pkt);

	zassert_equal(ret, 0, "Empty filter should send nothing");
	zassert_equal(sent_count, 0);
}

ZTEST(midi2_ump_stream, test_ep_info_notification)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(mixed_ep), mock_send);
	struct midi_ump pkt = make_ep_discovery(UMP_EP_DISC_FILTER_EP_INFO);

	int ret = ump_stream_respond(&cfg, pkt);

	zassert_equal(ret, 1, "Should send 1 packet");
	zassert_equal(sent_count, 1);

	zassert_equal(UMP_MT(sent[0]), UMP_MT_UMP_STREAM);
	zassert_equal(UMP_STREAM_STATUS(sent[0]), UMP_STREAM_STATUS_EP_INFO);

	/* UMP version 1.1 in low 16 bits */
	zassert_equal(sent[0].data[0] & 0xFFFF, 0x0101, "UMP version should be 1.1");

	/* Static function blocks flag and block count */
	zassert_true((sent[0].data[1] & BIT(31)) != 0, "Static FB flag should be set");
	zassert_equal((sent[0].data[1] >> 24) & 0x7F, 2, "Should report 2 blocks");

	/* Mixed endpoint: both MIDI1 and MIDI2 capability bits set */
	zassert_true((sent[0].data[1] & BIT(9)) != 0, "MIDI2 protocol bit should be set");
	zassert_true((sent[0].data[1] & BIT(8)) != 0, "MIDI1 protocol bit should be set");
}

ZTEST(midi2_ump_stream, test_ep_info_midi1_only)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(midi1_only_ep), mock_send);
	struct midi_ump pkt = make_ep_discovery(UMP_EP_DISC_FILTER_EP_INFO);

	int ret = ump_stream_respond(&cfg, pkt);

	zassert_equal(ret, 1);

	zassert_true((sent[0].data[1] & BIT(8)) != 0, "MIDI1 protocol bit should be set");
	zassert_true((sent[0].data[1] & BIT(9)) == 0, "MIDI2 protocol bit should be clear");
	zassert_equal((sent[0].data[1] >> 24) & 0x7F, 1, "Should report 1 block");
}

ZTEST(midi2_ump_stream, test_ep_info_midi2_only)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(midi2_only_ep), mock_send);
	struct midi_ump pkt = make_ep_discovery(UMP_EP_DISC_FILTER_EP_INFO);

	int ret = ump_stream_respond(&cfg, pkt);

	zassert_equal(ret, 1);

	zassert_true((sent[0].data[1] & BIT(9)) != 0, "MIDI2 protocol bit should be set");
	zassert_true((sent[0].data[1] & BIT(8)) == 0, "MIDI1 protocol bit should be clear");
}

ZTEST(midi2_ump_stream, test_ep_name_short)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(mixed_ep), mock_send);
	struct midi_ump pkt = make_ep_discovery(UMP_EP_DISC_FILTER_EP_NAME);

	int ret = ump_stream_respond(&cfg, pkt);

	/* "Test EP" = 7 chars, fits in 1 packet (limit 14) */
	zassert_equal(ret, 1);
	zassert_equal(sent_count, 1);

	zassert_equal(UMP_MT(sent[0]), UMP_MT_UMP_STREAM);
	zassert_equal(UMP_STREAM_STATUS(sent[0]), UMP_STREAM_STATUS_EP_NAME);
	zassert_equal(UMP_STREAM_FORMAT(sent[0]), UMP_STREAM_FORMAT_COMPLETE,
		      "Short name should use COMPLETE format");
}

ZTEST(midi2_ump_stream, test_ep_name_long)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(long_name_ep), mock_send);
	struct midi_ump pkt = make_ep_discovery(UMP_EP_DISC_FILTER_EP_NAME);

	int ret = ump_stream_respond(&cfg, pkt);

	/* "A Very Long Endpoint Name Here" = 30 chars, 3 packets (14+14+2) */
	zassert_equal(ret, 3, "Long name should split into 3 packets");
	zassert_equal(sent_count, 3);

	/* All packets should be EP Name notifications */
	for (int i = 0; i < 3; i++) {
		zassert_equal(UMP_MT(sent[i]), UMP_MT_UMP_STREAM);
		zassert_equal(UMP_STREAM_STATUS(sent[i]), UMP_STREAM_STATUS_EP_NAME);
	}

	zassert_equal(UMP_STREAM_FORMAT(sent[0]), UMP_STREAM_FORMAT_START);
	zassert_equal(UMP_STREAM_FORMAT(sent[1]), UMP_STREAM_FORMAT_CONTINUE);
	zassert_equal(UMP_STREAM_FORMAT(sent[2]), UMP_STREAM_FORMAT_END);
}

ZTEST(midi2_ump_stream, test_ep_name_null)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(null_name_ep), mock_send);
	struct midi_ump pkt = make_ep_discovery(UMP_EP_DISC_FILTER_EP_NAME);

	int ret = ump_stream_respond(&cfg, pkt);

	zassert_equal(ret, 0, "NULL name should not send any packets");
	zassert_equal(sent_count, 0);
}

ZTEST(midi2_ump_stream, test_product_instance_id)
{
	const char *id = ump_product_instance_id();

	/* Native hwinfo driver returns a 4-byte device ID -> 8 hex chars */
	zassert_true(strlen(id) > 0, "Product ID should not be empty");
	zassert_true(strlen(id) % 2 == 0, "Product ID length should be even");

	/* Verify all characters are uppercase hex */
	for (size_t i = 0; i < strlen(id); i++) {
		bool is_hex = (id[i] >= '0' && id[i] <= '9') ||
			      (id[i] >= 'A' && id[i] <= 'F');

		zassert_true(is_hex, "Product ID char '%c' at %zu is not hex",
			     id[i], i);
	}
}

ZTEST(midi2_ump_stream, test_ep_discovery_product_id)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(mixed_ep), mock_send);
	struct midi_ump pkt = make_ep_discovery(UMP_EP_DISC_FILTER_PRODUCT_ID);

	int ret = ump_stream_respond(&cfg, pkt);

	/* With CONFIG_HWINFO, product ID is sent as a Product Instance ID
	 * notification.  Native hwinfo gives 4 bytes -> 8 hex chars which
	 * fits in a single packet (strwidth=14).
	 */
	zassert_true(ret >= 1, "Should send at least 1 packet");
	zassert_true(sent_count >= 1);

	zassert_equal(UMP_MT(sent[0]), UMP_MT_UMP_STREAM);
	zassert_equal(UMP_STREAM_STATUS(sent[0]), UMP_STREAM_STATUS_PROD_ID);
}

ZTEST(midi2_ump_stream, test_ep_discovery_combined_filter)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(mixed_ep), mock_send);
	uint8_t filter = UMP_EP_DISC_FILTER_EP_INFO | UMP_EP_DISC_FILTER_EP_NAME;
	struct midi_ump pkt = make_ep_discovery(filter);

	int ret = ump_stream_respond(&cfg, pkt);

	/* 1 EP Info + 1 EP Name ("Test EP" fits in 1 packet) */
	zassert_equal(ret, 2, "Combined filter should send 2 packets");
	zassert_equal(sent_count, 2);

	zassert_equal(UMP_STREAM_STATUS(sent[0]), UMP_STREAM_STATUS_EP_INFO);
	zassert_equal(UMP_STREAM_STATUS(sent[1]), UMP_STREAM_STATUS_EP_NAME);
}

/* ---- Function Block Discovery tests ---- */

ZTEST(midi2_ump_stream, test_fb_discovery_empty_filter)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(mixed_ep), mock_send);
	struct midi_ump pkt = make_fb_discovery(0, 0);

	int ret = ump_stream_respond(&cfg, pkt);

	zassert_equal(ret, 0, "Empty filter should send nothing");
	zassert_equal(sent_count, 0);
}

ZTEST(midi2_ump_stream, test_fb_info_notification)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(mixed_ep), mock_send);
	struct midi_ump pkt = make_fb_discovery(0, UMP_FB_DISC_FILTER_INFO);

	int ret = ump_stream_respond(&cfg, pkt);

	zassert_equal(ret, 1);
	zassert_equal(sent_count, 1);

	zassert_equal(UMP_MT(sent[0]), UMP_MT_UMP_STREAM);
	zassert_equal(UMP_STREAM_STATUS(sent[0]), UMP_STREAM_STATUS_FB_INFO);

	/* Block is active */
	zassert_true((sent[0].data[0] & BIT(15)) != 0, "Block should be active");

	/* Block 0: is_input=true, is_output=false, is_midi1=false */
	zassert_true((sent[0].data[0] & BIT(4)) != 0, "Should have input UI hint");
	zassert_true((sent[0].data[0] & BIT(5)) == 0, "Should not have output UI hint");
	zassert_true((sent[0].data[0] & BIT(0)) != 0, "Should be input");
	zassert_true((sent[0].data[0] & BIT(1)) == 0, "Should not be output");

	/* MIDI1 mode = 0 (MIDI2 block) */
	uint8_t midi1_mode = (sent[0].data[0] >> 2) & 0x03;

	zassert_equal(midi1_mode, 0, "MIDI2 block should have midi1_mode=0");

	/* data[1]: first_group=0, groups_spanned=1, MIDI-CI=0x01, max_sysex=0xFF */
	zassert_equal((sent[0].data[1] >> 24) & 0xFF, 0, "first_group should be 0");
	zassert_equal((sent[0].data[1] >> 16) & 0xFF, 1, "groups_spanned should be 1");
	zassert_equal((sent[0].data[1] >> 8) & 0xFF, 0x01, "MIDI-CI version should be 0x01");
	zassert_equal(sent[0].data[1] & 0xFF, 0xFF, "Max sysex streams should be 0xFF");
}

ZTEST(midi2_ump_stream, test_fb_info_block1)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(mixed_ep), mock_send);
	struct midi_ump pkt = make_fb_discovery(1, UMP_FB_DISC_FILTER_INFO);

	int ret = ump_stream_respond(&cfg, pkt);

	zassert_equal(ret, 1);

	/* Block 1: is_input=false, is_output=true, is_midi1=true */
	uint8_t block_num = (sent[0].data[0] >> 8) & 0x7F;

	zassert_equal(block_num, 1, "Should report block 1");

	zassert_true((sent[0].data[0] & BIT(5)) != 0, "Should have output UI hint");
	zassert_true((sent[0].data[0] & BIT(4)) == 0, "Should not have input UI hint");
	zassert_true((sent[0].data[0] & BIT(1)) != 0, "Should be output");
	zassert_true((sent[0].data[0] & BIT(0)) == 0, "Should not be input");

	/* MIDI1 mode = 1 (is_midi1=true, is_31250bps=false) */
	uint8_t midi1_mode = (sent[0].data[0] >> 2) & 0x03;

	zassert_equal(midi1_mode, 1, "MIDI1 (non-serial) block should have midi1_mode=1");

	/* data[1]: first_group=1, groups_spanned=2 */
	zassert_equal((sent[0].data[1] >> 24) & 0xFF, 1, "first_group should be 1");
	zassert_equal((sent[0].data[1] >> 16) & 0xFF, 2, "groups_spanned should be 2");
}

ZTEST(midi2_ump_stream, test_fb_name_notification)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(mixed_ep), mock_send);
	struct midi_ump pkt = make_fb_discovery(0, UMP_FB_DISC_FILTER_NAME);

	int ret = ump_stream_respond(&cfg, pkt);

	/* "MIDI2 In" = 8 chars, fits in 1 packet (limit 13) */
	zassert_equal(ret, 1);
	zassert_equal(sent_count, 1);

	zassert_equal(UMP_MT(sent[0]), UMP_MT_UMP_STREAM);
	zassert_equal(UMP_STREAM_STATUS(sent[0]), UMP_STREAM_STATUS_FB_NAME);
	zassert_equal(UMP_STREAM_FORMAT(sent[0]), UMP_STREAM_FORMAT_COMPLETE);

	/* Block number embedded in the name packet */
	uint8_t block_in_pkt = (sent[0].data[0] >> 8) & 0xFF;

	zassert_equal(block_in_pkt, 0, "Block number should be 0");
}

ZTEST(midi2_ump_stream, test_fb_name_null)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(null_name_ep), mock_send);
	struct midi_ump pkt = make_fb_discovery(0, UMP_FB_DISC_FILTER_NAME);

	int ret = ump_stream_respond(&cfg, pkt);

	zassert_equal(ret, 0, "NULL block name should not send packets");
	zassert_equal(sent_count, 0);
}

ZTEST(midi2_ump_stream, test_fb_name_long)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(long_name_ep), mock_send);
	struct midi_ump pkt = make_fb_discovery(0, UMP_FB_DISC_FILTER_NAME);

	int ret = ump_stream_respond(&cfg, pkt);

	/* "Block With A Very Long Name!!" = 29 chars, 3 packets (13+13+3) */
	zassert_equal(ret, 3, "Long FB name should split into 3 packets");
	zassert_equal(sent_count, 3);

	for (int i = 0; i < 3; i++) {
		zassert_equal(UMP_STREAM_STATUS(sent[i]), UMP_STREAM_STATUS_FB_NAME);
	}

	zassert_equal(UMP_STREAM_FORMAT(sent[0]), UMP_STREAM_FORMAT_START);
	zassert_equal(UMP_STREAM_FORMAT(sent[1]), UMP_STREAM_FORMAT_CONTINUE);
	zassert_equal(UMP_STREAM_FORMAT(sent[2]), UMP_STREAM_FORMAT_END);
}

ZTEST(midi2_ump_stream, test_fb_discovery_combined_filter)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(mixed_ep), mock_send);
	uint8_t filter = UMP_FB_DISC_FILTER_INFO | UMP_FB_DISC_FILTER_NAME;
	struct midi_ump pkt = make_fb_discovery(0, filter);

	int ret = ump_stream_respond(&cfg, pkt);

	/* 1 FB Info + 1 FB Name ("MIDI2 In" fits in 1 packet) */
	zassert_equal(ret, 2);
	zassert_equal(sent_count, 2);

	zassert_equal(UMP_STREAM_STATUS(sent[0]), UMP_STREAM_STATUS_FB_INFO);
	zassert_equal(UMP_STREAM_STATUS(sent[1]), UMP_STREAM_STATUS_FB_NAME);
}

ZTEST(midi2_ump_stream, test_fb_discover_all_blocks)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(mixed_ep), mock_send);
	struct midi_ump pkt = make_fb_discovery(0xFF, UMP_FB_DISC_FILTER_INFO);

	int ret = ump_stream_respond(&cfg, pkt);

	zassert_equal(ret, 2, "Should send info for all 2 blocks");
	zassert_equal(sent_count, 2);

	zassert_equal(UMP_STREAM_STATUS(sent[0]), UMP_STREAM_STATUS_FB_INFO);
	zassert_equal(UMP_STREAM_STATUS(sent[1]), UMP_STREAM_STATUS_FB_INFO);

	uint8_t blk0 = (sent[0].data[0] >> 8) & 0x7F;
	uint8_t blk1 = (sent[1].data[0] >> 8) & 0x7F;

	zassert_equal(blk0, 0, "First should be block 0");
	zassert_equal(blk1, 1, "Second should be block 1");
}

ZTEST(midi2_ump_stream, test_fb_invalid_block)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(mixed_ep), mock_send);
	struct midi_ump pkt = make_fb_discovery(5, UMP_FB_DISC_FILTER_INFO);

	int ret = ump_stream_respond(&cfg, pkt);

	zassert_equal(ret, 0, "Invalid block should return 0");
	zassert_equal(sent_count, 0);
}

ZTEST(midi2_ump_stream, test_fb_midi1_serial_mode)
{
	struct ump_stream_responder_cfg cfg = make_cfg(EP_SPEC(serial_midi1_ep), mock_send);
	struct midi_ump pkt = make_fb_discovery(0, UMP_FB_DISC_FILTER_INFO);

	int ret = ump_stream_respond(&cfg, pkt);

	zassert_equal(ret, 1);

	/* is_31250bps=true -> midi1_mode=2 */
	uint8_t midi1_mode = (sent[0].data[0] >> 2) & 0x03;

	zassert_equal(midi1_mode, 2, "Serial MIDI1 should have midi1_mode=2");

	/* Bidirectional block */
	zassert_true((sent[0].data[0] & BIT(0)) != 0, "Should be input");
	zassert_true((sent[0].data[0] & BIT(1)) != 0, "Should be output");
	zassert_true((sent[0].data[0] & BIT(4)) != 0, "Should have receiver hint");
	zassert_true((sent[0].data[0] & BIT(5)) != 0, "Should have sender hint");
}

ZTEST_SUITE(midi2_ump_stream, NULL, NULL, before_each, NULL, NULL);
