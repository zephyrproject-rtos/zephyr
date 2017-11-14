#ifndef __ALT_FLAG_H__
#define __ALT_FLAG_H__

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
 * This header provides macro definitions that can be used to create and use
 * uc/OS-II style event flags. These macros can be used in both a uC/OS-II based 
 * environment, and a single threaded HAL based environment.
 *
 * The motivation for these macros is to allow code to be developed which is
 * thread safe under uC/OS-II, but incurs no additional overhead when used in a
 * single threaded HAL environment.  
 *
 * In the case of a single threaded HAL environment, they compile to 
 * "do nothing" directives, which ensures they do not contribute to the final
 * executable.
 *
 * The following macros are available:
 *
 * ALT_FLAG_GRP        - Create a flag group instance.
 * ALT_EXTERN_FLAG_GRP - Create a reference to an external flag group instance.
 * ALT_STATIC_FLAG_GRP - Create a static flag group instance.
 * ALT_FLAG_CREATE     - Initialise a flag group.
 * ALT_FLAG_PEND       - Pend on a flag group.
 * ALT_FLAG_POST       - Set a flag condition.
 
 *
 * Input arguments and return codes are all consistant with the equivalent
 * uC/OS-II function.
 *
 * It's important to be careful in the use of the macros: ALT_FLAG_GRP, 
 * ALT_EXTERN_FLAG_GRP, and ALT_STATIC_FLAG_GRP. In these three cases the 
 * semi-colon is included in the macro definition; so, for example, you should 
 * use:
 *
 * ALT_FLAG_GRP(mygroup)
 *
 * not:
 *
 * ALT_FLAG_GRP(mygroup);
 *
 * The inclusion of the semi-colon has been necessary to ensure the macros can
 * compile with no warnings when used in a single threaded HAL environment.
 *
 */ 

#include "priv/alt_no_error.h"

#define ALT_FLAG_GRP(group)
#define ALT_EXTERN_FLAG_GRP(group) 
#define ALT_STATIC_FLAG_GRP(group)
       
#define ALT_FLAG_CREATE(group, flags) alt_no_error ()   
#define ALT_FLAG_PEND(group, flags, wait_type, timeout) alt_no_error ()
#define ALT_FLAG_POST(group, flags, opt) alt_no_error ()

#ifndef ALT_SINGLE_THREADED
#define ALT_SINGLE_THREADED
#endif

#endif /* __ALT_FLAG_H__ */
