/*
 * Copyright (c) 2016 Linaro Limited.
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

#ifndef _ARM_CLOCK_CONTROL_H_
#define _ARM_CLOCK_CONTROL_H_

#include <clock_control.h>

/**
 * @file
 *
 * @brief Clock subsystem IDs for ARM family SoCs
 */

/* CMSDK BUS Mapping */
enum arm_bus_type_t {
	CMSDK_AHB = 0,
	CMSDK_APB,
};

/* CPU States */
enum arm_soc_state_t {
	SOC_ACTIVE = 0,
	SOC_SLEEP,
	SOC_DEEPSLEEP,
};

struct arm_clock_control_t {
	/* ARM family SoCs supported Bus types */
	enum arm_bus_type_t bus;
	/* Clock can be configured for 3 states: Active, Sleep, Deep Sleep */
	enum arm_soc_state_t state;
	/* Identifies the device on the bus */
	uint32_t device;
};

#endif /* _ARM_CLOCK_CONTROL_H_ */
