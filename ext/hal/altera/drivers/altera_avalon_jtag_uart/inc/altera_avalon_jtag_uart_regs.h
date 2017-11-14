/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2003 Altera Corporation, San Jose, California, USA.           *
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

#ifndef __ALTERA_AVALON_JTAG_UART_REGS_H__
#define __ALTERA_AVALON_JTAG_UART_REGS_H__

#include <io.h>

#define ALTERA_AVALON_JTAG_UART_DATA_REG                  0
#define IOADDR_ALTERA_AVALON_JTAG_UART_DATA(base)         \
        __IO_CALC_ADDRESS_NATIVE(base, ALTERA_AVALON_JTAG_UART_DATA_REG)
#define IORD_ALTERA_AVALON_JTAG_UART_DATA(base)           \
        IORD(base, ALTERA_AVALON_JTAG_UART_DATA_REG) 
#define IOWR_ALTERA_AVALON_JTAG_UART_DATA(base, data)     \
        IOWR(base, ALTERA_AVALON_JTAG_UART_DATA_REG, data)

#define ALTERA_AVALON_JTAG_UART_DATA_DATA_MSK             (0x000000FF)
#define ALTERA_AVALON_JTAG_UART_DATA_DATA_OFST            (0)
#define ALTERA_AVALON_JTAG_UART_DATA_RVALID_MSK           (0x00008000)
#define ALTERA_AVALON_JTAG_UART_DATA_RVALID_OFST          (15)
#define ALTERA_AVALON_JTAG_UART_DATA_RAVAIL_MSK           (0xFFFF0000)
#define ALTERA_AVALON_JTAG_UART_DATA_RAVAIL_OFST          (16)


#define ALTERA_AVALON_JTAG_UART_CONTROL_REG               1
#define IOADDR_ALTERA_AVALON_JTAG_UART_CONTROL(base)      \
        __IO_CALC_ADDRESS_NATIVE(base, ALTERA_AVALON_JTAG_UART_CONTROL_REG)
#define IORD_ALTERA_AVALON_JTAG_UART_CONTROL(base)        \
        IORD(base, ALTERA_AVALON_JTAG_UART_CONTROL_REG)
#define IOWR_ALTERA_AVALON_JTAG_UART_CONTROL(base, data)  \
        IOWR(base, ALTERA_AVALON_JTAG_UART_CONTROL_REG, data)

#define ALTERA_AVALON_JTAG_UART_CONTROL_RE_MSK            (0x00000001)
#define ALTERA_AVALON_JTAG_UART_CONTROL_RE_OFST           (0)
#define ALTERA_AVALON_JTAG_UART_CONTROL_WE_MSK            (0x00000002)
#define ALTERA_AVALON_JTAG_UART_CONTROL_WE_OFST           (1)
#define ALTERA_AVALON_JTAG_UART_CONTROL_RI_MSK            (0x00000100)
#define ALTERA_AVALON_JTAG_UART_CONTROL_RI_OFST           (8)
#define ALTERA_AVALON_JTAG_UART_CONTROL_WI_MSK            (0x00000200)
#define ALTERA_AVALON_JTAG_UART_CONTROL_WI_OFST           (9)
#define ALTERA_AVALON_JTAG_UART_CONTROL_AC_MSK            (0x00000400)
#define ALTERA_AVALON_JTAG_UART_CONTROL_AC_OFST           (10)
#define ALTERA_AVALON_JTAG_UART_CONTROL_WSPACE_MSK        (0xFFFF0000)
#define ALTERA_AVALON_JTAG_UART_CONTROL_WSPACE_OFST       (16)

#endif /* __ALTERA_AVALON_JTAG_UART_REGS_H__ */
