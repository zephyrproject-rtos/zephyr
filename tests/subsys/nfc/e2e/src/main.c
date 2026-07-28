/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/nfc/nfc_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/nfc/iso_dep.h>
#include <zephyr/nfc/poller.h>
#include <zephyr/nfc/t2t.h>
#include <zephyr/nfc/tag.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define EMUL     DEVICE_DT_GET(DT_NODELABEL(emul_reader))
#define EMUL_OFF DEVICE_DT_GET(DT_NODELABEL(emul_controller))

NFC_POLLER_DEFINE(poller, EMUL);
NFC_POLLER_DEFINE(offload, EMUL_OFF);

static const uint8_t sens_req[] = {0x26};
static const uint8_t sens_res[] = {0x44, 0x00};
static const uint8_t sdd_req[] = {0x93, 0x20};
/* UID 04 11 22 33, BCC = 04 ^ 11 ^ 22 ^ 33 */
static const uint8_t sdd_res[] = {0x04, 0x11, 0x22, 0x33, 0x04};
static const uint8_t sel_req[] = {0x93, 0x70, 0x04, 0x11, 0x22, 0x33, 0x04};
/* SAK without the ISO-DEP bit: a Type 2 Tag. */
static const uint8_t sel_res[] = {0x00};

#define ACTIVATION_FRAMES                                                                          \
	{.tx = sens_req,                                                                           \
	 .tx_len = sizeof(sens_req),                                                               \
	 .tx_last_bits = 7U,                                                                       \
	 .rx = sens_res,                                                                           \
	 .rx_len = sizeof(sens_res)},                                                              \
		{.tx = sdd_req,                                                                    \
		 .tx_len = sizeof(sdd_req),                                                        \
		 .rx = sdd_res,                                                                    \
		 .rx_len = sizeof(sdd_res)},                                                       \
	{                                                                                          \
		.tx = sel_req, .tx_len = sizeof(sel_req), .rx = sel_res, .rx_len = sizeof(sel_res) \
	}

static const uint8_t read_cc[] = {0x30, 0x03};
/*
 * Capability Container: magic E1, version 1.0, 4 * 8 bytes of data, writable.
 * Two blocks of data area, so reading it is more than one exchange.
 */
static const uint8_t cc_block[16] = {0xE1, 0x10, 0x04, 0x00};

static const uint8_t read_data0[] = {0x30, 0x04};
/* NDEF Message TLV holding one empty record, then the terminator. */
static const uint8_t data_block0[16] = {0x03, 0x03, 0xD0, 0x00, 0x00, 0xFE};
static const uint8_t ndef_record[] = {0xD0, 0x00, 0x00};

static const uint8_t cc_block_ro[16] = {0xE1, 0x10, 0x04, 0x0F};
static const uint8_t write_data0[] = {0xA2, 0x04, 0x03, 0x03, 0xD0, 0x00};
static const uint8_t write_data1[] = {0xA2, 0x05, 0x00, 0xFE, 0x00, 0x00};
static const uint8_t t2t_ack[] = {0x0A};
static const uint8_t t2t_nak[] = {0x00};

static const uint8_t read_data1[] = {0x30, 0x08};
/* An NDEF Message TLV whose 20-byte value runs past the first block. */
static const uint8_t long_block0[16] = {0x03, 0x14, 0xD0, 0x00, 0x11, 0x01, 0x02, 0x03,
					0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B};
static const uint8_t long_block1[16] = {0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0xFE};
static const uint8_t long_record[20] = {0xD0, 0x00, 0x11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
					0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11};

#define CC_FRAME                                                                                   \
	{.tx = read_cc, .tx_len = sizeof(read_cc), .rx = cc_block, .rx_len = sizeof(cc_block)}
#define DATA_FRAMES                                                                                \
	{.tx = read_data0,                                                                         \
	 .tx_len = sizeof(read_data0),                                                             \
	 .rx = data_block0,                                                                        \
	 .rx_len = sizeof(data_block0)}

#define CC_FRAME_RO                                                                                \
	{.tx = read_cc, .tx_len = sizeof(read_cc), .rx = cc_block_ro, .rx_len = sizeof(cc_block_ro)}

#define WRITE_FRAMES                                                                               \
	{.tx = write_data0,                                                                        \
	 .tx_len = sizeof(write_data0),                                                            \
	 .rx = t2t_ack,                                                                            \
	 .rx_len = sizeof(t2t_ack)},                                                               \
	{                                                                                          \
		.tx = write_data1, .tx_len = sizeof(write_data1), .rx = t2t_ack,                   \
		.rx_len = sizeof(t2t_ack)                                                          \
	}

