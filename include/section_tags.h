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
#define __scp_section		__in_section_unique(SCP_SECTION)
#define __security_frdm_k64f_section   __in_section_unique(SECURITY_FRDM_K64F)
#define __kinetis_flash_config_section __in_section_unique(KINETIS_FLASH_CONFIG)
#endif /* CONFIG_ARM */

#endif /* !_ASMLANGUAGE */

#endif /* _section_tags__h_ */
