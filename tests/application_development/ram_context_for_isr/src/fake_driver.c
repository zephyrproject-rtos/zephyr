/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cmsis_core.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/ztest.h>
#include "fake_driver.h"

#define DT_DRV_COMPAT fakedriver

static void fake_driver_isr(const void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct fake_driver_data *data = dev->data;

	/* Store the address of fake_driver_isr in user_data because it will be optimized by the
	 * compiler (even with __noinline__ attribute)
	 */
	data->user_data = (void *)&fake_driver_isr;

	if (data->irq_callback != NULL) {
		data->irq_callback(dev, data->user_data);
	}
}

#ifdef CONFIG_TEST_FIND_FREE_IRQ
/* Scan NVIC for a free IRQ */

#if CONFIG_2ND_LVL_ISR_TBL_OFFSET > 0
#define TEST_1ST_LEVEL_INTERRUPTS_MAX (CONFIG_2ND_LVL_ISR_TBL_OFFSET - 1)
#else
#define TEST_1ST_LEVEL_INTERRUPTS_MAX (CONFIG_NUM_IRQS - 1)
#endif

static unsigned int get_test_irq(void)
{
	int i;

	for (i = TEST_1ST_LEVEL_INTERRUPTS_MAX; i >= 0; i--) {
		if (NVIC_GetEnableIRQ(i) == 0) {
			/*
			 * Interrupts configured statically with IRQ_CONNECT(.)
			 * are automatically enabled. NVIC_GetEnableIRQ()
			 * returning false, here, implies that the IRQ line is
			 * either not implemented or it is not enabled, thus,
			 * currently not in use by Zephyr.
			 */

			/* Set the NVIC line to pending. */
			NVIC_SetPendingIRQ(i);

			if (NVIC_GetPendingIRQ(i)) {
				/* If the NVIC line is pending, it is
				 * guaranteed that it is implemented; clear the
				 * line.
				 */
				NVIC_ClearPendingIRQ(i);

				if (!NVIC_GetPendingIRQ(i)) {
					/*
					 * If the NVIC line can be successfully
					 * un-pended, it is guaranteed that it
					 * can be used for software interrupt
					 * triggering.
					 */
					break;
				}
			}
		}
	}

	zassert_true(i >= 0, "No available IRQ line to use in the test\n");

	return (unsigned int)i;
}
#else /* CONFIG_TEST_FIND_FREE_IRQ */
static unsigned int get_test_irq(void)
{
	return CONFIG_TEST_IRQ_NUM;
}
#endif /* CONFIG_TEST_FIND_FREE_IRQ */

static int fake_driver_register_irq_callback(const struct device *dev,
					     fake_driver_irq_callback_t cb, void *user_data)
{
	struct fake_driver_data *data = dev->data;

	data->irq_callback = cb;
	data->user_data = user_data;

	return 0;
}

DEVICE_API(fake, fake_driver_func) = {
	.register_irq_callback = fake_driver_register_irq_callback,
};

static int fake_driver_init(const struct device *dev)
{
	struct fake_driver_data *data = dev->data;

	data->irq_num = get_test_irq();
	data->irq_callback = NULL;
	data->user_data = NULL;

	(void)irq_connect_dynamic(data->irq_num, CONFIG_TEST_IRQ_PRIO, fake_driver_isr, dev, 0);
	irq_enable(data->irq_num);

	return 0;
}

#define FAKE_INIT(inst)                                                                            \
	static struct fake_driver_data fake_driver_data_##inst;                                    \
	                                                                                           \
	DEVICE_DT_INST_DEFINE(inst, &fake_driver_init, NULL, &fake_driver_data_##inst, NULL,       \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &fake_driver_func);

DT_INST_FOREACH_STATUS_OKAY(FAKE_INIT)