#define LONG_DATA_FRAMES                                                                           \
	{.tx = read_data0,                                                                         \
	 .tx_len = sizeof(read_data0),                                                             \
	 .rx = long_block0,                                                                        \
	 .rx_len = sizeof(long_block0)},                                                           \
	{                                                                                          \
		.tx = read_data1, .tx_len = sizeof(read_data1), .rx = long_block1,                 \
		.rx_len = sizeof(long_block1)                                                      \
	}

static int discover_and_connect(struct nfc_tag *tag)
{
	struct nfc_target target;
	int ret;

	ret = nfc_discover(&poller, K_MSEC(100), &target);
	if (ret < 0) {
		return ret;
	}

	return nfc_tag_connect(&poller, &target, tag, K_MSEC(100));
}

ZTEST(nfc_e2e, test_exchange_needs_an_open_session)
{
	struct nfc_target target;

	zassert_equal(nfc_discover(&poller, K_MSEC(100), &target), -EPERM);
}

ZTEST(nfc_e2e, test_poller_start)
{
	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	zassert_equal(nfc_poller_start(&poller, NFC_PROTO_ISO14443A), -EALREADY);
	zassert_ok(nfc_poller_stop(&poller));

	zassert_equal(nfc_poller_start(&poller, NFC_PROTO_FELICA), -ENOTSUP);
}

ZTEST(nfc_e2e, test_discovery_frame_sequence)
{
	static const struct nfc_emul_frame script[] = {ACTIVATION_FRAMES};
	struct nfc_target target;

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(nfc_discover(&poller, K_MSEC(100), &target));
	zassert_equal(nfc_emul_script_remaining(EMUL), 0);

	zassert_equal(target.tech, NFC_TECH_A);
	zassert_equal(target.a.uid_len, 4U);
	zassert_mem_equal(target.a.uid, sdd_res, 4U);
	zassert_equal(target.a.sak, 0x00U);
}

ZTEST(nfc_e2e, test_discovery_polls_until_the_deadline)
{
	static const struct nfc_emul_frame script[] = {
		{.ret = -ETIMEDOUT}, {.ret = -ETIMEDOUT}, {.ret = -ETIMEDOUT},
		{.ret = -ETIMEDOUT}, {.ret = -ETIMEDOUT}, {.ret = -ETIMEDOUT},
	};
	const int32_t interval = CONFIG_NFC_NFCA_POLL_INTERVAL_MS;
	struct nfc_target target;
	int64_t elapsed;

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	elapsed = k_uptime_get();
	zassert_equal(nfc_discover(&poller, K_MSEC(2 * interval), &target), -EAGAIN);
	elapsed = k_uptime_get() - elapsed;

	zassert_between_inclusive(elapsed, 2 * interval, 4 * interval,
				  "waited %lld ms, polled for less than the deadline", elapsed);

	zassert_true(nfc_emul_script_remaining(EMUL) < ARRAY_SIZE(script) - 1U);
}

/* A truncated SENS_RES, as a card entering the field can produce. */
static const uint8_t sens_res_partial[] = {0x44};

ZTEST(nfc_e2e, test_discovery_retries_a_partial_answer)
{
	static const struct nfc_emul_frame script[] = {
		{.tx = sens_req,
		 .tx_len = sizeof(sens_req),
		 .tx_last_bits = 7U,
		 .rx = sens_res_partial,
		 .rx_len = sizeof(sens_res_partial)},
		ACTIVATION_FRAMES,
	};
	struct nfc_target target;

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(nfc_discover(&poller, K_MSEC(500), &target));
	zassert_equal(nfc_emul_script_remaining(EMUL), 0);
	zassert_mem_equal(target.a.uid, sdd_res, 4U);
}

ZTEST(nfc_e2e, test_discovery_removes_the_field_between_cycles)
{
	static const struct nfc_emul_frame script[] = {
		{.ret = -ETIMEDOUT}, {.ret = -ETIMEDOUT}, {.ret = -ETIMEDOUT},
		{.ret = -ETIMEDOUT}, {.ret = -ETIMEDOUT}, {.ret = -ETIMEDOUT},
	};
	const int32_t interval = CONFIG_NFC_NFCA_POLL_INTERVAL_MS;
	struct nfc_target target;
	uint32_t before;

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	before = nfc_emul_field_cycles(EMUL);
	zassert_equal(nfc_discover(&poller, K_MSEC(2 * interval), &target), -EAGAIN);

	zassert_true(nfc_emul_field_cycles(EMUL) > before,
		     "a poll cycle must power the target down");
}

ZTEST(nfc_e2e, test_discovery_does_not_retry_a_transport_failure)
{
	static const struct nfc_emul_frame script[] = {{.ret = -EIO}, {.ret = -EIO}};
	struct nfc_target target;

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_equal(nfc_discover(&poller, K_SECONDS(10), &target), -EIO);
	zassert_equal(nfc_emul_script_remaining(EMUL), 1);
}

/* SAK bit 3: a MIFARE Classic, which is not an NFC Forum tag type. */
static const uint8_t sel_res_mifare_classic[] = {0x88};

