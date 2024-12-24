/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <grp.h>
#include <pwd.h>

#include <zephyr/ztest.h>

static char buf[CONFIG_POSIX_GETGR_R_SIZE_MAX];

ZTEST(posix_system_database_r, test_getpwnam_r)
{
	struct passwd pwd;
	struct passwd *result;

	{
		/* degenerate cases */
		zexpect_not_ok(getpwnam_r(NULL, NULL, NULL, 0, NULL));
		zexpect_not_ok(getpwnam_r(NULL, NULL, NULL, 0, &result));
		zexpect_not_ok(getpwnam_r(NULL, NULL, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getpwnam_r(NULL, NULL, NULL, sizeof(buf), &result));
		zexpect_not_ok(getpwnam_r(NULL, NULL, buf, 0, NULL));
		zexpect_not_ok(getpwnam_r(NULL, NULL, buf, 0, &result));
		zexpect_not_ok(getpwnam_r(NULL, NULL, buf, sizeof(buf), NULL));
		zexpect_not_ok(getpwnam_r(NULL, NULL, buf, sizeof(buf), &result));
		zexpect_not_ok(getpwnam_r(NULL, &pwd, NULL, 0, NULL));
		zexpect_not_ok(getpwnam_r(NULL, &pwd, NULL, 0, &result));
		zexpect_not_ok(getpwnam_r(NULL, &pwd, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getpwnam_r(NULL, &pwd, NULL, sizeof(buf), &result));
		zexpect_not_ok(getpwnam_r(NULL, &pwd, buf, 0, NULL));
		zexpect_not_ok(getpwnam_r(NULL, &pwd, buf, 0, &result));
		zexpect_not_ok(getpwnam_r(NULL, &pwd, buf, sizeof(buf), NULL));
		zexpect_not_ok(getpwnam_r(NULL, &pwd, buf, sizeof(buf), &result));
		zexpect_not_ok(getpwnam_r("root", NULL, NULL, 0, NULL));
		zexpect_not_ok(getpwnam_r("root", NULL, NULL, 0, &result));
		zexpect_not_ok(getpwnam_r("root", NULL, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getpwnam_r("root", NULL, NULL, sizeof(buf), &result));
		zexpect_not_ok(getpwnam_r("root", NULL, buf, 0, NULL));
		zexpect_not_ok(getpwnam_r("root", NULL, buf, 0, &result));
		zexpect_not_ok(getpwnam_r("root", NULL, buf, sizeof(buf), NULL));
		zexpect_not_ok(getpwnam_r("root", NULL, buf, sizeof(buf), &result));
		zexpect_not_ok(getpwnam_r("root", &pwd, NULL, 0, NULL));
		zexpect_not_ok(getpwnam_r("root", &pwd, NULL, 0, &result));
		zexpect_not_ok(getpwnam_r("root", &pwd, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getpwnam_r("root", &pwd, NULL, sizeof(buf), &result));
		zexpect_not_ok(getpwnam_r("root", &pwd, buf, 0, NULL));
		zexpect_not_ok(getpwnam_r("root", &pwd, buf, 0, &result));
		zexpect_not_ok(getpwnam_r("root", &pwd, buf, sizeof(buf), NULL));

		/* buffer is not large enough */
		zexpect_equal(getpwnam_r("root", &pwd, buf, 1, &result), ERANGE);

		/* user is not found in /etc/passwd */
		result = (struct passwd *)0x42;
		zexpect_ok(getpwnam_r("nobody", &pwd, buf, sizeof(buf), &result));
		zexpect_equal(result, NULL);
	}

	zexpect_ok(getpwnam_r("root", &pwd, buf, sizeof(buf), &result));
	zexpect_equal(result, &pwd);
	zexpect_str_equal(pwd.pw_name, "root");
	zexpect_equal(pwd.pw_uid, 0);
	zexpect_equal(pwd.pw_gid, 0);
	zexpect_str_equal(pwd.pw_dir, "/root");
	zexpect_str_equal(pwd.pw_shell, "/bin/sh");

	zexpect_ok(getpwnam_r("user", &pwd, buf, sizeof(buf), &result));
	zexpect_equal(result, &pwd);
	zexpect_str_equal(pwd.pw_name, "user");
	zexpect_equal(pwd.pw_uid, 1000);
	zexpect_equal(pwd.pw_gid, 1000);
	zexpect_str_equal(pwd.pw_dir, "/home/user");
	zexpect_str_equal(pwd.pw_shell, "/bin/sh");
}

ZTEST(posix_system_database_r, test_getpwuid_r)
{
	struct passwd pwd;
	struct passwd *result;

	{
		/* degenerate cases */
		zexpect_not_ok(getpwuid_r(-1, NULL, NULL, 0, NULL));
		zexpect_not_ok(getpwuid_r(-1, NULL, NULL, 0, &result));
		zexpect_not_ok(getpwuid_r(-1, NULL, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getpwuid_r(-1, NULL, NULL, sizeof(buf), &result));
		zexpect_not_ok(getpwuid_r(-1, NULL, buf, 0, NULL));
		zexpect_not_ok(getpwuid_r(-1, NULL, buf, 0, &result));
		zexpect_not_ok(getpwuid_r(-1, NULL, buf, sizeof(buf), NULL));
		zexpect_not_ok(getpwuid_r(-1, NULL, buf, sizeof(buf), &result));
		zexpect_not_ok(getpwuid_r(-1, &pwd, NULL, 0, NULL));
		zexpect_not_ok(getpwuid_r(-1, &pwd, NULL, 0, &result));
		zexpect_not_ok(getpwuid_r(-1, &pwd, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getpwuid_r(-1, &pwd, NULL, sizeof(buf), &result));
		zexpect_not_ok(getpwuid_r(-1, &pwd, buf, 0, NULL));
		zexpect_not_ok(getpwuid_r(-1, &pwd, buf, 0, &result));
		zexpect_not_ok(getpwuid_r(-1, &pwd, buf, sizeof(buf), NULL));
		zexpect_not_ok(getpwuid_r(-1, &pwd, buf, sizeof(buf), &result));
		zexpect_not_ok(getpwuid_r(0, NULL, NULL, 0, NULL));
		zexpect_not_ok(getpwuid_r(0, NULL, NULL, 0, &result));
		zexpect_not_ok(getpwuid_r(0, NULL, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getpwuid_r(0, NULL, NULL, sizeof(buf), &result));
		zexpect_not_ok(getpwuid_r(0, NULL, buf, 0, NULL));
		zexpect_not_ok(getpwuid_r(0, NULL, buf, 0, &result));
		zexpect_not_ok(getpwuid_r(0, NULL, buf, sizeof(buf), NULL));
		zexpect_not_ok(getpwuid_r(0, NULL, buf, sizeof(buf), &result));
		zexpect_not_ok(getpwuid_r(0, &pwd, NULL, 0, NULL));
		zexpect_not_ok(getpwuid_r(0, &pwd, NULL, 0, &result));
		zexpect_not_ok(getpwuid_r(0, &pwd, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getpwuid_r(0, &pwd, NULL, sizeof(buf), &result));
		zexpect_not_ok(getpwuid_r(0, &pwd, buf, 0, NULL));
		zexpect_not_ok(getpwuid_r(0, &pwd, buf, 0, &result));
		zexpect_not_ok(getpwuid_r(0, &pwd, buf, sizeof(buf), NULL));

		/* buffer is not large enough */
		zexpect_equal(getpwuid_r(0, &pwd, buf, 1, &result), ERANGE);

		/* user is not found in /etc/passwd */
		result = (struct passwd *)0x42;
		zexpect_ok(getpwuid_r(1001, &pwd, buf, sizeof(buf), &result));
		zexpect_equal(result, NULL);
	}

	zexpect_ok(getpwuid_r(0, &pwd, buf, sizeof(buf), &result));
	zexpect_equal(result, &pwd);
	zexpect_str_equal(pwd.pw_name, "root");
	zexpect_equal(pwd.pw_uid, 0);
	zexpect_equal(pwd.pw_gid, 0);
	zexpect_str_equal(pwd.pw_dir, "/root");
	zexpect_str_equal(pwd.pw_shell, "/bin/sh");

	zexpect_ok(getpwuid_r(1000, &pwd, buf, sizeof(buf), &result));
	zexpect_equal(result, &pwd);
	zexpect_str_equal(pwd.pw_name, "user");
	zexpect_equal(pwd.pw_uid, 1000);
	zexpect_equal(pwd.pw_gid, 1000);
	zexpect_str_equal(pwd.pw_dir, "/home/user");
	zexpect_str_equal(pwd.pw_shell, "/bin/sh");
}

static const char *const members[] = {
	"staff",
	"admin",
};

ZTEST(posix_system_database_r, test_getgrnam_r)
{
	struct group grp;
	struct group *result;

	{
		/* degenerate cases */
		zexpect_not_ok(getgrnam_r(NULL, NULL, NULL, 0, NULL));
		zexpect_not_ok(getgrnam_r(NULL, NULL, NULL, 0, &result));
		zexpect_not_ok(getgrnam_r(NULL, NULL, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getgrnam_r(NULL, NULL, NULL, sizeof(buf), &result));
		zexpect_not_ok(getgrnam_r(NULL, NULL, buf, 0, NULL));
		zexpect_not_ok(getgrnam_r(NULL, NULL, buf, 0, &result));
		zexpect_not_ok(getgrnam_r(NULL, NULL, buf, sizeof(buf), NULL));
		zexpect_not_ok(getgrnam_r(NULL, NULL, buf, sizeof(buf), &result));
		zexpect_not_ok(getgrnam_r(NULL, &grp, NULL, 0, NULL));
		zexpect_not_ok(getgrnam_r(NULL, &grp, NULL, 0, &result));
		zexpect_not_ok(getgrnam_r(NULL, &grp, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getgrnam_r(NULL, &grp, NULL, sizeof(buf), &result));
		zexpect_not_ok(getgrnam_r(NULL, &grp, buf, 0, NULL));
		zexpect_not_ok(getgrnam_r(NULL, &grp, buf, 0, &result));
		zexpect_not_ok(getgrnam_r(NULL, &grp, buf, sizeof(buf), NULL));
		zexpect_not_ok(getgrnam_r(NULL, &grp, buf, sizeof(buf), &result));
		zexpect_not_ok(getgrnam_r("root", NULL, NULL, 0, NULL));
		zexpect_not_ok(getgrnam_r("root", NULL, NULL, 0, &result));
		zexpect_not_ok(getgrnam_r("root", NULL, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getgrnam_r("root", NULL, NULL, sizeof(buf), &result));
		zexpect_not_ok(getgrnam_r("root", NULL, buf, 0, NULL));
		zexpect_not_ok(getgrnam_r("root", NULL, buf, 0, &result));
		zexpect_not_ok(getgrnam_r("root", NULL, buf, sizeof(buf), NULL));
		zexpect_not_ok(getgrnam_r("root", NULL, buf, sizeof(buf), &result));
		zexpect_not_ok(getgrnam_r("root", &grp, NULL, 0, NULL));
		zexpect_not_ok(getgrnam_r("root", &grp, NULL, 0, &result));
		zexpect_not_ok(getgrnam_r("root", &grp, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getgrnam_r("root", &grp, NULL, sizeof(buf), &result));
		zexpect_not_ok(getgrnam_r("root", &grp, buf, 0, NULL));
		zexpect_not_ok(getgrnam_r("root", &grp, buf, 0, &result));
		zexpect_not_ok(getgrnam_r("root", &grp, buf, sizeof(buf), NULL));

		/* buffer is not large enough */
		zexpect_equal(getgrnam_r("root", &grp, buf, 1, &result), ERANGE);

		/* group is not found in /etc/group */
		result = (struct group *)0x42;
		zexpect_ok(getgrnam_r("nobody", &grp, buf, sizeof(buf), &result));
		zexpect_equal(result, NULL);
	}

	zexpect_ok(getgrnam_r("root", &grp, buf, sizeof(buf), &result));
	zexpect_equal(result, &grp);
	zexpect_str_equal(grp.gr_name, "root");
	zexpect_equal(grp.gr_gid, 0);
	zassert_within((uintptr_t)grp.gr_mem, (uintptr_t)buf, sizeof(buf));
	zexpect_equal(grp.gr_mem[0], NULL);

	zexpect_ok(getgrnam_r("user", &grp, buf, sizeof(buf), &result));
	zexpect_equal(result, &grp);
	zexpect_str_equal(grp.gr_name, "user");
	zexpect_equal(grp.gr_gid, 1000);
	zassert_within((uintptr_t)grp.gr_mem, (uintptr_t)buf, sizeof(buf));
	ARRAY_FOR_EACH(members, i) {
		zexpect_str_equal(grp.gr_mem[i], members[i],
				  "members[%d] (%s) does not match gr.gr_mem[%d] (%s)", i,
				  members[i], i, grp.gr_mem[i]);
	}
	zexpect_equal(grp.gr_mem[2], NULL);
}

ZTEST(posix_system_database_r, test_getgrgid_r)
{
	struct group grp;
	struct group *result;

	{
		/* degenerate cases */
		zexpect_not_ok(getgrgid_r(-1, NULL, NULL, 0, NULL));
		zexpect_not_ok(getgrgid_r(-1, NULL, NULL, 0, &result));
		zexpect_not_ok(getgrgid_r(-1, NULL, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getgrgid_r(-1, NULL, NULL, sizeof(buf), &result));
		zexpect_not_ok(getgrgid_r(-1, NULL, buf, 0, NULL));
		zexpect_not_ok(getgrgid_r(-1, NULL, buf, 0, &result));
		zexpect_not_ok(getgrgid_r(-1, NULL, buf, sizeof(buf), NULL));
		zexpect_not_ok(getgrgid_r(-1, NULL, buf, sizeof(buf), &result));
		zexpect_not_ok(getgrgid_r(-1, &grp, NULL, 0, NULL));
		zexpect_not_ok(getgrgid_r(-1, &grp, NULL, 0, &result));
		zexpect_not_ok(getgrgid_r(-1, &grp, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getgrgid_r(-1, &grp, NULL, sizeof(buf), &result));
		zexpect_not_ok(getgrgid_r(-1, &grp, buf, 0, NULL));
		zexpect_not_ok(getgrgid_r(-1, &grp, buf, 0, &result));
		zexpect_not_ok(getgrgid_r(-1, &grp, buf, sizeof(buf), NULL));
		zexpect_not_ok(getgrgid_r(-1, &grp, buf, sizeof(buf), &result));
		zexpect_not_ok(getgrgid_r(0, NULL, NULL, 0, NULL));
		zexpect_not_ok(getgrgid_r(0, NULL, NULL, 0, &result));
		zexpect_not_ok(getgrgid_r(0, NULL, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getgrgid_r(0, NULL, NULL, sizeof(buf), &result));
		zexpect_not_ok(getgrgid_r(0, NULL, buf, 0, NULL));
		zexpect_not_ok(getgrgid_r(0, NULL, buf, 0, &result));
		zexpect_not_ok(getgrgid_r(0, NULL, buf, sizeof(buf), NULL));
		zexpect_not_ok(getgrgid_r(0, NULL, buf, sizeof(buf), &result));
		zexpect_not_ok(getgrgid_r(0, &grp, NULL, 0, NULL));
		zexpect_not_ok(getgrgid_r(0, &grp, NULL, 0, &result));
		zexpect_not_ok(getgrgid_r(0, &grp, NULL, sizeof(buf), NULL));
		zexpect_not_ok(getgrgid_r(0, &grp, NULL, sizeof(buf), &result));
		zexpect_not_ok(getgrgid_r(0, &grp, buf, 0, NULL));
		zexpect_not_ok(getgrgid_r(0, &grp, buf, 0, &result));
		zexpect_not_ok(getgrgid_r(0, &grp, buf, sizeof(buf), NULL));

		/* buffer is not large enough */
		zexpect_equal(getgrgid_r(0, &grp, buf, 1, &result), ERANGE);

		/* group is not found in /etc/group */
		result = (struct group *)0x42;
		zexpect_ok(getgrgid_r(1001, &grp, buf, sizeof(buf), &result));
		zexpect_equal(result, NULL);
	}

	zexpect_ok(getgrgid_r(0, &grp, buf, sizeof(buf), &result));
	zexpect_equal(result, &grp);
	zexpect_str_equal(grp.gr_name, "root");
	zexpect_equal(grp.gr_gid, 0);
	zassert_within((uintptr_t)grp.gr_mem, (uintptr_t)buf, sizeof(buf));
	zexpect_equal(grp.gr_mem[0], NULL);

	zexpect_ok(getgrgid_r(1000, &grp, buf, sizeof(buf), &result));
	zexpect_equal(result, &grp);
	zexpect_str_equal(grp.gr_name, "user");
	zexpect_equal(grp.gr_gid, 1000);
	zassert_within((uintptr_t)grp.gr_mem, (uintptr_t)buf, sizeof(buf));
	ARRAY_FOR_EACH(members, i) {
		zexpect_str_equal(grp.gr_mem[i], members[i],
				  "members[%d] (%s) does not match gr.gr_mem[%d] (%s)", i,
				  members[i], i, grp.gr_mem[i]);
	}
	zexpect_equal(grp.gr_mem[2], NULL);
}

void *setup(void);
void teardown(void *arg);

ZTEST_SUITE(posix_system_database_r, NULL, setup, NULL, NULL, teardown);
