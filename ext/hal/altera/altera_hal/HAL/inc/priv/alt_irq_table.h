#ifndef __ALT_IRQ_TABLE_H__
#define __ALT_IRQ_TABLE_H__

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2009      Altera Corporation, San Jose, California, USA.      *
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

/******************************************************************************
*                                                                             *
* THIS IS A LIBRARY READ-ONLY SOURCE FILE. DO NOT EDIT.                       *
*                                                                             *
******************************************************************************/

/*
 * Definition of a table describing each interrupt handler. The index into 
 * the array is the interrupt id associated with the handler. 
 *
 * When an interrupt occurs, the associated handler is called with
 * the argument stored in the context member.
 *
 * The table is physically created in alt_irq_handler.c 
 */
extern struct ALT_IRQ_HANDLER
{
#ifdef ALT_ENHANCED_INTERRUPT_API_PRESENT
  void (*handler)(void*);
#else
  void (*handler)(void*, alt_u32);
#endif
  void *context;
} alt_irq[ALT_NIRQ];

#endif
