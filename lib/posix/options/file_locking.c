/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/sys/util.h>

void zvfs_flockfile(int fd);
int zvfs_ftrylockfile(int fd);
void zvfs_funlockfile(int fd);

void flockfile(FILE *file)
{
	zvfs_flockfile(POINTER_TO_INT(file));
}
#ifdef CONFIG_POSIX_FD_MGMT_ALIAS_FLOCKFILE
FUNC_ALIAS(flockfile, _flockfile, void);
#endif /* CONFIG_POSIX_FD_MGMT_ALIAS_FLOCKFILE */

int ftrylockfile(FILE *file)
{
	return zvfs_ftrylockfile(POINTER_TO_INT(file));
}
#ifdef CONFIG_POSIX_FD_MGMT_ALIAS_FTRYLOCKFILE
FUNC_ALIAS(ftrylockfile, _ftrylockfile, int);
#endif /* CONFIG_POSIX_FD_MGMT_ALIAS_FTRYLOCKFILE */

void funlockfile(FILE *file)
{
	zvfs_funlockfile(POINTER_TO_INT(file));
}
#ifdef CONFIG_POSIX_FD_MGMT_ALIAS_FUNLOCKFILE
FUNC_ALIAS(funlockfile, _funlockfile, void);
#endif /* CONFIG_POSIX_FD_MGMT_ALIAS_FUNLOCKFILE */
