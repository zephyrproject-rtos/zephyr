/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2011 Altera Corporation, San Jose, California, USA.           *
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

/*
 * Altera HAL driver suitable for Nios II to drive the altera_avalon_sgdma
 * controller.
 *
 * The routines contained in this file are intended for developers who
 * wish to manage descriptors in their own application code. No facility
 * for automatic descriptor buffer management is provided. Use this API if
 * you want to:
 *   - Construct your own descriptors or descriptor chains in your
 *     application code
 *   - Use the SGDMA controller for simple "one off" transfers or for
 *     chains of transfers that you setup manually.
 *
 * Author: JRK
 */

#include "io.h"
#include "sys/alt_irq.h"
#include "sys/alt_errno.h"
#include "sys/alt_cache.h"

#include "altera_avalon_sgdma.h"
#include "altera_avalon_sgdma_regs.h"

/* The list of registered SGDMA components */
ALT_LLIST_HEAD(alt_sgdma_list);

/*
 * alt_avalon_sgdma_do_async_transfer
 *
 * Set up and commence a non-blocking transfer of one of more
 * descriptors (or descriptor chain).
 *
 * If the SGDMA controller is busy at the time of this call, the
 * routine will immediately reutrn -EBUSY; the application can then
 * decide how to proceed without being blocked.
 *
 * If a callback routine has been previously registered with this
 * particular SGDMA controller, the transfer will be set up to
 * issue an interrupt on error, EOP, or chain completion. Otherwise,
 * no interrupt is registered, and it is the responsibility of the
 * aplication developer to check for and suitably handle errors
 * and completion.
 *
 * Arguments:
 * - *dev: Pointer to SGDMA device (instance) struct.
 * - *desc: Pointer to single (ready to run) descriptor. The descriptor
 *   is expected to have its "next" descriptor field initialized either
 *   to a non-ready descriptor, or to the next in a chain.
 *
 * Returns:
 * - 0 for success, or various errors defined in <errno.h>
 */
int alt_avalon_sgdma_do_async_transfer(
  alt_sgdma_dev *dev,
  alt_sgdma_descriptor *desc)
{
  alt_u32 control;

  /* Return with error immediately if controller is busy */
  if( (IORD_ALTERA_AVALON_SGDMA_STATUS(dev->base) &
           ALTERA_AVALON_SGDMA_STATUS_BUSY_MSK) ) {
    return -EBUSY;
  }

  /* Clear Run */
  IOWR_ALTERA_AVALON_SGDMA_CONTROL(dev->base,
    (IORD_ALTERA_AVALON_SGDMA_CONTROL(dev->base) &
     ~ALTERA_AVALON_SGDMA_CONTROL_RUN_MSK) );

  /*
   * Clear any (previous) status register information
   * that might occlude our error checking later.
   */
  IOWR_ALTERA_AVALON_SGDMA_STATUS(dev->base, 0xFF);

  /* Point the controller at the descriptor */
  IOWR_ALTERA_AVALON_SGDMA_NEXT_DESC_POINTER(dev->base, (alt_u32) desc);

  /*
   * If a callback routine has been previously registered which will be
   * called from the SGDMA ISR. Set up controller to:
   *  - Run
   *  - Stop on an error with any particular descriptor
   *  - Include any control register bits registered with along with
   *    the callback routine (effectively, interrupts are controlled
   *    via the control bits set during callback-register time).
   */
  if(dev->callback) {
    control = IORD_ALTERA_AVALON_SGDMA_CONTROL(dev->base);

    control |= (dev->chain_control                          |
                ALTERA_AVALON_SGDMA_CONTROL_RUN_MSK         |
                ALTERA_AVALON_SGDMA_CONTROL_STOP_DMA_ER_MSK  );

    IOWR_ALTERA_AVALON_SGDMA_CONTROL(dev->base, control);
  }
  /*
   * No callback has been registered. Set up controller to:
   *   - Run
   *   - Stop on an error with any particular descriptor
   *   - Disable interrupt generation
   */
  else {
    control = IORD_ALTERA_AVALON_SGDMA_CONTROL(dev->base);

    control |= (ALTERA_AVALON_SGDMA_CONTROL_RUN_MSK         |
                ALTERA_AVALON_SGDMA_CONTROL_STOP_DMA_ER_MSK );
    control &= ~ALTERA_AVALON_SGDMA_CONTROL_IE_GLOBAL_MSK;

    IOWR_ALTERA_AVALON_SGDMA_CONTROL(dev->base, control);
  }

  /*
   * Error detection/handling should be performed at the application
   * or callback level as appropriate.
   */
  return 0;
}


