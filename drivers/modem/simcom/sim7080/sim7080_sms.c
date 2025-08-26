/*
 * Copyright (C) 2025 Metratec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/net/offloaded_netdev.h>
LOG_MODULE_REGISTER(modem_sim7080_sms, CONFIG_MODEM_LOG_LEVEL);

#include "sim7080.h"

#define SMS_TP_UDHI_HEADER 0x40

/**
 * Decode readable hex to "real" hex.
 */
static uint8_t mdm_pdu_decode_ascii(char byte)
{
	if ((byte >= '0') && (byte <= '9')) {
		return byte - '0';
	} else if ((byte >= 'A') && (byte <= 'F')) {
		return byte - 'A' + 10;
	} else if ((byte >= 'a') && (byte <= 'f')) {
		return byte - 'a' + 10;
	} else {
		return 255;
	}
}

/**
 * Reads "byte" from pdu.
 *
 * @param pdu pdu to read from.
 * @param index index of "byte".
 *
 * Sim module "encodes" one pdu byte as two human readable bytes
 * this functions squashes these two bytes into one.
 */
static uint8_t mdm_pdu_read_byte(const char *pdu, size_t index)
{
	return (mdm_pdu_decode_ascii(pdu[index * 2]) << 4 |
		mdm_pdu_decode_ascii(pdu[index * 2 + 1]));
}

/**
 * Decodes time from pdu.
 *
 * @param pdu pdu to read from.
 * @param index index of "byte".
 */
static uint8_t mdm_pdu_read_time(const char *pdu, size_t index)
{
	return (mdm_pdu_decode_ascii(pdu[index * 2]) +
		mdm_pdu_decode_ascii(pdu[index * 2 + 1]) * 10);
}

/**
 * Decode a sms from pdu mode.
 */
