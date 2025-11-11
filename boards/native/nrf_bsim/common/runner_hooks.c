/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Note this run in the runner context, and therefore
 * this file should therefore only be built once for all CPUs.
 */

#include "bs_tracing.h"
#include "bs_dump_files.h"
#include "bs_pc_backchannel.h"
#include "nsi_tasks.h"
#include "nsi_main.h"
#include "nsi_hw_scheduler.h"
#include "NRF_HWLowL.h"
#include "bsim_args_runner.h"

static bool bsim_disconnect_on_exit;

/*
 * Control what will happen to the overall simulation when this executable exists.
 * If <terminate> is true (default behavior) the Phy will be told to end the simulation
 * when this executable exits.
 * If <terminate> is false, this device will just disconnect, but let the simulation continue
 * otherwise.
 */
void bsim_set_terminate_on_exit(bool terminate)
{
	bsim_disconnect_on_exit = !terminate;
}

static uint8_t main_clean_up_trace_wrap(void)
{
	return nsi_exit_inner(0);
}

static void trace_registration(void)
{
	bs_trace_register_cleanup_function(main_clean_up_trace_wrap);
	bs_trace_register_time_function(nsi_hws_get_time);
}

NSI_TASK(trace_registration, PRE_BOOT_1,
	 0 /*we want to run this as early as possible */);

static void open_dumps(void)
{
	bs_dump_files_open(bsim_args_get_simid(),
			   bsim_args_get_global_device_nbr());
}

NSI_TASK(open_dumps, PRE_BOOT_2, 500);

static void exit_hooks(void)
{
	if (bsim_disconnect_on_exit) {
		hwll_disconnect_phy();
	} else {
		hwll_terminate_simulation();
	}
	bs_dump_files_close_all();
	bs_clean_back_channels();
}

NSI_TASK(exit_hooks, ON_EXIT_PRE, 500);

static void exit_control_args(void)
{
	static bs_args_struct_t args_struct_toadd[] = {
		{
		.option = "disconnect_on_exit",
		.type = 'b',
		.name = "term",
		.dest = (void *)&bsim_disconnect_on_exit,
		.descript = "If set to 1, on exit only disconnect this device from the Phy and let "
			    "the simulation continue. Otherwise (default) on exit terminate the "
			    "whole simulation."
		},
		ARG_TABLE_ENDMARKER
	};

	bs_add_extra_dynargs(args_struct_toadd);
}

NSI_TASK(exit_control_args, PRE_BOOT_1, 10);
