/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/debug/mipi_stp_decoder.h>
#include <string.h>

#if defined(CONFIG_CPU_CORTEX_M) && \
	!defined(CONFIG_CPU_CORTEX_M0) && \
	!defined(CONFIG_CPU_CORTEX_M0PLUS)
#define UNALIGNED_ACCESS_SUPPORTED 1
#else
#define UNALIGNED_ACCESS_SUPPORTED 0
#endif

enum stp_state {
	STP_STATE_OP,
	STP_STATE_DATA,
	STP_STATE_TS,
	STP_STATE_OUT_OF_SYNC,
};

enum stp_id {
	STP_NULL,
	STP_M8,
	STP_MERR,
	STP_C8,
	STP_D8,
	STP_D16,
	STP_D32,
	STP_D64,
	STP_D8MTS,
	STP_D16MTS,
	STP_D32MTS,
	STP_D64MTS,
	STP_D4,
	STP_D4MTS,
	STP_FLAG_TS,
	STP_VERSION,
	STP_TAG_3NIBBLE_OP = STP_VERSION,
	STP_NULL_TS,
	STP_USER,
	STP_USER_TS,
	STP_TIME,
	STP_TIME_TS,
	STP_TRIG,
	STP_TRIG_TS,
	STP_FREQ,
	STP_FREQ_TS,
	STP_XSYNC,
	STP_XSYNC_TS,
	STP_FREQ_40,
	STP_TAG_4NIBBLE_OP = STP_FREQ_40,
	STP_FREQ_40_TS,
	STP_DIP,
	STP_M16,
	STP_TAG_2NIBBLE_OP = STP_M16,
	STP_GERR,
	STP_C16,
	STP_D8TS,
	STP_D16TS,
	STP_D32TS,
	STP_D64TS,
	STP_D8M,
	STP_D16M,
	STP_D32M,
	STP_D64M,
	STP_D4TS,
	STP_D4M,
	STP_FLAG,
	STP_ASYNC,
	STP_INVALID,
	STP_OP_MAX
};

#define STP_LONG_OP_ID 0xF
#define STP_2B_OP_ID   0xF0

#define STP_VAR_DATA 0xff

typedef void (*stp_cb)(uint64_t data, uint64_t ts);

struct stp_item {
	const char *name;
	enum stp_id type;
	uint8_t id[3];
	uint8_t id_ncnt;
	uint8_t d_ncnt;
	bool has_ts;
	stp_cb cb;
};

#define STP_ITEM(_type, _id, _id_ncnt, _d_ncnt, _has_ts, _cb)                                      \
	{                                                                                          \
		.name = STRINGIFY(_type), .type = _type, .id = {__DEBRACKET _id},                  \
					  .id_ncnt = _id_ncnt, .d_ncnt = _d_ncnt,                  \
					  .has_ts = _has_ts, .cb = (stp_cb)_cb                     \
	}

static struct mipi_stp_decoder_config cfg;
static uint64_t prev_ts;
static uint64_t base_ts;
static enum stp_state state;
static size_t ntotal;
static size_t ncnt;
static size_t noff;
static uint16_t curr_ch;

static void data4_cb(uint64_t data, uint64_t ts)
{
	ARG_UNUSED(ts);
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA4, d, NULL, false);
}

static void data8_cb(uint64_t data, uint64_t ts)
{
	ARG_UNUSED(ts);
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA8, d, NULL, false);
}

static void data16_cb(uint64_t data, uint64_t ts)
{
	ARG_UNUSED(ts);
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA16, d, NULL, false);
}

static void data32_cb(uint64_t data, uint64_t ts)
{
	ARG_UNUSED(ts);
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA32, d, NULL, false);
}

static void data64_cb(uint64_t data, uint64_t ts)
{
	ARG_UNUSED(ts);
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA64, d, NULL, false);
}

static void data4_m_cb(uint64_t data, uint64_t ts)
{
	ARG_UNUSED(ts);
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA4, d, NULL, true);
}

static void data8_m_cb(uint64_t data, uint64_t ts)
{
	ARG_UNUSED(ts);
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA8, d, NULL, true);
}

static void data16_m_cb(uint64_t data, uint64_t ts)
{
	ARG_UNUSED(ts);
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA16, d, NULL, true);
}

