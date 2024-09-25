/*
 * Copyright (c) 2017 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BOARDS_POSIX_BSIM_COMMON_BSIM_ARGS_RUNNER_H
#define BOARDS_POSIX_BSIM_COMMON_BSIM_ARGS_RUNNER_H

#include <stdint.h>
#include "bs_cmd_line.h"

#ifdef __cplusplus
extern "C" {
#endif

void bs_add_extra_dynargs(bs_args_struct_t *args_struct_toadd);

char *bsim_args_get_simid(void);
char *bsim_args_get_2G4_phy_id(void);
unsigned int bsim_args_get_global_device_nbr(void);
unsigned int bsim_args_get_2G4_device_nbr(void);

/* Will be deprecated in favor of bsim_args_get_simid() */
char *get_simid(void);
/* Will be deprecated in favor of bsim_args_get_global_device_nbr() */
unsigned int get_device_nbr(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_POSIX_BSIM_COMMON_BSIM_ARGS_RUNNER_H */
