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
*                                                                             *
******************************************************************************/

#ifndef __ALTERA_QSPI_CONTROLLER2_REGS_H__
#define __ALTERA_QSPI_CONTROLLER2_REGS_H__

#include <io.h>

/*
 * QSPI_RD_STATUS register offset
 *
 * The QSPI_RD_STATUS register contains information from the read status 
 * register operation. A full description of the register can be found in the 
 * data sheet,
 *
 */
#define ALTERA_QSPI_CONTROLLER2_STATUS_REG                       (0x0)

/*
 * QSPI_RD_STATUS register access macros
 */
#define IOADDR_ALTERA_QSPI_CONTROLLER2_STATUS(base) \
    __IO_CALC_ADDRESS_DYNAMIC(base, ALTERA_QSPI_CONTROLLER2_STATUS_REG)

#define IORD_ALTERA_QSPI_CONTROLLER2_STATUS(base) \
    IORD_32DIRECT(base, ALTERA_QSPI_CONTROLLER2_STATUS_REG)

#define IOWR_ALTERA_QSPI_CONTROLLER2_STATUS(base, data) \
    IOWR_32DIRECT(base, ALTERA_QSPI_CONTROLLER2_STATUS_REG, data)

/*
 * QSPI_RD_STATUS register description macros
 */

/** Write in progress bit */
#define ALTERA_QSPI_CONTROLLER2_STATUS_WIP_MASK                  (0x00000001)
#define ALTERA_QSPI_CONTROLLER2_STATUS_WIP_AVAILABLE             (0x00000000)
#define ALTERA_QSPI_CONTROLLER2_STATUS_WIP_BUSY                  (0x00000001)
/** When to time out a poll of the write in progress bit */
/* 0.7 sec time out */
#define ALTERA_QSPI_CONTROLLER2_1US_TIMEOUT_VALUE    		    700000 

/*
 * QSPI_RD_SID register offset
 *
 * The QSPI_RD_SID register contains the information from the read silicon ID 
 * operation and can be used to determine what type of EPCS device we have.
 * Only support in EPCS16 and EPCS64.
 *
 * This register is valid only if the device is an EPCS.
 *
 */
#define ALTERA_QSPI_CONTROLLER2_SID_REG                          (0x4)

/*
 * QSPI_RD_SID register access macros
 */
#define IOADDR_ALTERA_QSPI_CONTROLLER2_SID(base) \
    __IO_CALC_ADDRESS_DYNAMIC(base, ALTERA_QSPI_CONTROLLER2_SID_REG)

#define IORD_ALTERA_QSPI_CONTROLLER2_SID(base) \
    IORD_32DIRECT(base, ALTERA_QSPI_CONTROLLER2_SID_REG)

#define IOWR_ALTERA_QSPI_CONTROLLER2_SID(base, data) \
    IOWR_32DIRECT(base, ALTERA_QSPI_CONTROLLER2_SID_REG, data)

/*
 * QSPI_RD_SID register description macros
 * 
 * Specific device values obtained from Table 14 of: 
 * "Serial Configuration (EPCS) Devices Datasheet"
 */
#define ALTERA_QSPI_CONTROLLER2_SID_MASK                         (0x000000FF)
#define ALTERA_QSPI_CONTROLLER2_SID_EPCS16                       (0x00000014)
#define ALTERA_QSPI_CONTROLLER2_SID_EPCS64                       (0x00000016)
#define ALTERA_QSPI_CONTROLLER2_SID_EPCS128                      (0x00000018)

/*
 * QSPI_RD_RDID register offset
 *
 * The QSPI_RD_RDID register contains the information from the read memory 
 * capacity operation and can be used to determine what type of EPCQ/QSPI device 
 * we have.
 *
 * This register is only valid if the device is an EPCQ/QSPI.
 *
 */
#define ALTERA_QSPI_CONTROLLER2_RDID_REG                         (0x8)

/*
 * QSPI_RD_RDID register access macros
 */
#define IOADDR_ALTERA_QSPI_CONTROLLER2_RDID(base) \
    __IO_CALC_ADDRESS_DYNAMIC(base, ALTERA_QSPI_CONTROLLER2_RDID_REG)

#define IORD_ALTERA_QSPI_CONTROLLER2_RDID(base) \
    IORD_32DIRECT(base, ALTERA_QSPI_CONTROLLER2_RDID_REG)

#define IOWR_ALTERA_QSPI_CONTROLLER2_RDID(base, data) \
    IOWR_32DIRECT(base, ALTERA_QSPI_CONTROLLER2_RDID_REG, data)

/*
 * QSPI_RD_RDID register description macros
 * 
 * Specific device values obtained from Table 28 of: 
 *  "Quad-Serial Configuration (EPCQ/QSPI? (www.altera.com/literature/hb/cfg/cfg_cf52012.pdf))
 *  Devices Datasheet"
 */
#define ALTERA_QSPI_CONTROLLER2_RDID_MASK                         (0x000000FF)
#define ALTERA_QSPI_CONTROLLER2_RDID_QSPI16                       (0x00000015)
#define ALTERA_QSPI_CONTROLLER2_RDID_QSPI32                       (0x00000016)
#define ALTERA_QSPI_CONTROLLER2_RDID_QSPI64                       (0x00000017)
#define ALTERA_QSPI_CONTROLLER2_RDID_QSPI128                      (0x00000018)
#define ALTERA_QSPI_CONTROLLER2_RDID_QSPI256                      (0x00000019)
#define ALTERA_QSPI_CONTROLLER2_RDID_QSPI512                      (0x00000020)
#define ALTERA_QSPI_CONTROLLER2_RDID_QSPI1024                     (0x00000021)

