/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Exati Tecnologia Ltda.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <soc.h>
#include <hal/nrf_rng.h>
#include <zephyr/irq.h>

#define DT_DRV_COMPAT	nordic_nrf_rng

#define IRQN		DT_INST_IRQN(0)
#define IRQ_PRIO	DT_INST_IRQ(0, priority)

/*
 * The nRF5 RNG HW has several characteristics that need to be taken
 * into account by the driver to achieve energy efficient generation
 * of entropy.
 *
 * The RNG does not support continuously DMA'ing entropy into RAM,
 * values must be read out by the CPU byte-by-byte. But once started,
 * it will continue to generate bytes until stopped.
 *
 * The generation time for byte 0 after starting generation (with BIAS
 * correction) is:
 *
 * nRF51822 - 677us
 * nRF52810 - 248us
 * nRF52840 - 248us
 *
 * The generation time for byte N >= 1 after starting generation (with
 * BIAS correction) is:
 *
 * nRF51822 - 677us
 * nRF52810 - 120us
 * nRF52840 - 120us
 *
 * Due to the first byte in a stream of bytes being more costly on
 * some platforms a "water system" inspired algorithm is used to
 * amortize the cost of the first byte.
 *
 * The algorithm will delay generation of entropy until the amount of
 * bytes goes below THRESHOLD, at which point it will generate entropy
 * until the BUF_LEN limit is reached.
 *
 * The entropy level is checked at the end of every consumption of
 * entropy.
 *
 * The algorithm and HW together has these characteristics:
 *
 * Setting a low threshold will highly amortize the extra 120us cost
 * of the first byte on nRF52.
 *
 * Setting a high threshold will minimize the time spent waiting for
 * entropy.
 *
 * To minimize power consumption the threshold should either be set
 * low or high depending on the HFCLK-usage pattern of other
 * components.
 *
 * If the threshold is set close to the BUF_LEN, and the system
 * happens to anyway be using the HFCLK for several hundred us after
 * entropy is requested there will be no extra current-consumption for
 * keeping clocks running for entropy generation.
 *
 */

struct rng_pool {
	uint8_t first_alloc;
	uint8_t first_read;
	uint8_t last;
	uint8_t mask;
	uint8_t threshold;
	uint8_t buffer[0];
};

#define RNG_POOL_DEFINE(name, len) uint8_t name[sizeof(struct rng_pool) + (len)]

BUILD_ASSERT((CONFIG_ENTROPY_NRF5_ISR_POOL_SIZE &
	      (CONFIG_ENTROPY_NRF5_ISR_POOL_SIZE - 1)) == 0,
	     "The CONFIG_ENTROPY_NRF5_ISR_POOL_SIZE must be a power of 2!");

BUILD_ASSERT((CONFIG_ENTROPY_NRF5_THR_POOL_SIZE &
	      (CONFIG_ENTROPY_NRF5_THR_POOL_SIZE - 1)) == 0,
	     "The CONFIG_ENTROPY_NRF5_THR_POOL_SIZE must be a power of 2!");

struct entropy_nrf5_dev_data {
	struct k_sem sem_lock;
	struct k_sem sem_sync;

	RNG_POOL_DEFINE(isr, CONFIG_ENTROPY_NRF5_ISR_POOL_SIZE);
	RNG_POOL_DEFINE(thr, CONFIG_ENTROPY_NRF5_THR_POOL_SIZE);
};

static struct entropy_nrf5_dev_data entropy_nrf5_data;

static int random_byte_get(void)
{
	int retval = -EAGAIN;
	unsigned int key;

	key = irq_lock();

	if (nrf_rng_event_check(NRF_RNG, NRF_RNG_EVENT_VALRDY)) {
		retval = nrf_rng_random_value_get(NRF_RNG);
		nrf_rng_event_clear(NRF_RNG, NRF_RNG_EVENT_VALRDY);
	}

	irq_unlock(key);

	return retval;
}

static uint16_t rng_pool_get(struct rng_pool *rngp, uint8_t *buf, uint16_t len)
{
	uint32_t last  = rngp->last;
	uint32_t mask  = rngp->mask;
	uint8_t *dst   = buf;
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
		nrf_rng_task_trigger(NRF_RNG, NRF_RNG_TASK_START);
	}

	return len;
}

static int rng_pool_put(struct rng_pool *rngp, uint8_t byte)
{
	uint8_t first = rngp->first_read;
	uint8_t last  = rngp->last;
	uint8_t mask  = rngp->mask;

	/* Signal error if the pool is full. */
	if (((last - first) & mask) == mask) {
		return -ENOBUFS;
	}

	rngp->buffer[last] = byte;
	rngp->last = (last + 1) & mask;

	return 0;
}

static void rng_pool_init(struct rng_pool *rngp, uint16_t size, uint8_t threshold)
{
	rngp->first_alloc = 0U;
	rngp->first_read  = 0U;
	rngp->last	  = 0U;
	rngp->mask	  = size - 1;
	rngp->threshold	  = threshold;
}

