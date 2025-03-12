/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HDLC_CODER_H_
#define HDLC_CODER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define HDLC_CODER_LENGTH_CRC 2

typedef void (*hdlc_coder_data)(uint8_t, const void *);
typedef void (*hdlc_coder_finish)(bool, const void *);

struct hdlc_coder {
	struct {
		hdlc_coder_data data_handler;
		uint16_t crc;
		bool finish;
	} out;
	struct {
		hdlc_coder_data data_handler;
		hdlc_coder_finish finish_handler;
		uint16_t crc;
		size_t cnt;
		bool escape;
	} inp;
	const void *ctx;
};

void hdlc_coder_init(struct hdlc_coder *coder, const void *ctx);

void hdlc_coder_out_data_set(struct hdlc_coder *coder, hdlc_coder_data handler);
void hdlc_coder_out_finish(struct hdlc_coder *coder, bool data_ok);
void hdlc_coder_out_poll(struct hdlc_coder *coder, uint8_t bt);

void hdlc_coder_inp_data_set(struct hdlc_coder *coder, hdlc_coder_data handler);
void hdlc_coder_inp_finish_set(struct hdlc_coder *coder, hdlc_coder_finish handler);
void hdlc_coder_inp_poll(struct hdlc_coder *coder, uint8_t bt);

#endif /* HDLC_CODER_H_ */
