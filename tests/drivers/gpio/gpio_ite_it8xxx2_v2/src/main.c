/*
 * Copyright 2023 The ChromiumOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/ite-it8xxx2-gpio.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#define MY_GPIO DT_NODELABEL(gpioa)

const struct device *const gpio_dev = DEVICE_DT_GET(MY_GPIO);
static struct {
	uint8_t fake;
	uint8_t gpdmr;
	uint8_t gpdr;
	uint8_t gpotr;
	uint8_t p18scr;
	uint8_t wuemr, wuesr, wubemr;
	uint8_t gpcr[DT_REG_SIZE_BY_IDX(MY_GPIO, 4)];
	bool clear_gpcr_before_read;
} registers;
static int callback_called;
static struct gpio_callback callback_struct;

/* These values must match what is set in the dts overlay. */
#define TEST_PIN  1
#define TEST_IRQ  DT_IRQ_BY_IDX(MY_GPIO, TEST_PIN, irq)
#define TEST_MASK DT_PROP_BY_IDX(MY_GPIO, wuc_mask, TEST_PIN)

DEFINE_FFF_GLOBALS;

uint8_t ite_intc_get_irq_num(void)
{
	return posix_get_current_irq();
}

unsigned int *fake_ecreg(intptr_t r)
{
	switch (r) {
	case DT_REG_ADDR_BY_IDX(MY_GPIO, 0): /* GPDR */
		return (unsigned int *)&registers.gpdr;
	case DT_REG_ADDR_BY_IDX(MY_GPIO, 1): /* GPDMR */
		return (unsigned int *)&registers.gpdmr;
	case DT_REG_ADDR_BY_IDX(MY_GPIO, 2): /* GPOTR */
		return (unsigned int *)&registers.gpotr;
	case DT_REG_ADDR_BY_IDX(MY_GPIO, 3): /* P18SCR */
		return (unsigned int *)&registers.p18scr;
	case DT_PROP_BY_IDX(MY_GPIO, wuc_base, TEST_PIN):
		return (unsigned int *)&registers.wuemr;
	case DT_PROP_BY_IDX(MY_GPIO, wuc_base, TEST_PIN) + 1:
		return (unsigned int *)&registers.wuesr;
	case DT_PROP_BY_IDX(MY_GPIO, wuc_base, TEST_PIN) + 3:
		return (unsigned int *)&registers.wubemr;
	}
	if (r >= DT_REG_ADDR_BY_IDX(MY_GPIO, 4) &&
	    r < DT_REG_ADDR_BY_IDX(MY_GPIO, 4) + DT_REG_SIZE_BY_IDX(MY_GPIO, 4)) {
		if (registers.clear_gpcr_before_read) {
			registers.gpcr[r - DT_REG_ADDR_BY_IDX(MY_GPIO, 4)] = 0;
		}
		return (unsigned int *)&registers.gpcr[r - DT_REG_ADDR_BY_IDX(MY_GPIO, 4)];
	}
	zassert_unreachable("Register access: %x", r);
	return (unsigned int *)&registers.fake;
}

static void callback(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	callback_called++;
	zexpect_equal(pins, BIT(TEST_PIN));

	/* If the callback has been called 5 or more times, toggle the pin in the input register. */
	if (callback_called >= 5) {
		registers.gpdmr ^= pins;
	}
}

static void before_test(void *fixture)
{
	callback_called = 0;
	memset(&registers, 0, sizeof(registers));
}

static void after_test(void *fixture)
{
	if (callback_struct.handler != NULL) {
		zassert_ok(gpio_remove_callback(gpio_dev, &callback_struct));
	}
	callback_struct.handler = NULL;
}

ZTEST_SUITE(gpio_ite_it8xxx2_v2, NULL, NULL, before_test, after_test, NULL);

ZTEST(gpio_ite_it8xxx2_v2, test_get_active_high)
{
	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_INPUT | GPIO_ACTIVE_HIGH));
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], GPCR_PORT_PIN_MODE_INPUT, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);
	registers.gpdmr = (uint8_t)~BIT(TEST_PIN);
	zassert_false(gpio_pin_get(gpio_dev, TEST_PIN));
	registers.gpdmr = BIT(TEST_PIN);
	zassert_true(gpio_pin_get(gpio_dev, TEST_PIN));
}

