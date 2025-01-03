/**
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32wb09_rng

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include <errno.h>

#include <soc.h>
#include <stm32_ll_rng.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(entropy_stm32wb0, CONFIG_ENTROPY_LOG_LEVEL);

/**
 * RM0505 §14.4 "TRNG functional description":
 *  To use the TRNG peripheral the system clock frequency must be
 *  at least 32 MHz. See also: §6.2.2 "Peripheral clock details".
 */
BUILD_ASSERT(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC >= (32 * 1000 * 1000),
	"STM32WB0: TRNG requires system clock frequency >= 32MHz");

/**
 * Driver private structures
 */

/**
 * RNG pool implementation, taken from existing STM32 entropy driver
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


struct wb09_trng_driver_config {
	struct stm32_pclken clk;
};

struct wb09_trng_driver_data {
	RNG_TypeDef *const reg;

	/**
	 * This semaphore is used to keep track of RNG state.
	 * When RNG is enabled, the semaphore is 0.
	 * When RNG is disabled, the semaphore is 1.
	 */
	struct k_sem rng_enable_sem;

	/**
	 * This semaphore is signaled when new entropy bytes
	 * are available in the thread entropy pool.
	 */
	struct k_sem thr_rng_avail_sem;

	/* Declare pools at the end to minimize padding */
	RNG_POOL_DEFINE(isr_pool, CONFIG_ENTROPY_STM32_ISR_POOL_SIZE);
	RNG_POOL_DEFINE(thr_pool, CONFIG_ENTROPY_STM32_THR_POOL_SIZE);
};

/**
 * Driver private forward declarations
 */
static const struct wb09_trng_driver_config drv_config;
static struct wb09_trng_driver_data drv_data;

static void turn_on_trng(struct wb09_trng_driver_data *data);

/**
 * RNG pool implementation (from STM32 entropy driver)
 */
static uint16_t rng_pool_get(struct rng_pool *rngp, uint8_t *buf,
	uint16_t len)
{
	unsigned int key = irq_lock();
	uint32_t last  = rngp->last;
	uint32_t mask  = rngp->mask;
	uint8_t *dst   = buf;
	uint32_t first, available;
	uint32_t other_read_in_progress;

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
		key = irq_lock();
		turn_on_trng(&drv_data);
		irq_unlock(key);
	}

	return len;
}

static int rng_pool_put(struct rng_pool *rngp, uint32_t random_val)
{
	unsigned int key = irq_lock();
	int res = 0;

	for (int i = 0; i < sizeof(random_val); i++) {
		uint8_t first = rngp->first_read;
		uint8_t last  = rngp->last;
		uint8_t mask  = rngp->mask;
		uint8_t byte  = (random_val & 0xFF);

		/* Signal error if the pool is full. */
		if (((last - first) & mask) == mask) {
			res = -ENOBUFS;
		}

		rngp->buffer[last] = byte;
		rngp->last = (last + 1) & mask;

		random_val >>= 8;
	}

	irq_unlock(key);
	return res;
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

/**
 * Driver private definitions
 */
#define TRNG_FIFO_SIZE	4		/* in 32-bit words */
#define TRNG_IRQN	DT_INST_IRQN(0)

/**
 * Driver utility functions
 */

/**
 * The following functions are missing from STM32CubeWB0 v1.0.0.
 * Once a version providing these functions is released and
 * integrated in Zephyr, we can get rid of them.
 */
static inline void ll_rng_clearflag_FF_FULL(RNG_TypeDef *RNGx)
{
	WRITE_REG(RNGx->IRQ_SR, RNG_IRQ_SR_FF_FULL_IRQ);
}

static inline void ll_rng_clearflag_ERROR(RNG_TypeDef *RNGx)
{
	WRITE_REG(RNGx->IRQ_SR, RNG_IRQ_SR_ERROR_IRQ);
}

/**
 * Driver private functions
 */

/**
 * @brief Turn on the TRNG
 *
 * @note Must be called with irq_lock held.
 */
static void turn_on_trng(struct wb09_trng_driver_data *data)
{
	RNG_TypeDef *rng = data->reg;
	int res;

	res = k_sem_take(&data->rng_enable_sem, K_NO_WAIT);
	if (res < 0) {
		/* RNG already on - nothing to do. */
		return;
	}

	/* Acquire power management locks */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	if (IS_ENABLED(CONFIG_PM_S2RAM)) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	}

	/* Turn on RNG */
	LL_RNG_Enable(rng);
	while (LL_RNG_IsActiveFlag_DISABLED(rng)) {
		/* Wait for RNG to be enabled */
	}
}

/**
 * @brief Turn off the TRNG
 *
 * @note Must be called with irq_lock held
 */
