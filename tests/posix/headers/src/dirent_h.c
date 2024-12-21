/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <dirent.h>
#else
#include <zephyr/posix/dirent.h>
#endif

/**
 * @brief existence test for `<dirent.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/dirent.h.html">dirent.h</a>
 */
ZTEST(posix_headers, test_dirent_h)
{
#ifdef CONFIG_POSIX_API
	zassert_not_equal((DIR *)-1, (DIR *)NULL);

	zassert_not_equal(-1, offsetof(struct dirent, d_ino));
	zassert_not_equal(-1, offsetof(struct dirent, d_name));

	/* zassert_not_null(alphasort); */ /* not implemented */
	zassert_not_null(closedir);
	/* zassert_not_null(dirfd); */     /* not implemented */
	/* zassert_not_null(fdopendir); */ /* not implemented */
	zassert_not_null(opendir);
	zassert_not_null(readdir);
	zassert_not_null(readdir_r);
	/* zassert_not_null(rewinddir); */ /* not implemented */
	/* zassert_not_null(scandir); */   /* not implemented */
	/* zassert_not_null(seekdir); */   /* not implemented */
	/* zassert_not_null(telldir); */   /* not implemented */
#endif
}