ZTEST(gpio_ite_it8xxx2_v2, test_get_active_low)
{
	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_INPUT | GPIO_ACTIVE_LOW));
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], GPCR_PORT_PIN_MODE_INPUT, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);
	registers.gpdmr = (uint8_t)~BIT(TEST_PIN);
	zassert_true(gpio_pin_get(gpio_dev, TEST_PIN));
	registers.gpdmr = BIT(TEST_PIN);
	zassert_false(gpio_pin_get(gpio_dev, TEST_PIN));
}

ZTEST(gpio_ite_it8xxx2_v2, test_interrupt_edge_rising)
{
	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_INPUT | GPIO_ACTIVE_HIGH));

	gpio_init_callback(&callback_struct, &callback, BIT(TEST_PIN));
	zassert_ok(gpio_add_callback(gpio_dev, &callback_struct));
	zassert_ok(gpio_pin_interrupt_configure(gpio_dev, TEST_PIN, GPIO_INT_EDGE_TO_ACTIVE));
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], GPCR_PORT_PIN_MODE_INPUT, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);
	zexpect_equal(registers.wubemr, 0, "wubemr=%x", registers.wubemr);
	zexpect_equal(registers.wuemr, 0, "wuemr=%x", registers.wuemr);
	zexpect_equal(registers.wuesr, TEST_MASK, "wuesr=%x", registers.wuesr);
	registers.wuesr = 0;

	registers.gpdmr = BIT(TEST_PIN);
	/* Mock the hardware interrupt. */
	posix_sw_set_pending_IRQ(TEST_IRQ);
	k_sleep(K_MSEC(100));
	zassert_equal(callback_called, 1, "callback_called=%d", callback_called);
}

ZTEST(gpio_ite_it8xxx2_v2, test_interrupt_enable_disable)
{
	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_INPUT | GPIO_ACTIVE_HIGH));

	gpio_init_callback(&callback_struct, &callback, BIT(TEST_PIN));
	zassert_ok(gpio_add_callback(gpio_dev, &callback_struct));
	zassert_ok(gpio_pin_interrupt_configure(gpio_dev, TEST_PIN, GPIO_INT_EDGE_TO_ACTIVE));
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], GPCR_PORT_PIN_MODE_INPUT, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);
	zexpect_equal(registers.wubemr, 0, "wubemr=%x", registers.wubemr);
	zexpect_equal(registers.wuemr, 0, "wuemr=%x", registers.wuemr);
	zexpect_equal(registers.wuesr, TEST_MASK, "wuesr=%x", registers.wuesr);
	registers.wuesr = 0;

	registers.gpdmr = BIT(TEST_PIN);
	/* Mock the hardware interrupt. */
	posix_sw_set_pending_IRQ(TEST_IRQ);
	k_sleep(K_MSEC(100));
	zassert_equal(callback_called, 1, "callback_called=%d", callback_called);
	registers.gpdmr = 0;

	zassert_ok(gpio_pin_interrupt_configure(gpio_dev, TEST_PIN, GPIO_INT_MODE_DISABLED));
	registers.gpdmr = BIT(TEST_PIN);
	/* Mock the hardware interrupt, should be ignored */
	posix_sw_set_pending_IRQ(TEST_IRQ);
	k_sleep(K_MSEC(100));
	zassert_equal(callback_called, 1, "callback_called=%d", callback_called);
	/* Clear the missed interrupt */
	posix_sw_clear_pending_IRQ(TEST_IRQ);
	registers.gpdmr = 0;

	zassert_ok(gpio_pin_interrupt_configure(gpio_dev, TEST_PIN, GPIO_INT_EDGE_TO_ACTIVE));
	registers.gpdmr = BIT(TEST_PIN);
	/* Mock the hardware interrupt */
	posix_sw_set_pending_IRQ(TEST_IRQ);
	k_sleep(K_MSEC(100));
	zassert_equal(callback_called, 2, "callback_called=%d", callback_called);
}

ZTEST(gpio_ite_it8xxx2_v2, test_interrupt_edge_falling)
{
	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_INPUT | GPIO_ACTIVE_HIGH));

	gpio_init_callback(&callback_struct, &callback, BIT(TEST_PIN));
	zassert_ok(gpio_add_callback(gpio_dev, &callback_struct));
	zassert_ok(gpio_pin_interrupt_configure(gpio_dev, TEST_PIN, GPIO_INT_EDGE_TO_INACTIVE));
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], GPCR_PORT_PIN_MODE_INPUT, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);
	zexpect_equal(registers.wubemr, 0, "wubemr=%x", registers.wubemr);
	zexpect_equal(registers.wuemr, TEST_MASK, "wuemr=%x", registers.wuemr);
	zexpect_equal(registers.wuesr, TEST_MASK, "wuesr=%x", registers.wuesr);
	registers.wuesr = 0;

	registers.gpdmr = (uint8_t)~BIT(TEST_PIN);
	/* Mock the hardware interrupt. */
	posix_sw_set_pending_IRQ(TEST_IRQ);
	k_sleep(K_MSEC(100));
	zassert_equal(callback_called, 1, "callback_called=%d", callback_called);
}