static void data32_m_cb(uint64_t data, uint64_t ts)
{
	ARG_UNUSED(ts);
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA32, d, NULL, true);
}

static void data64_m_cb(uint64_t data, uint64_t ts)
{
	ARG_UNUSED(ts);
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA64, d, NULL, true);
}

static void data4_ts_cb(uint64_t data, uint64_t ts)
{
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA4, d, &ts, false);
}

static void data8_ts_cb(uint64_t data, uint64_t ts)
{
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA8, d, &ts, false);
}

static void data16_ts_cb(uint64_t data, uint64_t ts)
{
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA16, d, &ts, false);
}

static void data32_ts_cb(uint64_t data, uint64_t ts)
{
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA32, d, &ts, false);
}

static void data64_ts_cb(uint64_t data, uint64_t ts)
{
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA64, d, &ts, false);
}

static void data4_mts_cb(uint64_t data, uint64_t ts)
{
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA4, d, &ts, true);
}

static void data8_mts_cb(uint64_t data, uint64_t ts)
{
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA8, d, &ts, true);
}

static void data16_mts_cb(uint64_t data, uint64_t ts)
{
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA16, d, &ts, true);
}

static void data32_mts_cb(uint64_t data, uint64_t ts)
{
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA32, d, &ts, true);
}

static void data64_mts_cb(uint64_t data, uint64_t ts)
{
	union mipi_stp_decoder_data d = {.data = data};

	cfg.cb(STP_DATA64, d, &ts, true);
}

static void master_cb(uint64_t id, uint64_t ts)
{
	ARG_UNUSED(ts);
	uint16_t m_id = (uint16_t)id;
	union mipi_stp_decoder_data data = {.id = m_id};

	curr_ch = 0;

	cfg.cb(STP_DECODER_MASTER, data, NULL, false);
}

static void channel16_cb(uint64_t id, uint64_t ts)
{
	uint16_t ch = (uint16_t)id;

	curr_ch = 0xFF00 & ch;

	ARG_UNUSED(ts);
	union mipi_stp_decoder_data data = {.id = ch};

	cfg.cb(STP_DECODER_CHANNEL, data, NULL, false);
}
static void channel_cb(uint64_t id, uint64_t ts)
{
	uint16_t ch = (uint16_t)id;

	ch |= curr_ch;

	ARG_UNUSED(ts);
	union mipi_stp_decoder_data data = {.id = ch};

	cfg.cb(STP_DECODER_CHANNEL, data, NULL, false);
}

static void merror_cb(uint64_t err, uint64_t ts)
{
	ARG_UNUSED(ts);
	union mipi_stp_decoder_data data = {.err = (uint32_t)err};

	cfg.cb(STP_DECODER_MERROR, data, NULL, false);
}

static void gerror_cb(uint64_t err, uint64_t ts)
{
	ARG_UNUSED(ts);
	union mipi_stp_decoder_data data = {.err = (uint32_t)err};

	cfg.cb(STP_DECODER_GERROR, data, NULL, false);
}

static void flag_cb(uint64_t data, uint64_t ts)
{
	ARG_UNUSED(ts);
	ARG_UNUSED(data);
	union mipi_stp_decoder_data dummy = {.dummy = 0};

	cfg.cb(STP_DECODER_FLAG, dummy, NULL, false);
}

static void flag_ts_cb(uint64_t unused, uint64_t ts)
{
	ARG_UNUSED(unused);
	union mipi_stp_decoder_data data = {.dummy = 0};

	cfg.cb(STP_DECODER_FLAG, data, &ts, false);
}

static void version_cb(uint64_t version, uint64_t ts)
{
	ARG_UNUSED(ts);

	curr_ch = 0;

	union mipi_stp_decoder_data data = {.ver = version};

	cfg.cb(STP_DECODER_VERSION, data, NULL, false);
}

static void notsup_cb(uint64_t data, uint64_t ts)
{
	ARG_UNUSED(ts);
	ARG_UNUSED(data);

	union mipi_stp_decoder_data dummy = {.dummy = 0};

	cfg.cb(STP_DECODER_NOT_SUPPORTED, dummy, NULL, false);
}

static void freq_cb(uint64_t freq, uint64_t ts)
{
	ARG_UNUSED(ts);

	union mipi_stp_decoder_data data = {.freq = freq};

	cfg.cb(STP_DECODER_FREQ, data, NULL, false);
}

