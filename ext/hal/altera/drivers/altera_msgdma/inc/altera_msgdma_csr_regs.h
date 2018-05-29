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

#ifndef ALTERA_MSGDMA_CSR_REGS_H_
#define ALTERA_MSGDMA_CSR_REGS_H_

#include "io.h"
/*
  Enhanced features off:

  Bytes     Access Type     Description
  -----     -----------     -----------
  0-3       R/Clr           Status(1)
  4-7       R/W             Control(2)
  8-12      R               Descriptor Fill Level(write fill level[15:0], read 
                            fill level[15:0])
  13-15     R               Response Fill Level[15:0]
  16-31     N/A             <Reserved>


  Enhanced features on:

  Bytes     Access Type     Description
  -----     -----------     -----------
  0-3       R/Clr           Status(1)
  4-7       R/W             Control(2)
  8-12      R               Descriptor Fill Level (write fill level[15:0], read 
                            fill level[15:0])
  13-15     R               Response Fill Level[15:0]
  16-20     R               Sequence Number (write sequence number[15:0], read 
                            sequence number[15:0])
  21-31     N/A             <Reserved>

  (1)  Writing a '1' to the interrupt bit of the status register clears the 
       interrupt bit (when applicable), all other bits are unaffected by writes.
  (2)  Writing to the software reset bit will clear the entire register 
       (as well as all the registers for the entire msgdma).

  Status Register:

  Bits      Description
  ----      -----------
  0         Busy
  1         Descriptor Buffer Empty
  2         Descriptor Buffer Full
  3         Response Buffer Empty
  4         Response Buffer Full
  5         Stop State
  6         Reset State
  7         Stopped on Error
  8         Stopped on Early Termination
  9         IRQ
  10-31     <Reserved>

  Control Register:

  Bits      Description
  ----      -----------
  0         Stop (will also be set if a stop on error/early termination 
            condition occurs)
  1         Software Reset
  2         Stop on Error
  3         Stop on Early Termination
  4         Global Interrupt Enable Mask
  5         Stop dispatcher (stops the dispatcher from issuing more read/write 
            commands)
  6-31      <Reserved>
*/



#define ALTERA_MSGDMA_CSR_STATUS_REG                          0x0
#define ALTERA_MSGDMA_CSR_CONTROL_REG                         0x4
#define ALTERA_MSGDMA_CSR_DESCRIPTOR_FILL_LEVEL_REG           0x8
#define ALTERA_MSGDMA_CSR_RESPONSE_FILL_LEVEL_REG             0xC
/* this register only exists when the enhanced features are enabled */
#define ALTERA_MSGDMA_CSR_SEQUENCE_NUMBER_REG                 0x10


/* masks for the status register bits */
#define ALTERA_MSGDMA_CSR_BUSY_MASK                           1
#define ALTERA_MSGDMA_CSR_BUSY_OFFSET                         0
#define ALTERA_MSGDMA_CSR_DESCRIPTOR_BUFFER_EMPTY_MASK        (1 << 1)
#define ALTERA_MSGDMA_CSR_DESCRIPTOR_BUFFER_EMPTY_OFFSET      1
#define ALTERA_MSGDMA_CSR_DESCRIPTOR_BUFFER_FULL_MASK         (1 << 2)
#define ALTERA_MSGDMA_CSR_DESCRIPTOR_BUFFER_FULL_OFFSET       2
#define ALTERA_MSGDMA_CSR_RESPONSE_BUFFER_EMPTY_MASK          (1 << 3)
#define ALTERA_MSGDMA_CSR_RESPONSE_BUFFER_EMPTY_OFFSET        3
#define ALTERA_MSGDMA_CSR_RESPONSE_BUFFER_FULL_MASK           (1 << 4)
#define ALTERA_MSGDMA_CSR_RESPONSE_BUFFER_FULL_OFFSET         4
#define ALTERA_MSGDMA_CSR_STOP_STATE_MASK                     (1 << 5)
#define ALTERA_MSGDMA_CSR_STOP_STATE_OFFSET                   5
#define ALTERA_MSGDMA_CSR_RESET_STATE_MASK                    (1 << 6)
#define ALTERA_MSGDMA_CSR_RESET_STATE_OFFSET                  6
#define ALTERA_MSGDMA_CSR_STOPPED_ON_ERROR_MASK               (1 << 7)
#define ALTERA_MSGDMA_CSR_STOPPED_ON_ERROR_OFFSET             7
#define ALTERA_MSGDMA_CSR_STOPPED_ON_EARLY_TERMINATION_MASK   (1 << 8)
#define ALTERA_MSGDMA_CSR_STOPPED_ON_EARLY_TERMINATION_OFFSET 8
#define ALTERA_MSGDMA_CSR_IRQ_SET_MASK                        (1 << 9)
#define ALTERA_MSGDMA_CSR_IRQ_SET_OFFSET                      9

