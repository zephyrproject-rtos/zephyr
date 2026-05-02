/*
 * Copyright (c) 2023 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>
#include <DA1469xAB.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smartbond_entropy, CONFIG_ENTROPY_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_smartbond_trng

#define IRQN	 DT_INST_IRQN(0)
#define IRQ_PRIO DT_INST_IRQ(0, priority)

struct rng_pool {
	uint8_t first_alloc;
	uint8_t first_read;
	uint8_t last;
	uint8_t mask;
	uint8_t threshold;
	FLEXIBLE_ARRAY_DECLARE(uint8_t, buffer);
};

#define RNG_POOL_DEFINE(name, len) uint8_t name[sizeof(struct rng_pool) + (len)]

BUILD_ASSERT((CONFIG_ENTROPY_SMARTBOND_ISR_POOL_SIZE &
	      (CONFIG_ENTROPY_SMARTBOND_ISR_POOL_SIZE - 1)) == 0,
	     "The CONFIG_ENTROPY_SMARTBOND_ISR_POOL_SIZE must be a power of 2!");

BUILD_ASSERT((CONFIG_ENTROPY_SMARTBOND_THR_POOL_SIZE &
	      (CONFIG_ENTROPY_SMARTBOND_THR_POOL_SIZE - 1)) == 0,
	     "The CONFIG_ENTROPY_SMARTBOND_THR_POOL_SIZE must be a power of 2!");

struct entropy_smartbond_dev_data {
	struct k_sem sem_lock;
	struct k_sem sem_sync;

	RNG_POOL_DEFINE(isr, CONFIG_ENTROPY_SMARTBOND_ISR_POOL_SIZE);
	RNG_POOL_DEFINE(thr, CONFIG_ENTROPY_SMARTBOND_THR_POOL_SIZE);
};

static struct entropy_smartbond_dev_data entropy_smartbond_data;

/* TRNG FIFO definitions are not in DA1469x.h */
#define DA1469X_TRNG_FIFO_SIZE (32 * sizeof(uint32_t))
#define DA1469X_TRNG_FIFO_ADDR (0x30050000UL)

#define FIFO_COUNT_MASK                                                                            \
	(TRNG_TRNG_FIFOLVL_REG_TRNG_FIFOFULL_Msk | TRNG_TRNG_FIFOLVL_REG_TRNG_FIFOLVL_Msk)

static inline void entropy_smartbond_pm_policy_state_lock_get(void)
{
#if defined(CONFIG_PM_DEVICE)
	/*
	 * Prevent the SoC from etering the normal sleep state as PDC does not support
	 * waking up the application core following TRNG events.
	 */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif
}

static inline void entropy_smartbond_pm_policy_state_lock_put(void)
{
#if defined(CONFIG_PM_DEVICE)
	/* Allow the SoC to enter the nornmal sleep state once TRNG is inactive */
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif
}

static void trng_enable(bool enable)
{
	unsigned int key;

	key = irq_lock();
	if (enable) {
		CRG_TOP->CLK_AMBA_REG |= CRG_TOP_CLK_AMBA_REG_TRNG_CLK_ENABLE_Msk;
		TRNG->TRNG_CTRL_REG = TRNG_TRNG_CTRL_REG_TRNG_ENABLE_Msk;

		/*
		 * Sleep is not allowed as long as the ISR and thread SW FIFOs
		 * are being filled with random numbers.
		 */
		entropy_smartbond_pm_policy_state_lock_get();
	} else {
		CRG_TOP->CLK_AMBA_REG &= ~CRG_TOP_CLK_AMBA_REG_TRNG_CLK_ENABLE_Msk;
		TRNG->TRNG_CTRL_REG = 0;
		NVIC_ClearPendingIRQ(IRQN);

		entropy_smartbond_pm_policy_state_lock_put();
	}
	irq_unlock(key);
}

static int trng_available(void)
{
	return TRNG->TRNG_FIFOLVL_REG & FIFO_COUNT_MASK;
}

static inline uint32_t trng_fifo_read(void)
{
	return *(uint32_t *)DA1469X_TRNG_FIFO_ADDR;
}

static int random_word_get(uint8_t buf[4])
{
	uint32_t word = 0;
	int retval = -EAGAIN;
	unsigned int key;

	key = irq_lock();

	if (trng_available()) {
		word = trng_fifo_read();
		retval = 0;
	}

	irq_unlock(key);

	buf[0] = (uint8_t)word;
	buf[1] = (uint8_t)(word >> 8);
	buf[2] = (uint8_t)(word >> 16);
	buf[3] = (uint8_t)(word >> 24);

	return retval;
}

