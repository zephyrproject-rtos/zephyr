/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/zephyr.h>
#include <stdio.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_litex.h>
/* Test defines */

/* Select clock outputs for tests [0-6] */
#define LITEX_CLK_TEST_CLK1	0
#define LITEX_CLK_TEST_CLK2	1

/* Values for frequency test */
#define LITEX_TEST_FREQUENCY_DUTY_VAL		50         /* [%]   */
#define LITEX_TEST_FREQUENCY_PHASE_VAL		0          /* [deg] */
#define LITEX_TEST_FREQUENCY_DELAY_MS		1000       /* [ms]  */
#define LITEX_TEST_FREQUENCY_MIN		5000000    /* [Hz]  */
#define LITEX_TEST_FREQUENCY_MAX		1200000000 /* [Hz]  */
#define LITEX_TEST_FREQUENCY_STEP		1000000    /* [Hz]  */

/* Values for duty test */
#define LITEX_TEST_DUTY_FREQ_VAL		20000000   /* [Hz]  */
#define LITEX_TEST_DUTY_PHASE_VAL		0          /* [deg] */
#define LITEX_TEST_DUTY_DELAY_MS		250        /* [ms]  */
#define LITEX_TEST_DUTY_MIN			0          /* [%]   */
#define LITEX_TEST_DUTY_MAX			100        /* [%]   */
#define LITEX_TEST_DUTY_STEP			1          /* [%]   */

/* Values for phase test */
#define LITEX_TEST_PHASE_FREQ_VAL		25000000   /* [Hz]  */
#define LITEX_TEST_PHASE_DUTY_VAL		50         /* [%]   */
#define LITEX_TEST_PHASE_DELAY_MS		50         /* [ms]  */
#define LITEX_TEST_PHASE_MIN			0          /* [deg] */
#define LITEX_TEST_PHASE_MAX			360        /* [deg] */
#define LITEX_TEST_PHASE_STEP			1          /* [deg] */

/* Values for single parameters test */
#define LITEX_TEST_SINGLE_FREQ_VAL		15000000   /* [Hz]  */
#define LITEX_TEST_SINGLE_DUTY_VAL		25         /* [%]   */
#define LITEX_TEST_SINGLE_PHASE_VAL		90         /* [deg] */
#define LITEX_TEST_SINGLE_FREQ_VAL2		15000000   /* [Hz]  */
#define LITEX_TEST_SINGLE_DUTY_VAL2		75         /* [%]   */
#define LITEX_TEST_SINGLE_PHASE_VAL2		0          /* [deg] */

/* loop tests infinitely if true, otherwise do one loop */
#define LITEX_TEST_LOOP		0

/* Choose test type */
#define LITEX_TEST		4

#define LITEX_TEST_FREQUENCY	1
#define LITEX_TEST_DUTY		2
#define LITEX_TEST_PHASE	3
#define LITEX_TEST_SINGLE	4

#define LITEX_TEST_DUTY_DEN	100

/* LiteX Common Clock Driver tests */
int litex_clk_test_getters(const struct device *dev)
{
	struct litex_clk_setup setup;
	uint32_t rate;
	int i;

	clock_control_subsys_t sub_system = (clock_control_subsys_t *)&setup;

	printf("Getters test\n");
	for (i = 0; i < NCLKOUT; i++) {
		setup.clkout_nr = i;
		clock_control_get_status(dev, sub_system);
		printf("CLKOUT%d: get_status: rate:%d phase:%d duty:%d\n",
			i, setup.rate, setup.phase, setup.duty);
		clock_control_get_rate(dev, sub_system, &rate);
		printf("CLKOUT%d: get_rate:%d\n", i, rate);
	}

	return 0;
}

