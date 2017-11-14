/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2016 Altera Corporation, San Jose, California, USA.           *
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

#ifndef __ALT_AVALON_I2C_REGS_H__
#define __ALT_AVALON_I2C_REGS_H__

#include <io.h>

#define IORMW(base, reg, data, mask)                           IOWR(base,reg,(IORD(base,reg) & (~mask)) | (data & mask))

#define ALT_AVALON_I2C_TFR_CMD_REG                          0
#define IOADDR_ALT_AVALON_I2C_TFR_CMD(base)                 __IO_CALC_ADDRESS_NATIVE(base, ALT_AVALON_I2C_TFR_CMD_REG)
#define IORD_ALT_AVALON_I2C_TFR_CMD(base)                   IORD(base, ALT_AVALON_I2C_TFR_CMD_REG) 
#define IOWR_ALT_AVALON_I2C_TFR_CMD(base, data)             IOWR(base, ALT_AVALON_I2C_TFR_CMD_REG, data)
#define ALT_AVALON_I2C_TFR_CMD_STA_OFST                     (9)
#define ALT_AVALON_I2C_TFR_CMD_STA_MSK                      (1 << ALT_AVALON_I2C_TFR_CMD_STA_OFST)
#define ALT_AVALON_I2C_TFR_CMD_STO_OFST                     (8)
#define ALT_AVALON_I2C_TFR_CMD_STO_MSK                      (1 << ALT_AVALON_I2C_TFR_CMD_STO_OFST)
#define ALT_AVALON_I2C_TFR_CMD_AD_OFST                      (1)
#define ALT_AVALON_I2C_TFR_CMD_AD_MSK                       (0x7f << ALT_AVALON_I2C_TFR_CMD_AD_OFST)
#define ALT_AVALON_I2C_TFR_CMD_RW_D_OFST                    (0)
#define ALT_AVALON_I2C_TFR_CMD_RW_D_MSK                     (1 << ALT_AVALON_I2C_TFR_CMD_RW_D_OFST)   
#define ALT_AVALON_I2C_TFR_CMD_ALL_BITS_MSK                 (0x3ff)                               
                                                               
#define ALT_AVALON_I2C_RX_DATA_REG                          1
#define IOADDR_ALT_AVALON_I2C_RX_DATA(base)                 __IO_CALC_ADDRESS_NATIVE(base, ALT_AVALON_I2C_RX_DATA_REG)
#define IORD_ALT_AVALON_I2C_RX_DATA(base)                   IORD(base, ALT_AVALON_I2C_RX_DATA_REG) 
#define IOWR_ALT_AVALON_I2C_RX_DATA(base, data)             IOWR(base, ALT_AVALON_I2C_RX_DATA_REG, data)
#define ALT_AVALON_I2C_RX_DATA_RXDATA_OFST                  (0)
#define ALT_AVALON_I2C_RX_DATA_RXDATA_MSK                   (0xff << ALT_AVALON_I2C_RX_DATA_RXDATA_OFST)
                                                               
#define ALT_AVALON_I2C_CTRL_REG                             2
#define IOADDR_ALT_AVALON_I2C_CTRL(base)                    __IO_CALC_ADDRESS_NATIVE(base, ALT_AVALON_I2C_CTRL_REG)
#define IORD_ALT_AVALON_I2C_CTRL(base)                      IORD(base, ALT_AVALON_I2C_CTRL_REG) 
#define IOWR_ALT_AVALON_I2C_CTRL(base, data)                IOWR(base, ALT_AVALON_I2C_CTRL_REG, data)
#define IORMW_ALT_AVALON_I2C_CTRL(base, data, mask)         IORMW(base,ALT_AVALON_I2C_CTRL_REG,data,mask)
#define ALT_AVALON_I2C_CTRL_RX_DATA_FIFO_THD_OFST           (4)
#define ALT_AVALON_I2C_CTRL_RX_DATA_FIFO_THD_MSK            (3 << ALT_AVALON_I2C_CTRL_RX_DATA_FIFO_THD_OFST)
#define ALT_AVALON_I2C_CTRL_TFR_CMD_FIFO_THD_OFST           (2)
#define ALT_AVALON_I2C_CTRL_TFR_CMD_FIFO_THD_MSK            (3 << ALT_AVALON_I2C_CTRL_TFR_CMD_FIFO_THD_OFST)
#define ALT_AVALON_I2C_CTRL_BUS_SPEED_OFST                  (1)
#define ALT_AVALON_I2C_CTRL_BUS_SPEED_MSK                   (1 << ALT_AVALON_I2C_CTRL_BUS_SPEED_OFST)
#define ALT_AVALON_I2C_CTRL_EN_OFST                         (0)
#define ALT_AVALON_I2C_CTRL_EN_MSK                          (1 << ALT_AVALON_I2C_CTRL_EN_OFST)
                                                               
