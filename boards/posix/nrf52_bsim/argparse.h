/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BSIM_NRF_ARGS_H
#define BSIM_NRF_ARGS_H

#include <stdint.h>
#include "NRF_hw_args.h"
#include "bs_cmd_line.h"
#include "bs_cmd_line_typical.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAXPARAMS_TESTCASES 1024

struct NRF_bsim_args_t {
	BS_BASIC_DEVICE_OPTIONS_FIELDS
	char *test_case_argv[MAXPARAMS_TESTCASES];
	int test_case_argc;
	bool delay_init;
	nrf_hw_sub_args_t nrf_hw;
};

struct NRF_bsim_args_t *nrfbsim_argsparse(int argc, char *argv[]);
void bs_add_extra_dynargs(bs_args_struct_t *args_struct_toadd);
char *get_simid(void);
unsigned int get_device_nbr(void);

#ifdef __cplusplus
}
#endif

#endif
