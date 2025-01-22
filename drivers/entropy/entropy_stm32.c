/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Exati Tecnologia Ltda.
 * Copyright (c) 2020 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/random/random.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <soc.h>
#include <zephyr/pm/policy.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_rng.h>
#include <stm32_ll_pka.h>
#include <stm32_ll_system.h>
#include <zephyr/sys/printk.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>
#include "stm32_hsem.h"

#include "entropy_stm32.h"

#if defined(RNG_CR_CONDRST)
#define STM32_CONDRST_SUPPORT
#endif

/*
 * This driver need to take into account all STM32 family:
 *  - simple rng without hardware fifo and no DMA.
 *  - Variable delay between two consecutive random numbers
 *    (depending on family and clock settings)
 *  - IRQ-less TRNG instances
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
 * For TRNG instances with no IRQ, a delayable work item is scheduled
 * on the system work queue and used to "simulate" device-generated
 * interrupts - this is done to reduce polling to a minimum.
 */

struct rng_pool {
	uint8_t first_alloc;
	uint8_t first_read;
	uint8_t last;
	uint8_t mask;
	uint8_t threshold;
	FLEXIBLE_ARRAY_DECLARE(uint8_t, buffer);
};

#define RNG_POOL_DEFINE(name, len) uint8_t name[sizeof(struct rng_pool) + (len)]

BUILD_ASSERT((CONFIG_ENTROPY_STM32_ISR_POOL_SIZE &
	      (CONFIG_ENTROPY_STM32_ISR_POOL_SIZE - 1)) == 0,
	     "The CONFIG_ENTROPY_STM32_ISR_POOL_SIZE must be a power of 2!");

BUILD_ASSERT((CONFIG_ENTROPY_STM32_THR_POOL_SIZE &
	      (CONFIG_ENTROPY_STM32_THR_POOL_SIZE - 1)) == 0,
	     "The CONFIG_ENTROPY_STM32_THR_POOL_SIZE must be a power of 2!");

/**
 * RM0505 §14.4 "TRNG functional description":
 *  To use the TRNG peripheral the system clock frequency must be
 *  at least 32 MHz. See also: §6.2.2 "Peripheral clock details".
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_SOC_STM32WB09XX) ||
		CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC >= (32 * 1000 * 1000),
	"STM32WB09: TRNG requires system clock frequency >= 32MHz");

struct entropy_stm32_rng_dev_cfg {
	struct stm32_pclken *pclken;
};

struct entropy_stm32_rng_dev_data {
	RNG_TypeDef *rng;
	const struct device *clock;
	struct k_sem sem_lock;
	struct k_sem sem_sync;
	struct k_work filling_work;
#if IRQLESS_TRNG
	/* work item that polls TRNG to refill pools */
	struct k_work_delayable trng_poll_work;
#endif /* IRQLESS_TRNG */
	bool filling_pools;

	RNG_POOL_DEFINE(isr, CONFIG_ENTROPY_STM32_ISR_POOL_SIZE);
	RNG_POOL_DEFINE(thr, CONFIG_ENTROPY_STM32_THR_POOL_SIZE);
};

static struct stm32_pclken pclken_rng[] = STM32_DT_INST_CLOCKS(0);

static struct entropy_stm32_rng_dev_cfg entropy_stm32_rng_config = {
	.pclken	= pclken_rng
};

static struct entropy_stm32_rng_dev_data entropy_stm32_rng_data = {
	.rng = (RNG_TypeDef *)DT_INST_REG_ADDR(0),
};

