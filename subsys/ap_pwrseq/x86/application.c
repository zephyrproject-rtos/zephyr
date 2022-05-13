/* Copyright 2022 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <zephyr/ap_pwrseq/ap_pwrseq_sm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ap_pwrseq, LOG_LEVEL_INF);

void ap_pweseq_app_s0_entry(void *ctx)
{
	LOG_INF("%s",__func__ );
}

void ap_pweseq_app_s0_run(void *ctx)
{
	enum ap_pwrseq_state prev_state = ap_pwrseq_sm_get_prev_state(ctx);

	LOG_INF("%s",__func__ );
	LOG_INF("   Previous state: %s", ap_pwrseq_get_state_str(prev_state));
	if (ap_pwrseq_sm_get_prev_state(ctx) != AP_POWER_STATE_S0IX_2) {
		ap_pwrseq_sm_set_state(ctx, AP_POWER_STATE_APP_S0IX);
		return;
	}

	if (IS_EVENT_SET(ctx, AP_PWRSEQ_EVENT_POWER_BUTTON) ) {
		ap_pwrseq_sm_set_state(ctx, AP_POWER_STATE_G3);
	}
}

void ap_pweseq_app_s0_exit(void *ctx)
{
	LOG_INF("%s",__func__ );
}

AP_POWER_APP_STATE_DEFINE(AP_POWER_STATE_S0,
	ap_pweseq_app_s0_entry,
	ap_pweseq_app_s0_run,
	ap_pweseq_app_s0_exit)

void ap_pweseq_app_s0ix_entry(void *ctx)
{
	LOG_INF("%s",__func__ );
}

void ap_pweseq_app_s0ix_run(void *ctx)
{
	enum ap_pwrseq_state prev_state = ap_pwrseq_sm_get_prev_state(ctx);

	LOG_INF("%s",__func__ );
	LOG_INF("   Previous state: %s", ap_pwrseq_get_state_str(prev_state));
	ap_pwrseq_sm_set_state(ctx, AP_POWER_STATE_S0IX);
}

void ap_pweseq_app_s0ix_exit(void *ctx)
{
	LOG_INF("%s",__func__ );
}

AP_POWER_APP_SUB_STATE_DEFINE(AP_POWER_STATE_APP_S0IX,
	ap_pweseq_app_s0ix_entry,
	ap_pweseq_app_s0ix_run,
	ap_pweseq_app_s0ix_exit,
	AP_POWER_STATE_S0IX)
