/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Intel Corporation
 *
 * __popcountsi2 — compiled with -mtext-section-literals (see CMakeLists.txt)
 * so that all literal-pool entries are interleaved with the code and remain
 * within l32r reach.  This overrides the precompiled libgcc version, whose
 * separate .literal section can fall out of range when the image is large.
 */

int __popcountsi2(unsigned int x)
{
	x = x - ((x >> 1) & 0x55555555u);
	x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
	x = (x + (x >> 4)) & 0x0f0f0f0fu;
	return (int)((x * 0x01010101u) >> 24);
}
