/*
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ti_drivers_uart_RingBuf__include
#define ti_drivers_uart_RingBuf__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct RingBuf_Object {
    unsigned char      *buffer;
    size_t              length;
    size_t              count;
    size_t              head;
    size_t              tail;
    size_t              maxCount;
} RingBuf_Object, *RingBuf_Handle;

/*!
 *  @brief  Initialize circular buffer
 *
 *  @param  object  Pointer to a RingBuf Object that contains the member
 *                  variables to operate a circular buffer.
 *
 *  @param  bufPtr  Pointer to data buffer to be used for the circular buffer.
 *                  The buffer is NOT stored in RingBuf_Object.
 *
 *  @param  bufSize The size of bufPtr in number of unsigned chars.
 */
void RingBuf_construct(RingBuf_Handle object, unsigned char *bufPtr,
    size_t bufSize);

/*!
 *  @brief  Get an unsigned char from the end of the circular buffer and remove
 *          it.
 *
 *  @param  object  Pointer to a RingBuf Object that contains the member
 *                  variables to operate a circular buffer.
 *
 *  @param  data    Pointer to an unsigned char to be filled with the data from
 *                  the front of the circular buffer.
 *
 *  @return         Number of unsigned chars on the buffer after taking it out
 *                  of the circular buffer. If it returns -1, the circular
 *                  buffer was already empty and data is invalid.
 */
int RingBuf_get(RingBuf_Handle object, unsigned char *data);

/*!
 *  @brief  Get the number of unsigned chars currently stored on the circular
 *          buffer.
 *
 *  @param  object  Pointer to a RingBuf Object that contains the member
 *          variables to operate a circular buffer.
 *
 *  @return         Number of unsigned chars on the circular buffer.
 */
int RingBuf_getCount(RingBuf_Handle object);

/*!
 *  @brief  Function to determine if the circular buffer is full or not.
 *
 *  @param  object  Pointer to a RingBuf Object that contains the member
 *                  variables to operate a circular buffer.
 *
 *  @return         true if circular buffer is full, else false.
 */
bool RingBuf_isFull(RingBuf_Handle object);

/*!
 *  @brief  A high-water mark indicating the largest number of unsigned chars
 *          stored on the circular buffer since it was constructed.
 *
 *  @return         Get the largest number of unsigned chars that were at one
 *                  point in the circular buffer.
 */
int RingBuf_getMaxCount(RingBuf_Handle object);

/*!
 *  @brief  Get an unsigned char from the end of the circular buffer without
 *          removing it.
 *
 *  @param  object  Pointer to a RingBuf Object that contains the member
 *                  variables to operate a circular buffer.
 *
 *  @param  data    Pointer to an unsigned char to be filled with the data from
 *                  the front of the circular buffer. This function does not
 *                  remove the data from the circular buffer. Do not evaluate
 *                  data if the count returned is equal to 0.
 *
 *  @return         Number of unsigned chars on the circular buffer. If the
 *                  number != 0, then data will contain the unsigned char at the
 *                  end of the circular buffer.
 */
int RingBuf_peek(RingBuf_Handle object, unsigned char *data);

/*!
 *  @brief  Put an unsigned char into the end of the circular buffer.
 *
 *  @param  object  Pointer to a RingBuf Object that contains the member
 *                  variables to operate a circular buffer.
 *
 *  @param  data    unsigned char to be placed at the end of the circular
 *                  buffer.
 *
 *  @return         Number of unsigned chars on the buffer after it was added,
 *                  or -1 if it's already full.
 */
int RingBuf_put(RingBuf_Handle object, unsigned char data);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_uart_RingBuf__include */
