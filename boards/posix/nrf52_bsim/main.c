/*
 * Copyright (c) 2017-2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "soc.h"
#include "posix_soc.h"
#include "NRF_HW_model_top.h"
#include "NRF_HWLowL.h"
#include "bs_tracing.h"
#include "bs_types.h"
#include "bs_utils.h"
#include "bs_rand_main.h"
#include "bs_pc_backchannel.h"
#include "bs_dump_files.h"
#include "time_machine.h"
#include "argparse.h"
#include "bstests.h"
#include <stdlib.h>

uint8_t inner_main_clean_up(int exit_code)
{
	static int max_exit_code;

	max_exit_code = BS_MAX(exit_code, max_exit_code);

	/*
	 * posix_soc_clean_up may not return if this is called from a SW thread,
	 * but instead it would get posix_exit() recalled again
	 * ASAP from the HW thread
	 */
	posix_soc_clean_up();

	hwll_terminate_simulation();
	nrf_hw_models_free_all();
	bs_dump_files_close_all();

	bs_clean_back_channels();

	uint8_t bst_result = bst_delete();

	if (bst_result != 0U) {
		bs_trace_raw_time(2, "main: The TESTCASE FAILED with return "
				  "code %u\n", bst_result);
	}
	return BS_MAX(bst_result, max_exit_code);
}

uint8_t main_clean_up_trace_wrap(void)
{
	return inner_main_clean_up(0);
}

void posix_exit(int exit_code)
{
	exit(inner_main_clean_up(exit_code));
}

uint global_device_nbr;

int main(int argc, char *argv[])
{
	/*
	 * Let's ensure that even if we are redirecting to a file, we get stdout
	 * and stderr line buffered (default for console)
	 * Note that glibc ignores size. But just in case we set a reasonable
	 * number in case somebody tries to compile against a different library
	 */
	setvbuf(stdout, NULL, _IOLBF, 512);
	setvbuf(stderr, NULL, _IOLBF, 512);

	bs_trace_register_cleanup_function(main_clean_up_trace_wrap);
	bs_trace_register_time_function(tm_get_abs_time);

	nrf_hw_pre_init();
	run_native_tasks(_NATIVE_PRE_BOOT_1_LEVEL);

	struct NRF_bsim_args_t *args;

	args = nrfbsim_argsparse(argc, argv);
	global_device_nbr = args->global_device_nbr;

	run_native_tasks(_NATIVE_PRE_BOOT_2_LEVEL);

	bs_trace_raw(9, "%s: Connecting to phy...\n", __func__);
	hwll_connect_to_phy(args->device_nbr, args->s_id, args->p_id);
	bs_trace_raw(9, "%s: Connected\n", __func__);

	bs_random_init(args->rseed);
	bs_dump_files_open(args->s_id, args->global_device_nbr);

	/* We pass to a possible testcase its command line arguments */
	bst_pass_args(args->test_case_argc, args->test_case_argv);

	if (((args->nrf_hw.start_offset > 0) && (args->delay_init))
	    || args->sync_preinit) {
		/* Delay the next steps until the simulation time has
		 * reached either time 0 or start_offset.
		 */
		hwll_wait_for_phy_simu_time(BS_MAX(args->nrf_hw.start_offset, 0));
		args->sync_preboot = false; /* Already sync'ed */
	}

	nrf_hw_initialize(&args->nrf_hw);

	run_native_tasks(_NATIVE_PRE_BOOT_3_LEVEL);

	bst_pre_init();

	if (args->sync_preboot) {
		hwll_wait_for_phy_simu_time(BS_MAX(args->nrf_hw.start_offset, 0));
	}

	posix_boot_cpu();

	run_native_tasks(_NATIVE_FIRST_SLEEP_LEVEL);

	bst_post_init();

	tm_run_forever();

	/* This code is unreachable */
	bs_trace_exit_line("\n");
	return 0;
}