ZTEST(gpio_ite_it8xxx2_v2, test_interrupt_edge_both)
{
	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_INPUT | GPIO_ACTIVE_HIGH));

	gpio_init_callback(&callback_struct, &callback, BIT(TEST_PIN));
	zassert_ok(gpio_add_callback(gpio_dev, &callback_struct));
	zassert_ok(gpio_pin_interrupt_configure(gpio_dev, TEST_PIN, GPIO_INT_EDGE_BOTH));
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], GPCR_PORT_PIN_MODE_INPUT, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);
	zexpect_equal(registers.wubemr, TEST_MASK, "wubemr=%x", registers.wubemr);
	zexpect_equal(registers.wuemr, TEST_MASK, "wuemr=%x", registers.wuemr);
	zexpect_equal(registers.wuesr, TEST_MASK, "wuesr=%x", registers.wuesr);
	registers.wuesr = 0;

	registers.gpdmr = BIT(TEST_PIN);
	/* Mock the hardware interrupt. */
	posix_sw_set_pending_IRQ(TEST_IRQ);
	k_sleep(K_MSEC(100));
	zassert_equal(callback_called, 1, "callback_called=%d", callback_called);
	registers.gpdmr &= ~BIT(TEST_PIN);
	/* Mock the hardware interrupt. */
	posix_sw_set_pending_IRQ(TEST_IRQ);
	k_sleep(K_MSEC(100));
	zassert_equal(callback_called, 2, "callback_called=%d", callback_called);
}

/* Tests both the active level case and the interrupt not firing at configure case. */
ZTEST(gpio_ite_it8xxx2_v2, test_interrupt_level_active)
{
	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_INPUT | GPIO_ACTIVE_HIGH));

	gpio_init_callback(&callback_struct, &callback, BIT(TEST_PIN));
	zassert_ok(gpio_add_callback(gpio_dev, &callback_struct));
	zassert_ok(gpio_pin_interrupt_configure(gpio_dev, TEST_PIN, GPIO_INT_LEVEL_ACTIVE));
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], GPCR_PORT_PIN_MODE_INPUT, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);
	zexpect_equal(registers.wubemr, 0, "wubemr=%x", registers.wubemr);
	zexpect_equal(registers.wuemr, 0, "wuemr=%x", registers.wuemr);
	zexpect_equal(registers.wuesr, TEST_MASK, "wuesr=%x", registers.wuesr);
	registers.wuesr = 0;
	k_sleep(K_MSEC(100));
	zexpect_equal(callback_called, 0, "callback_called=%d", callback_called);

	registers.gpdmr = BIT(TEST_PIN);
	/* Mock the hardware interrupt. */
	posix_sw_set_pending_IRQ(TEST_IRQ);
	k_sleep(K_MSEC(100));
	zexpect_equal(callback_called, 5, "callback_called=%d", callback_called);
}

/* Tests both the inactive level case and the interrupt already firing at configure case. */
ZTEST(gpio_ite_it8xxx2_v2, test_interrupt_level_inactive)
{
	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_INPUT | GPIO_ACTIVE_HIGH));

	gpio_init_callback(&callback_struct, &callback, BIT(TEST_PIN));
	zassert_ok(gpio_add_callback(gpio_dev, &callback_struct));
	zassert_ok(gpio_pin_interrupt_configure(gpio_dev, TEST_PIN, GPIO_INT_LEVEL_INACTIVE));
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], GPCR_PORT_PIN_MODE_INPUT, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);
	zexpect_equal(registers.wubemr, 0, "wubemr=%x", registers.wubemr);
	zexpect_equal(registers.wuemr, TEST_MASK, "wuemr=%x", registers.wuemr);
	zexpect_equal(registers.wuesr, TEST_MASK, "wuesr=%x", registers.wuesr);
	registers.wuesr = 0;
	k_sleep(K_MSEC(100));
	/* The interrupt was already active when we started. */
	zexpect_equal(callback_called, 5, "callback_called=%d", callback_called);

	registers.gpdmr = 0;
	callback_called = 0;
	/* Mock the hardware interrupt. */
	posix_sw_set_pending_IRQ(TEST_IRQ);
	k_sleep(K_MSEC(100));
	zexpect_equal(callback_called, 5, "callback_called=%d", callback_called);
}

