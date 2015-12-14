/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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

/**
 * @file
 * @brief TI LM3S6965 System Control Peripherals interface
 *
 *
 * Library for controlling target-specific devices present in the 0x400fe000
 * peripherals memory region.
 *
 * Currently, only enabling the main OSC with default value is implemented.
 */

#include <stdint.h>
#include <toolchain.h>
#include <sections.h>

#include "scp.h"

/* System Control Peripheral (SCP) Registers */

volatile struct __scp __scp_section __scp;

/**
 *
 * @brief Enable main oscillator with default frequency of 6MHz
 *
 * @return N/A
 */
void _ScpMainOscEnable(void)
{
	union __rcc reg;

	reg.value = __scp.clock.rcc.value;
	reg.bit.moscdis = 0;
	reg.bit.oscsrc = _SCP_OSC_SOURCE_MAIN;
	reg.bit.xtal = _SCP_CRYSTAL_6MHZ;

	__scp.clock.rcc.value = reg.value;
}
