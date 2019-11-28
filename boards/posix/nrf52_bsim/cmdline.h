/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This header exists solely to allow drivers meant for the native_posix board
 * to be used directly in the nrf52_bsim board.
 * Note that such reuse should be done with great care.
 *
 * The command line arguments parsing logic from native_posix was born as a copy
 * of the one from the BabbleSim's libUtil library
 * They are therefore mostly equal except for types and functions names.
 *
 * This header converts these so the native_posix call to dynamically register
 * command line arguments is passed to the nrf52_bsim one
 */

#ifndef BOARDS_POSIX_NRF52_BSIM_CMDLINE_H
#define BOARDS_POSIX_NRF52_BSIM_CMDLINE_H

#include "../native_posix/cmdline_common.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void native_add_command_line_opts(struct args_struct_t *args)
{
	void bs_add_extra_dynargs(struct args_struct_t *args);
	bs_add_extra_dynargs(args);
}

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_POSIX_NRF52_BSIM_CMDLINE_H */
