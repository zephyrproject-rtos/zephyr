/*
 * Copyright (c) The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <zephyr/posix/fnmatch.h>

#include <zephyr/ztest.h>

/*
 * Note: the \x00 control character is specifically excluded below, since testing for it is
 * equivalent to reading past the end of a '\0'-terminated string (i.e. can fault).
 */
#define TEST_BLANK_CHARS " \t"
#define TEST_CNTRL_CHARS                                                                           \
	"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16" \
	"\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x7f"
#define TEST_DIGIT_CHARS  "0123456789"
#define TEST_LOWER_CHARS  "abcdefghijklmnopqrstuvwxyz"
#define TEST_PUNCT_CHARS  "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
#define TEST_SPACE_CHARS  " \f\n\r\t\v"
#define TEST_UPPER_CHARS  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define TEST_XDIGIT_CHARS "0123456789ABCDEFabcdef"

/*
 * Adapted from
 * https://git.musl-libc.org/cgit/libc-testsuite/tree/fnmatch.c
 */
ZTEST(posix_c_lib_ext, test_fnmatch)
{
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
	/*
	 * '\' in pattern escapes ']'. bracket expression is incomplete. pattern is interpreted as
	 * literal sequence '[[?*\]'. which does not match input '\'
	 */
	zassert_equal(fnmatch("[[?*\\]", "\\", 0), FNM_NOMATCH);
	/* '\' in pattern does not escape ']'. bracket expression complete. '\' matches input '\' */
	zassert_ok(fnmatch("[[?*\\]", "\\", FNM_NOESCAPE));
	/* '\' in pattern escapes '\', match '\' */
	zassert_ok(fnmatch("[[?*\\\\]", "\\", 0));
	/*
	 * "[]" (empty bracket expression) is an invalid pattern.
	 * > The ( ']' ) shall lose its special meaning and represent itself in a bracket expression
	 * > if it occurs first in the list (after an initial ( '^' ), if any)
	 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap09.html#tag_09_03_05
	 *
	 * So the next test is (again) and incomplete bracket expression and should return error.
	 * The two tests that follow it also require the ']' to be treated as a literal character to
	 * match within the bracket expression.
	 */
	zassert_equal(fnmatch("[]?*\\]", "]", 0), FNM_NOMATCH);
	/* '\' in pattern does not escape. bracket expression complete. ']' matches input ']' */
	zassert_ok(fnmatch("[]?*\\]", "]", FNM_NOESCAPE));
	/* '\' in pattern escapes '\'. bracket expression complete. ']' matches input ']' */
	zassert_ok(fnmatch("[]?*\\\\]", "]", 0));

	zassert_ok(fnmatch("[!]a-]", "b", 0));
	zassert_ok(fnmatch("[]-_]", "^", 0));
	zassert_ok(fnmatch("[!]-_]", "X", 0));
	zassert_equal(fnmatch("??", "-", 0), FNM_NOMATCH);
	zassert_equal(fnmatch("*LIB*", "lib", FNM_PERIOD), FNM_NOMATCH);
	zassert_ok(fnmatch("a[/]b", "a/b", 0));
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
	zassert_equal(fnmatch("[.]a/b", ".a/b", FNM_PATHNAME | FNM_PERIOD), FNM_NOMATCH);
	zassert_equal(fnmatch("a/[.]b", "a/.b", FNM_PATHNAME | FNM_PERIOD), FNM_NOMATCH);
	zassert_ok(fnmatch("*/?", "a/b", FNM_PATHNAME | FNM_PERIOD));
	zassert_ok(fnmatch("?/*", "a/b", FNM_PATHNAME | FNM_PERIOD));
	zassert_ok(fnmatch(".*/?", ".a/b", FNM_PATHNAME | FNM_PERIOD));
	zassert_ok(fnmatch("*/.?", "a/.b", FNM_PATHNAME | FNM_PERIOD));
	zassert_equal(fnmatch("*/*", "a/.b", FNM_PATHNAME | FNM_PERIOD), FNM_NOMATCH);
	zassert_ok(fnmatch("*?*/*", "a/.b", FNM_PERIOD));
	zassert_ok(fnmatch("*[.]/b", "a./b", FNM_PATHNAME | FNM_PERIOD));
	zassert_ok(fnmatch("*[[:alpha:]]/*[[:alnum:]]", "a/b", FNM_PATHNAME));
	zassert_ok(fnmatch("*[![:digit:]]*/[![:d-d]", "a/b", FNM_PATHNAME), 0);
	zassert_ok(fnmatch("*[![:digit:]]*/[[:d-d]", "a/[", FNM_PATHNAME), 0);
	zassert_equal(fnmatch("*[![:digit:]]*/[![:d-d]", "a/[", FNM_PATHNAME), FNM_NOMATCH);
	zassert_ok(fnmatch("a?b", "a.b", FNM_PATHNAME | FNM_PERIOD));
	zassert_ok(fnmatch("a*b", "a.b", FNM_PATHNAME | FNM_PERIOD));
	zassert_ok(fnmatch("a[.]b", "a.b", FNM_PATHNAME | FNM_PERIOD));

	/* Additional test cases for POSIX character classes (C-locale only) */
	static const struct test_data_s {
		const char *pattern;
		const char *match;
		const char *nomatch;
	} test_data[] = {
		{"[[:alnum:]]", TEST_DIGIT_CHARS TEST_UPPER_CHARS TEST_LOWER_CHARS, " "},
		{"[[:alpha:]]", TEST_UPPER_CHARS TEST_LOWER_CHARS, "0"},
		{"[[:blank:]]", TEST_BLANK_CHARS, "x"},
		{"[[:cntrl:]]", TEST_CNTRL_CHARS, "x"},
		{"[[:digit:]]", TEST_DIGIT_CHARS, "a"},
		{"[[:graph:]]", TEST_DIGIT_CHARS TEST_UPPER_CHARS TEST_LOWER_CHARS TEST_PUNCT_CHARS,
		 " "},
		{"[[:lower:]]", TEST_LOWER_CHARS, "X"},
		{"[[:print:]]",
		 TEST_DIGIT_CHARS TEST_UPPER_CHARS TEST_LOWER_CHARS TEST_PUNCT_CHARS " ", "\t"},
		{"[[:punct:]]", TEST_PUNCT_CHARS, "x"},
		{"[[:space:]]", TEST_SPACE_CHARS, "x"},
		{"[[:upper:]]", TEST_UPPER_CHARS, "x"},
		{"[[:xdigit:]]", TEST_XDIGIT_CHARS, "h"},
	};

	ARRAY_FOR_EACH_PTR(test_data, data) {
		/* ensure that characters in "nomatch" do not match "pattern" */
		for (size_t j = 0; j < strlen(data->nomatch); j++) {
			char c = data->nomatch[j];
			char input[] = {c, '\0'};

			zexpect_equal(fnmatch(data->pattern, input, 0), FNM_NOMATCH,
				      "pattern \"%s\" unexpectedly matched char 0x%02x (%c)",
				      data->pattern, c, isprint(c) ? c : '.');
		}

		/* ensure that characters in "match" do match "pattern" */
		for (size_t j = 0; j < strlen(data->match); j++) {
			char c = data->match[j];
			char input[] = {c, '\0'};

			zexpect_ok(fnmatch(data->pattern, input, 0),
				   "pattern \"%s\" did not match char 0x%02x (%c)", data->pattern,
				   c, isprint(c) ? c : '.');
		}
	}

	/* ensure that an invalid character class generates an error */
	zassert_equal(fnmatch("[[:foobarbaz:]]", "Z", 0), FNM_NOMATCH);
}
