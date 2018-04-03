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
******************************************************************************/

#ifndef ALT_MSGDMA_PREFETCHER_REGS_H_
#define ALT_MSGDMA_PREFETCHER_REGS_H_

#include "io.h"

/*
  MSGDMA Prefetcher core is an additional micro core to existing MSGDMA core which 
  already consists of dispatcher, read master and write master micro core. Prefetcher
  core provides functionality to fetch a series of descriptors from memory that 
  describes the required data transfers before pass them to dispatcher core for data 
  transfer execution.
*/


/*
 * Component : MSGDMA PREFETCHER
 *
 */
#define ALT_MSGDMA_PREFETCHER_CONTROL_OFST				          0x00
#define ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_LOW_OFST        0x04
#define ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_HIGH_OFST       0x08
#define ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLL_FREQ_OFST           0x0C
#define ALT_MSGDMA_PREFETCHER_STATUS_OFST                         0x10

/*
 * New MSGDMA PREFETCHER Descriptor fields.  These are not prefetcher registers
 * they are in the prefetcher descriptor structs
 */
/* The mask used to set the ALT_MSGDMA_PREFETCHER_DESCRIPTOR_CTRL_OWN_BY_HW  value. */
#define ALT_MSGDMA_PREFETCHER_DESCRIPTOR_CTRL_OWN_BY_HW_SET_MASK      (1 << 30)
/* The mask used to clear the ALT_MSGDMA_PREFETCHER_DESCRIPTOR_CTRL_OWN_BY_HW value. */
#define ALT_MSGDMA_PREFETCHER_DESCRIPTOR_CTRL_OWN_BY_HW_CLR_MASK      0xBFFFFFFF
/* The bit offset of the ALT_MSGDMA_PREFETCHER_DESCRIPTOR_CTRL_OWN_BY_HW field. */
#define ALT_MSGDMA_PREFETCHER_DESCRIPTOR_CTRL_OWN_BY_HW_BIT_OFFSET    30
/* Extracts the ALT_MSGDMA_PREFETCHER_CTRL_DESC_POLL_EN field value from a register. */
#define ALT_MSGDMA_PREFETCHER_DESCRIPTOR_CTRL_OWN_BY_HW_GET(value) (((value) & 0x40000000) >> 30)
/* Produces a ALT_MSGDMA_PREFETCHER_CTRL_DESC_POLL_EN register field value suitable for setting the register. */
#define ALT_MSGDMA_PREFETCHER_DESCRIPTOR_CTRL_OWN_BY_HW_SET(value) (((value) << 30) & 0x40000000)

/*
 * Register : control 
 *
 * The control register has two defined bits.
 *
 * DESC_POLL_EN and RUN .
 *
 * Detailed description available in their individual bitfields
 *
 * Register Layout
 *
 *  Bits   | Access | Reset | Description
 * :-------|:-------|:------|:------------
 *  [0]    | R/W    | 0x0   | RUN
 *  [1]    | R/W    | 0x0   | DESC_POLL_EN
 *  [2]    | R/W1S  | 0x0   | RESET_PREFETCHER
 *  [3]    | R/W    | 0x0   | GLOBAL_INTR_EN_MASK
 *  [4]    | R/W    | 0x0   | PARK_MODE
 *  [31:5] | R      | 0x0   | RESERVED
 *
 */

/* bits making up the "control" register */

/* the RUN bit field in the control register */
/* The mask used to set the ALT_MSGDMA_PREFETCHER_CTRL_RUN register field value. */
#define ALT_MSGDMA_PREFETCHER_CTRL_RUN_SET_MASK         0x1
/* The mask used to clear the ALT_MSGDMA_PREFETCHER_CTRL_RUN register field value. */
#define ALT_MSGDMA_PREFETCHER_CTRL_RUN_CLR_MASK         0xFFFFFFFE
/* The bit offset of the ALT_MSGDMA_PREFETCHER_CTRL_RUN register field. */
#define ALT_MSGDMA_PREFETCHER_CTRL_RUN_BIT_OFFSET           0
/* Extracts the ALT_MSGDMA_PREFETCHER_CTRL_RUN field value from a register. */
#define ALT_MSGDMA_PREFETCHER_CTRL_RUN_GET(value) (((value) & 0x00000001) >> 0)
/* Produces a ALT_MSGDMA_PREFETCHER_CTRL_RUN register field value suitable for setting the register. */
#define ALT_MSGDMA_PREFETCHER_CTRL_RUN_SET(value) (((value) << 0) & 0x00000001)

