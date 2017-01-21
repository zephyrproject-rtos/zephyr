/* Macros for tagging symbols and putting them in the correct sections. */

/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _section_tags__h_
#define _section_tags__h_

#if !defined(_ASMLANGUAGE)

#define __in_section(seg, hash, line)					\
	__attribute__((section("." _STRINGIFY(seg)		\
				"." _STRINGIFY(hash)				\
				"." _STRINGIFY(line))))

#define __noinit     __in_section(NOINIT,    _FILE_PATH_HASH, __COUNTER__)

#if defined(CONFIG_ARM)
#define __scs_section  __in_section(SCS_SECTION, _FILE_PATH_HASH, __COUNTER__)
#define __scp_section  __in_section(SCP_SECTION, _FILE_PATH_HASH, __COUNTER__)

#define __irq_vector_table  __in_section(IRQ_VECTOR_TABLE, _FILE_PATH_HASH, \
				      __COUNTER__)

#define __kinetis_flash_config_section   __in_section(KINETIS_FLASH_CONFIG, \
						 _FILE_PATH_HASH, __COUNTER__)
#elif defined(CONFIG_ARC)
	#define __irq_vector_table \
		__in_section(IRQ_VECTOR_TABLE, _FILE_PATH_HASH, __COUNTER__)

#endif /* CONFIG_ARC */

#endif /* !_ASMLANGUAGE */

#endif /* _section_tags__h_ */
