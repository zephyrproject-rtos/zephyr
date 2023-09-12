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
	hwll_terminate_simulation();
	bs_dump_files_close_all();
	bs_clean_back_channels();
}

NSI_TASK(exit_hooks, ON_EXIT_PRE, 500);