static int mdm_decode_pdu(const char *pdu, size_t pdu_len, struct sim7080_sms *target_buf)
{
	size_t index;

	/*
	 * GSM_03.38 to Unicode conversion table
	 */
	const short enc7_basic[128] = {
		'@',	0xA3,	'$',	0xA5,	0xE8,	0xE9,	0xF9,	0xEC,	0xF2,	0xE7,
		'\n',	0xD8,	0xF8,	'\r',	0xC5,	0xF8,	0x0394, '_',	0x03A6, 0x0393,
		0x039B, 0x03A9, 0x03A0, 0x03A8, 0x03A3, 0x0398, 0x039E, '\x1b', 0xC6,	0xE6,
		0xDF,	0xC9,	' ',	'!',	'\"',	'#',	0xA4,	'%',	'&',	'\'',
		'(',	')',	'*',	'+',	',',	'-',	'.',	'/',	'0',	'1',
		'2',	'3',	'4',	'5',	'6',	'7',	'8',	'9',	':',	';',
		'<',	'=',	'>',	'?',	0xA1,	'A',	'B',	'C',	'D',	'E',
		'F',	'G',	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
		'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',	'X',	'Y',
		'Z',	0xC4,	0xD6,	0xD1,	0xDC,	0xA7,	0xBF,	'a',	'b',	'c',
		'd',	'e',	'f',	'g',	'h',	'i',	'j',	'k',	'l',	'm',
		'n',	'o',	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',
		'x',	'y',	'z',	0xE4,	0xF6,	0xF1,	0xFC,	0xE0
	};

	/* two bytes in pdu are on real byte */
	pdu_len = (pdu_len / 2);

	/* first byte of pdu is length of trailing SMSC information
	 * skip it by setting index to SMSC length + 1.
	 */
	index = mdm_pdu_read_byte(pdu, 0) + 1;

	if (index >= pdu_len) {
		return -1;
	}

	/* read first octet */
	target_buf->first_octet = mdm_pdu_read_byte(pdu, index++);

	if (index >= pdu_len) {
		return -1;
	}

	/* pdu_index now points to the address field.
	 * first byte of addr field is the addr length -> skip it.
	 * address type is not included in addr len -> add +1.
	 * address is coded in semi octets
	 *  + addr_len/2 if even
	 *  + addr_len/2 + 1 if odd
	 */
	uint8_t addr_len = mdm_pdu_read_byte(pdu, index);

	index += ((addr_len % 2) == 0) ? (addr_len / 2) + 2 : (addr_len / 2) + 3;

	if (index >= pdu_len) {
		return -1;
	}

	/* read protocol identifier */
	target_buf->tp_pid = mdm_pdu_read_byte(pdu, index++);

	if (index >= pdu_len) {
		return -1;
	}

	/* read coding scheme */
	uint8_t tp_dcs = mdm_pdu_read_byte(pdu, index++);

	/* parse date and time */
	if ((index + 7) >= pdu_len) {
		return -1;
	}

	target_buf->time.year = mdm_pdu_read_time(pdu, index++);
	target_buf->time.month = mdm_pdu_read_time(pdu, index++);
	target_buf->time.day = mdm_pdu_read_time(pdu, index++);
	target_buf->time.hour = mdm_pdu_read_time(pdu, index++);
	target_buf->time.minute = mdm_pdu_read_time(pdu, index++);
	target_buf->time.second = mdm_pdu_read_time(pdu, index++);
	target_buf->time.timezone = mdm_pdu_read_time(pdu, index++);

	/* Read user data length */
	uint8_t tp_udl = mdm_pdu_read_byte(pdu, index++);

	/* Discard header */
	uint8_t header_skip = 0;

	if (target_buf->first_octet & SMS_TP_UDHI_HEADER) {
		uint8_t tp_udhl = mdm_pdu_read_byte(pdu, index);

		index += tp_udhl + 1;
		header_skip = tp_udhl + 1;

		if (index >= pdu_len) {
			return -1;
		}
	}

	/* Read data according to type set in TP-DCS */
	if (tp_dcs == 0x00) {
		/* 7 bit GSM coding */
		uint8_t fill_level = 0;
		uint16_t buf = 0;

		if (target_buf->first_octet & SMS_TP_UDHI_HEADER) {
			/* Initial fill because septets are aligned to
			 * septet boundary after header
			 */
			uint8_t fill_bits = 7 - ((header_skip * 8) % 7);

			if (fill_bits == 7) {
				fill_bits = 0;
			}

			buf = mdm_pdu_read_byte(pdu, index++);

			fill_level = 8 - fill_bits;
		}

		uint16_t data_index = 0;

		for (unsigned int idx = 0; idx < tp_udl; idx++) {
			if (fill_level < 7) {
				uint8_t octet = mdm_pdu_read_byte(pdu, index++);

				buf &= ((1 << fill_level) - 1);
				buf |= (octet << fill_level);
				fill_level += 8;
			}

			/*
			 * Convert 7-bit encoded data to Unicode and
			 * then to UTF-8
			 */
			short letter = enc7_basic[buf & 0x007f];

			if (letter < 0x0080) {
				target_buf->data[data_index++] = letter & 0x007f;
			} else if (letter < 0x0800) {
				target_buf->data[data_index++] = 0xc0 | ((letter & 0x07c0) >> 6);
				target_buf->data[data_index++] = 0x80 | ((letter & 0x003f) >> 0);
			}
			buf >>= 7;
			fill_level -= 7;
		}
		target_buf->data_len = data_index;
	} else if (tp_dcs == 0x04) {
		/* 8 bit binary coding */
		for (int idx = 0; idx < tp_udl - header_skip; idx++) {
			target_buf->data[idx] = mdm_pdu_read_byte(pdu, index++);
		}
		target_buf->data_len = tp_udl;
	} else if (tp_dcs == 0x08) {
		/* Unicode (16 bit per character) */
		for (int idx = 0; idx < tp_udl - header_skip; idx++) {
			target_buf->data[idx] = mdm_pdu_read_byte(pdu, index++);
		}
		target_buf->data_len = tp_udl;
	} else {
		return -1;
	}

	return 0;
}

/**
 * Check if given char sequence is crlf.
 *
 * @param c The char sequence.
 * @param len Total length of the fragment.
 * @return @c true if char sequence is crlf.
 *         Otherwise @c false is returned.
 */
static bool is_crlf(uint8_t *c, uint8_t len)
{
	/* crlf does not fit. */
	if (len < 2) {
		return false;
	}

	return c[0] == '\r' && c[1] == '\n';
}

