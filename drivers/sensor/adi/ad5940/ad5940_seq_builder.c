/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ad5940.h"

#include <errno.h>
#include <zephyr/drivers/sensor/ad5940.h>

void ad5940_seq_builder_init(struct ad5940_seq_builder *b, uint32_t *buf, uint16_t cap)
{
	b->buf = buf;
	b->cap = cap;
	b->len = 0u;
	b->err = 0;
}

uint16_t ad5940_seq_builder_len(const struct ad5940_seq_builder *b)
{
	return b->len;
}

int ad5940_seq_builder_error(const struct ad5940_seq_builder *b)
{
	return b->err;
}

static void ad5940_seq_builder_append(struct ad5940_seq_builder *b, uint32_t opcode)
{
	if (b->err != 0) {
		return;
	}
	if (b->len >= b->cap) {
		b->err = -ENOSPC;
		return;
	}
	b->buf[b->len] = opcode;
	b->len++;
}

void ad5940_seq_builder_add_wait(struct ad5940_seq_builder *b, uint32_t sysclk_cycles)
{
	ad5940_seq_builder_append(b, AD5940_SEQ_WAIT_CLKS(sysclk_cycles));
}

void ad5940_seq_builder_add_afe_state(struct ad5940_seq_builder *b,
				      enum ad5940_afe_state state)
{
	uint32_t afecon;

	switch (state) {
	case AD5940_AFE_STATE_ANALOG:
		afecon = AD5940_AFECON_ANALOG;
		break;
	case AD5940_AFE_STATE_CONV:
		afecon = AD5940_AFECON_CONV;
		break;
	case AD5940_AFE_STATE_OFF:
	default:
		afecon = AD5940_AFECON_OFF;
		break;
	}

	ad5940_seq_builder_append(b, AD5940_SEQ_WR_REG(AD5940_REG_AFECON, afecon));
}

void ad5940_seq_builder_add_adc_mux(struct ad5940_seq_builder *b,
				    uint8_t mux_p, uint8_t mux_n)
{
	ad5940_seq_builder_append(b, AD5940_SEQ_WR_REG(AD5940_REG_ADCCON,
							AD5940_ADCMUX_WORD(mux_p, mux_n)));
}

void ad5940_seq_builder_add_adc_mux_gain(struct ad5940_seq_builder *b,
					 uint8_t mux_p, uint8_t mux_n,
					 uint8_t pga_gain)
{
	ad5940_seq_builder_append(b, AD5940_SEQ_WR_REG(AD5940_REG_ADCCON,
				  AD5940_ADCMUX_WORD(mux_p, mux_n) |
				  FIELD_PREP(AD5940_ADCCON_GNPGA_MSK, pga_gain)));
}

void ad5940_seq_builder_add_adc_filter(struct ad5940_seq_builder *b,
				       uint8_t sinc3_osr, uint8_t sinc2_osr,
				       bool lpf_bypass, uint8_t sample_rate)
{
	uint32_t adcfiltercon = FIELD_PREP(AD5940_ADCFILTERCON_SINC3OSR_MSK, sinc3_osr) |
				FIELD_PREP(AD5940_ADCFILTERCON_SINC2OSR_MSK, sinc2_osr);

	if (lpf_bypass) {
		adcfiltercon |= AD5940_ADCFILTERCON_LPFBYPEN_MSK;
	}
	if (sample_rate != 0u) {
		adcfiltercon |= AD5940_ADCFILTERCON_ADCSAMPLERATE_MSK;
	}

	ad5940_seq_builder_append(b, AD5940_SEQ_WR_REG(AD5940_REG_ADCFILTERCON, adcfiltercon));
}

void ad5940_seq_builder_add_switch_matrix(struct ad5940_seq_builder *b,
					  uint32_t dsw, uint32_t psw,
					  uint32_t nsw, uint32_t tsw)
{
	ad5940_seq_builder_append(b, AD5940_SEQ_WR_REG(AD5940_REG_DSWFULLCON, dsw));
	ad5940_seq_builder_append(b, AD5940_SEQ_WR_REG(AD5940_REG_PSWFULLCON, psw));
	ad5940_seq_builder_append(b, AD5940_SEQ_WR_REG(AD5940_REG_NSWFULLCON, nsw));
	ad5940_seq_builder_append(b, AD5940_SEQ_WR_REG(AD5940_REG_TSWFULLCON, tsw));
	ad5940_seq_builder_append(b, AD5940_SEQ_WR_REG(AD5940_REG_SWCON,
							AD5940_SWCON_SWSOURCESEL_MSK));
}

void ad5940_seq_builder_add_fifo_watermark(struct ad5940_seq_builder *b, uint16_t words)
{
	ad5940_seq_builder_append(b, AD5940_SEQ_WR_REG(AD5940_REG_DATAFIFOTHRES,
				  FIELD_PREP(AD5940_DATAFIFOTHRES_HIGHTHRES_MSK, words)));
}

void ad5940_seq_builder_add_sleep(struct ad5940_seq_builder *b)
{
	ad5940_seq_builder_append(b, AD5940_SEQ_WR_REG(AD5940_REG_SEQSLPLOCK,
							AD5940_SEQSLPLOCK_KEY));
	ad5940_seq_builder_append(b, AD5940_SEQ_SLP());
}

void ad5940_seq_builder_add_stop(struct ad5940_seq_builder *b)
{
	ad5940_seq_builder_append(b, AD5940_SEQ_STOP());
}
