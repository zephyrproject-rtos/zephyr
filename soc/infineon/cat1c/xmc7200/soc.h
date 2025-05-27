/*
 * Copyright (c) 2021 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon XMC7200 SOC.
 */

#ifndef _SOC__H_
#define _SOC__H_

#ifndef _ASMLANGUAGE
#include <cy_device_headers.h>
#include <cy_sysint.h>

/* Used to pull values from the device tree array */
#define SYS_INT_NUM 0
#define SYS_INT_PRI 1

#define ENABLE_SYS_INT(n, isr_handler)                                         \
	enable_sys_int(DT_INST_PROP_BY_IDX(n, system_interrupts, SYS_INT_NUM), \
		       DT_INST_PROP_BY_IDX(n, system_interrupts, SYS_INT_PRI), \
		       (void (*)(const void *))(void *)isr_handler,            \
		       DEVICE_DT_INST_GET(n));

void enable_sys_int(uint32_t int_num, uint32_t priority, void(*isr)(const void *),
		    const void *arg);

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
