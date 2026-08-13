/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_psa_crypto_rng

#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/minmax.h>
#include <psa/crypto.h>

#ifdef CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR
#define ISR_BUFFER_SIZE		CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR_BUFFER_SIZE
#define ISR_REFILL_SIZE		CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR_REFILL_SIZE
#define ISR_REFILL_THRESHOLD	CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR_REFILL_THRESHOLD
#else
#define ISR_BUFFER_SIZE		1 /* Dummy value to keep compiler happy */
#define ISR_REFILL_SIZE		0 /* Dummy value to keep compiler happy */
#define ISR_REFILL_THRESHOLD	0 /* Dummy value to keep compiler happy */
#endif /* CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR */

#ifdef CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR_WORKQUEUE
#define ISR_WQ_STACK_SIZE	CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR_WQ_STACK_SIZE
#define ISR_WQ_PRIORITY		CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR_WQ_PRIO
#else
#define ISR_WQ_STACK_SIZE	0 /* Dummy value to keep compiler happy */
#define ISR_WQ_PRIORITY		0 /* Dummy value to keep compiler happy */
#endif /* CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR_WORKQUEUE */

LOG_MODULE_REGISTER(entropy_psa_crypto, CONFIG_ENTROPY_LOG_LEVEL);

struct entropy_psa_crypto_context {
	struct ring_buf isr_rbuf;
	struct k_spinlock isr_lock;
	struct k_work isr_refill_work;
	struct k_work_q *isr_wq;
};

/* Context used only when CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR is enabled */
static struct entropy_psa_crypto_context entropy_psa_crypto_ctx;
static uint8_t __noinit entropy_psa_crypto_isr_pool[ISR_BUFFER_SIZE];

/* Dedicated workqueue used only when CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR_WORKQUEUE is enabled */
static K_THREAD_STACK_DEFINE(entropy_psa_crypto_isr_wq_stack, ISR_WQ_STACK_SIZE);
static struct k_work_q entropy_psa_crypto_isr_wq;

