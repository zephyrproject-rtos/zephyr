/* version.c - version number library */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "version.h"

void show_version(const char *name, const struct version *ver)
{
	fprintf(stdout,
		"\n%s"
		" %4d.%d.%d.%3.3d, (c) Wind River Systems, Inc.\n"
		" Built on %s at %s\n\n",
		name,
		ver->kernel, ver->major, ver->minor, ver->maintenance,
		__DATE__, __TIME__);
}
