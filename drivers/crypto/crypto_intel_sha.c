/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <errno.h>
#include <zephyr/crypto/crypto.h>
#include "crypto_intel_sha_priv.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(SHA);

#define DT_DRV_COMPAT intel_adsp_sha

static struct sha_session sha_sessions[SHA_MAX_SESSIONS];

static int intel_sha_get_unused_session_idx(void)
{
	int i;

	for (i = 0; i < SHA_MAX_SESSIONS; i++) {
		if (!sha_sessions[i].in_use) {
			sha_sessions[i].in_use = true;
			return i;
		}
	}
	return -1;
}

static int intel_sha_set_ctl_enable(struct sha_container *sha, int status)
{
	/* wait until not busy when turning off */
	if (status == 0 && sha->dfsha->shactl.part.en == 1) {
		while (sha->dfsha->shasts.part.busy) {
		}
	}

	sha->dfsha->shactl.part.en = status;
	return 0;
}

static int intel_sha_set_resume_length_dw0(struct sha_container *sha, uint32_t lower_length)
{
	int err = -EINVAL;

	if (IS_ALIGNED(lower_length, SHA_REQUIRED_BLOCK_ALIGNMENT)) {
		sha->dfsha->sharldw0.full = lower_length;
		err = 0;
	}

	return err;
}

static int intel_sha_set_resume_length_dw1(struct sha_container *sha, uint32_t upper_length)
{
	sha->dfsha->sharldw1.full = upper_length;
	return 0;
}

static int intel_sha_regs_cpy(void *dst, const void *src, size_t len)
{
	uint32_t counter;
	int err = -EINVAL;

	if ((IS_ALIGNED(len, sizeof(uint32_t))) && (IS_ALIGNED(dst, sizeof(uint32_t))) &&
	    (IS_ALIGNED(src, sizeof(uint32_t)))) {
		len /= sizeof(uint32_t);
		for (counter = 0; counter != len; ++counter) {
			((uint32_t *)dst)[counter] = ((uint32_t *)src)[counter];
		}

		err = 0;
	}

	return err;
}

/* ! Perform SHA computation over requested region. */
static int intel_sha_device_run(const struct device *dev, const void *buf_in, size_t buf_in_size,
				size_t max_buff_len, uint32_t state)
{
	int err;
	struct sha_container *const self = dev->data;
	union sha_state state_u = { .full = state };
	/* align to OWORD */
	const size_t aligned_buff_size = ROUND_UP(buf_in_size, 0x10);

	err = intel_sha_set_ctl_enable(self, 0);
	if (err) {
		return err;
	}

	/* set processing element disable */
	self->dfsha->pibcs.part.peen = 0;
	/* set pib base addr */
	self->dfsha->pibba.full = (uint32_t)buf_in;

	if (max_buff_len < aligned_buff_size) {
		return -EINVAL;
	}

	self->dfsha->pibs.full = aligned_buff_size;
	/* enable interrupt */
	self->dfsha->pibcs.part.bscie = 1;
	self->dfsha->pibcs.part.teie = 0;
	/* set processing element enable */
	self->dfsha->pibcs.part.peen = 1;

	if (self->dfsha->shactl.part.en) {
		return -EINVAL; /* already enabled */
	}

	self->dfsha->shactl.part.hrsm = state_u.part.hrsm;

	/* set initial values if resuming */
	if (state_u.part.hrsm) {
		err = intel_sha_set_resume_length_dw0(self, self->dfsha->shaaldw0.full);
		if (err) {
			return err;
		}
		err = intel_sha_set_resume_length_dw1(self, self->dfsha->shaaldw1.full);
		if (err) {
			return err;
		}
		err = intel_sha_regs_cpy((void *)self->dfsha->initial_vector,
					 (void *)self->dfsha->sha_result,
					 sizeof(self->dfsha->initial_vector));
		if (err) {
			return err;
		}
	}

	/* set ctl hash first middle */
	if (self->dfsha->shactl.part.en) {
		return -EINVAL; /* already enabled */
	}

	self->dfsha->shactl.part.hfm = state_u.part.state;

	/* increment pointer */
	self->dfsha->pibfpi.full = buf_in_size;

	err = intel_sha_set_ctl_enable(self, 1);
	if (err) {
		return err;
	}

	err = intel_sha_set_ctl_enable(self, 0);

	return err;
}

static int intel_sha_copy_hash(struct sha_container *const self, void *dst, size_t len)
{
	/* NOTE: generated hash value should be read from the end */

	int err = -EINVAL;
	uint32_t counter = 0;
	uint32_t last_idx = 0;

	if ((IS_ALIGNED(len, sizeof(uint32_t))) && (IS_ALIGNED(dst, sizeof(uint32_t)))) {
		len /= sizeof(uint32_t);
		counter = 0;
		/* The index of a last element in the sha result buffer. */
		last_idx = (sizeof(self->dfsha->sha_result) / sizeof(uint32_t)) - 1;

		for (counter = 0; counter != len; counter++) {
			((uint32_t *)dst)[counter] =
				((uint32_t *)self->dfsha->sha_result)[last_idx - counter];
		}

		err = 0;
	}

	return err;
}

static int intel_sha_device_get_hash(const struct device *dev, void *buf_out, size_t buf_out_size)
{
	int err;
	struct sha_container *const self = dev->data;

	if (buf_out == NULL) {
		return -EINVAL;
	}
	/* wait until not busy */
	while (self->dfsha->shasts.part.busy) {
	}

	err = intel_sha_copy_hash(self, buf_out, buf_out_size);

	return err;
}

