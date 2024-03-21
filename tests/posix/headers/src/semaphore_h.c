/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <semaphore.h>
#else
#include <zephyr/posix/semaphore.h>
#endif

/**
 * @brief existence test for `<semaphore.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/semaphore.h.html">semaphore.h</a>
 */
ZTEST(posix_headers, test_semaphore_h)
{
	/* zassert_not_equal(SEM_FAILED, (sem_t *)42); */ /* not implemented */

	if (IS_ENABLED(CONFIG_POSIX_API)) {
		zassert_not_null(sem_close);
		zassert_not_null(sem_destroy);
		zassert_not_null(sem_getvalue);
		zassert_not_null(sem_init);
		zassert_not_null(sem_open);
		zassert_not_null(sem_post);
		zassert_not_null(sem_timedwait);
		zassert_not_null(sem_trywait);
		zassert_not_null(sem_unlink);
		zassert_not_null(sem_wait);
	}
}