ZTEST(gpio_ite_it8xxx2_v2, test_set_active_high)
{
	gpio_flags_t flags;

	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_HIGH));
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], GPCR_PORT_PIN_MODE_OUTPUT, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);

	zexpect_equal(registers.gpdr, 0, "gpdr=%x", registers.gpdr);
	zassert_ok(gpio_pin_set(gpio_dev, TEST_PIN, true));
	zexpect_equal(registers.gpdr, BIT(TEST_PIN), "gpdr=%x", registers.gpdr);
	zassert_ok(gpio_pin_set(gpio_dev, TEST_PIN, false));
	zexpect_equal(registers.gpdr, 0, "gpdr=%x", registers.gpdr);
	zassert_ok(gpio_port_toggle_bits(gpio_dev, BIT(TEST_PIN)));
	zexpect_equal(registers.gpdr, BIT(TEST_PIN), "gpdr=%x", registers.gpdr);
	registers.gpdr = 0;
	zassert_ok(gpio_port_set_masked(gpio_dev, BIT(TEST_PIN), 255));
	zexpect_equal(registers.gpdr, BIT(TEST_PIN), "gpdr=%x", registers.gpdr);
	registers.gpdr = 255;
	zassert_ok(gpio_port_set_masked(gpio_dev, BIT(TEST_PIN), 0));
	zexpect_equal(registers.gpdr, (uint8_t)~BIT(TEST_PIN), "gpdr=%x", registers.gpdr);

	registers.gpdr = BIT(TEST_PIN);
	zassert_ok(gpio_pin_get_config(gpio_dev, TEST_PIN, &flags));
	zexpect_equal(flags, GPIO_OUTPUT_HIGH, "flags=%x", flags);
	registers.gpdr = 0;
	zassert_ok(gpio_pin_get_config(gpio_dev, TEST_PIN, &flags));
	zexpect_equal(flags, GPIO_OUTPUT_LOW, "flags=%x", flags);
}

ZTEST(gpio_ite_it8xxx2_v2, test_set_active_low)
{
	gpio_flags_t flags;

	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_LOW));
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], GPCR_PORT_PIN_MODE_OUTPUT, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);

	zexpect_equal(registers.gpdr, BIT(TEST_PIN), "gpdr=%x", registers.gpdr);
	zassert_ok(gpio_pin_set(gpio_dev, TEST_PIN, true));
	zexpect_equal(registers.gpdr, 0, "gpdr=%x", registers.gpdr);
	zassert_ok(gpio_pin_set(gpio_dev, TEST_PIN, false));
	zexpect_equal(registers.gpdr, BIT(TEST_PIN), "gpdr=%x", registers.gpdr);
	zassert_ok(gpio_port_toggle_bits(gpio_dev, BIT(TEST_PIN)));
	zexpect_equal(registers.gpdr, 0, "gpdr=%x", registers.gpdr);
	registers.gpdr = 255;
	zassert_ok(gpio_port_set_masked(gpio_dev, BIT(TEST_PIN), 255));
	zexpect_equal(registers.gpdr, (uint8_t)~BIT(TEST_PIN), "gpdr=%x", registers.gpdr);
	registers.gpdr = 0;
	zassert_ok(gpio_port_set_masked(gpio_dev, BIT(TEST_PIN), 0));
	zexpect_equal(registers.gpdr, BIT(TEST_PIN), "gpdr=%x", registers.gpdr);

	registers.gpdr = 0;
	zassert_ok(gpio_pin_get_config(gpio_dev, TEST_PIN, &flags));
	zexpect_equal(flags, GPIO_OUTPUT_LOW, "flags=%x", flags);
	registers.gpdr = BIT(TEST_PIN);
	zassert_ok(gpio_pin_get_config(gpio_dev, TEST_PIN, &flags));
	zexpect_equal(flags, GPIO_OUTPUT_HIGH, "flags=%x", flags);
}

/* The next few tests just verify that the registers are set as expected on configure. */

ZTEST(gpio_ite_it8xxx2_v2, test_open_source)
{
	zassert_true(device_is_ready(gpio_dev));
	zassert_equal(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_OPEN_SOURCE), -ENOTSUP);
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], 0, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);
}

