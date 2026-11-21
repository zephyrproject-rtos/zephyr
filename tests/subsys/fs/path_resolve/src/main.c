/*
 * Copyright (c) 2026 Asif Nadaf <postasif@protonmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include <string.h>

struct path_case {
	const char *in;
	const char *want;   /* NULL means fs_path_resolve() must return -EINVAL */
};

/*
 * Covers both reported defects this helper exists to close:
 *  - CVE-2026-19520 / GHSA-hq8j-54fm-vc2x: ".." parent-directory segments in
 *    an fs_mgmt path, which ext2 resolves but a prefix-allow-list hook does
 *    not expect.
 *  - GHSA-r79v-mj76-vf4w: FatFS silently stripping trailing '.', ' ' and
 *    control bytes from a path component, so a filename- or directory-based
 *    deny-list hook can be bypassed by a spelling it does not recognise.
 */
static const struct path_case cases[] = {
	/* plain paths: nothing to normalise */
	{"/SD:/public/readme.txt", "/SD:/public/readme.txt"},
	{"SD:/public/readme.txt", "SD:/public/readme.txt"},

	/* dot-segment resolution */
	{"/SD:/private/../public/secret", "/SD:/public/secret"},
	{"SD:/private/../public/secret", "SD:/public/secret"},
	{"/SD:/public/./secret", "/SD:/public/secret"},
	{"/SD:/a/b/../../c", "/SD:/c"},
	{"/SD:/a/./b/../c", "/SD:/a/c"},
	{"/SD:/..", "/"},
	{"/../../etc/passwd", "/etc/passwd"},
	{"/SD:/private/..", "/SD:/"},
	{"/SD:/private/../..", "/"},
	{"..", ""},
	{".", ""},
	{"", ""},

	/*
	 * '\' is NOT a separator: it's an ordinary byte in a component name.
	 * This runs before dispatch to any specific filesystem, so it cannot
	 * assume FatFS's own IsSeparator() rule applies to this path.
	 */
	{"/SD:/a\\b/c", "/SD:/a\\b/c"},
	{"/SD:/.\\x", "/SD:/.\\x"},

	/* FatFS trailing-junk stripping on the final component */
	{"/SD:/private/key.bin.", "/SD:/private/key.bin"},
	{"/SD:/private/key.bin ", "/SD:/private/key.bin"},
	{"/SD:/private/key.bin  ", "/SD:/private/key.bin"},
	{"/SD:/private/key.bin. ", "/SD:/private/key.bin"},
	{"/SD:/private/key.bin .", "/SD:/private/key.bin"},
	{"/SD:/private/key.bin...", "/SD:/private/key.bin"},
	{"/SD:/private/key.bin\x09", "/SD:/private/key.bin"},
	{"/SD:/private/key.bin\x01", "/SD:/private/key.bin"},

	/* FatFS trailing-junk stripping on an intermediate (directory) component */
	{"/SD:/private /key.bin", "/SD:/private/key.bin"},
	{"/SD:/private./key.bin", "/SD:/private/key.bin"},
	{"/SD:/private../key.bin", "/SD:/private/key.bin"},

	/* a component that is entirely junk: reject, matching FatFS's own
	 * "reject a null name" behaviour
	 */
	{"/SD:/private/...", NULL},
	{"/SD:/private/   ", NULL},
	{"/SD:/.../key.bin", NULL},
	{"/SD:/   /key.bin", NULL},

	/* dot-segments and trailing junk combined in one path */
	{"/SD:/private./../public/secret.", "/SD:/public/secret"},
	{"/SD:/a./b. /../c ", "/SD:/a/c"},
};

ZTEST_SUITE(fs_path_resolve, NULL, NULL, NULL, NULL, NULL);

ZTEST(fs_path_resolve, test_probe_table)
{
	char buf[256];

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		int rc;

		strncpy(buf, cases[i].in, sizeof(buf) - 1);
		buf[sizeof(buf) - 1] = '\0';

		rc = fs_path_resolve(buf);

		if (cases[i].want == NULL) {
			zassert_equal(rc, -EINVAL,
				      "case %zu: in=\"%s\" expected -EINVAL, got rc=%d out=\"%s\"",
				      i, cases[i].in, rc, buf);
			continue;
		}

		zassert_equal(rc, 0, "case %zu: in=\"%s\" expected success, got rc=%d", i,
			     cases[i].in, rc);
		zassert_str_equal(buf, cases[i].want, "case %zu: in=\"%s\" want=\"%s\" got=\"%s\"",
				  i, cases[i].in, cases[i].want, buf);
		zassert_true(strlen(buf) <= strlen(cases[i].in),
			    "case %zu: output longer than input", i);
	}
}

ZTEST(fs_path_resolve, test_idempotent)
{
	char buf[256];

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		int rc;

		if (cases[i].want == NULL) {
			continue;
		}

		strncpy(buf, cases[i].want, sizeof(buf) - 1);
		buf[sizeof(buf) - 1] = '\0';

		rc = fs_path_resolve(buf);

		zassert_equal(rc, 0, "case %zu: already-resolved input rejected", i);
		zassert_str_equal(buf, cases[i].want,
				  "case %zu: resolving an already-resolved path changed it", i);
	}
}
