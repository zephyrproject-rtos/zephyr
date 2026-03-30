/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CK_SG01_H_
#define SOC_MICROCHIP_PIC32CK_SG01_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CK0512SG01064)
#include "pic32ck0512sg01064.h"
#elif defined(CONFIG_SOC_PIC32CK0512SG01100)
#include "pic32ck0512sg01100.h"
#elif defined(CONFIG_SOC_PIC32CK1012SG01048)
#include "pic32ck1012sg01048.h"
#elif defined(CONFIG_SOC_PIC32CK1012SG01064)
#include "pic32ck1012sg01064.h"
#elif defined(CONFIG_SOC_PIC32CK1012SG01100)
#include "pic32ck1012sg01100.h"
#elif defined(CONFIG_SOC_PIC32CK1025SG01064)
#include "pic32ck1025sg01064.h"
#elif defined(CONFIG_SOC_PIC32CK1025SG01100)
#include "pic32ck1025sg01100.h"
#elif defined(CONFIG_SOC_PIC32CK1025SG01144)
#include "pic32ck1025sg01144.h"
#elif defined(CONFIG_SOC_PIC32CK2051SG01064)
#include "pic32ck2051sg01064.h"
#elif defined(CONFIG_SOC_PIC32CK2051SG01100)
#include "pic32ck2051sg01100.h"
#elif defined(CONFIG_SOC_PIC32CK2051SG01144)
#include "pic32ck2051sg01144.h"
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_PIC32CK_SG01_H_ */