ZTEST(nfc_e2e, test_connect_rejects_a_mifare_classic)
{
	static const struct nfc_emul_frame script[] = {
		{.tx = sens_req,
		 .tx_len = sizeof(sens_req),
		 .tx_last_bits = 7U,
		 .rx = sens_res,
		 .rx_len = sizeof(sens_res)},
		{.tx = sdd_req,
		 .tx_len = sizeof(sdd_req),
		 .rx = sdd_res,
		 .rx_len = sizeof(sdd_res)},
		{.tx = sel_req,
		 .tx_len = sizeof(sel_req),
		 .rx = sel_res_mifare_classic,
		 .rx_len = sizeof(sel_res_mifare_classic)},
	};
	struct nfc_target target;
	struct nfc_tag tag;

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(nfc_discover(&poller, K_MSEC(100), &target));
	zassert_equal(target.a.sak, 0x88U);

	zassert_equal(nfc_tag_connect(&poller, &target, &tag, K_MSEC(100)), -ENOTSUP,
		      "a MIFARE Classic must not be taken for a Type 2 Tag");
	zassert_equal(nfc_emul_script_remaining(EMUL), 0,
		      "detection must not put a command on the air");
}

ZTEST(nfc_e2e, test_release_halts_a_target_that_was_not_connected)
{
	static const uint8_t halt_req[] = {0x50, 0x00};
	static const struct nfc_emul_frame script[] = {
		{.tx = sens_req,
		 .tx_len = sizeof(sens_req),
		 .tx_last_bits = 7U,
		 .rx = sens_res,
		 .rx_len = sizeof(sens_res)},
		{.tx = sdd_req,
		 .tx_len = sizeof(sdd_req),
		 .rx = sdd_res,
		 .rx_len = sizeof(sdd_res)},
		{.tx = sel_req,
		 .tx_len = sizeof(sel_req),
		 .rx = sel_res_mifare_classic,
		 .rx_len = sizeof(sel_res_mifare_classic)},
		{.tx = halt_req, .tx_len = sizeof(halt_req), .ret = -ETIMEDOUT},
	};
	struct nfc_target target;
	struct nfc_tag tag;

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(nfc_discover(&poller, K_MSEC(100), &target));
	zassert_equal(nfc_tag_connect(&poller, &target, &tag, K_MSEC(100)), -ENOTSUP);

	zassert_ok(nfc_target_release(&poller, &target, K_MSEC(100)));
	zassert_equal(nfc_emul_script_remaining(EMUL), 0);
}

ZTEST(nfc_e2e, test_release_needs_an_open_session)
{
	struct nfc_target target = {.tech = NFC_TECH_A, .proto = NFC_PROTO_ISO14443A};

	zassert_equal(nfc_target_release(&poller, &target, K_MSEC(100)), -EPERM);

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	zassert_equal(nfc_target_release(&poller, NULL, K_MSEC(100)), -EINVAL);
}

ZTEST(nfc_e2e, test_connect_reads_capability_container)
{
	static const struct nfc_emul_frame script[] = {ACTIVATION_FRAMES, CC_FRAME};
	struct nfc_tag tag;

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(discover_and_connect(&tag));
	zassert_equal(nfc_emul_script_remaining(EMUL), 0);

	zassert_equal(tag.type, NFC_TAG_TYPE_T2T);
	zassert_equal(nfc_tag_t2t(&tag)->data_size, 32U);
	zassert_true(nfc_tag_t2t(&tag)->writable);
	zassert_is_null(nfc_tag_iso_dep(&tag), "a Type 2 Tag has no ISO-DEP connection");
}

ZTEST(nfc_e2e, test_read_ndef_spanning_blocks)
{
	static const struct nfc_emul_frame script[] = {ACTIVATION_FRAMES, CC_FRAME,
						       LONG_DATA_FRAMES};
	struct nfc_tag tag;
	uint8_t buf[64];
	uint16_t len = sizeof(buf);

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(discover_and_connect(&tag));
	zassert_ok(nfc_tag_read_ndef(&tag, buf, &len, K_MSEC(100)));

	zassert_equal(len, sizeof(long_record));
	zassert_mem_equal(buf, long_record, sizeof(long_record));
	zassert_equal(nfc_emul_script_remaining(EMUL), 0);
}

ZTEST(nfc_e2e, test_read_ndef)
{
	static const struct nfc_emul_frame script[] = {ACTIVATION_FRAMES, CC_FRAME, DATA_FRAMES};
	struct nfc_tag tag;
	uint8_t buf[64];
	uint16_t len = sizeof(buf);

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(discover_and_connect(&tag));
	zassert_ok(nfc_tag_read_ndef(&tag, buf, &len, K_MSEC(100)));
	zassert_equal(nfc_emul_script_remaining(EMUL), 0);

	zassert_equal(len, sizeof(ndef_record));
	zassert_mem_equal(buf, ndef_record, sizeof(ndef_record));
}