static int entropy_stm32_suspend(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct entropy_stm32_rng_dev_data *dev_data = dev->data;
	const struct entropy_stm32_rng_dev_cfg *dev_cfg = dev->config;
	RNG_TypeDef *rng = dev_data->rng;
	int res;

#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_STM32H7_DUAL_CORE)
	/* Prevent concurrent access with PM */
	z_stm32_hsem_lock(CFG_HW_RNG_SEMID, HSEM_LOCK_WAIT_FOREVER);
#endif /* CONFIG_SOC_SERIES_STM32WBX || CONFIG_STM32H7_DUAL_CORE */
	LL_RNG_Disable(rng);
#if defined(CONFIG_SOC_STM32WB09XX)
	/* RM0505 Rev.2 §14.4:
	 * "After the TRNG IP is disabled by setting CR.DISABLE, in order to
	 * properly restart the TRNG IP, the AES_RESET bit must be set to 1
	 * (that is, resetting the AES core and restarting all health tests)."
	 */
	LL_RNG_SetAesReset(rng, 1);
#endif /* CONFIG_SOC_STM32WB09XX */

#ifdef CONFIG_SOC_SERIES_STM32WBAX
	uint32_t wait_cycles, rng_rate;

	if (LL_PKA_IsEnabled(PKA)) {
		return 0;
	}

	if (clock_control_get_rate(dev_data->clock,
			(clock_control_subsys_t) &dev_cfg->pclken[0],
			&rng_rate) < 0) {
		return -EIO;
	}

	wait_cycles = SystemCoreClock / rng_rate * 2;

	for (int i = wait_cycles; i >= 0; i--) {
	}
#endif /* CONFIG_SOC_SERIES_STM32WBAX */

	res = clock_control_off(dev_data->clock,
			(clock_control_subsys_t)&dev_cfg->pclken[0]);

#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_STM32H7_DUAL_CORE)
	z_stm32_hsem_unlock(CFG_HW_RNG_SEMID);
#endif /* CONFIG_SOC_SERIES_STM32WBX || CONFIG_STM32H7_DUAL_CORE */

	return res;
}

static int entropy_stm32_resume(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct entropy_stm32_rng_dev_data *dev_data = dev->data;
	const struct entropy_stm32_rng_dev_cfg *dev_cfg = dev->config;
	RNG_TypeDef *rng = dev_data->rng;
	int res;

	res = clock_control_on(dev_data->clock,
			(clock_control_subsys_t)&dev_cfg->pclken[0]);
	LL_RNG_Enable(rng);
	ll_rng_enable_it(rng);

	return res;
}

static void configure_rng(void)
{
	RNG_TypeDef *rng = entropy_stm32_rng_data.rng;

#ifdef STM32_CONDRST_SUPPORT
	uint32_t desired_nist_cfg = DT_INST_PROP_OR(0, nist_config, 0U);
	uint32_t desired_htcr = DT_INST_PROP_OR(0, health_test_config, 0U);
	uint32_t cur_nist_cfg = 0U;
	uint32_t cur_htcr = 0U;

#if DT_INST_NODE_HAS_PROP(0, nist_config)
	/*
	 * Configure the RNG_CR in compliance with the NIST SP800.
	 * The nist-config is direclty copied from the DTS.
	 * The RNG clock must be 48MHz else the clock DIV is not adpated.
	 * The RNG_CR_CONDRST is set to 1 at the same time the RNG_CR is written
	 */
	cur_nist_cfg = READ_BIT(rng->CR,
				(RNG_CR_NISTC | RNG_CR_CLKDIV | RNG_CR_RNG_CONFIG1 |
				RNG_CR_RNG_CONFIG2 | RNG_CR_RNG_CONFIG3
#if defined(RNG_CR_ARDIS)
				| RNG_CR_ARDIS
	/* For STM32U5 series, the ARDIS bit7 is considered in the nist-config */
#endif /* RNG_CR_ARDIS */
			));
#endif /* nist_config */

#if DT_INST_NODE_HAS_PROP(0, health_test_config)
	cur_htcr = LL_RNG_GetHealthConfig(rng);
#endif /* health_test_config */

	if (cur_nist_cfg != desired_nist_cfg || cur_htcr != desired_htcr) {
		MODIFY_REG(rng->CR, cur_nist_cfg, (desired_nist_cfg | RNG_CR_CONDRST));

#if DT_INST_NODE_HAS_PROP(0, health_test_config)
#if DT_INST_NODE_HAS_PROP(0, health_test_magic)
		LL_RNG_SetHealthConfig(rng, DT_INST_PROP(0, health_test_magic));
#endif /* health_test_magic */
		LL_RNG_SetHealthConfig(rng, desired_htcr);
#endif /* health_test_config */

		LL_RNG_DisableCondReset(rng);
		/* Wait for conditioning reset process to be completed */
		while (LL_RNG_IsEnabledCondReset(rng) == 1) {
		}
	}
#endif /* STM32_CONDRST_SUPPORT */

	LL_RNG_Enable(rng);
	ll_rng_enable_it(rng);
}

