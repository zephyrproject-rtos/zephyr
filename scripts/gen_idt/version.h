/* version.h - version number library */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct version {
	int kernel;
	int major;
	int minor;
	int maintenance;
};

extern void show_version(const char *name, const struct version *ver);
