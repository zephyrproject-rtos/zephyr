/*
 * Copyright (c) 2021 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/gicv3_its.h>

#define DT_DRV_COMPAT   arm_gic_v3_its

static volatile unsigned int last_lpi_irq_num;

static void lpi_irq_handle(const void *parameter)
{
	uintptr_t i = (uintptr_t)parameter;

	last_lpi_irq_num = i;
}

#if defined(CONFIG_SOC_MIMX9596_A55) || defined(CONFIG_SOC_MIMX94398_A55)
/* DeviceID is 8bits */
#define ITS_TEST_DEV(id)        (id & 0xff)
/* Cover up to 832 LPIs over 26 DevicesIDs and 32 EventIDs per DeviceID */
#define ITS_TEST_NUM_DEVS       26
#define ITS_TEST_NUM_ITES       32
#elif defined(CONFIG_BOARD_QEMU_CORTEX_A53)
/* Keep DeviceIDs within the test overlay's 9-bit provision. */
#define ITS_TEST_DEV(id)        (id)
/* Cover up to 8192 LPIs over configured DeviceIDs and 32 EventIDs per DeviceID */
#define ITS_TEST_NUM_DEVS       CONFIG_GIC_V3_ITS_MAX_DEVICES
#define ITS_TEST_NUM_ITES       32
#else
/* Generate a DeviceID over the whole 16bits */
#define ITS_TEST_DEV(id)        ((((id + 256) % 16) << 12) | (((id + 256) % 24) << 8) | (id & 0xff))
/* Cover up to 8192 LPIs over 256 DevicesIDs and 32 EventIDs per DeviceID */
#define ITS_TEST_NUM_DEVS       256
#define ITS_TEST_NUM_ITES       32
#endif

/* Do not test all 8192 irqs, iterate with a prime offset to cover most of the possible event_ids */
#define ITS_TEST_NEXT		13

/* Active-wait loops waiting for an interrupt */
#define ITS_TEST_LOOPS		10

unsigned int vectors[ITS_TEST_NUM_DEVS][ITS_TEST_NUM_ITES];

ZTEST(arm64_gicv3_its, test_gicv3_its_alloc)
{
	int devn, event_id;
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	uint32_t first;
	int device_id = ITS_TEST_DEV(0);

	zassert_false(dev == NULL, "");
	zassert_equal(its_alloc_intids(dev, 0U), 0U, "");
	zassert_equal(its_setup_deviceid(dev, device_id, 0U), -EINVAL, "");
	zassert_equal(its_setup_deviceid(dev, device_id, CONFIG_GIC_V3_ITS_MAX_VECTORS + 1U),
		      -ENOMEM, "");

	first = its_alloc_intids(dev, 2U);
	zassert_true(first >= 8192U, "");
	zassert_equal(its_alloc_intid(dev), first + 2U, "");

	for (devn = 0; devn < ITS_TEST_NUM_DEVS; ++devn) {
		device_id = ITS_TEST_DEV(devn);

		if (devn == 0) {
			zassert_true(its_setup_deviceid(dev, device_id, 1) == 0, "");
			vectors[devn][0] = first;
			zassert_true(vectors[devn][0] >= 8192, "");
			zassert_true(its_map_intid(dev, device_id, 0, vectors[devn][0]) == 0, "");
		}

		int ret = its_setup_deviceid(dev, device_id, ITS_TEST_NUM_ITES);

		zassert_equal(ret, 0, "DeviceID %x setup failed: %d", device_id, ret);
		if (devn == 1) {
			first = its_alloc_map_intids(dev, device_id, ITS_TEST_NUM_ITES);
			zassert_true(first >= 8192U, "");
			for (event_id = 0; event_id < ITS_TEST_NUM_ITES; event_id++) {
				vectors[devn][event_id] = first + event_id;
			}
			continue;
		}

		for (event_id = (devn == 0) ? 1 : 0; event_id < ITS_TEST_NUM_ITES; ++event_id) {
			vectors[devn][event_id] = (devn == 0 && event_id < 3)
							  ? first + event_id
							  : its_alloc_intid(dev);
			zassert_true(vectors[devn][event_id] >= 8192, "");
			zassert_true(vectors[devn][event_id] < CONFIG_NUM_IRQS, "");

			zassert_true(its_map_intid(dev, device_id, event_id,
						   vectors[devn][event_id]) == 0, "");
		}
	}

	if (ITS_TEST_NUM_DEVS == CONFIG_GIC_V3_ITS_MAX_DEVICES) {
		zassert_true(its_setup_deviceid(dev, ITS_TEST_DEV(ITS_TEST_NUM_DEVS),
						ITS_TEST_NUM_ITES) == -ENOMEM,
			     "");
	}

	if (CONFIG_NUM_IRQS == (8192U + ITS_TEST_NUM_DEVS * ITS_TEST_NUM_ITES)) {
		zassert_equal(its_alloc_intid(dev), 0U, "");
		zassert_equal(its_map_intid(dev, ITS_TEST_DEV(0), 0U, CONFIG_NUM_IRQS), -EINVAL,
			      "");
	}
}

