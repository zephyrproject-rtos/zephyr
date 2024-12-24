/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/sys/util.h>
#include <zephyr/posix/grp.h>
#include <zephyr/posix/pwd.h>

static int count(const char *s, char c)
{
	int count;

	for (count = 0; *s != '\0'; ++s) {
		count += (*s == c) ? 1 : 0;
	}

	return count;
}

int __getgr_r(const char *name, gid_t gid, struct group *grp, char *buffer, size_t bufsize,
	      struct group **result)
{
	int ret;
	int nmemb;
	FILE *file;

	if (((name == NULL) && (gid == (gid_t)-1)) || (grp == NULL) || (buffer == NULL) ||
	    (result == NULL)) {
		if (result != NULL) {
			*result = NULL;
		}
		return EINVAL;
	}

	/*
	 * Originally, this checked for a bufsize of 0, but newlib will return NULL from fgets when
	 * bufsize is < 2
	 */
	if (bufsize < 2) {
		return ERANGE;
	}

	file = fopen("/etc/group", "r");
	if (file == NULL) {
		return EIO;
	}

	while (fgets(buffer, bufsize, file) != NULL) {
		char *p = buffer;
		char *q;

		if (*p == '\0') {
			goto close_erange;
		}

		if (*p == '\n') {
			continue;
		}

		/* name */
		q = strchr(p, ':');
		if (q == NULL) {
			goto close_erange;
		}
		*q = '\0';
		if ((name != NULL) && (strcmp(p, name) != 0)) {
			continue;
		}
		grp->gr_name = p;
		p = q + 1;

		/* password */
		q = strchr(p, ':');
		if (q == NULL) {
			goto close_erange;
		}
		*q = '\0';
		p = q + 1;

		/* gid */
		q = strchr(p, ':');
		if (q == NULL) {
			goto close_erange;
		}
		*q = '\0';
		if ((name == NULL) && (atoi(p) != gid)) {
			continue;
		}
		grp->gr_gid = atoi(p);
		p = q + 1;

		/* members */
		q = strchr(p, '\n');
		if (q == NULL) {
			goto close_erange;
		}
		*q = '\0';

		/* count members */
		int min_size;

		nmemb = (p == q) ? 0 : 1 + count(p, ',');
		min_size = (q - buffer + 1 + nmemb * sizeof(char *)) + 32;
		if (bufsize < min_size) {
			goto close_erange;
		}

		/* set up member array inside of buffer */
		grp->gr_mem = (char **)(q + 1);
		grp->gr_mem = (char **)ROUND_UP((uintptr_t)grp->gr_mem, 16);
		grp->gr_mem[nmemb] = NULL;

		for (int i = 0; i < nmemb; ++i) {
			char *x = strchr(p, ',');

			grp->gr_mem[i] = p;
			if (x == NULL) {
				break;
			}
			*x = '\0';
			p = x + 1;
		}

		/* group found \o/ */
		*result = grp;
		ret = 0;
		goto close_ret;
	}

	/* group not found :( )*/
	ret = 0;
	*result = NULL;
	goto close_ret;

close_erange:
	ret = ERANGE;

close_ret:
	fclose(file);
	return ret;
}

int __getpw_r(const char *name, uid_t uid, struct passwd *pwd, char *buffer, size_t bufsize,
	      struct passwd **result)
{
	int ret;
	FILE *file;

	if (((name == NULL) && (uid == (uid_t)-1)) || (pwd == NULL) || (buffer == NULL) ||
	    (result == NULL)) {
		if (result != NULL) {
			*result = NULL;
		}
		return EINVAL;
	}

	/*
	 * Originally, this checked for a bufsize of 0, but newlib will return NULL from fgets when
	 * bufsize is < 2
	 */
	if (bufsize < 2) {
		return ERANGE;
	}

	file = fopen("/etc/passwd", "r");
	if (file == NULL) {
		return EIO;
	}

	while (fgets(buffer, bufsize, file) != NULL) {
		char *p = buffer;
		char *q;

		if (*p == '\0') {
			goto close_erange;
		}

		if (*p == '\n') {
			continue;
		}

		/* name */
		q = strchr(p, ':');
		if (q == NULL) {
			goto close_erange;
		}
		*q = '\0';
		if ((name != NULL) && (strcmp(p, name) != 0)) {
			continue;
		}
		pwd->pw_name = p;
		p = q + 1;

		/* password */
		q = strchr(p, ':');
		if (q == NULL) {
			goto close_erange;
		}
		*q = '\0';
		p = q + 1;

		/* uid */
		q = strchr(p, ':');
		if (q == NULL) {
			goto close_erange;
		}
		*q = '\0';
		if ((name == NULL) && (atoi(p) != uid)) {
			continue;
		}
		pwd->pw_uid = atoi(p);
		p = q + 1;

		/* gid */
		q = strchr(p, ':');
		if (q == NULL) {
			goto close_erange;
		}
		*q = '\0';
		pwd->pw_gid = atoi(p);
		p = q + 1;

		/* gecos */
		q = strchr(p, ':');
		if (q == NULL) {
			goto close_erange;
		}
		*q = '\0';
		p = q + 1;

		/* dir */
		q = strchr(p, ':');
		if (q == NULL) {
			goto close_erange;
		}
		*q = '\0';
		pwd->pw_dir = p;
		p = q + 1;

		/* shell */
		q = strchr(p, '\n');
		if (q == NULL) {
			goto close_erange;
		}
		*q = '\0';
		pwd->pw_shell = p;

		/* user found \o/ */
		*result = pwd;
		ret = 0;
		goto close_ret;
	}

	/* user not found :( )*/
	ret = 0;
	*result = NULL;
	goto close_ret;

close_erange:
	ret = ERANGE;

close_ret:
	fclose(file);
	return ret;
}
