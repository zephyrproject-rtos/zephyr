/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CK_GC00_H_
#define SOC_MICROCHIP_PIC32CK_GC00_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CK0512GC00064)
#include "pic32ck0512gc00064.h"
#elif defined(CONFIG_SOC_PIC32CK0512GC00100)
#include "pic32ck0512gc00100.h"
#elif defined(CONFIG_SOC_PIC32CK1012GC00048)
#include "pic32ck1012gc00048.h"
#elif defined(CONFIG_SOC_PIC32CK1012GC00064)
#include "pic32ck1012gc00064.h"
#elif defined(CONFIG_SOC_PIC32CK1012GC00100)
#include "pic32ck1012gc00100.h"
##elif defined(CONFIG_SOC_PIC32CK1025GC00064)
#include "pic32ck1025gc00064.h"
#elif defined(CONFIG_SOC_PIC32CK1025GC00100)
#include "pic32ck1025gc00100.h"
#elif defined(CONFIG_SOC_PIC32CK2051GC00064)
#include "pic32ck2051gc00064.h"
#elif defined(CONFIG_SOC_PIC32CK2051GC00100)
#include "pic32ck2051gc00100.h"
#elif defined(CONFIG_SOC_PIC32CK2051GC00144)
#include "pic32ck2051gc00144.h"
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_PIC32CK_GC00_H_ */
