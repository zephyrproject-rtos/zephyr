/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

static const struct device *radio_clk_dev =
	DEVICE_DT_GET_OR_NULL(DT_CLOCKS_CTLR(DT_NODELABEL(radio)));
static struct onoff_client radio_cli;

static void fake_radio_timer_handler(struct k_timer *timer);
static K_TIMER_DEFINE(fake_radio_timer, fake_radio_timer_handler, NULL);

static void radio_clock_started_cb(struct onoff_manager *mgr,
				   struct onoff_client *cli,
				   uint32_t state,
				   int res)
{
	printk("* radio_clk_dev ");
	if (res < 0) {
		printk("failed to start: %d\n", res);
	} else {
		printk("started\n");

		/* Radio operation is to be started here and when it is
		 * finished, the clock should be released. A timer is
		 * used instead just for simplicity of the sample.
		 */
		k_timer_start(&fake_radio_timer, K_MSEC(150), K_NO_WAIT);
	}
}

static void fake_radio_timer_handler(struct k_timer *timer)
{
	(void)nrf_clock_control_release(radio_clk_dev, NULL);
	printk("* radio_clk_dev released\n");
}

static void lf_clock_started_cb(struct onoff_manager *mgr,
				struct onoff_client *cli,
				uint32_t state,
				int res)
{
	printk("* %s, res: %d\n", __func__, res);
}

static int request_and_wait_for_clock(const struct device *clk_dev,
				      const struct nrf_clock_spec *clk_spec,
				      struct onoff_client *cli)
{
	int rc;
	int res;

	sys_notify_init_spinwait(&cli->notify);

	rc = nrf_clock_control_request(clk_dev, clk_spec, cli);
	if (rc < 0) {
		printk("Request for %s returned: %d\n", clk_dev->name, rc);
		return rc;
	}

	printk("Waiting for %s\n", clk_dev->name);
	do {
		rc = sys_notify_fetch_result(&cli->notify, &res);
	} while (rc == -EAGAIN);

	if (res < 0) {
		printk("%s could not be started: %d\n",
			clk_dev->name, res);
		return -ENODEV;
	}

	printk("%s started successfully\n", clk_dev->name);

	return 0;
}

