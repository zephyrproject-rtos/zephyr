/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2014 Altera Corporation, San Jose, California, USA.           *
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
******************************************************************************/

#ifndef ALTERA_MSGDMA_DESCRIPTOR_REGS_H_
#define ALTERA_MSGDMA_DESCRIPTOR_REGS_H_

#include "io.h"

/*
  Descriptor formats:

  Standard Format:
  
  Offset         |    3                 2                 1                   0
  ------------------------------------------------------------------------------
   0x0           |                      Read Address[31..0]
   0x4           |                      Write Address[31..0]
   0x8           |                      Length[31..0]
   0xC           |                      Control[31..0]

  Extended Format:
  
Offset|   3                  2                  1                  0
 ------------------------------------------------------------------------------
 0x0  |                      Read Address[31..0]
 0x4  |                      Write Address[31..0]
 0x8  |                      Length[31..0]
 0xC  |Write Burst Count[7..0] | Read Burst Count[7..0] | Sequence Number[15..0]
 0x10 | Write Stride[15..0]           |            Read Stride[15..0]
 0x14 |                      Read Address[63..32]
 0x18 |                      Write Address[63..32]
 0x1C |                      Control[31..0]

  Note:  The control register moves from offset 0xC to 0x1C depending on the 
         format used

*/




#define ALTERA_MSGDMA_DESCRIPTOR_READ_ADDRESS_REG                      0x0
#define ALTERA_MSGDMA_DESCRIPTOR_WRITE_ADDRESS_REG                     0x4
#define ALTERA_MSGDMA_DESCRIPTOR_LENGTH_REG                            0x8
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_STANDARD_REG                  0xC
#define ALTERA_MSGDMA_DESCRIPTOR_SEQUENCE_NUMBER_REG                   0xC
#define ALTERA_MSGDMA_DESCRIPTOR_READ_BURST_REG                        0xE
#define ALTERA_MSGDMA_DESCRIPTOR_WRITE_BURST_REG                       0xF
#define ALTERA_MSGDMA_DESCRIPTOR_READ_STRIDE_REG                       0x10
#define ALTERA_MSGDMA_DESCRIPTOR_WRITE_STRIDE_REG                      0x12
#define ALTERA_MSGDMA_DESCRIPTOR_READ_ADDRESS_HIGH_REG                 0x14
#define ALTERA_MSGDMA_DESCRIPTOR_WRITE_ADDRESS_HIGH_REG                0x18
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_ENHANCED_REG                  0x1C


/* masks and offsets for the sequence number and programmable burst counts */
#define ALTERA_MSGDMA_DESCRIPTOR_SEQUENCE_NUMBER_MASK                  0xFFFF
#define ALTERA_MSGDMA_DESCRIPTOR_SEQUENCE_NUMBER_OFFSET                0
#define ALTERA_MSGDMA_DESCRIPTOR_READ_BURST_COUNT_MASK                 0x00FF0000
#define ALTERA_MSGDMA_DESCRIPTOR_READ_BURST_COUNT_OFFSET               16
#define ALTERA_MSGDMA_DESCRIPTOR_WRITE_BURST_COUNT_MASK                0xFF000000
#define ALTERA_MSGDMA_DESCRIPTOR_WRITE_BURST_COUNT_OFFSET              24


/* masks and offsets for the read and write strides */
#define ALTERA_MSGDMA_DESCRIPTOR_READ_STRIDE_MASK                      0xFFFF
#define ALTERA_MSGDMA_DESCRIPTOR_READ_STRIDE_OFFSET                    0
#define ALTERA_MSGDMA_DESCRIPTOR_WRITE_STRIDE_MASK                     0xFFFF0000
#define ALTERA_MSGDMA_DESCRIPTOR_WRITE_STRIDE_OFFSET                   16


