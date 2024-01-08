/*
 * Copyright (c) 2017 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Common command line arguments and overall command line argument handling
 * for Zephyr Babblesim boards.
 *
 * Note that this is code runs in the native simulator runner context,
 * and not in any embedded CPU context.
 * This file should therefore only be built once for all CPUs.
 */

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include "bs_cmd_line.h"
#include "bs_cmd_line_typical.h"
#include "bs_dynargs.h"
#include "bs_tracing.h"
#include "bs_dump_files.h"
#include "bs_rand_main.h"
#include "nsi_cpu_if.h"
#include "nsi_tasks.h"
#include "nsi_main.h"
#include "nsi_cpu_ctrl.h"
#include "NRF_HWLowL.h"
#include "NHW_misc.h"

static bs_args_struct_t *args_struct;
/* Direct use of this global is deprecated, use bsim_args_get_global_device_nbr() instead */
uint global_device_nbr;

#define MAXPARAMS_TESTCASES 1024

static struct bsim_global_args_t {
	BS_BASIC_DEVICE_OPTIONS_FIELDS
} global_args;

static bool nosim;

static void cmd_trace_lvl_found(char *argv, int offset)
{
	bs_trace_set_level(global_args.verb);
}

static void cmd_gdev_nbr_found(char *argv, int offset)
{
	bs_trace_set_prefix_dev(global_args.global_device_nbr);
}

static void print_no_sim_warning(void)
{
	bs_trace_warning("Neither simulation id or the device number "
			"have been set. I assume you want to run "
			"without a BabbleSim phy (-nosim)\n");
	bs_trace_warning("If this is not what you wanted, check with "
			"--help how to set them\n");
	bs_trace_raw(3, "setting sim_id to 'bogus', device number to 0 "
			"and nosim\n");
}

static void print_mcus_info(char *argv, int offset)
{
	(void) argv;
	(void) offset;
	bs_trace_raw(0, "CPU  #,       Name  , Autostart\n");
	bs_trace_raw(0, "-------------------------------\n");
	for (int i = 0; i < NSI_N_CPUS; i++) {
		bs_trace_raw(0, "CPU %2i, %12s,    %i\n",
			     i, nhw_get_core_name(i), nsi_cpu_get_auto_start(i));
	}
}

static void bsim_register_basic_args(void)
{
#define args (&global_args)
	/* This define allows reusing the definitions provided by the utils library */
	static bs_args_struct_t args_struct_toadd[] = {
		ARG_TABLE_S_ID,
		ARG_TABLE_P_ID_2G4,
		ARG_TABLE_DEV_NBR,
		ARG_TABLE_GDEV_NBR,
		ARG_TABLE_VERB,
		ARG_TABLE_SEED,
		ARG_TABLE_COLOR,
		ARG_TABLE_NOCOLOR,
		ARG_TABLE_FORCECOLOR,
		{
		.is_switch = true,
		.option = "nosim",
		.type = 'b',
		.dest = (void *)&nosim,
		.descript = "Do not connect to the Physical layer simulator"
		},
		BS_DUMP_FILES_ARGS,
		{
		.manual = true,
		.option = "argstest",
		.name = "arg",
		.type = 'l',
		.descript = "The arguments that follow will be passed straight to the testcase "
			"init function (Note: If more than 1 MCU is present, argtest corresponds "
			"to argstests" NSI_STRINGIFY(NSI_PRIMARY_MCU_N) " )"
		},
		{
		.manual = true,
		.option = "argstest<n>",
		.name = "arg",
		.type = 'l',
		.descript = "The arguments that follow will be passed straight to cpu<n>'s "
			"testcase init function), where 0 <= n < " NSI_STRINGIFY(NSI_N_CPUS)
			" is the cpu number"
		},
		{
		.manual = true,
		.option = "argsmain",
		.name = "arg",
		.type = 'l',
		.descript = "The arguments that follow will be passed to main (default)"
		},
		{
		.is_switch = true,
		.option = "cpu_print_info",
		.call_when_found = print_mcus_info,
		.type = 'b',
		.descript = "Print information about each MCUs",
		},
		ARG_TABLE_ENDMARKER
	};
#undef args

	bs_add_dynargs(&args_struct, args_struct_toadd);
}

NSI_TASK(bsim_register_basic_args, PRE_BOOT_1, 0);

