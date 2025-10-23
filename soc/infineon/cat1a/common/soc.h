/*
 * Copyright (c) 2021 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon PSOC 6 SOC.
 */

#ifndef _SOC__H_
#define _SOC__H_

#ifndef _ASMLANGUAGE
#include <cy_device_headers.h>
#include <cy_sysint.h>
#if defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_ISR_TABLES)
void psoc6_irq_connect_dynamic(unsigned int irq, unsigned int priority,
		    void (*routine)(const void *parameter),
		    const void *parameter, uint32_t flags);
#endif /* defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_ISR_TABLES) */
#endif  /* !_ASMLANGUAGE */

#endif  /* _SOC__H_ */
