/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio/gpio_irq.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define TEST_DUT0_IRQ0_INTC  (DEVICE_DT_GET(DT_NODELABEL(gpio0)))
#define TEST_DUT0_IRQ0_PIN   (4)
#define TEST_DUT0_IRQ0_FLAGS (IRQ_TYPE_EDGE_BOTH)

#define TEST_DUT0_IRQ1_INTC  (DEVICE_DT_GET(DT_NODELABEL(gpio0)))
#define TEST_DUT0_IRQ1_PIN   (5)
#define TEST_DUT0_IRQ1_FLAGS (GPIO_PULL_UP | IRQ_TYPE_EDGE_FALLING)

#define TEST_DUT1_IRQ_DRDY_INTC  (DEVICE_DT_GET(DT_NODELABEL(gpio0)))
#define TEST_DUT1_IRQ_DRDY_PIN   (3)
#define TEST_DUT1_IRQ_DRDY_FLAGS (IRQ_TYPE_EDGE_RISING)

#define TEST_DUT1_IRQ_INT1_INTC  (DEVICE_DT_GET(DT_NODELABEL(gpio1)))
#define TEST_DUT1_IRQ_INT1_PIN   (6)
#define TEST_DUT1_IRQ_INT1_FLAGS (GPIO_PULL_DOWN | IRQ_TYPE_LEVEL_HIGH)

const struct gpio_irq_dt_spec dut0_irq0 =
	GPIO_IRQ_DT_SPEC_GET(DT_NODELABEL(dut0));

const struct gpio_irq_dt_spec dut0_irq0_opt =
	GPIO_IRQ_DT_SPEC_GET_OPT(DT_NODELABEL(dut0));

const struct gpio_irq_dt_spec dut0_irq1 =
	GPIO_IRQ_DT_SPEC_GET_BY_IDX(DT_NODELABEL(dut0), 1);

const struct gpio_irq_dt_spec dut0_irq1_opt =
	GPIO_IRQ_DT_SPEC_GET_OPT_BY_IDX(DT_NODELABEL(dut0), 1);

const struct gpio_irq_dt_spec dut0_irq2_opt =
	GPIO_IRQ_DT_SPEC_GET_OPT_BY_IDX(DT_NODELABEL(dut0), 2);

const struct gpio_irq_dt_spec dut1_irq_drdy =
	GPIO_IRQ_DT_SPEC_GET_BY_NAME(DT_NODELABEL(dut1), drdy);

const struct gpio_irq_dt_spec dut1_irq_int1 =
	GPIO_IRQ_DT_SPEC_GET_BY_NAME(DT_NODELABEL(dut1), int1);

const struct gpio_irq_dt_spec dut1_irq_int1_opt =
	GPIO_IRQ_DT_SPEC_GET_OPT_BY_NAME(DT_NODELABEL(dut1), int1);

const struct gpio_irq_dt_spec dut1_irq_int2_opt =
	GPIO_IRQ_DT_SPEC_GET_OPT_BY_NAME(DT_NODELABEL(dut1), int2);

#define DT_DRV_COMPAT vnd_interrupt_holder
const struct gpio_irq_dt_spec dut0_inst0_irq0 =
	GPIO_IRQ_DT_INST_SPEC_GET(0);

const struct gpio_irq_dt_spec dut0_inst0_irq0_opt =
	GPIO_IRQ_DT_INST_SPEC_GET_OPT(0);

const struct gpio_irq_dt_spec dut0_inst0_irq1 =
	GPIO_IRQ_DT_INST_SPEC_GET_BY_IDX(0, 1);

const struct gpio_irq_dt_spec dut0_inst0_irq1_opt =
	GPIO_IRQ_DT_INST_SPEC_GET_OPT_BY_IDX(0, 1);

const struct gpio_irq_dt_spec dut0_inst0_irq2_opt =
	GPIO_IRQ_DT_INST_SPEC_GET_OPT_BY_IDX(0, 2);
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT vnd_interrupt_holder_extended
const struct gpio_irq_dt_spec dut1_inst0_irq_drdy =
	GPIO_IRQ_DT_INST_SPEC_GET_BY_NAME(0, drdy);

const struct gpio_irq_dt_spec dut1_inst0_irq_int1 =
	GPIO_IRQ_DT_INST_SPEC_GET_BY_NAME(0, int1);

const struct gpio_irq_dt_spec dut1_inst0_irq_int1_opt =
	GPIO_IRQ_DT_INST_SPEC_GET_OPT_BY_NAME(0, int1);

