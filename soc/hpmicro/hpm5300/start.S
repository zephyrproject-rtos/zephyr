/*
 * Copyright (c) 2026 Cody Gu <gujiaqi@iscas.ac.cn>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

GTEXT(_start)

SECTION_FUNC(reset, _start)
	csrrw x0, mstatus, x0
	j __start
