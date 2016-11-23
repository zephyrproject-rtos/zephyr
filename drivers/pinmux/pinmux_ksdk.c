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

#include <pinmux/pinmux_ksdk.h>
#include <fsl_common.h>
#include <fsl_clock.h>

int pinmux_ksdk_init(void)
{
#ifdef CONFIG_PINMUX_KSDK_PORTA
	CLOCK_EnableClock(kCLOCK_PortA);
#endif
#ifdef CONFIG_PINMUX_KSDK_PORTB
	CLOCK_EnableClock(kCLOCK_PortB);
#endif
#ifdef CONFIG_PINMUX_KSDK_PORTC
	CLOCK_EnableClock(kCLOCK_PortC);
#endif
#ifdef CONFIG_PINMUX_KSDK_PORTD
	CLOCK_EnableClock(kCLOCK_PortD);
#endif
#ifdef CONFIG_PINMUX_KSDK_PORTE
	CLOCK_EnableClock(kCLOCK_PortE);
#endif
	return 0;
}
