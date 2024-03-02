/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nus_internal.h"

#if defined(CONFIG_BT_NUS_DEFAULT_INSTANCE)
BT_NUS_INST_DEFINE(nus_def);
struct bt_nus_inst *bt_nus_inst_default(void)
{
	return &nus_def;
}
#else
struct bt_nus_inst *bt_nus_inst_default(void) { return NULL; }
#endif

struct bt_nus_inst *bt_nus_inst_get_from_attr(const struct bt_gatt_attr *attr)
{
	STRUCT_SECTION_FOREACH(bt_nus_inst, instance) {
		for (size_t i = 0 ; i < instance->svc->attr_count ; i++) {
			if (attr == &instance->svc->attrs[i]) {
				return instance;
			}
		}
	}

	return NULL;
}
