/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>

#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/aud/mpipe_aud_gain.h>

NET_BUF_POOL_FIXED_DEFINE(gain_pool, 1, 8, sizeof(struct mpipe_buffer_meta), NULL);

static struct mpipe_aud_gain gain;

/* At 200% gain a loud sample must saturate, not wrap (the 32-bit overflow bug). */
ZTEST(mpipe_aud_gain, test_16bit_saturates_above_unity)
{
	int gain_val = 200;
	struct net_buf *buf;
	struct net_buf *out = NULL;
	int16_t *s;

	zassert_ok(mpipe_aud_gain_init(&gain, 1));
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&gain,
					       MPIPE_PROP_AUD_TRANSFORM_GAIN, &gain_val,
					       MPIPE_PROP_LIST_END));

	/* Select the 16-bit path; a full pipeline sets this during negotiation. */
	gain.bit_width = 16;

	buf = net_buf_alloc_len(&gain_pool, 8, K_NO_WAIT);
	zassert_not_null(buf);
	buf->len = 8;
	s = (int16_t *)buf->data;
	s[0] = 20000;  /* * 2.0 -> 40000, must clamp to INT16_MAX */
	s[1] = -20000; /* -> -40000, must clamp to INT16_MIN */
	s[2] = 100;    /* -> 200 */
	s[3] = 0;      /* -> 0 */
	mpipe_buffer_get_meta(buf)->bytes_used = 8;

	zassert_ok(gain.transform.sink_pad.chain_fn(&gain.transform.sink_pad, buf, &out));

	zassert_equal(s[0], INT16_MAX, "got %d, expected saturation (overflow bug wraps)", s[0]);
	zassert_equal(s[1], INT16_MIN, "got %d", s[1]);
	zassert_equal(s[2], 200, "got %d", s[2]);
	zassert_equal(s[3], 0, "got %d", s[3]);

	net_buf_unref(buf);
}

ZTEST_SUITE(mpipe_aud_gain, NULL, NULL, NULL, NULL, NULL);