ZTEST(nfc_e2e, test_write_ndef)
{
	static const struct nfc_emul_frame script[] = {ACTIVATION_FRAMES, CC_FRAME, WRITE_FRAMES};
	struct nfc_tag tag;

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(discover_and_connect(&tag));
	zassert_ok(nfc_tag_write_ndef(&tag, ndef_record, sizeof(ndef_record), K_MSEC(100)));
	zassert_equal(nfc_emul_script_remaining(EMUL), 0);
}

ZTEST(nfc_e2e, test_write_ndef_needs_an_acknowledgment)
{
	static const struct nfc_emul_frame script[] = {
		ACTIVATION_FRAMES,
		CC_FRAME,
		{.tx = write_data0,
		 .tx_len = sizeof(write_data0),
		 .rx = t2t_ack,
		 .rx_len = sizeof(t2t_ack)},
		{.tx = write_data1,
		 .tx_len = sizeof(write_data1),
		 .rx = t2t_nak,
		 .rx_len = sizeof(t2t_nak)},
	};
	struct nfc_tag tag;

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(discover_and_connect(&tag));
	zassert_equal(nfc_tag_write_ndef(&tag, ndef_record, sizeof(ndef_record), K_MSEC(100)),
		      -EIO);
}

ZTEST(nfc_e2e, test_write_ndef_refuses_a_read_only_tag)
{
	static const struct nfc_emul_frame script[] = {ACTIVATION_FRAMES, CC_FRAME_RO};
	struct nfc_tag tag;

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(discover_and_connect(&tag));
	zassert_equal(nfc_tag_write_ndef(&tag, ndef_record, sizeof(ndef_record), K_MSEC(100)),
		      -EACCES);
	zassert_equal(nfc_emul_script_remaining(EMUL), 0);
}

ZTEST(nfc_e2e, test_write_ndef_refuses_a_message_needing_a_long_tlv)
{
	static const struct nfc_emul_frame script[] = {ACTIVATION_FRAMES, CC_FRAME};
	static const uint8_t oversized[0xFF] = {0};
	struct nfc_tag tag;

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(discover_and_connect(&tag));
	zassert_equal(nfc_tag_write_ndef(&tag, oversized, sizeof(oversized), K_MSEC(100)),
		      -ENOTSUP);
}

ZTEST(nfc_e2e, test_raw_block_access)
{
	static const struct nfc_emul_frame script[] = {ACTIVATION_FRAMES,
						       CC_FRAME,
						       {.tx = read_data0,
							.tx_len = sizeof(read_data0),
							.rx = data_block0,
							.rx_len = sizeof(data_block0)}};
	struct nfc_tag tag;
	uint8_t block[16];

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(discover_and_connect(&tag));
	zassert_ok(nfc_t2t_read_block(nfc_tag_t2t(&tag), 4U, block, K_MSEC(100)));
	zassert_mem_equal(block, data_block0, sizeof(data_block0));
}

ZTEST(nfc_e2e, test_close_halts_a_type_2_tag)
{
	static const uint8_t halt_req[] = {0x50, 0x00};
	static const struct nfc_emul_frame script[] = {
		ACTIVATION_FRAMES,
		CC_FRAME,
		{.tx = halt_req, .tx_len = sizeof(halt_req), .ret = -ETIMEDOUT},
	};
	struct nfc_tag tag;
	uint8_t buf[64];
	uint16_t len = sizeof(buf);

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(discover_and_connect(&tag));
	zassert_ok(nfc_tag_close(&tag, K_MSEC(100)));
	zassert_equal(nfc_emul_script_remaining(EMUL), 0);

	zassert_equal(nfc_tag_read_ndef(&tag, buf, &len, K_MSEC(100)), -EPERM,
		      "a closed tag must not still be readable");
}

#ifdef CONFIG_NFC_T4T

static const uint8_t sel_res_iso_dep[] = {0x20};
static const uint8_t rats_req[] = {0xE0, 0x10};
static const uint8_t ats_res[] = {0x01};

#define ACTIVATION_FRAMES_ISO_DEP                                                                  \
	{.tx = sens_req,                                                                           \
	 .tx_len = sizeof(sens_req),                                                               \
	 .tx_last_bits = 7U,                                                                       \
	 .rx = sens_res,                                                                           \
	 .rx_len = sizeof(sens_res)},                                                              \
		{.tx = sdd_req,                                                                    \
		 .tx_len = sizeof(sdd_req),                                                        \
		 .rx = sdd_res,                                                                    \
		 .rx_len = sizeof(sdd_res)},                                                       \
		{.tx = sel_req,                                                                    \
		 .tx_len = sizeof(sel_req),                                                        \
		 .rx = sel_res_iso_dep,                                                            \
		 .rx_len = sizeof(sel_res_iso_dep)},                                               \
	{                                                                                          \
		.tx = rats_req, .tx_len = sizeof(rats_req), .rx = ats_res,                         \
		.rx_len = sizeof(ats_res)                                                          \
	}

