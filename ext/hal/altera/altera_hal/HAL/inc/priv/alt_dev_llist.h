#ifndef __ALT_DEV_LLIST_H__
#define __ALT_DEV_LLIST_H__

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

#include "sys/alt_llist.h"
#include "alt_types.h"

/*
 * This header provides the internal defenitions required to control file 
 * access. These variables and functions are not guaranteed to exist in 
 * future implementations of the HAL.
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * The alt_dev_llist is an internal structure used to form a common base 
 * class for all device types. The use of this structure allows common code
 * to be used to manipulate the various device lists.
 */

typedef struct {
  alt_llist llist;
  const char* name;
} alt_dev_llist;

/*
 *
 */

extern int alt_dev_llist_insert (alt_dev_llist* dev, alt_llist* list);

#ifdef __cplusplus
}
#endif

#endif /* __ALT_DEV_LLIST_H__ */