static uint16_t rng_pool_get(struct rng_pool *rngp, uint8_t *buf, uint16_t len)
{
	uint32_t last = rngp->last;
	uint32_t mask = rngp->mask;
	uint8_t *dst = buf;
	uint32_t first, available;
	uint32_t other_read_in_progress;
	unsigned int key;

	key = irq_lock();
	first = rngp->first_alloc;

	/*
	 * The other_read_in_progress is non-zero if rngp->first_read != first,
	 * which means that lower-priority code (which was interrupted by this
	 * call) already allocated area for read.
	 */
	other_read_in_progress = (rngp->first_read ^ first);

	available = (last - first) & mask;
	if (available < len) {
		len = available;
	}

	/*
	 * Move alloc index forward to signal, that part of the buffer is
	 * now reserved for this call.
	 */
	rngp->first_alloc = (first + len) & mask;
	irq_unlock(key);

	while (likely(len--)) {
		*dst++ = rngp->buffer[first];
		first = (first + 1) & mask;
	}

	/*
	 * If this call is the last one accessing the pool, move read index
	 * to signal that all allocated regions are now read and could be
	 * overwritten.
	 */
	if (likely(!other_read_in_progress)) {
		key = irq_lock();
		rngp->first_read = rngp->first_alloc;
		irq_unlock(key);
	}

	len = dst - buf;
	available = available - len;
	if (available <= rngp->threshold) {
		trng_enable(true);
	}

	return len;
}

static int rng_pool_put(struct rng_pool *rngp, uint8_t byte)
{
	uint8_t first = rngp->first_read;
	uint8_t last = rngp->last;
	uint8_t mask = rngp->mask;

	/* Signal error if the pool is full. */
	if (((last - first) & mask) == mask) {
		return -ENOBUFS;
	}

	rngp->buffer[last] = byte;
	rngp->last = (last + 1) & mask;

	return 0;
}

static const uint8_t *rng_pool_put_bytes(struct rng_pool *rngp, const uint8_t *bytes,
					 const uint8_t *limit)
{
	unsigned int key;

	key = irq_lock();
	for (; bytes < limit; ++bytes) {
		if (rng_pool_put(rngp, *bytes) < 0) {
			break;
		}
	}
	irq_unlock(key);

	return bytes;
}

static void rng_pool_init(struct rng_pool *rngp, uint16_t size, uint8_t threshold)
{
	rngp->first_alloc = 0U;
	rngp->first_read = 0U;
	rngp->last = 0U;
	rngp->mask = size - 1;
	rngp->threshold = threshold;
}

static void smartbond_trng_isr(const void *arg)
{
	uint8_t word[4];
	const uint8_t *const limit = word + 4;
	const uint8_t *ptr;
	bool thread_signaled = false;

	ARG_UNUSED(arg);

	while (true) {
		if (random_word_get(word) < 0) {
			/* Nothing in FIFO -> nothing to do */
			break;
		}
		ptr = word;

		/* Put bytes in ISR FIFO first */
		ptr = rng_pool_put_bytes((struct rng_pool *)(entropy_smartbond_data.isr), ptr,
					 limit);
		if (ptr < limit) {
			/* Put leftovers in thread FIFO */
			if (!thread_signaled) {
				thread_signaled = true;
				k_sem_give(&entropy_smartbond_data.sem_sync);
			}
			ptr = rng_pool_put_bytes((struct rng_pool *)(entropy_smartbond_data.thr),
						 ptr, limit);
		}
		/* Bytes did not fit in isr nor thread FIFO, disable TRNG for now */
		if (ptr < limit) {
			trng_enable(false);
			break;
		}
	}
}

static int entropy_smartbond_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	ARG_UNUSED(dev);
	/* Check if this API is called on correct driver instance. */
	__ASSERT_NO_MSG(&entropy_smartbond_data == dev->data);

	while (len) {
		uint16_t bytes;

		k_sem_take(&entropy_smartbond_data.sem_lock, K_FOREVER);
		bytes = rng_pool_get((struct rng_pool *)(entropy_smartbond_data.thr), buf, len);
		k_sem_give(&entropy_smartbond_data.sem_lock);

		if (bytes == 0U) {
			/* Pool is empty: Sleep until next interrupt. */
			k_sem_take(&entropy_smartbond_data.sem_sync, K_FOREVER);
			continue;
		}

		len -= bytes;
		buf += bytes;
	}

	return 0;
}

