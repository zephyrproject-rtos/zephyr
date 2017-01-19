/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
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
