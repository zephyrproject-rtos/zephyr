/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Decoding of SMS-DELIVER PDUs as the SIM7080 lists them with AT+CMGL. The
 * PDU comes from whoever sent the SMS, so whatever its length fields say,
 * decoding must stay within the PDU and within the caller's sim7080_sms.
 */

#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>

#include "sim7080_sms.c"

/* driver state the SMS code refers to, normally defined in sim7080.c */
struct sim7080_data mdata;
struct modem_context mctx;

int modem_cmd_send_ext(struct modem_iface *iface, struct modem_cmd_handler *handler,
		       const struct modem_cmd *handler_cmds, size_t handler_cmds_len,
		       const uint8_t *buf, struct k_sem *sem, k_timeout_t timeout, int flags)
{
	return -ENOTSUP;
}

#define GSM_E_GRAVE	0x04	/* 'è' in the GSM 7 bit default alphabet */
#define UTF8_E_GRAVE	"\xc3\xa8"
#define CANARY		0xa5
#define MAX_UD_LEN	255

/* the PDU in hex, with room for more user data than any valid SMS holds */
static char pdu[2 * (SMS_PDU_MAX_LEN + MAX_UD_LEN) + 1];
static size_t pdu_pos;

/* sms[1] is never written to: it catches writes past sms[0] */
static struct sim7080_sms sms[2];

static void put_octet(uint8_t octet)
{
	pdu_pos += snprintf(&pdu[pdu_pos], sizeof(pdu) - pdu_pos, "%02X", octet);
}

/* SMS-DELIVER up to and including TP-UDL, without SMSC address */
static void put_header(uint8_t first_octet, uint8_t tp_dcs, uint8_t tp_udl)
{
	/* 10 digit international number */
	static const uint8_t oa[] = {0x0a, 0x91, 0x94, 0x21, 0x43, 0x65, 0x87};
	static const uint8_t scts[] = {0x62, 0x11, 0x91, 0x21, 0x43, 0x65, 0x00};

	pdu_pos = 0;
	put_octet(0x00);
	put_octet(first_octet);
	ARRAY_FOR_EACH(oa, i) {
		put_octet(oa[i]);
	}
	put_octet(0x00);
	put_octet(tp_dcs);
	ARRAY_FOR_EACH(scts, i) {
		put_octet(scts[i]);
	}
	put_octet(tp_udl);
}

/* n septets of the same character, packed into octets */
static void put_septets(uint8_t septet, size_t n)
{
	uint32_t acc = 0U;
	unsigned int bits = 0U;

	for (size_t i = 0; i < n; i++) {
		acc |= (uint32_t)septet << bits;
		bits += 7U;
		while (bits >= 8U) {
			put_octet(acc & 0xffU);
			acc >>= 8;
			bits -= 8U;
		}
	}
	if (bits > 0U) {
		put_octet(acc & 0xffU);
	}
}

static void assert_canary_intact(void)
{
	const uint8_t *p = (const uint8_t *)&sms[1];

	for (size_t i = 0; i < sizeof(sms[1]); i++) {
		zassert_equal(p[i], CANARY, "write past sms[0] at offset %zu", i);
	}
}

static void before(void *unused)
{
	ARG_UNUSED(unused);

	memset(&sms[0], 0, sizeof(sms[0]));
	memset(&sms[1], CANARY, sizeof(sms[1]));
	pdu_pos = 0;
}

ZTEST(sim7080_sms, test_7bit)
{
	put_header(0x04, 0x00, 5);
	put_septets('a', 5);

	zassert_ok(mdm_decode_pdu(pdu, pdu_pos, &sms[0]));
	zassert_equal(sms[0].data_len, 5);
	zassert_mem_equal(sms[0].data, "aaaaa", 5);
}

/*
 * 160 characters is the most one SMS carries, and 160 characters taking two
 * bytes in UTF-8 cannot fit in the 160 bytes of data: the decoded text is
 * cut at a character boundary.
 */
ZTEST(sim7080_sms, test_7bit_two_byte_characters)
{
	put_header(0x04, 0x00, 160);
	put_septets(GSM_E_GRAVE, 160);

	zassert_ok(mdm_decode_pdu(pdu, pdu_pos, &sms[0]));
	zassert_equal(sms[0].data_len, 160);
	for (size_t i = 0; i < 80; i++) {
		zassert_mem_equal(&sms[0].data[2 * i], UTF8_E_GRAVE, 2, "character %zu", i);
	}
	assert_canary_intact();
}

/* TP-UDL promises more septets than the PDU holds */
ZTEST(sim7080_sms, test_7bit_short_pdu)
{
	put_header(0x04, 0x00, 160);
	put_septets('a', 10);

	zassert_not_ok(mdm_decode_pdu(pdu, pdu_pos, &sms[0]));
	assert_canary_intact();
}