ZTEST(gpio_ite_it8xxx2_v2, test_open_drain_output)
{
	gpio_flags_t flags;

	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_OUTPUT | GPIO_OPEN_DRAIN));
	zexpect_equal(registers.gpotr, BIT(TEST_PIN), "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], GPCR_PORT_PIN_MODE_OUTPUT, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);

	zassert_ok(gpio_pin_get_config(gpio_dev, TEST_PIN, &flags));
	zexpect_equal(flags, GPIO_OUTPUT_LOW | GPIO_OPEN_DRAIN, "flags=%x", flags);
}

ZTEST(gpio_ite_it8xxx2_v2, test_pull_up_input)
{
	gpio_flags_t flags;

	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_INPUT | GPIO_PULL_UP));
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN],
		      GPCR_PORT_PIN_MODE_INPUT | GPCR_PORT_PIN_MODE_PULLUP, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);

	zassert_ok(gpio_pin_get_config(gpio_dev, TEST_PIN, &flags));
	zexpect_equal(flags, GPIO_INPUT | GPIO_PULL_UP, "flags=%x", flags);
}

ZTEST(gpio_ite_it8xxx2_v2, test_pull_down_input)
{
	gpio_flags_t flags;

	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_INPUT | GPIO_PULL_DOWN));
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN],
		      GPCR_PORT_PIN_MODE_INPUT | GPCR_PORT_PIN_MODE_PULLDOWN, "gpcr[%d]=%x",
		      TEST_PIN, registers.gpcr[TEST_PIN]);

	zassert_ok(gpio_pin_get_config(gpio_dev, TEST_PIN, &flags));
	zexpect_equal(flags, GPIO_INPUT | GPIO_PULL_DOWN, "flags=%x", flags);
}

ZTEST(gpio_ite_it8xxx2_v2, test_disconnected_tristate_supported)
{
	gpio_flags_t flags;

	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_DISCONNECTED));
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], GPCR_PORT_PIN_MODE_TRISTATE, "gpcr[%d]=%x",
		      TEST_PIN, registers.gpcr[TEST_PIN]);

	zassert_ok(gpio_pin_get_config(gpio_dev, TEST_PIN, &flags));
	zexpect_equal(flags, GPIO_PULL_UP | GPIO_PULL_DOWN | GPIO_INPUT | IT8XXX2_GPIO_VOLTAGE_3P3,
		      "flags=%x", flags);
}

ZTEST(gpio_ite_it8xxx2_v2, test_disconnected_tristate_unsupported)
{
	registers.clear_gpcr_before_read = true;
	zassert_true(device_is_ready(gpio_dev));
	zassert_equal(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_DISCONNECTED), -ENOTSUP);
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], GPCR_PORT_PIN_MODE_INPUT, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);
}

ZTEST(gpio_ite_it8xxx2_v2, test_input_1P8V)
{
	gpio_flags_t flags;

	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_INPUT | IT8XXX2_GPIO_VOLTAGE_1P8));
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, BIT(TEST_PIN));
	zexpect_equal(registers.gpcr[TEST_PIN], GPCR_PORT_PIN_MODE_INPUT, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);

	zassert_ok(gpio_pin_get_config(gpio_dev, TEST_PIN, &flags));
	zexpect_equal(flags, GPIO_INPUT | IT8XXX2_GPIO_VOLTAGE_1P8, "flags=%x", flags);
}

ZTEST(gpio_ite_it8xxx2_v2, test_input_3P3V)
{
	gpio_flags_t flags;

	zassert_true(device_is_ready(gpio_dev));
	zassert_ok(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_INPUT | IT8XXX2_GPIO_VOLTAGE_3P3));
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], GPCR_PORT_PIN_MODE_INPUT, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);

	zassert_ok(gpio_pin_get_config(gpio_dev, TEST_PIN, &flags));
	zexpect_equal(flags, GPIO_INPUT | IT8XXX2_GPIO_VOLTAGE_3P3, "flags=%x", flags);
}

ZTEST(gpio_ite_it8xxx2_v2, test_input_5V)
{
	zassert_true(device_is_ready(gpio_dev));
	zassert_equal(gpio_pin_configure(gpio_dev, TEST_PIN, GPIO_INPUT | IT8XXX2_GPIO_VOLTAGE_5P0),
		      -EINVAL);
	zexpect_equal(registers.gpotr, 0, "gpotr=%x", registers.gpotr);
	zexpect_equal(registers.p18scr, 0);
	zexpect_equal(registers.gpcr[TEST_PIN], 0, "gpcr[%d]=%x", TEST_PIN,
		      registers.gpcr[TEST_PIN]);
}
