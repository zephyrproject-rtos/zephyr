/* Macros for tagging symbols and putting them in the correct sections. */

/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _section_tags__h_
#define _section_tags__h_

#include <toolchain.h>

#if !defined(_ASMLANGUAGE)

#define __noinit		__in_section_unique(NOINIT)
#define __irq_vector_table	_GENERIC_SECTION(IRQ_VECTOR_TABLE)
#define __sw_isr_table		_GENERIC_SECTION(SW_ISR_TABLE)

#if defined(CONFIG_ARM)
#define __kinetis_flash_config_section __in_section_unique(KINETIS_FLASH_CONFIG)
#define __ti_ccfg_section _GENERIC_SECTION(TI_CCFG)
#endif /* CONFIG_ARM */

#endif /* !_ASMLANGUAGE */

#endif /* _section_tags__h_ */
