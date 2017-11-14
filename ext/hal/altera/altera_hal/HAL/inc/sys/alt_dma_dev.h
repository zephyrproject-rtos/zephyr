#ifndef __ALT_DMA_DEV_H__
#define __ALT_DMA_DEV_H__

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

#include "priv/alt_dev_llist.h"

#include "alt_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * This header contains the device driver interface for accessing DMA 
 * resources. See alt_dma.h for the DMA application side interface.
 *
 * The interface model treats a DMA transaction as being composed of two 
 * halves (read and write). 
 *
 * An "alt_dma_txchan_dev" is used to describe the device associated with a
 * DMA transmit channel. An "alt_dma_rxchan_dev" is used to describe the
 * device associated with a DMA receive channel. 
 */

/*
 * List of generic ioctl requests that may be supported by a DMA device.
 *
 * ALT_DMA_RX_ONLY_ON:    This causes a DMA channel to operate in a mode
 *                        where only the receiver is under software control.
 *                        The other side reads continously from a single
 *                        location. The address to read is the argument to
 *                        this request.
 * ALT_DMA_RX_ONLY_OFF:   Return to the default mode where both the receive
 *                        and transmit sides of the DMA can be under software
 *                        control.
 * ALT_DMA_TX_ONLY_ON:    This causes a DMA channel to operate in a mode
 *                        where only the transmitter is under software control.
 *                        The other side writes continously to a single
 *                        location. The address to write to is the argument to
 *                        this request.
 * ALT_DMA_TX_ONLY_OFF:   Return to the default mode where both the receive
 *                        and transmit sides of the DMA can be under software
 *                        control.
 * ALT_DMA_SET_MODE_8:    Transfer data in units of 8 bits.
 * ALT_DMA_SET_MODE_16:   Transfer data in units of 16 bits.
 * ALT_DMA_SET_MODE_32:   Transfer data in units of 32 bits.
 * ALT_DMA_SET_MODE_64:   Transfer data in units of 64 bits.
 * ALT_DMA_SET_MODE_128:  Transfer data in units of 128 bits.
 * ALT_DMA_GET_MODE:      Get the current transfer mode.
 *
 * The use of the macros: ALT_DMA_TX_STREAM_ON, ALT_DMA_TX_STREAM_OFF
 * ALT_DMA_RX_STREAM_OFF and ALT_DMA_RX_STREAM_ON are depreciated. You should
 * instead use the macros: ALT_DMA_RX_ONLY_ON, ALT_DMA_RX_ONLY_OFF, 
 * ALT_DMA_TX_ONLY_ON and ALT_DMA_TX_ONLY_OFF.
 */

#define ALT_DMA_TX_STREAM_ON  (0x1)
#define ALT_DMA_TX_STREAM_OFF (0x2)
#define ALT_DMA_RX_STREAM_ON  (0x3)
#define ALT_DMA_RX_STREAM_OFF (0x4)
#define ALT_DMA_SET_MODE_8    (0x5)
#define ALT_DMA_SET_MODE_16   (0x6)
#define ALT_DMA_SET_MODE_32   (0x7)
#define ALT_DMA_SET_MODE_64   (0x8)
#define ALT_DMA_SET_MODE_128  (0x9)
#define ALT_DMA_GET_MODE      (0xa)

#define ALT_DMA_RX_ONLY_ON    ALT_DMA_TX_STREAM_ON
#define ALT_DMA_RX_ONLY_OFF   ALT_DMA_TX_STREAM_OFF
#define ALT_DMA_TX_ONLY_ON    ALT_DMA_RX_STREAM_ON
#define ALT_DMA_TX_ONLY_OFF   ALT_DMA_RX_STREAM_OFF

/*
 *
 */

typedef struct alt_dma_txchan_dev_s alt_dma_txchan_dev;
typedef struct alt_dma_rxchan_dev_s alt_dma_rxchan_dev;

typedef alt_dma_txchan_dev* alt_dma_txchan;
typedef alt_dma_rxchan_dev* alt_dma_rxchan;

typedef void (alt_txchan_done)(void* handle);
typedef void (alt_rxchan_done)(void* handle, void* data);

/*
 * devices that provide a DMA transmit channel are required to provide an
 * instance of the "alt_dma_txchan_dev" structure. 
 */

struct alt_dma_txchan_dev_s {
  alt_llist  llist;                  /* for internal use */
  const char* name;                  /* name of the device instance 
                                      * (e.g. "/dev/dma_0"). 
                                      */
  int (*space) (alt_dma_txchan dma); /* returns the maximum number of 
                                      * transmit requests that can be posted
              */
  int (*dma_send) (alt_dma_txchan dma,   
         const void* from, 
         alt_u32 len,
         alt_txchan_done* done, 
         void* handle);        /* post a transmit request */
  int (*ioctl) (alt_dma_txchan dma, int req, void* arg); /* perform device
              * specific I/O control.
                                      */ 
};

/*
 * devices that provide a DMA receive channel are required to provide an
 * instance of the "alt_dma_rxchan_dev" structure. 
 */

struct alt_dma_rxchan_dev_s {
  alt_llist list;                    /* for internal use */
  const char* name;                  /* name of the device instance 
                                      * (e.g. "/dev/dma_0"). 
                                      */
  alt_u32        depth;              /* maximum number of receive requests that
                                      * can be posted.
                                      */
  int (*prepare) (alt_dma_rxchan   dma, 
                  void*            data,
                  alt_u32          len,
                  alt_rxchan_done* done,  
                  void*            handle); /* post a receive request */
  int (*ioctl) (alt_dma_rxchan dma, int req, void* arg);  /* perform device
              * specific I/O control.
                                      */ 
};

/*
 * Register a DMA transmit channel with the system.
 */

static ALT_INLINE int alt_dma_txchan_reg (alt_dma_txchan_dev* dev)
{
  extern alt_llist alt_dma_txchan_list;

  return alt_dev_llist_insert((alt_dev_llist*) dev, &alt_dma_txchan_list);
}

/*
 * Register a DMA receive channel with the system.
 */

static ALT_INLINE int alt_dma_rxchan_reg (alt_dma_rxchan_dev* dev)
{
  extern alt_llist alt_dma_rxchan_list;

  return alt_dev_llist_insert((alt_dev_llist*) dev, &alt_dma_rxchan_list);
}

/*
 *
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ALT_DMA_DEV_H__ */
