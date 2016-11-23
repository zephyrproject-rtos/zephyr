/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
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

#ifndef __INCLUDE_PINMUX_KSDK_H
#define __INCLUDE_PINMUX_KSDK_H

#include <device.h>
#include <fsl_port.h>

int pinmux_ksdk_init(void);

static inline int pinmux_ksdk_set(PORT_Type *base, uint32_t pin, uint32_t func)
{
	base->PCR[pin] = func;
	return 0;
}

static inline int pinmux_ksdk_get(PORT_Type *base, uint32_t pin, uint32_t *func)
{
	*func = base->PCR[pin];
	return 0;
}

#endif /* __INCLUDE_PINMUX_KSDK_H */
