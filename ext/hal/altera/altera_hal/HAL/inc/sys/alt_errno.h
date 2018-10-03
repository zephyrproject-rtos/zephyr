#ifndef __ALT_ERRNO_H__
#define __ALT_ERRNO_H__

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2004 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* Altera does not recommend, suggest or require that this reference design    *
* file be used in conjunction or combination with any other product.          *
******************************************************************************/

/******************************************************************************
*                                                                             *
* THIS IS A LIBRARY READ-ONLY SOURCE FILE. DO NOT EDIT.                       *
*                                                                             *
******************************************************************************/

/*
 * errno is defined in <errno.h> so that it uses the thread local version 
 * stored in the location pointed to by "_impure_ptr". This means that the 
 * accesses to errno within the HAL library can cause the entirety of 
 * of the structure pointed to by "_impure_ptr" to be added to the 
 * users application. This can be undesirable in very small footprint systems.
 *
 * To avoid this happening, the HAL uses the macro ALT_ERRNO, defined below,
 * to access errno, rather than accessing it directly. This macro will only 
 * use the thread local version if some other code has already caused it to be 
 * included into the system, otherwise it will use the global errno value.
 *
 * This causes a slight increases in code size where errno is accessed, but 
 * can lead to significant overall benefits in very small systems. The 
 * increase is inconsequential when compared to the size of the structure
 * pointed to by _impure_ptr.
 *
 * Note that this macro accesses __errno() using an externally declared 
 * function pointer (alt_errno). This is done so that the function call uses the
 * subroutine call instruction via a register rather than an immediate address.
 * This is important in the case that the code has been linked for a high
 * address, but __errno() is not being used. In this case the weak linkage
 * would have resulted in the instruction: "call 0" which would fail to link. 
 */

extern int* (*alt_errno) (void);

/* Must define this so that values such as EBADFD are defined in errno.h. */
#ifndef __LINUX_ERRNO_EXTENSIONS__
#define __LINUX_ERRNO_EXTENSIONS__
#endif

#include <errno.h>

#include "alt_types.h"

#undef errno

extern int errno;

static ALT_INLINE int* alt_get_errno(void)
{
  return ((alt_errno) ? alt_errno() : &errno);
}

#define ALT_ERRNO *alt_get_errno()

#endif /* __ALT_ERRNO_H__ */
