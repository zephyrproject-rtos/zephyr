/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

extern char *z_getenv(const char *name);
extern int z_getenv_r(const char *name, char *buf, size_t len);
extern int z_setenv(const char *name, const char *val, int overwrite);
extern int z_unsetenv(const char *name);

char *getenv(const char *name)
{
	return z_getenv(name);
}

int getenv_r(const char *name, char *buf, size_t len)
{
	return z_getenv_r(name, buf, len);
}

int setenv(const char *name, const char *val, int overwrite)
{
	return z_setenv(name, val, overwrite);
}

int unsetenv(const char *name)
{
	return z_unsetenv(name);
}
