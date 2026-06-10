/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Password database access.
 * @ingroup posix
 *
 * Defines the passwd structure and the thread-safe functions used to search
 * the system user database by login name or numeric user ID.
 *
 * @posix_header{pwd.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_PWD_H_
#define ZEPHYR_INCLUDE_POSIX_PWD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/posix/sys/stat.h>

/**
 * @brief User database entry.
 */
struct passwd {
	/**
	 * @brief User login name.
	 */
	char *pw_name;
	/**
	 * @brief Numeric user ID.
	 */
	uid_t pw_uid;
	/**
	 * @brief Numeric group ID.
	 */
	gid_t pw_gid;
	/**
	 * @brief Initial working directory.
	 */
	char *pw_dir;
	/**
	 * @brief Program to use as the shell.
	 */
	char *pw_shell;
};

/**
 * @brief Look up a password entry by name (thread-safe).
 *
 * @param nam     User login name to search for.
 * @param pwd     Caller-supplied structure to fill in.
 * @param buffer  Caller-supplied buffer for string fields.
 * @param bufsize Size of @p buffer.
 * @param result  Output: pointer to @p pwd on success, or NULL if not found.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{getpwnam_r}
 */
int getpwnam_r(const char *nam, struct passwd *pwd, char *buffer, size_t bufsize,
	       struct passwd **result);

/**
 * @brief Look up a password entry by user ID (thread-safe).
 *
 * @param uid     Numerical user ID to search for.
 * @param pwd     Caller-supplied structure to fill in.
 * @param buffer  Caller-supplied buffer for string fields.
 * @param bufsize Size of @p buffer.
 * @param result  Output: pointer to @p pwd on success, or NULL if not found.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{getpwuid_r}
 */
int getpwuid_r(uid_t uid, struct passwd *pwd, char *buffer, size_t bufsize, struct passwd **result);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_PWD_H_ */