/**
 * Find terminating crlf in a netbuffer.
 *
 * @param buf The netbuffer.
 * @param skip Bytes to skip before search.
 * @return Length of the returned fragment or 0 if not found.
 */
static size_t net_buf_find_crlf(struct net_buf *buf, size_t skip)
{
	size_t len = 0, pos = 0;
	struct net_buf *frag = buf;

	/* Skip to the start. */
	while (frag && skip >= frag->len) {
		skip -= frag->len;
		frag = frag->frags;
	}

	/* Need to wait for more data. */
	if (!frag) {
		return 0;
	}

	pos = skip;

	while (frag && !is_crlf(frag->data + pos, frag->len - pos)) {
		if (pos + 1 >= frag->len) {
			len += frag->len;
			frag = frag->frags;
			pos = 0U;
		} else {
			pos++;
		}
	}

	if (frag && is_crlf(frag->data + pos, frag->len - pos)) {
		len += pos;
		return len - skip;
	}

	return 0;
}

/**
 * Parses list sms and add them to buffer.
 * Format is:
 *
 * +CMGL: <index>,<stat>,,<length><CR><LF><pdu><CR><LF>
 * +CMGL: <index>,<stat>,,<length><CR><LF><pdu><CR><LF>
 * ...
 * OK
 */
MODEM_CMD_DEFINE(on_cmd_cmgl)
{
	int sms_index, sms_stat, ret;
	char pdu_buffer[256];
	size_t out_len, sms_len, param_len;
	struct sim7080_sms *sms;

	sms_index = atoi(argv[0]);
	sms_stat = atoi(argv[1]);

	/* Get the length of the "length" parameter.
	 * The last parameter will be stuck in the netbuffer.
	 * It is not the actual length of the trailing pdu so
	 * we have to search the next crlf.
	 */
	param_len = net_buf_find_crlf(data->rx_buf, 0);
	if (param_len == 0) {
		LOG_INF("No <CR><LF>");
		return -EAGAIN;
	}

	/* Get actual trailing pdu len. +2 to skip crlf. */
	sms_len = net_buf_find_crlf(data->rx_buf, param_len + 2);
	if (sms_len == 0) {
		return -EAGAIN;
	}

	/* Skip to start of pdu. */
	data->rx_buf = net_buf_skip(data->rx_buf, param_len + 2);

	out_len = net_buf_linearize(pdu_buffer, sizeof(pdu_buffer) - 1, data->rx_buf, 0, sms_len);
	pdu_buffer[out_len] = '\0';

	data->rx_buf = net_buf_skip(data->rx_buf, sms_len);

	/* No buffer specified. */
	if (!mdata.sms_buffer) {
		return 0;
	}

	/* No space left in buffer. */
	if (mdata.sms_buffer_pos >= mdata.sms_buffer->nsms) {
		return 0;
	}

	sms = &mdata.sms_buffer->sms[mdata.sms_buffer_pos];

	ret = mdm_decode_pdu(pdu_buffer, out_len, sms);
	if (ret < 0) {
		return 0;
	}

	sms->stat = sms_stat;
	sms->index = sms_index;
	sms->data[sms->data_len] = '\0';

	mdata.sms_buffer_pos++;

	return 0;
}

int mdm_sim7080_read_sms(struct sim7080_sms_buffer *buffer)
{
	int ret;
	struct modem_cmd cmds[] = { MODEM_CMD("+CMGL: ", on_cmd_cmgl, 4U, ",\r") };

	mdata.sms_buffer = buffer;
	mdata.sms_buffer_pos = 0;

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmds, ARRAY_SIZE(cmds), "AT+CMGL=4",
				 &mdata.sem_response, K_SECONDS(20));
	if (ret < 0) {
		return -1;
	}

	return mdata.sms_buffer_pos;
}

int mdm_sim7080_delete_sms(uint16_t index)
{
	int ret;
	char buf[sizeof("AT+CMGD=#####")] = { 0 };

	ret = snprintk(buf, sizeof(buf), "AT+CMGD=%u", index);
	if (ret < 0) {
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, buf, &mdata.sem_response,
				 K_SECONDS(5));
	if (ret < 0) {
		return -1;
	}

	return 0;
}