/* I-block carrying SELECT of the NDEF Tag Application by AID. */
static const uint8_t select_app_req[] = {0x02, 0x00, 0xA4, 0x04, 0x00, 0x07, 0xD2,
					 0x76, 0x00, 0x00, 0x85, 0x01, 0x01, 0x00};
/* SW=6A82: the tag has no NDEF application. */
static const uint8_t select_app_not_found[] = {0x02, 0x6A, 0x82};

static const uint8_t select_app_ok[] = {0x02, 0x90, 0x00};
static const uint8_t select_cc_req[] = {0x03, 0x00, 0xA4, 0x00, 0x0C, 0x02, 0xE1, 0x03};
static const uint8_t select_cc_ok[] = {0x03, 0x90, 0x00};
static const uint8_t read_cc_req[] = {0x02, 0x00, 0xB0, 0x00, 0x00, 0x0F};
/* CCLEN=000F ver=20 MLe/MLc=00FB, NDEF File Control TLV: file E104, 255 bytes, writable. */
static const uint8_t read_cc_res[] = {0x02, 0x00, 0x0F, 0x20, 0x00, 0xFB, 0x00, 0xFB, 0x04,
				      0x06, 0xE1, 0x04, 0x00, 0xFF, 0x00, 0x00, 0x90, 0x00};

ZTEST(nfc_e2e, test_connect_reads_type_4_tag)
{
	static const struct nfc_emul_frame script[] = {
		ACTIVATION_FRAMES_ISO_DEP,
		{.tx = select_app_req,
		 .tx_len = sizeof(select_app_req),
		 .rx = select_app_ok,
		 .rx_len = sizeof(select_app_ok)},
		{.tx = select_cc_req,
		 .tx_len = sizeof(select_cc_req),
		 .rx = select_cc_ok,
		 .rx_len = sizeof(select_cc_ok)},
		{.tx = read_cc_req,
		 .tx_len = sizeof(read_cc_req),
		 .rx = read_cc_res,
		 .rx_len = sizeof(read_cc_res)},
	};
	struct nfc_target target;
	struct nfc_tag tag;

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(nfc_discover(&poller, K_MSEC(100), &target));
	zassert_ok(nfc_tag_connect(&poller, &target, &tag, K_MSEC(100)));
	zassert_equal(nfc_emul_script_remaining(EMUL), 0);

	zassert_equal(tag.type, NFC_TAG_TYPE_T4T_A);
	zassert_equal(tag.t4t.ndef_file_id, 0xE104);
	zassert_equal(tag.t4t.max_ndef_len, 0x00FF);
	zassert_true(tag.t4t.writable);
	zassert_not_null(nfc_tag_iso_dep(&tag));
}

ZTEST(nfc_e2e, test_connect_rejects_a_tag_without_ndef)
{
	static const struct nfc_emul_frame script[] = {
		ACTIVATION_FRAMES_ISO_DEP,
		{.tx = select_app_req,
		 .tx_len = sizeof(select_app_req),
		 .rx = select_app_not_found,
		 .rx_len = sizeof(select_app_not_found)},
	};
	struct nfc_target target;
	struct nfc_tag tag;

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(nfc_discover(&poller, K_MSEC(100), &target));
	zassert_equal(target.a.sak, 0x20U);

	zassert_equal(nfc_tag_connect(&poller, &target, &tag, K_MSEC(100)), -ENOTSUP,
		      "a tag that refuses the NDEF application must not connect");
	zassert_equal(nfc_emul_script_remaining(EMUL), 0);
}

#endif /* CONFIG_NFC_T4T */

/*
 * Reading NDEF is several frames. The emulated controller yields inside every
 * exchange, so a second thread at the same priority gets to run in the middle of
 * the sequence; without the session lock its SENS_REQ would be matched against
 * the script's next Type 2 command and both callers would fail.
 */
static K_THREAD_STACK_DEFINE(intruder_stack, 2048);
static struct k_thread intruder;
static int intruder_ret;

static void intruder_fn(void *a, void *b, void *c)
{
	struct nfc_target target;

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	intruder_ret = nfc_discover(&poller, K_MSEC(100), &target);
}

