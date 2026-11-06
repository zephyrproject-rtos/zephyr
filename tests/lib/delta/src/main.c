/*
 * Copyright (c) 2026 Clovis Corde
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/delta/delta.h>
#include <zephyr/delta/backend_bsdiffhs.h>
#include <zephyr/ztest.h>

#include "vectors.inc"

/* RAM-backed delta_io harness driving the backend in-process. */
struct ram_ctx {
	const uint8_t *source;
	size_t source_len;
	const uint8_t *patch;
	size_t patch_len;
	uint8_t *target;
	size_t target_cap;
	size_t target_len;

	/* If >= 0, fail that source read (0-based) with -EIO. */
	int source_read_fail_after_n;
	int source_read_count;

	/* Returned by ram_finalize; counts its invocations. */
	int finalize_ret;
	int finalize_count;
};

static int ram_read_source(void *vctx, size_t off, void *buf, size_t len)
{
	struct ram_ctx *ctx = vctx;

	ctx->source_read_count++;
	if (ctx->source_read_fail_after_n >= 0 &&
	    ctx->source_read_count > ctx->source_read_fail_after_n) {
		return -EIO;
	}
	if (off + len > ctx->source_len) {
		return -ERANGE;
	}
	memcpy(buf, ctx->source + off, len);
	return 0;
}

static int ram_read_patch(void *vctx, size_t off, void *buf, size_t len)
{
	struct ram_ctx *ctx = vctx;

	if (off + len > ctx->patch_len) {
		return -ERANGE;
	}
	memcpy(buf, ctx->patch + off, len);
	return 0;
}

static int ram_write_target(void *vctx, size_t off, const void *buf, size_t len)
{
	struct ram_ctx *ctx = vctx;

	if (off != ctx->target_len) {
		return -ESPIPE;
	}
	if (off + len > ctx->target_cap) {
		return -ENOSPC;
	}
	memcpy(ctx->target + off, buf, len);
	ctx->target_len += len;
	return 0;
}

static int ram_finalize(void *vctx)
{
	struct ram_ctx *ctx = vctx;

	ctx->finalize_count++;
	return ctx->finalize_ret;
}

#define WORK_BUF_SZ   4096
#define TARGET_BUF_SZ 1024

static uint8_t work_buf[WORK_BUF_SZ];
static uint8_t target_buf[TARGET_BUF_SZ];

static void prep_ctx(struct ram_ctx *ctx, const uint8_t *src, size_t src_len, const uint8_t *patch,
		     size_t patch_len)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->source = src;
	ctx->source_len = src_len;
	ctx->patch = patch;
	ctx->patch_len = patch_len;
	ctx->target = target_buf;
	ctx->target_cap = sizeof(target_buf);
	ctx->source_read_fail_after_n = -1;
	memset(target_buf, 0xCC, sizeof(target_buf));
}

static struct delta_io make_io(struct ram_ctx *ctx)
{
	return (struct delta_io){
		.read_source = ram_read_source,
		.read_patch = ram_read_patch,
		.write_target = ram_write_target,
		.ctx = ctx,
	};
}

static struct delta_config make_cfg(const struct ram_ctx *ctx, size_t max_target)
{
	return (struct delta_config){
		.work_buf = work_buf,
		.work_sz = sizeof(work_buf),
		.source_size = ctx->source_len,
		.patch_size = ctx->patch_len,
		.max_target_size = max_target,
	};
}

static void run_positive(const struct delta_test_vector *v)
{
	struct ram_ctx ctx;

	prep_ctx(&ctx, v->src, v->src_len, v->patch, v->patch_len);
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, 0, "vector '%s' returned %d", v->name, ret);
	zassert_equal(ctx.target_len, v->dst_len, "vector '%s' wrote %zu bytes, expected %zu",
		      v->name, ctx.target_len, v->dst_len);
	zassert_mem_equal(target_buf, v->dst, v->dst_len, "vector '%s' target mismatch", v->name);
}

ZTEST_SUITE(delta, NULL, NULL, NULL, NULL, NULL);

ZTEST(delta, test_positive_identity)
{
	run_positive(&delta_test_vectors[0]);
}

ZTEST(delta, test_positive_one_byte)
{
	run_positive(&delta_test_vectors[1]);
}

ZTEST(delta, test_positive_shrink)
{
	run_positive(&delta_test_vectors[2]);
}

ZTEST(delta, test_positive_grow)
{
	run_positive(&delta_test_vectors[3]);
}

ZTEST(delta, test_positive_scattered)
{
	run_positive(&delta_test_vectors[4]);
}

ZTEST(delta, test_positive_negative_seek)
{
	run_positive(&delta_test_vectors[5]);
}

