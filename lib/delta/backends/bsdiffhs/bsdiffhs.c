/*
 * Copyright (c) 2026 Clovis Corde
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <zephyr/delta/delta.h>
#include <zephyr/delta/backend_bsdiffhs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "heatshrink_decoder.h"

LOG_MODULE_DECLARE(delta, CONFIG_DELTA_LOG_LEVEL);

/*
 * Patch = 18-byte header
 *   [0..7]   magic "BSDIFFHS"
 *   [8..15]  target_size (uint64)
 *   [16]     window_sz2 (heatshrink configuration)
 *   [17]     lookahead_sz2 (heatshrink configuration)
 */
#define MAGIC                "BSDIFFHS"
#define MAGIC_SIZE           8
#define TARGET_SIZE_OFFSET   MAGIC_SIZE
#define WINDOW_SZ2_OFFSET    (TARGET_SIZE_OFFSET + MAGIC_SIZE)
#define LOOKAHEAD_SZ2_OFFSET (WINDOW_SZ2_OFFSET + 1)
#define HEADER_SIZE          (LOOKAHEAD_SZ2_OFFSET + 1)

/* bsdiff control tuple: (diff_len, extra_len, source_seek) */
#define CTRL_FIELD_SIZE 8
#define CTRL_FIELDS     3
#define CTRL_TUPLE_SIZE (CTRL_FIELD_SIZE * CTRL_FIELDS)

/*
 * The whole work_buf is used as the patch read-ahead buffer; this is
 * the smallest length that keeps the refill overhead reasonable.
 */
#define MIN_WORK_SIZE 16

/* Per-iteration block chunk, on the stack. */
#define CHUNK_SIZE 256

/* heatshrink input buffer for the heap-allocated decoder. */
#define HS_INPUT_BUF_SIZE 64

/* A tuple drives a diff block (added to the source) then an extra block. */
enum block_kind {
	BLOCK_DIFF,
	BLOCK_EXTRA,
};

/* One decoded control tuple. */
struct bsdiff_control {
	int64_t diff_len;
	int64_t extra_len;
	int64_t source_seek;
};

struct bsdiffhs_state {
	const struct delta_io *io;
	const struct delta_config *cfg;

	/* Patch stream position and the read-ahead window (work_buf). */
	size_t patch_pos;
	uint8_t *patch_buf;
	size_t patch_buf_len;
	size_t patch_buf_pos;

	size_t source_pos;
	size_t target_pos;
	size_t target_size;

	/* Embedded in static-alloc mode, heap-allocated otherwise. */
	heatshrink_decoder *decoder;
};

/* Decode a bsdiff sign-magnitude int64 (high bit of buf[7] is the sign). */
static int64_t decode_sign_magnitude(const uint8_t *buf)
{
	int64_t value = buf[7] & 0x7F;

	value = (value << 8) | buf[6];
	value = (value << 8) | buf[5];
	value = (value << 8) | buf[4];
	value = (value << 8) | buf[3];
	value = (value << 8) | buf[2];
	value = (value << 8) | buf[1];
	value = (value << 8) | buf[0];

	return (buf[7] & 0x80) ? -value : value;
}

/*
 * A block ends on its own frame boundary, so bytes already read ahead
 * belong to the next block: rewind patch_pos before resetting.
 */
static void reset_for_next_block(struct bsdiffhs_state *state)
{
	state->patch_pos -= state->patch_buf_len - state->patch_buf_pos;
	state->patch_buf_pos = 0;
	state->patch_buf_len = 0;
	heatshrink_decoder_reset(state->decoder);
}

/* Refill the read-ahead buffer from the patch, return -EINVAL once it is exhausted. */
static int refill_patch_buf(struct bsdiffhs_state *state)
{
	size_t remaining = state->cfg->patch_size - state->patch_pos;
	size_t to_read = MIN(state->cfg->work_sz, remaining);
	int ret;

	if (remaining == 0) {
		return -EINVAL;
	}

	ret = state->io->read_patch(state->io->ctx, state->patch_pos, state->patch_buf, to_read);
	if (ret != 0) {
		LOG_ERR("read_patch failed at %zu: %d", state->patch_pos, ret);
		return ret;
	}
	state->patch_pos += to_read;
	state->patch_buf_len = to_read;
	state->patch_buf_pos = 0;
	return 0;
}

/*
 * Decompress exactly out_len bytes from the current heatshrink frame,
 * the caller always knows the length from the bsdiff block sizes.
 */
