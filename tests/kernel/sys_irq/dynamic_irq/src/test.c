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

BUILD_ASSERT(INTD0_SYS_IRQN != INTD1_SYS_IRQN, "INTDs must not share INTL");
BUILD_ASSERT(DT_NODE_HAS_STATUS(INTD0_NODE, reserved), "INTD0 must have status reserved");
BUILD_ASSERT(DT_NODE_HAS_STATUS(INTD1_NODE, reserved), "INTD1 must have status reserved");

static K_SEM_DEFINE(irq0_sem, 0, 1);
static K_SEM_DEFINE(irq1_sem, 0, 1);

static uint32_t irq0_data;
static uint32_t irq1_data;
static struct sys_irq irq0;
static struct sys_irq irq1;

static int irq0_handler(const void *data)
{
	zassert_equal(data, &irq0_data);
	k_sem_give(&irq0_sem);
	return SYS_IRQ_HANDLED;
}

static int irq1_handler(const void *data)
{
	zassert_equal(data, &irq1_data);
	k_sem_give(&irq1_sem);
	return SYS_IRQ_HANDLED;
}

ZTEST(sys_irq_irq, test__enable_then_req_then_trigger__irq)
{
	zassert_ok(sys_irq_enable(INTD0_SYS_IRQN));
	zassert_ok(sys_irq_enable(INTD1_SYS_IRQN));
	zassert_ok(sys_irq_request(INTD0_SYS_IRQN, &irq0, irq0_handler, &irq0_data));
	zassert_ok(sys_irq_request(INTD1_SYS_IRQN, &irq1, irq1_handler, &irq1_data));
	zassert_ok(sys_irq_trigger(INTD0_SYS_IRQN));
	zassert_equal(k_sem_take(&irq0_sem, TEST_IRQ_MAX_DELAY), 0);
	zassert_ok(sys_irq_trigger(INTD1_SYS_IRQN));
	zassert_equal(k_sem_take(&irq1_sem, TEST_IRQ_MAX_DELAY), 0);
}

ZTEST(sys_irq_irq, test__req_then_trigger__no_irq)
{
	zassert_ok(sys_irq_request(INTD0_SYS_IRQN, &irq0, irq0_handler, &irq0_data));
	zassert_ok(sys_irq_request(INTD1_SYS_IRQN, &irq1, irq1_handler, &irq1_data));
	zassert_ok(sys_irq_trigger(INTD0_SYS_IRQN));
	zassert_equal(k_sem_take(&irq0_sem, TEST_IRQ_MAX_DELAY), -EAGAIN);
	zassert_ok(sys_irq_trigger(INTD1_SYS_IRQN));
	zassert_equal(k_sem_take(&irq1_sem, TEST_IRQ_MAX_DELAY), -EAGAIN);
}

ZTEST(sys_irq_irq, test__trigger_then_req__no_irq)
{
	zassert_ok(sys_irq_trigger(INTD0_SYS_IRQN));
	zassert_ok(sys_irq_trigger(INTD1_SYS_IRQN));
	zassert_ok(sys_irq_request(INTD0_SYS_IRQN, &irq0, irq0_handler, &irq0_data));
	zassert_equal(k_sem_take(&irq0_sem, TEST_IRQ_MAX_DELAY), -EAGAIN);
	zassert_ok(sys_irq_request(INTD1_SYS_IRQN, &irq1, irq1_handler, &irq1_data));
	zassert_equal(k_sem_take(&irq1_sem, TEST_IRQ_MAX_DELAY), -EAGAIN);
}

ZTEST(sys_irq_irq, test__trigger_then_req_then_enable__no_irq)
{
	zassert_ok(sys_irq_trigger(INTD0_SYS_IRQN));
	zassert_ok(sys_irq_trigger(INTD1_SYS_IRQN));
	zassert_ok(sys_irq_request(INTD0_SYS_IRQN, &irq0, irq0_handler, &irq0_data));
	zassert_equal(k_sem_take(&irq0_sem, TEST_IRQ_MAX_DELAY), -EAGAIN);
	zassert_ok(sys_irq_request(INTD1_SYS_IRQN, &irq1, irq1_handler, &irq1_data));
	zassert_equal(k_sem_take(&irq1_sem, TEST_IRQ_MAX_DELAY), -EAGAIN);
	zassert_ok(sys_irq_enable(INTD0_SYS_IRQN));
	zassert_equal(k_sem_take(&irq0_sem, TEST_IRQ_MAX_DELAY), 0);
	zassert_ok(sys_irq_enable(INTD1_SYS_IRQN));
	zassert_equal(k_sem_take(&irq1_sem, TEST_IRQ_MAX_DELAY), 0);
}

void *test_init(void)
{
	zassert_ok(sys_irq_configure(INTD0_SYS_IRQN, INTD0_IRQ_FLAGS));
	zassert_ok(sys_irq_configure(INTD1_SYS_IRQN, INTD1_IRQ_FLAGS));
	return NULL;
}

void test_before(void *f)
{
	ARG_UNUSED(f);

	k_sem_reset(&irq0_sem);
	k_sem_reset(&irq1_sem);
}

void test_after(void *f)
{
	ARG_UNUSED(f);

	zassert(sys_irq_disable(INTD0_SYS_IRQN) > -1, "retval must be positive");
	zassert(sys_irq_disable(INTD1_SYS_IRQN) > -1, "retval must be positive");
	zassert(sys_irq_clear(INTD0_SYS_IRQN) > -1, "retval must be positive");
	zassert(sys_irq_clear(INTD1_SYS_IRQN) > -1, "retval must be positive");
	zassert_ok(sys_irq_release(INTD0_SYS_IRQN, &irq0));
	zassert_ok(sys_irq_release(INTD1_SYS_IRQN, &irq1));
}

ZTEST_SUITE(sys_irq_dynamic_irq, NULL, test_init, test_before, test_after, NULL);