/* a user data header is not part of the text, nor of its length */
ZTEST(sim7080_sms, test_8bit_with_header)
{
	static const uint8_t udh[] = {0x05, 0x00, 0x03, 0x01, 0x02, 0x01};
	static const uint8_t text[] = {0xde, 0xad, 0xbe, 0xef};

	put_header(0x44, 0x04, sizeof(udh) + sizeof(text));
	ARRAY_FOR_EACH(udh, i) {
		put_octet(udh[i]);
	}
	ARRAY_FOR_EACH(text, i) {
		put_octet(text[i]);
	}

	zassert_ok(mdm_decode_pdu(pdu, pdu_pos, &sms[0]));
	zassert_equal(sms[0].data_len, sizeof(text));
	zassert_mem_equal(sms[0].data, text, sizeof(text));
}

/* TP-UDL beyond what the data buffer holds, even with the octets present */
ZTEST(sim7080_sms, test_8bit_length_too_long)
{
	put_header(0x04, 0x04, MAX_UD_LEN);
	for (size_t i = 0; i < MAX_UD_LEN; i++) {
		put_octet(0x55);
	}

	zassert_not_ok(mdm_decode_pdu(pdu, pdu_pos, &sms[0]));
	assert_canary_intact();
}

/* TP-UDL promises more octets than the PDU holds */
ZTEST(sim7080_sms, test_ucs2_short_pdu)
{
	put_header(0x04, 0x08, 140);
	for (size_t i = 0; i < 10; i++) {
		put_octet(0x55);
	}

	zassert_not_ok(mdm_decode_pdu(pdu, pdu_pos, &sms[0]));
	assert_canary_intact();
}

/* a user data header longer than the user data */
ZTEST(sim7080_sms, test_header_longer_than_data)
{
	put_header(0x44, 0x04, 2);
	put_octet(0x05);
	for (size_t i = 0; i < 5; i++) {
		put_octet(0x00);
	}

	zassert_not_ok(mdm_decode_pdu(pdu, pdu_pos, &sms[0]));
	assert_canary_intact();
}

NET_BUF_POOL_DEFINE(rx_pool, 1, 1024, 0, NULL);

/*
 * Hand on_cmd_cmgl() what the command handler leaves it after parsing
 * "+CMGL: <index>,<stat>,,": the length parameter, then the PDU.
 */
static struct modem_cmd_handler_data *cmgl_input(void)
{
	static struct modem_cmd_handler_data handler_data;
	struct net_buf *buf = net_buf_alloc(&rx_pool, K_NO_WAIT);

	zassert_not_null(buf);
	net_buf_add_mem(buf, "160\r\n", 5);
	net_buf_add_mem(buf, pdu, pdu_pos);
	net_buf_add_mem(buf, "\r\n", 2);
	handler_data.rx_buf = buf;

	return &handler_data;
}

static int cmgl(struct modem_cmd_handler_data *handler_data)
{
	static struct sim7080_sms_buffer buffer = {
		.sms = sms,
		.nsms = 1,
	};
	uint8_t index[] = "3";
	uint8_t stat[] = "1";
	uint8_t *argv[] = {index, stat};
	int ret;

	mdata.sms_buffer = &buffer;
	mdata.sms_buffer_pos = 0;
	ret = on_cmd_cmgl(handler_data, 0, argv, ARRAY_SIZE(argv));
	mdata.sms_buffer = NULL;

	return ret;
}

/* a full 160 character message comes through whole */
ZTEST(sim7080_sms, test_cmgl_full_message)
{
	struct modem_cmd_handler_data *handler_data;

	put_header(0x04, 0x00, 160);
	put_septets('a', 160);
	handler_data = cmgl_input();

	zassert_ok(cmgl(handler_data));
	zassert_equal(mdata.sms_buffer_pos, 1);
	zassert_equal(sms[0].index, 3);
	zassert_equal(sms[0].data_len, 160);
	for (size_t i = 0; i < 160; i++) {
		zassert_equal(sms[0].data[i], 'a', "character %zu", i);
	}
	zassert_equal(sms[0].data[160], '\0');
	assert_canary_intact();

	net_buf_unref(handler_data->rx_buf);
}

/* a PDU longer than any SMS is skipped rather than decoded in part */
ZTEST(sim7080_sms, test_cmgl_pdu_too_long)
{
	struct modem_cmd_handler_data *handler_data;

	put_header(0x04, 0x04, 140);
	for (size_t i = 0; i < SMS_PDU_MAX_LEN; i++) {
		put_octet(0x55);
	}
	handler_data = cmgl_input();

	zassert_ok(cmgl(handler_data));
	zassert_equal(mdata.sms_buffer_pos, 0);
	zassert_equal(handler_data->rx_buf->len, 2, "PDU not consumed");
	assert_canary_intact();

	net_buf_unref(handler_data->rx_buf);
}

ZTEST_SUITE(sim7080_sms, NULL, NULL, before, NULL, NULL);
