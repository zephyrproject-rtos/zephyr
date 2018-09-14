/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BSIM_NRF_ARGS_H
#define BSIM_NRF_ARGS_H

#include <stdint.h>
#include "NRF_hw_args.h"
#include "bs_cmd_line_typical.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAXPARAMS_TESTCASES 1024

struct NRF_bsim_args_t {
	BS_BASIC_DEVICE_OPTIONS_FIELDS
	char *test_case_argv[MAXPARAMS_TESTCASES];
	int test_case_argc;
	nrf_hw_sub_args_t nrf_hw;
};

void nrfbsim_argsparse(int argc, char *argv[], struct NRF_bsim_args_t *args);

#ifdef __cplusplus
}
#endif

#endif