/*
 * alt_avalon_sgdma_do_sync_transfer
 *
 * Send a fully formed descriptor (or list of descriptors) to SGDMA
 * for transfer. This routine will block both before transfer (if the
 * controller is busy), and until the requested transfer has completed.
 *
 * If an error is detected during the transfer it is abandoned and the
 * controller's status register contents are returned to the caller.
 *
 * Additional error information is available in the status bits of
 * each descriptor that the SGDMA processed; it is the responsibility
 * of the user's application to search through the descriptor (or list
 * of descriptors) to gather specific error information.
 *
 * Arguments:
 * - *dev: Pointer to SGDMA device (instance) struct.
 * - *desc: Pointer to single (ready to run) descriptor. The descriptor
 *   is expected to have its "next" descriptor field initialized either
 *   to a non-ready descriptor, or to the next in a chain.
 *
 * Returns:
 * - status: Content of SGDMA status register.
 */
alt_u8 alt_avalon_sgdma_do_sync_transfer(
  alt_sgdma_dev *dev,
  alt_sgdma_descriptor *desc)
{
  alt_u8 status;

  /* Wait for any pending transfers to complete */
  while ( (IORD_ALTERA_AVALON_SGDMA_STATUS(dev->base) &
           ALTERA_AVALON_SGDMA_STATUS_BUSY_MSK) );


  /* Clear Run */
  IOWR_ALTERA_AVALON_SGDMA_CONTROL(dev->base,
    (IORD_ALTERA_AVALON_SGDMA_CONTROL(dev->base) &
     ~ALTERA_AVALON_SGDMA_CONTROL_RUN_MSK) );

  /*
   * Clear any (previous) status register information
   * that might occlude our error checking later.
   */
  IOWR_ALTERA_AVALON_SGDMA_STATUS(dev->base, 0xFF);

  /* Point the controller at the descriptor */
  IOWR_ALTERA_AVALON_SGDMA_NEXT_DESC_POINTER(dev->base, (alt_u32) desc);

  /*
   * Set up SGDMA controller to:
   * - Disable interrupt generation
   * - Run once a valid descriptor is written to controller
   * - Stop on an error with any particular descriptor
   */
  IOWR_ALTERA_AVALON_SGDMA_CONTROL(dev->base,
    (ALTERA_AVALON_SGDMA_CONTROL_RUN_MSK |
     ALTERA_AVALON_SGDMA_CONTROL_STOP_DMA_ER_MSK | 
     IORD_ALTERA_AVALON_SGDMA_CONTROL(dev->base)) );

  /* Wait for the descriptor (chain) to complete */
  while ( (IORD_ALTERA_AVALON_SGDMA_STATUS(dev->base) &
           ALTERA_AVALON_SGDMA_STATUS_BUSY_MSK) );

  /* Clear Run */
  IOWR_ALTERA_AVALON_SGDMA_CONTROL(dev->base, 
    (IORD_ALTERA_AVALON_SGDMA_CONTROL(dev->base) &
     ~ALTERA_AVALON_SGDMA_CONTROL_RUN_MSK) );

  /* Get & clear status register contents */
  status = IORD_ALTERA_AVALON_SGDMA_STATUS(dev->base);
  IOWR_ALTERA_AVALON_SGDMA_STATUS(dev->base, 0xFF);

  return status;
}