static int intel_sha_compute(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	int ret;
	struct sha_container *self = (struct sha_container *const)(ctx->device)->data;
	struct sha_session *session = (struct sha_session *)ctx->drv_sessn_state;
	size_t frag_length;
	size_t output_size;
	uint32_t *hash_int_ptr = (uint32_t *)(pkt->out_buf);

	/* set algo */
	self->dfsha->shactl.full = 0x0;
	self->dfsha->shactl.part.algo = session->algo;

	/* restore ctx */
	self->dfsha->shaaldw0 = session->sha_ctx.shaaldw0;
	self->dfsha->shaaldw1 = session->sha_ctx.shaaldw1;

	ret = intel_sha_regs_cpy((void *)self->dfsha->initial_vector,
				 (void *)session->sha_ctx.initial_vector,
				 sizeof(self->dfsha->initial_vector));
	if (ret) {
		return ret;
	}

	ret = intel_sha_regs_cpy((void *)self->dfsha->sha_result,
				 (void *)session->sha_ctx.sha_result,
				 sizeof(self->dfsha->sha_result));

	if (ret) {
		return ret;
	}

	/* compute hash */
	do {
		frag_length = pkt->in_len > SHA_API_MAX_FRAG_LEN ?
					     SHA_API_MAX_FRAG_LEN :
						   pkt->in_len;

		if ((frag_length == pkt->in_len) && finish) {
			session->state.part.state = SHA_LAST;
		}

		ret = intel_sha_device_run(ctx->device, pkt->in_buf, frag_length, frag_length,
					   session->state.full);
		if (ret) {
			return ret;
		}

		/* set state for next iteration */
		session->state.part.hrsm = SHA_HRSM_ENABLE;
		session->state.part.state = SHA_MIDLE;

		pkt->in_len -= frag_length;
		pkt->in_buf += frag_length;
	} while (pkt->in_len > 0);

	if (finish) {
		switch (self->dfsha->shactl.part.algo) {
		case CRYPTO_HASH_ALGO_SHA224:
			output_size = SHA224_ALGORITHM_HASH_SIZEOF;
			break;
		case CRYPTO_HASH_ALGO_SHA256:
			output_size = SHA256_ALGORITHM_HASH_SIZEOF;
			break;
		case CRYPTO_HASH_ALGO_SHA384:
			output_size = SHA384_ALGORITHM_HASH_SIZEOF;
			break;
		case CRYPTO_HASH_ALGO_SHA512:
			output_size = SHA512_ALGORITHM_HASH_SIZEOF;
			break;
		default:
			return -ENOTSUP;
		}
		ret = intel_sha_device_get_hash(ctx->device, pkt->out_buf, output_size);

		if (ret) {
			return ret;
		}

		/* Fix byte ordering to match common hash representation. */
		for (size_t i = 0; i != output_size / sizeof(uint32_t); i++) {
			hash_int_ptr[i] = BYTE_SWAP32(hash_int_ptr[i]);
		}
	}
	return ret;
}

static int intel_sha_device_set_hash_type(const struct device *dev, struct hash_ctx *ctx,
					  enum hash_algo algo)
{
	int ctx_idx;
	struct sha_container *self = (struct sha_container *const)(dev)->data;

	ctx_idx = intel_sha_get_unused_session_idx();

	if (ctx_idx < 0) {
		LOG_ERR("All sessions in use!");
		return -ENOSPC;
	}
	ctx->drv_sessn_state = &sha_sessions[ctx_idx];

	/* set processing element enable */
	self->dfsha->pibcs.part.peen = 0;

	/* populate sha session data */
	sha_sessions[ctx_idx].state.part.state = SHA_FIRST;
	sha_sessions[ctx_idx].state.part.hrsm = SHA_HRSM_DISABLE;
	sha_sessions[ctx_idx].algo = algo;

	ctx->hash_hndlr = intel_sha_compute;
	return 0;
}

static int intel_sha_device_free(const struct device *dev, struct hash_ctx *ctx)
{
	struct sha_container *self = (struct sha_container *const)(dev)->data;
	struct sha_session *session = (struct sha_session *)ctx->drv_sessn_state;

	(void)memset((void *)self->dfsha, 0, sizeof(struct sha_hw_regs));
	(void)memset(&session->sha_ctx, 0, sizeof(struct sha_context));
	(void)memset(&session->state, 0, sizeof(union sha_state));
	session->in_use = 0;
	session->algo = 0;
	return 0;
}

static int intel_sha_device_hw_caps(const struct device *dev)
{
	return (CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS);
}

static DEVICE_API(crypto, hash_enc_funcs) = {
	.hash_begin_session = intel_sha_device_set_hash_type,
	.hash_free_session = intel_sha_device_free,
	.hash_async_callback_set = NULL,
	.query_hw_caps = intel_sha_device_hw_caps,
};

#define INTEL_SHA_DEVICE_INIT(inst)                                               \
static struct sha_container sha_data_##inst  = {                                  \
	.dfsha = (volatile struct sha_hw_regs *)DT_INST_REG_ADDR_BY_IDX(inst, 0)  \
};                                                                                \
DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &sha_data_##inst, NULL,                   \
	POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY, (void *)&hash_enc_funcs);

DT_INST_FOREACH_STATUS_OKAY(INTEL_SHA_DEVICE_INIT)
