#ifndef __ALT_AVALON_SYSID_H__
#define __ALT_AVALON_SYSID_H__

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2003 Altera Corporation, San Jose, California, USA.           *
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

#include "alt_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * ALTERA_AVALON_SYSID_INSTANCE is the macro used by alt_sys_init() to
 * allocate any per device memory that may be required. In this case no
 * allocation is necessary.
 */

#define ALTERA_AVALON_SYSID_INSTANCE(name, dev) extern int alt_no_storage
#define ALTERA_AVALON_SYSID_INIT(name, dev) while (0)

#ifdef SYSID_BASE
alt_32 alt_avalon_sysid_test(void);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ALT_AVALON_SYSID_H__ */