/* the DESC_POLL_EN bit field in the control register */
/* The mask used to set the ALT_MSGDMA_PREFETCHER_CTRL_DESC_POLL_EN register field value. */
#define ALT_MSGDMA_PREFETCHER_CTRL_DESC_POLL_EN_MASK         0x2
/* The mask used to clear the ALT_MSGDMA_PREFETCHER_CTRL_DESC_POLL_EN register field value. */
#define ALT_MSGDMA_PREFETCHER_CTRL_DESC_POLL_EN_CLR_MASK     0xFFFFFFFD
/* The bit offset of the ALT_MSGDMA_PREFETCHER_CTRL_DESC_POLL_EN register field. */
#define ALT_MSGDMA_PREFETCHER_CTRL_DESC_POLL_EN_BIT_OFFSET           1
/* Extracts the ALT_MSGDMA_PREFETCHER_CTRL_DESC_POLL_EN field value from a register. */
#define ALT_MSGDMA_PREFETCHER_CTRL_DESC_POLL_EN_GET(value) (((value) & 0x00000002) >> 1)
/* Produces a ALT_MSGDMA_PREFETCHER_CTRL_DESC_POLL_EN register field value suitable for setting the register. */
#define ALT_MSGDMA_PREFETCHER_CTRL_DESC_POLL_EN_SET(value) (((value) << 1) & 0x00000002)

/* the RESET_PREFETCHER bit field in the control register */
/* The mask used to set the ALT_MSGDMA_PREFETCHER_CTRL_RESET register field value. */
#define ALT_MSGDMA_PREFETCHER_CTRL_RESET_SET_MASK         0x4
/* The mask used to clear the ALT_MSGDMA_PREFETCHER_CTRL_RUN register field value. */
#define ALT_MSGDMA_PREFETCHER_CTRL_RESET_CLR_MASK         0xFFFFFFFB
/* The bit offset of the ALT_MSGDMA_PREFETCHER_CTRL_RUN register field. */
#define ALT_MSGDMA_PREFETCHER_CTRL_RESET_BIT_OFFSET           2
/* Extracts the ALT_MSGDMA_PREFETCHER_CTRL_RUN field value from a register. */
#define ALT_MSGDMA_PREFETCHER_CTRL_RESET_GET(value) (((value) & 0x00000004) >> 2)
/* Produces a ALT_MSGDMA_PREFETCHER_CTRL_RUN register field value suitable for setting the register. */
#define ALT_MSGDMA_PREFETCHER_CTRL_RESET_SET(value) (((value) << 2) & 0x00000004)

/* the GLOBAL_INTR_EN_MASK bit field in the control register */
/* The mask used to set the ALT_MSGDMA_PREFETCHER_CTRL_GLOBAL_INTR_EN_MASK register field value. */
#define ALT_MSGDMA_PREFETCHER_CTRL_GLOBAL_INTR_EN_SET_MASK         0x8
/* The mask used to clear the ALT_MSGDMA_PREFETCHER_CTRL_RUN register field value. */
#define ALT_MSGDMA_PREFETCHER_CTRL_GLOBAL_INTR_EN_CLR_MASK         0xFFFFFFF7
/* The bit offset of the ALT_MSGDMA_PREFETCHER_CTRL_RUN register field. */
#define ALT_MSGDMA_PREFETCHER_CTRL_GLOBAL_INTR_EN_BIT_OFFSET           3
/* Extracts the ALT_MSGDMA_PREFETCHER_CTRL_RUN field value from a register. */
#define ALT_MSGDMA_PREFETCHER_CTRL_GLOBAL_INTR_EN_GET(value) (((value) & 0x00000008) >> 3)
/* Produces a ALT_MSGDMA_PREFETCHER_CTRL_RUN register field value suitable for setting the register. */
#define ALT_MSGDMA_PREFETCHER_CTRL_GLOBAL_INTR_EN_SET(value) (((value) << 3) & 0x00000008)

