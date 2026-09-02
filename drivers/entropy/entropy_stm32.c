/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Exati Tecnologia Ltda.
 * Copyright (c) 2020 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <soc.h>
#include <stm32_bitops.h>
#include <stm32_hsem.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pka.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_rng.h>
#include <stm32_ll_system.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "entropy_stm32.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(entropy_stm32, CONFIG_ENTROPY_LOG_LEVEL);

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
		STM32_HCLK_FREQUENCY >= (32 * 1000 * 1000),
	"STM32WB09: TRNG requires system clock frequency >= 32MHz");

struct entropy_stm32_rng_dev_cfg {
	RNG_TypeDef *rng;
	const struct device *clock;
	struct stm32_pclken *pclken;
};

struct entropy_stm32_rng_dev_data {
	struct k_sem sem_lock;
	struct k_sem sem_sync;
	struct k_work filling_work;
#if IRQLESS_TRNG
	/* work item that polls TRNG to refill pools */
	struct k_work_delayable trng_poll_work;
#endif /* IRQLESS_TRNG */
	struct k_work_q *filling_wq;
	atomic_t filling_pools;		/* 1 when pools filling is in progress, 0 otherwise */
	atomic_t pm_locked;		/* 1 when PM is locked for pool refill, 0 otherwise */

	RNG_POOL_DEFINE(isr, CONFIG_ENTROPY_STM32_ISR_POOL_SIZE);
	RNG_POOL_DEFINE(thr, CONFIG_ENTROPY_STM32_THR_POOL_SIZE);
};

#define TRNG_BASE ((RNG_TypeDef *)DT_INST_REG_ADDR(0))

#ifdef CONFIG_ENTROPY_STM32_WORKQUEUE
static K_THREAD_STACK_DEFINE(entropy_stm32_wq_stack, CONFIG_ENTROPY_STM32_WQ_STACK_SIZE);
static struct k_work_q entropy_stm32_wq;
#endif

static struct stm32_pclken pclken_rng[] = STM32_DT_INST_CLOCKS(0);

static struct entropy_stm32_rng_dev_cfg entropy_stm32_rng_config = {
	.rng = TRNG_BASE,
	.clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
	.pclken	= pclken_rng
};

static struct entropy_stm32_rng_dev_data entropy_stm32_rng_data;

#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_STM32H7_DUAL_CORE)
#define HAS_MULTICORE_SHARED_RNG 1
#else /* CONFIG_SOC_SERIES_STM32WBX || CONFIG_STM32H7_DUAL_CORE */
#define HAS_MULTICORE_SHARED_RNG 0
#endif /* CONFIG_SOC_SERIES_STM32WBX || CONFIG_STM32H7_DUAL_CORE */

static void entropy_stm32_hsem_acquire(void)
{
	z_stm32_hsem_lock(CFG_HW_RNG_SEMID, HSEM_LOCK_WAIT_FOREVER);
}

static int entropy_stm32_hsem_try_acquire(void)
{
	return z_stm32_hsem_try_lock(CFG_HW_RNG_SEMID);
}

static void entropy_stm32_hsem_release(void)
{
	z_stm32_hsem_unlock(CFG_HW_RNG_SEMID);
}

/* Note API quirk: on uniprocessor, this returns false! */
static bool entropy_stm32_hsem_is_owned(void)
{
	return z_stm32_hsem_is_owned(CFG_HW_RNG_SEMID);
}

#define ASSERT_RNG_HSEM_OWNED() \
	__ASSERT_NO_MSG(!HAS_MULTICORE_SHARED_RNG || entropy_stm32_hsem_is_owned())