ZTEST(nfc_e2e, test_session_is_not_interleaved)
{
	static const struct nfc_emul_frame script[] = {ACTIVATION_FRAMES, CC_FRAME, DATA_FRAMES};
	struct nfc_tag tag;
	uint8_t buf[64];
	uint16_t len = sizeof(buf);

	zassert_ok(nfc_poller_start(&poller, NFC_PROTO_ISO14443A));
	nfc_emul_load_script(EMUL, script, ARRAY_SIZE(script));

	zassert_ok(discover_and_connect(&tag));

	intruder_ret = INT_MIN;
	k_thread_create(&intruder, intruder_stack, K_THREAD_STACK_SIZEOF(intruder_stack),
			intruder_fn, NULL, NULL, NULL, k_thread_priority_get(k_current_get()), 0,
			K_NO_WAIT);

	zassert_ok(nfc_tag_read_ndef(&tag, buf, &len, K_MSEC(100)));
	zassert_equal(len, sizeof(ndef_record));
	zassert_mem_equal(buf, ndef_record, sizeof(ndef_record));

	zassert_ok(k_thread_join(&intruder, K_MSEC(500)));

	zassert_equal(intruder_ret, -ENODATA, "intruder saw %d, expected the script to be spent",
		      intruder_ret);
}

/*
 * A controller that resolves targets in firmware never sees the activation
 * frames, and it frames the Type 2 commands itself, so only the tag commands
 * appear in the script.
 */
static const struct nfc_target staged_t2t = {
	.tech = NFC_TECH_A,
	.proto = NFC_PROTO_ISO14443A,
	.a = {.uid = {0x04, 0x11, 0x22, 0x33}, .uid_len = 4U, .sak = 0x00U},
};

ZTEST(nfc_e2e, test_offload_reads_type_2_tag)
{
	static const struct nfc_emul_frame script[] = {CC_FRAME, DATA_FRAMES};
	struct nfc_target target;
	struct nfc_tag tag;
	uint8_t buf[64];
	uint16_t len = sizeof(buf);

	zassert_ok(nfc_poller_start(&offload, NFC_PROTO_ISO14443A));
	zassert_ok(nfc_emul_set_target(EMUL_OFF, &staged_t2t));
	nfc_emul_load_script(EMUL_OFF, script, ARRAY_SIZE(script));

	zassert_ok(nfc_discover(&offload, K_MSEC(100), &target));
	zassert_equal(target.a.sak, 0x00U);

	zassert_ok(nfc_tag_connect(&offload, &target, &tag, K_MSEC(100)));
	zassert_equal(tag.type, NFC_TAG_TYPE_T2T);

	zassert_ok(nfc_tag_read_ndef(&tag, buf, &len, K_MSEC(100)));
	zassert_equal(nfc_emul_script_remaining(EMUL_OFF), 0);
	zassert_equal(len, sizeof(ndef_record));
	zassert_mem_equal(buf, ndef_record, sizeof(ndef_record));
}

ZTEST(nfc_e2e, test_offload_closes_a_type_2_tag)
{
	static const struct nfc_emul_frame script[] = {CC_FRAME};
	struct nfc_target target;
	struct nfc_tag tag;

	zassert_ok(nfc_poller_start(&offload, NFC_PROTO_ISO14443A));
	zassert_ok(nfc_emul_set_target(EMUL_OFF, &staged_t2t));
	nfc_emul_load_script(EMUL_OFF, script, ARRAY_SIZE(script));

	zassert_ok(nfc_discover(&offload, K_MSEC(100), &target));
	zassert_ok(nfc_tag_connect(&offload, &target, &tag, K_MSEC(100)));
	zassert_false(nfc_emul_target_released(EMUL_OFF));

	zassert_ok(nfc_tag_close(&tag, K_MSEC(100)));
	zassert_true(nfc_emul_target_released(EMUL_OFF));
	zassert_equal(nfc_emul_script_remaining(EMUL_OFF), 0);
}

ZTEST(nfc_e2e, test_offload_releases_a_target_it_did_not_connect)
{
	struct nfc_target target;

	zassert_ok(nfc_poller_start(&offload, NFC_PROTO_ISO14443A));
	zassert_ok(nfc_emul_set_target(EMUL_OFF, &staged_t2t));

	zassert_ok(nfc_discover(&offload, K_MSEC(100), &target));
	zassert_false(nfc_emul_target_released(EMUL_OFF));

	zassert_ok(nfc_target_release(&offload, &target, K_MSEC(100)));
	zassert_true(nfc_emul_target_released(EMUL_OFF));
}

ZTEST(nfc_e2e, test_offload_refuses_iso_dep_without_an_ats)
{
	struct nfc_target target = staged_t2t;
	struct nfc_iso_dep_tag iso_dep;
	struct nfc_tag tag;

	zassert_ok(nfc_poller_start(&offload, NFC_PROTO_ISO14443A));

	zassert_equal(nfc_iso_dep_connect(&offload, &target, NULL, &iso_dep), -ENOTSUP);

	target.a.sak = 0x20U;
	zassert_equal(nfc_tag_connect(&offload, &target, &tag, K_MSEC(100)), -ENOTSUP);
}

