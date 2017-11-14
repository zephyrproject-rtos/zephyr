/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2005 Altera Corporation, San Jose, California, USA.           *
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

/*
 * This macro is used to load code/data from its load address to its 
 * execution address for a given section. The section name is the input
 * argument. Note that a leading '.' is assumed in the name. For example
 * to load the section .onchip_ram, use:
 * 
 * ALT_LOAD_SECTION_BY_NAME(onchip_ram);
 * 
 * This requires that the apropriate linker symbols have been generated
 * for the section in question. This will be the case if you are using the
 * default linker script.
 */ 

#define ALT_LOAD_SECTION_BY_NAME(name)                         \
  {                                                            \
    extern void _alt_partition_##name##_start;                 \
    extern void _alt_partition_##name##_end;                   \
    extern void _alt_partition_##name##_load_addr;             \
                                                               \
    alt_load_section(&_alt_partition_##name##_load_addr,       \
                     &_alt_partition_##name##_start,           \
                     &_alt_partition_##name##_end);            \
  }

/*
 * Function used to load an individual section from flash to RAM.
 *
 * There is an implicit assumption here that the linker script will ensure
 * that all sections are word aligned.
 *
 */

static void ALT_INLINE alt_load_section (alt_u32* from, 
                                         alt_u32* to, 
                                         alt_u32* end)
{
  if (to != from)
  {
    while( to != end )
    {
      *to++ = *from++;
    }
  }
}