static void acquire_rng(void)
{
	entropy_stm32_resume();
#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_STM32H7_DUAL_CORE)
	/* Lock the RNG to prevent concurrent access */
	z_stm32_hsem_lock(CFG_HW_RNG_SEMID, HSEM_LOCK_WAIT_FOREVER);
	/* RNG configuration could have been changed by the other core */
	configure_rng();
#endif /* CONFIG_SOC_SERIES_STM32WBX || CONFIG_STM32H7_DUAL_CORE */
}

static void release_rng(void)
{
	entropy_stm32_suspend();
#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_STM32H7_DUAL_CORE)
	z_stm32_hsem_unlock(CFG_HW_RNG_SEMID);
#endif /* CONFIG_SOC_SERIES_STM32WBX || CONFIG_STM32H7_DUAL_CORE */
}

static int entropy_stm32_got_error(RNG_TypeDef *rng)
{
	__ASSERT_NO_MSG(rng != NULL);

#if defined(STM32_CONDRST_SUPPORT)
	if (LL_RNG_IsActiveFlag_CECS(rng)) {
		return 1;
	}
#endif

	if (ll_rng_is_active_seis(rng)) {
		return 1;
	}

	return 0;
}

#if defined(STM32_CONDRST_SUPPORT)
/* SOCS w/ soft-reset support: execute the reset */
static int recover_seed_error(RNG_TypeDef *rng)
{
	uint32_t count_timeout = 0;

	LL_RNG_EnableCondReset(rng);
	LL_RNG_DisableCondReset(rng);
	/* When reset process is done cond reset bit is read 0
	 * This typically takes: 2 AHB clock cycles + 2 RNG clock cycles.
	 */

	while (LL_RNG_IsEnabledCondReset(rng) ||
		ll_rng_is_active_seis(rng) ||
		ll_rng_is_active_secs(rng)) {
		count_timeout++;
		if (count_timeout == 10) {
			return -ETIMEDOUT;
		}
	}

	return 0;
}

#else /* !STM32_CONDRST_SUPPORT */
/* SOCS w/o soft-reset support: flush pipeline */
static int recover_seed_error(RNG_TypeDef *rng)
{
	ll_rng_clear_seis(rng);

	for (int i = 0; i < 12; ++i) {
		(void)ll_rng_read_rand_data(rng);
	}

	if (ll_rng_is_active_seis(rng) != 0) {
		return -EIO;
	}

	return 0;
}
#endif /* !STM32_CONDRST_SUPPORT */

