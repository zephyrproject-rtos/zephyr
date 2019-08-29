/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_gpio.h"

static struct device *dev;
static gpio_port_pins_t out_pins;
static gpio_port_pins_t in_pins;
static gpio_port_pins_t os1_pins; /* active high pins */
static gpio_port_pins_t os2_pins; /* active low pins */

static void measure(const char* tag,
		    void (*op)(void))
{
	u32_t ts = k_cycle_get_32();
	u32_t const dur_cyc = sys_clock_hw_cycles_per_sec() / 2;
	u32_t te;
	unsigned int count = 0;

	do {
		op();
		te = k_cycle_get_32();
		++count;
	} while ((te - ts) < dur_cyc);


	gpio_port_clear_bits_raw(dev, out_pins);

	u64_t nom = (te - ts) * (u64_t)NSEC_PER_SEC;
	u64_t div = count * (u64_t)sys_clock_hw_cycles_per_sec();
	u32_t op_us = ceiling_fraction(nom, div);

	TC_PRINT("- %s : %u iterations in %u cycles : %u ns / op\n",
		 tag, count, te - ts, op_us);
}

static void iter_nop(void)
{
}

static void iter_s1(void)
{
	/* All pins high */
	gpio_port_set_bits_raw(dev, out_pins);

	/* All pins inactive: s1 goes low, s2 stays high */
	gpio_port_clear_bits(dev, out_pins);

	/* s1 goes high, s2 stays high */
	gpio_port_set_bits(dev, os1_pins);

	/* s1 stays high, s2 goes low */
	gpio_port_set_bits(dev, os2_pins);

	/* toggle: s1 low, s2 high */
	gpio_port_toggle_bits(dev, out_pins);

	/* OUT high, rest unchanged */
	gpio_pin_set(dev, PIN_OUT, 1);

	/* All pins low */
	gpio_port_clear_bits_raw(dev, out_pins);
}

static int test_perf(void)
{
	static const struct scope_type {
		const char* devname;
		u8_t pin;
	} scope_pins[] = {
#ifdef DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_PIN_2
		{ DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_CONTROLLER_2,
		  DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_PIN_2 },
#endif
#ifdef DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_PIN_3
		{ DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_CONTROLLER_3,
		  DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_PIN_3 },
#endif
#ifdef DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_PIN_4
		{ DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_CONTROLLER_4,
		  DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_PIN_4 },
#endif
#ifdef DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_PIN_5
		{ DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_CONTROLLER_5,
		  DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_PIN_5 },
#endif
#ifdef DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_PIN_6
		{ DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_CONTROLLER_6,
		  DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_PIN_6 },
#endif
#ifdef DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_PIN_7
		{ DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_CONTROLLER_7,
		  DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_PIN_7 },
#endif
#ifdef DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_PIN_8
		{ DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_CONTROLLER_8,
		  DT_TEST_SCOPE_PINS_SCOPE_PINS_GPIOS_PIN_8 },
#endif
	};
	const struct scope_type *sp = scope_pins;
	const struct scope_type *const spe = sp + sizeof(scope_pins) / sizeof(*scope_pins);
	size_t nout = 1;

	out_pins = BIT(PIN_OUT);
	dev = device_get_binding(DEV_NAME);
	zassert_not_equal(dev, NULL,
			  "Device not found");

	zassert_equal(gpio_pin_configure(dev, PIN_OUT, GPIO_OUTPUT_LOW), 0,
		      "PIN_OUT configure failed");
	zassert_equal(gpio_pin_configure(dev, PIN_IN, GPIO_INPUT), 0,
		      "PIN_OUT configure failed");

	while (sp < spe) {
		printk("P%u : %s %u\n", (u32_t)(sp - scope_pins), sp->devname, sp->pin);
		if (strcmp(sp->devname, DEV_NAME) == 0) {
			u32_t flags = GPIO_OUTPUT_LOW;
			out_pins |= BIT(sp->pin);
			if ((nout & 1) == 1) {
				flags |= GPIO_ACTIVE_LOW;
				os2_pins |= BIT(sp->pin);
			}
			zassert_equal(gpio_pin_configure(dev, sp->pin, flags), 0,
				      "Scope pin configure failed");
			++nout;
		}
		++sp;
	}

	os1_pins = out_pins ^ os2_pins;

	out_pins |= BIT(PIN_OUT);
	in_pins = BIT(PIN_IN);

	TC_PRINT("%s : on %s os1 %x os2 %x in %x\n", __func__,
		 DEV_NAME, os1_pins, os2_pins, in_pins);

	k_usleep(1);

	measure("nop", iter_nop);
	measure("s1", iter_s1);

	return TC_PASS;
}


void test_gpio_perf(void)
{
	if (!IS_ENABLED(DT_INST_0_TEST_SCOPE_PINS)) {
		TC_PRINT("Performance test not supported\n");
		return;
	}
	zassert_equal(test_perf(), TC_PASS,
		      "performance test completed");
}