static void entropy_stm32_suspend(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	const struct entropy_stm32_rng_dev_cfg *dev_cfg = dev->config;
	RNG_TypeDef *rng = dev_cfg->rng;
	int res = 0;

	entropy_stm32_hsem_acquire();

	LL_RNG_Disable(rng);
#if defined(CONFIG_SOC_STM32WB09XX)
	/* RM0505 Rev.2 §14.4:
	 * "After the TRNG IP is disabled by setting CR.DISABLE, in order to
	 * properly restart the TRNG IP, the AES_RESET bit must be set to 1
	 * (that is, resetting the AES core and restarting all health tests)."
	 */
	LL_RNG_SetAesReset(rng, 1);
#endif /* CONFIG_SOC_STM32WB09XX */

/*
 * The PKA IP is currently not supported by Zephyr but may be used by
 * external code, such as wireless stack for example. Since the RNG
 * clock must be enabled when PKA is used on certain series, check if
 * the PKA is in use and keep RNG clock active if so.
 *
 * A notable exception is the STM32WB0 series where PKA can operate
 * autonomously and, on certain SoCs, lacks PKA_CR.EN and corresponding
 * LL_PKA_IsEnabled(). Since RNG clock is not required by PKA, we can
 * ignore the check on this series.
 */
#if defined(PKA) && !defined(CONFIG_SOC_SERIES_STM32WB0X)
#if defined(CONFIG_STM32_HAL2)
	uint32_t pka_clock_enabled = HAL_RCC_PKA_IsEnabledClock();
#else /* CONFIG_STM32_HAL2 */
	uint32_t pka_clock_enabled = __HAL_RCC_PKA_IS_CLK_ENABLED();
#endif /* CONFIG_STM32_HAL2 */

	if (pka_clock_enabled && LL_PKA_IsEnabled(PKA)) {
		entropy_stm32_hsem_release();

		/* PKA needs RNG clock, so exit here if in use */
		return;
	}
#endif /* PKA && !CONFIG_SOC_SERIES_STM32WB0X */

#ifdef CONFIG_SOC_SERIES_STM32WBAX
	uint32_t wait_cycles, rng_rate;

	res = clock_control_get_rate(dev_cfg->clock,
			(clock_control_subsys_t) &dev_cfg->pclken[0],
			&rng_rate);
	if (res == 0) {
		wait_cycles = SystemCoreClock / rng_rate * 2;

		for (int i = wait_cycles; i >= 0; i--) {
		}
	}

	/* STM32WBAX contrainsts prevent to disable the clock unless a few
	 * cycles were spent hence clock disabling below depends on @c res
	 * value.
	 */
#endif /* CONFIG_SOC_SERIES_STM32WBAX */

	if (res == 0) {
		/* Disabling the RNG clock is not expected to fail */
		res = clock_control_off(dev_cfg->clock,
					(clock_control_subsys_t)&dev_cfg->pclken[0]);
		__ASSERT_NO_MSG(res == 0);
	}

	entropy_stm32_hsem_release();
}

static void entropy_stm32_resume(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	const struct entropy_stm32_rng_dev_cfg *dev_cfg = dev->config;
	RNG_TypeDef *rng = dev_cfg->rng;
	__maybe_unused int res;

	/* Enabling the RNG clock is not expected to fail */
	res = clock_control_on(dev_cfg->clock, (clock_control_subsys_t)&dev_cfg->pclken[0]);
	__ASSERT_NO_MSG(res == 0);

#if defined(CONFIG_SOC_STM32WB09XX)
	/**
	 * STM32WB09 RNG clock domain runs at (16 MHz / CLKDIV).
	 * CLKDIV is 256 after reset which makes the RNG runs VERY slow.
	 * Configure CLKDIV=1 to ensure RNG runs at an acceptable speed.
	 */
	LL_RNG_SetSamplingClockEnableDivider(rng, 0);
#endif
	LL_RNG_Enable(rng);
	ll_rng_enable_it(rng);
}