static int decompress(struct bsdiffhs_state *state, uint8_t *out, size_t out_len)
{
	size_t produced = 0;

	while (produced < out_len) {
		HSD_poll_res poll_res;
		size_t sunk;
		HSD_sink_res sink_res;
		int ret;

		do {
			size_t polled;

			poll_res = heatshrink_decoder_poll(state->decoder, out + produced,
							   out_len - produced, &polled);
			if (poll_res < 0) {
				LOG_ERR("heatshrink poll error: %d", poll_res);
				return -EIO;
			}
			produced += polled;
		} while (poll_res == HSDR_POLL_MORE && produced < out_len);

		if (produced == out_len) {
			/* Never sink past the frame we were asked to decode. */
			break;
		}

		if (state->patch_buf_pos >= state->patch_buf_len) {
			ret = refill_patch_buf(state);
			if (ret != 0) {
				return ret;
			}
		}

		/* Sink one byte at a time: more would pull bytes from the next
		 * block into the decoder, lost on the reset.
		 */
		sink_res = heatshrink_decoder_sink(
			state->decoder, state->patch_buf + state->patch_buf_pos, 1, &sunk);
		if (sink_res != HSDR_SINK_OK) {
			LOG_ERR("heatshrink sink error: %d", sink_res);
			return -EIO;
		}
		state->patch_buf_pos += sunk;
	}

	return 0;
}

/*
 * Parse and validate the header, return the heatshrink parameters the
 * patch was compressed with (the caller sets up the decoder from them).
 */
static int parse_header(struct bsdiffhs_state *state, uint8_t *window_sz2, uint8_t *lookahead_sz2)
{
	uint8_t header[HEADER_SIZE];
	uint64_t target_size;
	int ret;

	if (state->cfg->patch_size < HEADER_SIZE) {
		return -EINVAL;
	}

	ret = state->io->read_patch(state->io->ctx, 0, header, HEADER_SIZE);
	if (ret != 0) {
		LOG_ERR("read patch header failed: %d", ret);
		return ret;
	}

	if (memcmp(header, MAGIC, MAGIC_SIZE) != 0) {
		return -EINVAL;
	}

	target_size = sys_get_le64(header + TARGET_SIZE_OFFSET);
	if (target_size == 0) {
		return -EINVAL;
	}
	if (target_size > state->cfg->max_target_size) {
		LOG_ERR("target size %llu exceeds max %zu", target_size,
			state->cfg->max_target_size);
		return -ENOSPC;
	}

	*window_sz2 = header[WINDOW_SZ2_OFFSET];
	*lookahead_sz2 = header[LOOKAHEAD_SZ2_OFFSET];
	if (*window_sz2 < 4 || *window_sz2 > 15 || *lookahead_sz2 < 3 || *lookahead_sz2 > 14 ||
	    *lookahead_sz2 >= *window_sz2) {
		return -EINVAL;
	}

	state->target_size = (size_t)target_size;
	state->patch_pos = HEADER_SIZE;

	LOG_DBG("patch header: target %zu bytes, window_sz2 %u, lookahead_sz2 %u",
		state->target_size, *window_sz2, *lookahead_sz2);
	return 0;
}

/* Read and decode the next bsdiff control tuple. */
static int read_control_tuple(struct bsdiffhs_state *state, struct bsdiff_control *control)
{
	uint8_t tuple[CTRL_TUPLE_SIZE];
	int ret;

	ret = decompress(state, tuple, sizeof(tuple));
	if (ret != 0) {
		return ret;
	}
	reset_for_next_block(state);

	control->diff_len = decode_sign_magnitude(tuple + 0 * CTRL_FIELD_SIZE);
	control->extra_len = decode_sign_magnitude(tuple + 1 * CTRL_FIELD_SIZE);
	control->source_seek = decode_sign_magnitude(tuple + 2 * CTRL_FIELD_SIZE);

	/* diff and extra lengths must be non-negative and fit a size_t bound. */
	if (control->diff_len < 0 || control->diff_len > INT_MAX || control->extra_len < 0 ||
	    control->extra_len > INT_MAX) {
		return -EINVAL;
	}
	return 0;
}

/*
 * Reconstruct block_len target bytes: a diff block adds the decompressed
 * delta to the source, an extra block writes it verbatim.
 */
