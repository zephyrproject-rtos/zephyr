/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <soc.h>
#include <posix_native_task.h>
#include <nsi_cpu_if.h>
#include "bstests.h"
#include "bs_tracing.h"
#include "phy_sync_ctrl.h"
#include "nsi_hw_scheduler.h"
#include "nsi_cpu_ctrl.h"

/*
 * These hooks are to be named nsif_cpu<cpu_number>_<hook_name>, for example nsif_cpu0_boot
 */
#define nsif_cpun_pre_cmdline_hooks  _CONCAT(_CONCAT(nsif_cpu, CONFIG_NATIVE_SIMULATOR_MCU_N),\
					     _pre_cmdline_hooks)
#define nsif_cpun_save_test_arg      _CONCAT(_CONCAT(nsif_cpu, CONFIG_NATIVE_SIMULATOR_MCU_N),\
					     _save_test_arg)
#define nsif_cpun_pre_hw_init_hooks  _CONCAT(_CONCAT(nsif_cpu, CONFIG_NATIVE_SIMULATOR_MCU_N),\
					     _pre_hw_init_hooks)
#define nsif_cpun_boot               _CONCAT(_CONCAT(nsif_cpu, CONFIG_NATIVE_SIMULATOR_MCU_N),\
					     _boot)
#define nsif_cpun_cleanup            _CONCAT(_CONCAT(nsif_cpu, CONFIG_NATIVE_SIMULATOR_MCU_N),\
					     _cleanup)
#define nsif_cpun_irq_raised         _CONCAT(_CONCAT(nsif_cpu, CONFIG_NATIVE_SIMULATOR_MCU_N),\
					     _irq_raised)
#define nsif_cpun_test_hook          _CONCAT(_CONCAT(nsif_cpu, CONFIG_NATIVE_SIMULATOR_MCU_N),\
					     _test_hook)
#define nsif_cpun_irq_raised_from_sw _CONCAT(_CONCAT(nsif_cpu, CONFIG_NATIVE_SIMULATOR_MCU_N),\
					     _irq_raised_from_sw)

NATIVE_SIMULATOR_IF void nsif_cpun_pre_cmdline_hooks(void)
{
	run_native_tasks(_NATIVE_PRE_BOOT_1_LEVEL);
#if defined(CONFIG_NATIVE_SIMULATOR_AUTOSTART_MCU)
	nsi_cpu_set_auto_start(CONFIG_NATIVE_SIMULATOR_MCU_N, true);
#endif
}

#define TESTCASAE_ARGV_ALLOCSIZE 128

static struct {
	char **test_case_argv;
	int test_case_argc;
	int allocated_size;
} test_args;

NATIVE_SIMULATOR_IF void nsif_cpun_save_test_arg(char *argv)
{
	if (test_args.test_case_argc >= test_args.allocated_size) {
		test_args.allocated_size += TESTCASAE_ARGV_ALLOCSIZE;
		test_args.test_case_argv = realloc(test_args.test_case_argv,
						   test_args.allocated_size * sizeof(char *));
		if (test_args.test_case_argv == NULL) { /* LCOV_EXCL_BR_LINE */
			bs_trace_error_line("Can't allocate memory\n"); /* LCOV_EXCL_LINE */
		}
	}

	bs_trace_raw(9, "cmdarg: adding '%s' to testcase args[%i]\n",
		     argv, test_args.test_case_argc);
	test_args.test_case_argv[test_args.test_case_argc++] = argv;
}

static void test_args_free(void)
{
	if (test_args.test_case_argv) {
		free(test_args.test_case_argv);
		test_args.test_case_argv = NULL;
	}
}

NATIVE_TASK(test_args_free, ON_EXIT_PRE, 100);

NATIVE_SIMULATOR_IF void nsif_cpun_pre_hw_init_hooks(void)
{
	run_native_tasks(_NATIVE_PRE_BOOT_2_LEVEL);
	phy_sync_ctrl_connect_to_2G4_phy();

	/* We pass to a possible testcase its command line arguments */
	bst_pass_args(test_args.test_case_argc, test_args.test_case_argv);
	phy_sync_ctrl_pre_boot2();
}

NATIVE_SIMULATOR_IF void nsif_cpun_boot(void)
{
	run_native_tasks(_NATIVE_PRE_BOOT_3_LEVEL);
	bst_pre_init();
	phy_sync_ctrl_pre_boot3();
	posix_boot_cpu();
	run_native_tasks(_NATIVE_FIRST_SLEEP_LEVEL);
	bst_post_init();
}

NATIVE_SIMULATOR_IF int nsif_cpun_cleanup(void)
{
	/*
	 * Note posix_soc_clean_up() may not return, but in that case,
	 * nsif_cpun_cleanup() will be called again
	 */
	posix_soc_clean_up();

	uint8_t bst_result = bst_delete();
	return bst_result;
}

NATIVE_SIMULATOR_IF void nsif_cpun_irq_raised(void)
{
	posix_interrupt_raised();
}

NATIVE_SIMULATOR_IF int nsif_cpun_test_hook(void *p)
{
	(void) p;
	bst_tick(nsi_hws_get_time());
	return 0;
}

NATIVE_SIMULATOR_IF void nsif_cpun_irq_raised_from_sw(void)
{
	void posix_irq_handler_im_from_sw(void);
	posix_irq_handler_im_from_sw();
}