static void freq_ts_cb(uint64_t freq, uint64_t ts)
{
	union mipi_stp_decoder_data data = {.freq = freq};

	cfg.cb(STP_DECODER_FREQ, data, &ts, false);
}

static void null_cb(uint64_t data, uint64_t ts)
{
	ARG_UNUSED(ts);
	ARG_UNUSED(data);

	union mipi_stp_decoder_data dummy = {.dummy = 0};

	cfg.cb(STP_DECODER_NULL, dummy, NULL, false);
}

static void async_cb(uint64_t data, uint64_t ts)
{
	ARG_UNUSED(ts);
	ARG_UNUSED(data);

	union mipi_stp_decoder_data dummy = {.dummy = 0};

	cfg.cb(STP_DECODER_ASYNC, dummy, NULL, false);
}

static const struct stp_item items[] = {
	STP_ITEM(STP_NULL, (0x0), 1, 0, false, null_cb),
	STP_ITEM(STP_M8, (0x1), 1, 2, false, master_cb),
	STP_ITEM(STP_MERR, (0x2), 1, 2, false, merror_cb),
	STP_ITEM(STP_C8, (0x3), 1, 2, false, channel_cb),
	STP_ITEM(STP_D8, (0x4), 1, 2, false, data8_cb),
	STP_ITEM(STP_D16, (0x5), 1, 4, false, data16_cb),
	STP_ITEM(STP_D32, (0x6), 1, 8, false, data32_cb),
	STP_ITEM(STP_D64, (0x7), 1, 16, false, data64_cb),
	STP_ITEM(STP_D8MTS, (0x8), 1, 2, true, data8_mts_cb),
	STP_ITEM(STP_D16MTS, (0x9), 1, 4, true, data16_mts_cb),
	STP_ITEM(STP_D32MTS, (0xa), 1, 8, true, data32_mts_cb),
	STP_ITEM(STP_D64MTS, (0xb), 1, 16, true, data64_mts_cb),
	STP_ITEM(STP_D4, (0xc), 1, 1, false, data4_cb),
	STP_ITEM(STP_D4MTS, (0xd), 1, 1, true, data4_mts_cb),
	STP_ITEM(STP_FLAG_TS, (0xe), 1, 0, true, flag_ts_cb),
	STP_ITEM(STP_VERSION, (0xf0, 0x00), 3, 1, false, version_cb),
	STP_ITEM(STP_NULL_TS, (0xf0, 0x01), 3, 0, true, notsup_cb),
	STP_ITEM(STP_USER, (0xf0, 0x02), 3, 0, false, notsup_cb),
	STP_ITEM(STP_USER_TS, (0xf0, 0x03), 3, 0, true, notsup_cb),
	STP_ITEM(STP_TIME, (0xf0, 0x04), 3, 0, false, notsup_cb),
	STP_ITEM(STP_TIME_TS, (0xf0, 0x05), 3, 0, true, notsup_cb),
	STP_ITEM(STP_TRIG, (0xf0, 0x06), 3, 0, false, notsup_cb),
	STP_ITEM(STP_TRIG_TS, (0xf0, 0x07), 3, 0, true, notsup_cb),
	STP_ITEM(STP_FREQ, (0xf0, 0x08), 3, 8, false, freq_cb),
	STP_ITEM(STP_FREQ_TS, (0xf0, 0x09), 3, 8, true, freq_ts_cb),
	STP_ITEM(STP_XSYNC, (0xf0, 0x0a), 3, 0, false, notsup_cb),
	STP_ITEM(STP_XSYNC_TS, (0xf0, 0x0b), 3, 0, true, notsup_cb),
	STP_ITEM(STP_FREQ_40, (0xf0, 0xf0), 4, 10, false, freq_cb),
	STP_ITEM(STP_FREQ_40_TS, (0xf0, 0xf1), 4, 0, true, notsup_cb),
	STP_ITEM(STP_DIP, (0xf0, 0xf2), 4, 0, false, notsup_cb),
	STP_ITEM(STP_M16, (0xf1), 2, 4, false, master_cb),
	STP_ITEM(STP_GERR, (0xf2), 2, 2, false, gerror_cb),
	STP_ITEM(STP_C16, (0xf3), 2, 4, false, channel16_cb),
	STP_ITEM(STP_D8TS, (0xf4), 2, 2, true, data8_ts_cb),
	STP_ITEM(STP_D16TS, (0xf5), 2, 4, true, data16_ts_cb),
	STP_ITEM(STP_D32TS, (0xf6), 2, 8, true, data32_ts_cb),
	STP_ITEM(STP_D64TS, (0xf7), 2, 16, true, data64_ts_cb),
	STP_ITEM(STP_D8M, (0xf8), 2, 2, false, data8_m_cb),
	STP_ITEM(STP_D16M, (0xf9), 2, 4, false, data16_m_cb),
	STP_ITEM(STP_D32M, (0xfa), 2, 8, false, data32_m_cb),
	STP_ITEM(STP_D64M, (0xfb), 2, 16, false, data64_m_cb),
	STP_ITEM(STP_D4TS, (0xfc), 2, 1, true, data4_ts_cb),
	STP_ITEM(STP_D4M, (0xfd), 2, 1, false, data4_m_cb),
	STP_ITEM(STP_FLAG, (0xfe), 2, 0, false, flag_cb),
	STP_ITEM(STP_ASYNC, (0xff, 0xff, 0xff), 6, 16, false, async_cb),
	STP_ITEM(STP_INVALID, (0x0), 0, 0, false, NULL),
};

