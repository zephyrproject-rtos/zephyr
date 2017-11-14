#ifndef __ALT_DMA_H__
#define __ALT_DMA_H__

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2004-2005 Altera Corporation, San Jose, California, USA.      *
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
* Altera does not recommend, suggest or require that this reference design    *
* file be used in conjunction or combination with any other product.          *
******************************************************************************/

/******************************************************************************
*                                                                             *
* THIS IS A LIBRARY READ-ONLY SOURCE FILE. DO NOT EDIT.                       *
*                                                                             *
******************************************************************************/

#include "sys/alt_dma_dev.h"
#include "alt_types.h"

#include <errno.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * This header contains the application side interface for accessing DMA 
 * resources. See alt_dma_dev.h for the dma device driver interface.
 *
 * The interface model treats a DMA transaction as being composed of two 
 * halves (read and write). 
 *
 * The application can supply data for transmit using an "alt_dma_txchan"
 * descriptor. Alternatively an "alt_dma_rxchan" descriptor can be used to
 * receive data.
 */

/*
 * alt_dma_txchan_open() is used to obtain an "alt_dma_txchan" descriptor for
 * a DMA transmit device. The name is the name of the associated physical
 * device (e.g. "/dev/dma_0").
 *
 * The return value will be NULL on failure, and non-NULL otherwise. 
 */

extern alt_dma_txchan alt_dma_txchan_open (const char* name);

/*
 * alt_dma_txchan_close() is provided so that an application can notify the 
 * system that it has finished with a given DMA transmit channel. This is only
 * provided for completness.
 */

static ALT_INLINE int alt_dma_txchan_close (alt_dma_txchan dma)
{
  return 0;
}

/*
 * alt_dma_txchan_send() posts a transmit request to a DMA transmit channel.
 * The input arguments are:
 *
 * dma: the channel to use.
 * from: a pointer to the start of the data to send.
 * length: the length of the data to send in bytes.
 * done: callback function that will be called once the data has been sent.
 * handle: opaque value passed to "done".
 *
 * The return value will be negative if the request cannot be posted, and
 * zero otherwise.
 */

static ALT_INLINE int alt_dma_txchan_send (alt_dma_txchan dma, 
             const void* from, 
             alt_u32 length,
             alt_txchan_done* done, 
             void* handle)
{
  return dma ? dma->dma_send (dma, 
        from, 
        length,
        done, 
        handle) : -ENODEV;
}

/*
 * alt_dma_txchan_space() returns the number of tranmit requests that can be 
 * posted to the specified DMA transmit channel.
 *
 * A negative value indicates that the value could not be determined. 
 */

static ALT_INLINE int alt_dma_txchan_space (alt_dma_txchan dma)
{
  return dma ? dma->space (dma) : -ENODEV;
}

/*
 * alt_dma_txchan_ioctl() can be used to perform device specific I/O 
 * operations on the indicated DMA transmit channel. For example some drivers
 * support options to control the width of the transfer operations. See
 * alt_dma_dev.h for the list of generic requests.
 *
 * A negative return value indicates failure, otherwise the interpretation
 * of the return value is request specific.  
 */

static ALT_INLINE int alt_dma_txchan_ioctl (alt_dma_txchan dma, 
              int            req, 
              void*          arg)
{
  return dma ? dma->ioctl (dma, req, arg) : -ENODEV;
}

/*
 * alt_dma_rxchan_open() is used to obtain an "alt_dma_rxchan" descriptor for
 * a DMA receive channel. The name is the name of the associated physical
 * device (e.g. "/dev/dma_0").
 *
 * The return value will be NULL on failure, and non-NULL otherwise. 
 */

extern alt_dma_rxchan alt_dma_rxchan_open (const char* dev);

/*
 * alt_dma_rxchan_close() is provided so that an application can notify the 
 * system that it has finished with a given DMA receive channel. This is only
 * provided for completness.
 */

static ALT_INLINE int alt_dma_rxchan_close (alt_dma_rxchan dma)
{
  return 0;
}

/*
 *
 */

/*
 * alt_dma_rxchan_prepare() posts a receive request to a DMA receive channel.
 * 
 * The input arguments are:
 *
 * dma: the channel to use.
 * data: a pointer to the location that data is to be received to.
 * len: the maximum length of the data to receive. 
 * done: callback function that will be called once the data has been 
 *       received.
 * handle: opaque value passed to "done".
 *
 * The return value will be negative if the request cannot be posted, and
 * zero otherwise.
 */

static ALT_INLINE int alt_dma_rxchan_prepare (alt_dma_rxchan   dma, 
                                              void*            data,
                                              alt_u32          len,
                                              alt_rxchan_done* done,  
                                              void*            handle)
{
  return dma ? dma->prepare (dma, data, len, done, handle) : -ENODEV;
}

/*
 * alt_dma_rxchan_ioctl() can be used to perform device specific I/O 
 * operations on the indicated DMA receive channel. For example some drivers
 * support options to control the width of the transfer operations. See
 * alt_dma_dev.h for the list of generic requests.
 *
 * A negative return value indicates failure, otherwise the interpretation
 * of the return value is request specific.  
 */

static ALT_INLINE int alt_dma_rxchan_ioctl (alt_dma_rxchan dma, 
              int            req, 
              void*          arg)
{
  return dma ? dma->ioctl (dma, req, arg) : -ENODEV;
}

/*
 * alt_dma_rxchan_depth() returns the depth of the receive FIFO used to store
 * receive requests.
 */

static ALT_INLINE alt_u32 alt_dma_rxchan_depth(alt_dma_rxchan dma)
{
  return dma->depth;
}

/*
 *
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ALT_DMA_H__ */
