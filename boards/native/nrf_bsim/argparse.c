/*
 * Copyright (c) 2017 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stdlib.h>
#include "bs_tracing.h"
#include "bstests.h"
#include "bs_cmd_line.h"
#include "bs_dynargs.h"
#include "posix_native_task.h"
#include "nsi_tracing.h"
#include "nsi_main.h"
#include "nsi_cpu_ctrl.h"

static const char exe_name[] = "nrf_bsim options:";

static char *testid;
static bool cpu_autostart;

static void cmd_testid_found(char *argv, int offset)
{
	bst_set_testapp_mode(testid);
}

static void cmd_testlist_found(char *argv, int offset)
{
	bst_print_testslist();
	nsi_exit(0);
}

static void cmd_autostart_found(char *argv, int offset)
{
	nsi_cpu_set_auto_start(CONFIG_NATIVE_SIMULATOR_MCU_N, cpu_autostart);
}

static void nrfbsim_register_args(void)
{
	static bs_args_struct_t args_struct_toadd[] = {
		{
		.option = "cpu" STRINGIFY(CONFIG_NATIVE_SIMULATOR_MCU_N) "_testid",
		.name = "testid",
		.type = 's',
		.dest = (void *)&testid,
		.call_when_found = cmd_testid_found,
		.descript = "Which of the bs tests shall be run. Run -testslist for more info"
		},
		{
		.is_switch = true,
		.option = "cpu" STRINGIFY(CONFIG_NATIVE_SIMULATOR_MCU_N) "_testslist",
		.type = 'b',
		.call_when_found = cmd_testlist_found,
		.descript = "Print information about the available bs application tests"
		},
#if (CONFIG_NATIVE_SIMULATOR_MCU_N == CONFIG_NATIVE_SIMULATOR_PRIMARY_MCU_INDEX)
		{
		.option = "testid",
		.name = "testid",
		.type = 's',
		.dest = (void *)&testid,
		.call_when_found = cmd_testid_found,
		.descript = "Alias of cpu" STRINGIFY(CONFIG_NATIVE_SIMULATOR_MCU_N) "_testid"
		},
		{
		.is_switch = true,
		.option = "testslist",
		.type = 'b',
		.call_when_found = cmd_testlist_found,
		.descript = "Alias of cpu" STRINGIFY(CONFIG_NATIVE_SIMULATOR_MCU_N) "_testslist"
		},
#endif
		{
		.option = "cpu" STRINGIFY(CONFIG_NATIVE_SIMULATOR_MCU_N) "_autostart",
		.name = "autostart",
		.type = 'b',
		.dest = (void *)&cpu_autostart,
		.call_when_found = cmd_autostart_found,
		.descript = "Automatically start CPU" STRINGIFY(CONFIG_NATIVE_SIMULATOR_MCU_N)
		},
		ARG_TABLE_ENDMARKER
	};

	bs_add_extra_dynargs(args_struct_toadd);
	bs_args_override_exe_name((char *)exe_name);
}

NATIVE_TASK(nrfbsim_register_args, PRE_BOOT_1, 100);
