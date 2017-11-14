/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2006 Altera Corporation, San Jose, California, USA.           *
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
#ifndef __ALTERA_AVALON_SGDMA_DESCRIPTOR_H__
#define __ALTERA_AVALON_SGDMA_DESCRIPTOR_H__

#include "alt_types.h"

/* Each Scatter-gather DMA buffer descriptor spans 0x20 of memory */
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_SIZE (0x20)


/*
 * Descriptor control bit masks & offsets
 *
 * Note: The control byte physically occupies bits [31:24] in memory.
 *       The following bit-offsets are expressed relative to the LSB of
 *       the control register bitfield.
 */
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_GENERATE_EOP_MSK (0x1)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_GENERATE_EOP_OFST (0)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_READ_FIXED_ADDRESS_MSK (0x2)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_READ_FIXED_ADDRESS_OFST (1)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_WRITE_FIXED_ADDRESS_MSK (0x4)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_WRITE_FIXED_ADDRESS_OFST (2)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_ATLANTIC_CHANNEL_MSK (0x8)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_ATLANTIC_CHANNEL_OFST (3)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_OWNED_BY_HW_MSK (0x80)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_OWNED_BY_HW_OFST (7)

/*
 * Descriptor status bit masks & offsets
 *
 * Note: The status byte physically occupies bits [23:16] in memory.
 *       The following bit-offsets are expressed relative to the LSB of
 *       the status register bitfield.
 */
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_CRC_MSK (0x1)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_CRC_OFST (0)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_PARITY_MSK (0x2)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_PARITY_OFST (1)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_OVERFLOW_MSK (0x4)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_OVERFLOW_OFST (2)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_SYNC_MSK (0x8)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_SYNC_OFST (3)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_UEOP_MSK (0x10)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_UEOP_OFST (4)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_MEOP_MSK (0x20)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_MEOP_OFST (5)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_MSOP_MSK (0x40)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_MSOP_OFST (6)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_TERMINATED_BY_EOP_MSK (0x80)
#define ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_TERMINATED_BY_EOP_OFST (7)

/*
 * To ensure that a descriptor is created without spaces
 * between the struct members, we call upon GCC's ability
 * to pack to a byte-aligned boundary.
 */
#define alt_avalon_sgdma_packed __attribute__ ((packed,aligned(1)))

/*
 * Buffer Descriptor data structure
 *
 * The SGDMA controller buffer descriptor allocates
 * 64 bits for each address. To support ANSI C, the
 * struct implementing a descriptor places 32-bits
 * of padding directly above each address; each pad must
 * be cleared when initializing a descriptor.
 */
typedef struct {
    alt_u32   *read_addr;
    alt_u32   read_addr_pad;

    alt_u32   *write_addr;
    alt_u32   write_addr_pad;

    alt_u32   *next;
    alt_u32   next_pad;
    
    alt_u16   bytes_to_transfer;
    alt_u8    read_burst;
    alt_u8    write_burst;

    alt_u16   actual_bytes_transferred;
    alt_u8    status;
    alt_u8    control;

} alt_avalon_sgdma_packed alt_sgdma_descriptor;

#endif /* __ALTERA_AVALON_SGDMA_DESCRIPTOR_H__ */