static int entropy_smartbond_get_entropy_isr(const struct device *dev, uint8_t *buf, uint16_t len,
					     uint32_t flags)
{
	ARG_UNUSED(dev);
	uint16_t cnt = len;

	/* Check if this API is called on correct driver instance. */
	__ASSERT_NO_MSG(&entropy_smartbond_data == dev->data);

	if (likely((flags & ENTROPY_BUSYWAIT) == 0U)) {
		return rng_pool_get((struct rng_pool *)(entropy_smartbond_data.isr), buf, len);
	}

	if (len) {
		unsigned int key;
		int irq_enabled;

		key = irq_lock();
		irq_enabled = irq_is_enabled(IRQN);
		irq_disable(IRQN);
		irq_unlock(key);

		trng_enable(true);

		/* Clear NVIC pending bit. This ensures that a subsequent
		 * RNG event will set the Cortex-M single-bit event register
		 * to 1 (the bit is set when NVIC pending IRQ status is
		 * changed from 0 to 1)
		 */
		NVIC_ClearPendingIRQ(IRQN);

		do {
			uint8_t bytes[4];
			const uint8_t *ptr = bytes;
			const uint8_t *const limit = bytes + 4;

			while (!trng_available()) {
				/*
				 * To guarantee waking up from the event, the
				 * SEV-On-Pend feature must be enabled (enabled
				 * during ARCH initialization).
				 *
				 * DSB is recommended by spec before WFE (to
				 * guarantee completion of memory transactions)
				 */
				barrier_dsync_fence_full();
				__WFE();
				__SEV();
				__WFE();
			}

			NVIC_ClearPendingIRQ(IRQN);
			if (random_word_get(bytes) != 0) {
				continue;
			}

			while (ptr < limit && len) {
				buf[--len] = *ptr++;
			}
			/* Store remaining data for later use */
			if (unlikely(ptr < limit)) {
				rng_pool_put_bytes((struct rng_pool *)(entropy_smartbond_data.isr),
						   ptr, limit);
			}
		} while (len);

		if (irq_enabled) {
			irq_enable(IRQN);
		}
	}

	return cnt;
}

#if defined(CONFIG_PM_DEVICE)
static int entropy_smartbond_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/*
		 * No need to turn on TRNG. It should be done when we the space in the FIFOs
		 * are below the defined ISR and thread FIFO's thresholds.
		 *
		 * \sa CONFIG_ENTROPY_SMARTBOND_THR_THRESHOLD
		 * \sa CONFIG_ENTROPY_SMARTBOND_ISR_THRESHOLD
		 *
		 */
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* At this point TRNG should be disabled; no need to turn it off. */
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif

static DEVICE_API(entropy, entropy_smartbond_api_funcs) = {
	.get_entropy = entropy_smartbond_get_entropy,
	.get_entropy_isr = entropy_smartbond_get_entropy_isr};

static int entropy_smartbond_init(const struct device *dev)
{
	/* Check if this API is called on correct driver instance. */
	__ASSERT_NO_MSG(&entropy_smartbond_data == dev->data);

	/* Locking semaphore initialized to 1 (unlocked) */
	k_sem_init(&entropy_smartbond_data.sem_lock, 1, 1);

	/* Syncing semaphore */
	k_sem_init(&entropy_smartbond_data.sem_sync, 0, 1);

	rng_pool_init((struct rng_pool *)(entropy_smartbond_data.thr),
		      CONFIG_ENTROPY_SMARTBOND_THR_POOL_SIZE,
		      CONFIG_ENTROPY_SMARTBOND_THR_THRESHOLD);
	rng_pool_init((struct rng_pool *)(entropy_smartbond_data.isr),
		      CONFIG_ENTROPY_SMARTBOND_ISR_POOL_SIZE,
		      CONFIG_ENTROPY_SMARTBOND_ISR_THRESHOLD);

	IRQ_CONNECT(IRQN, IRQ_PRIO, smartbond_trng_isr, &entropy_smartbond_data, 0);
	irq_enable(IRQN);

	trng_enable(true);

	return 0;
}

PM_DEVICE_DT_INST_DEFINE(0, entropy_smartbond_pm_action);

DEVICE_DT_INST_DEFINE(0, entropy_smartbond_init, PM_DEVICE_DT_INST_GET(0),
			&entropy_smartbond_data, NULL, PRE_KERNEL_1,
			CONFIG_ENTROPY_INIT_PRIORITY, &entropy_smartbond_api_funcs);