int main(void)
{
	const struct device *cpu_clk_dev =
		DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_NODELABEL(cpu)));
	const struct device *clk_dev[] = {
		DEVICE_DT_GET_OR_NULL(DT_CLOCKS_CTLR(DT_NODELABEL(uart130))),
		DEVICE_DT_GET_OR_NULL(DT_CLOCKS_CTLR(DT_NODELABEL(spi131))),
		DEVICE_DT_GET_OR_NULL(DT_CLOCKS_CTLR(DT_NODELABEL(rtc130))),
	};
	struct onoff_client cli[ARRAY_SIZE(clk_dev)];
	uint32_t rate = 0;
	int rc;

	/* Give nrfs and DVFS some time to complete initialization. */
	printk("Initial wait\n");
	k_sleep(K_MSEC(1000));

	rate = 0;
	(void)clock_control_get_rate(cpu_clk_dev, 0, &rate);
	printk("cpu_clk_dev is %s, rate: %u\n", cpu_clk_dev->name, rate);

	printk("radio_clk_dev is ");
	if (radio_clk_dev) {
		rate = 0;
		(void)clock_control_get_rate(radio_clk_dev, 0, &rate);
		printk("%s, rate: %u\n", radio_clk_dev->name, rate);
	} else {
		printk("NULL\n");
	}

	for (int i = 0; i < ARRAY_SIZE(clk_dev); ++i) {
		printk("clk_dev[%d] is ", i);
		if (clk_dev[i]) {
			rate = 0;
			(void)clock_control_get_rate(clk_dev[i], 0, &rate);
			printk("%s, rate: %u\n", clk_dev[i]->name, rate);
		} else {
			printk("NULL\n");
		}
	}

	if (radio_clk_dev) {
		sys_notify_init_callback(&radio_cli.notify,
					 radio_clock_started_cb);
		rc = nrf_clock_control_request(radio_clk_dev, NULL, &radio_cli);
		if (rc < 0) {
			printk("Request for radio_clk_dev returned: %d\n", rc);
		}
	}

	const struct nrf_clock_spec clk_spec0 = {
		.accuracy = NRF_CLOCK_CONTROL_ACCURACY_PPM(10000),
	};
	rc = request_and_wait_for_clock(clk_dev[0], &clk_spec0, &cli[0]);
	if (rc >= 0) {
		/* Placeholder. Do stuff that requires better clock accuracy. */
		k_sleep(K_MSEC(100));

		rc = nrf_clock_control_release(clk_dev[0], &clk_spec0);
		if (rc < 0) {
			printk("Release for clk_dev[0] returned: %d\n", rc);
		}
	}

	const struct nrf_clock_spec clk_spec1 = {
		.accuracy = NRF_CLOCK_CONTROL_ACCURACY_MAX,
	};
	rc = request_and_wait_for_clock(clk_dev[1], &clk_spec1, &cli[1]);
	if (rc >= 0) {
		/* Placeholder. Do stuff that requires better clock accuracy. */
		k_sleep(K_MSEC(100));

		rc = nrf_clock_control_release(clk_dev[1], &clk_spec1);
		if (rc < 0) {
			printk("Release for clk_dev[1] returned: %d\n", rc);
		}
	}

	const struct nrf_clock_spec clk_spec2 = {
		.accuracy  = NRF_CLOCK_CONTROL_ACCURACY_MAX,
		.precision = NRF_CLOCK_CONTROL_PRECISION_HIGH,
	};
	sys_notify_init_callback(&cli[2].notify, lf_clock_started_cb);
	rc = nrf_clock_control_request(clk_dev[2], &clk_spec2, &cli[2]);
	if (rc < 0) {
		printk("Request for clk_dev[2] returned: %d\n", rc);
	}

	/* Use the cancel_or_release function as the clock may not have started
	 * yet at this point.
	 */
	nrf_clock_control_cancel_or_release(clk_dev[2], &clk_spec2, &cli[2]);

#if CONFIG_NRFS_HAS_DVFS_SERVICE
	struct onoff_client cpu_cli;
	const struct nrf_clock_spec cpu_clk_100 = {
		.frequency = MHZ(100),
	};
	rc = request_and_wait_for_clock(cpu_clk_dev, &cpu_clk_100, &cpu_cli);
	if (rc >= 0) {
		rate = 0;
		(void)clock_control_get_rate(cpu_clk_dev, 0, &rate);
		printk("cpu_clk_dev rate is now: %u\n", rate);

		/* Placeholder. */
		k_sleep(K_MSEC(100));

		nrf_clock_control_release(cpu_clk_dev, &cpu_clk_100);
	}

	const struct nrf_clock_spec cpu_clk_max = {
		.frequency = NRF_CLOCK_CONTROL_FREQUENCY_MAX,
	};
	rc = request_and_wait_for_clock(cpu_clk_dev, &cpu_clk_max, &cpu_cli);
	if (rc >= 0) {
		rate = 0;
		(void)clock_control_get_rate(cpu_clk_dev, 0, &rate);
		printk("cpu_clk_dev rate is now: %u\n", rate);

		/* Placeholder. */
		k_sleep(K_MSEC(100));

		nrf_clock_control_release(cpu_clk_dev, &cpu_clk_max);
	}

	/* Wait a moment to ensure that CPU frequency releasing is completed. */
	k_sleep(K_MSEC(5));

	rate = 0;
	(void)clock_control_get_rate(cpu_clk_dev, 0, &rate);
	printk("cpu_clk_dev rate is now: %u\n", rate);
#endif

	printk("Finished\n");

	return 0;
}