static void bsim_cleanup_args(void)
{
	bs_cleanup_dynargs(&args_struct);
}

NSI_TASK(bsim_cleanup_args, ON_EXIT_POST, 0);

void bs_add_extra_dynargs(bs_args_struct_t *args_struct_toadd)
{
	bs_add_dynargs(&args_struct, args_struct_toadd);
}

static void nsif_cpun_save_test_arg(int n, char *c)
{
	F_TRAMP_LIST(NATIVE_SIMULATOR_IF void nsif_cpu, _save_test_arg(char *argv))

	void(*fptrs[])(char *) = {
		F_TRAMP_TABLE(nsif_cpu, _save_test_arg)
	};

	fptrs[n](c);
}

/**
 * Check the arguments provided in the command line: set args based on it or
 * defaults, and check they are correct
 */
void nsi_handle_cmd_line(int argc, char *argv[])
{
	const char *bogus_sim_id = "bogus";

	bs_args_set_defaults(args_struct);
	global_args.verb = 2;
	bs_trace_set_level(global_args.verb);

	static const char default_phy[] = "2G4";

	enum {Main = 0, Test = 1} parsing = Main;
	uint test_cpu_n;

	for (int i = 1; i < argc; i++) {
		if (bs_is_option(argv[i], "argstest", 0)) {
			parsing = Test;
			test_cpu_n = NSI_PRIMARY_MCU_N;
			continue;
		} else if (bs_is_multi_opt(argv[i], "argstest", &test_cpu_n, 0)) {
			parsing = Test;
			continue;
		} else if (bs_is_option(argv[i], "argsmain", 0)) {
			parsing = Main;
			continue;
		}

		if (parsing == Main) {
			if (!bs_args_parse_one_arg(argv[i], args_struct)) {
				bs_args_print_switches_help(args_struct);
				bs_trace_error_line("Incorrect option %s\n",
						    argv[i]);
			}
		} else if (parsing == Test) {
			nsif_cpun_save_test_arg(test_cpu_n, argv[i]);
		} else {
			bs_trace_error_line("Bad error\n");
		}
	}

	/**
	 * If the user did not set the simulation id or device number
	 * we assume he wanted to run with nosim (but warn him)
	 */
	if ((!nosim) && (global_args.s_id == NULL) && (global_args.device_nbr == UINT_MAX)) {
		print_no_sim_warning();
		nosim = true;
	}
	if (nosim) {
		if (global_args.s_id == NULL) {
			global_args.s_id = (char *)bogus_sim_id;
		}
		if (global_args.device_nbr == UINT_MAX) {
			global_args.device_nbr = 0;
		}
		hwll_set_nosim(true);
	}

	if (global_args.device_nbr == UINT_MAX) {
		bs_args_print_switches_help(args_struct);
		bs_trace_error_line("The command line option <device number> "
				    "needs to be set\n");
	}
	if (global_args.global_device_nbr == UINT_MAX) {
		global_args.global_device_nbr = global_args.device_nbr;
		bs_trace_set_prefix_dev(global_args.global_device_nbr);
	}
	global_device_nbr = global_args.global_device_nbr;
	if (!global_args.s_id) {
		bs_args_print_switches_help(args_struct);
		bs_trace_error_line("The command line option <simulation ID> "
				    "needs to be set\n");
	}
	if (!global_args.p_id) {
		global_args.p_id = (char *)default_phy;
	}

	if (global_args.rseed == UINT_MAX) {
		global_args.rseed = 0x1000 + global_args.device_nbr;
	}

	bs_random_init(global_args.rseed);
}

/*
 * Get the simulation id
 */
char *bsim_args_get_simid(void)
{
	return global_args.s_id;
}

/*
 * Get this device number in the simulation, as it is
 * known in the overall simulation.
 * In general this is the device number you want
 */
unsigned int bsim_args_get_global_device_nbr(void)
{
	return global_args.global_device_nbr;
}

/*
 * Get this device number in the 2G4 Phy simulation
 */
unsigned int bsim_args_get_2G4_device_nbr(void)
{
	return global_args.device_nbr;
}

/*
 * Get this device number in the 2G4 Phy simulation
 */
char *bsim_args_get_2G4_phy_id(void)
{
	return global_args.p_id;
}

char *get_simid(void)
{
	return bsim_args_get_simid();
}

unsigned int get_device_nbr(void)
{
	return bsim_args_get_global_device_nbr();
}