/*
 * An offloading controller consumes the Type 2 acknowledgment itself and
 * reports the outcome in its own status, so no ACK reaches the subsystem.
 */
ZTEST(nfc_e2e, test_offload_writes_type_2_tag)
{
	static const struct nfc_emul_frame script[] = {
		CC_FRAME,
		{.tx = write_data0, .tx_len = sizeof(write_data0)},
		{.tx = write_data1, .tx_len = sizeof(write_data1)},
	};
	struct nfc_target target;
	struct nfc_tag tag;

	zassert_ok(nfc_poller_start(&offload, NFC_PROTO_ISO14443A));
	zassert_ok(nfc_emul_set_target(EMUL_OFF, &staged_t2t));
	nfc_emul_load_script(EMUL_OFF, script, ARRAY_SIZE(script));

	zassert_ok(nfc_discover(&offload, K_MSEC(100), &target));
	zassert_ok(nfc_tag_connect(&offload, &target, &tag, K_MSEC(100)));

	zassert_ok(nfc_tag_write_ndef(&tag, ndef_record, sizeof(ndef_record), K_MSEC(100)));
	zassert_equal(nfc_emul_script_remaining(EMUL_OFF), 0);
}

#ifdef CONFIG_NFC_T4T

/* The controller ran RATS itself, so activation reaches the subsystem as an ATS. */
static const struct nfc_target staged_t4t = {
	.tech = NFC_TECH_A,
	.proto = NFC_PROTO_ISO14443A,
	.a = {.uid = {0x95, 0xF4, 0xA9, 0xDB},
	      .uid_len = 4U,
	      .sak = 0x20U,
	      .ats = {0x05, 0x78, 0xF7, 0xB1, 0x02},
	      .ats_len = 5U},
};

static const uint8_t off_select_app_req[] = {0x00, 0xA4, 0x04, 0x00, 0x07, 0xD2, 0x76,
					     0x00, 0x00, 0x85, 0x01, 0x01, 0x00};
static const uint8_t off_sw_ok[] = {0x90, 0x00};
static const uint8_t off_select_cc_req[] = {0x00, 0xA4, 0x00, 0x0C, 0x02, 0xE1, 0x03};
static const uint8_t off_read_cc_req[] = {0x00, 0xB0, 0x00, 0x00, 0x0F};
static const uint8_t off_read_cc_res[] = {0x00, 0x0F, 0x20, 0x00, 0xFB, 0x00, 0xFB, 0x04, 0x06,
					  0xE1, 0x04, 0x00, 0xFF, 0x00, 0x00, 0x90, 0x00};

static const uint8_t off_select_ndef_req[] = {0x00, 0xA4, 0x00, 0x0C, 0x02, 0xE1, 0x04};
static const uint8_t off_nlen_zero[] = {0x00, 0xD6, 0x00, 0x00, 0x02, 0x00, 0x00};
static const uint8_t off_write_value[] = {0x00, 0xD6, 0x00, 0x02, 0x03, 0xD0, 0x00, 0x00};
static const uint8_t off_nlen_three[] = {0x00, 0xD6, 0x00, 0x00, 0x02, 0x00, 0x03};

/* T4T_READ_CHUNK is 0x3B, so a 60-byte message needs a second UPDATE BINARY. */
static const uint8_t long_ndef[60] = {0xD0, 0x00, 0x3C};
static const uint8_t off_write_chunk0[5 + 0x3B] = {0x00, 0xD6, 0x00, 0x02, 0x3B, 0xD0, 0x00, 0x3C};
static const uint8_t off_write_chunk1[] = {0x00, 0xD6, 0x00, 0x3D, 0x01, 0x00};
static const uint8_t off_nlen_sixty[] = {0x00, 0xD6, 0x00, 0x00, 0x02, 0x00, 0x3C};

#define OFF_CONNECT_FRAMES                                                                         \
	{.tx = off_select_app_req,                                                                 \
	 .tx_len = sizeof(off_select_app_req),                                                     \
	 .rx = off_sw_ok,                                                                          \
	 .rx_len = sizeof(off_sw_ok)},                                                             \
		{.tx = off_select_cc_req,                                                          \
		 .tx_len = sizeof(off_select_cc_req),                                              \
		 .rx = off_sw_ok,                                                                  \
		 .rx_len = sizeof(off_sw_ok)},                                                     \
	{                                                                                          \
		.tx = off_read_cc_req, .tx_len = sizeof(off_read_cc_req), .rx = off_read_cc_res,   \
		.rx_len = sizeof(off_read_cc_res)                                                  \
	}

#define OFF_SW_OK_FRAME(_tx)                                                                       \
	{.tx = (_tx), .tx_len = sizeof(_tx), .rx = off_sw_ok, .rx_len = sizeof(off_sw_ok)}

