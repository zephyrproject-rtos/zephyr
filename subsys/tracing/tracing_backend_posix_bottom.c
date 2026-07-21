/*
 * Copyright (c) 2018 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "nsi_tracing.h"

void *tracing_backend_posix_init_bottom(const char *file_name)
{
	FILE *f;

#if defined(__GLIBC_PREREQ) && __GLIBC_PREREQ(2, 7)
	f = fopen(file_name, "wbe"); /* Set O_CLOEXEC flag which is supported in glibc >= 2.7 */
#else
	f = fopen(file_name, "wb");
#endif

	if (f == NULL) {
		nsi_print_error_and_exit("%s: Could not open CTF backend file %s\n",
					 __func__, file_name);
	}

	return (void *)f;
}

void tracing_backend_posix_output_bottom(const void *data, unsigned long length, void *out_stream)
{
	int rc = fwrite(data, length, 1, (FILE *)out_stream);

	if (rc != 1) {
		nsi_print_warning("%s: Failure writing to CTF backend file\n", __func__);
	}

	fflush((FILE *)out_stream);
}
