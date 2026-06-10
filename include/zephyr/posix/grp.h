/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Group database access.
 * @ingroup posix
 *
 * Defines the group structure and the thread-safe functions used to search
 * the system group database by group name or numeric group ID.
 *
 * @posix_header{grp.h}
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
	/**
	 * @brief Group name.
	 */
	char *gr_name;
	/**
	 * @brief Numeric group ID.
	 */
	gid_t gr_gid;
	/**
	 * @brief NULL-terminated array of member login names.
	 */
	char **gr_mem;
};

/**
 * @brief Look up a group entry by name (thread-safe).
 *
 * @param name    Group name to search for.
 * @param grp     Caller-supplied structure to fill in.
 * @param buffer  Caller-supplied buffer for string fields.
 * @param bufsize Size of @p buffer.
 * @param result  Output: pointer to @p grp on success, or NULL if not found.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{getgrnam_r}
 */
int getgrnam_r(const char *name, struct group *grp, char *buffer, size_t bufsize,
	       struct group **result);

/**
 * @brief Look up a group entry by group ID (thread-safe).
 *
 * @param gid     Numerical group ID to search for.
 * @param grp     Caller-supplied structure to fill in.
 * @param buffer  Caller-supplied buffer for string fields.
 * @param bufsize Size of @p buffer.
 * @param result  Output: pointer to @p grp on success, or NULL if not found.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{getgrgid_r}
 */
int getgrgid_r(gid_t gid, struct group *grp, char *buffer, size_t bufsize, struct group **result);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_GRP_H_ */
