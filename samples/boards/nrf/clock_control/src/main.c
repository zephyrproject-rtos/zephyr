/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#if DT_NODE_EXISTS(DT_ALIAS(sample_device))
#define SAMPLE_CLOCK_NODE DT_CLOCKS_CTLR(DT_ALIAS(sample_device))
#elif DT_NODE_EXISTS(DT_ALIAS(sample_clock))
#define SAMPLE_CLOCK_NODE DT_ALIAS(sample_clock)
#endif

#define SAMPLE_CLOCK_NAME DT_NODE_FULL_NAME(SAMPLE_CLOCK_NODE)

#define SAMPLE_NOTIFY_TIMEOUT       K_SECONDS(2)
#define SAMPLE_PRE_REQUEST_TIMEOUT  K_SECONDS(CONFIG_SAMPLE_PRE_REQUEST_TIMEOUT)
#define SAMPLE_KEEP_REQUEST_TIMEOUT K_SECONDS(CONFIG_SAMPLE_KEEP_REQUEST_TIMEOUT)

const struct device *sample_clock_dev = DEVICE_DT_GET(SAMPLE_CLOCK_NODE);

static K_SEM_DEFINE(sample_sem, 0, 1);

static void sample_notify_cb(struct onoff_manager *mgr,
			     struct onoff_client *cli,
			     uint32_t state,
			     int res)
{
	ARG_UNUSED(mgr);
	ARG_UNUSED(cli);
	ARG_UNUSED(state);
	ARG_UNUSED(res);

	k_sem_give(&sample_sem);
}

int main(void)
{
	struct onoff_client cli;
	int ret;
	int res;
	int64_t req_start_uptime;
	int64_t req_stop_uptime;

	printk("\n");
	printk("clock name: %s\n", SAMPLE_CLOCK_NAME);
	printk("minimum frequency request: %uHz\n", CONFIG_SAMPLE_CLOCK_FREQUENCY_HZ);
	printk("minimum accuracy request: %uPPM\n", CONFIG_SAMPLE_CLOCK_ACCURACY_PPM);
	printk("minimum precision request: %u\n", CONFIG_SAMPLE_CLOCK_PRECISION);

	const struct nrf_clock_spec spec = {
		.frequency = CONFIG_SAMPLE_CLOCK_FREQUENCY_HZ,
		.accuracy = CONFIG_SAMPLE_CLOCK_ACCURACY_PPM,
		.precision = CONFIG_SAMPLE_CLOCK_PRECISION,
	};

	sys_notify_init_callback(&cli.notify, sample_notify_cb);

	k_sleep(SAMPLE_PRE_REQUEST_TIMEOUT);

	printk("\n");
	printk("requesting minimum clock specs\n");
	req_start_uptime = k_uptime_get();
	ret = nrf_clock_control_request(sample_clock_dev, &spec, &cli);
	if (ret < 0) {
		printk("minimum clock specs could not be met\n");
		return 0;
	}

	ret = k_sem_take(&sample_sem, SAMPLE_NOTIFY_TIMEOUT);
	if (ret < 0) {
		printk("timed out waiting for clock to meet request\n");
		return 0;
	}

	req_stop_uptime = k_uptime_get();

	ret = sys_notify_fetch_result(&cli.notify, &res);
	if (ret < 0) {
		printk("sys notify fetch failed\n");
		return 0;
	}

	if (res < 0) {
		printk("failed to apply request to clock\n");
		return 0;
	}

	printk("request applied to clock in %llims\n", req_stop_uptime - req_start_uptime);
	k_sleep(SAMPLE_KEEP_REQUEST_TIMEOUT);

	printk("\n");
	printk("releasing requested clock specs\n");
	ret = nrf_clock_control_release(sample_clock_dev, &spec);
	if (ret < 0) {
		printk("failed to release requested clock specs\n");
		return 0;
	}

	printk("clock spec request released\n");
	return 0;
}