static int apply_block(struct bsdiffhs_state *state, int64_t block_len, enum block_kind kind)
{
	uint8_t target_chunk[CHUNK_SIZE];
	uint8_t source_chunk[CHUNK_SIZE];
	int ret;

	if ((size_t)block_len > state->target_size - state->target_pos) {
		return -EINVAL;
	}

	while (block_len > 0) {
		size_t chunk_len = MIN((size_t)block_len, CHUNK_SIZE);

		ret = decompress(state, target_chunk, chunk_len);
		if (ret != 0) {
			return ret;
		}

		if (kind == BLOCK_DIFF) {
			if (state->source_pos + chunk_len > state->cfg->source_size) {
				return -EINVAL;
			}
			ret = state->io->read_source(state->io->ctx, state->source_pos,
						     source_chunk, chunk_len);
			if (ret != 0) {
				LOG_ERR("read_source failed at %zu: %d", state->source_pos, ret);
				return ret;
			}
			for (size_t i = 0; i < chunk_len; i++) {
				target_chunk[i] += source_chunk[i];
			}
			state->source_pos += chunk_len;
		}

		ret = state->io->write_target(state->io->ctx, state->target_pos, target_chunk,
					      chunk_len);
		if (ret != 0) {
			LOG_ERR("write_target failed at %zu: %d", state->target_pos, ret);
			return ret;
		}
		state->target_pos += chunk_len;
		block_len -= chunk_len;
	}

	reset_for_next_block(state);
	return 0;
}

/* Drive the bsdiff control stream until the whole target is rebuilt. */
static int reconstruct_target(struct bsdiffhs_state *state)
{
	struct bsdiff_control control;
	int ret;

	while (state->target_pos < state->target_size) {
		ret = read_control_tuple(state, &control);
		if (ret != 0) {
			return ret;
		}

		ret = apply_block(state, control.diff_len, BLOCK_DIFF);
		if (ret != 0) {
			return ret;
		}

		ret = apply_block(state, control.extra_len, BLOCK_EXTRA);
		if (ret != 0) {
			return ret;
		}

		/*
		 * Range-check the signed seek before adding, so a bad offset
		 * can't overflow the int64 sum.
		 */
		if (control.source_seek < -(int64_t)state->source_pos ||
		    control.source_seek > (int64_t)(state->cfg->source_size - state->source_pos)) {
			return -EINVAL;
		}
		state->source_pos += control.source_seek;
	}

	return 0;
}

/* Backend entry point: set up state and the decoder, then reconstruct. */
static int bsdiffhs_apply(const struct delta_io *io, const struct delta_config *cfg)
{
	struct bsdiffhs_state state;
	uint8_t window_sz2;
	uint8_t lookahead_sz2;
	int ret;
#if !defined(CONFIG_HEATSHRINK_DYNAMIC_ALLOC)
	heatshrink_decoder static_decoder;
#endif

	if (cfg->work_buf == NULL || cfg->work_sz < MIN_WORK_SIZE) {
		LOG_ERR("work_buf too small: have %zu, need at least %u", cfg->work_sz,
			MIN_WORK_SIZE);
		return -ENOMEM;
	}

	memset(&state, 0, sizeof(state));
	state.io = io;
	state.cfg = cfg;
	state.patch_buf = cfg->work_buf;

	ret = parse_header(&state, &window_sz2, &lookahead_sz2);
	if (ret != 0) {
		return ret;
	}

	/*
	 * The decoder must match the patch's window/lookahead: heap-sized to
	 * the patch, or compile-time fixed (then the patch must match).
	 */
#if defined(CONFIG_HEATSHRINK_DYNAMIC_ALLOC)
	state.decoder = heatshrink_decoder_alloc(HS_INPUT_BUF_SIZE, window_sz2, lookahead_sz2);
	if (state.decoder == NULL) {
		LOG_ERR("decoder alloc failed (window=%u lookahead=%u)", window_sz2, lookahead_sz2);
		return -ENOMEM;
	}
#else
	if (window_sz2 != CONFIG_HEATSHRINK_STATIC_WINDOW_BITS ||
	    lookahead_sz2 != CONFIG_HEATSHRINK_STATIC_LOOKAHEAD_BITS) {
		LOG_ERR("patch window/lookahead %u/%u != build %d/%d", window_sz2, lookahead_sz2,
			CONFIG_HEATSHRINK_STATIC_WINDOW_BITS,
			CONFIG_HEATSHRINK_STATIC_LOOKAHEAD_BITS);
		return -ENOTSUP;
	}
	state.decoder = &static_decoder;
	heatshrink_decoder_reset(state.decoder);
#endif

	ret = reconstruct_target(&state);

#if defined(CONFIG_HEATSHRINK_DYNAMIC_ALLOC)
	heatshrink_decoder_free(state.decoder);
#endif
	if (ret == 0) {
		LOG_INF("patch applied: %zu bytes written", state.target_pos);
	}
	return ret;
}

const struct delta_backend delta_backend_bsdiffhs = {
	.apply = bsdiffhs_apply,
};
