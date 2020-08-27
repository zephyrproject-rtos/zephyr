/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BS_RADIO_ARGPARSE_H
#define _BS_RADIO_ARGPARSE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bs_radio_args {
	char *s_id;
	char *p_id;
	unsigned int device_nbr;
	bool is_bsim;
};

void bs_radio_argparse_add_options(void);
struct bs_radio_args *bs_radio_argparse_get(void);
void bs_radio_argparse_validate(void);

#ifdef __cplusplus
}
#endif

#endif /* _BS_RADIO_ARGPARSE */