/* the PARK_MODE bit field in the control register */
/* The mask used to set the ALT_MSGDMA_PREFETCHER_CTRL_PARK_MODE register field value. */
#define ALT_MSGDMA_PREFETCHER_CTRL_PARK_MODE_SET_MASK         0x10
/* The mask used to clear the ALT_MSGDMA_PREFETCHER_CTRL_RUN register field value. */
#define ALT_MSGDMA_PREFETCHER_CTRL_PARK_MODE_CLR_MASK         0xFFFFFFEF
/* The bit offset of the ALT_MSGDMA_PREFETCHER_CTRL_RUN register field. */
#define ALT_MSGDMA_PREFETCHER_CTRL_PARK_MODE_BIT_OFFSET           4
/* Extracts the ALT_MSGDMA_PREFETCHER_CTRL_RUN field value from a register. */
#define ALT_MSGDMA_PREFETCHER_CTRL_PARK_MODE_GET(value) (((value) & 0x00000010) >> 4)
/* Produces a ALT_MSGDMA_PREFETCHER_CTRL_RUN register field value suitable for setting the register. */
#define ALT_MSGDMA_PREFETCHER_CTRL_PARK_MODE_SET(value) (((value) << 4) & 0x00000010)

/*
 * Registers : Next Descriptor Pointer Low/High 
 *
 * The register has no bit fields, the 64 bits represent an address.
 *
 * Register Layout
 *
 *  Bits   | Access | Reset | Description
 * :-------|:-------|:------|:------------
 *  [31:0] | R/W    | 0x0   | NEXT_PTR_ADDR_LOW
 *  [63:32]| R/W    | 0x0   | NEXT_PTR_ADDR_HIGH
 *
 */

/* bits making up the "Next Descriptor Pointer " register */

/* the NEXT_PTR_ADDR_LOW bit field in the ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_LOW register */
/* The mask used to set the ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_LOW_REG register field value. */
#define ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_LOW_SET_MASK         0xFFFFFFFF
/* The mask used to clear the ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_REG register field value. */
#define ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_LOW_CLR_MASK         0x0
/* The bit offset of the ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_REG register field. */
#define ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_LOW_BIT_OFFSET           0
/* Extracts the ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_REG field value from a register. */
#define ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_LOW_GET(value) (((value) & 0xFFFFFFFF) >> 0)
/* Produces a ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_REG register field value suitable for setting the register. */
#define ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_LOW_SET(value) (((value) << 0) & 0xFFFFFFFF)

/* the NEXT_PTR_ADDR_HIGH bit field in the ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_HIGH register */
/* The mask used to set the ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_HIGH_REG register field value. */
#define ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_HIGH_SET_MASK         0xFFFFFFFF
/* The mask used to clear the ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_REG register field value. */
#define ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_HIGH_CLR_MASK         0x0
/* The bit offset of the ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_REG register field. */
#define ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_HIGH_BIT_OFFSET           0
/* Extracts the ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_REG field value from a register. */
#define ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_HIGH_GET(value) (((value) & 0xFFFFFFFF) >> 0)
/* Produces a ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_REG register field value suitable for setting the register. */
#define ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_HIGH_SET(value) (((value) << 0) & 0xFFFFFFFF)


/*
 * Register : Descriptor Polling Frequency 
 *
 * The Descriptor Polling Frequency register has one defined bit field.
 *
 * POLL_FREQ
 *
 * Detailed description available in their individual bitfields
 *
 * Register Layout
 *
 *  Bits    | Access | Reset | Description
 * :--------|:-------|:------|:------------
 *  [15:0]  | R/W    | 0x0   | POLL_FREQ
 *  [31:16] | R      | 0x0   | RESERVED
 *
 */

/* bits making up the "DESC_POLL_FREQ" register */

