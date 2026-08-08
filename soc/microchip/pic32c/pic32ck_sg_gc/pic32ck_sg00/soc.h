/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CK_SG00_H_
#define SOC_MICROCHIP_PIC32CK_SG00_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CK0512SG00064)
#include "pic32ck0512sg00064.h"
#elif defined(CONFIG_SOC_PIC32CK0512SG00100)
#include "pic32ck0512sg00100.h"
#elif defined(CONFIG_SOC_PIC32CK1012SG00048)
#include "pic32ck1012sg00048.h"
#elif defined(CONFIG_SOC_PIC32CK1012SG00064)
#include "pic32ck1012sg00064.h"
#elif defined(CONFIG_SOC_PIC32CK1012SG00100)
#include "pic32ck1012sg00100.h"
#elif defined(CONFIG_SOC_PIC32CK1025SG00064)
#include "pic32ck1025sg00064.h"
#elif defined(CONFIG_SOC_PIC32CK1025SG00100)
#include "pic32ck1025sg00100.h"
#elif defined(CONFIG_SOC_PIC32CK1025SG00144)
#include "pic32ck1025sg00144.h"
#elif defined(CONFIG_SOC_PIC32CK2051SG00064)
#include "pic32ck2051sg00064.h"
#elif defined(CONFIG_SOC_PIC32CK2051SG00100)
#include "pic32ck2051sg00100.h"
#elif defined(CONFIG_SOC_PIC32CK2051SG00144)
#include "pic32ck2051sg00144.h"
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_PIC32CK_SG00_H_ */
