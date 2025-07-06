/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ptp_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <errno.h>
#include <stdlib.h>

#include "ptp/clock.h"
#include "ptp/port.h"

static int run_duration = CONFIG_NET_SAMPLE_RUN_DURATION;
static struct k_work_delayable stop_sample;
static struct k_sem quit_lock;

static void stop_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	k_sem_give(&quit_lock);
}

static int get_current_status(void)
{
	struct ptp_port *port;
	sys_slist_t *ports_list = ptp_clock_ports_list();

	if (!ports_list || sys_slist_len(ports_list) == 0) {
		return -EINVAL;
	}

	port = CONTAINER_OF(sys_slist_peek_head(ports_list), struct ptp_port, node);

	if (!port) {
		return -EINVAL;
	}

	switch (ptp_port_state(port)) {
	case PTP_PS_INITIALIZING:
	case PTP_PS_FAULTY:
	case PTP_PS_DISABLED:
	case PTP_PS_LISTENING:
	case PTP_PS_PRE_TIME_TRANSMITTER:
	case PTP_PS_PASSIVE:
	case PTP_PS_UNCALIBRATED:
		printk("FAIL\n");
		return 0;
	case PTP_PS_TIME_RECEIVER:
		printk("TIME RECEIVER\n");
		return 2;
	case PTP_PS_TIME_TRANSMITTER:
	case PTP_PS_GRAND_MASTER:
		printk("TIME TRANSMITTER\n");
		return 1;
	}

	return -1;
}

void init_testing(void)
{
	uint32_t uptime = k_uptime_get_32();
	int ret;

	if (run_duration == 0) {
		return;
	}

	k_sem_init(&quit_lock, 0, K_SEM_MAX_LIMIT);

	k_work_init_delayable(&stop_sample, stop_handler);
	k_work_reschedule(&stop_sample, K_SECONDS(run_duration));

	k_sem_take(&quit_lock, K_FOREVER);

	LOG_INF("Stopping after %u seconds",
		(k_uptime_get_32() - uptime) / 1000);

	/* Try to figure out what is the sync state.
	 * Return:
	 *  <0 - configuration error
	 *   0 - not time sync
	 *   1 - we are TimeTransmitter
	 *   2 - we are TimeReceiver
	 */
	ret = get_current_status();

	exit(ret);
}

int main(void)
{
	init_testing();
	return 0;
}
