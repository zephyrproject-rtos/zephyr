/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "device_imx.h"
#include "debug_console_imx.h"
#include "uart_imx.h"
#include "print_scan.h"

#if __ICCARM__
#include <yfuns.h>
#endif

static int debug_putc(int ch, void* stream);
static void UART_SendDataPolling(void *base, const uint8_t *txBuff, uint32_t txSize);
static void UART_ReceiveDataPolling(void *base, uint8_t *rxBuff, uint32_t rxSize);

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief Operation functions definitions for debug console. */
typedef struct DebugConsoleOperationFunctions {
    void (* Send)(void *base, const uint8_t *buf, uint32_t count);
    void (* Receive)(void *base, uint8_t *buf, uint32_t count);
} debug_console_ops_t;

/*! @brief State structure storing debug console. */
typedef struct DebugConsoleState {
    bool  inited;                    /*<! Identify debug console inited or not. */
    void* base;                      /*<! Base of the IP register. */
    debug_console_ops_t ops;         /*<! Operation function pointers for debug uart operations. */
} debug_console_state_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Debug UART state information.*/
static debug_console_state_t s_debugConsole;

/*******************************************************************************
 * Code
 ******************************************************************************/
/* See fsl_debug_console_imx.h for documentation of this function.*/
debug_console_status_t DbgConsole_Init(UART_Type* base,
                                       uint32_t clockRate,
                                       uint32_t baudRate,
                                       uint32_t mode)
{
    if (s_debugConsole.inited)
    {
        return status_DEBUGCONSOLE_Failed;
    }

    s_debugConsole.base = base;
    /* Setup UART init structure. */
    uart_init_config_t uart_init_str = {.clockRate  = clockRate,
                                        .baudRate   = baudRate,
                                        .wordLength = uartWordLength8Bits,
                                        .stopBitNum = uartStopBitNumOne,
                                        .parity     = uartParityDisable,
                                        .direction  = uartDirectionTxRx};
    /* UART Init operation */
    UART_Init(s_debugConsole.base, &uart_init_str);
    UART_Enable(s_debugConsole.base);
    UART_SetModemMode(s_debugConsole.base, mode);
    /* Set the function pointer for send and receive for this kind of device. */
    s_debugConsole.ops.Send = UART_SendDataPolling;
    s_debugConsole.ops.Receive = UART_ReceiveDataPolling;

    s_debugConsole.inited = true;
    return status_DEBUGCONSOLE_Success;
}

/* See fsl_debug_console.h for documentation of this function.*/
debug_console_status_t DbgConsole_DeInit(void)
{
    if (!s_debugConsole.inited)
    {
        return status_DEBUGCONSOLE_Success;
    }

    /* UART Deinit operation */
    UART_Disable(s_debugConsole.base);
    UART_Deinit(s_debugConsole.base);

    s_debugConsole.inited = false;

    return status_DEBUGCONSOLE_Success;
}

#if __ICCARM__
#pragma weak __write
size_t __write(int handle, const unsigned char * buffer, size_t size)
{
    if (buffer == 0)
    {
        /* This means that we should flush internal buffers.  Since we*/
        /* don't we just return.  (Remember, "handle" == -1 means that all*/
        /* handles should be flushed.)*/
        return 0;
    }

    /* This function only writes to "standard out" and "standard err",*/
    /* for all other file handles it returns failure.*/
    if ((handle != _LLIO_STDOUT) && (handle != _LLIO_STDERR))
    {
        return _LLIO_ERROR;
    }

    /* Do nothing if the debug uart is not initialized.*/
    if (!s_debugConsole.inited)
    {
        return _LLIO_ERROR;
    }

    /* Send data.*/
    s_debugConsole.ops.Send(s_debugConsole.base, (uint8_t const *)buffer, size);
    return size;
}

#pragma weak __read
size_t __read(int handle, unsigned char * buffer, size_t size)
{
    /* This function only reads from "standard in", for all other file*/
    /* handles it returns failure.*/
    if (handle != _LLIO_STDIN)
    {
        return _LLIO_ERROR;
    }

    /* Do nothing if the debug uart is not initialized.*/
    if (!s_debugConsole.inited)
    {
        return _LLIO_ERROR;
    }

    /* Receive data.*/
    s_debugConsole.ops.Receive(s_debugConsole.base, buffer, size);

    return size;
}

#elif (defined(__GNUC__))
#pragma weak _write
int _write (int handle, char *buffer, int size)
{
    if (buffer == 0)
    {
        /* return -1 if error */
        return -1;
    }

    /* This function only writes to "standard out" and "standard err",*/
    /* for all other file handles it returns failure.*/
    if ((handle != 1) && (handle != 2))
    {
        return -1;
    }

    /* Do nothing if the debug uart is not initialized.*/
    if (!s_debugConsole.inited)
    {
        return -1;
    }

    /* Send data.*/
    s_debugConsole.ops.Send(s_debugConsole.base, (uint8_t *)buffer, size);
    return size;
}