static void isr(const void *arg)
{
	int byte, ret;

	ARG_UNUSED(arg);

	byte = random_byte_get();
	if (byte < 0) {
		return;
	}

	ret = rng_pool_put((struct rng_pool *)(entropy_nrf5_data.isr), byte);
	if (ret < 0) {
		ret = rng_pool_put((struct rng_pool *)(entropy_nrf5_data.thr),
				   byte);
		if (ret < 0) {
			nrf_rng_task_trigger(NRF_RNG, NRF_RNG_TASK_STOP);
		}

		k_sem_give(&entropy_nrf5_data.sem_sync);
	}
}

static int entropy_nrf5_get_entropy(const struct device *dev, uint8_t *buf,
				    uint16_t len)
{
	/* Check if this API is called on correct driver instance. */
	__ASSERT_NO_MSG(&entropy_nrf5_data == dev->data);

	while (len) {
		uint16_t bytes;

		k_sem_take(&entropy_nrf5_data.sem_lock, K_FOREVER);
		bytes = rng_pool_get((struct rng_pool *)(entropy_nrf5_data.thr),
				     buf, len);
		k_sem_give(&entropy_nrf5_data.sem_lock);

		if (bytes == 0U) {
			/* Pool is empty: Sleep until next interrupt. */
			k_sem_take(&entropy_nrf5_data.sem_sync, K_FOREVER);
			continue;
		}

		len -= bytes;
		buf += bytes;
	}

	return 0;
}

static int entropy_nrf5_get_entropy_isr(const struct device *dev,
					uint8_t *buf, uint16_t len,
					uint32_t flags)
{
	uint16_t cnt = len;

	/* Check if this API is called on correct driver instance. */
	__ASSERT_NO_MSG(&entropy_nrf5_data == dev->data);

	if (likely((flags & ENTROPY_BUSYWAIT) == 0U)) {
		return rng_pool_get((struct rng_pool *)(entropy_nrf5_data.isr),
				    buf, len);
	}

	if (len) {
		unsigned int key;
		int irq_enabled;

		key = irq_lock();
		irq_enabled = irq_is_enabled(IRQN);
		irq_disable(IRQN);
		irq_unlock(key);

		nrf_rng_event_clear(NRF_RNG, NRF_RNG_EVENT_VALRDY);
		nrf_rng_task_trigger(NRF_RNG, NRF_RNG_TASK_START);

		/* Clear NVIC pending bit. This ensures that a subsequent
		 * RNG event will set the Cortex-M single-bit event register
		 * to 1 (the bit is set when NVIC pending IRQ status is
		 * changed from 0 to 1)
		 */
		NVIC_ClearPendingIRQ(IRQN);

		do {
			int byte;

			while (!nrf_rng_event_check(NRF_RNG,
						    NRF_RNG_EVENT_VALRDY)) {
				k_cpu_atomic_idle(irq_lock());
			}

			byte = random_byte_get();
			NVIC_ClearPendingIRQ(IRQN);

			if (byte < 0) {
				continue;
			}

			buf[--len] = byte;
		} while (len);

		if (irq_enabled) {
			irq_enable(IRQN);
		}
	}

	return cnt;
}

static int entropy_nrf5_init(const struct device *dev);

static const struct entropy_driver_api entropy_nrf5_api_funcs = {
	.get_entropy = entropy_nrf5_get_entropy,
	.get_entropy_isr = entropy_nrf5_get_entropy_isr
};

DEVICE_DT_INST_DEFINE(0,
		    entropy_nrf5_init, NULL,
		    &entropy_nrf5_data, NULL,
		    PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		    &entropy_nrf5_api_funcs);

static int entropy_nrf5_init(const struct device *dev)
{
	/* Check if this API is called on correct driver instance. */
	__ASSERT_NO_MSG(&entropy_nrf5_data == dev->data);

	/* Locking semaphore initialized to 1 (unlocked) */
	k_sem_init(&entropy_nrf5_data.sem_lock, 1, 1);

	/* Synching semaphore */
	k_sem_init(&entropy_nrf5_data.sem_sync, 0, 1);

	rng_pool_init((struct rng_pool *)(entropy_nrf5_data.thr),
		      CONFIG_ENTROPY_NRF5_THR_POOL_SIZE,
		      CONFIG_ENTROPY_NRF5_THR_THRESHOLD);
	rng_pool_init((struct rng_pool *)(entropy_nrf5_data.isr),
		      CONFIG_ENTROPY_NRF5_ISR_POOL_SIZE,
		      CONFIG_ENTROPY_NRF5_ISR_THRESHOLD);

	/* Enable or disable bias correction */
	if (IS_ENABLED(CONFIG_ENTROPY_NRF5_BIAS_CORRECTION)) {
		nrf_rng_error_correction_enable(NRF_RNG);
	} else {
		nrf_rng_error_correction_disable(NRF_RNG);
	}

	nrf_rng_event_clear(NRF_RNG, NRF_RNG_EVENT_VALRDY);
	nrf_rng_int_enable(NRF_RNG, NRF_RNG_INT_VALRDY_MASK);
	nrf_rng_task_trigger(NRF_RNG, NRF_RNG_TASK_START);

	IRQ_CONNECT(IRQN, IRQ_PRIO, isr, &entropy_nrf5_data, 0);
	irq_enable(IRQN);

	return 0;
}
