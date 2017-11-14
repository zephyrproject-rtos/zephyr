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

#ifndef __ALTERA_AVALON_SGDMA_REGS_H__
#define __ALTERA_AVALON_SGDMA_REGS_H__

#include <io.h>

#define IOADDR_ALTERA_AVALON_SGDMA_STATUS(base)       __IO_CALC_ADDRESS_DYNAMIC(base, 0)
#define IORD_ALTERA_AVALON_SGDMA_STATUS(base)         IORD(base, 0)
#define IOWR_ALTERA_AVALON_SGDMA_STATUS(base, data)   IOWR(base, 0, data)

#define ALTERA_AVALON_SGDMA_STATUS_ERROR_MSK                        (0x1)
#define ALTERA_AVALON_SGDMA_STATUS_ERROR_OFST                       (0)
#define ALTERA_AVALON_SGDMA_STATUS_EOP_ENCOUNTERED_MSK              (0x2)
#define ALTERA_AVALON_SGDMA_STATUS_EOP_ENCOUNTERED_OFST             (1)
#define ALTERA_AVALON_SGDMA_STATUS_DESC_COMPLETED_MSK               (0x4)
#define ALTERA_AVALON_SGDMA_STATUS_DESC_COMPLETED_OFST              (2)
#define ALTERA_AVALON_SGDMA_STATUS_CHAIN_COMPLETED_MSK              (0x8)
#define ALTERA_AVALON_SGDMA_STATUS_CHAIN_COMPLETED_OFST             (3)
#define ALTERA_AVALON_SGDMA_STATUS_BUSY_MSK                         (0x10)
#define ALTERA_AVALON_SGDMA_STATUS_BUSY_OFST                        (4)

#define IOADDR_ALTERA_AVALON_SGDMA_VERSION(base)       __IO_CALC_ADDRESS_DYNAMIC(base, 1)
#define IORD_ALTERA_AVALON_SGDMA_VERSION(base)         IORD(base, 1)
#define IOWR_ALTERA_AVALON_SGDMA_VERSION(base, data)   IOWR(base, 1, data)
#define ALTERA_AVALON_SGDMA_VERSION_VERSION_MSK                     (0xFFFF)
#define ALTERA_AVALON_SGDMA_VERSION_VERSION_OFST                    (0)


#define IOADDR_ALTERA_AVALON_SGDMA_CONTROL(base)     __IO_CALC_ADDRESS_DYNAMIC(base, 4)
#define IORD_ALTERA_AVALON_SGDMA_CONTROL(base)        IORD(base, 4)
#define IOWR_ALTERA_AVALON_SGDMA_CONTROL(base, data)  IOWR(base, 4, data)
#define ALTERA_AVALON_SGDMA_CONTROL_IE_ERROR_MSK                     (0x1)
#define ALTERA_AVALON_SGDMA_CONTROL_IE_ERROR_OFST                    (0)
#define ALTERA_AVALON_SGDMA_CONTROL_IE_EOP_ENCOUNTERED_MSK           (0x2)
#define ALTERA_AVALON_SGDMA_CONTROL_IE_EOP_ENCOUNTERED_OFST          (1)
#define ALTERA_AVALON_SGDMA_CONTROL_IE_DESC_COMPLETED_MSK            (0x4)
#define ALTERA_AVALON_SGDMA_CONTROL_IE_DESC_COMPLETED_OFST           (2)
#define ALTERA_AVALON_SGDMA_CONTROL_IE_CHAIN_COMPLETED_MSK           (0x8)
#define ALTERA_AVALON_SGDMA_CONTROL_IE_CHAIN_COMPLETED_OFST          (3)
#define ALTERA_AVALON_SGDMA_CONTROL_IE_GLOBAL_MSK                    (0x10)
#define ALTERA_AVALON_SGDMA_CONTROL_IE_GLOBAL_OFST                   (4)
#define ALTERA_AVALON_SGDMA_CONTROL_RUN_MSK                          (0x20)
#define ALTERA_AVALON_SGDMA_CONTROL_RUN_OFST                         (5)
#define ALTERA_AVALON_SGDMA_CONTROL_STOP_DMA_ER_MSK                  (0x40)
#define ALTERA_AVALON_SGDMA_CONTROL_STOP_DMA_ER_OFST                 (6)
#define ALTERA_AVALON_SGDMA_CONTROL_IE_MAX_DESC_PROCESSED_MSK        (0x80)
#define ALTERA_AVALON_SGDMA_CONTROL_IE_MAX_DESC_PROCESSED_OFST       (7)
#define ALTERA_AVALON_SGDMA_CONTROL_MAX_DESC_PROCESSED_MSK           (0xFF00)
#define ALTERA_AVALON_SGDMA_CONTROL_MAX_DESC_PROCESSED_OFST          (8)
#define ALTERA_AVALON_SGDMA_CONTROL_SOFTWARERESET_MSK                (0X10000)
#define ALTERA_AVALON_SGDMA_CONTROL_SOFTWARERESET_OFST               (16)
#define ALTERA_AVALON_SGDMA_CONTROL_PARK_MSK                         (0X20000)
#define ALTERA_AVALON_SGDMA_CONTROL_PARK_OFST                        (17)
#define ALTERA_AVALON_SGDMA_CONTROL_DESC_POLL_EN_MSK                 (0X40000)
#define ALTERA_AVALON_SGDMA_CONTROL_DESC_POLL_EN_OFST                (18)
#define ALTERA_AVALON_SGDMA_CONTROL_DESC_POLL_FREQ_MSK               (0x7FF00000)
#define ALTERA_AVALON_SGDMA_CONTROL_DESC_POLL_FREQ_OFST              (20)
#define ALTERA_AVALON_SGDMA_CONTROL_CLEAR_INTERRUPT_MSK              (0X80000000)
#define ALTERA_AVALON_SGDMA_CONTROL_CLEAR_INTERRUPT_OFST             (31)

#define IOADDR_ALTERA_AVALON_SGDMA_NEXT_DESC_POINTER(base)     __IO_CALC_ADDRESS_DYNAMIC(base, 8)
#define IORD_ALTERA_AVALON_SGDMA_NEXT_DESC_POINTER(base)        IORD(base, 8)
#define IOWR_ALTERA_AVALON_SGDMA_NEXT_DESC_POINTER(base, data)  IOWR(base, 8, data)


#endif /* __ALTERA_AVALON_SGDMA_REGS_H__ */
