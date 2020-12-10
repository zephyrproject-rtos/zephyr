/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Exati Tecnologia Ltda.
 * Copyright (c) 2020 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_rng

#include <kernel.h>
#include <device.h>
#include <drivers/entropy.h>
#include <random/rand32.h>
#include <init.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <errno.h>
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_rng.h>
#include <stm32_ll_system.h>
#include <sys/printk.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include "stm32_hsem.h"

#define IRQN		DT_INST_IRQN(0)
#define IRQ_PRIO	DT_INST_IRQ(0, priority)

/*
 * This driver need to take into account all STM32 family:
 *  - simple rng without harware fifo and no DMA.
 *  - Variable delay between two consecutive random numbers
 *    (depending on family and clock settings)
 *
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

BUILD_ASSERT((CONFIG_ENTROPY_STM32_ISR_POOL_SIZE &
	      (CONFIG_ENTROPY_STM32_ISR_POOL_SIZE - 1)) == 0,
	     "The CONFIG_ENTROPY_STM32_ISR_POOL_SIZE must be a power of 2!");

BUILD_ASSERT((CONFIG_ENTROPY_STM32_THR_POOL_SIZE &
	      (CONFIG_ENTROPY_STM32_THR_POOL_SIZE - 1)) == 0,
	     "The CONFIG_ENTROPY_STM32_THR_POOL_SIZE must be a power of 2!");

struct entropy_stm32_rng_dev_cfg {
	struct stm32_pclken pclken;
};

struct entropy_stm32_rng_dev_data {
	RNG_TypeDef *rng;
	const struct device *clock;
	struct k_sem sem_lock;
	struct k_sem sem_sync;

	RNG_POOL_DEFINE(isr, CONFIG_ENTROPY_STM32_ISR_POOL_SIZE);
	RNG_POOL_DEFINE(thr, CONFIG_ENTROPY_STM32_THR_POOL_SIZE);
};

#define DEV_DATA(dev) \
	((struct entropy_stm32_rng_dev_data *)(dev)->data)

#define DEV_CFG(dev) \
	((const struct entropy_stm32_rng_dev_cfg *)(dev)->config)


static const struct entropy_stm32_rng_dev_cfg entropy_stm32_rng_config = {
	.pclken	= { .bus = DT_INST_CLOCKS_CELL(0, bus),
		    .enr = DT_INST_CLOCKS_CELL(0, bits) },
};

static struct entropy_stm32_rng_dev_data entropy_stm32_rng_data = {
	.rng = (RNG_TypeDef *)DT_INST_REG_ADDR(0),
};

static int entropy_stm32_got_error(RNG_TypeDef *rng)
{
	__ASSERT_NO_MSG(rng != NULL);

	if (LL_RNG_IsActiveFlag_CECS(rng)) {
		return 1;
	}

	if (LL_RNG_IsActiveFlag_SECS(rng)) {
		return 1;
	}

	return 0;
}

static int random_byte_get(void)
{
	int retval = -EAGAIN;
	unsigned int key;

	key = irq_lock();

	if ((LL_RNG_IsActiveFlag_DRDY(entropy_stm32_rng_data.rng) == 1)) {
		if (entropy_stm32_got_error(entropy_stm32_rng_data.rng)) {
			retval = -EIO;
		} else {
			retval = LL_RNG_ReadRandData32(
						    entropy_stm32_rng_data.rng);
		}
	}

	irq_unlock(key);

	return retval;
}

#pragma GCC push_options
#if defined(CONFIG_BT_CTLR_FAST_ENC)
#pragma GCC optimize ("Ofast")
#endif
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
		LL_RNG_EnableIT(entropy_stm32_rng_data.rng);
	}

	return len;
}
#pragma GCC pop_options

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

static void stm32_rng_isr(const void *arg)
{
	int byte, ret;

	ARG_UNUSED(arg);

	byte = random_byte_get();
	if (byte < 0) {
		return;
	}

	ret = rng_pool_put((struct rng_pool *)(entropy_stm32_rng_data.isr),
				byte);
	if (ret < 0) {
		ret = rng_pool_put(
				(struct rng_pool *)(entropy_stm32_rng_data.thr),
				byte);
		if (ret < 0) {
			LL_RNG_DisableIT(entropy_stm32_rng_data.rng);
		}

		k_sem_give(&entropy_stm32_rng_data.sem_sync);
	}
}

static int entropy_stm32_rng_get_entropy(const struct device *device,
					 uint8_t *buf,
					 uint16_t len)
{
	/* Check if this API is called on correct driver instance. */
	__ASSERT_NO_MSG(&entropy_stm32_rng_data == DEV_DATA(device));

	while (len) {
		uint16_t bytes;

		k_sem_take(&entropy_stm32_rng_data.sem_lock, K_FOREVER);
		bytes = rng_pool_get(
				(struct rng_pool *)(entropy_stm32_rng_data.thr),
				buf, len);
		k_sem_give(&entropy_stm32_rng_data.sem_lock);

		if (bytes == 0U) {
			/* Pool is empty: Sleep until next interrupt. */
			k_sem_take(&entropy_stm32_rng_data.sem_sync, K_FOREVER);
			continue;
		}

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
	__ASSERT_NO_MSG(&entropy_stm32_rng_data == DEV_DATA(dev));

	if (likely((flags & ENTROPY_BUSYWAIT) == 0U)) {
		return rng_pool_get(
				(struct rng_pool *)(entropy_stm32_rng_data.isr),
				buf, len);
	}

	if (len) {
		unsigned int key;
		int irq_enabled;

		key = irq_lock();
		irq_enabled = irq_is_enabled(IRQN);
		irq_disable(IRQN);
		irq_unlock(key);

		/* Clear NVIC pending bit. This ensures that a subsequent
		 * RNG event will set the Cortex-M single-bit event register
		 * to 1 (the bit is set when NVIC pending IRQ status is
		 * changed from 0 to 1)
		 */
		NVIC_ClearPendingIRQ(IRQN);

		do {
			int byte;

			while (LL_RNG_IsActiveFlag_DRDY(
					entropy_stm32_rng_data.rng) != 1) {
				/*
				 * To guarantee waking up from the event, the
				 * SEV-On-Pend feature must be enabled (enabled
				 * during ARCH initialization).
				 *
				 * DSB is recommended by spec before WFE (to
				 * guarantee completion of memory transactions)
				 */
				__DSB();
				__WFE();
				__SEV();
				__WFE();
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

static int entropy_stm32_rng_init(const struct device *dev)
{
	struct entropy_stm32_rng_dev_data *dev_data;
	const struct entropy_stm32_rng_dev_cfg *dev_cfg;
	int res;

	__ASSERT_NO_MSG(dev != NULL);

	dev_data = DEV_DATA(dev);
	dev_cfg = DEV_CFG(dev);

	__ASSERT_NO_MSG(dev_data != NULL);
	__ASSERT_NO_MSG(dev_cfg != NULL);

#if CONFIG_SOC_SERIES_STM32L4X
	/* Configure PLLSA11 to enable 48M domain */
	LL_RCC_PLLSAI1_ConfigDomain_48M(LL_RCC_PLLSOURCE_MSI,
					LL_RCC_PLLM_DIV_1,
					24, LL_RCC_PLLSAI1Q_DIV_2);

	/* Enable PLLSA1 */
	LL_RCC_PLLSAI1_Enable();

	/*  Enable PLLSAI1 output mapped on 48MHz domain clock */
	LL_RCC_PLLSAI1_EnableDomain_48M();

	/* Wait for PLLSA1 ready flag */
	while (LL_RCC_PLLSAI1_IsReady() != 1) {
	}

	/*  Write the peripherals independent clock configuration register :
	 *  choose PLLSAI1 source as the 48 MHz clock is needed for the RNG
	 *  Linear Feedback Shift Register
	 */
	 LL_RCC_SetRNGClockSource(LL_RCC_RNG_CLKSOURCE_PLLSAI1);
#elif defined(RCC_CR2_HSI48ON) || defined(RCC_CR_HSI48ON) \
	|| defined(RCC_CRRCR_HSI48ON)

#if CONFIG_SOC_SERIES_STM32L0X
	/* We need SYSCFG to control VREFINT, so make sure it is clocked */
	if (!LL_APB2_GRP1_IsEnabledClock(LL_APB2_GRP1_PERIPH_SYSCFG)) {
		return -EINVAL;
	}
	/* HSI48 requires VREFINT (see RM0376 section 7.2.4). */
	LL_SYSCFG_VREFINT_EnableHSI48();
#endif /* CONFIG_SOC_SERIES_STM32L0X */

	z_stm32_hsem_lock(CFG_HW_CLK48_CONFIG_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	/* Use the HSI48 for the RNG */
	LL_RCC_HSI48_Enable();
	while (!LL_RCC_HSI48_IsReady()) {
		/* Wait for HSI48 to become ready */
	}

	LL_RCC_SetRNGClockSource(LL_RCC_RNG_CLKSOURCE_HSI48);

#if !defined(CONFIG_SOC_SERIES_STM32WBX)
	/* Specially for STM32WB, don't unlock the HSEM to prevent M0 core
	 * to disable HSI48 clock used for RNG.
	 */
	z_stm32_hsem_unlock(CFG_HW_CLK48_CONFIG_SEMID);
#endif /* CONFIG_SOC_SERIES_STM32WBX */

#endif /* CONFIG_SOC_SERIES_STM32L4X */

	dev_data->clock = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	__ASSERT_NO_MSG(dev_data->clock != NULL);

	res = clock_control_on(dev_data->clock,
		(clock_control_subsys_t *)&dev_cfg->pclken);
	__ASSERT_NO_MSG(res == 0);

	LL_RNG_EnableIT(dev_data->rng);

	LL_RNG_Enable(dev_data->rng);



	/* Locking semaphore initialized to 1 (unlocked) */
	k_sem_init(&dev_data->sem_lock, 1, 1);

	/* Synching semaphore */
	k_sem_init(&dev_data->sem_sync, 0, 1);

	rng_pool_init((struct rng_pool *)(dev_data->thr),
		      CONFIG_ENTROPY_STM32_THR_POOL_SIZE,
		      CONFIG_ENTROPY_STM32_THR_THRESHOLD);
	rng_pool_init((struct rng_pool *)(dev_data->isr),
		      CONFIG_ENTROPY_STM32_ISR_POOL_SIZE,
		      CONFIG_ENTROPY_STM32_ISR_THRESHOLD);

	IRQ_CONNECT(IRQN, IRQ_PRIO, stm32_rng_isr, &entropy_stm32_rng_data, 0);
	irq_enable(IRQN);

	return 0;
}

static const struct entropy_driver_api entropy_stm32_rng_api = {
	.get_entropy = entropy_stm32_rng_get_entropy,
	.get_entropy_isr = entropy_stm32_rng_get_entropy_isr
};

DEVICE_DT_INST_DEFINE(0,
		    entropy_stm32_rng_init, device_pm_control_nop,
		    &entropy_stm32_rng_data, &entropy_stm32_rng_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &entropy_stm32_rng_api);