/*
 * SGDMA Descriptor construction routines.
 *
 * General information:
 * These routines construct a single SGDMA descriptor in the memory
 * pointed to in alt_avalon_sgdma-descriptor *desc.
 *
 * The completed descriptor is made ready to run by setting its "Owned
 * by HW" bit; once the SGDMA controller receives the descriptor (and
 * its RUN bit is asserted), the descriptor will be processed.
 *
 * The descriptor under construction's "next" value is set to the "next"
 * descriptor passed to this routine, but the "next" descriptor's "Owned
 * by HW" bit is explicitly negated; once the SGDMA completes processing of
 * *desc it will not continue to *next until it, too, is made ready. You
 * can subsequently call the appropriate descriptor construction routine
 * on *next to make it ready, too.
 *
 * You are responsible for the creation of both the descriptor under
 * constuction as well as the "next" descriptor in the chain.
 *
 * Important: descriptors must be in a memory device mastered by the SGDMA
 * controller's "chain read" and "chain write" Avalon master-ports. Care must be
 * taken to ensure that both *desc and *next point to areas of memory mastered
 * by the controller.
 */

void alt_avalon_sgdma_construct_mem_to_mem_desc(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *read_addr,
  alt_u32              *write_addr,
  alt_u16               length,
  int                   read_fixed,
  int                   write_fixed)
{
  alt_avalon_sgdma_construct_mem_to_mem_desc_burst(desc, next, read_addr, 
    write_addr, length, read_fixed, write_fixed, 0, 0);
}
/*
 * alt_avalon_sgdma_construct_mem_to_mem_desc
 *
 * This routine constructs a single SGDMA descriptor in the memory
 * specified in alt_avalon_sgdma-descriptor *desc for an Avalon
 * memory-to-memory transfer.
 *
 * Arguments:
 * - *desc: Pointer to descriptor being constructed.
 * - *next: Pointer to "next" descriptor. This need not be a complete or
 *   functional descriptor, but must be properly allocated as described above.
 * - *read_addr: First read address for the DMA transfer
 * - *write_addr: First write address for the DMA transfer
 * - length: Number of bytes for this transfer
 * - read_fixed: If non-zero, DMA will read from a fixed address.
 * - write_fixed: If non-zer, DMA will write to a fixed address.
 */
void alt_avalon_sgdma_construct_mem_to_mem_desc_burst(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *read_addr,
  alt_u32              *write_addr,
  alt_u16               length,
  int                   read_fixed,
  int                   write_fixed,
  int                   read_burst,
  int                   write_burst)
{
  alt_avalon_sgdma_construct_descriptor_burst(
    desc,
    next,
    read_addr,
    write_addr,
    length,
    0,              // Generate EOP: N/A in mem-to-mem mode
    read_fixed,
    write_fixed,
    read_burst,
    write_burst,
    (alt_u8) 0x0);  // Atlantic channel: N/A in mem-to-mem mode
}

void alt_avalon_sgdma_construct_stream_to_mem_desc(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *write_addr,
  alt_u16               length_or_eop,
  int                   write_fixed)
{
  alt_avalon_sgdma_construct_stream_to_mem_desc_burst(desc, next, write_addr, 
    length_or_eop, write_fixed, 0);
}
/*
 * alt_avalon_sgdma_construct_stream_to_mem_desc
 *
 * This routine constructs a single SGDMA descriptor in the memory
 * specified in alt_avalon_sgdma-descriptor *desc) for an Avalon
 * Streaming interface-to-Avalon transfer.
 *
 * The source (read) data for the transfer comes from the Avalon
 * Streaming Interface connected to the SGDMA controller's streaming
 * read-port.
 *
 * Arguments:
 * - *desc: Pointer to descriptor being constructed.
 * - *next: Pointer to "next" descriptor. This need not be a complete or
 *   functional descriptor, but must be properly allocated as described above.
 * - *write_addr: First write address for the DMA transfer
 * - length_or_eop: Number of bytes for this transfer. If set to zero (0x0),
 *   the transfer will continue until an EOP signal is received from the Avalon
 *   Streaming interface
 * - write_fixed: If non-zero, SGDMA will write to a fixed address.
 */
void alt_avalon_sgdma_construct_stream_to_mem_desc_burst(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *write_addr,
  alt_u16               length_or_eop,
  int                   write_fixed,
  int                   write_burst)
{
  alt_avalon_sgdma_construct_descriptor_burst(
    desc,
    next,
    (alt_u32) 0x0,  // Read addr: N/A in stream-to-mem mode
    write_addr,
    length_or_eop,
    0x0,            // Generate EOP: N/A in stream-to-mem mode
    0x0,            // Read fixed: N/A in stream-to-mem mode
    write_fixed,
    0,              // Read_burst : N/A in stream-to-mem mode
    write_burst,
    (alt_u8) 0x0);  // Atlantic channel: N/A in stream-to-mem mode
}