int litex_clk_test_single(const struct device *dev)
{
	struct litex_clk_setup setup1 = {
		.clkout_nr = LITEX_CLK_TEST_CLK1,
		.rate = LITEX_TEST_SINGLE_FREQ_VAL,
		.duty = LITEX_TEST_SINGLE_DUTY_VAL,
		.phase = LITEX_TEST_SINGLE_PHASE_VAL
	};
	struct litex_clk_setup setup2 = {
		.clkout_nr = LITEX_CLK_TEST_CLK2,
		.rate = LITEX_TEST_SINGLE_FREQ_VAL2,
		.duty = LITEX_TEST_SINGLE_DUTY_VAL2,
		.phase = LITEX_TEST_SINGLE_PHASE_VAL2,
	};
	uint32_t ret = 0;
	clock_control_subsys_t sub_system1 = (clock_control_subsys_t *)&setup1;
	clock_control_subsys_t sub_system2 = (clock_control_subsys_t *)&setup2;

	printf("Single test\n");
	ret = clock_control_on(dev, sub_system1);
	if (ret != 0) {
		return ret;
	}
	ret = clock_control_on(dev, sub_system2);
	if (ret != 0) {
		return ret;
	}

	litex_clk_test_getters(dev);

	return 0;
}

int litex_clk_test_freq(const struct device *dev)
{
	struct litex_clk_setup setup = {
		.clkout_nr = LITEX_CLK_TEST_CLK1,
		.duty = LITEX_TEST_FREQUENCY_DUTY_VAL,
		.phase = LITEX_TEST_FREQUENCY_PHASE_VAL
	};
	clock_control_subsys_t sub_system = (clock_control_subsys_t *)&setup;
	uint32_t i, ret = 0;

	printf("Frequency test\n");

	do {
		for (i = LITEX_TEST_FREQUENCY_MIN; i < LITEX_TEST_FREQUENCY_MAX;
				i += LITEX_TEST_FREQUENCY_STEP) {
			setup.clkout_nr = LITEX_CLK_TEST_CLK1;
			setup.rate = i;
			sub_system = (clock_control_subsys_t *)&setup;
			/*
			 * Don't check for ENOTSUP here because it is expected.
			 * The reason is that set of possible frequencies for
			 * specific clock output depends on devicetree config
			 * (including margin) and also on other active clock
			 * outputs configuration. Some configurations may cause
			 * errors when the driver is trying to update one of
			 * the clkouts.
			 * Ignoring these errors ensure that
			 * test will be finished
			 *
			 */
			ret = clock_control_on(dev, sub_system);
			if (ret != 0 && ret != -ENOTSUP) {
				return ret;
			}
			setup.clkout_nr = LITEX_CLK_TEST_CLK2;
			ret = clock_control_on(dev, sub_system);
			if (ret != 0) {
				return ret;
			}
			k_sleep(K_MSEC(LITEX_TEST_FREQUENCY_DELAY_MS));
		}
		for (i = LITEX_TEST_FREQUENCY_MAX; i > LITEX_TEST_FREQUENCY_MIN;
				i -= LITEX_TEST_FREQUENCY_STEP) {
			setup.clkout_nr = LITEX_CLK_TEST_CLK1;
			setup.rate = i;
			sub_system = (clock_control_subsys_t *)&setup;
			ret = clock_control_on(dev, sub_system);
			if (ret != 0 && ret != -ENOTSUP) {
				return ret;
			}
			setup.clkout_nr = LITEX_CLK_TEST_CLK2;
			ret = clock_control_on(dev, sub_system);
			if (ret != 0) {
				return ret;
			}
			k_sleep(K_MSEC(LITEX_TEST_FREQUENCY_DELAY_MS));
		}
	} while (LITEX_TEST_LOOP);

	return 0;
}

int litex_clk_test_phase(const struct device *dev)
{
	struct litex_clk_setup setup1 = {
		.clkout_nr = LITEX_CLK_TEST_CLK1,
		.rate = LITEX_TEST_PHASE_FREQ_VAL,
		.duty = LITEX_TEST_PHASE_DUTY_VAL,
		.phase = 0
	};
	struct litex_clk_setup setup2 = {
		.clkout_nr = LITEX_CLK_TEST_CLK2,
		.rate = LITEX_TEST_PHASE_FREQ_VAL,
		.duty = LITEX_TEST_PHASE_DUTY_VAL
	};
	clock_control_subsys_t sub_system1 = (clock_control_subsys_t *)&setup1;
	clock_control_subsys_t sub_system2 = (clock_control_subsys_t *)&setup2;
	uint32_t ret = 0;
	int i;

	printf("Phase test\n");

	ret = clock_control_on(dev, sub_system1);
	if (ret != 0 && ret != -ENOTSUP) {
		return ret;
	}

	do {
		for (i = LITEX_TEST_PHASE_MIN; i <= LITEX_TEST_PHASE_MAX;
				i += LITEX_TEST_PHASE_STEP) {
			setup2.phase = i;
			sub_system2 = (clock_control_subsys_t *)&setup2;
			ret = clock_control_on(dev, sub_system2);
			if (ret != 0) {
				return ret;
			}
			k_sleep(K_MSEC(LITEX_TEST_PHASE_DELAY_MS));
		}
	} while (LITEX_TEST_LOOP);

	return 0;
}

