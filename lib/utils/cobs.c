/*
 * Copyright (c) 2024 Kelly Helmut Lord
 * Copyright (c) 2026 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <zephyr/data/cobs.h>

#if defined(CONFIG_NET_BUF)

struct cobs_net_buf_ctx {
	struct net_buf *buf;
	bool frame_end;
};

static int cobs_net_buf_cb(const uint8_t *buf, size_t len, void *user_data)
{
	struct cobs_net_buf_ctx *ctx = user_data;

	if (buf == NULL) {
		/* Stop decoding after the first frame */
		ctx->frame_end = true;
		return 1;
	}

	if (net_buf_tailroom(ctx->buf) < len) {
		return -ENOMEM;
	}

	(void)net_buf_add_mem(ctx->buf, buf, len);

	return 0;
}

int cobs_encode(struct net_buf *src, struct net_buf *dst, uint32_t flags)
{
	struct cobs_net_buf_ctx ctx = {.buf = dst};
	struct cobs_encoder enc;
	size_t dst_len = dst->len;
	int ret;

	(void)cobs_encoder_init(&enc, cobs_net_buf_cb, &ctx, flags);

	ret = cobs_encoder_write(&enc, src->data, src->len);
	if (ret >= 0) {
		ret = cobs_encoder_close(&enc);
	}

	if (ret < 0) {
		/* Leave both buffers as they were */
		(void)net_buf_remove_mem(dst, dst->len - dst_len);
		return ret;
	}

	(void)net_buf_pull(src, src->len);

	return 0;
}

int cobs_decode(struct net_buf *src, struct net_buf *dst, uint32_t flags)
{
	struct cobs_net_buf_ctx ctx = {.buf = dst};
	struct cobs_decoder dec;
	size_t dst_len = dst->len;
	int used;
	int ret;

	(void)cobs_decoder_init(&dec, cobs_net_buf_cb, &ctx, flags);

	used = cobs_decoder_write(&dec, src->data, src->len);
	if (used < 0) {
		ret = used;
	} else if ((flags & COBS_FLAG_TRAILING_DELIMITER) != 0U) {
		/* The frame must be complete, the decoder is not closed */
		ret = ctx.frame_end ? 0 : -EAGAIN;
	} else {
		ret = cobs_decoder_close(&dec);
	}

	if (ret < 0) {
		/* Leave both buffers as they were */
		(void)net_buf_remove_mem(dst, dst->len - dst_len);
		return ret;
	}

	(void)net_buf_pull(src, used);

	return 0;
}

#endif /* defined(CONFIG_NET_BUF) */

static inline void cobs_encoder_reset(struct cobs_encoder *enc)
{
	/* Reset buffer */
	enc->fragment[0] = 1;
}

static int cobs_encoder_finish(struct cobs_encoder *enc, bool close)
{
	uint8_t sentinel = COBS_FLAG_CUSTOM_DELIMITER(enc->flags);
	size_t len = enc->fragment[0];
	int ret;

	if (sentinel != 0x00) {
		for (size_t i = 0; i < len; ++i) {
			enc->fragment[i] ^= sentinel;
		}
	}

	ret = enc->cb(enc->fragment, len, enc->cb_user_data);
	if (ret < 0) {
		cobs_encoder_reset(enc);
		return ret;
	}

	if (close && (enc->flags & COBS_FLAG_TRAILING_DELIMITER) != 0U) {
		ret = enc->cb(&sentinel, 1, enc->cb_user_data);
		if (ret < 0) {
			cobs_encoder_reset(enc);
			return ret;
		}
	}

	cobs_encoder_reset(enc);

	return 0;
}

int cobs_encoder_init(struct cobs_encoder *enc, cobs_stream_cb cb, void *user_data, uint32_t flags)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	__ASSERT_NO_MSG(enc != NULL);

	enc->cb = cb;
	enc->cb_user_data = user_data;
	enc->flags = flags;

	cobs_encoder_reset(enc);

	return 0;
}