static int random_byte_get(void)
{
	int retval = -EAGAIN;
	unsigned int key;
	RNG_TypeDef *rng = entropy_stm32_rng_data.rng;

	key = irq_lock();

#if defined(CONFIG_ENTROPY_STM32_CLK_CHECK)
	if (!k_is_pre_kernel()) {
		/* CECS bit signals that a clock configuration issue is detected,
		 * which may lead to generation of non truly random data.
		 */
		__ASSERT(LL_RNG_IsActiveFlag_CECS(rng) == 0,
			 "CECS = 1: RNG domain clock is too slow.\n"
			 "\tSee ref man and update target clock configuration.");
	}
#endif /* CONFIG_ENTROPY_STM32_CLK_CHECK */

	if (ll_rng_is_active_seis(rng) && (recover_seed_error(rng) < 0)) {
		retval = -EIO;
		goto out;
	}

	if (ll_rng_is_active_drdy(rng) == 1) {
		if (entropy_stm32_got_error(rng)) {
			retval = -EIO;
			goto out;
		}

		retval = ll_rng_read_rand_data(rng);
		if (retval == 0) {
			/* A seed error could have occurred between RNG_SR
			 * polling and RND_DR output reading.
			 */
			retval = -EAGAIN;
			goto out;
		}

		retval &= 0xFF;
	}

out:

	irq_unlock(key);

	return retval;
}

static uint16_t generate_from_isr(uint8_t *buf, uint16_t len)
{
	uint16_t remaining_len = len;

#if !IRQLESS_TRNG
	__ASSERT_NO_MSG(!irq_is_enabled(IRQN));
#endif /* !IRQLESS_TRNG */

#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_STM32H7_DUAL_CORE)
	__ASSERT_NO_MSG(z_stm32_hsem_is_owned(CFG_HW_RNG_SEMID));
#endif /* CONFIG_SOC_SERIES_STM32WBX || CONFIG_STM32H7_DUAL_CORE */

	/* do not proceed if a Seed error occurred */
	if (ll_rng_is_active_secs(entropy_stm32_rng_data.rng) ||
		ll_rng_is_active_seis(entropy_stm32_rng_data.rng)) {

		(void)random_byte_get(); /* this will recover the error */

		return 0; /* return cnt is null : no random data available */
	}

#if !IRQLESS_TRNG
	/* Clear NVIC pending bit. This ensures that a subsequent
	 * RNG event will set the Cortex-M single-bit event register
	 * to 1 (the bit is set when NVIC pending IRQ status is
	 * changed from 0 to 1)
	 */
	NVIC_ClearPendingIRQ(IRQN);
#endif /* !IRQLESS_TRNG */

	do {
		int byte;

		while (ll_rng_is_active_drdy(
				entropy_stm32_rng_data.rng) != 1) {
#if !IRQLESS_TRNG
			/*
			 * Enter low-power mode while waiting for event
			 * generated by TRNG interrupt becoming pending.
			 *
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
#endif /* !IRQLESS_TRNG */
		}

		byte = random_byte_get();
#if !IRQLESS_TRNG
		NVIC_ClearPendingIRQ(IRQN);
#endif /* IRQLESS_TRNG */

		if (byte < 0) {
			continue;
		}

		buf[--remaining_len] = byte;
	} while (remaining_len);

	return len;
}

static int start_pool_filling(bool wait)
{
	unsigned int key;
	bool already_filling;

	key = irq_lock();
#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_STM32H7_DUAL_CORE)
	/* In non-blocking mode, return immediately if the RNG is not available */
	if (!wait && z_stm32_hsem_try_lock(CFG_HW_RNG_SEMID) != 0) {
		irq_unlock(key);
		return -EAGAIN;
	}
#else
	ARG_UNUSED(wait);
#endif /* CONFIG_SOC_SERIES_STM32WBX || CONFIG_STM32H7_DUAL_CORE */

	already_filling = entropy_stm32_rng_data.filling_pools;
	entropy_stm32_rng_data.filling_pools = true;
	irq_unlock(key);

	if (unlikely(already_filling)) {
		return 0;
	}

	/* Prevent the clocks to be stopped during the duration the rng pool is
	 * being populated. The ISR will release the constraint again when the
	 * rng pool is filled.
	 */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	if (IS_ENABLED(CONFIG_PM_S2RAM)) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	}

	acquire_rng();
#if IRQLESS_TRNG
	k_work_schedule(&entropy_stm32_rng_data.trng_poll_work, TRNG_GENERATION_DELAY);