/*
 * QSPI_MEM_OP register offset
 *
 * The QSPI_MEM_OP register is used to do memory protect and erase operations
 *
 */
#define ALTERA_QSPI_CONTROLLER2_MEM_OP_REG                       (0xC)

/*
 * QSPI_MEM_OP register access macros
 */
#define IOADDR_ALTERA_QSPI_CONTROLLER2_MEM_OP(base) \
    __IO_CALC_ADDRESS_DYNAMIC(base, ALTERA_QSPI_CONTROLLER2_MEM_OP_REG)

#define IORD_ALTERA_QSPI_CONTROLLER2_MEM_OP(base) \
    IORD_32DIRECT(base, ALTERA_QSPI_CONTROLLER2_MEM_OP_REG)

#define IOWR_ALTERA_QSPI_CONTROLLER2_MEM_OP(base, data) \
    IOWR_32DIRECT(base, ALTERA_QSPI_CONTROLLER2_MEM_OP_REG, data)

/*
 * QSPI_MEM_OP register description macros
 */
#define ALTERA_QSPI_CONTROLLER2_MEM_OP_CMD_MASK                  (0x00000003)
#define ALTERA_QSPI_CONTROLLER2_MEM_OP_BULK_ERASE_CMD            (0x00000001)
#define ALTERA_QSPI_CONTROLLER2_MEM_OP_SECTOR_ERASE_CMD          (0x00000002)
#define ALTERA_QSPI_CONTROLLER2_MEM_OP_SECTOR_PROTECT_CMD        (0x00000003)

/** see datasheet for sector values */
#define ALTERA_QSPI_CONTROLLER2_MEM_OP_SECTOR_VALUE_MASK         (0x00FFFF00)

/*
 * QSPI_ISR register offset
 *
 * The QSPI_ISR register is used to determine whether an invalid write or erase 
 * operation triggered an interrupt
 *
 */
#define ALTERA_QSPI_CONTROLLER2_ISR_REG                          (0x10)

/*
 * QSPI_ISR register access macros
 */
#define IOADDR_ALTERA_QSPI_CONTROLLER2_ISR(base) \
    __IO_CALC_ADDRESS_DYNAMIC(base, ALTERA_QSPI_CONTROLLER2_ISR_REG)

#define IORD_ALTERA_QSPI_CONTROLLER2_ISR(base) \
    IORD_32DIRECT(base, ALTERA_QSPI_CONTROLLER2_ISR_REG)

#define IOWR_ALTERA_QSPI_CONTROLLER2_ISR(base, data) \
    IOWR_32DIRECT(base, ALTERA_QSPI_CONTROLLER2_ISR_REG, data)

/*
 * QSPI_ISR register description macros
 */
#define ALTERA_QSPI_CONTROLLER2_ISR_ILLEGAL_ERASE_MASK           (0x00000001)
#define ALTERA_QSPI_CONTROLLER2_ISR_ILLEGAL_ERASE_ACTIVE         (0x00000001)

#define ALTERA_QSPI_CONTROLLER2_ISR_ILLEGAL_WRITE_MASK           (0x00000002)
#define ALTERA_QSPI_CONTROLLER2_ISR_ILLEGAL_WRITE_ACTIVE         (0x00000002)


/*
 * QSPI_IMR register offset
 *
 * The QSPI_IMR register is used to mask the invalid erase or the invalid write 
 * interrupts.
 *
 */
#define ALTERA_QSPI_CONTROLLER2_IMR_REG                          (0x14)

/*
 * QSPI_IMR register access macros
 */
#define IOADDR_ALTERA_QSPI_CONTROLLER2_IMR(base) \
    __IO_CALC_ADDRESS_DYNAMIC(base, ALTERA_QSPI_CONTROLLER2_IMR_REG)

#define IORD_ALTERA_QSPI_CONTROLLER2_IMR(base) \
    IORD_32DIRECT(base, ALTERA_QSPI_CONTROLLER2_IMR_REG)

#define IOWR_ALTERA_QSPI_CONTROLLER2_IMR(base, data) \
    IOWR_32DIRECT(base, ALTERA_QSPI_CONTROLLER2_IMR_REG, data)

/*
 * QSPI_IMR register description macros
 */
#define ALTERA_QSPI_CONTROLLER2_IMR_ILLEGAL_ERASE_MASK           (0x00000001)
#define ALTERA_QSPI_CONTROLLER2_IMR_ILLEGAL_ERASE_ENABLED        (0x00000001)

#define ALTERA_QSPI_CONTROLLER2_IMR_ILLEGAL_WRITE_MASK           (0x00000002)
#define ALTERA_QSPI_CONTROLLER2_IMR_ILLEGAL_WRITE_ENABLED        (0x00000002)

/*
 * QSPI_CHIP_SELECT register offset
 *
 * The QSPI_CHIP_SELECT register is used to issue chip select 
 */
#define ALTERA_QSPI_CHIP_SELECT_REG                          (0x18)

/*
 * QSPI_CHIP_SELECT register access macros
 */
#define IOADDR_ALTERA_QSPI_CHIP_SELECT(base) \
    __IO_CALC_ADDRESS_DYNAMIC(base, ALTERA_QSPI_CHIP_SELECT_REG)

#define IOWR_ALTERA_QSPI_CHIP_SELECT(base, data) \
    IOWR_32DIRECT(base, ALTERA_QSPI_CHIP_SELECT_REG, data)

/*
 * QSPI_CHIP_SELECT register description macros
 */
#define ALTERA_QSPI_CHIP1_SELECT        (0x00000001)
#define ALTERA_QSPI_CHIP2_SELECT        (0x00000002)
#define ALTERA_QSPI_CHIP3_SELECT        (0x00000003)

#endif /* __ALTERA_QSPI_CONTROLLER2_REGS_H__ */
