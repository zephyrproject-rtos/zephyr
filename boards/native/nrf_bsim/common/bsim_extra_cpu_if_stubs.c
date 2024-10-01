/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_tracing.h"
#include "nsi_cpu_if.h"

/*
 * Default (weak) implementation for nsif_cpu<n>_save_test_arg() expected by the argument
 * parsing defined in bsim_args_runner.
 * Note that the real implementation is provided by the board code.
 * This exists in case the total device image is assembled lacking some of the embedded MCU images
 * or the user tries to target a non-existent MCU
 */

static void save_test_arg_warn(const char *func, char *argv)
{
	bs_trace_warning("%s not defined. You may be passing a test argument to a CPU without"
			 " image or a non-existent CPU. Argument \"%s\" will be ignored\n",
			 func, argv);
}

/* Define N instances of void nsif_cpu<n>_save_test_arg */
F_TRAMP_BODY_LIST(NATIVE_SIMULATOR_IF __attribute__((weak)) void nsif_cpu,
		_save_test_arg(char *argv) { save_test_arg_warn(__func__, argv); })