#define ALT_AVALON_I2C_ISER_REG                             3
#define IOADDR_ALT_AVALON_I2C_ISER(base)                    __IO_CALC_ADDRESS_NATIVE(base, ALT_AVALON_I2C_ISER_REG)
#define IORD_ALT_AVALON_I2C_ISER(base)                      IORD(base, ALT_AVALON_I2C_ISER_REG) 
#define IOWR_ALT_AVALON_I2C_ISER(base, data)                IOWR(base, ALT_AVALON_I2C_ISER_REG, data)
#define IORMW_ALT_AVALON_I2C_ISER(base, data, mask)         IORMW(base,ALT_AVALON_I2C_ISER_REG,data,mask)
#define ALT_AVALON_I2C_ISER_RX_OVER_EN_OFST                 (4)
#define ALT_AVALON_I2C_ISER_RX_OVER_EN_MSK                  (1 << ALT_AVALON_I2C_ISER_RX_OVER_EN_OFST)
#define ALT_AVALON_I2C_ISER_ARBLOST_DET_EN_OFST             (3)
#define ALT_AVALON_I2C_ISER_ARBLOST_DET_EN_MSK              (1 << ALT_AVALON_I2C_ISER_ARBLOST_DET_EN_OFST)
#define ALT_AVALON_I2C_ISER_NACK_DET_EN_OFST                (2)
#define ALT_AVALON_I2C_ISER_NACK_DET_EN_MSK                 (1 << ALT_AVALON_I2C_ISER_NACK_DET_EN_OFST)
#define ALT_AVALON_I2C_ISER_RX_READY_EN_OFST                (1)
#define ALT_AVALON_I2C_ISER_RX_READY_EN_MSK                 (1 << ALT_AVALON_I2C_ISER_RX_READY_EN_OFST)
#define ALT_AVALON_I2C_ISER_TX_READY_EN_OFST                (0)
#define ALT_AVALON_I2C_ISER_TX_READY_EN_MSK                 (1 << ALT_AVALON_I2C_ISER_TX_READY_EN_OFST)                                             
                                                               
