/*
 * Copyright (c) 2021 Advanced Climate Systems
 *
 * SPDX-License-Identiier: Apache-2.0
 */
#ifndef __INCLUDE_MGMT_HAWKBIT_CUSTOM_CONFIG_DATA_H
#define __INCLUDE_MGMT_HAWKBIT_CUSTOM_CONFIG_DATA_H

#include <data/json.h>

struct hawkbit_cfg_data {
	char *hwRevision;
	int customInt;
};

static const struct json_obj_descr json_cfg_data_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_cfg_data, hwRevision, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct hawkbit_cfg_data, customInt, JSON_TOK_NUMBER),
};

#include <mgmt/hawkbit/config_data.h>

#endif /* __INCLUDE_MGMT_HAWKBIT_CUSTOM_CONFIG_DATA_H */