static void turn_off_trng(struct wb09_trng_driver_data *data)
{
	RNG_TypeDef *rng = data->reg;

	/* Turn off RNG */
	LL_RNG_Disable(rng);
	while (!LL_RNG_IsActiveFlag_DISABLED(rng)) {
		/* Wait for RNG to be disabled */
	}

	/* RM0505 Rev.2 §14.4:
	 * "After the TRNG IP is disabled by setting CR.DISABLE, in order to
	 * properly restart the TRNG IP, the AES_RESET bit must be set to 1
	 * (that is, resetting the AES core and restarting all health tests)."
	 */
	LL_RNG_SetAesReset(rng, 1);

	/* Mark RNG as disabled in semaphore */
	k_sem_give(&data->rng_enable_sem);
}

static void wb09_trng_isr(struct wb09_trng_driver_data *data)
{
	RNG_TypeDef *rng = data->reg;
	unsigned int key;
	uint32_t rnd;
	int res;

	/* Interrupt cause: TRNG health test error */
	if (LL_RNG_GetErrorIrq(rng)) {
		LOG_ERR("TRNG health test error occurred");

		/* Restart the TRNG (must be done atomically) */
		key = irq_lock();
			turn_off_trng(data);
			ll_rng_clearflag_ERROR(rng);
			ll_rng_clearflag_FF_FULL(rng);
			turn_on_trng(data);
		irq_unlock(key);
		return;
	}

	/* Interrupt cause: TRNG FIFO is full */
	if (LL_RNG_GetFfFullIrq(rng)) {
		/* Write the random data from FIFO to entropy pools */
		bool wrote_to_thr_pool = false;
		bool pools_full = false;

		for (unsigned int i = 0; i < TRNG_FIFO_SIZE; i++) {
			/**
			 * Don't bother checking the VAL_READY flag:
			 * interrupt was raised because FIFO is full!
			 */
			rnd = LL_RNG_GetRndVal(rng);

			/* Add entropy to ISR pool first */
			res = rng_pool_put((struct rng_pool *)&data->isr_pool, rnd);
			if (res >= 0) {
				continue;
			}

			/* ISR pool is full - try to fill thread pool instead */
			res = rng_pool_put((struct rng_pool *)&data->thr_pool, rnd);
			if (res >= 0) {
				wrote_to_thr_pool = true;
				continue;
			}

			/* Both pools are full - lock context and stop processing */
			key = irq_lock();
			pools_full = true;
			break;
		}

		/* Clear interrupt flag */
		ll_rng_clearflag_FF_FULL(rng);

		/* Signal "new data available" semaphore if applicable */
		if (wrote_to_thr_pool) {
			k_sem_give(&data->thr_rng_avail_sem);
		}

		/* Stop TRNG if driver pools are full.
		 * This has to be done with interrupts suspended to
		 * prevent race conditions with higher priority ISRs
		 * which is why we suspended interrupts earlier.
		 */
		if (pools_full) {
			turn_off_trng(data);
			irq_unlock(key);
		}
	}
}

/**
 * Driver subsystem API implementation
 */
static int wb09_trng_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	struct wb09_trng_driver_data *data = dev->data;
	unsigned int key;

	/* Reset "data available" semaphore */
	key = irq_lock();
		k_sem_reset(&data->thr_rng_avail_sem);
	irq_unlock(key);

	while (length > 0) {
		uint16_t read = rng_pool_get(
					(struct rng_pool *)&data->thr_pool,
					buffer, length);
		buffer += read;
		length -= read;

		if (length > 0) {
			k_sem_take(&data->thr_rng_avail_sem, K_FOREVER);
		}
	}

	return 0;
}

