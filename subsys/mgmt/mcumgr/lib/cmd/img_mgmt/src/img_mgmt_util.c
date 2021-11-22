/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/mcumgr_util.h"
#include "img_mgmt/image.h"
#include "img_mgmt/img_mgmt.h"

int
img_mgmt_ver_str(const struct image_version *ver, char *dst)
{
	int off = 0;

	off += ull_to_s(ver->iv_major, INT_MAX, dst + off);

	dst[off++] = '.';
	off += ull_to_s(ver->iv_minor, INT_MAX, dst + off);

	dst[off++] = '.';
	off += ull_to_s(ver->iv_revision, INT_MAX, dst + off);

	if (ver->iv_build_num != 0) {
		dst[off++] = '.';
		off += ull_to_s(ver->iv_build_num, INT_MAX, dst + off);
	}

	return 0;
}
