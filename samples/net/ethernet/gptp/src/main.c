/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_gptp_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <errno.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/gptp.h>

extern void init_testing(void);

static struct gptp_phase_dis_cb phase_dis;

static void gptp_phase_dis_cb(uint8_t *gm_identity,
			      uint16_t *time_base,
			      struct gptp_scaled_ns *last_gm_ph_change,
			      double *last_gm_freq_change)
{
	char output[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];
	static uint8_t id[8];

	if (memcmp(id, gm_identity, sizeof(id))) {
		memcpy(id, gm_identity, sizeof(id));

		LOG_DBG("GM %s last phase %d.%" PRId64 "",
			gptp_sprint_clock_id(gm_identity, output, sizeof(output)),
			last_gm_ph_change->high,
			last_gm_ph_change->low);
	}
}

static int init_app(void)
{
	gptp_register_phase_dis_cb(&phase_dis, gptp_phase_dis_cb);

	return 0;
}

int main(void)
{
	init_app();

	init_testing();
	return 0;
}