const struct gpio_irq_dt_spec dut1_inst0_irq_int2_opt =
	GPIO_IRQ_DT_INST_SPEC_GET_OPT_BY_NAME(0, int2);
#undef DT_DRV_COMPAT

static struct gpio_irq test_irq;
static K_SEM_DEFINE(test_irq_called, 1, 1);

static void gpio_irq_callback_handler(struct gpio_irq *irq)
{
	__ASSERT(irq == &test_irq, "Incorrect irq passed to callback");
	k_sem_give(&test_irq_called);
}

ZTEST(gpio_irq, test_validate_gpio_irq_dt_spec_get_macros)
{
	zassert_equal(dut0_irq0.controller, TEST_DUT0_IRQ0_INTC,
		      "Incorrect interrupt controller");
	zassert_equal(dut0_irq0.irq_pin, TEST_DUT0_IRQ0_PIN,
		      "Incorrect pin");
	zassert_equal(dut0_irq0.irq_flags, TEST_DUT0_IRQ0_FLAGS,
		      "Incorrect flags");

	zassert_equal(dut0_irq1.controller, TEST_DUT0_IRQ1_INTC,
		      "Incorrect interrupt controller");
	zassert_equal(dut0_irq1.irq_pin, TEST_DUT0_IRQ1_PIN,
		      "Incorrect pin");
	zassert_equal(dut0_irq1.irq_flags, TEST_DUT0_IRQ1_FLAGS,
		      "Incorrect flags");

	zassert_equal(dut1_irq_drdy.controller, TEST_DUT1_IRQ_DRDY_INTC,
		      "Incorrect interrupt controller");
	zassert_equal(dut1_irq_drdy.irq_pin, TEST_DUT1_IRQ_DRDY_PIN,
		      "Incorrect pin");
	zassert_equal(dut1_irq_drdy.irq_flags, TEST_DUT1_IRQ_DRDY_FLAGS,
		      "Incorrect flags");

	zassert_equal(dut1_irq_int1.controller, TEST_DUT1_IRQ_INT1_INTC,
		      "Incorrect interrupt controller");
	zassert_equal(dut1_irq_int1.irq_pin, TEST_DUT1_IRQ_INT1_PIN,
		      "Incorrect pin");
	zassert_equal(dut1_irq_int1.irq_flags, TEST_DUT1_IRQ_INT1_FLAGS,
		      "Incorrect flags");
}

ZTEST(gpio_irq, test_request_irq)
{
	int ret;

	ret = gpio_irq_request_dt(&dut0_irq0, &test_irq, gpio_irq_callback_handler);
	zassert_ok(ret, "Failed to request GPIO, ret %i", ret);

	k_sem_reset(&test_irq_called);
	gpio_emul_input_set(TEST_DUT0_IRQ0_INTC, TEST_DUT0_IRQ0_PIN, 0);
	gpio_emul_input_set(TEST_DUT0_IRQ0_INTC, TEST_DUT0_IRQ0_PIN, 1);
	ret = k_sem_take(&test_irq_called, K_MSEC(100));
	zassert_ok(ret, "IRQ calllback not called after request, ret %i", ret);

	ret = gpio_irq_disable(&test_irq);
	zassert_ok(ret, "Failed to disable IRQ, ret %i", ret);

	k_sem_reset(&test_irq_called);
	gpio_emul_input_set(TEST_DUT0_IRQ0_INTC, TEST_DUT0_IRQ0_PIN, 0);
	ret = k_sem_take(&test_irq_called, K_MSEC(100));
	zassert_equal(ret, -EAGAIN, "IRQ calllback called after disabled, ret %i", ret);

	ret = gpio_irq_enable(&test_irq);
	zassert_ok(ret, "Failed to enable IRQ, %i", ret);

	k_sem_reset(&test_irq_called);
	gpio_emul_input_set(TEST_DUT0_IRQ0_INTC, TEST_DUT0_IRQ0_PIN, 1);
	ret = k_sem_take(&test_irq_called, K_MSEC(100));
	zassert_ok(ret, "IRQ calllback not called while IRQ enabled after enable, ret %i", ret);

	ret = gpio_irq_release(&test_irq);
	zassert_ok(ret, "Failed to release IRQ, ret %i", ret);

	k_sem_reset(&test_irq_called);
	gpio_emul_input_set(TEST_DUT0_IRQ0_INTC, TEST_DUT0_IRQ0_PIN, 1);
	ret = k_sem_take(&test_irq_called, K_MSEC(100));
	zassert_equal(ret, -EAGAIN, "IRQ calllback called after IRQ released, ret %i", ret);
}

