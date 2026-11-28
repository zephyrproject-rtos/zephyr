/*
 * Copyright (c) 2026 Asif Nadaf <postasif@protonmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/fs/fs.h>

static bool is_separator(char c)
{
	return c == '/';
}

/*
 * Resolve "." and ".." path segments in place, RFC 3986-style. Adapted from
 * http_server_remove_dot_segments() (subsys/net/lib/http/http_server_core.c),
 * with the query/fragment handling dropped (meaningless for a filesystem
 * path). '/' is the only separator recognised: this runs before the path is
 * dispatched to any specific mounted filesystem, so it cannot know whether a
 * '\' in the input is meant as a separator or is just an ordinary byte in a
 * component name on whatever filesystem ends up handling it.
 *
 * The output is always shorter than or equal in length to the input, so the
 * in-place rewrite is always safe.
 */
static void resolve_dot_segments(char *path)
{
	char *in = path;
	char *out = path;

	while (*in != '\0') {
		if (in[0] == '.') {
			if (is_separator(in[1])) {
				in += 2;
				continue;
			} else if ((in[1] == '.') && is_separator(in[2])) {
				in += 3;
				continue;
			} else if (in[1] == '\0') {
				in += 1;
				continue;
			} else if ((in[1] == '.') && (in[2] == '\0')) {
				in += 2;
				continue;
			}
		} else if (is_separator(in[0]) && (in[1] == '.')) {
			if (is_separator(in[2])) {
				in += 2;
				continue;
			} else if (in[2] == '\0') {
				in += 2;
				*out++ = '/';
				continue;
			} else if ((in[2] == '.') && is_separator(in[3])) {
				in += 3;
				while ((out > path) && (*(out - 1) != '/')) {
					out--;
				}
				if (out > path) {
					out--;
				}
				continue;
			} else if ((in[2] == '.') && (in[3] == '\0')) {
				in += 3;
				while ((out > path) && (*(out - 1) != '/')) {
					out--;
				}
				if (out > path) {
					out--;
				}
				*out++ = '/';
				continue;
			}
		}

		/*
		 * Move the first segment to output: a canonicalised leading
		 * separator (if any) plus all non-separator bytes up to the
		 * next segment boundary.
		 */
		if (is_separator(*in)) {
			*out++ = '/';
			in++;
		} else {
			*out++ = *in++;
		}

		while ((*in != '\0') && !is_separator(*in)) {
			*out++ = *in++;
		}
	}

	*out = '\0';
}

/*
 * Strip trailing '.', ' ' and any byte below 0x20 from each path component,
 * in place, matching the normalisation FatFS's create_name() applies to the
 * final component of a path when it builds or matches a short (8.3)
 * directory entry (ff.c, both the FF_USE_LFN and non-LFN branches snip the
 * same set of trailing bytes). Operates on a path that has already been
 * through resolve_dot_segments(), so '/' is the only separator byte present.
 *
 * Returns -EINVAL if any component is entirely made up of the bytes being
 * stripped -- the same "reject a null name" case FatFS itself applies -- so
 * that a caller never ends up comparing against, or opening, a path whose
 * meaning silently changed underneath it.
 */
static int strip_trailing_component_junk(char *path)
{
	char *in = path;
	char *out = path;

	while (*in != '\0') {
		char *comp_start = out;
		bool comp_had_bytes = false;

		while ((*in != '\0') && (*in != '/')) {
			*out++ = *in++;
			comp_had_bytes = true;
		}

		while ((out > comp_start) &&
		       ((out[-1] == '.') || (out[-1] == ' ') ||
			((unsigned char)out[-1] < 0x20))) {
			out--;
		}

		if (comp_had_bytes && (out == comp_start)) {
			return -EINVAL;
		}

		if (*in == '/') {
			*out++ = *in++;
		}
	}

	*out = '\0';

	return 0;
}

int fs_path_resolve(char *path)
{
	resolve_dot_segments(path);

	return strip_trailing_component_junk(path);
}