#define ALT_AVALON_I2C_ISR_REG                              4
#define IOADDR_ALT_AVALON_I2C_ISR(base)                     __IO_CALC_ADDRESS_NATIVE(base, ALT_AVALON_I2C_ISR_REG)
#define IORD_ALT_AVALON_I2C_ISR(base)                       IORD(base, ALT_AVALON_I2C_ISR_REG) 
#define IOWR_ALT_AVALON_I2C_ISR(base, data)                 IOWR(base, ALT_AVALON_I2C_ISR_REG, data)
#define IORMW_ALT_AVALON_I2C_ISR(base, data, mask)          IORMW(base,ALT_AVALON_I2C_ISR_REG,data,mask)
#define ALT_AVALON_I2C_ISR_RX_OVER_OFST                    (4)
#define ALT_AVALON_I2C_ISR_RX_OVER_MSK                     (1 << ALT_AVALON_I2C_ISR_RX_OVER_OFST)
#define ALT_AVALON_I2C_ISR_ARBLOST_DET_OFST                (3)
#define ALT_AVALON_I2C_ISR_ARBLOST_DET_MSK                 (1 << ALT_AVALON_I2C_ISR_ARBLOST_DET_OFST)
#define ALT_AVALON_I2C_ISR_NACK_DET_OFST                   (2)
#define ALT_AVALON_I2C_ISR_NACK_DET_MSK                    (1 << ALT_AVALON_I2C_ISR_NACK_DET_OFST)
#define ALT_AVALON_I2C_ISR_RX_READY_OFST                   (1)
#define ALT_AVALON_I2C_ISR_RX_READY_MSK                    (1 << ALT_AVALON_I2C_ISR_RX_READY_OFST)
#define ALT_AVALON_I2C_ISR_TX_READY_OFST                   (0)
#define ALT_AVALON_I2C_ISR_TX_READY_MSK                    (1 << ALT_AVALON_I2C_ISR_TX_READY_OFST)
#define ALT_AVALON_I2C_ISR_ALLINTS_MSK                     (ALT_AVALON_I2C_ISR_RX_OVER_MSK | ALT_AVALON_I2C_ISR_ARBLOST_DET_MSK |  \
                                                               ALT_AVALON_I2C_ISR_NACK_DET_MSK | ALT_AVALON_I2C_ISR_RX_READY_MSK | \
                                                               ALT_AVALON_I2C_ISR_TX_READY_MSK)
#define ALT_AVALON_I2C_ISR_ALL_CLEARABLE_INTS_MSK          (ALT_AVALON_I2C_ISR_RX_OVER_MSK | ALT_AVALON_I2C_ISR_ARBLOST_DET_MSK |  \
                                                               ALT_AVALON_I2C_ISR_NACK_DET_MSK)                                                               
                                                              
#define ALT_AVALON_I2C_STATUS_REG                           5
#define IOADDR_ALT_AVALON_I2C_STATUS(base)                  __IO_CALC_ADDRESS_NATIVE(base, ALT_AVALON_I2C_STATUS_REG)
#define IORD_ALT_AVALON_I2C_STATUS(base)                    IORD(base, ALT_AVALON_I2C_STATUS_REG) 
#define IOWR_ALT_AVALON_I2C_STATUS(base, data)              IOWR(base, ALT_AVALON_I2C_STATUS_REG, data)
#define ALT_AVALON_I2C_STATUS_CORE_STATUS_OFST              (0)
#define ALT_AVALON_I2C_STATUS_CORE_STATUS_MSK               (1 << ALT_AVALON_I2C_STATUS_CORE_STATUS_OFST)
                                                               
#define ALT_AVALON_I2C_TFR_CMD_FIFO_LVL_REG                 6
#define IOADDR_ALT_AVALON_I2C_TFR_CMD_FIFO_LVL(base)        __IO_CALC_ADDRESS_NATIVE(base, ALT_AVALON_I2C_TFR_CMD_FIFO_LVL_REG)
#define IORD_ALT_AVALON_I2C_TFR_CMD_FIFO_LVL(base)          IORD(base, ALT_AVALON_I2C_TFR_CMD_FIFO_LVL_REG) 
#define IOWR_ALT_AVALON_I2C_TFR_CMD_FIFO_LVL(base, data)    IOWR(base, ALT_AVALON_I2C_TFR_CMD_FIFO_LVL_REG, data)
#define ALT_AVALON_I2C_TFR_CMD_FIFO_LVL_FIFO_DEPTH_OFST     (0)
#define ALT_AVALON_I2C_TFR_CMD_FIFO_LVL_FIFO_DEPTH_MSK(cmdfifodepth)      ((cmdfifodepth-1) << ALT_AVALON_I2C_TFR_CMD_FIFO_LVL_FIFO_DEPTH_OFST)
                                                               
