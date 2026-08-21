/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include "virtio_pci_internal.h"

#define TEST_IRQ 5U

DEFINE_FFF_GLOBALS;
FAKE_VOID_FUNC(arch_irq_enable, unsigned int);

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	RESET_FAKE(arch_irq_enable);
}

ZTEST(virtio_pci_init_irq, test_failed_init_does_not_enable_irq)
{
	virtio_pci_enable_irq_on_success(1, TEST_IRQ);

	zassert_equal(arch_irq_enable_fake.call_count, 0U);
}

ZTEST(virtio_pci_init_irq, test_successful_init_enables_irq)
{
	virtio_pci_enable_irq_on_success(0, TEST_IRQ);

	zassert_equal(arch_irq_enable_fake.call_count, 1U);
	zassert_equal(arch_irq_enable_fake.arg0_val, TEST_IRQ);
}

ZTEST_SUITE(virtio_pci_init_irq, NULL, NULL, before, NULL, NULL);
