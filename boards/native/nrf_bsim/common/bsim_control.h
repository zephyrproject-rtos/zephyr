/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BOARDS_NATIVE_BSIM_COMMON_BSIM_CONTROL_H
#define BOARDS_NATIVE_BSIM_COMMON_BSIM_CONTROL_H

#include <stdint.h>
#include "bsim_args_runner.h"

#ifdef __cplusplus
extern "C" {
#endif

void bsim_set_terminate_on_exit(bool terminate);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_NATIVE_BSIM_COMMON_BSIM_CONTROL_H */
