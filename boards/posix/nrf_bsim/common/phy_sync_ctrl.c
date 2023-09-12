/*
 * Copyright (c) 2017 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdbool.h>
#include "bs_cmd_line.h"
#include "bs_dynargs.h"
#include "bs_utils.h"
#include "bs_types.h"
#include "bs_tracing.h"
#include "nsi_tasks.h"
#include "nsi_hws_models_if.h"
#include "NRF_HWLowL.h"
#include "xo_if.h"
#include "bsim_args_runner.h"

/* By default every second we will inform the Phy simulator about our timing */
#define BSIM_DEFAULT_PHY_MAX_RESYNC_OFFSET 1000000

static struct {
	double start_offset;
	bs_time_t max_resync_offset;
	bool delay_init;
	bool sync_preinit;
	bool sync_preboot;
} sync_args;

static bs_time_t phy_sync_ctrl_event_timer = TIME_NEVER;
static bs_time_t last_resync_time;

static void psc_program_next_event(void)
{
	if (sync_args.max_resync_offset != TIME_NEVER) {
		phy_sync_ctrl_event_timer = last_resync_time + sync_args.max_resync_offset;

		bs_time_t now = nsi_hws_get_time();

		if (phy_sync_ctrl_event_timer < now) {
			phy_sync_ctrl_event_timer = now;
		}
	} else {
		phy_sync_ctrl_event_timer = TIME_NEVER;
	}
	nsi_hws_find_next_event();
}

static void phy_sync_ctrl_event_reached(void)
{

	last_resync_time = nsi_hws_get_time();

	hwll_sync_time_with_phy(last_resync_time);
	psc_program_next_event();
}

NSI_HW_EVENT(phy_sync_ctrl_event_timer, phy_sync_ctrl_event_reached, 900);

static void phy_sync_ctrl_init(void)
{
	last_resync_time = nsi_hws_get_time();

	psc_program_next_event();
}

NSI_TASK(phy_sync_ctrl_init, HW_INIT, 100);

/**
 * Keep track of the last time we synchronized the time with the Phy
 */
void phy_sync_ctrl_set_last_phy_sync_time(bs_time_t time)
{
	last_resync_time = time;
	psc_program_next_event();
}

/**
 * Configure the maximum resynchronization offset
 * (How long, in simulation time, can we spend without talking with the Phy)
 * Note that this value may be configured as a command line argument too
 */
void phy_sync_ctrl_set_max_resync_offset(bs_time_t max_resync_offset)
{
	sync_args.max_resync_offset = max_resync_offset;

	psc_program_next_event();
}

/* For backwards compatibility with the old board code */
void tm_set_phy_max_resync_offset(bs_time_t offset_in_us)
{
	phy_sync_ctrl_set_max_resync_offset(offset_in_us);
}

static double tmp_start_of;

static void cmd_start_of_found(char *argv, int offset)
{
	if (tmp_start_of < 0) {
		bs_trace_error_line("start offset (%lf) cannot be smaller than 0\n", tmp_start_of);
	}
	sync_args.start_offset = tmp_start_of;
	xo_model_set_toffset(sync_args.start_offset);
}

static void cmd_no_delay_init_found(char *argv, int offset)
{
	sync_args.delay_init = false;
}

static void cmd_no_sync_preinit_found(char *argv, int offset)
{
	sync_args.sync_preinit = false;
}

static void cmd_no_sync_preboot_found(char *argv, int offset)
{
	sync_args.sync_preboot = false;
}

static double tmp_max_resync_offset;

static void cmd_max_resync_offset_found(char *argv, int offset)
{
	if (tmp_max_resync_offset < 500) {
		bs_trace_warning("You are attempting to set a very low phy resynchronization of %d."
				"Note this will have a performance impact\n",
				tmp_max_resync_offset);
	}
	sync_args.max_resync_offset = tmp_max_resync_offset;
}