/* masks for the control register bits */
#define ALTERA_MSGDMA_CSR_STOP_MASK                           1
#define ALTERA_MSGDMA_CSR_STOP_OFFSET                         0
#define ALTERA_MSGDMA_CSR_RESET_MASK                          (1 << 1)
#define ALTERA_MSGDMA_CSR_RESET_OFFSET                        1
#define ALTERA_MSGDMA_CSR_STOP_ON_ERROR_MASK                  (1 << 2)
#define ALTERA_MSGDMA_CSR_STOP_ON_ERROR_OFFSET                2
#define ALTERA_MSGDMA_CSR_STOP_ON_EARLY_TERMINATION_MASK      (1 << 3)
#define ALTERA_MSGDMA_CSR_STOP_ON_EARLY_TERMINATION_OFFSET    3
#define ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK               (1 << 4)
#define ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_OFFSET             4
#define ALTERA_MSGDMA_CSR_STOP_DESCRIPTORS_MASK               (1 << 5)
#define ALTERA_MSGDMA_CSR_STOP_DESCRIPTORS_OFFSET             5

/* masks for the FIFO fill levels and sequence number */
#define ALTERA_MSGDMA_CSR_READ_FILL_LEVEL_MASK                0xFFFF
#define ALTERA_MSGDMA_CSR_READ_FILL_LEVEL_OFFSET              0
#define ALTERA_MSGDMA_CSR_WRITE_FILL_LEVEL_MASK               0xFFFF0000
#define ALTERA_MSGDMA_CSR_WRITE_FILL_LEVEL_OFFSET             16
#define ALTERA_MSGDMA_CSR_RESPONSE_FILL_LEVEL_MASK            0xFFFF
#define ALTERA_MSGDMA_CSR_RESPONSE_FILL_LEVEL_OFFSET          0
#define ALTERA_MSGDMA_CSR_READ_SEQUENCE_NUMBER_MASK           0xFFFF
#define ALTERA_MSGDMA_CSR_READ_SEQUENCE_NUMBER_OFFSET         0
#define ALTERA_MSGDMA_CSR_WRITE_SEQUENCE_NUMBER_MASK          0xFFFF0000
#define ALTERA_MSGDMA_CSR_WRITE_SEQUENCE_NUMBER_OFFSET        16


/* read/write macros for each 32 bit register of the CSR port */
#define IOWR_ALTERA_MSGDMA_CSR_STATUS(base, data)  \
        IOWR_32DIRECT(base, ALTERA_MSGDMA_CSR_STATUS_REG, data)
#define IOWR_ALTERA_MSGDMA_CSR_CONTROL(base, data)  \
        IOWR_32DIRECT(base, ALTERA_MSGDMA_CSR_CONTROL_REG, data)
#define IORD_ALTERA_MSGDMA_CSR_STATUS(base)  \
        IORD_32DIRECT(base, ALTERA_MSGDMA_CSR_STATUS_REG)
#define IORD_ALTERA_MSGDMA_CSR_CONTROL(base)  \
        IORD_32DIRECT(base, ALTERA_MSGDMA_CSR_CONTROL_REG)
#define IORD_ALTERA_MSGDMA_CSR_DESCRIPTOR_FILL_LEVEL(base)  \
        IORD_32DIRECT(base, ALTERA_MSGDMA_CSR_DESCRIPTOR_FILL_LEVEL_REG)
#define IORD_ALTERA_MSGDMA_CSR_RESPONSE_FILL_LEVEL(base)  \
        IORD_32DIRECT(base, ALTERA_MSGDMA_CSR_RESPONSE_FILL_LEVEL_REG)
#define IORD_ALTERA_MSGDMA_CSR_SEQUENCE_NUMBER(base)  \
        IORD_32DIRECT(base, ALTERA_MSGDMA_CSR_SEQUENCE_NUMBER_REG)



#endif /*ALTERA_MSGDMA_ALTERA_MSGDMA_CSR_REGS_H_*/