void alt_avalon_sgdma_construct_mem_to_stream_desc(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *read_addr,
  alt_u16               length,
  int                   read_fixed,
  int                   generate_sop,
  int                   generate_eop,
  alt_u8                atlantic_channel)
{
  alt_avalon_sgdma_construct_mem_to_stream_desc_burst(desc, next, read_addr, 
    length, read_fixed, generate_sop, generate_eop, 0, atlantic_channel);

}
/*
 * alt_avalon_sgdma_construct_mem_to_stream_desc
 *
 * This routine constructs a single SGDMA descriptor in the memory
 * specified in alt_avalon_sgdma-descriptor *desc) for an Avalon to
 * Avalon Streaming interface transfer.
 *
 * The destination (write) data for the transfer goes to the Avalon
 * Streaming Interface connected to the SGDMA controller's streaming
 * write-port.
 *
 * Arguments:
 * - *desc: Pointer to descriptor being constructed.
 * - *next: Pointer to "next" descriptor. This need not be a complete or
 *   functional descriptor, but must be properly allocated as described above.
 * - *read_addr: First read address for the transfer
 * - length: Number of bytes for this transfer
 * - read_fixed: If non-zero, SGDMA will read from a fixed address.
 * - generate_sop: If non-zero, SGDMA will generate a start-of-packet (SOP)
 *   on the Avalon Streaming interface when commencing the transfer.
 * - generate_eop: If non-zero, SGDMA will generate an end-of-packet (EOP)
 *   on the Avalon Streaming interface when completing the transfer.
 * - atlantic_channel: 8-bit channel identification number that will be
 *   passed to the Avalon Streaming interface.
 */
void alt_avalon_sgdma_construct_mem_to_stream_desc_burst(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *read_addr,
  alt_u16               length,
  int                   read_fixed,
  int                   generate_sop,
  int                   generate_eop,
  int                   read_burst,
  alt_u8                atlantic_channel)
{
  alt_avalon_sgdma_construct_descriptor_burst(
    desc,
    next,
    read_addr,
    (alt_u32) 0x0,     // Write address N/A in mem-to-stream mode
    length,
    generate_eop,
    read_fixed,
    generate_sop,
    read_burst,
    0,                 // Write_burst : N/A in mem-to-stream mode
    atlantic_channel);
}

/*
 * alt_avalon_sgdma_register_callback
 *
 * Associate a user-specifiC routine with the SGDMA interrupt handler.
 * If a callback is registered, all non-blocking SGDMA transfers will
 * enable interrupts that will cause the callback to be executed.
 * The callback runs as part of the interrupt service routine, and
 * great care must be taken to follow the guidelines for acceptable
 * interrupt service routine behavior as described in the Nios II
 * Software Developer's Handbook.
 *
 * Note: To disable callbacks after registering one, this routine
 * may be called passing 0x0 to the callback argument.
 *
 * Arguments:
 * - *dev: Pointer to SGDMA device (instance) struct.
 * - callback: Pointer to callback routine to execute at interrupt level
 * - chain_control: SGDMA control register contents. This value will be
 *   OR'd together with control bits to:
 *     (1) Set the the SGDMA "run" bit, and
 *     (2) Stop SGDMA on any error
 *   in the SGDMA control register, when an asynchronous transfer is initiated.
 *   This allows you to control the conditions that will generate an interrupt
 *   from SGDMA. If you want interrupts to be generated (and thus call the
 *   callback routine), you MUST set one or more bits in the chain_control
 *   variable that correspond to SGDMA control register interrupt enable bits
 *   for interrupts to be generated, as well as the master inerrupt enable bit.
 */
void alt_avalon_sgdma_register_callback(
  alt_sgdma_dev *dev,
  alt_avalon_sgdma_callback callback,
  alt_u32 chain_control,
  void *context)
{
  dev->callback         = callback;
  dev->callback_context = context;
  dev->chain_control    = chain_control;
}


