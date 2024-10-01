/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>

#define TRACK_ALLOC (IS_ENABLED(CONFIG_POSIX_ENV_LOG_LEVEL_DBG) || IS_ENABLED(CONFIG_ZTEST))

LOG_MODULE_REGISTER(posix_env, CONFIG_POSIX_ENV_LOG_LEVEL);

static struct k_spinlock environ_lock;
static size_t allocated;
char **environ;

#ifdef CONFIG_ZTEST
size_t posix_env_get_allocated_space(void)
{
	return allocated;
}
#endif

static size_t environ_size(void)
{
	size_t ret;

	if (environ == NULL) {
		return 0;
	}

	for (ret = 0; environ[ret] != NULL; ++ret) {
	}

	return ret;
}

static int findenv(const char *name, size_t namelen)
{
	const char *env;

	if (name == NULL || namelen == 0 || strchr(name, '=') != NULL) {
		/* Note: '=' is not a valid name character */
		return -EINVAL;
	}

	if (environ == NULL) {
		return -ENOENT;
	}

	for (char **envp = &environ[0]; *envp != NULL; ++envp) {
		env = *envp;
		if (strncmp(env, name, namelen) == 0 && env[namelen] == '=') {
			return envp - environ;
		}
	}

	return -ENOENT;
}

char *getenv(const char *name)
{
	int ret;
	size_t nsize;
	char *val = NULL;

	nsize = (name == NULL) ? 0 : strlen(name);
	K_SPINLOCK(&environ_lock)
	{
		ret = findenv(name, nsize);
		if (ret < 0) {
			K_SPINLOCK_BREAK;
		}

		val = environ[ret] + nsize + 1;
	}

	return val;
}

int getenv_r(const char *name, char *buf, size_t len)
{
	int ret = 0;
	size_t vsize;
	size_t nsize;
	char *val = NULL;

	nsize = (name == NULL) ? 0 : strlen(name);
	K_SPINLOCK(&environ_lock)
	{
		ret = findenv(name, nsize);
		if (ret < 0) {
			LOG_DBG("No entry for name '%s'", name);
			K_SPINLOCK_BREAK;
		}

		val = environ[ret] + nsize + 1;
		vsize = strlen(val) + 1;
		if (vsize > len) {
			ret = -ERANGE;
			K_SPINLOCK_BREAK;
		}
		strcpy(buf, val);
		LOG_DBG("Found entry %s", environ[ret]);
	}

	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}

int setenv(const char *name, const char *val, int overwrite)
{
	int ret = 0;
	char *env;
	char **envp;
	size_t esize;
	const size_t vsize = (val == NULL) ? 0 : strlen(val);
	const size_t nsize = (name == NULL) ? 0 : strlen(name);
	/* total size of name + '=' + val + '\0' */
	const size_t tsize = nsize + 1 /* '=' */ + vsize + 1 /* '\0' */;

	if (name == NULL || val == NULL) {
		LOG_DBG("Invalid name '%s' or value '%s'", name, val);
		errno = EINVAL;
		return -1;
	}

	K_SPINLOCK(&environ_lock)
	{
		ret = findenv(name, nsize);
		if (ret == -EINVAL) {
			LOG_DBG("Invalid name '%s'", name);
			K_SPINLOCK_BREAK;
		}
		if (ret >= 0) {
			/* name was found in environ */
			esize = strlen(environ[ret]) + 1;
			if (overwrite == 0) {
				LOG_DBG("Found entry %s", environ[ret]);
				ret = 0;
				K_SPINLOCK_BREAK;
			}
		} else {
			/* name was not found in environ -> add new entry */
			esize = environ_size();
			envp = realloc(environ, sizeof(char **) *
							(esize + 1 /* new entry */ + 1 /* NULL */));
			if (envp == NULL) {
				ret = -ENOMEM;
				K_SPINLOCK_BREAK;
			}

			if (TRACK_ALLOC) {
				allocated += sizeof(char **) * (esize + 2);
				LOG_DBG("realloc %zu bytes (allocated: %zu)",
					sizeof(char **) * (esize + 2), allocated);
			}

			environ = envp;
			ret = esize;
			environ[ret] = NULL;
			environ[ret + 1] = NULL;
			esize = 0;
		}

		if (esize < tsize) {
			/* need to malloc or realloc space for new environ entry */
			env = realloc(environ[ret], tsize);
			if (env == NULL) {
				ret = -ENOMEM;
				K_SPINLOCK_BREAK;
			}
			if (TRACK_ALLOC) {
				allocated += tsize - esize;
				LOG_DBG("realloc %zu bytes (allocated: %zu)", tsize - esize,
					allocated);
			}
			environ[ret] = env;
		}

		strcpy(environ[ret], name);
		environ[ret][nsize] = '=';
		strncpy(environ[ret] + nsize + 1, val, vsize + 1);
		LOG_DBG("Added entry %s", environ[ret]);

		ret = 0;
	}

	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}

int unsetenv(const char *name)
{
	int ret = 0;
	char **envp;
	size_t esize;
	size_t nsize;

	nsize = (name == NULL) ? 0 : strlen(name);
	K_SPINLOCK(&environ_lock)
	{
		ret = findenv(name, nsize);
		if (ret < 0) {
			ret = (ret == -EINVAL) ? -EINVAL : 0;
			K_SPINLOCK_BREAK;
		}

		esize = environ_size();
		if (TRACK_ALLOC) {
			allocated -= strlen(environ[ret]) + 1;
			LOG_DBG("free %zu bytes (allocated: %zu)", strlen(environ[ret]) + 1,
				allocated);
		}
		free(environ[ret]);

		/* shuffle remaining environment variable pointers forward */
		for (; ret < esize; ++ret) {
			environ[ret] = environ[ret + 1];
		}
		/* environ must be terminated with a NULL pointer */
		environ[ret] = NULL;

		/* reduce environ size and update allocation */
		--esize;
		if (esize == 0) {
			free(environ);
			environ = NULL;
		} else {
			envp = realloc(environ, (esize + 1 /* NULL */) * sizeof(char **));
			if (envp != NULL) {
				environ = envp;
			}
		}
		__ASSERT_NO_MSG((esize >= 1 && environ != NULL) || environ == NULL);

		if (TRACK_ALLOC) {
			/* recycle nsize here */
			nsize = ((esize == 0) ? 2 : 1) * sizeof(char **);
			allocated -= nsize;
			LOG_DBG("free %zu bytes (allocated: %zu)", nsize, allocated);
		}

		ret = 0;
	}

	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}
