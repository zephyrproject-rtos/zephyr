/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_trng

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/irq.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>

#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/sys_io.h>

#include <driverlib/prcm.h>
#include <driverlib/trng.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26X2.h>

#define CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

#define US_PER_SAMPLE (1000000ULL * \
	CONFIG_ENTROPY_CC13XX_CC26XX_SAMPLES_PER_CYCLE / CPU_FREQ + 1ULL)

struct entropy_cc13xx_cc26xx_data {
	struct k_sem lock;
	struct k_sem sync;
	struct ring_buf pool;
	uint8_t data[CONFIG_ENTROPY_CC13XX_CC26XX_POOL_SIZE];
#ifdef CONFIG_PM
	Power_NotifyObj post_notify;
	bool constrained;
#endif
};

static void start_trng(struct entropy_cc13xx_cc26xx_data *data)
{
	/* Initialization as described in TRM section 18.6.1.2 */
	TRNGReset();
	while (sys_read32(TRNG_BASE + TRNG_O_SWRESET)) {
		continue;
	}

	/* Set samples per cycle */
	TRNGConfigure(0, CONFIG_ENTROPY_CC13XX_CC26XX_SAMPLES_PER_CYCLE,
		0);
	/* De-tune FROs */
	sys_write32(TRNG_FRODETUNE_FRO_MASK_M, TRNG_BASE +
		TRNG_O_FRODETUNE);
	/* Enable FROs */
	sys_write32(TRNG_FROEN_FRO_MASK_M, TRNG_BASE + TRNG_O_FROEN);
	/* Set shutdown and alarm thresholds */
	sys_write32((CONFIG_ENTROPY_CC13XX_CC26XX_SHUTDOWN_THRESHOLD
		<< 16) | CONFIG_ENTROPY_CC13XX_CC26XX_ALARM_THRESHOLD,
		TRNG_BASE + TRNG_O_ALARMCNT);

	TRNGEnable();
	TRNGIntEnable(TRNG_NUMBER_READY | TRNG_FRO_SHUTDOWN);
}

#ifdef CONFIG_PM_DEVICE
static void stop_trng(struct entropy_cc13xx_cc26xx_data *data)
{
	TRNGDisable();

	TRNGIntClear(TRNG_NUMBER_READY | TRNG_FRO_SHUTDOWN);
	TRNGIntDisable(TRNG_NUMBER_READY | TRNG_FRO_SHUTDOWN);
}
#endif

static void handle_shutdown_ovf(void)
{
	uint32_t off;

	/* Clear shutdown */
	TRNGIntClear(TRNG_FRO_SHUTDOWN);
	/* Disabled FROs */
	off = sys_read32(TRNG_BASE + TRNG_O_ALARMSTOP);
	/* Clear alarms */
	sys_write32(0, TRNG_BASE + TRNG_O_ALARMMASK);
	sys_write32(0, TRNG_BASE + TRNG_O_ALARMSTOP);
	/* De-tune FROs */
	sys_write32(off, TRNG_BASE + TRNG_O_FRODETUNE);
	/* Re-enable FROs */
	sys_write32(off, TRNG_BASE + TRNG_O_FROEN);
}

static int entropy_cc13xx_cc26xx_get_entropy(const struct device *dev,
					     uint8_t *buf,
					     uint16_t len)
{
	struct entropy_cc13xx_cc26xx_data *data = dev->data;
	uint32_t cnt;

#ifdef CONFIG_PM
	unsigned int key = irq_lock();

	if (!data->constrained) {
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
		data->constrained = true;
	}
	irq_unlock(key);
#endif

	TRNGIntEnable(TRNG_NUMBER_READY);

	while (len) {
		k_sem_take(&data->lock, K_FOREVER);
		cnt = ring_buf_get(&data->pool, buf, len);
		k_sem_give(&data->lock);

		if (cnt) {
			buf += cnt;
			len -= cnt;
		} else {
			k_sem_take(&data->sync, K_FOREVER);
		}
	}

	return 0;
}

static void entropy_cc13xx_cc26xx_isr(const struct device *dev)
{
	struct entropy_cc13xx_cc26xx_data *data = dev->data;
	uint32_t src = 0;
	uint32_t cnt;
	uint32_t num[2];

	/* Interrupt service routine as described in TRM section 18.6.1.3.2 */
	src = TRNGStatusGet();

	if (src & TRNG_NUMBER_READY) {
		/* This function acknowledges the ready status */
		num[1] = TRNGNumberGet(TRNG_HI_WORD);
		num[0] = TRNGNumberGet(TRNG_LOW_WORD);

		cnt = ring_buf_put(&data->pool, (uint8_t *)num, sizeof(num));

		/* When pool is full disable interrupt and stop reading numbers */
		if (cnt != sizeof(num)) {
#ifdef CONFIG_PM
			if (data->constrained) {
				pm_policy_state_lock_put(
					PM_STATE_STANDBY,
					PM_ALL_SUBSTATES);
				data->constrained = false;
			}
#endif
			TRNGIntDisable(TRNG_NUMBER_READY);
		}

		k_sem_give(&data->sync);
	}

	/* Change the shutdown FROs' oscillating frequency in an attempt to
	 * prevent further locking on to the sampling clock frequency.
	 */
	if (src & TRNG_FRO_SHUTDOWN) {
		handle_shutdown_ovf();
	}
}