/*
 * alt_avalon_sgdma_start
 *
 * Start the DMA engine. The descriptor pointed to in the controller's
 * "Next descriptor pointer" and subsequent descriptor, will be processed.
 *
 * Arguments:
 * - *dev: Pointer to SGDMA device (instance) struct.
 */
void alt_avalon_sgdma_start(alt_sgdma_dev *dev)
{
  alt_u32 control;

  control = IORD_ALTERA_AVALON_SGDMA_CONTROL(dev->base);
  control |= ALTERA_AVALON_SGDMA_CONTROL_RUN_MSK;
  IOWR_ALTERA_AVALON_SGDMA_CONTROL(dev->base, control);
}

/*
 * alt_avalon_sgdma_stop
 *
 * Stop the DMA engine (following completion of the current buffer descriptor).
 *
 * Arguments:
 * - *dev: Pointer to SGDMA device (instance) struct
 */
void alt_avalon_sgdma_stop(alt_sgdma_dev *dev)
{
  alt_u32 control;

  control = IORD_ALTERA_AVALON_SGDMA_CONTROL(dev->base);
  control &= ~ALTERA_AVALON_SGDMA_CONTROL_RUN_MSK;
  IOWR_ALTERA_AVALON_SGDMA_CONTROL(dev->base, control);
}

/*
 * alt_avalon_sgdma_check_descriptor_status
 *
 * This routine will report:
 *  - Any errors reported by the SGDMA controller specific
 *    to a particular descriptor by retirning -EIO
 *  - The descriptor currently in use ("Owned by HW" bit set)
 *    by returning -EINPROGRESS.
 *
 * Arguments:
 * - *desc: Pointer to descriptor to examine
 *
 * Returns:
 * - 0 if the descriptor is error-free, not "owned by hardware", or
 *   a previously requested transfer has appeared to have completed
 *   normally. Or, various error conditions defined in <errno.h>
 */
int alt_avalon_sgdma_check_descriptor_status(alt_sgdma_descriptor *desc)
{
  /* Errors take precedence */
  if( IORD_8DIRECT(&desc->status, 0) &
      ( ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_CRC_MSK      |
        ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_PARITY_MSK   |
        ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_OVERFLOW_MSK |
        ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_SYNC_MSK     |
        ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_UEOP_MSK     |
        ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_MEOP_MSK     |
        ALTERA_AVALON_SGDMA_DESCRIPTOR_STATUS_E_MSOP_MSK ) ) {
    return -EIO;
  }

  if( IORD_8DIRECT(&desc->control, 0) &
      ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_OWNED_BY_HW_MSK) {
    return -EINPROGRESS;
  }

    return 0;
}

/*
 * alt_avalon_sgdma_open - Retrieve a pointer to the SGDMA
 *
 * Search the list of registered SGDMAs for one with the supplied name.
 *
 * The return value will be NULL on failure, and non-NULL otherwise.
 *
 * Arguments:
 * - *name: Character pointer to name of SGDMA peripheral as registered
 *   with the HAL. For example, an SGDMA controller named "my_sgdma"
 *   in SOPC Builder would be oped by asking for "/dev/my_sgdma".
 *
 * Returns:
 * - Pointer to SGDMA device instance struct, or null if the device
 *   could not be opened.
 */
alt_sgdma_dev* alt_avalon_sgdma_open (const char* name)
{
  alt_sgdma_dev* dev;

  dev = (alt_sgdma_dev*) alt_find_dev (name, &alt_sgdma_list);

  if (NULL == dev) {
    ALT_ERRNO = ENODEV;
  }

  return dev;
}


void alt_avalon_sgdma_construct_descriptor(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *read_addr,
  alt_u32              *write_addr,
  alt_u16               length_or_eop,
  int                   generate_eop,
  int                   read_fixed,
  int                   write_fixed_or_sop,
  alt_u8                atlantic_channel)
{
  alt_avalon_sgdma_construct_descriptor_burst(desc, next, read_addr, 
    write_addr, length_or_eop, generate_eop, read_fixed, write_fixed_or_sop, 
    0, 0, atlantic_channel);
}