/** @brief Decode a nibble and read opcode from the stream.
 *
 * Function reads a nibble and continues or starts decoding of a STP opcode.
 *
 * @param	  data		Pointer to the stream.
 * @param[in,out] noff		Offset (in nibbles).
 * @param	  nlen		Length (in nibbles).
 * @param[in,out] ncnt		Current number of nibbles
 * @param[in,out] ntotal	Number of nibbles in the opcode.
 *
 * @retval STP_INVALID	Opcode decoding is in progress.
 * @retval opcode	Decoded opcode.
 */
static inline uint8_t get_nibble(const uint8_t *data, size_t noff)
{
	uint8_t ret = data[noff / 2];

	if (noff & 0x1UL) {
		ret >>= 4;
	}

	ret &= 0x0F;

	return ret;
}

static inline void get_nibbles64(const uint8_t *src, size_t src_noff, uint8_t *dst, size_t dst_noff,
				 size_t nlen)
{
	bool src_ba = (src_noff & 0x1UL) == 0;
	bool dst_ba = (dst_noff & 0x1UL) == 0;
	uint64_t *src64 = (uint64_t *)&src[src_noff / 2];
	uint64_t *dst64 = (uint64_t *)&dst[dst_noff / 2];

	if (nlen == 16) {
		/* dst must be aligned. */
		if (src_ba) {
			uint32_t *s32 = (uint32_t *)src64;
			uint32_t *d32 = (uint32_t *)dst64;

			d32[0] = s32[0];
			d32[1] = s32[1];
		} else {
			uint64_t part_a = src64[0] >> 4;
			uint64_t part_b = src64[1] << 60;

			dst64[0] = part_a | part_b;
		}
		return;
	}

	uint64_t mask = BIT64_MASK(nlen * 4) << (src_ba ? 0 : 4);
	uint64_t src_d = src64[0] & mask;

	if (((src_noff ^ dst_noff) & 0x1UL) == 0) {
		dst64[0] |= src_d;
	} else if (dst_ba) {
		dst64[0] |= (src_d >> 4);
	} else {
		dst64[0] |= (src_d << 4);
	}
}

/* Function performs getting nibbles in less efficient way but does not use unaligned
 * access which may not be supported by some platforms.
 */
static void get_nibbles_unaligned(const uint8_t *src, size_t src_noff, uint8_t *dst,
				  size_t dst_noff, size_t nlen)
{
	for (size_t i = 0; i < nlen; i++) {
		size_t idx = (src_noff + i) / 2;
		size_t ni = (src_noff + i) & 0x1;
		uint8_t n = src[idx] >> (ni ? 4 : 0);
		size_t d_idx = (dst_noff + i) / 2;
		size_t dni = (dst_noff + i) & 0x1;

		if (dni == 0) {
			dst[d_idx] = n;
		} else {
			dst[d_idx] |= n << 4;
		}
	}
}