ZTEST(delta, test_null_io)
{
	const struct delta_test_vector *v = &delta_test_vectors[0];
	struct ram_ctx ctx;

	prep_ctx(&ctx, v->src, v->src_len, v->patch, v->patch_len);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	int ret = delta_apply_patch(NULL, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -EINVAL, "got %d", ret);
}

ZTEST(delta, test_null_backend)
{
	const struct delta_test_vector *v = &delta_test_vectors[0];
	struct ram_ctx ctx;

	prep_ctx(&ctx, v->src, v->src_len, v->patch, v->patch_len);
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	int ret = delta_apply_patch(&io, NULL, &cfg);

	zassert_equal(ret, -EINVAL, "got %d", ret);
}

ZTEST(delta, test_null_cfg)
{
	const struct delta_test_vector *v = &delta_test_vectors[0];
	struct ram_ctx ctx;

	prep_ctx(&ctx, v->src, v->src_len, v->patch, v->patch_len);
	struct delta_io io = make_io(&ctx);

	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, NULL);

	zassert_equal(ret, -EINVAL, "got %d", ret);
}

ZTEST(delta, test_null_callback)
{
	const struct delta_test_vector *v = &delta_test_vectors[0];
	struct ram_ctx ctx;

	prep_ctx(&ctx, v->src, v->src_len, v->patch, v->patch_len);
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	io.read_source = NULL;
	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -EINVAL, "got %d", ret);
}

ZTEST(delta, test_zero_patch_size)
{
	const struct delta_test_vector *v = &delta_test_vectors[0];
	struct ram_ctx ctx;

	prep_ctx(&ctx, v->src, v->src_len, v->patch, v->patch_len);
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	cfg.patch_size = 0;
	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -EINVAL, "got %d", ret);
}

ZTEST(delta, test_zero_max_target_size)
{
	const struct delta_test_vector *v = &delta_test_vectors[0];
	struct ram_ctx ctx;

	prep_ctx(&ctx, v->src, v->src_len, v->patch, v->patch_len);
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	cfg.max_target_size = 0;
	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -EINVAL, "got %d", ret);
}

/* Build a synthetic 18-byte header inline. */
static void build_header(uint8_t *out, uint64_t target_size, uint8_t window_sz2,
			 uint8_t lookahead_sz2)
{
	memcpy(out, "BSDIFFHS", 8);
	for (int i = 0; i < 8; i++) {
		out[8 + i] = (uint8_t)((target_size >> (8 * i)) & 0xFF);
	}
	out[16] = window_sz2;
	out[17] = lookahead_sz2;
}

ZTEST(delta, test_bad_magic)
{
	uint8_t patch[18];

	build_header(patch, 16, 8, 4);
	memcpy(patch, "XXXXXXXX", 8);

	struct ram_ctx ctx;
	uint8_t dummy_src[16] = {0};

	prep_ctx(&ctx, dummy_src, sizeof(dummy_src), patch, sizeof(patch));
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -EINVAL, "got %d", ret);
}

ZTEST(delta, test_truncated_header)
{
	uint8_t patch[10] = {'B', 'S', 'D', 'I', 'F', 'F', 'H', 'S', 0, 0};
	struct ram_ctx ctx;
	uint8_t dummy_src[16] = {0};

	prep_ctx(&ctx, dummy_src, sizeof(dummy_src), patch, sizeof(patch));
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -EINVAL, "got %d", ret);
}

ZTEST(delta, test_truncated_streams)
{
	/* Valid 18-byte header, target_size=100, no body bytes. */
	uint8_t patch[18];

	build_header(patch, 100, 8, 4);

	struct ram_ctx ctx;
	uint8_t dummy_src[16] = {0};

	prep_ctx(&ctx, dummy_src, sizeof(dummy_src), patch, sizeof(patch));
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -EINVAL, "got %d", ret);
}

ZTEST(delta, test_bad_target_size_zero)
{
	uint8_t patch[18];

	build_header(patch, 0, 8, 4);

	struct ram_ctx ctx;
	uint8_t dummy_src[16] = {0};

	prep_ctx(&ctx, dummy_src, sizeof(dummy_src), patch, sizeof(patch));
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -EINVAL, "got %d", ret);
}

ZTEST(delta, test_target_exceeds_max)
{
	uint8_t patch[18];

	build_header(patch, 10000, 8, 4);

	struct ram_ctx ctx;
	uint8_t dummy_src[16] = {0};

	prep_ctx(&ctx, dummy_src, sizeof(dummy_src), patch, sizeof(patch));
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, 100);

	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -ENOSPC, "got %d", ret);
}

