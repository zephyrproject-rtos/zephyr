/* Macros for tagging symbols and putting them in the correct sections. */

/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _section_tags__h_
#define _section_tags__h_

#if !defined(_ASMLANGUAGE)
#define __section_stringify(x)  #x

#define __section(seg, hash, line)					\
	__attribute__((section("." __section_stringify(seg)		\
				"." __section_stringify(hash)		\
				"." __section_stringify(line))))

#define __noinit     __section(NOINIT,    _FILE_PATH_HASH, __COUNTER__)

#if defined(VXMICRO_ARCH_arm)
#define __scs_section  __section(SCS_SECTION, _FILE_PATH_HASH, __COUNTER__)
#define __scp_section  __section(SCP_SECTION, _FILE_PATH_HASH, __COUNTER__)

#define __isr_table_section __section(ISR_TABLE_SECTION, _FILE_PATH_HASH, \
				      __COUNTER__)

#define __irq_vector_table  __section(IRQ_VECTOR_TABLE, _FILE_PATH_HASH, \
				      __COUNTER__)

#define __security_frdm_k64f_section   __section(SECURITY_FRDM_K64F,	\
						 _FILE_PATH_HASH, __COUNTER__)

#if defined(CONFIG_GDB_INFO) && !defined(CONFIG_SW_ISR_TABLE)
#define __gdb_stub_irq_vector_table  __section(GDB_STUB_IRQ_VECTOR_TABLE, \
					       _FILE_PATH_HASH, __COUNTER__)
#endif  /* CONFIG_GDB_INFO && !CONFIG_SW_ISR_TABLE */

#elif defined(VXMICRO_ARCH_arc)

	#define __irq_vector_table \
		__section(IRQ_VECTOR_TABLE, _FILE_PATH_HASH, __COUNTER__)

	#define __isr_table_section \
		__section(ISR_TABLE_SECTION, _FILE_PATH_HASH, __COUNTER__)

#endif /* VXMICRO_ARCH_arc */

#endif /* !_ASMLANGUAGE */

#endif /* _section_tags__h_ */