static const struct gpio_irq_dt_spec zero_dt_spec;

ZTEST(gpio_irq, test_validate_gpio_irq_dt_spec_get_OPT_macros)
{
	zassert_mem_equal(&dut0_irq0, &dut0_irq0_opt,
			  sizeof(struct gpio_irq_dt_spec),
			  "Optional GPIO IRQ DT spec not equal");

	zassert_mem_equal(&dut0_irq1, &dut0_irq1_opt,
			  sizeof(struct gpio_irq_dt_spec),
			  "Optional GPIO IRQ DT spec not equal");

	zassert_mem_equal(&dut0_irq2_opt, &zero_dt_spec,
			  sizeof(struct gpio_irq_dt_spec),
			  "Optional GPIO IRQ DT spec should be zero");

	zassert_mem_equal(&dut1_irq_int1, &dut1_irq_int1_opt,
			  sizeof(struct gpio_irq_dt_spec),
			  "Optional GPIO IRQ DT spec not equal");

	zassert_mem_equal(&dut1_irq_int2_opt, &zero_dt_spec,
			  sizeof(struct gpio_irq_dt_spec),
			  "Optional GPIO IRQ DT spec should be zero");
}

ZTEST(gpio_irq, test_validate_gpio_irq_dt_inst_spec_get_macros)
{
	zassert_mem_equal(&dut0_irq0, &dut0_inst0_irq0,
			  sizeof(struct gpio_irq_dt_spec),
			  "instance not equal to node");

	zassert_mem_equal(&dut0_irq0_opt, &dut0_inst0_irq0_opt,
			  sizeof(struct gpio_irq_dt_spec),
			  "instance not equal to node");

	zassert_mem_equal(&dut0_irq1, &dut0_inst0_irq1,
			  sizeof(struct gpio_irq_dt_spec),
			  "instance not equal to node");

	zassert_mem_equal(&dut0_irq1_opt, &dut0_inst0_irq1_opt,
			  sizeof(struct gpio_irq_dt_spec),
			  "instance not equal to node");

	zassert_mem_equal(&dut0_irq2_opt, &dut0_inst0_irq2_opt,
			  sizeof(struct gpio_irq_dt_spec),
			  "instance not equal to node");

	zassert_mem_equal(&dut0_irq0, &dut0_inst0_irq0,
			  sizeof(struct gpio_irq_dt_spec),
			  "instance not equal to node");

	zassert_mem_equal(&dut1_irq_drdy, &dut1_inst0_irq_drdy,
			  sizeof(struct gpio_irq_dt_spec),
			  "instance not equal to node");

	zassert_mem_equal(&dut1_irq_int1, &dut1_inst0_irq_int1,
			  sizeof(struct gpio_irq_dt_spec),
			  "instance not equal to node");

	zassert_mem_equal(&dut1_irq_int1_opt, &dut1_inst0_irq_int1_opt,
			  sizeof(struct gpio_irq_dt_spec),
			  "instance not equal to node");

	zassert_mem_equal(&dut1_irq_int2_opt, &dut1_inst0_irq_int2_opt,
			  sizeof(struct gpio_irq_dt_spec),
			  "instance not equal to node");
}

ZTEST(gpio_irq, test_validate_gpio_irq_dt_spec_exists)
{
	zassert_true(gpio_irq_dt_spec_exists(&dut0_irq0), "");
	zassert_true(gpio_irq_dt_spec_exists(&dut0_irq1), "");
	zassert_false(gpio_irq_dt_spec_exists(&dut0_irq2_opt), "");
	zassert_true(gpio_irq_dt_spec_exists(&dut1_irq_drdy), "");
	zassert_true(gpio_irq_dt_spec_exists(&dut1_irq_int1), "");
	zassert_false(gpio_irq_dt_spec_exists(&dut1_irq_int2_opt), "");
}

ZTEST(gpio_irq, test_fail_to_request_non_existent_irq)
{
	int ret;

	ret = gpio_irq_request_dt(&dut0_irq2_opt, &test_irq,
				  gpio_irq_callback_handler);
	zassert_equal(ret, -ENODEV, "Should have failed to request IRQ");
}

ZTEST_SUITE(gpio_irq, NULL, NULL, NULL, NULL, NULL);
