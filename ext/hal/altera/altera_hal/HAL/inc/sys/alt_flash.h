#ifndef __ALT_FLASH_H__
#define __ALT_FLASH_H__
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
* Alt_flash.h - User interface for flash code                                 *
*                                                                             *
* Use this interface to avoid being exposed to the internals of the device    *
* driver architecture. If you chose to use the flash driver internal          *
* structures we don't guarantee not to change them                            *
*                                                                             *
* Author PRR                                                                  *
*                                                                             *
******************************************************************************/



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "alt_types.h"
#include "alt_flash_types.h"
#include "alt_flash_dev.h"
#include "sys/alt_cache.h"

alt_flash_fd* alt_flash_open_dev(const char* name);
void alt_flash_close_dev(alt_flash_fd* fd );

/*
 *  alt_flash_lock
 *
 *  Locks the range of the memory sectors, which 
 *  protected from write and erase.
 *
 */
static __inline__ int __attribute__ ((always_inline)) alt_lock_flash( 
                                      alt_flash_fd* fd, alt_u32 sectors_to_lock)
{
  return fd->lock( fd, sectors_to_lock);
}

/*
 *  alt_write_flash
 *
 *  Program a buffer into flash.
 *
 *  This routine erases all the affected erase blocks (if necessary)
 *  and then programs the data. However it does not read the data out first
 *  and preserve and none overwritten data, because this would require very 
 *  large buffers on the target. If you need
 *  that functionality use the functions below.
 */
static __inline__ int __attribute__ ((always_inline)) alt_write_flash(
                                                           alt_flash_fd* fd, 
                                                           int offset, 
                                                           const void* src_addr, 
                                                           int length )
{
  return fd->write( fd, offset, src_addr, length );
}

/*
 *  alt_read_flash
 *
 *  Read a block of flash for most flashes this is just memcpy
 *  it's here for completeness in case we need it for some serial flash device
 *
 */
static __inline__ int __attribute__ ((always_inline)) alt_read_flash( 
                                      alt_flash_fd* fd, int offset, 
                                      void* dest_addr, int length )
{
  return fd->read( fd, offset, dest_addr, length );
}

/*
 *  alt_get_flash_info
 *
 *  Return the information on the flash sectors.
 * 
 */
static __inline__ int __attribute__ ((always_inline)) alt_get_flash_info( 
                                      alt_flash_fd* fd, flash_region** info, 
                                      int* number_of_regions)
{
  return fd->get_info( fd, info, number_of_regions);
}

/*
 *  alt_erase_flash_block
 *
 *  Erase a particular erase block, pass in the offset to the start of 
 *  the block and it's size
 */
static __inline__ int __attribute__ ((always_inline)) alt_erase_flash_block( 
                                      alt_flash_fd* fd, int offset, int length) 
{
  int ret_code;
  ret_code = fd->erase_block( fd, offset );
  
/* remove dcache_flush call for FB330552  
  if(!ret_code)
      alt_dcache_flush((alt_u8*)fd->base_addr + offset, length);
*/
  return ret_code;
}

/*
 *  alt_write_flash_block
 *
 *  Write a particular flash block, block_offset is the offset 
 *  (from the base of flash) to start of the block
 *  data_offset is the offset (from the base of flash)
 *  where you wish to start programming
 *  
 *  NB this function DOES NOT check that you are only writing a single
 *  block of data as that would slow down this function. 
 *
 *  Use alt_write_flash if you want that level of error checking.
 */

static __inline__ int __attribute__ ((always_inline)) alt_write_flash_block( 
                                      alt_flash_fd* fd, int block_offset,
                                      int data_offset,
                                      const void *data, int length)
{

  int ret_code;
  ret_code = fd->write_block( fd, block_offset, data_offset, data, length );

/* remove dcache_flush call for FB330552  
  if(!ret_code)
      alt_dcache_flush((alt_u8*)fd->base_addr + data_offset, length);
*/
  return ret_code;
}

#ifdef __cplusplus
}
#endif

#endif /* __ALT_FLASH_H__ */
