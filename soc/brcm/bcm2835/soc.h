/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#ifndef _ASMLANGUAGE

/*
 * CMSIS Core-A support expects a SoC header to declare the A-profile core
 * generation. ARM1176 is treated as the minimal A-profile variant while the
 * broader Zephyr ARM11 support is still being brought up.
 */
#define __CORTEX_A 5U

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
