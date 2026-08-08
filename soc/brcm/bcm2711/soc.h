/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#ifndef _ASMLANGUAGE

/*
 * The following definitions are required for the inclusion of the CMSIS
 * Common Peripheral Access Layer for aarch64 Cortex-A CPUs:
 */

#define __CORTEX_A 72U

#define BCM2711_CPU_TO_VC_ADDR(x) (((x) & 0x3FFFFFFFUL) | 0xC0000000UL)
#define BCM2711_VC_TO_CPU_ADDR(x) ((x) & 0x3FFFFFFFUL)

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