static inline void get_nibbles(const uint8_t *src, size_t src_noff, uint8_t *dst, size_t dst_noff,
			       size_t nlen)
{
	if (!UNALIGNED_ACCESS_SUPPORTED) {
		get_nibbles_unaligned(src, src_noff, dst, dst_noff, nlen);
		return;
	}

	bool src_ba = (src_noff & 0x1UL) == 0;
	bool dst_ba = (dst_noff & 0x1UL) == 0;
	uint32_t *src32 = (uint32_t *)&src[src_noff / 2];
	uint32_t *dst32 = (uint32_t *)&dst[dst_noff / 2];

	if (nlen > 8) {
		get_nibbles64(src, src_noff, dst, dst_noff, nlen);
		return;
	} else if (nlen == 8) {
		/* dst must be aligned. */
		if (src_ba) {
			dst32[0] = src32[0];
		} else {
			uint32_t part_a = src32[0] >> 4;
			uint32_t part_b = src32[1] << 28;

			dst32[0] = part_a | part_b;
		}
		return;
	}

	uint32_t mask = BIT_MASK(nlen * 4) << (src_ba ? 0 : 4);
	uint32_t src_d = src32[0] & mask;

	if (((src_noff ^ dst_noff) & 0x1UL) == 0) {
		dst32[0] |= src_d;
	} else if (dst_ba) {
		dst32[0] |= (src_d >> 4);
	} else {
		dst32[0] |= (src_d << 4);
	}
}

/* Function swaps nibbles in a byte. */
static uint8_t swap8(uint8_t byte)
{
	return (byte << 4) | (byte >> 4);
}

/* Function swaps nibbles in a 16 bit variable. */
static uint16_t swap16(uint16_t halfword)
{
	halfword = __builtin_bswap16(halfword);
	uint16_t d1 = (halfword & 0xf0f0) >> 4;
	uint16_t d2 = (halfword & 0x0f0f) << 4;

	return d1 | d2;
}

/* Function swaps nibbles in a 32 bit word. */
static uint32_t swap32(uint32_t word)
{
	word = __builtin_bswap32(word);
	uint32_t d1 = (word & 0xf0f0f0f0) >> 4;
	uint32_t d2 = (word & 0x0f0f0f0f) << 4;

	return d1 | d2;
}

/* Function swaps nibbles in a 64 bit word. */
static uint64_t swap64(uint64_t dword)
{
	uint32_t l = (uint32_t)dword;
	uint32_t u = (uint32_t)(dword >> 32);

	return ((uint64_t)swap32(l) << 32) | (uint64_t)swap32(u);
}

static void swap_n(uint8_t *data, uint32_t n)
{
	switch (n) {
	case 2:
		*data = swap8(*data);
		break;
	case 4:
		*(uint16_t *)data = swap16(*(uint16_t *)data);
		break;
	case 8:
		*(uint32_t *)data = swap32(*(uint32_t *)data);
		break;
	case 16:
		*(uint64_t *)data = swap64(*(uint64_t *)data);
		break;
	default:
		*(uint64_t *)data = swap64(*(uint64_t *)data);
		*(uint64_t *)data >>= (4 * (16 - n));
		break;
	}
}

static enum stp_id get_op(const uint8_t *data, size_t *noff, size_t *nlen, size_t *ncnt,
			  size_t *ntotal)
{
	uint8_t op = 0;

	op = get_nibble(data, *noff);

	*noff += 1;
	*ncnt += 1;
	if (*ntotal == 0 && *ncnt == 1) {
		/* Starting to read op. */
		/* op code has only 1 nibble. */
		if (op != 0xF) {
			return (enum stp_id)op;
		}
	} else if (*ncnt == 2) {
		if (op == 0xF) {
			/* ASYNC*/
			*ntotal = 6;
		} else if (op != 0) {
			return (enum stp_id)(STP_TAG_2NIBBLE_OP - 1 + op);
		}
	} else if (*ncnt == 3) {
		if (op != 0xf) {
			return (enum stp_id)(STP_TAG_3NIBBLE_OP + op);
		} else if (*ntotal == 0) {
			*ntotal = 4;
		}
	} else if (*ncnt == *ntotal) {
		if (*ntotal == 4) {
			return (enum stp_id)(STP_TAG_4NIBBLE_OP + op);
		} else {
			return STP_ASYNC;
		}
	}

	return STP_INVALID;
}

