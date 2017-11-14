/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2011 Altera Corporation, San Jose, California, USA.           *
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

#ifndef __ALTERA_AVALON_SGDMA_H__
#define __ALTERA_AVALON_SGDMA_H__

#include <stddef.h>
#include <errno.h>

#include "sys/alt_dev.h"
#include "alt_types.h"
#include "altera_avalon_sgdma_descriptor.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * The function alt_find_dev() is used to search the device list "list" to
 * locate a device named "name". If a match is found, then a pointer to the
 * device is returned, otherwise NULL is returned.
 */
extern alt_dev* alt_find_dev (const char* name, alt_llist* list);

/* Callback routine type definition */
typedef void (*alt_avalon_sgdma_callback)(void *context);

/* SGDMA Device Structure */
typedef struct alt_sgdma_dev
{
  alt_llist                 llist;               // Device linked-list entry
  const char                *name;               // Name of SGDMA in SOPC System
  void                      *base;               // Base address of SGDMA
  alt_u32                   *descriptor_base;    // reserved
  alt_u32                   next_index;          // reserved
  alt_u32                   num_descriptors;     // reserved
  alt_sgdma_descriptor      *current_descriptor; // reserved
  alt_sgdma_descriptor      *next_descriptor;    // reserved
  alt_avalon_sgdma_callback callback;            // Callback routine pointer
  void                      *callback_context;   // Callback context pointer
  alt_u32                   chain_control;       // Value OR'd into control reg
} alt_sgdma_dev;


/*******************************************************************************
 *  Public API
 ******************************************************************************/

/* API for "application managed" operation */
int alt_avalon_sgdma_do_async_transfer(
  alt_sgdma_dev *dev,
  alt_sgdma_descriptor *desc);

alt_u8 alt_avalon_sgdma_do_sync_transfer(
  alt_sgdma_dev *dev,
  alt_sgdma_descriptor *desc);

void alt_avalon_sgdma_construct_mem_to_mem_desc(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *read_addr,
  alt_u32              *write_addr,
  alt_u16               length,
  int                   read_fixed,
  int                   write_fixed);

void alt_avalon_sgdma_construct_mem_to_mem_desc_burst(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *read_addr,
  alt_u32              *write_addr,
  alt_u16               length,
  int                   read_fixed,
  int                   write_fixed,
  int                   read_burst,
  int                   write_burst);

void alt_avalon_sgdma_construct_stream_to_mem_desc(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *write_addr,
  alt_u16               length_or_eop,
  int                   write_fixed);

void alt_avalon_sgdma_construct_stream_to_mem_desc_burst(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *write_addr,
  alt_u16               length_or_eop,
  int                   write_fixed,
  int                   write_burst);

  void alt_avalon_sgdma_construct_mem_to_stream_desc(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *read_addr,
  alt_u16               length,
  int                   read_fixed,
  int                   generate_sop,
  int                   generate_eop,
  alt_u8                atlantic_channel);

  void alt_avalon_sgdma_construct_mem_to_stream_desc_burst(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *read_addr,
  alt_u16               length,
  int                   read_fixed,
  int                   generate_sop,
  int                   generate_eop,
  int                   read_burst,
  alt_u8                atlantic_channel);

void alt_avalon_sgdma_register_callback(
  alt_sgdma_dev *dev,
  alt_avalon_sgdma_callback callback,
  alt_u32 chain_control,
  void *context);

void alt_avalon_sgdma_start(alt_sgdma_dev *dev);

void alt_avalon_sgdma_stop(alt_sgdma_dev *dev);

int alt_avalon_sgdma_check_descriptor_status(alt_sgdma_descriptor *desc);

alt_sgdma_dev* alt_avalon_sgdma_open (const char* name);

void alt_avalon_sgdma_enable_desc_poll(alt_sgdma_dev *dev, alt_u32 frequency);

void alt_avalon_sgdma_disable_desc_poll(alt_sgdma_dev *dev);

/* Private API */
void alt_avalon_sgdma_construct_descriptor(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *read_addr,
  alt_u32              *write_addr,
  alt_u16               length_or_eop,
  int                   generate_eop,
  int                   read_fixed,
  int                   write_fixed_or_sop,
  alt_u8                atlantic_channel);

/* Private API */
void alt_avalon_sgdma_construct_descriptor_burst(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *read_addr,
  alt_u32              *write_addr,
  alt_u16               length_or_eop,
  int                   generate_eop,
  int                   read_fixed,
  int                   write_fixed_or_sop,
  int                   read_burst,
  int                   write_burst,
  alt_u8                atlantic_channel);

void alt_avalon_sgdma_init (alt_sgdma_dev *dev, alt_u32 ic_id, alt_u32 irq);

/* HAL initialization macros */

/*
 * ALTERA_AVALON_SGDMA_INSTANCE is the macro used by alt_sys_init() to
 * allocate any per device memory that may be required.
 */
#define ALTERA_AVALON_SGDMA_INSTANCE(name, dev)                         \
static alt_sgdma_dev dev =                                              \
{                                                                       \
  ALT_LLIST_ENTRY,                                                      \
  name##_NAME,                                                          \
  ((void *)(name##_BASE)),                                              \
  ((alt_u32 *) 0x0),                                                    \
  ((alt_u32) 0x0),                                                      \
  ((alt_u32) 0x0),                                                      \
  ((alt_sgdma_descriptor *) 0x0),                                       \
  ((alt_sgdma_descriptor *) 0x0),                                       \
  ((void *) 0x0),                                                       \
  ((void *) 0x0),                                                       \
  ((alt_u16) 0x0)                                                       \
};

/*
 * The macro ALTERA_AVALON_SGDMA_INIT is called by the auto-generated function
 * alt_sys_init() to initialize a given device instance.
 */
#define ALTERA_AVALON_SGDMA_INIT(name, dev)                                 \
  if (name##_IRQ == ALT_IRQ_NOT_CONNECTED)                                  \
  {                                                                         \
    ALT_LINK_ERROR ("Error: Interrupt not connected for " #dev ". "         \
                    "The Altera Avalon Scatter-Gather DMA driver requires " \
                    "that an interrupt is connected. Please select an IRQ " \
                    "for this device in SOPC builder.");                    \
  }                                                                         \
  else                                                                      \
  {                                                                         \
    alt_avalon_sgdma_init(&dev, name##_IRQ_INTERRUPT_CONTROLLER_ID,         \
      name##_IRQ);                                                          \
  }

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ALTERA_AVALON_SGDMA_H__ */