int litex_clk_test_duty(const struct device *dev)
{
	struct litex_clk_setup setup1 = {
		.clkout_nr = LITEX_CLK_TEST_CLK1,
		.rate = LITEX_TEST_DUTY_FREQ_VAL,
		.phase = LITEX_TEST_DUTY_PHASE_VAL,
		.duty = 0
	};
	struct litex_clk_setup setup2 = {
		.clkout_nr = LITEX_CLK_TEST_CLK2,
		.rate = LITEX_TEST_DUTY_FREQ_VAL,
		.phase = LITEX_TEST_DUTY_PHASE_VAL,
		.duty = 0
	};
	uint32_t ret = 0, i;
	clock_control_subsys_t sub_system1 = (clock_control_subsys_t *)&setup1;
	clock_control_subsys_t sub_system2 = (clock_control_subsys_t *)&setup2;

	ret = clock_control_on(dev, sub_system1);
	if (ret != 0 && ret != -ENOTSUP) {
		return ret;
	}
	ret = clock_control_on(dev, sub_system2);
	if (ret != 0 && ret != -ENOTSUP) {
		return ret;
	}

	printf("Duty test\n");

	do {
		for (i = LITEX_TEST_DUTY_MIN; i <= LITEX_TEST_DUTY_MAX;
				i += LITEX_TEST_DUTY_STEP) {
			setup1.duty = i;
			sub_system1 = (clock_control_subsys_t *)&setup1;
			ret = clock_control_on(dev, sub_system1);
			if (ret != 0) {
				return ret;
			}
			setup2.duty = 100 - i;
			sub_system2 = (clock_control_subsys_t *)&setup2;
			ret = clock_control_on(dev, sub_system2);
			if (ret != 0) {
				return ret;
			}
			k_sleep(K_MSEC(LITEX_TEST_DUTY_DELAY_MS));
		}
		for (i = LITEX_TEST_DUTY_MAX; i > LITEX_TEST_DUTY_MIN;
				i -= LITEX_TEST_DUTY_STEP) {
			setup1.duty = i;
			sub_system1 = (clock_control_subsys_t *)&setup1;
			ret = clock_control_on(dev, sub_system1);
			if (ret != 0) {
				return ret;
			}
			setup2.duty = 100 - i;
			sub_system2 = (clock_control_subsys_t *)&setup2;
			ret = clock_control_on(dev, sub_system2);
			if (ret != 0) {
				return ret;
			}
			k_sleep(K_MSEC(LITEX_TEST_DUTY_DELAY_MS));
		}
	} while (LITEX_TEST_LOOP);

	return 0;
}

int litex_clk_test(const struct device *dev)
{
	int ret;

	printf("Clock test\n");

	switch (LITEX_TEST) {
	case LITEX_TEST_DUTY:
		ret = litex_clk_test_duty(dev);
		break;
	case LITEX_TEST_PHASE:
		ret = litex_clk_test_phase(dev);
		break;
	case LITEX_TEST_FREQUENCY:
		ret = litex_clk_test_freq(dev);
		break;
	case LITEX_TEST_SINGLE:
	default:
		ret = litex_clk_test_single(dev);
	}
	printf("Clock test done returning: %d\n", ret);
	return ret;

}

void main(void)
{
	const struct device *dev = DEVICE_DT_GET(MMCM);

	printf("Clock Control Example! %s\n", CONFIG_ARCH);

	printf("device name: %s\n", dev->name);
	if (!device_is_ready(dev)) {
		printf("error: device %s is not ready\n", dev->name);
		return;
	}

	printf("clock control device is %p, name is %s\n",
	       dev, dev->name);

	litex_clk_test(dev);
}
