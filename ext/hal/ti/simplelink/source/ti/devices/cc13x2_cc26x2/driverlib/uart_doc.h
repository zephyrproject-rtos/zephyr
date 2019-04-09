/******************************************************************************
*  Filename:       uart_doc.h
*  Revised:        2018-02-09 15:45:36 +0100 (fr, 09 feb 2018)
*  Revision:       51470
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/
/*!
\addtogroup uart_api
@{

\section sec_uart_printf Use printf()

DriverLib only supports writing a single character at a time to the UART buffer but it is
possible to utilize the library function \c printf by overriding a few of the functions used by
\c printf with a device specific definition. However, the implementation of \c printf is
compiler specific and requires different functions to be overridden depending on the compiler.

Using \c printf can increase code size significantly but some compilers provide a highly optimized
and configurable implementation suitable for embedded systems which makes the code size increase
acceptable for most applications. See the compiler's documentation for details about how to
configure the \c printf library function.

It is required that the application configures and enables the UART module before using \c printf
function.

\subsection sec_uart_printf_ccs Code Composer Studio

In Code Composer Studio the functions \c fputc and \c fputs must be overridden.

\code{.c}
#include <stdio.h>
#include <string.h>

#define PRINTF_UART                     UART0_BASE

// Override 'fputc' function in order to use printf() to output to UART
int fputc(int _c, register FILE *_fp)
{
    UARTCharPut(PRINTF_UART, (uint8_t)_c);
    return _c;
}

// Override 'fputs' function in order to use printf() to output to UART
int fputs(const char *_ptr, register FILE *_fp)
{
  unsigned int i, len;

  len = strlen(_ptr);

  for(i=0 ; i<len ; i++)
  {
      UARTCharPut(PRINTF_UART, (uint8_t)_ptr[i]);
  }

  return len;
}
\endcode

\subsection sec_uart_printf_iar IAR

In IAR the function \c putchar must be overridden.

\code{.c}
#include <stdio.h>
#include <string.h>

#define PRINTF_UART                     UART0_BASE

// Override 'putchar' function in order to use printf() to output to UART.
int putchar(int data)
{
    UARTCharPut(PRINTF_UART, (uint8_t)data);
    return data;
}
\endcode

@}
*/
