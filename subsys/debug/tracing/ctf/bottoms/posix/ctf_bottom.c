/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ctf_bottom.h"
#include "soc.h"
#include "cmdline.h" /* native_posix command line options header */
#include "posix_trace.h"


ctf_bottom_ctx_t ctf_bottom;

void ctf_bottom_configure(void)
{
	if (ctf_bottom.pathname == NULL) {
		ctf_bottom.pathname = "channel0_0";
	}

	ctf_bottom.ostream = fopen(ctf_bottom.pathname, "wb");
	if (ctf_bottom.ostream == NULL) {
		posix_print_error_and_exit("CTF trace: "
					   "Problem opening file %s.\n",
					   ctf_bottom.pathname);
	}
}

void ctf_bottom_start(void)
{
}

/* command line option to specify ctf output file */
void add_ctf_option(void)
{
	static struct args_struct_t ctf_options[] = {
		/*
		 * Fields:
		 * manual, mandatory, switch,
		 * option_name, var_name ,type,
		 * destination, callback,
		 * description
		 */
		{ .manual = false,
		  .is_mandatory = false,
		  .is_switch = false,
		  .option = "ctf-path",
		  .name = "file_name",
		  .type = 's',
		  .dest = (void *)&ctf_bottom.pathname,
		  .call_when_found = NULL,
		  .descript = "File name for CTF tracing output." },
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(ctf_options);
}
NATIVE_TASK(add_ctf_option, PRE_BOOT_1, 1);

