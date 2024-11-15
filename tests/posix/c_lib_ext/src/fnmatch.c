/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/posix/fnmatch.h>
#include <zephyr/ztest.h>

/*
 * Adapted from
 * https://git.musl-libc.org/cgit/libc-testsuite/tree/fnmatch.c
 */
ZTEST(posix_c_lib_ext, test_fnmatch)
{
	/* Note: commented out lines indicate known problems to be addressed in #55186 */

	zassert_ok(fnmatch("*.c", "foo.c", 0));
	zassert_ok(fnmatch("*.c", ".c", 0));
	zassert_equal(fnmatch("*.a", "foo.c", 0), FNM_NOMATCH);
	zassert_ok(fnmatch("*.c", ".foo.c", 0));
	zassert_equal(fnmatch("*.c", ".foo.c", FNM_PERIOD), FNM_NOMATCH);
	zassert_ok(fnmatch("*.c", "foo.c", FNM_PERIOD));
	zassert_equal(fnmatch("a\\*.c", "a*.c", FNM_NOESCAPE), FNM_NOMATCH);
	zassert_equal(fnmatch("a\\*.c", "ax.c", 0), FNM_NOMATCH);
	zassert_ok(fnmatch("a[xy].c", "ax.c", 0));
	zassert_ok(fnmatch("a[!y].c", "ax.c", 0));
	zassert_equal(fnmatch("a[a/z]*.c", "a/x.c", FNM_PATHNAME), FNM_NOMATCH);
	zassert_ok(fnmatch("a/*.c", "a/x.c", FNM_PATHNAME));
	zassert_equal(fnmatch("a*.c", "a/x.c", FNM_PATHNAME), FNM_NOMATCH);
	zassert_ok(fnmatch("*/foo", "/foo", FNM_PATHNAME));
	zassert_ok(fnmatch("-O[01]", "-O1", 0));
	/* zassert_ok(fnmatch("[[?*\\]", "\\", 0)); */
	/* zassert_ok(fnmatch("[]?*\\]", "]", 0)); */
	zassert_ok(fnmatch("[!]a-]", "b", 0));
	zassert_ok(fnmatch("[]-_]", "^", 0));
	zassert_ok(fnmatch("[!]-_]", "X", 0));
	zassert_equal(fnmatch("??", "-", 0), FNM_NOMATCH);
	zassert_equal(fnmatch("*LIB*", "lib", FNM_PERIOD), FNM_NOMATCH);
	/* zassert_ok(fnmatch("a[/]b", "a/b", 0)); */
	zassert_equal(fnmatch("a[/]b", "a/b", FNM_PATHNAME), FNM_NOMATCH);
	zassert_ok(fnmatch("[a-z]/[a-z]", "a/b", 0));
	zassert_equal(fnmatch("*", "a/b", FNM_PATHNAME), FNM_NOMATCH);
	zassert_equal(fnmatch("*[/]b", "a/b", FNM_PATHNAME), FNM_NOMATCH);
	zassert_equal(fnmatch("*[b]", "a/b", FNM_PATHNAME), FNM_NOMATCH);
	zassert_equal(fnmatch("[*]/b", "a/b", 0), FNM_NOMATCH);
	zassert_ok(fnmatch("[*]/b", "*/b", 0));
	zassert_equal(fnmatch("[?]/b", "a/b", 0), FNM_NOMATCH);
	zassert_ok(fnmatch("[?]/b", "?/b", 0));
	zassert_ok(fnmatch("[[a]/b", "a/b", 0));
	zassert_ok(fnmatch("[[a]/b", "[/b", 0));
	zassert_equal(fnmatch("\\*/b", "a/b", 0), FNM_NOMATCH);
	zassert_ok(fnmatch("\\*/b", "*/b", 0));
	zassert_equal(fnmatch("\\?/b", "a/b", 0), FNM_NOMATCH);
	zassert_ok(fnmatch("\\?/b", "?/b", 0));
	zassert_ok(fnmatch("[/b", "[/b", 0));
	zassert_ok(fnmatch("\\[/b", "[/b", 0));
	zassert_ok(fnmatch("??"
			   "/b",
			   "aa/b", 0));
	zassert_ok(fnmatch("???b", "aa/b", 0));
	zassert_equal(fnmatch("???b", "aa/b", FNM_PATHNAME), FNM_NOMATCH);
	zassert_equal(fnmatch("?a/b", ".a/b", FNM_PATHNAME | FNM_PERIOD), FNM_NOMATCH);
	zassert_equal(fnmatch("a/?b", "a/.b", FNM_PATHNAME | FNM_PERIOD), FNM_NOMATCH);
	zassert_equal(fnmatch("*a/b", ".a/b", FNM_PATHNAME | FNM_PERIOD), FNM_NOMATCH);
	zassert_equal(fnmatch("a/*b", "a/.b", FNM_PATHNAME | FNM_PERIOD), FNM_NOMATCH);
	/* zassert_equal(fnmatch("[.]a/b", ".a/b", FNM_PATHNAME | FNM_PERIOD), FNM_NOMATCH); */
	/* zassert_equal(fnmatch("a/[.]b", "a/.b", FNM_PATHNAME | FNM_PERIOD), FNM_NOMATCH); */
	zassert_ok(fnmatch("*/?", "a/b", FNM_PATHNAME | FNM_PERIOD));
	zassert_ok(fnmatch("?/*", "a/b", FNM_PATHNAME | FNM_PERIOD));
	zassert_ok(fnmatch(".*/?", ".a/b", FNM_PATHNAME | FNM_PERIOD));
	zassert_ok(fnmatch("*/.?", "a/.b", FNM_PATHNAME | FNM_PERIOD));
	zassert_equal(fnmatch("*/*", "a/.b", FNM_PATHNAME | FNM_PERIOD), FNM_NOMATCH);
	zassert_ok(fnmatch("*?*/*", "a/.b", FNM_PERIOD));
	zassert_ok(fnmatch("*[.]/b", "a./b", FNM_PATHNAME | FNM_PERIOD));
	/* zassert_ok(fnmatch("*[[:alpha:]]/""*[[:alnum:]]", "a/b", FNM_PATHNAME)); */
	zassert_not_equal(fnmatch("*[![:digit:]]*/[![:d-d]", "a/b", FNM_PATHNAME), 0);
	zassert_not_equal(fnmatch("*[![:digit:]]*/[[:d-d]", "a/[", FNM_PATHNAME), 0);
	zassert_not_equal(fnmatch("*[![:digit:]]*/[![:d-d]", "a/[", FNM_PATHNAME), 0);
	zassert_ok(fnmatch("a?b", "a.b", FNM_PATHNAME | FNM_PERIOD));
	zassert_ok(fnmatch("a*b", "a.b", FNM_PATHNAME | FNM_PERIOD));
	zassert_ok(fnmatch("a[.]b", "a.b", FNM_PATHNAME | FNM_PERIOD));
}