int cobs_encoder_close(struct cobs_encoder *enc)
{
	__ASSERT_NO_MSG(enc != NULL);

	return cobs_encoder_finish(enc, true);
}

int cobs_encoder_write(struct cobs_encoder *enc, const uint8_t *buf, size_t len)
{
	int ret;

	__ASSERT_NO_MSG(enc != NULL);
	__ASSERT_NO_MSG(len <= INT_MAX);

	for (size_t i = 0; i < len; ++i) {
		/* Finish if group is full */
		if (enc->fragment[0] == 0xff) {
			ret = cobs_encoder_finish(enc, false);
			if (ret < 0) {
				return ret;
			}
		}

		if (buf[i] == 0x00) {
			ret = cobs_encoder_finish(enc, false);
			if (ret < 0) {
				return ret;
			}

			continue;
		}

		enc->fragment[enc->fragment[0]] = buf[i];
		enc->fragment[0]++;
	}

	return len;
}

static inline void cobs_decoder_reset(struct cobs_decoder *dec)
{
	dec->code = 0U;
	dec->code_index = 0;
}

static inline bool cobs_decoder_needs_more_data(struct cobs_decoder *dec)
{
	return dec->code_index != 0;
}

int cobs_decoder_init(struct cobs_decoder *dec, cobs_stream_cb cb, void *user_data, uint32_t flags)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	__ASSERT_NO_MSG(dec != NULL);

	dec->cb = cb;
	dec->cb_user_data = user_data;
	dec->flags = flags;

	cobs_decoder_reset(dec);

	return 0;
}

int cobs_decoder_close(struct cobs_decoder *dec)
{
	int ret;

	__ASSERT_NO_MSG(dec != NULL);

	if ((dec->flags & COBS_FLAG_TRAILING_DELIMITER) != 0U) {
		/* Every frame must be terminated by a delimiter */
		ret = dec->code != 0U ? -EINVAL : 0;
	} else {
		/* The stream is a single frame of at least one complete group */
		ret = (dec->code == 0U || cobs_decoder_needs_more_data(dec)) ? -EINVAL : 0;
	}

	cobs_decoder_reset(dec);

	return ret;
}

int cobs_decoder_write(struct cobs_decoder *dec, const uint8_t *buf, size_t len)
{
	uint8_t sentinel;
	int ret;

	__ASSERT_NO_MSG(dec != NULL);
	__ASSERT_NO_MSG(len <= INT_MAX);

	sentinel = COBS_FLAG_CUSTOM_DELIMITER(dec->flags);

	for (size_t i = 0; i < len; ++i) {
		uint8_t data = buf[i] ^ sentinel;

		if (data == 0x00) {
			if ((dec->flags & COBS_FLAG_TRAILING_DELIMITER) == 0U ||
			    cobs_decoder_needs_more_data(dec)) {
				/* Decoder shouldn't get delimiters or unexpected end of data */
				cobs_decoder_reset(dec);
				return -EINVAL;
			}

			if (dec->code == 0U) {
				/* No frame started, ignore consecutive delimiters */
				continue;
			}

			/* Notify frame delimiter was seen */
			ret = dec->cb(NULL, 0, dec->cb_user_data);
			if (ret < 0) {
				cobs_decoder_reset(dec);
				return ret;
			}

			/* Reset state */
			cobs_decoder_reset(dec);

			if (ret > 0) {
				/* Callback requested to stop at the frame end */
				return i + 1;
			}

			continue;
		}

		if (dec->code_index > 0) {
			ret = dec->cb(&data, 1, dec->cb_user_data);
			if (ret < 0) {
				cobs_decoder_reset(dec);
				return ret;
			}

			dec->code_index--;
			continue;
		}

		dec->code_index = data;

		if (dec->code != 0U && dec->code != 0xff) {
			/* Group finished, output zero byte */
			data = 0x00;

			ret = dec->cb(&data, 1, dec->cb_user_data);
			if (ret < 0) {
				cobs_decoder_reset(dec);
				return ret;
			}
		}

		dec->code = dec->code_index;
		dec->code_index--;
	}

	return len;
}
