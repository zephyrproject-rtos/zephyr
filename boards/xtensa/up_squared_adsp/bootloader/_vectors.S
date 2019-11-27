/*
 * Copyright (c) 2004, 2006-2007 Tensilica Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <xtensa/coreasm.h>
#include <xtensa/config/system.h>	/* for XSHAL_VECTORS_PACKED */

.global	_ResetVector

#if XCHAL_HAVE_EXCEPTIONS

# if XCHAL_HAVE_DEBUG
.global	_DebugExceptionVector
# endif

.global	_KernelExceptionVector
.global	_UserExceptionVector

# ifdef XCHAL_DOUBLEEXC_VECTOR_VADDR
.global	_DoubleExceptionVector
# endif

# if XCHAL_HAVE_NMI
.global	_NMIExceptionVector
# endif

# if XCHAL_HAVE_WINDOWED
.global _WindowOverflow4
.global _WindowUnderflow4
.global _WindowOverflow8
.global _WindowUnderflow8
.global _WindowOverflow12
.global _WindowUnderflow12
# endif

# if XCHAL_HAVE_MEM_ECC_PARITY
.global _MemErrorVector
# endif

#endif /* XCHAL_HAVE_EXCEPTIONS */


#if (XCHAL_NUM_INTLEVELS >= 2) && (2 != XCHAL_DEBUGLEVEL)
.global _Level2Vector
#endif
#if (XCHAL_NUM_INTLEVELS >= 3) && (3 != XCHAL_DEBUGLEVEL)
.global _Level3Vector
#endif
#if (XCHAL_NUM_INTLEVELS >= 4) && (4 != XCHAL_DEBUGLEVEL)
.global _Level4Vector
#endif
#if (XCHAL_NUM_INTLEVELS >= 5) && (5 != XCHAL_DEBUGLEVEL)
.global _Level5Vector
#endif
#if (XCHAL_NUM_INTLEVELS >= 6) && (6 != XCHAL_DEBUGLEVEL)
.global _Level6Vector
#endif

#if XCHAL_HAVE_VECBASE && XSHAL_VECTORS_PACKED
.global	_RelocVectors
#endif

/*  These don't take up space:  */
.global xthals_hw_configid0
.global xthals_hw_configid1
.global xthals_release_major
.global xthals_release_minor

/*  The following allows this object file to be pulled in by reference:  */
.text
.global	_xtos_vectors_ref_
.set	_xtos_vectors_ref_, 0
