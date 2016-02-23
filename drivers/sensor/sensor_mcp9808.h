/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SENSOR_MCP9808_H__
#define __SENSOR_MCP9808_H__

#include <stdint.h>
#include <device.h>
#include <misc/util.h>

#define MCP9808_REG_TEMP_AMB		0x05

#define MCP9808_SIGN_BIT		BIT(12)
#define MCP9808_TEMP_INT_MASK		0x0ff0
#define MCP9808_TEMP_INT_SHIFT		4
#define MCP9808_TEMP_FRAC_MASK		0x000f

struct mcp9808_data {
	struct device *i2c_master;
	uint16_t i2c_slave_addr;

	uint16_t reg_val;
};

#endif /* __SENSOR_MCP9808_H__ */