static void configure_rng(void)
{
	RNG_TypeDef *rng = TRNG_BASE;

#ifdef STM32_CONDRST_SUPPORT
	uint32_t desired_nist_cfg = DT_INST_PROP_OR(0, nist_config, 0U);
	uint32_t desired_htcr = DT_INST_PROP_OR(0, health_test_config, 0U);
	uint32_t desired_nscr = DT_INST_PROP_OR(0, noise_source_control, 0U);
	uint32_t cur_nist_cfg = 0U;
	uint32_t cur_htcr = 0U;
	uint32_t cur_nscr = 0U;

#if DT_INST_NODE_HAS_PROP(0, nist_config)
	/*
	 * Configure the RNG_CR in compliance with the NIST SP800.
	 * The nist-config is directly copied from the DTS.
	 * The RNG clock must be 48MHz else the clock DIV is not adapted.
	 * The RNG_CR_CONDRST is set to 1 at the same time the RNG_CR is written
	 */
	cur_nist_cfg = stm32_reg_read_bits(&rng->CR,
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

#if DT_INST_NODE_HAS_PROP(0, noise_source_control)
	cur_nscr = LL_RNG_GetNoiseConfig(rng);
#endif /* noise_source_control */

	if (cur_nist_cfg != desired_nist_cfg || cur_htcr != desired_htcr ||
	    cur_nscr != desired_nscr) {
		stm32_reg_modify_bits(&rng->CR, cur_nist_cfg, desired_nist_cfg | RNG_CR_CONDRST);

#if DT_INST_NODE_HAS_PROP(0, health_test_config)
#if DT_INST_NODE_HAS_PROP(0, health_test_magic)
		/* On certain series, a magic value must be written first as
		 * health configuration before the actual configuration value.
		 */
		LL_RNG_SetHealthConfig(rng, DT_INST_PROP(0, health_test_magic));
#endif /* health_test_magic */
		LL_RNG_SetHealthConfig(rng, desired_htcr);
#endif /* health_test_config */

#if DT_INST_NODE_HAS_PROP(0, noise_source_control)
		LL_RNG_SetNoiseConfig(rng, DT_INST_PROP(0, noise_source_control));
#endif /* noise_source_control */

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
	entropy_stm32_hsem_acquire();
	entropy_stm32_resume();

#if HAS_MULTICORE_SHARED_RNG
	/* RNG configuration could have been changed by the other core */
	configure_rng();
#endif /* HAS_MULTICORE_SHARED_RNG */
}

static void release_rng(void)
{
	entropy_stm32_suspend();
	entropy_stm32_hsem_release();
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

#if !defined(CONFIG_SOC_SERIES_STM32WB0X)
	/* After a noise source error is detected, 12 words must be read from the RNG_DR register
	 * and discarded to restart the entropy generation.
	 */
	for (int i = 0; i < 12; ++i) {
		(void)ll_rng_read_rand_data(rng);
	}
#endif /* !CONFIG_SOC_SERIES_STM32WB0X */

#if defined(CONFIG_SOC_STM32WB09XX)
	if (ll_rng_is_active_seis(rng) != 0) {
		/* RM0505 §14.7.11 "Health Test Control Register (TRNG_HEALTH_CR)":
		 * When some oscillators are powered down, the cutoff values
		 * must be increased as health tests could trigger an error.
		 * The values 100 and 850 are arbitrarily higher than the default ones.
		 * It is recommended to disable TRNG before changing these values.
		 */
		LL_RNG_Disable(rng);
		ll_rng_clear_seis(rng);
		LL_RNG_SetAesReset(rng, 1);
		if (LL_RNG_IsActiveFlag_OSCS_REPET_ERROR(rng)) {
			LL_RNG_SetRepetCutoff(rng, 100);
		}
		if (LL_RNG_IsActiveFlag_OSCS_ADAPT_ERROR(rng)) {
			LL_RNG_SetAdapCutoff(rng, 850);
		}
		LL_RNG_Enable(rng);
	}
#endif /* CONFIG_SOC_STM32WB09XX */

	if (ll_rng_is_active_seis(rng) != 0) {
		return -EIO;
	}

	return 0;
}
#endif /* !STM32_CONDRST_SUPPORT */

static int random_sample_get(rng_sample_t *rnd_sample)
{
	int retval = -EAGAIN;
	unsigned int key;
	RNG_TypeDef *rng = TRNG_BASE;

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

		*rnd_sample = ll_rng_read_rand_data(rng);
		if (*rnd_sample == 0) {
			/* A seed error could have occurred between RNG_SR
			 * polling and RND_DR output reading.
			 */
			retval = -EAGAIN;
			goto out;
		}

		retval = 0;
	}

out:

	irq_unlock(key);

	return retval;
}

static uint16_t generate_from_isr(uint8_t *buf, uint16_t len)
{
	uint16_t remaining_len = len;
	rng_sample_t rnd_sample;
	int ret;

#if !IRQLESS_TRNG
	__ASSERT_NO_MSG(!irq_is_enabled(IRQN));
#endif /* !IRQLESS_TRNG */

	ASSERT_RNG_HSEM_OWNED();

	/* do not proceed if a Seed error occurred */
	if (ll_rng_is_active_secs(TRNG_BASE) ||
		ll_rng_is_active_seis(TRNG_BASE)) {

		(void)random_sample_get(&rnd_sample); /* this will recover the error */

		return 0; /* return cnt is null : no random data available */
	}

#if !IRQLESS_TRNG
	/* Clear NVIC pending bit. This ensures that a subsequent
	 * RNG event will set the Cortex-M single-bit event register
	 * to 1 (the bit is set when NVIC pending IRQ status is
	 * changed from 0 to 1)
	 */
	k_irq_clear_pending(IRQN);
#endif /* !IRQLESS_TRNG */

	do {
		while (ll_rng_is_active_drdy(TRNG_BASE) != 1) {

#if !defined(CONFIG_PM_S2RAM)
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
#endif /* !CONFIG_PM_S2RAM */
		}

		ret = random_sample_get(&rnd_sample);
#if !IRQLESS_TRNG
		k_irq_clear_pending(IRQN);
#endif /* !IRQLESS_TRNG */

		if (ret < 0) {
			continue;
		}

		/* push each byte of the RNG sample in buffer */
		size_t i = sizeof(rnd_sample);

		while (remaining_len && i) {
			buf[--remaining_len] = (uint8_t)(rnd_sample & 0xFFu);
			rnd_sample >>= 8;
			i--;
		}
	} while (remaining_len);

	return len;
}

static int start_pool_filling(bool wait)
{
	bool already_filling;

	if (!wait && entropy_stm32_hsem_try_acquire() != 0) {
		/* In non-blocking mode, return immediately if the RNG is not available */
		return -EAGAIN;
	}

	already_filling = atomic_set(&entropy_stm32_rng_data.filling_pools, 1) != 0;

	if (unlikely(already_filling)) {
		return 0;
	}

	acquire_rng();
#if IRQLESS_TRNG
	k_work_schedule_for_queue(entropy_stm32_rng_data.filling_wq,
				  &entropy_stm32_rng_data.trng_poll_work,
				  TRNG_GENERATION_DELAY);
#else /* !IRQLESS_TRNG */
	irq_enable(IRQN);
#endif /* IRQLESS_TRNG */
	return 0;
}

static void pool_filling_work_handler(struct k_work *work)
{
	if (start_pool_filling(false) != 0) {
		/* RNG could not be acquired, try again */
		k_work_submit_to_queue(entropy_stm32_rng_data.filling_wq, work);
	}
}

static void pool_refill_requested(void)
{
	struct entropy_stm32_rng_dev_data *dev_data = &entropy_stm32_rng_data;
	unsigned int key = irq_lock();

	if (atomic_set(&dev_data->pm_locked, 1) == 0) {
		/* Prevent the clocks to be stopped during the duration the rng pool is
		 * being populated. The ISR will release the constraint again when the
		 * rng pool is filled. This also ensures the pools are filled enough
		 * at wakeup where an ISR may request some.
		 */
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		if (IS_ENABLED(CONFIG_PM_S2RAM)) {
			pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
		}
	}

	irq_unlock(key);
}

static void pool_refill_completed(void)
{
	struct entropy_stm32_rng_dev_data *dev_data = &entropy_stm32_rng_data;
	unsigned int key = irq_lock();

	if (atomic_set(&dev_data->pm_locked, 0) != 0) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		if (IS_ENABLED(CONFIG_PM_S2RAM)) {
			pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
		}
	}

	atomic_set(&dev_data->filling_pools, 0);

	irq_unlock(key);
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
		pool_refill_requested();

		/*
		 * Avoid starting pool filling from ISR as it might require
		 * blocking if RNG is not available and a race condition could
		 * also occur if this ISR has interrupted the RNG ISR.
		 *
		 * If the TRNG has no IRQ line, always schedule the work item,
		 * as this is what fills the RNG pools instead of the ISR.
		 */
		if (k_is_in_isr() || IRQLESS_TRNG) {
			k_work_submit_to_queue(entropy_stm32_rng_data.filling_wq,
					       &entropy_stm32_rng_data.filling_work);
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
	rng_sample_t rnd_sample;
	bool refilled_thr = false;
	int ret;

	ret = random_sample_get(&rnd_sample);
	if (ret < 0) {
		return ret;
	}

	/* push each byte of the RNG sample in pools */
	for (size_t i = 0; i < sizeof(rnd_sample); i++, rnd_sample >>= 8) {
		uint8_t byte = rnd_sample & 0xFFu;

		ret = rng_pool_put((struct rng_pool *)(entropy_stm32_rng_data.isr), byte);
		if (ret < 0) {
			/* Take note that data has been added to thread pool */
			refilled_thr = true;

			ret = rng_pool_put((struct rng_pool *)(entropy_stm32_rng_data.thr), byte);
			if (ret < 0) {
#if !IRQLESS_TRNG
				irq_disable(IRQN);
#endif /* !IRQLESS_TRNG */
				release_rng();
				pool_refill_completed();
				break;
			}
		}
	}

	if (refilled_thr) {
		/**
		 * Wake up threads that may be waiting for new data to be
		 * available in thread pool if we added entropy in it.
		 */
		k_sem_give(&entropy_stm32_rng_data.sem_sync);
	}

	return ret;
}

#if IRQLESS_TRNG
static void trng_poll_work_item(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	RNG_TypeDef *rng = TRNG_BASE;

	/* Seed error occurred: reset TRNG and try again */
	if (ll_rng_is_active_secs(TRNG_BASE) ||
		ll_rng_is_active_seis(TRNG_BASE)) {

		rng_sample_t dummy;
		(void)random_sample_get(&dummy); /* this will recover the error */
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
		if (entropy_stm32_hsem_is_owned()) {
			rng_already_acquired = true;
		}
		if (!rng_already_acquired) {
			acquire_rng();
		}

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

static int entropy_stm32_init_hw_rng(const struct entropy_stm32_rng_dev_cfg *dev_cfg,
				     struct entropy_stm32_rng_dev_data *dev_data)
{
	int res;

	res = clock_control_on(dev_cfg->clock,
		(clock_control_subsys_t)&dev_cfg->pclken[0]);
	if (res != 0) {
		LOG_ERR("Failed to enable RNG bus clock (err %d). "
			"Check clock configuration in DTS.", res);
		return res;
	}

	/* Configure domain clock if any */
	if (DT_INST_NUM_CLOCKS(0) > 1) {
		res = clock_control_configure(dev_cfg->clock,
					      (clock_control_subsys_t)&dev_cfg->pclken[1],
					      NULL);
		if (res != 0) {
			LOG_ERR("Failed to configure RNG kernel clock (err %d). "
				"Verify domain clock (e.g. HSI48) is enabled in DTS.", res);
			return res;
		}
	}

#if !HAS_MULTICORE_SHARED_RNG
	/* For multi-core MCUs, RNG configuration is automatically performed
	 * after acquiring the RNG in start_pool_filling()
	 */
	configure_rng();
#endif /* !HAS_MULTICORE_SHARED_RNG */

	return 0;
}

static int entropy_stm32_init_wq(void)
{
	struct entropy_stm32_rng_dev_data *dev_data = &entropy_stm32_rng_data;

#ifdef CONFIG_ENTROPY_STM32_WORKQUEUE
	k_work_queue_init(&entropy_stm32_wq);
	k_work_queue_start(&entropy_stm32_wq, (k_thread_stack_t *)&entropy_stm32_wq_stack,
			   K_THREAD_STACK_SIZEOF(entropy_stm32_wq_stack),
			   CONFIG_ENTROPY_STM32_WQ_PRIO, NULL);
	k_thread_name_set(entropy_stm32_wq.thread_id, "stm32-rng-pool-filling");
	dev_data->filling_wq = &entropy_stm32_wq;
#else
	dev_data->filling_wq = &k_sys_work_q;
#endif /* CONFIG_ENTROPY_STM32_WORKQUEUE */

	pool_refill_requested();
	start_pool_filling(true);

	return 0;
}

#if !IRQLESS_TRNG
SYS_INIT(entropy_stm32_init_wq, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* !IRQLESS_TRNG */

static int entropy_stm32_init(const struct device *dev)
{
	const struct entropy_stm32_rng_dev_cfg *dev_cfg = dev->config;
	struct entropy_stm32_rng_dev_data *dev_data = dev->data;
	int res;

	/* Locking semaphore initialized to 1 (unlocked) */
	k_sem_init(&dev_data->sem_lock, 1, 1);

	/* Syncing semaphore */
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

	res = entropy_stm32_init_hw_rng(dev_cfg, dev_data);
	if (res < 0) {
		return res;
	}

	if (DT_INST_NUM_CLOCKS(0) > 1) {
		uint32_t rng_clock_rate;

		if (clock_control_get_rate(dev_cfg->clock,
					   (clock_control_subsys_t)&dev_cfg->pclken[1],
					   &rng_clock_rate) != 0) {
			LOG_ERR("Failed to get RNG domain clock rate");
			return -EIO;
		}

		if (rng_clock_rate == 0) {
			LOG_ERR("RNG domain clock is not running (null rate)");
			return -ENOTSUP;
		}
	}

	if (IRQLESS_TRNG) {
		entropy_stm32_init_wq();
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int entropy_stm32_rng_pm_action(const struct device *dev,
				       enum pm_device_action action)
{
	int res = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_RESUME:
		if (IS_ENABLED(CONFIG_PM_S2RAM)) {
			const struct entropy_stm32_rng_dev_cfg *dev_cfg = dev->config;
			struct entropy_stm32_rng_dev_data *dev_data = dev->data;

			entropy_stm32_hsem_acquire();

			res = entropy_stm32_init_hw_rng(dev_cfg, dev_data);
			if (res < 0) {
				LOG_ERR("Failed to re-init STM32 RNG: %d", res);
			}

			entropy_stm32_hsem_release();
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
		    entropy_stm32_init, PM_DEVICE_DT_INST_GET(0),
		    &entropy_stm32_rng_data, &entropy_stm32_rng_config,
		    STM32_TRNG_INIT_LEVEL, CONFIG_ENTROPY_INIT_PRIORITY,
		    &entropy_stm32_rng_api);
