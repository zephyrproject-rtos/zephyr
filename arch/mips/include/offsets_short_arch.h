/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_MIPS_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_MIPS_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

#define _thread_offset_to_sp \
	(___thread_t_callee_saved_OFFSET + 0)

#define _thread_offset_to_swap_return_value \
	(___thread_t_arch_OFFSET + 0)


#define CTX_REG(REGNO)	((SZREG)*((REGNO)-1))

#define CTX_EPC		((SZREG)*31)
#define CTX_BADVADDR	((SZREG)*32)
#define CTX_HI0		((SZREG)*33)
#define CTX_LO0		((SZREG)*34)
#define CTX_LINK	((SZREG)*35)
#define CTX_STATUS	(((SZREG)*35)+SZPTR)
#define CTX_CAUSE	(((SZREG)*35)+SZPTR+4)
#define CTX_BADINSTR	(((SZREG)*35)+SZPTR+8)
#define CTX_BADPINSTR	(((SZREG)*35)+SZPTR+12)
#define CTX_SIZE	(((SZREG)*35)+SZPTR+16)



#endif /* ZEPHYR_ARCH_MIPS_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