/* masks and offsets for the bits in the descriptor control field */
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_TRANSMIT_CHANNEL_MASK         0xFF
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_TRANSMIT_CHANNEL_OFFSET       0
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GENERATE_SOP_MASK             (1 << 8)
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GENERATE_SOP_OFFSET           8
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GENERATE_EOP_MASK             (1 << 9)
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GENERATE_EOP_OFFSET           9
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_PARK_READS_MASK               (1 << 10)
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_PARK_READS_OFFSET             10
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_PARK_WRITES_MASK              (1 << 11)
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_PARK_WRITES_OFFSET            11
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_END_ON_EOP_MASK               (1 << 12)
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_END_ON_EOP_OFFSET             12
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_TRANSFER_COMPLETE_IRQ_MASK    (1 << 14)
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_TRANSFER_COMPLETE_IRQ_OFFSET  14
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_EARLY_TERMINATION_IRQ_MASK    (1 << 15)
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_EARLY_TERMINATION_IRQ_OFFSET  15
/* the read master will use this as the transmit error, the dispatcher will use 
this to generate an interrupt if any of the error bits are asserted by the 
write master */
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_ERROR_IRQ_MASK                (0xFF << 16)
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_ERROR_IRQ_OFFSET              16
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_EARLY_DONE_ENABLE_MASK        (1 << 24)
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_EARLY_DONE_ENABLE_OFFSET      24
/* at a minimum you always have to write '1' to this bit as it commits the 
descriptor to the dispatcher */
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GO_MASK                       (1 << 31)
#define ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GO_OFFSET                     31

/* Each register is byte lane accessible so the some of the values that are
 * less than 32 bits wide are written to according to the field width.
 */
#define IOWR_ALTERA_MSGDMA_DESCRIPTOR_READ_ADDRESS(base, data)  \
        IOWR_32DIRECT(base, ALTERA_MSGDMA_DESCRIPTOR_READ_ADDRESS_REG, data)
#define IOWR_ALTERA_MSGDMA_DESCRIPTOR_WRITE_ADDRESS(base, data)  \
        IOWR_32DIRECT(base, ALTERA_MSGDMA_DESCRIPTOR_WRITE_ADDRESS_REG, data)
#define IOWR_ALTERA_MSGDMA_DESCRIPTOR_LENGTH(base, data)  \
        IOWR_32DIRECT(base, ALTERA_MSGDMA_DESCRIPTOR_LENGTH_REG, data)
/* this pushes the descriptor into the read/write FIFOs when standard descriptors 
are used */
#define IOWR_ALTERA_MSGDMA_DESCRIPTOR_CONTROL_STANDARD(base, data)  \
        IOWR_32DIRECT(base, ALTERA_MSGDMA_DESCRIPTOR_CONTROL_STANDARD_REG, data)
#define IOWR_ALTERA_MSGDMA_DESCRIPTOR_SEQUENCE_NUMBER(base, data)  \
        IOWR_16DIRECT(base, ALTERA_MSGDMA_DESCRIPTOR_SEQUENCE_NUMBER_REG, data)
#define IOWR_ALTERA_MSGDMA_DESCRIPTOR_READ_BURST(base, data)  \
        IOWR_8DIRECT(base, ALTERA_MSGDMA_DESCRIPTOR_READ_BURST_REG, data)
#define IOWR_ALTERA_MSGDMA_DESCRIPTOR_WRITE_BURST(base, data)  \
        IOWR_8DIRECT(base, ALTERA_MSGDMA_DESCRIPTOR_WRITE_BURST_REG, data)
#define IOWR_ALTERA_MSGDMA_DESCRIPTOR_READ_STRIDE(base, data)  \
        IOWR_16DIRECT(base, ALTERA_MSGDMA_DESCRIPTOR_READ_STRIDE_REG, data)
#define IOWR_ALTERA_MSGDMA_DESCRIPTOR_WRITE_STRIDE(base, data)  \
        IOWR_16DIRECT(base, ALTERA_MSGDMA_DESCRIPTOR_WRITE_STRIDE_REG, data)
#define IOWR_ALTERA_MSGDMA_DESCRIPTOR_READ_ADDRESS_HIGH(base, data)  \
        IOWR_32DIRECT(base, ALTERA_MSGDMA_DESCRIPTOR_READ_ADDRESS_HIGH_REG, data)
#define IOWR_ALTERA_MSGDMA_DESCRIPTOR_WRITE_ADDRESS_HIGH(base, data)  \
        IOWR_32DIRECT(base, ALTERA_MSGDMA_DESCRIPTOR_WRITE_ADDRESS_HIGH_REG, data)
/* this pushes the descriptor into the read/write FIFOs when the extended 
descriptors are used */
#define IOWR_ALTERA_MSGDMA_DESCRIPTOR_CONTROL_ENHANCED(base, data)  \
        IOWR_32DIRECT(base, ALTERA_MSGDMA_DESCRIPTOR_CONTROL_ENHANCED_REG, data)



#endif /*ALTERA_MSGDMA_ALTERA_MSGDMA_DESCRIPTOR_REGS_H_*/