void mipi_stp_decoder_sync_loss(void)
{
	state = STP_STATE_OUT_OF_SYNC;
	ncnt = 0;
	ntotal = 0;
}

int mipi_stp_decoder_decode(const uint8_t *data, size_t len)
{
	static enum stp_id curr_id = STP_INVALID;
	static uint8_t data_buf[8] __aligned(sizeof(uint64_t));
	static uint8_t ts_buf[8] __aligned(sizeof(uint64_t));
	uint64_t *data64 = (uint64_t *)data_buf;
	uint64_t *ts64 = (uint64_t *)ts_buf;
	size_t nlen = 2 * len;

	do {
		switch (state) {
		case STP_STATE_OUT_OF_SYNC: {
			uint8_t b = get_nibble(data, noff);

			noff++;
			if (ncnt < 21 && b == 0xF) {
				ncnt++;
			} else if (ncnt == 21 && b == 0) {
				curr_id = STP_INVALID;
				ncnt = 0;

				items[STP_ASYNC].cb(0, 0);
				state = STP_STATE_OP;
			} else {
				ncnt = 0;
			}
			break;
		}
		case STP_STATE_OP: {
			curr_id = get_op(data, &noff, &nlen, &ncnt, &ntotal);
			if (curr_id != STP_INVALID) {
				ntotal = items[curr_id].d_ncnt;
				ncnt = 0;
				if (ntotal > 0) {
					state = STP_STATE_DATA;
					data64[0] = 0;
				} else if (items[curr_id].has_ts) {
					state = STP_STATE_TS;
				} else {
					/* item without data and ts, notify. */
					items[curr_id].cb(0, 0);
					curr_id = STP_INVALID;
				}
			}
			break;
		}
		case STP_STATE_DATA: {
			size_t ncpy = MIN(ntotal - ncnt, nlen - noff);

			get_nibbles(data, noff, data_buf, ncnt, ncpy);

			ncnt += ncpy;
			noff += ncpy;
			if (ncnt == ntotal) {
				swap_n(data_buf, ntotal);
				ncnt = 0;
				if (items[curr_id].has_ts) {
					ncnt = 0;
					ntotal = 0;
					state = STP_STATE_TS;
				} else {
					items[curr_id].cb(*data64, 0);
					curr_id = STP_INVALID;
					state = STP_STATE_OP;
					ntotal = 0;
					ncnt = 0;
				}
			}
			break;
		}
		case STP_STATE_TS:
			if (ntotal == 0 && ncnt == 0) {
				/* TS to be read but length is unknown yet */
				*ts64 = 0;
				ntotal = get_nibble(data, noff);
				noff++;
				/* Values up to 12 represents number of nibbles on which
				 * timestamp is encoded. Above are the exceptions:
				 * - 13 => 14 nibbles
				 * - 14 => 16 nibbles
				 */
				if (ntotal > 12) {
					if (ntotal == 13) {
						ntotal = 14;
						base_ts = ~BIT64_MASK(4 * ntotal) & prev_ts;
					} else {
						ntotal = 16;
						base_ts = 0;
					}
				} else {
					base_ts = ~BIT64_MASK(4 * ntotal) & prev_ts;
				}

			} else {
				size_t ncpy = MIN(ntotal - ncnt, nlen - noff);

				get_nibbles(data, noff, ts_buf, ncnt, ncpy);
				ncnt += ncpy;
				noff += ncpy;
				if (ncnt == ntotal) {
					swap_n(ts_buf, ntotal);
					prev_ts = base_ts | *ts64;
					items[curr_id].cb(*data64, prev_ts);
					curr_id = STP_INVALID;
					state = STP_STATE_OP;
					ntotal = 0;
					ncnt = 0;
				}
			}
			break;

		default:
			break;
		}
	} while (noff < nlen);

	noff = 0;

	return 0;
}

int mipi_stp_decoder_init(const struct mipi_stp_decoder_config *config)
{
	state = config->start_out_of_sync ? STP_STATE_OUT_OF_SYNC : STP_STATE_OP;
	ntotal = 0;
	ncnt = 0;
	cfg = *config;
	prev_ts = 0;
	base_ts = 0;
	noff = 0;

	return 0;
}
