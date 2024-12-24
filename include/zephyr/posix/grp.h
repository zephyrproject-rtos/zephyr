/*
 * Copyright (c) 2024 Meta Platforms
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_GRP_H_
#define ZEPHYR_INCLUDE_POSIX_GRP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/posix/sys/stat.h>

/**
 * @brief Group structure
 */
struct group {
	/**< the name of the group */
	char *gr_name;
	/**< numerical group ID */
	gid_t gr_gid;
	/**< pointer to a null-terminated array of character pointers to member names */
	char **gr_mem;
};

#if defined(_XOPEN_SOURCE)
void endgrent(void);
struct group *getgrent(void);
#endif
struct group *getgrgid(gid_t gid);
int getgrgid_r(gid_t gid, struct group *grp, char *buffer, size_t bufsize, struct group **result);
struct group *getgrnam(const char *name);
int getgrnam_r(const char *name, struct group *grp, char *buffer, size_t bufsize,
	       struct group **result);
#if defined(_XOPEN_SOURCE)
void setgrent(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_GRP_H_ */
