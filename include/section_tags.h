/* Macros for tagging symbols and putting them in the correct sections. */

/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#define __security_frdm_k64f_section   __in_section(SECURITY_FRDM_K64F,	\
						 _FILE_PATH_HASH, __COUNTER__)

#if defined(CONFIG_GDB_INFO) && !defined(CONFIG_SW_ISR_TABLE)
#define __gdb_stub_irq_vector_table  __in_section(GDB_STUB_IRQ_VECTOR_TABLE, \
					       _FILE_PATH_HASH, __COUNTER__)
#endif  /* CONFIG_GDB_INFO && !CONFIG_SW_ISR_TABLE */

#elif defined(CONFIG_ARC)

	#define __irq_vector_table \
		__in_section(IRQ_VECTOR_TABLE, _FILE_PATH_HASH, __COUNTER__)

#endif /* CONFIG_ARC */

#endif /* !_ASMLANGUAGE */

#endif /* _section_tags__h_ */
