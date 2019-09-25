/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <drivers/entropy.h>
#include <irq.h>
#include <sys/ring_buffer.h>
#include <sys/sys_io.h>

#include <driverlib/prcm.h>
#include <driverlib/trng.h>

struct entropy_cc13xx_cc26xx_data {
	struct k_sem lock;
	struct k_sem sync;
	struct ring_buf pool;
	u8_t data[CONFIG_ENTROPY_CC13XX_CC26XX_POOL_SIZE];
};

DEVICE_DECLARE(entropy_cc13xx_cc26xx);

static inline struct entropy_cc13xx_cc26xx_data *
get_dev_data(struct device *dev)
{
	return dev->driver_data;
}

static int entropy_cc13xx_cc26xx_get_entropy(struct device *dev, u8_t *buf,
					     u16_t len)
{
	struct entropy_cc13xx_cc26xx_data *data = get_dev_data(dev);
	u32_t cnt;

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

static void entropy_cc13xx_cc26xx_isr(void *arg)
{
	struct entropy_cc13xx_cc26xx_data *data = get_dev_data(arg);
	u32_t src, cnt, off;
	u32_t num[2];

	/* Interrupt service routine as described in TRM section 18.6.1.3.2 */
	src = TRNGStatusGet();

	if (src & TRNG_NUMBER_READY) {
		/* This function acknowledges the ready status */
		num[1] = TRNGNumberGet(TRNG_HI_WORD);
		num[0] = TRNGNumberGet(TRNG_LOW_WORD);

		cnt = ring_buf_put(&data->pool, (u8_t *)num, sizeof(num));

		/* When pool is full disable interrupt and stop reading numbers */
		if (cnt != sizeof(num)) {
			TRNGIntDisable(TRNG_NUMBER_READY);
		}

		k_sem_give(&data->sync);
	}

	/* Change the shutdown FROs' oscillating frequncy in an attempt to
	 * prevent further locking on to the sampling clock frequncy.
	 */
	if (src & TRNG_FRO_SHUTDOWN) {
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
}

static int entropy_cc13xx_cc26xx_init(struct device *dev)
{
	struct entropy_cc13xx_cc26xx_data *data = get_dev_data(dev);

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
	while (PRCMPowerDomainStatus(PRCM_DOMAIN_PERIPH) !=
	       PRCM_DOMAIN_POWER_ON) {
		continue;
	}

	/* Initialize driver data */
	ring_buf_init(&data->pool, sizeof(data->data), data->data);

	/* Initialization as described in TRM section 18.6.1.2 */
	TRNGReset();
	while (sys_read32(TRNG_BASE + TRNG_O_SWRESET)) {
		continue;
	}

	/* Set samples per cycle */
	TRNGConfigure(0, CONFIG_ENTROPY_CC13XX_CC26XX_SAMPLES_PER_CYCLE, 0);
	/* De-tune FROs */
	sys_write32(TRNG_FRODETUNE_FRO_MASK_M, TRNG_BASE + TRNG_O_FRODETUNE);
	/* Enable FROs */
	sys_write32(TRNG_FROEN_FRO_MASK_M, TRNG_BASE + TRNG_O_FROEN);
	/* Set shutdown and alarm thresholds */
	sys_write32((CONFIG_ENTROPY_CC13XX_CC26XX_SHUTDOWN_THRESHOLD << 16) |
			    CONFIG_ENTROPY_CC13XX_CC26XX_ALARM_THRESHOLD,
		    TRNG_BASE + TRNG_O_ALARMCNT);

	TRNGEnable();
	TRNGIntEnable(TRNG_NUMBER_READY | TRNG_FRO_SHUTDOWN);

	IRQ_CONNECT(DT_INST_0_TI_CC13XX_CC26XX_TRNG_IRQ_0,
		    DT_INST_0_TI_CC13XX_CC26XX_TRNG_IRQ_0_PRIORITY,
		    entropy_cc13xx_cc26xx_isr,
		    DEVICE_GET(entropy_cc13xx_cc26xx), 0);
	irq_enable(DT_INST_0_TI_CC13XX_CC26XX_TRNG_IRQ_0);

	return 0;
}

static const struct entropy_driver_api entropy_cc13xx_cc26xx_driver_api = {
	.get_entropy = entropy_cc13xx_cc26xx_get_entropy,
};

static struct entropy_cc13xx_cc26xx_data entropy_cc13xx_cc26xx_data = {
	.lock = Z_SEM_INITIALIZER(entropy_cc13xx_cc26xx_data.lock, 1, 1),
	.sync = Z_SEM_INITIALIZER(entropy_cc13xx_cc26xx_data.sync, 0, 1),
};

DEVICE_AND_API_INIT(entropy_cc13xx_cc26xx, DT_INST_0_TI_CC13XX_CC26XX_TRNG_LABEL,
		    entropy_cc13xx_cc26xx_init, &entropy_cc13xx_cc26xx_data,
		    NULL, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &entropy_cc13xx_cc26xx_driver_api);
