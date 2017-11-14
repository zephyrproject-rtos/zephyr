#ifndef __ALT_NO_ERROR_H__
#define __ALT_NO_ERROR_H__

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

#include "alt_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * alt_no_error() is a dummy function used by alt_sem.h and alt_flag.h. It
 * substitutes for functions that have a return code by creating a function 
 * that always returns zero. 
 *
 * This may seem a little obscure, but what happens is that the compiler can 
 * then optomise away the call to this function, and any code written which
 * handles the error path (i.e. non zero return values). 
 *
 * This allows code to be written which correctly use the uC/OS-II semaphore
 * and flag utilities, without the use of those utilities impacting on 
 * excutables built for a single threaded HAL environment.
 *
 * This function is considered to be part of the internal implementation of
 * the HAL, and should not be called directly by application code or device
 * drivers. It is not guaranteed to be preserved in future versions of the
 * HAL.
 */

static ALT_INLINE int ALT_ALWAYS_INLINE alt_no_error (void)
{
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __ALT_NO_ERROR_H__ */