ZTEST(delta, test_bad_window_sz2)
{
	uint8_t patch[18];

	build_header(patch, 16, 3, 4); /* 3 is below the [4,15] range. */

	struct ram_ctx ctx;
	uint8_t dummy_src[16] = {0};

	prep_ctx(&ctx, dummy_src, sizeof(dummy_src), patch, sizeof(patch));
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -EINVAL, "got %d", ret);
}

ZTEST(delta, test_bad_lookahead_sz2)
{
	uint8_t patch[18];

	build_header(patch, 16, 8, 2); /* 2 is below the [3,14] range. */

	struct ram_ctx ctx;
	uint8_t dummy_src[16] = {0};

	prep_ctx(&ctx, dummy_src, sizeof(dummy_src), patch, sizeof(patch));
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -EINVAL, "got %d", ret);
}

/* These compile-time bounds only exist in static-alloc mode. */
#if !defined(CONFIG_HEATSHRINK_DYNAMIC_ALLOC)
ZTEST(delta, test_window_sz2_mismatch)
{
	uint8_t patch[18];

	/* Static window bits is pinned to 8; declare 9. */
	build_header(patch, 16, 9, 4);

	struct ram_ctx ctx;
	uint8_t dummy_src[16] = {0};

	prep_ctx(&ctx, dummy_src, sizeof(dummy_src), patch, sizeof(patch));
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -ENOTSUP, "got %d", ret);
}

ZTEST(delta, test_lookahead_sz2_mismatch)
{
	uint8_t patch[18];

	/* Static lookahead bits is pinned to 4; declare 5. */
	build_header(patch, 16, 8, 5);

	struct ram_ctx ctx;
	uint8_t dummy_src[16] = {0};

	prep_ctx(&ctx, dummy_src, sizeof(dummy_src), patch, sizeof(patch));
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -ENOTSUP, "got %d", ret);
}
#endif /* !CONFIG_HEATSHRINK_DYNAMIC_ALLOC */

ZTEST(delta, test_undersized_work_buf)
{
	const struct delta_test_vector *v = &delta_test_vectors[0];
	struct ram_ctx ctx;

	prep_ctx(&ctx, v->src, v->src_len, v->patch, v->patch_len);
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	cfg.work_sz = 10; /* below the backend's 16-byte minimum. */
	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -ENOMEM, "got %d", ret);
}

ZTEST(delta, test_null_work_buf)
{
	const struct delta_test_vector *v = &delta_test_vectors[0];
	struct ram_ctx ctx;

	prep_ctx(&ctx, v->src, v->src_len, v->patch, v->patch_len);
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	cfg.work_buf = NULL;
	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -ENOMEM, "got %d", ret);
}

ZTEST(delta, test_source_read_failure)
{
	/* The backend must propagate the callback's errno unchanged. */
	const struct delta_test_vector *v = &delta_test_vectors[0]; /* identity */
	struct ram_ctx ctx;

	prep_ctx(&ctx, v->src, v->src_len, v->patch, v->patch_len);
	ctx.source_read_fail_after_n = 0; /* fail the 1st source read */
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -EIO, "got %d (expected -EIO)", ret);
}

ZTEST(delta, test_finalize_called_on_success)
{
	const struct delta_test_vector *v = &delta_test_vectors[0];
	struct ram_ctx ctx;

	prep_ctx(&ctx, v->src, v->src_len, v->patch, v->patch_len);
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	io.finalize = ram_finalize;
	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, 0, "got %d", ret);
	zassert_equal(ctx.finalize_count, 1, "finalize called %d times", ctx.finalize_count);
}

ZTEST(delta, test_finalize_not_called_on_failure)
{
	uint8_t patch[18];

	build_header(patch, 16, 8, 4);
	memcpy(patch, "XXXXXXXX", 8);

	struct ram_ctx ctx;
	uint8_t dummy_src[16] = {0};

	prep_ctx(&ctx, dummy_src, sizeof(dummy_src), patch, sizeof(patch));
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	io.finalize = ram_finalize;
	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -EINVAL, "got %d", ret);
	zassert_equal(ctx.finalize_count, 0, "finalize called %d times", ctx.finalize_count);
}

ZTEST(delta, test_finalize_error_propagates)
{
	const struct delta_test_vector *v = &delta_test_vectors[0];
	struct ram_ctx ctx;

	prep_ctx(&ctx, v->src, v->src_len, v->patch, v->patch_len);
	struct delta_io io = make_io(&ctx);
	struct delta_config cfg = make_cfg(&ctx, sizeof(target_buf));

	io.finalize = ram_finalize;
	ctx.finalize_ret = -EROFS;
	int ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	zassert_equal(ret, -EROFS, "got %d (expected -EROFS)", ret);
	zassert_equal(ctx.finalize_count, 1, "finalize called %d times", ctx.finalize_count);
}
