/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hdlc_coder.h"
#include <zephyr/sys/crc.h>

#define HDLC_CODER_INIT_CRC  0xffff
#define HDLC_CODER_VALID_CRC 0xf0b8

enum {
	HDLC_CODER_FLAG_XON = 0x11,
	HDLC_CODER_FLAG_XOFF = 0x13,
	HDLC_CODER_ESCAPE_SYMBOL = 0x20,
	HDLC_CODER_FLAG_SEQUENCE = 0x7e,
	HDLC_CODER_ESCAPE_SEQUENCE = 0x7d,
	HDLC_CODER_FLAG_SPECIAL = 0xf8
};

void hdlc_coder_init(struct hdlc_coder *coder, const void *ctx)
{
	coder->out.data_handler = NULL;
	coder->out.crc = HDLC_CODER_INIT_CRC;
	coder->out.finish = true;

	coder->inp.data_handler = NULL;
	coder->inp.finish_handler = NULL;
	coder->inp.crc = HDLC_CODER_INIT_CRC;
	coder->inp.cnt = 0;
	coder->inp.escape = false;

	coder->ctx = ctx;
}

static inline bool hdlc_coder_escape(uint8_t bt)
{
	bool result = false;

	switch (bt) {
	case HDLC_CODER_FLAG_XON:
	case HDLC_CODER_FLAG_XOFF:
	case HDLC_CODER_FLAG_SEQUENCE:
	case HDLC_CODER_ESCAPE_SEQUENCE:
	case HDLC_CODER_FLAG_SPECIAL:
		result = true;
		break;
	}
	return result;
}

void hdlc_coder_out_data_set(struct hdlc_coder *coder, hdlc_coder_data handler)
{
	coder->out.data_handler = handler;
}

void hdlc_coder_out_finish(struct hdlc_coder *coder, bool data_ok)
{
	(void)data_ok;
	if (!coder->out.finish) {
		if (coder->out.data_handler) {
			coder->out.crc ^= 0xffff;
			uint8_t crc_h = (uint8_t)(coder->out.crc >> 8);
			uint8_t crc_l = (uint8_t)coder->out.crc;

			if (hdlc_coder_escape(crc_l)) {
				coder->out.data_handler(HDLC_CODER_ESCAPE_SEQUENCE, coder->ctx);
				coder->out.data_handler(crc_l ^ HDLC_CODER_ESCAPE_SYMBOL,
							coder->ctx);
			} else {
				coder->out.data_handler(crc_l, coder->ctx);
			}
			if (hdlc_coder_escape(crc_h)) {
				coder->out.data_handler(HDLC_CODER_ESCAPE_SEQUENCE, coder->ctx);
				coder->out.data_handler(crc_h ^ HDLC_CODER_ESCAPE_SYMBOL,
							coder->ctx);
			} else {
				coder->out.data_handler(crc_h, coder->ctx);
			}
			coder->out.data_handler(HDLC_CODER_FLAG_SEQUENCE, coder->ctx);
		}
	}
	coder->out.crc = HDLC_CODER_INIT_CRC;
	coder->out.finish = true;
}

void hdlc_coder_out_poll(struct hdlc_coder *coder, uint8_t bt)
{
	if (coder->out.finish) {
		if (coder->out.data_handler) {
			coder->out.data_handler(HDLC_CODER_FLAG_SEQUENCE, coder->ctx);
		}
		coder->out.finish = false;
	}
	if (coder->out.data_handler) {
		if (hdlc_coder_escape(bt)) {
			coder->out.data_handler(HDLC_CODER_ESCAPE_SEQUENCE, coder->ctx);
			coder->out.data_handler(bt ^ HDLC_CODER_ESCAPE_SYMBOL, coder->ctx);
		} else {
			coder->out.data_handler(bt, coder->ctx);
		}
	}
	coder->out.crc = crc16_ccitt(coder->out.crc, &bt, sizeof(bt));
}

void hdlc_coder_inp_data_set(struct hdlc_coder *coder, hdlc_coder_data handler)
{
	coder->inp.data_handler = handler;
}

void hdlc_coder_inp_finish_set(struct hdlc_coder *coder, hdlc_coder_finish handler)
{
	coder->inp.finish_handler = handler;
}

void hdlc_coder_inp_poll(struct hdlc_coder *coder, uint8_t bt)
{
	if (bt == HDLC_CODER_FLAG_SEQUENCE) {
		if (coder->inp.cnt && coder->inp.finish_handler) {
			coder->inp.finish_handler(coder->inp.cnt >= HDLC_CODER_LENGTH_CRC &&
							  coder->inp.crc == HDLC_CODER_VALID_CRC,
						  coder->ctx);
		}
		coder->inp.crc = HDLC_CODER_INIT_CRC;
		coder->inp.escape = false;
		coder->inp.cnt = 0;
	} else {
		if (coder->inp.escape) {
			bt ^= HDLC_CODER_ESCAPE_SYMBOL;
			if (coder->inp.data_handler) {
				coder->inp.data_handler(bt, coder->ctx);
			}
			coder->inp.crc = crc16_ccitt(coder->inp.crc, &bt, sizeof(bt));
			coder->inp.cnt++;
			coder->inp.escape = false;
		} else if (bt == HDLC_CODER_ESCAPE_SEQUENCE) {
			coder->inp.escape = true;
		} else {
			if (coder->inp.data_handler) {
				coder->inp.data_handler(bt, coder->ctx);
			}
			coder->inp.crc = crc16_ccitt(coder->inp.crc, &bt, sizeof(bt));
			coder->inp.cnt++;
		}
	}
}
