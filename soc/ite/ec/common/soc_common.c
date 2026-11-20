/*
 * Copyright (c) 2026 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include "soc_common.h"

void ite_apply_extend_control(const struct ite_extend_control *extend_ctrl, const size_t cnt)
{
	for (size_t i = 0; i < cnt; i++) {
		const struct ite_extend_control *ctrl = &extend_ctrl[i];
		uint8_t reg_val = sys_read8(ctrl->addr);

		reg_val |= ctrl->enable_bitmap;
		reg_val &= ~ctrl->disable_bitmap;
		sys_write8(reg_val, ctrl->addr);
	}
}
