/* Copyright 2022 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <zephyr/ap_pwrseq/ap_pwrseq_sm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ap_pwrseq, LOG_LEVEL_INF);

/*
 * POWER_STATE_G3 Handlers
 */
void ap_pweseq_adl_g3_entry(void *ctx)
{
	LOG_INF("%s",__func__ );
}

void ap_pweseq_adl_g3_run(void *ctx)
{
	LOG_INF("%s",__func__ );
}

void ap_pweseq_adl_g3_exit(void *ctx)
{
	LOG_INF("%s",__func__ );
}

AP_POWER_CHIPSET_STATE_DEFINE(AP_POWER_STATE_G3, ap_pweseq_adl_g3_entry,
			ap_pweseq_adl_g3_run, ap_pweseq_adl_g3_exit)
/*
 * POWER_STATE_S5 Handlers
 */
void ap_pweseq_adl_s5_entry(void *ctx)
{
	enum ap_pwrseq_state prev_state = ap_pwrseq_sm_get_prev_state(ctx);

	LOG_INF("%s",__func__ );
	LOG_INF("   Previous state: %s", ap_pwrseq_get_state_str(prev_state));
}

void ap_pweseq_adl_s5_run(void *ctx)
{
	LOG_INF("%s",__func__ );
	if (IS_EVENT_SET(ctx, AP_PWRSEQ_EVENT_POWER_SIGNAL) ) {
		ap_pwrseq_sm_set_state(ctx, AP_POWER_STATE_S4);
	}
}

void ap_pweseq_adl_s5_exit(void *ctx)
{
	LOG_INF("%s",__func__ );
}

AP_POWER_CHIPSET_STATE_DEFINE(AP_POWER_STATE_S5, ap_pweseq_adl_s5_entry,
			ap_pweseq_adl_s5_run, ap_pweseq_adl_s5_exit)
/*
 * POWER_STATE_S4 Handlers
 */
void ap_pweseq_adl_s4_entry(void *ctx)
{

	enum ap_pwrseq_state prev_state = ap_pwrseq_sm_get_prev_state(ctx);

	LOG_INF("%s",__func__ );
	LOG_INF("   Previous state: %s", ap_pwrseq_get_state_str(prev_state));
}

void ap_pweseq_adl_s4_run(void *ctx)
{
	LOG_INF("%s",__func__ );
}

void ap_pweseq_adl_s4_exit(void *ctx)
{
	LOG_INF("%s",__func__ );
}

AP_POWER_CHIPSET_STATE_DEFINE(AP_POWER_STATE_S4, ap_pweseq_adl_s4_entry,
			ap_pweseq_adl_s4_run, ap_pweseq_adl_s4_exit)
/*
 * POWER_STATE_S3 Handlers
 */
void ap_pweseq_adl_s3_entry(void *ctx)
{
	enum ap_pwrseq_state prev_state = ap_pwrseq_sm_get_prev_state(ctx);

	LOG_INF("%s",__func__ );
	LOG_INF("   Previous state: %s", ap_pwrseq_get_state_str(prev_state));
}

void ap_pweseq_adl_s3_run(void *ctx)
{
	LOG_INF("%s",__func__ );
}

void ap_pweseq_adl_s3_exit(void *ctx)
{
	LOG_INF("%s",__func__ );
}

AP_POWER_CHIPSET_STATE_DEFINE(AP_POWER_STATE_S3, ap_pweseq_adl_s3_entry,
			ap_pweseq_adl_s3_run, ap_pweseq_adl_s3_exit)
/*
 * POWER_STATE_S0 Handlers
 */
void ap_pweseq_adl_s0_entry(void *ctx)
{
	LOG_INF("%s",__func__ );
}

void ap_pweseq_adl_s0_run(void *ctx)
{
	LOG_INF("%s",__func__ );
}

void ap_pweseq_adl_s0_exit(void *ctx)
{
	LOG_INF("%s",__func__ );
}

AP_POWER_CHIPSET_STATE_DEFINE(AP_POWER_STATE_S0, ap_pweseq_adl_s0_entry,
			ap_pweseq_adl_s0_run, ap_pweseq_adl_s0_exit)

void ap_pweseq_adl_s0ix_entry(void *ctx)
{
	LOG_INF("%s",__func__ );
}

void ap_pweseq_adl_s0ix_run(void *ctx)
{
	enum ap_pwrseq_state prev_state = ap_pwrseq_sm_get_prev_state(ctx);

	LOG_INF("%s",__func__ );
	LOG_INF("   Previous state: %s", ap_pwrseq_get_state_str(prev_state));
	ap_pwrseq_sm_set_state(ctx, AP_POWER_STATE_S0IX_2);
}

void ap_pweseq_adl_s0ix_exit(void *ctx)
{
	LOG_INF("%s",__func__ );
}

AP_POWER_CHIPSET_SUB_STATE_DEFINE(AP_POWER_STATE_S0IX,
	ap_pweseq_adl_s0ix_entry,
	ap_pweseq_adl_s0ix_run,
	ap_pweseq_adl_s0ix_exit,
	AP_POWER_STATE_S0)

void ap_pweseq_adl_s0ix_2_entry(void *ctx)
{
	LOG_INF("%s",__func__ );
}

void ap_pweseq_adl_s0ix_2_run(void *ctx)
{
	enum ap_pwrseq_state prev_state = ap_pwrseq_sm_get_prev_state(ctx);

	LOG_INF("%s",__func__ );
	LOG_INF("   Previous state: %s", ap_pwrseq_get_state_str(prev_state));
	ap_pwrseq_sm_set_state(ctx, AP_POWER_STATE_S0);
}

void ap_pweseq_adl_s0ix_2_exit(void *ctx)
{
	LOG_INF("%s",__func__ );
}

AP_POWER_CHIPSET_SUB_STATE_DEFINE(AP_POWER_STATE_S0IX_2,
	ap_pweseq_adl_s0ix_2_entry,
	ap_pweseq_adl_s0ix_2_run,
	ap_pweseq_adl_s0ix_2_exit,
	AP_POWER_STATE_S0)