ZTEST(arm64_gicv3_its, test_gicv3_its_connect)
{
	int devn, event_id;
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	unsigned int remain = 0;

	zassert_false(dev == NULL, "");

	for (devn = 0; devn < ITS_TEST_NUM_DEVS; ++devn) {
		for (event_id = remain; event_id < ITS_TEST_NUM_ITES; event_id += ITS_TEST_NEXT) {
			unsigned int irqn = vectors[devn][event_id];

			zassert_true(irq_connect_dynamic(irqn, 0, lpi_irq_handle,
							 (void *)(uintptr_t)(irqn), 0) == irqn, "");

			irq_enable(irqn);
		}
		remain = event_id - ITS_TEST_NUM_ITES;
	}
}

ZTEST(arm64_gicv3_its, test_gicv3_its_irq_simple)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	unsigned int irqn = vectors[0][0];
	unsigned int timeout;
	int device_id = ITS_TEST_DEV(0);
	int event_id = 0;

	zassert_false(dev == NULL, "");

	last_lpi_irq_num = 0;
	zassert_true(its_send_int(dev, device_id, event_id) == 0, "");

	timeout = ITS_TEST_LOOPS;
	while (!last_lpi_irq_num && timeout) {
		timeout--;
	}

	zassert_true(last_lpi_irq_num == irqn,
			"IRQ %d of DeviceID %x EventID %d failed",
			irqn, device_id, event_id);
}

ZTEST(arm64_gicv3_its, test_gicv3_its_irq_disable)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	unsigned int irqn = vectors[0][0];
	unsigned int timeout;
	int device_id = ITS_TEST_DEV(0);
	int event_id = 0;

	zassert_false(dev == NULL, "");

	irq_disable(irqn);

	last_lpi_irq_num = 0;
	zassert_true(its_send_int(dev, device_id, event_id) == 0, "");

	timeout = ITS_TEST_LOOPS;
	while (!last_lpi_irq_num && timeout) {
		timeout--;
	}

	zassert_true(last_lpi_irq_num == 0,
			"IRQ %d of DeviceID %x EventID %d disable failed",
			irqn, device_id, event_id);

	irq_enable(irqn);

	last_lpi_irq_num = 0;
	zassert_true(its_send_int(dev, device_id, event_id) == 0, "");

	timeout = ITS_TEST_LOOPS;
	while (!last_lpi_irq_num && timeout) {
		timeout--;
	}

	zassert_true(last_lpi_irq_num == irqn,
			"IRQ %d of DeviceID %x EventID %d re-enable failed",
			irqn, device_id, event_id);
}

ZTEST(arm64_gicv3_its, test_gicv3_its_irq)
{
	int devn, event_id;
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	unsigned int timeout;
	unsigned int remain = 0;

	zassert_false(dev == NULL, "");

	for (devn = 0; devn < ITS_TEST_NUM_DEVS; ++devn) {
		int device_id = ITS_TEST_DEV(devn);

		for (event_id = remain; event_id < ITS_TEST_NUM_ITES; event_id += ITS_TEST_NEXT) {
			unsigned int irqn = vectors[devn][event_id];

			last_lpi_irq_num = 0;
			zassert_true(its_send_int(dev, device_id, event_id) == 0, "");

			timeout = ITS_TEST_LOOPS;
			while (!last_lpi_irq_num && timeout) {
				timeout--;
			}

			zassert_true(last_lpi_irq_num == irqn,
				     "IRQ %d of DeviceID %x EventID %d failed",
				     irqn, device_id, event_id);
		}

		remain = event_id - ITS_TEST_NUM_ITES;
	}
}

ZTEST_SUITE(arm64_gicv3_its, NULL, NULL, NULL, NULL, NULL);
