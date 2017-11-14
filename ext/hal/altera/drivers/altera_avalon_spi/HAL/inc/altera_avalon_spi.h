#ifndef __ALT_AVALON_SPI_H__
#define __ALT_AVALON_SPI_H__

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
******************************************************************************/
#include <stddef.h>

#include "alt_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * Macros used by alt_sys_init
 */

#define ALTERA_AVALON_SPI_INSTANCE(name, device) extern int alt_no_storage
#define ALTERA_AVALON_SPI_INIT(name, device) while (0)

/*
 * Use this function to perform one SPI access on your target.  'base' should
 * be the base address of your SPI peripheral, while 'slave' indicates which
 * bit in the slave select register should be set.
 */

/* If you need to make multiple accesses to the same slave then you should 
 * set the merge bit in the flags for all of them except the first.
 */
#define ALT_AVALON_SPI_COMMAND_MERGE (0x01)

/*
 * If you need the slave select line to be toggled between words then you
 * should set the toggle bit in the flag.
 */
#define ALT_AVALON_SPI_COMMAND_TOGGLE_SS_N (0x02)


int alt_avalon_spi_command(alt_u32 base, alt_u32 slave,
                           alt_u32 write_length, const alt_u8 * write_data,
                           alt_u32 read_length, alt_u8 * read_data,
                           alt_u32 flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ALT_AVALON_SPI_H__ */
