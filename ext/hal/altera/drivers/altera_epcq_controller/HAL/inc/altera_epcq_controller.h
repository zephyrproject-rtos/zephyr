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
******************************************************************************/

#ifndef __ALT_EPCQ_CONTROLLER_H__
#define __ALT_EPCQ_CONTROLLER_H__

#include "alt_types.h"
#include "sys/alt_flash_dev.h"
#include "sys/alt_llist.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 *  Description of the EPCQ controller
 */
typedef struct alt_epcq_controller_dev
{
    alt_flash_dev dev;

    alt_u32 data_base; /** base address of data slave */
    alt_u32 data_end; /** end address of data slave (not inclusive) */
    alt_u32 csr_base; /** base address of CSR slave */
    alt_u32 size_in_bytes; /** size of memory in bytes */
    alt_u32 is_epcs; /** 1 if device is an EPCS device */
    alt_u32 number_of_sectors; /** number of flash sectors */
    alt_u32 sector_size; /** size of each flash sector */
    alt_u32 page_size; /** page size */
    alt_u32 silicon_id; /** ID of silicon used with EPCQ IP */
} alt_epcq_controller_dev;

/**
*   Macros used by alt_sys_init.c to create data storage for driver instance
*/
#define ALTERA_EPCQ_CONTROLLER_AVL_MEM_AVL_CSR_INSTANCE(epcq_name, avl_mem, avl_csr, epcq_dev) \
static alt_epcq_controller_dev epcq_dev =                                                       \
{                                                                                               \
  .dev = {                                                                                      \
            .llist = ALT_LLIST_ENTRY,                                                           \
            .name = avl_mem##_NAME,                                                             \
            .write = alt_epcq_controller_write,                                                 \
            .read = alt_epcq_controller_read,                                                   \
            .get_info = alt_epcq_controller_get_info,                                           \
            .erase_block = alt_epcq_controller_erase_block,                                     \
            .write_block = alt_epcq_controller_write_block,                                     \
            .base_addr = ((void*)(avl_mem##_BASE)),                                             \
            .length = ((int)(avl_mem##_SPAN)),                                                  \
            .lock = alt_epcq_controller_lock ,                                                  \
         },                                                                                     \
  .data_base = ((alt_u32)(avl_mem##_BASE)),   							\
  .data_end = ((alt_u32)(avl_mem##_BASE) + (alt_u32)(avl_mem##_SPAN)),	                        \
  .csr_base = ((alt_u32)(avl_csr##_BASE)),   							\
  .size_in_bytes = ((alt_u32)(avl_mem##_SPAN)),         				        \
  .is_epcs = ((alt_u32)(avl_mem##_IS_EPCS)),							\
  .number_of_sectors = ((alt_u32)(avl_mem##_NUMBER_OF_SECTORS)),			        \
  .sector_size = ((alt_u32)(avl_mem##_SECTOR_SIZE)),						\
  .page_size = ((alt_u32)(avl_mem##_PAGE_SIZE))	,						\
}

/*
    Public API

    Refer to Using Flash Devices in the
    Developing Programs Using the Hardware Abstraction Layer chapter
    of the Nios II Software Developer's Handbook.

*/
int alt_epcq_controller_read(alt_flash_dev *flash_info, int offset, void *dest_addr, int length);

int alt_epcq_controller_get_info(alt_flash_fd *fd, flash_region **info, int *number_of_regions);

int alt_epcq_controller_erase_block(alt_flash_dev *flash_info, int block_offset);

int alt_epcq_controller_write_block(alt_flash_dev *flash_info, int block_offset, int data_offset, const void *data, int length);

int alt_epcq_controller_write(alt_flash_dev *flash_info, int offset, const void *src_addr, int length);

int alt_epcq_controller_lock(alt_flash_dev *flash_info, alt_u32 sectors_to_lock);


/*
 * Initialization function
 */
extern alt_32 altera_epcq_controller_init(alt_epcq_controller_dev *dev);

/*
 * alt_sys_init.c will call this macro automatically initialize the driver instance
 */
#define ALTERA_EPCQ_CONTROLLER_INIT(name, dev) 	\
		altera_epcq_controller_init(&dev);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ALT_EPCQ_CONTROLLER_H__ */