ZTEST(nfc_e2e, test_offload_connects_type_4_tag_from_the_reported_ats)
{
	static const struct nfc_emul_frame script[] = {
		{.tx = off_select_app_req,
		 .tx_len = sizeof(off_select_app_req),
		 .rx = off_sw_ok,
		 .rx_len = sizeof(off_sw_ok)},
		{.tx = off_select_cc_req,
		 .tx_len = sizeof(off_select_cc_req),
		 .rx = off_sw_ok,
		 .rx_len = sizeof(off_sw_ok)},
		{.tx = off_read_cc_req,
		 .tx_len = sizeof(off_read_cc_req),
		 .rx = off_read_cc_res,
		 .rx_len = sizeof(off_read_cc_res)},
	};
	struct nfc_target target;
	struct nfc_tag tag;

	zassert_ok(nfc_poller_start(&offload, NFC_PROTO_ISO14443A));
	zassert_ok(nfc_emul_set_target(EMUL_OFF, &staged_t4t));
	nfc_emul_load_script(EMUL_OFF, script, ARRAY_SIZE(script));

	zassert_ok(nfc_discover(&offload, K_MSEC(100), &target));
	zassert_equal(target.a.ats_len, 5U);

	zassert_ok(nfc_tag_connect(&offload, &target, &tag, K_MSEC(100)));
	zassert_equal(tag.type, NFC_TAG_TYPE_T4T_A);
	zassert_equal(tag.t4t.iso_dep.fsci, 8U);
	zassert_equal(tag.t4t.iso_dep.fwi, 11U);
	zassert_true(tag.t4t.iso_dep.cid_supported);
	zassert_false(tag.t4t.iso_dep.nad_supported);
	zassert_equal(nfc_emul_script_remaining(EMUL_OFF), 0);

	zassert_ok(nfc_tag_close(&tag, K_MSEC(100)));
	zassert_true(nfc_emul_target_released(EMUL_OFF));
}

ZTEST(nfc_e2e, test_offload_writes_type_4_tag)
{
	static const struct nfc_emul_frame script[] = {
		OFF_CONNECT_FRAMES,
		OFF_SW_OK_FRAME(off_select_ndef_req),
		OFF_SW_OK_FRAME(off_nlen_zero),
		OFF_SW_OK_FRAME(off_write_value),
		OFF_SW_OK_FRAME(off_nlen_three),
	};
	struct nfc_target target;
	struct nfc_tag tag;

	zassert_ok(nfc_poller_start(&offload, NFC_PROTO_ISO14443A));
	zassert_ok(nfc_emul_set_target(EMUL_OFF, &staged_t4t));
	nfc_emul_load_script(EMUL_OFF, script, ARRAY_SIZE(script));

	zassert_ok(nfc_discover(&offload, K_MSEC(100), &target));
	zassert_ok(nfc_tag_connect(&offload, &target, &tag, K_MSEC(100)));

	zassert_ok(nfc_tag_write_ndef(&tag, ndef_record, sizeof(ndef_record), K_MSEC(100)));
	zassert_equal(nfc_emul_script_remaining(EMUL_OFF), 0);
}

ZTEST(nfc_e2e, test_offload_writes_type_4_tag_in_chunks)
{
	static const struct nfc_emul_frame script[] = {
		OFF_CONNECT_FRAMES,
		OFF_SW_OK_FRAME(off_select_ndef_req),
		OFF_SW_OK_FRAME(off_nlen_zero),
		OFF_SW_OK_FRAME(off_write_chunk0),
		OFF_SW_OK_FRAME(off_write_chunk1),
		OFF_SW_OK_FRAME(off_nlen_sixty),
	};
	struct nfc_target target;
	struct nfc_tag tag;

	zassert_ok(nfc_poller_start(&offload, NFC_PROTO_ISO14443A));
	zassert_ok(nfc_emul_set_target(EMUL_OFF, &staged_t4t));
	nfc_emul_load_script(EMUL_OFF, script, ARRAY_SIZE(script));

	zassert_ok(nfc_discover(&offload, K_MSEC(100), &target));
	zassert_ok(nfc_tag_connect(&offload, &target, &tag, K_MSEC(100)));

	zassert_ok(nfc_tag_write_ndef(&tag, long_ndef, sizeof(long_ndef), K_MSEC(100)));
	zassert_equal(nfc_emul_script_remaining(EMUL_OFF), 0);
}

#endif /* CONFIG_NFC_T4T */

static void nfc_e2e_before(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)nfc_poller_stop(&poller);
	(void)nfc_poller_stop(&offload);
	nfc_emul_load_script(EMUL, NULL, 0);
	nfc_emul_load_script(EMUL_OFF, NULL, 0);
	(void)nfc_offload_poll_stop(EMUL_OFF);
	(void)nfc_emul_set_target(EMUL_OFF, NULL);
}

ZTEST_SUITE(nfc_e2e, NULL, NULL, nfc_e2e_before, NULL, NULL);