static int entropy_cc13xx_cc26xx_get_entropy_isr(const struct device *dev,
						 uint8_t *buf, uint16_t len,
						 uint32_t flags)
{
	struct entropy_cc13xx_cc26xx_data *data = dev->data;
	uint16_t cnt;
	uint16_t read = len;
	uint32_t src;
	uint32_t num[2];
	unsigned int key;

	key = irq_lock();
	cnt = ring_buf_get(&data->pool, buf, len);
	irq_unlock(key);

	if ((cnt == len) || ((flags & ENTROPY_BUSYWAIT) == 0U)) {
		read = cnt;
	} else {
		buf += cnt;
		len -= cnt;

		/* Allowed to busy-wait. We should use a polling approach */
		while (len) {
			key = irq_lock();

			src = TRNGStatusGet();
			if (src & TRNG_NUMBER_READY) {
				/*
				 * This function acknowledges the ready
				 * status
				 */
				num[1] = TRNGNumberGet(TRNG_HI_WORD);
				num[0] = TRNGNumberGet(TRNG_LOW_WORD);

				ring_buf_put(&data->pool, (uint8_t *)num,
					sizeof(num));
			}

			/*
			 * If interrupts were enabled during busy wait, this
			 * would allow us to pick up anything that has been put
			 * in by the ISR as well.
			 */
			cnt = ring_buf_get(&data->pool, buf, len);

			if (src & TRNG_FRO_SHUTDOWN) {
				handle_shutdown_ovf();
			}

			irq_unlock(key);

			if (cnt) {
				buf += cnt;
				len -= cnt;
			} else {
				k_busy_wait(US_PER_SAMPLE);
			}
		}

	}

	return read;
}

#ifdef CONFIG_PM
/*
 *  ======== post_notify_fxn ========
 *  Called by Power module when waking up the CPU from Standby. The TRNG needs
 *  to be reconfigured afterwards, unless Zephyr's device PM turned it off, in
 *  which case it'd be responsible for turning it back on and reconfiguring it.
 */
static int post_notify_fxn(unsigned int eventType, uintptr_t eventArg,
	uintptr_t clientArg)
{
	const struct device *dev = (const struct device *)clientArg;
	int ret = Power_NOTIFYDONE;
	int16_t res_id;

	/* Reconfigure the hardware if returning from sleep */
	if (eventType == PowerCC26XX_AWAKE_STANDBY) {
		res_id = PowerCC26XX_PERIPH_TRNG;

		if (Power_getDependencyCount(res_id) != 0) {
			/* Reconfigure and enable TRNG only if powered */
			start_trng(dev->data);
		}
	}

	return (ret);
}
#endif

#ifdef CONFIG_PM_DEVICE
static int entropy_cc13xx_cc26xx_pm_action(const struct device *dev,
					   enum pm_device_action action)
{
	struct entropy_cc13xx_cc26xx_data *data = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		Power_setDependency(PowerCC26XX_PERIPH_TRNG);
		start_trng(data);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		stop_trng(data);
		Power_releaseDependency(PowerCC26XX_PERIPH_TRNG);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static int entropy_cc13xx_cc26xx_init(const struct device *dev)
{
	struct entropy_cc13xx_cc26xx_data *data = dev->data;

	/* Initialize driver data */
	ring_buf_init(&data->pool, sizeof(data->data), data->data);

#if defined(CONFIG_PM)
	Power_setDependency(PowerCC26XX_PERIPH_TRNG);
	/* Stay out of standby until buffer is filled with entropy */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	data->constrained = true;
	/* Register notification function */
	Power_registerNotify(&data->post_notify,
		PowerCC26XX_AWAKE_STANDBY,
		post_notify_fxn, (uintptr_t)dev);
#else
	/* Power TRNG domain */
	PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH);

	/* Enable TRNG peripheral clocks */
	PRCMPeripheralRunEnable(PRCM_PERIPH_TRNG);
	/* Enabled the TRNG while in sleep mode to keep the entropy pool full. After
	 * the pool is full the TRNG will enter idle mode when random numbers are no
	 * longer being read. */
	PRCMPeripheralSleepEnable(PRCM_PERIPH_TRNG);
	PRCMPeripheralDeepSleepEnable(PRCM_PERIPH_TRNG);


	/* Load PRCM settings */
	PRCMLoadSet();
	while (!PRCMLoadGet()) {
		continue;
	}

	/* Peripherals should not be accessed until power domain is on. */
	while (PRCMPowerDomainsAllOn(PRCM_DOMAIN_PERIPH) !=
	       PRCM_DOMAIN_POWER_ON) {
		continue;
	}
#endif

	start_trng(data);

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    entropy_cc13xx_cc26xx_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static DEVICE_API(entropy, entropy_cc13xx_cc26xx_driver_api) = {
	.get_entropy = entropy_cc13xx_cc26xx_get_entropy,
	.get_entropy_isr = entropy_cc13xx_cc26xx_get_entropy_isr,
};

static struct entropy_cc13xx_cc26xx_data entropy_cc13xx_cc26xx_data = {
	.lock = Z_SEM_INITIALIZER(entropy_cc13xx_cc26xx_data.lock, 1, 1),
	.sync = Z_SEM_INITIALIZER(entropy_cc13xx_cc26xx_data.sync, 0, 1),
};

PM_DEVICE_DT_INST_DEFINE(0, entropy_cc13xx_cc26xx_pm_action);

DEVICE_DT_INST_DEFINE(0,
		entropy_cc13xx_cc26xx_init,
		PM_DEVICE_DT_INST_GET(0),
		&entropy_cc13xx_cc26xx_data, NULL,
		PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		&entropy_cc13xx_cc26xx_driver_api);
