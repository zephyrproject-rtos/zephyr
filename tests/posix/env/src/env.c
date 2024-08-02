/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/ztest.h>

#define M_HOME "/home/zephyr"
#define M_UID  "1000"
#define M_PWD  "/tmp"

#define _m_alt_home "/this/path/is/much/longer/than" M_HOME

#define DEFINE_ENVIRON(_handle, _key, _val) char _handle[] = _key "=" _val
#define RESET_ENVIRON(_handle, _key, _val)                                                         \
	snprintf(_handle, ARRAY_SIZE(_handle), "%s=%s", _key, _val)

#if defined(CONFIG_NEWLIB_LIBC) || defined(CONFIG_PICOLIBC)
/* newlib headers seem to be missing this */
int getenv_r(const char *name, char *val, size_t len);
#endif

extern char **environ;
static char **old_environ;

static DEFINE_ENVIRON(home, "HOME", M_HOME);
static DEFINE_ENVIRON(uid, "UID", M_UID);
static DEFINE_ENVIRON(pwd, "PWD", M_PWD);

static char *environ_for_test[] = {home, uid, pwd, NULL};

ZTEST(env, test_getenv)
{
	zassert_equal(getenv(NULL), NULL);
	zassert_equal(getenv(""), NULL);
	zassert_equal(getenv("invalid=key"), NULL);
	zassert_equal(getenv("HOME=" M_HOME), NULL);
	zassert_equal(getenv("PWDR"), NULL);

	zassert_mem_equal(getenv("HOME"), M_HOME, strlen(M_HOME) + 1);
	zassert_mem_equal(getenv("UID"), M_UID, strlen(M_UID) + 1);
	zassert_mem_equal(getenv("PWD"), M_PWD, strlen(M_PWD) + 1);
}

ZTEST(env, test_getenv_r)
{
	static char buf[16];
	static const int exp_errno[] = {
		EINVAL, EINVAL, EINVAL, EINVAL, ENOENT, ENOENT, ENOENT, EINVAL, EINVAL, EINVAL,
	};
	static const struct args_s {
		const char *name;
		char *buf;
		size_t size;
	} args[] = {
		/* invalid input */
		{NULL, NULL, 0},
		{NULL, NULL, 42},
		{NULL, buf, 0},
		{NULL, buf, sizeof(buf)},
		{"hello", NULL, 0},
		{"hello", NULL, 42},
		{"hello", buf, 0},

		/* invalid names */
		{"", buf, sizeof(buf)},
		{"invalid=key", buf, sizeof(buf)},
		{"HOME=", buf, sizeof(buf)},
	};

	BUILD_ASSERT(ARRAY_SIZE(exp_errno) == ARRAY_SIZE(args));

	ARRAY_FOR_EACH(args, i)
	{
		errno = 0;
		zassert_equal(getenv_r(args[i].name, args[i].buf, args[i].size), -1,
			      "getenv_r(\"%s\", %p, %zu): expected to fail", args[i].name,
			      args[i].buf, args[i].size);
		zassert_equal(errno, exp_errno[i],
			      "getenv_r(\"%s\", %p, %zu): act_errno: %d exp_errno: %d",
			      args[i].name, args[i].buf, args[i].size, errno, exp_errno[i]);
	}

	zassert_mem_equal(getenv("HOME"), M_HOME, strlen(M_HOME) + 1);
	zassert_mem_equal(getenv("UID"), M_UID, strlen(M_UID) + 1);
	zassert_mem_equal(getenv("PWD"), M_PWD, strlen(M_PWD) + 1);
}

ZTEST(env, test_setenv)
{
	zassert_equal(setenv(NULL, NULL, 0), -1);
	zassert_equal(errno, EINVAL);

	/*
	 * bug in picolibc / newlib
	 * https://github.com/picolibc/picolibc/issues/648
	 */
	zassert_equal(setenv("", "42", 0), -1);
	zassert_equal(errno, EINVAL);

	zassert_equal(setenv("invalid=key", "42", 0), -1);
	zassert_equal(errno, EINVAL);

	/* do not overwrite if environ[key] exists */
	zassert_ok(setenv("HOME", "/root", 0));
	zassert_mem_equal(getenv("HOME"), M_HOME, strlen(M_HOME) + 1);

	/* should overwrite (without malloc) */
	zassert_ok(setenv("HOME", "/root", 1));
	zassert_mem_equal(getenv("HOME"), "/root", strlen("/root") + 1);
}

ZTEST(env, test_unsetenv)
{
	/* not hardened / application should fault */
	zassert_equal(unsetenv(NULL), -1);
	zassert_equal(errno, EINVAL);

	errno = 0;
	/* bug in picolibc / newlib */
	zassert_equal(unsetenv(""), -1);
	zassert_equal(errno, EINVAL);

	zassert_equal(unsetenv("invalid=key"), -1);
	zassert_equal(errno, EINVAL);

	/* restore original environ */
	environ = old_environ;
	/* should overwrite (requires realloc) */
	zassert_ok(setenv("HOME", _m_alt_home, 1));
	zassert_mem_equal(getenv("HOME"), _m_alt_home, strlen(_m_alt_home) + 1);
	zassert_ok(unsetenv("HOME"));
	zassert_is_null(getenv("HOME"));
}

ZTEST(env, test_watertight)
{
	extern size_t posix_env_get_allocated_space(void);

	char buf[4];

	/* restore original environ, which should support realloc, free, etc */
	environ = old_environ;

	for (int i = 0; i < 256; ++i) {
		snprintf(buf, sizeof(buf), "%u", i);
		zassert_ok(setenv("COUNTER", buf, 1));
		zassert_mem_equal(getenv("COUNTER"), buf, strlen(buf));
		zassert_ok(getenv_r("COUNTER", buf, sizeof(buf)));
		zassert_equal(atoi(buf), i);
		zassert_ok(unsetenv("COUNTER"));
	}

	zassert_equal(posix_env_get_allocated_space(), 0);
}

static void before(void *arg)
{
	old_environ = environ;

	RESET_ENVIRON(home, "HOME", M_HOME);
	RESET_ENVIRON(uid, "UID", M_UID);
	RESET_ENVIRON(pwd, "PWD", M_PWD);
	environ_for_test[0] = home;
	environ_for_test[1] = uid;
	environ_for_test[2] = pwd;

	zassert_equal((environ = environ_for_test), environ_for_test);
}

static void after(void *arg)
{
	environ = old_environ;
}

ZTEST_SUITE(env, NULL, NULL, before, after, NULL);
