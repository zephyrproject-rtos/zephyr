#ifndef __ALT_FLASH_TYPES_H__
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

/******************************************************************************
*                                                                             *
* Alt_flash_types.h - Some generic types and defines used by the flash code   *
*                                                                             *
* Author PRR                                                                  *
*                                                                             *
******************************************************************************/
#define __ALT_FLASH_TYPES_H__

#ifndef ALT_MAX_NUMBER_OF_FLASH_REGIONS
#define ALT_MAX_NUMBER_OF_FLASH_REGIONS 8
#endif /* ALT_MAX_NUMBER_OF_FLASH_REGIONS */

/*
 * Description of a single Erase region
 */
typedef struct flash_region
{
  int   offset;
  int   region_size;
  int   number_of_blocks;
  int   block_size;
}flash_region;

#endif /* __ALT_FLASH_TYPES_H__ */