/*
 * alt_avalon_sgdma_enable_desc_poll  
 *                                     
 * Enables descriptor polling mode with specified frequency. User needs to make 
 * sure this feature is supported by hardware. There is no effect if hardware 
 * does not support this feature.
 *
 * Arguments:
 * - *dev: Pointer to SGDMA device (instance) struct
 * - frequency: Frequency value to set. Only lower 11-bit value of frequency 
                 will write to control register.
 *
 * Returns:
 * - None
 */
void alt_avalon_sgdma_enable_desc_poll(alt_sgdma_dev *dev, alt_u32 frequency)
{
  alt_u32 control;

  control = IORD_ALTERA_AVALON_SGDMA_CONTROL(dev->base);
  /* Clear descriptor polling frequency */
  control &= ~ALTERA_AVALON_SGDMA_CONTROL_DESC_POLL_FREQ_MSK;
  
  control |= ALTERA_AVALON_SGDMA_CONTROL_DESC_POLL_EN_MSK | 
            ((frequency << ALTERA_AVALON_SGDMA_CONTROL_DESC_POLL_FREQ_OFST) & 
            ALTERA_AVALON_SGDMA_CONTROL_DESC_POLL_FREQ_MSK);
  IOWR_ALTERA_AVALON_SGDMA_CONTROL(dev->base, control);
  
  return;
}

/*
 * alt_avalon_sgdma_disable_desc_poll  
 *                                     
 * Disables descriptor polling mode.
 *
 * Arguments:
 * - *dev: Pointer to SGDMA device (instance) struct
 *
 * Returns:
 * - None
 */
void alt_avalon_sgdma_disable_desc_poll(alt_sgdma_dev *dev)
{
  alt_u32 control;

  control = IORD_ALTERA_AVALON_SGDMA_CONTROL(dev->base);
  control &= ~ALTERA_AVALON_SGDMA_CONTROL_DESC_POLL_EN_MSK;

  IOWR_ALTERA_AVALON_SGDMA_CONTROL(dev->base, control);
  
  return;
}

/*******************************************************************************
 * Altera HAL support & Private API
 ******************************************************************************/
/*
 * alt_avalon_sgdma_construct_descriptor
 *
 * This is a genertic routine that the SGDMA mode-specific routines
 * call to populate a descriptor.
 */
void alt_avalon_sgdma_construct_descriptor_burst(
  alt_sgdma_descriptor *desc,
  alt_sgdma_descriptor *next,
  alt_u32              *read_addr,
  alt_u32              *write_addr,
  alt_u16               length_or_eop,
  int                   generate_eop,
  int                   read_fixed,
  int                   write_fixed_or_sop,
  int                   read_burst,
  int                   write_burst,
  alt_u8                atlantic_channel)
{
  /*
   * Mark the "next" descriptor as "not" owned by hardware. This prevents
   * The SGDMA controller from continuing to process the chain. This is
   * done as a single IO write to bypass cache, without flushing
   * the entire descriptor, since only the 8-bit descriptor status must
   * be flushed.
   */
  IOWR_8DIRECT(&next->control, 0,
    (next->control & ~ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_OWNED_BY_HW_MSK));

  desc->read_addr                = read_addr;
  desc->write_addr               = write_addr;
  desc->next                     = (alt_u32 *) next;
  desc->read_addr_pad            = 0x0;
  desc->write_addr_pad           = 0x0;
  desc->next_pad                 = 0x0;
  desc->bytes_to_transfer        = length_or_eop;
  desc->actual_bytes_transferred = 0;
  desc->status                   = 0x0;

  /* SGDMA burst not currently supported */
  desc->read_burst               = read_burst;
  desc->write_burst              = write_burst;

  /*
   * Set the descriptor control block as follows:
   * - Set "owned by hardware" bit
   * - Optionally set "generte EOP" bit
   * - Optionally set the "read from fixed address" bit
   * - Optionally set the "write to fixed address bit (which serves
   *   serves as a "generate SOP" control bit in memory-to-stream mode).
   * - Set the 4-bit atlantic channel, if specified
   *
   * Note that this step is performed after all other descriptor information
   * has been filled out so that, if the controller already happens to be
   * pointing at this descriptor, it will not run (via the "owned by hardware"
   * bit) until all other descriptor information has been set up.
   */
  desc->control = (
    (ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_OWNED_BY_HW_MSK)                 |
    (generate_eop ?
       ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_GENERATE_EOP_MSK : 0x0)        |
    (read_fixed ?
       ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_READ_FIXED_ADDRESS_MSK : 0x0)  |
    (write_fixed_or_sop ?
       ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_WRITE_FIXED_ADDRESS_MSK : 0x0) |
    (atlantic_channel ? ( (atlantic_channel & 0x0F) << 3) : 0)
  );

  /*
   * Flush completed buffer out of cache. This is done rather than
   * individual cache-bypassed writes to take advantage of any
   * burst-capabilities in the memory we're writing to.
   */
  alt_dcache_flush(desc, sizeof(alt_sgdma_descriptor));
}

