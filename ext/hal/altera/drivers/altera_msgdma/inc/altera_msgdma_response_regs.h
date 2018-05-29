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

#ifndef ALTERA_MSGDMA_RESPONSE_REGS_H_
#define ALTERA_MSGDMA_RESPONSE_REGS_H_

#include "io.h"

/*
  The response slave port only carries the actual bytes transferred,
  error, and early termination bits.  Reading from the upper most byte
  of the 2nd register pops the response FIFO.  For proper FIFO popping
  always read the actual bytes transferred followed by the error and early
  termination bits using 'little endian' accesses.  If a big endian
  master accesses the response slave port make sure that address 0x7 is the
  last byte lane access as it's the one that pops the reponse FIFO.
  
  If you use a pre-fetching descriptor master in front of the dispatcher
  port then you do not need to access this response slave port. 
*/



#define ALTERA_MSGDMA_RESPONSE_ACTUAL_BYTES_TRANSFERRED_REG    0x0
#define ALTERA_MSGDMA_RESPONSE_ERRORS_REG                      0x4

/* bits making up the "errors" register */
#define ALTERA_MSGDMA_RESPONSE_ERROR_MASK                      0xFF
#define ALTERA_MSGDMA_RESPONSE_ERROR_OFFSET                    0
#define ALTERA_MSGDMA_RESPONSE_EARLY_TERMINATION_MASK          (1 << 8)
#define ALTERA_MSGDMA_RESPONSE_EARLY_TERMINATION_OFFSET        8


/* read macros for each 32 bit register */
#define IORD_ALTERA_MSGDMA_RESPONSE_ACTUAL_BYTES_TRANSFERRED(base)  \
        IORD_32DIRECT(base, ALTERA_MSGDMA_RESPONSE_ACTUAL_BYTES_TRANSFERRED_REG)
/* this read pops the response FIFO */
#define IORD_ALTERA_MSGDMA_RESPONSE_ERRORS_REG(base)  \
        IORD_32DIRECT(base, ALTERA_MSGDMA_RESPONSE_ERRORS_REG)   


#endif /*ALTERA_MSGDMA_RESPONSE_REGS_H_*/