#pragma weak _read
int _read(int handle, char *buffer, int size)
{
    /* This function only reads from "standard in", for all other file*/
    /* handles it returns failure.*/
    if (handle != 0)
    {
        return -1;
    }

    /* Do nothing if the debug uart is not initialized.*/
    if (!s_debugConsole.inited)
    {
        return -1;
    }

    /* Receive data.*/
    s_debugConsole.ops.Receive(s_debugConsole.base, (uint8_t *)buffer, size);
    return size;
}
#elif defined(__CC_ARM) 
struct __FILE
{
    int handle;
    /* Whatever you require here. If the only file you are using is */
    /* standard output using printf() for debugging, no file handling */
    /* is required. */
};

/* FILE is typedef in stdio.h. */
#pragma weak __stdout
FILE __stdout;
FILE __stdin;

#pragma weak fputc
int fputc(int ch, FILE *f)
{
    /* Do nothing if the debug uart is not initialized.*/
    if (!s_debugConsole.inited)
    {
        return -1;
    }

    /* Send data.*/
    s_debugConsole.ops.Send(s_debugConsole.base, (const uint8_t*)&ch, 1);
    return 1;
}

#pragma weak fgetc
int fgetc(FILE *f)
{
    uint8_t temp;
    /* Do nothing if the debug uart is not initialized.*/
    if (!s_debugConsole.inited)
    {
        return -1;
    }

    /* Receive data.*/
    s_debugConsole.ops.Receive(s_debugConsole.base, &temp, 1);
    return temp;
}
#endif

/*************Code for debug_printf/scanf/assert*******************************/
int debug_printf(const char  *fmt_s, ...)
{
   va_list  ap;
   int  result;
   /* Do nothing if the debug uart is not initialized.*/
   if (!s_debugConsole.inited)
   {
       return -1;
   }

   va_start(ap, fmt_s);
   result = _doprint(NULL, debug_putc, -1, (char *)fmt_s, ap);
   va_end(ap);

   return result;
}

static int debug_putc(int ch, void* stream)
{
    const unsigned char c = (unsigned char) ch;
    /* Do nothing if the debug uart is not initialized.*/
    if (!s_debugConsole.inited)
    {
        return -1;
    }
    s_debugConsole.ops.Send(s_debugConsole.base, &c, 1);

    return 0;
}

int debug_putchar(int ch)
{
    /* Do nothing if the debug uart is not initialized.*/
    if (!s_debugConsole.inited)
    {
        return -1;
    }
    debug_putc(ch, NULL);

    return 1;
}

int debug_scanf(const char  *fmt_ptr, ...)
{
    char temp_buf[IO_MAXLINE];
    va_list ap;
    uint32_t i;
    char result;

    /* Do nothing if the debug uart is not initialized.*/
    if (!s_debugConsole.inited)
    {
        return -1;
    }
    va_start(ap, fmt_ptr);
    temp_buf[0] = '\0';

    for (i = 0; i < IO_MAXLINE; i++)
    {
        temp_buf[i] = result = debug_getchar();

        if ((result == '\r') || (result == '\n'))
        {
            /* End of Line */
            if (i == 0)
            {
                i = (uint32_t)-1;
            }
            else
            {
                break;
            }
        }

        temp_buf[i + 1] = '\0';
    }

    result = scan_prv(temp_buf, (char *)fmt_ptr, ap);
    va_end(ap);

    return result;
}

int debug_getchar(void)
{
    unsigned char c;

    /* Do nothing if the debug uart is not initialized.*/
    if (!s_debugConsole.inited)
    {
        return -1;
    }
    s_debugConsole.ops.Receive(s_debugConsole.base, &c, 1);

    return c;
}

void UART_SendDataPolling(void *base, const uint8_t *txBuff, uint32_t txSize)
{
    while (txSize--)
    {
        UART_Putchar((UART_Type*)base, *txBuff++);
        while (!UART_GetStatusFlag((UART_Type*)base, uartStatusTxEmpty));
        while (!UART_GetStatusFlag((UART_Type*)base, uartStatusTxComplete));
    }
}

void UART_ReceiveDataPolling(void *base, uint8_t *rxBuff, uint32_t rxSize)
{
    while (rxSize--)
    {
        while (!UART_GetStatusFlag((UART_Type*)base, uartStatusRxDataReady));

        *rxBuff = UART_Getchar((UART_Type*)base);
        rxBuff++;
    }
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