/*
 * alt_avalon_sgdma_irq()
 *
 * Interrupt handler for the Scatter-Gather DMA controller.
 */
#ifdef ALT_ENHANCED_INTERRUPT_API_PRESENT
static void alt_avalon_sgdma_irq(void *context)
#else
static void alt_avalon_sgdma_irq(void *context, alt_u32 id)
#endif
{
  alt_sgdma_dev *dev = (alt_sgdma_dev *) context;
  alt_irq_context cpu_sr;

  /*
   * Clear the pending interrupt request from the SGDMA controller.
   * Writing 1 to bit-31 of the control register clears the interrupt.
   * Note: This is explicitly done before calling user interrupt-handling
   * code rather than after; if user ISR code initiates another SGDMA
   * transfer which completes quickly, reading the control register after
   * the callback routine may result in a lost interrupt.
   */
  IOWR_ALTERA_AVALON_SGDMA_CONTROL(dev->base, 
    IORD_ALTERA_AVALON_SGDMA_CONTROL(dev->base) | 0x80000000);
  
  /* Dummy read to ensure IRQ is negated before the ISR returns */
  IORD_ALTERA_AVALON_SGDMA_CONTROL(dev->base);
  
  /* 
   * Other interrupts are explicitly disabled if callbacks
   * are registered because there is no guarantee that they are 
   * preemption-safe. This allows the driver to support 
   * interrupt preemption.
   */
  if(dev->callback) {
    cpu_sr = alt_irq_disable_all();
    (dev->callback)(dev->callback_context);
    alt_irq_enable_all(cpu_sr);
  }
}

/*
 * alt_avalon_sgdma_init()
 *
 * Initializes the Scatter-Gather DMA controller. This routine is called
 * from the ALTERA_AVALON_SGDMA_INIT macro and is called automatically
 * by alt_sys_init.c
 *
 * This routine disables interrupts, future descriptor processing,
 * registers a specific instance of the device with the HAL,
 * and installs an interrupt handler for the device.
 */
void alt_avalon_sgdma_init (alt_sgdma_dev *dev, alt_u32 ic_id, alt_u32 irq)
{
  extern alt_llist alt_sgdma_list;

  /* 
   * Halt any current transactions (reset the device)
   * SW reset is written twice per SGDMA documentation 
   */
  IOWR_ALTERA_AVALON_SGDMA_CONTROL(dev->base,
    ALTERA_AVALON_SGDMA_CONTROL_SOFTWARERESET_MSK);
  IOWR_ALTERA_AVALON_SGDMA_CONTROL(dev->base,
    ALTERA_AVALON_SGDMA_CONTROL_SOFTWARERESET_MSK);

  /*
   * Disable interrupts, halt future descriptor processing,
   * and clear status register content
   */
  IOWR_ALTERA_AVALON_SGDMA_CONTROL(dev->base, 0x0);
  IOWR_ALTERA_AVALON_SGDMA_STATUS(dev->base, 0xFF);

  /* Register this instance of the SGDMA controller with HAL */
  alt_dev_llist_insert((alt_dev_llist*) dev, &alt_sgdma_list);

  /* Install IRQ handler */
#ifdef ALT_ENHANCED_INTERRUPT_API_PRESENT
  alt_ic_isr_register(ic_id, irq, alt_avalon_sgdma_irq, dev, 0x0);
#else
  alt_irq_register(irq, dev, alt_avalon_sgdma_irq);
#endif  
}