#else /* !IRQLESS_TRNG */
	irq_enable(IRQN);
#endif /* IRQLESS_TRNG */
	return 0;
}

static void pool_filling_work_handler(struct k_work *work)
{
	if (start_pool_filling(false) != 0) {
		/* RNG could not be acquired, try again */
		k_work_submit(work);
	}
}

static uint16_t rng_pool_get(struct rng_pool *rngp, uint8_t *buf,
	uint16_t len)
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
		/*
		 * Avoid starting pool filling from ISR as it might require
		 * blocking if RNG is not available and a race condition could
		 * also occur if this ISR has interrupted the RNG ISR.
		 *
		 * If the TRNG has no IRQ line, always schedule the work item,
		 * as this is what fills the RNG pools instead of the ISR.
		 */
		if (k_is_in_isr() || IRQLESS_TRNG) {
			k_work_submit(&entropy_stm32_rng_data.filling_work);
		} else {
			start_pool_filling(true);
		}
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

static void rng_pool_init(struct rng_pool *rngp, uint16_t size,
			uint8_t threshold)
{
	rngp->first_alloc = 0U;
	rngp->first_read  = 0U;
	rngp->last	  = 0U;
	rngp->mask	  = size - 1;
	rngp->threshold	  = threshold;
}

static int perform_pool_refill(void)
{
	int byte, ret;

	byte = random_byte_get();
	if (byte < 0) {
		return -EIO;
	}

	ret = rng_pool_put((struct rng_pool *)(entropy_stm32_rng_data.isr),
				byte);
	if (ret < 0) {
		ret = rng_pool_put(
				(struct rng_pool *)(entropy_stm32_rng_data.thr),
				byte);
		if (ret < 0) {
#if !IRQLESS_TRNG
			irq_disable(IRQN);
#endif /* !IRQLESS_TRNG */
			release_rng();
			pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
			if (IS_ENABLED(CONFIG_PM_S2RAM)) {
				pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
			}
			entropy_stm32_rng_data.filling_pools = false;
		}

		k_sem_give(&entropy_stm32_rng_data.sem_sync);
	}

	return ret;
}

#if IRQLESS_TRNG
static void trng_poll_work_item(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	RNG_TypeDef *rng = entropy_stm32_rng_data.rng;

	/* Seed error occurred: reset TRNG and try again */
	if (ll_rng_is_active_secs(entropy_stm32_rng_data.rng) ||
		ll_rng_is_active_seis(entropy_stm32_rng_data.rng)) {

		(void)random_byte_get(); /* this will recover the error */
	} else if (ll_rng_is_active_drdy(rng)) {
		/* Entropy available: read it and fill pools */
		int res = perform_pool_refill();

		if (res == -ENOBUFS) {
			/**
			 * All RNG pools are full - no more work needed.
			 * Exit early to stop the work item from re-scheduling
			 * itself. The RNG peripheral has already been released
			 * by perform_pool_refill().
			 */
			return;
		}
	} else {
		/**
		 * No entropy available - try again later
		 */
	}

	/* Schedule ourselves for next cycle */
	k_work_schedule(dwork, TRNG_GENERATION_DELAY);
}
#else /* !IRQLESS_TRNG */
static void stm32_rng_isr(const void *arg)
{
	ARG_UNUSED(arg);

	(void)perform_pool_refill();
}
#endif /* IRQLESS_TRNG */

static int entropy_stm32_rng_get_entropy(const struct device *dev,
					 uint8_t *buf,
					 uint16_t len)
{
	/* Check if this API is called on correct driver instance. */
	__ASSERT_NO_MSG(&entropy_stm32_rng_data == dev->data);

	while (len) {
		uint16_t bytes;

		k_sem_take(&entropy_stm32_rng_data.sem_lock, K_FOREVER);
		bytes = rng_pool_get(
				(struct rng_pool *)(entropy_stm32_rng_data.thr),
				buf, len);

		if (bytes == 0U) {
			/* Pool is empty: Sleep until next interrupt. */
			k_sem_take(&entropy_stm32_rng_data.sem_sync, K_FOREVER);
		}

		k_sem_give(&entropy_stm32_rng_data.sem_lock);

		len -= bytes;
		buf += bytes;
	}

	return 0;
}