static void phy_sync_ctrl_register_args(void)
{
	static bs_args_struct_t args_struct_toadd[] = {
		{
		.option = "start_offset",
		.name = "start_of",
		.type = 'f',
		.dest = (void *)&tmp_start_of,
		.call_when_found = cmd_start_of_found,
		.descript = "Offset in time (at the start of the simulation) of this device. "
			    "At time 0 of the device, the phy will be at <start_of>"
		},
		{
		.is_switch = true,
		.option = "sync_preinit",
		.type = 'b',
		.dest = (void *)&sync_args.sync_preinit,
		.descript = "Postpone HW initialization and CPU boot until the Phy has "
			    "reached time 0 (or start_offset) (by default not set)"
		},
		{
		.is_switch = true,
		.option = "no_sync_preinit",
		.type = 'b',
		.call_when_found = cmd_no_sync_preinit_found,
		.descript = "Clear sync_preinit. Note that by default sync_preinit is not set"
		},
		{
		.is_switch = true,
		.option = "sync_preboot",
		.type = 'b',
		.dest = (void *)&sync_args.sync_preboot,
		.descript = "Postpone CPU boot (but not HW initialization) until the "
			    "Phy has reached time 0 (or start_offset) (by default not set)"
			    "If sync_preinit is set, this option has no effect."
		},
		{
		.is_switch = true,
		.option = "no_sync_preboot",
		.type = 'b',
		.call_when_found = cmd_no_sync_preboot_found,
		.descript = "Clear sync_preboot. Note that by default sync_preboot is not set"
		},
		{
		.is_switch = true,
		.option = "delay_init",
		.type = 'b',
		.dest = (void *)&sync_args.delay_init,
		.descript = "If start_offset is used, postpone HW initialization and CPU boot "
			    "until start_offset is reached (by default not set)"
		},
		{
		.is_switch = true,
		.option = "no_delay_init",
		.type = 'b',
		.call_when_found = cmd_no_delay_init_found,
		.descript = "Clear delay_init. Note that by default delay_init is not set"
		},
		{
		.option = "mro",
		.name = "max_resync_offset",
		.type = 'd',
		.dest = (void *)&tmp_max_resync_offset,
		.call_when_found = cmd_max_resync_offset_found,
		.descript = "Set the max Phy synchronization offset, that is, how far the device "
			    "time can be from the Phy time before it resynchronizes with the Phy "
			    "again (by default 1e6, 1s). Note that this value may be changed "
			    "programmatically by tests"
		},
		ARG_TABLE_ENDMARKER
	};

	bs_add_extra_dynargs(args_struct_toadd);

	sync_args.max_resync_offset = BSIM_DEFAULT_PHY_MAX_RESYNC_OFFSET;
}

NSI_TASK(phy_sync_ctrl_register_args, PRE_BOOT_1, 10);

void phy_sync_ctrl_connect_to_2G4_phy(void)
{
	bs_trace_raw(9, "%s: Connecting to phy...\n", __func__);
	hwll_connect_to_phy(bsim_args_get_2G4_device_nbr(),
			    bsim_args_get_simid(),
			    bsim_args_get_2G4_phy_id());
	bs_trace_raw(9, "%s: Connected\n", __func__);
}

void phy_sync_ctrl_pre_boot2(void)
{
	if (((sync_args.start_offset > 0) && (sync_args.delay_init))
		|| sync_args.sync_preinit) {
		/* Delay the next steps until the simulation time has
		 * reached either time 0 or start_offset.
		 */
		hwll_wait_for_phy_simu_time(BS_MAX(sync_args.start_offset, 0));
		sync_args.sync_preboot = false; /* Already sync'ed */
	}
}

void phy_sync_ctrl_pre_boot3(void)
{
	/*
	 * If sync_preboot was set, we sync with the phy
	 * right before booting the CPU
	 */
	if (sync_args.sync_preboot) {
		hwll_wait_for_phy_simu_time(BS_MAX(sync_args.start_offset, 0));
	}
}
