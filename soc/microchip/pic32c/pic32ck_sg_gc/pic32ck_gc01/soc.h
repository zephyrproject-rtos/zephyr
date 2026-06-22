/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CK_GC01_H_
#define SOC_MICROCHIP_PIC32CK_GC01_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CK0512GC01064)
#include "pic32ck0512gc01064.h"
#elif defined(CONFIG_SOC_PIC32CK0512GC01100)
#include "pic32ck0512gc01100.h"
#elif defined(CONFIG_SOC_PIC32CK1012GC01048)
#include "pic32ck1012gc01048.h"
#elif defined(CONFIG_SOC_PIC32CK1012GC01064)
#include "pic32ck1012gc01064.h"
#elif defined(CONFIG_SOC_PIC32CK1012GC01100)
#include "pic32ck1012gc01100.h"
#elif defined(CONFIG_SOC_PIC32CK1025GC01064)
#include "pic32ck1025gc01064.h"
#elif defined(CONFIG_SOC_PIC32CK1025GC01100)
#include "pic32ck1025gc01100.h"
#elif defined(CONFIG_SOC_PIC32CK2051GC01064)
#include "pic32ck2051gc01064.h"
#elif defined(CONFIG_SOC_PIC32CK2051GC01100)
#include "pic32ck2051gc01100.h"
#elif defined(CONFIG_SOC_PIC32CK2051GC01144)
#include "pic32ck2051gc01144.h"
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_PIC32CK_GC01_H_ */
