/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define TEST_IRQ_MAX_DELAY K_MSEC(10)

#define INTD0_NODE DT_ALIAS(test_intd0)
#define INTD1_NODE DT_ALIAS(test_intd1)

#define INTD0_SYS_IRQN SYS_DT_IRQN(INTD0_NODE)
#define INTD1_SYS_IRQN SYS_DT_IRQN(INTD1_NODE)

#define INTD0_IRQ_FLAGS SYS_DT_IRQ_FLAGS(INTD0_NODE)
#define INTD1_IRQ_FLAGS SYS_DT_IRQ_FLAGS(INTD1_NODE)

BUILD_ASSERT(INTD0_SYS_IRQN == INTD1_SYS_IRQN, "INTDs must share INTL");
BUILD_ASSERT(INTD0_IRQ_FLAGS == INTD1_IRQ_FLAGS, "INTDs must share configuration flags");
BUILD_ASSERT(DT_NODE_HAS_STATUS(INTD0_NODE, reserved), "INTD0 must have status reserved");
BUILD_ASSERT(DT_NODE_HAS_STATUS(INTD1_NODE, reserved), "INTD1 must have status reserved");

static K_SEM_DEFINE(irq0_sem, 0, 1);
static K_SEM_DEFINE(irq1_sem, 0, 1);

static uint32_t irq0_data;
static uint32_t irq1_data;

static int irq0_handler(const void *data)
{
	zassert_equal(data, &irq0_data);
	k_sem_give(&irq0_sem);
	return *((const uint32_t *)data);
}

static int irq1_handler(const void *data)
{
	zassert_equal(data, &irq1_data);
	k_sem_give(&irq1_sem);
	return *((const uint32_t *)data);
}

SYS_DT_DEFINE_IRQ_HANDLER(INTD0_NODE, irq0_handler, &irq0_data);
SYS_DT_DEFINE_IRQ_HANDLER(INTD1_NODE, irq1_handler, &irq1_data);

ZTEST(sys_irq_shared_irq, test__trigger_irq_intd0_origin__intd0_handles_irq)
{
	irq0_data = SYS_IRQ_HANDLED;
	zassert_ok(sys_irq_enable(INTD0_SYS_IRQN));
	zassert_ok(sys_irq_trigger(INTD0_SYS_IRQN));
	zassert_equal(k_sem_take(&irq0_sem, TEST_IRQ_MAX_DELAY), 0);
}

ZTEST(sys_irq_shared_irq, test__trigger_irq_intd1_origin__intd1_handles_irq)
{
	irq1_data = SYS_IRQ_HANDLED;
	zassert_ok(sys_irq_enable(INTD0_SYS_IRQN));
	zassert_ok(sys_irq_trigger(INTD0_SYS_IRQN));
	zassert_equal(k_sem_take(&irq1_sem, TEST_IRQ_MAX_DELAY), 0);
}

void *test_init(void)
{
	zassert_ok(sys_irq_configure(INTD0_SYS_IRQN, INTD0_IRQ_FLAGS));
	zassert_equal(k_sem_take(&irq0_sem, TEST_IRQ_MAX_DELAY), -EAGAIN);
	return NULL;
}

void test_before(void *f)
{
	ARG_UNUSED(f);

	zassert(sys_irq_disable(INTD0_SYS_IRQN) > -1, "retval must be positive");
	zassert(sys_irq_disable(INTD1_SYS_IRQN) > -1, "retval must be positive");
	zassert(sys_irq_clear(INTD0_SYS_IRQN) > -1, "retval must be positive");
	zassert(sys_irq_clear(INTD1_SYS_IRQN) > -1, "retval must be positive");
	k_sem_reset(&irq0_sem);
	k_sem_reset(&irq1_sem);
	irq0_data = SYS_IRQ_NONE;
	irq1_data = SYS_IRQ_NONE;
}

ZTEST_SUITE(sys_irq_shared_irq, NULL, test_init, test_before, NULL, NULL);