static int entropy_stm32_rng_get_entropy_isr(const struct device *dev,
					     uint8_t *buf,
					     uint16_t len,
					     uint32_t flags)
{
	uint16_t cnt = len;

	/* Check if this API is called on correct driver instance. */
	__ASSERT_NO_MSG(&entropy_stm32_rng_data == dev->data);

	if (likely((flags & ENTROPY_BUSYWAIT) == 0U)) {
		return rng_pool_get(
				(struct rng_pool *)(entropy_stm32_rng_data.isr),
				buf, len);
	}

	if (len) {
		/**
		 * On TRNG without interrupt line, we cannot allow reentrancy,
		 * so we have to suspend all interrupts. Otherwise, only suspend
		 * it until we have established ourselves as owner of the TRNG
		 * to prevent race with a higher priority interrupt handler.
		 */
		unsigned int key = irq_lock();
		bool rng_already_acquired = false;
#if !IRQLESS_TRNG
		int irq_enabled = irq_is_enabled(IRQN);

		rng_already_acquired = (irq_enabled != 0);
		irq_disable(IRQN);
		irq_unlock(key);
#endif /* !IRQLESS_TRNG */

		/* Do not release if IRQ is enabled. RNG will be released in ISR
		 * when the pools are full. On TRNG without interrupt line, the
		 * default value of false ensures TRNG is always released.
		 */
		if (z_stm32_hsem_is_owned(CFG_HW_RNG_SEMID)) {
			rng_already_acquired = true;
		}
		acquire_rng();

		cnt = generate_from_isr(buf, len);

		/* Restore the state of the RNG lock and IRQ */
		if (!rng_already_acquired) {
			release_rng();
		}

#if IRQLESS_TRNG
		/* Exit critical section */
		irq_unlock(key);
#else
		if (irq_enabled) {
			irq_enable(IRQN);
		}
#endif /* !IRQLESS_TRNG */
	}

	return cnt;
}

static int entropy_stm32_rng_init(const struct device *dev)
{
	struct entropy_stm32_rng_dev_data *dev_data;
	const struct entropy_stm32_rng_dev_cfg *dev_cfg;
	int res;

	__ASSERT_NO_MSG(dev != NULL);

	dev_data = dev->data;
	dev_cfg = dev->config;

	__ASSERT_NO_MSG(dev_data != NULL);
	__ASSERT_NO_MSG(dev_cfg != NULL);

	dev_data->clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(dev_data->clock)) {
		return -ENODEV;
	}

	res = clock_control_on(dev_data->clock,
		(clock_control_subsys_t)&dev_cfg->pclken[0]);
	__ASSERT_NO_MSG(res == 0);

	/* Configure domain clock if any */
	if (DT_INST_NUM_CLOCKS(0) > 1) {
		res = clock_control_configure(dev_data->clock,
					      (clock_control_subsys_t)&dev_cfg->pclken[1],
					      NULL);
		__ASSERT(res == 0, "Could not select RNG domain clock");
	}

	/* Locking semaphore initialized to 1 (unlocked) */
	k_sem_init(&dev_data->sem_lock, 1, 1);

	/* Synching semaphore */
	k_sem_init(&dev_data->sem_sync, 0, 1);

	k_work_init(&dev_data->filling_work, pool_filling_work_handler);

#if IRQLESS_TRNG
	k_work_init_delayable(&dev_data->trng_poll_work, trng_poll_work_item);
