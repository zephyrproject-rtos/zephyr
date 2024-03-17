/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/intc/irq_vector.h>
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

BUILD_ASSERT(INTD0_SYS_IRQN != INTD1_SYS_IRQN, "INTDs must not share INTL");
BUILD_ASSERT(DT_NODE_HAS_STATUS(INTD0_NODE, reserved), "INTD0 must have status reserved");
BUILD_ASSERT(DT_NODE_HAS_STATUS(INTD1_NODE, reserved), "INTD1 must have status reserved");

static K_SEM_DEFINE(irq0_sem, 0, 1);
static K_SEM_DEFINE(irq1_sem, 0, 1);

INTC_DT_DEFINE_IRQ_VECTOR(INTD0_NODE)
{
	k_sem_give(&irq0_sem);
	return 1;
}

INTC_DT_DEFINE_IRQ_VECTOR(INTD1_NODE)
{
	k_sem_give(&irq1_sem);
	return 1;
}

ZTEST(sys_irq_direct_irq, test__trigger_irq_while_disabled__no_irq)
{
	zassert_ok(sys_irq_trigger(INTD0_SYS_IRQN));
	zassert_equal(k_sem_take(&irq0_sem, TEST_IRQ_MAX_DELAY), -EAGAIN);
	zassert_ok(sys_irq_trigger(INTD1_SYS_IRQN));
	zassert_equal(k_sem_take(&irq1_sem, TEST_IRQ_MAX_DELAY), -EAGAIN);
}

ZTEST(sys_irq_direct_irq, test__trigger_irq_while_enabled__irq)
{
	zassert_ok(sys_irq_enable(INTD0_SYS_IRQN));
	zassert_equal(k_sem_take(&irq0_sem, TEST_IRQ_MAX_DELAY), -EAGAIN);
	zassert_ok(sys_irq_enable(INTD1_SYS_IRQN));
	zassert_equal(k_sem_take(&irq1_sem, TEST_IRQ_MAX_DELAY), -EAGAIN);
	zassert_ok(sys_irq_trigger(INTD0_SYS_IRQN));
	zassert_equal(k_sem_take(&irq0_sem, TEST_IRQ_MAX_DELAY), 0);
	zassert_ok(sys_irq_trigger(INTD1_SYS_IRQN));
	zassert_equal(k_sem_take(&irq1_sem, TEST_IRQ_MAX_DELAY), 0);
}

ZTEST(sys_irq_irq, test__disable_disabled_irq__retval_0)
{
	zassert_equal(sys_irq_disable(INTD0_SYS_IRQN), 0);
	zassert_equal(sys_irq_disable(INTD1_SYS_IRQN), 0);
}

ZTEST(sys_irq_direct_irq, test__disable_enabled_irq__retval_1)
{
	zassert_ok(sys_irq_enable(INTD0_SYS_IRQN));
	zassert_ok(sys_irq_enable(INTD1_SYS_IRQN));
	zassert_equal(sys_irq_disable(INTD0_SYS_IRQN), 1);
	zassert_equal(sys_irq_disable(INTD1_SYS_IRQN), 1);
}

ZTEST(sys_irq_direct_irq, test__clear_cleared_irq__retval_0)
{
	zassert_equal(sys_irq_clear(INTD0_SYS_IRQN), 0);
	zassert_equal(sys_irq_clear(INTD1_SYS_IRQN), 0);
}

ZTEST(sys_irq_direct_irq, test__clear_triggered_irq__retval_1)
{
	zassert_ok(sys_irq_trigger(INTD0_SYS_IRQN));
	zassert_ok(sys_irq_trigger(INTD1_SYS_IRQN));
	zassert_equal(sys_irq_clear(INTD0_SYS_IRQN), 1);
	zassert_equal(sys_irq_clear(INTD1_SYS_IRQN), 1);
}

void *test_init(void)
{
	zassert_ok(sys_irq_configure(INTD0_SYS_IRQN, INTD0_IRQ_FLAGS));
	zassert_equal(k_sem_take(&irq0_sem, TEST_IRQ_MAX_DELAY), -EAGAIN);
	zassert_ok(sys_irq_configure(INTD1_SYS_IRQN, INTD1_IRQ_FLAGS));
	zassert_equal(k_sem_take(&irq1_sem, TEST_IRQ_MAX_DELAY), -EAGAIN);
	zassert_ok(sys_irq_trigger(INTD0_SYS_IRQN));
	zassert_equal(k_sem_take(&irq0_sem, TEST_IRQ_MAX_DELAY), -EAGAIN);
	zassert_ok(sys_irq_trigger(INTD1_SYS_IRQN));
	zassert_equal(k_sem_take(&irq1_sem, TEST_IRQ_MAX_DELAY), -EAGAIN);
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
}

ZTEST_SUITE(sys_irq_direct_irq, NULL, test_init, test_before, NULL, NULL);