/* the POLL_FREQ bit field in the ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLL_FREQ register */
/* The mask used to set the ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLL_FREQ register field value. */
#define ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLL_FREQ_SET_MASK         0xFFFF
/* The mask used to clear the ALT_MSGDMA_PREFETCHER_CTRL_RUN register field value. */
#define ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLL_FREQ_CLR_MASK         0xFFFF0000
/* The bit offset of the ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLL_FREQ register field. */
#define ALT_MSGDMA_PREFETCHER_CTRL_RUN_BIT_OFFSET           0
/* Extracts the ALT_MSGDMA_PREFETCHER_CTRL_RUN field value from a register. */
#define ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLL_FREQ_GET(value) (((value) & 0x0000FFFF) >> 0)
/* Produces a ALT_MSGDMA_PREFETCHER_CTRL_RUN register field value suitable for setting the register. */
#define ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLL_FREQ_SET(value) (((value) << 0) & 0x0000FFFF)


/*
 * Register : Status 
 *
 * The Status register has one defined bit field.
 *
 * IRQ
 *
 * Detailed description available in their individual bitfields
 *
 * Register Layout
 *
 *  Bits    | Access | Reset | Description
 * :--------|:-------|:------|:------------
 *  [0]     | R/W1C  | 0x0   | IRQ
 *  [31:1]  | R      | 0x0   | RESERVED
 *
 */

/* bits making up the "STATUS" register */

/* the IRQ bit field in the ALT_MSGDMA_PREFETCHER_STATUS register */
/* The mask used to set the ALT_MSGDMA_PREFETCHER_STATUS_IRQ register field value. */
#define ALT_MSGDMA_PREFETCHER_STATUS_IRQ_SET_MASK         0x1
/* The mask used to clear the ALT_MSGDMA_PREFETCHER_STATUS_IRQ register field value. */
#define ALT_MSGDMA_PREFETCHER_STATUS_IRQ_CLR_MASK         0xFFFFFFFE
/* The bit offset of the ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLL_FREQ register field. */
#define ALT_MSGDMA_PREFETCHER_STATUS_IRQ_BIT_OFFSET       0
/* Extracts the ALT_MSGDMA_PREFETCHER_CTRL_RUN field value from a register. */
#define ALT_MSGDMA_PREFETCHER_STATUS_IRQ_GET(value) (((value) & 0x00000001) >> 0)
/* Produces a ALT_MSGDMA_PREFETCHER_CTRL_RUN register field value suitable for setting the register. */
#define ALT_MSGDMA_PREFETCHER_STATUS_IRQ_SET(value) (((value) << 0) & 0x00000001)



/*****************************************************************/
/***   READ/WRITE macros for the MSGDMA PREFETCHER registers   ***/
/*****************************************************************/
/* ALT_MSGDMA_PREFETCHER_CONTROL_REG */
#define IORD_ALT_MSGDMA_PREFETCHER_CONTROL(base)  \
        IORD_32DIRECT(base, ALT_MSGDMA_PREFETCHER_CONTROL_OFST)
#define IOWR_ALT_MSGDMA_PREFETCHER_CONTROL(base, data)  \
        IOWR_32DIRECT(base, ALT_MSGDMA_PREFETCHER_CONTROL_OFST, data)
/* ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_LOW_REG */
#define IORD_ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_LOW(base)  \
        IORD_32DIRECT(base, ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_LOW_OFST)
#define IOWR_ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_LOW(base, data)  \
        IOWR_32DIRECT(base, ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_LOW_OFST, data)
/* ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_HIGH_REG */
#define IORD_ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_HIGH(base)  \
        IORD_32DIRECT(base, ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_HIGH_OFST)
#define IOWR_ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_HIGH(base, data)  \
        IOWR_32DIRECT(base, ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_HIGH_OFST, data)
/* ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLLING_FREQ_REG */
#define IORD_ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLLING_FREQ(base)  \
        IORD_32DIRECT(base, ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLL_FREQ_OFST)
#define IOWR_ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLLING_FREQ(base, data)  \
        IOWR_32DIRECT(base, ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLL_FREQ_OFST, data)
/* ALT_MSGDMA_PREFETCHER_STATUS_REG */
#define IORD_ALT_MSGDMA_PREFETCHER_STATUS(base)  \
        IORD_32DIRECT(base, ALT_MSGDMA_PREFETCHER_STATUS_OFST)
#define IOWR_ALT_MSGDMA_PREFETCHER_STATUS(base, data)  \
        IOWR_32DIRECT(base, ALT_MSGDMA_PREFETCHER_STATUS_OFST, data)

#endif /*ALT_MSGDMA_PREFETCHER_REGS_H_*/
