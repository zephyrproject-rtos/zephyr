/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stdlib.h>

#include <cmdline.h>
#include <arch/posix/posix_soc_if.h>
#include "bs_radio_argparse.h"

static struct bs_radio_args bs_args;

void bs_radio_argparse_add_options(void)
{
	static struct args_struct_t bs_options[] = {
		{ false, false, false, "d", "device_number", 'u',
		  (void *)&bs_args.device_nbr, NULL,
		  "Device number (for this phy)" },
		{ false, false, false, "s", "s_id", 's', (void *)&bs_args.s_id,
		  NULL, "String which uniquely identifies the simulation" },
		{ false, false, false, "p", "p_id", 's', (void *)&bs_args.p_id,
		  NULL,
		  "(2G4) String which uniquely identifies the phy inside the "
		  "simulation" },
		{ false, false, true, "bsim", "bsim", 'b',
		  (void *)&bs_args.is_bsim, NULL,
		  "Enable BabbleSim to simulate radio activity" },
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(bs_options);
}

void bs_radio_argparse_validate(void)
{
	bool p_id_set = bs_args.p_id != NULL;
	bool s_id_set = bs_args.s_id != NULL;
	bool dev_nbr_set = bs_args.device_nbr != -1;

	if (bs_args.is_bsim) {
		if (!p_id_set || !s_id_set || !dev_nbr_set) {
			posix_print_warning(
				"%s%s%snot set. It must be set prior to run the "
				"simulation\n\n",
				!p_id_set ? "[p_id] " : "",
				!s_id_set ? "[s_id] " : "",
				!dev_nbr_set ? "[device_number] " : "");
		}
	} else if (p_id_set || s_id_set || dev_nbr_set) {
		posix_print_warning(
			"%s%s%sset, but will not take any effect, "
			"because app is not running with BabbleSim.\n"
			"It can be enabled with -bsim option.\n\n",
			p_id_set ? "[p_id] " : "", s_id_set ? "[s_id] " : "",
			dev_nbr_set ? "[device_number] " : "");
	}
}

struct bs_radio_args *bs_radio_argparse_get(void)
{
	return &bs_args;
}