/* API implementation: get_entropy */
static int entropy_psa_crypto_rng_get_entropy(const struct device *dev,
					      uint8_t *buffer, uint16_t length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	ARG_UNUSED(dev);

	status = psa_generate_random(buffer, length);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int __maybe_unused entropy_psa_crypto_rng_get_entropy_isr(const struct device *dev,
								 uint8_t *buffer, uint16_t length,
								 uint32_t flags)
{
	struct entropy_psa_crypto_context *ctx = &entropy_psa_crypto_ctx;
	k_spinlock_key_t key;
	uint32_t rand_size;
	bool refill;
	int ret;

	key = k_spin_lock(&ctx->isr_lock);

	rand_size = ring_buf_size_get(&ctx->isr_rbuf);

	if (rand_size >= length || likely((flags & ENTROPY_BUSYWAIT) == 0U)) {
		rand_size = ring_buf_get(&ctx->isr_rbuf, buffer, min(rand_size, length));
	}

	refill = (ring_buf_size_get(&ctx->isr_rbuf) <= ISR_REFILL_THRESHOLD);

	k_spin_unlock(&ctx->isr_lock, key);

	if (refill && ctx->isr_wq != NULL) {
		ret = k_work_submit_to_queue(ctx->isr_wq, &ctx->isr_refill_work);
		if (ret < 0 && ret != -ENODEV) {
			LOG_ERR("Failed to launch RNG pool refill: %d", ret);
		}
	}

	if (rand_size < length && unlikely((flags & ENTROPY_BUSYWAIT) != 0U)) {
		LOG_WRN_RATELIMIT("ISR random bytes underflow: %u/%u", rand_size, length);
		return -EBUSY;
	}

	return (int)rand_size;
}

static void entropy_psa_crypto_isr_refill_work_fn(struct k_work *work)
{
	struct entropy_psa_crypto_context *ctx = &entropy_psa_crypto_ctx;
	__maybe_unused size_t total = 0;
	k_spinlock_key_t key;
	bool done = false;
	uint32_t size;
	uint8_t *ptr;

	/* Get random byte per small chunks to lower latency to TF-M services */
	do {
		if (ISR_REFILL_SIZE == 0) {
			size = UINT32_MAX;
		} else {
			size = ISR_REFILL_SIZE;
		}

		key = k_spin_lock(&ctx->isr_lock);
		size = ring_buf_put_claim(&ctx->isr_rbuf, &ptr, size);
		k_spin_unlock(&ctx->isr_lock, key);

		if (psa_generate_random(ptr, size) != PSA_SUCCESS) {
			(void)ring_buf_put_finish(&ctx->isr_rbuf, 0);
			LOG_ERR("psa_generate_random() failed");
			break;
		}

		key = k_spin_lock(&ctx->isr_lock);

		if (unlikely(ring_buf_put_finish(&ctx->isr_rbuf, size) < 0)) {
			LOG_ERR("Failed to finish ring buffer update");
			done = true;
		} else {
			total += size;
			done = (ring_buf_space_get(&ctx->isr_rbuf) == 0);
		}

		k_spin_unlock(&ctx->isr_lock, key);
	} while (!done);

	LOG_DBG("Refilled %zu bytes", total);
}

static int __maybe_unused entropy_psa_crypto_init_post_kernel(void)
{
	struct entropy_psa_crypto_context *ctx = &entropy_psa_crypto_ctx;
	int ret;

	if (IS_ENABLED(CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR_WORKQUEUE)) {
		k_work_queue_init(&entropy_psa_crypto_isr_wq);
		k_work_queue_start(&entropy_psa_crypto_isr_wq,
				   ((k_thread_stack_t *)&entropy_psa_crypto_isr_wq_stack),
				   K_THREAD_STACK_SIZEOF(entropy_psa_crypto_isr_wq_stack),
				   ISR_WQ_PRIORITY, NULL);
		k_thread_name_set(entropy_psa_crypto_isr_wq.thread_id, "psa-rng-isr-pool");

		ctx->isr_wq = &entropy_psa_crypto_isr_wq;
	} else {
		ctx->isr_wq = &k_sys_work_q;
	}

	/* Refill the pool in case it was used prior workqueue was started */
	ret = k_work_submit_to_queue(ctx->isr_wq, &ctx->isr_refill_work);
	if (ret < 0) {
		LOG_ERR("Failed to launch RNG pool refill: %d", ret);
	}

	return ret;
}

#ifdef CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR
SYS_INIT(entropy_psa_crypto_init_post_kernel, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR */

static int entropy_psa_crypto_init_isr_pool(void)
{
	struct entropy_psa_crypto_context *ctx = &entropy_psa_crypto_ctx;
	k_spinlock_key_t key;
	uint32_t filled_size;

	ring_buf_init(&ctx->isr_rbuf, sizeof(entropy_psa_crypto_isr_pool),
		      entropy_psa_crypto_isr_pool);

	/* Start with a fully filled pool of random bytes */
	entropy_psa_crypto_isr_refill_work_fn(&ctx->isr_refill_work);

	key = k_spin_lock(&ctx->isr_lock);
	filled_size = ring_buf_size_get(&ctx->isr_rbuf);
	k_spin_unlock(&ctx->isr_lock, key);

	k_work_init(&ctx->isr_refill_work, entropy_psa_crypto_isr_refill_work_fn);

	if (filled_size < ISR_BUFFER_SIZE) {
		LOG_ERR("Failed to fully fill the ISR pool: %u/%u bytes",
			filled_size, ISR_BUFFER_SIZE);
		return -EIO;
	}

	return 0;
}

/* API implementation: PSA Crypto initialization */
static int entropy_psa_crypto_rng_init(const struct device *dev)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	ARG_UNUSED(dev);

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR)) {
		return entropy_psa_crypto_init_isr_pool();
	}

	return 0;
}

/* Entropy driver APIs structure */
static DEVICE_API(entropy, entropy_psa_crypto_rng_api) = {
	.get_entropy = entropy_psa_crypto_rng_get_entropy,
#ifdef CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR
	.get_entropy_isr = entropy_psa_crypto_rng_get_entropy_isr,
#endif /* CONFIG_ENTROPY_PSA_CRYPTO_RNG_ISR */
};

/* Entropy driver registration */
DEVICE_DT_INST_DEFINE(0, entropy_psa_crypto_rng_init, NULL, NULL, NULL,
		      PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		      &entropy_psa_crypto_rng_api);