static int wb09_trng_get_entropy_from_isr(const struct device *dev, uint8_t *buffer,
					uint16_t length, uint32_t flags)
{
	struct wb09_trng_driver_data *data = dev->data;
	RNG_TypeDef *rng = data->reg;
	uint16_t poll_read_remain;
	int trng_irq_enabled;
	unsigned int key;

	const uint16_t pool_read_size = rng_pool_get(
						(struct rng_pool *)&data->isr_pool,
						buffer, length);
	if (pool_read_size == length || !(flags & ENTROPY_BUSYWAIT)) {
		/* We fullfilled the request from ISR pool or caller doesn't want to block.
		 * Either way, we are done and can return what we got now.
		 */
		return pool_read_size;
	}

	/* Blocking call: read data from TRNG until buffer is filled
	 *
	 * Start by masking the TRNG interrupt at NVIC level to prevent the
	 * driver's ISR from executing. (The driver ISR itself may have been
	 * preempted before a call to this function - this is fine as the ISR
	 * performs "dangerous" operations in IRQ-locked critical sections).
	 *
	 * The only thing we have to be careful about is re-entrancy; however,
	 * since STM32WB09 is a uniprocessor SoC, the only case in which this
	 * function may be called while it is executing is if a higher-priority
	 * ISR preempts the currently executing one. To protect from this, check
	 * whether the interrupt was enabled before disabling it, and re-enable
	 * it only if it was enabled on entry - this way, interrupts will be
	 * re-enabled by the lowest priority ISR once everyone is finished.
	 *
	 * Since we know we'll need the TRNG, turn it on here too.
	 */
	key = irq_lock();
		trng_irq_enabled = irq_is_enabled(TRNG_IRQN);
		irq_disable(TRNG_IRQN);

		turn_on_trng(data);
	irq_unlock(key);

	/* Take into account partial fill-up from ISR pool */
	poll_read_remain = length - pool_read_size;
	buffer += pool_read_size;

	do {
		while (!LL_RNG_IsActiveFlag_VAL_READY(rng)) {
			/* Wait for random data to be generated */
		}

		/* Write value from TRNG to user buffer */
		uint32_t random_val = LL_RNG_GetRndVal(rng);
		size_t copy_length = MIN(poll_read_remain, sizeof(random_val));

		memcpy(buffer, &random_val, copy_length);

		buffer += copy_length;
		poll_read_remain -= copy_length;
	} while (poll_read_remain > 0);

	if (trng_irq_enabled) {
		/* Re-enable the TRNG interrupt if we disabled it */
		key = irq_lock();
			irq_enable(TRNG_IRQN);
		irq_unlock(key);
	}

	return length;
}

static DEVICE_API(entropy, entropy_stm32wb09_api) = {
	.get_entropy = wb09_trng_get_entropy,
	.get_entropy_isr = wb09_trng_get_entropy_from_isr
};

static int wb09_trng_init(const struct device *dev)
{
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct wb09_trng_driver_config *config = dev->config;
	struct wb09_trng_driver_data *data = dev->data;
	RNG_TypeDef *rng = data->reg;
	int err;

	k_sem_init(&data->rng_enable_sem, 1, 1);
	k_sem_init(&data->thr_rng_avail_sem, 0, 1);

	if (!device_is_ready(clk)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(clk, (clock_control_subsys_t)&config->clk);
	if (err < 0) {
		LOG_ERR("Failed to turn on TRNG clock");
		return err;
	}

	/* Initialize RNG pools */
	rng_pool_init((struct rng_pool *)(&data->thr_pool),
		      CONFIG_ENTROPY_STM32_THR_POOL_SIZE,
		      CONFIG_ENTROPY_STM32_THR_THRESHOLD);
	rng_pool_init((struct rng_pool *)(&data->isr_pool),
		      CONFIG_ENTROPY_STM32_ISR_POOL_SIZE,
		      CONFIG_ENTROPY_STM32_ISR_THRESHOLD);

	/* Turn on RNG IP to generate some entropy */
	turn_on_trng(data);

	/* Attach ISR and unmask RNG interrupt in NVIC */
	IRQ_CONNECT(TRNG_IRQN, DT_INST_IRQ(0, priority),
		wb09_trng_isr, &drv_data, 0);

	irq_enable(TRNG_IRQN);

	/* Enable RNG FIFO full and error interrupts */
	LL_RNG_EnableEnFfFullIrq(rng);
	LL_RNG_EnableEnErrorIrq(rng);

	return 0;
}

/**
 * Driver power management callbacks
 */
#ifdef CONFIG_PM_DEVICE
static int wb09_trng_pm_action(const struct device *dev,
				       enum pm_device_action action)
{
	int res = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		turn_off_trng(dev->data);
		break;
	case PM_DEVICE_ACTION_RESUME:
		wb09_trng_init(dev);
		break;
	default:
		res = -ENOTSUP;
	}

	return res;
}
#endif /* CONFIG_PM_DEVICE */

/**
 * Driver device instantiation
 */

PM_DEVICE_DT_INST_DEFINE(0, wb09_trng_pm_action);

static const struct wb09_trng_driver_config drv_config = {
	.clk = STM32_CLOCK_INFO(0, DT_DRV_INST(0))
};

static struct wb09_trng_driver_data drv_data = {
	.reg = (RNG_TypeDef *)DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0,
		    wb09_trng_init,
		    PM_DEVICE_DT_INST_GET(0),
		    &drv_data, &drv_config, PRE_KERNEL_1,
		    CONFIG_ENTROPY_INIT_PRIORITY,
		    &entropy_stm32wb09_api);