#endif /* IRQLESS_TRNG */

	rng_pool_init((struct rng_pool *)(dev_data->thr),
		      CONFIG_ENTROPY_STM32_THR_POOL_SIZE,
		      CONFIG_ENTROPY_STM32_THR_THRESHOLD);
	rng_pool_init((struct rng_pool *)(dev_data->isr),
		      CONFIG_ENTROPY_STM32_ISR_POOL_SIZE,
		      CONFIG_ENTROPY_STM32_ISR_THRESHOLD);

#if !IRQLESS_TRNG
	IRQ_CONNECT(IRQN, IRQ_PRIO, stm32_rng_isr, &entropy_stm32_rng_data, 0);
#endif /* !IRQLESS_TRNG */

#if !defined(CONFIG_SOC_SERIES_STM32WBX) && !defined(CONFIG_STM32H7_DUAL_CORE)
	/* For multi-core MCUs, RNG configuration is automatically performed
	 * after acquiring the RNG in start_pool_filling()
	 */
	configure_rng();
#endif /* !CONFIG_SOC_SERIES_STM32WBX && !CONFIG_STM32H7_DUAL_CORE */

	start_pool_filling(true);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int entropy_stm32_rng_pm_action(const struct device *dev,
				       enum pm_device_action action)
{
	struct entropy_stm32_rng_dev_data *dev_data = dev->data;

	int res = 0;

	/* Remove warning on some platforms */
	ARG_UNUSED(dev_data);

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_STM32H7_DUAL_CORE)
		/* Lock to Prevent concurrent access with PM */
		z_stm32_hsem_lock(CFG_HW_RNG_SEMID, HSEM_LOCK_WAIT_FOREVER);
	/* Call release_rng instead of entropy_stm32_suspend to avoid double hsem_unlock */
#endif /* CONFIG_SOC_SERIES_STM32WBX || CONFIG_STM32H7_DUAL_CORE */
		release_rng();
		break;
	case PM_DEVICE_ACTION_RESUME:
		if (IS_ENABLED(CONFIG_PM_S2RAM)) {
#if DT_INST_NODE_HAS_PROP(0, health_test_config)
			entropy_stm32_resume();
#if DT_INST_NODE_HAS_PROP(0, health_test_magic)
			LL_RNG_SetHealthConfig(dev_data->rng, DT_INST_PROP(0, health_test_magic));
#endif /* health_test_magic */
			if (LL_RNG_GetHealthConfig(dev_data->rng) !=
				DT_INST_PROP_OR(0, health_test_config, 0U)) {
				entropy_stm32_rng_init(dev);
			} else if (!entropy_stm32_rng_data.filling_pools) {
				/* Resume RNG only if it was suspended during filling pool */
#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_STM32H7_DUAL_CORE)
				/* Lock to Prevent concurrent access with PM */
				z_stm32_hsem_lock(CFG_HW_RNG_SEMID, HSEM_LOCK_WAIT_FOREVER);
				/*
				 * Call release_rng instead of entropy_stm32_suspend
				 * to avoid double hsem_unlock
				 */
#endif /* CONFIG_SOC_SERIES_STM32WBX || CONFIG_STM32H7_DUAL_CORE */
				release_rng();
			}
#endif /* health_test_config */
		} else {
			/* Resume RNG only if it was suspended during filling pool */
			if (entropy_stm32_rng_data.filling_pools) {
				res = entropy_stm32_resume();
			}
		}
		break;
	default:
		return -ENOTSUP;
	}

	return res;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(entropy, entropy_stm32_rng_api) = {
	.get_entropy = entropy_stm32_rng_get_entropy,
	.get_entropy_isr = entropy_stm32_rng_get_entropy_isr
};

PM_DEVICE_DT_INST_DEFINE(0, entropy_stm32_rng_pm_action);

DEVICE_DT_INST_DEFINE(0,
		    entropy_stm32_rng_init,
		    PM_DEVICE_DT_INST_GET(0),
		    &entropy_stm32_rng_data, &entropy_stm32_rng_config,
		    PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		    &entropy_stm32_rng_api);
