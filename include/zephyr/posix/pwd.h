/*
 * Copyright (c) 2024 Meta Platforms
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_PWD_H_
#define ZEPHYR_INCLUDE_POSIX_PWD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/posix/sys/stat.h>

struct passwd {
	/* user's login name */
	char *pw_name;
	/* numerical user ID */
	uid_t pw_uid;
	/* numerical group ID */
	gid_t pw_gid;
	/* initial working directory */
	char *pw_dir;
	/* program to use as shell */
	char *pw_shell;
};

#if defined(_XOPEN_SOURCE)
void endpwent(void);
struct passwd *getpwent(void);
#endif
struct passwd *getpwnam(const char *name);
int getpwnam_r(const char *name, struct passwd *pwd, char *buffer, size_t bufsize,
	       struct passwd **result);
struct passwd *getpwuid(uid_t uid);
int getpwuid_r(uid_t uid, struct passwd *pwd, char *buffer, size_t bufsize, struct passwd **result);
#if defined(_XOPEN_SOURCE)
void setpwent(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_PWD_H_ */