#define ALT_AVALON_I2C_RX_DATA_FIFO_LVL_REG                 7
#define IOADDR_ALT_AVALON_I2C_RX_DATA_FIFO_LVL(base)        __IO_CALC_ADDRESS_NATIVE(base, ALT_AVALON_I2C_RX_DATA_FIFO_LVL_REG)
#define IORD_ALT_AVALON_I2C_RX_DATA_FIFO_LVL(base)          IORD(base, ALT_AVALON_I2C_RX_DATA_FIFO_LVL_REG) 
#define IOWR_ALT_AVALON_I2C_RX_DATA_FIFO_LVL(base, data)    IOWR(base, ALT_AVALON_I2C_RX_DATA_FIFO_LVL_REG, data)
#define ALT_AVALON_I2C_RX_DATA_FIFO_LVL_FIFO_DEPTH_OFST     (0)
#define ALT_AVALON_I2C_RX_DATA_FIFO_LVL_FIFO_DEPTH_MSK(rxfifodepth)      ((rxfifodepth-1) << ALT_AVALON_I2C_TFR_CMD_FIFO_LVL_FIFO_DEPTH_OFST)

#define ALT_AVALON_I2C_SCL_LOW_REG                          8
#define IOADDR_ALT_AVALON_I2C_SCL_LOW(base)                 __IO_CALC_ADDRESS_NATIVE(base, ALT_AVALON_I2C_SCL_LOW_REG)
#define IORD_ALT_AVALON_I2C_SCL_LOW(base)                   IORD(base, ALT_AVALON_I2C_SCL_LOW_REG) 
#define IOWR_ALT_AVALON_I2C_SCL_LOW(base, data)             IOWR(base, ALT_AVALON_I2C_SCL_LOW_REG, data)
#define ALT_AVALON_I2C_SCL_LOW_COUNT_PERIOD_OFST            (0)
#define ALT_AVALON_I2C_SCL_LOW_COUNT_PERIOD_MSK             (0xffff << ALT_AVALON_I2C_SCL_LOW_COUNT_PERIOD_OFST)
                                                               
#define ALT_AVALON_I2C_SCL_HIGH_REG                         9
#define IOADDR_ALT_AVALON_I2C_SCL_HIGH(base)                __IO_CALC_ADDRESS_NATIVE(base, ALT_AVALON_I2C_SCL_HIGH_REG)
#define IORD_ALT_AVALON_I2C_SCL_HIGH(base)                  IORD(base, ALT_AVALON_I2C_SCL_HIGH_REG) 
#define IOWR_ALT_AVALON_I2C_SCL_HIGH(base, data)            IOWR(base, ALT_AVALON_I2C_SCL_HIGH_REG, data)
#define ALT_AVALON_I2C_SCL_HIGH_COUNT_PERIOD_OFST           (0)
#define ALT_AVALON_I2C_SCL_HIGH_COUNT_PERIOD_MSK            (0xffff << ALT_AVALON_I2C_SCL_HIGH_COUNT_PERIOD_OFST)
                                                               
#define ALT_AVALON_I2C_SDA_HOLD_REG                         0xa
#define IOADDR_ALT_AVALON_I2C_SDA_HOLD(base)                __IO_CALC_ADDRESS_NATIVE(base, ALT_AVALON_I2C_SDA_HOLD_REG)
#define IORD_ALT_AVALON_I2C_SDA_HOLD(base)                  IORD(base, ALT_AVALON_I2C_SDA_HOLD_REG) 
#define IOWR_ALT_AVALON_I2C_SDA_HOLD(base, data)            IOWR(base, ALT_AVALON_I2C_SDA_HOLD_REG, data)
#define ALT_AVALON_I2C_SDA_HOLD_COUNT_PERIOD_OFST           (0)
#define ALT_AVALON_I2C_SDA_HOLD_COUNT_PERIOD_MSK            (0xffff << ALT_AVALON_I2C_SDA_HOLD_COUNT_PERIOD_OFST)

#endif /* __ALT_AVALON_I2C_REGS_H__ */
