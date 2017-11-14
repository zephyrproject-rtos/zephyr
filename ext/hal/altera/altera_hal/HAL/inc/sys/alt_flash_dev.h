#ifndef __ALT_FLASH_DEV_H__
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2015 Altera Corporation, San Jose, California, USA.           *
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
* Alt_flash_dev.h - Generic Flash device interfaces                           *
*                                                                             *
* Author PRR                                                                  *
*                                                                             *
******************************************************************************/
#define __ALT_FLASH_DEV_H__

#include "alt_flash_types.h"
#include "sys/alt_llist.h"
#include "priv/alt_dev_llist.h"

#include "alt_types.h"

typedef struct alt_flash_dev alt_flash_dev; 
typedef alt_flash_dev alt_flash_fd;

static ALT_INLINE int alt_flash_device_register( alt_flash_fd* fd)
{
  extern alt_llist alt_flash_dev_list;

  return alt_dev_llist_insert ((alt_dev_llist*) fd, &alt_flash_dev_list);
}

typedef alt_flash_dev* (*alt_flash_open)(alt_flash_dev* flash, 
                                         const char* name );
typedef int (*alt_flash_close)(alt_flash_dev* flash_info);

typedef int (*alt_flash_write)( alt_flash_dev* flash, int offset, 
                                const void* src_addr, int length );

typedef int (*alt_flash_get_flash_info)( alt_flash_dev* flash, flash_region** info, 
                                          int* number_of_regions);
typedef int (*alt_flash_write_block)( alt_flash_dev* flash, int block_offset, 
                                      int data_offset, const void* data, 
                                      int length);
typedef int (*alt_flash_erase_block)( alt_flash_dev* flash, int offset);
typedef int (*alt_flash_read)(alt_flash_dev* flash, int offset, 
                              void* dest_addr, int length );
typedef int (*alt_flash_lock)(alt_flash_dev* flash, alt_u32 sectors_to_lock);						  

struct alt_flash_dev
{
  alt_llist                 llist;
  const char*               name;
  alt_flash_open            open;
  alt_flash_close           close;
  alt_flash_write           write;
  alt_flash_read            read;
  alt_flash_get_flash_info  get_info;
  alt_flash_erase_block     erase_block;
  alt_flash_write_block     write_block;
  void*                     base_addr;
  int                       length;
  int                       number_of_regions;
  flash_region              region_info[ALT_MAX_NUMBER_OF_FLASH_REGIONS]; 
  alt_flash_lock            lock;
};

#endif /* __ALT_FLASH_DEV_H__ */
