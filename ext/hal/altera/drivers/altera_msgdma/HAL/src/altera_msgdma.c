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
*                                                                             *
******************************************************************************/
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "altera_msgdma_descriptor_regs.h"
#include "altera_msgdma_csr_regs.h"
#include "altera_msgdma_response_regs.h"
#include "io.h"
#include "altera_msgdma.h"
#include "priv/alt_busy_sleep.h"
#include "sys/alt_errno.h"
#include "sys/alt_irq.h"
#include "sys/alt_stdio.h"




/*******************************************************************************
 *  Private API
 ******************************************************************************/
static int alt_msgdma_write_standard_descriptor (
	alt_u32 *csr_base,
	alt_u32 *descriptor_base,
	alt_msgdma_standard_descriptor *descriptor);
static int alt_msgdma_write_extended_descriptor (
	alt_u32 *csr_base, 
	alt_u32 *descriptor_base,
	alt_msgdma_extended_descriptor *descriptor);
static void alt_msgdma_irq(void *context);
static int alt_msgdma_construct_standard_descriptor(
    	alt_msgdma_dev *dev,
	alt_msgdma_standard_descriptor *descriptor,
	alt_u32 *read_address, 
	alt_u32 *write_address,
	alt_u32 length, 
	alt_u32 control);
static int alt_msgdma_construct_extended_descriptor(
    	alt_msgdma_dev *dev,
	alt_msgdma_extended_descriptor *descriptor,
	alt_u32 *read_address, 
	alt_u32 *write_address, 
	alt_u32 length, 
	alt_u32 control,
	alt_u16 sequence_number, 
	alt_u8 read_burst_count, 
	alt_u8 write_burst_count,
	alt_u16 read_stride, 
	alt_u16 write_stride);
static int alt_msgdma_descriptor_async_transfer (
	alt_msgdma_dev *dev, 
	alt_msgdma_standard_descriptor *standard_desc, 
	alt_msgdma_extended_descriptor *extended_desc);
static int alt_msgdma_descriptor_sync_transfer (
	alt_msgdma_dev *dev, 
	alt_msgdma_standard_descriptor *standard_desc, 
	alt_msgdma_extended_descriptor *extended_desc);
/* The list of registered msgdma components */
ALT_LLIST_HEAD(alt_msgdma_list);

/* 
 * Functions for writing descriptor structure to the dispatcher.  If you disable 
 * some of the extended features in the hardware then you should pass in 0 for 
 * that particular descriptor element. These disabled elements will not be 
 * buffered by the dispatcher block.
 * 
 * This function is non-blocking and will return an error code if there is no 
 * room to write another descriptor to the dispatcher. It is recommended to call
 * 'read_descriptor_buffer_full' and make sure it returns '0' before calling 
 * this function.
 */
static int alt_msgdma_write_standard_descriptor (
	alt_u32 *csr_base, 
	alt_u32 *descriptor_base,
	alt_msgdma_standard_descriptor *descriptor)
{
    if (0 != (IORD_ALTERA_MSGDMA_CSR_STATUS(csr_base) & 
    	ALTERA_MSGDMA_CSR_DESCRIPTOR_BUFFER_FULL_MASK))
    {
      /*at least one descriptor buffer is full, returning so that this function 
		is non-blocking*/
        return -ENOSPC;
    }

	IOWR_ALTERA_MSGDMA_DESCRIPTOR_READ_ADDRESS(descriptor_base, 
		(alt_u32)descriptor->read_address);
	IOWR_ALTERA_MSGDMA_DESCRIPTOR_WRITE_ADDRESS(descriptor_base, 
	(	alt_u32)descriptor->write_address);
	IOWR_ALTERA_MSGDMA_DESCRIPTOR_LENGTH(descriptor_base, 
		descriptor->transfer_length);
	IOWR_ALTERA_MSGDMA_DESCRIPTOR_CONTROL_STANDARD(descriptor_base, 
		descriptor->control);
        return 0;
}

/*
 * This function is used for writing extended descriptors to the dispatcher.  
   It handles only 32-bit descriptors.
 */
static int alt_msgdma_write_extended_descriptor (
	alt_u32 *csr_base, 
	alt_u32 *descriptor_base,
	alt_msgdma_extended_descriptor *descriptor)
{
    if (0 != (IORD_ALTERA_MSGDMA_CSR_STATUS(csr_base) & 
    	ALTERA_MSGDMA_CSR_DESCRIPTOR_BUFFER_FULL_MASK))
    {
      /*at least one descriptor buffer is full, returning so that this function 
	is non-blocking*/
        return -ENOSPC;
    }

    IOWR_ALTERA_MSGDMA_DESCRIPTOR_READ_ADDRESS(
		descriptor_base, 
    	(alt_u32)descriptor->read_address_low);
    IOWR_ALTERA_MSGDMA_DESCRIPTOR_WRITE_ADDRESS(
		descriptor_base, 
		(alt_u32)descriptor->write_address_low);
    IOWR_ALTERA_MSGDMA_DESCRIPTOR_LENGTH(
		descriptor_base, 
    	descriptor->transfer_length);
    IOWR_ALTERA_MSGDMA_DESCRIPTOR_SEQUENCE_NUMBER(
		descriptor_base, 
    	descriptor->sequence_number);
    IOWR_ALTERA_MSGDMA_DESCRIPTOR_READ_BURST(
		descriptor_base, 
    	descriptor->read_burst_count);
    IOWR_ALTERA_MSGDMA_DESCRIPTOR_WRITE_BURST(
		descriptor_base, 
    	descriptor->write_burst_count);
    IOWR_ALTERA_MSGDMA_DESCRIPTOR_READ_STRIDE(
		descriptor_base, 
    	descriptor->read_stride);
    IOWR_ALTERA_MSGDMA_DESCRIPTOR_WRITE_STRIDE(
		descriptor_base, 
    	descriptor->write_stride);
    IOWR_ALTERA_MSGDMA_DESCRIPTOR_READ_ADDRESS_HIGH(descriptor_base, 0);
    IOWR_ALTERA_MSGDMA_DESCRIPTOR_WRITE_ADDRESS_HIGH(descriptor_base, 0);
    IOWR_ALTERA_MSGDMA_DESCRIPTOR_CONTROL_ENHANCED(
		descriptor_base, 
    	descriptor->control);
    return 0;
}

/*
 * alt_msgdma_irq()
 *
 * Interrupt handler for the Modular Scatter-Gather DMA controller.
 */
static void alt_msgdma_irq(void *context)
{
    alt_msgdma_dev *dev = (alt_msgdma_dev *) context;
    alt_irq_context cpu_sr;
    alt_u32 temporary_control;

    
    /* disable global interrupt*/
    if (dev->prefetcher_enable)
    {
        temporary_control = 
        		IORD_ALT_MSGDMA_PREFETCHER_CONTROL(dev->prefetcher_base)
				& ALT_MSGDMA_PREFETCHER_CTRL_GLOBAL_INTR_EN_CLR_MASK;
        
        IOWR_ALT_MSGDMA_PREFETCHER_CONTROL(dev->prefetcher_base,
        		temporary_control);
        
        /* clear the IRQ status- W1C */
        IOWR_ALT_MSGDMA_PREFETCHER_STATUS(dev->prefetcher_base,
        		ALT_MSGDMA_PREFETCHER_STATUS_IRQ_SET_MASK);
    }
    else
    {
    	temporary_control = IORD_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base)
    			& (~ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK);
    	
    	IOWR_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base, temporary_control);
    	/* clear the IRQ status */
    	IOWR_ALTERA_MSGDMA_CSR_STATUS(dev->csr_base,
    			ALTERA_MSGDMA_CSR_IRQ_SET_MASK);
    }

    /* 
    * Other interrupts are explicitly disabled if callbacks
    * are registered because there is no guarantee that they are 
    * pre-emption-safe. This allows the driver to support 
    * interrupt pre-emption.
    */
    if(dev->callback) 
    {
        cpu_sr = alt_irq_disable_all();
        dev->callback (dev->callback_context);
        alt_irq_enable_all(cpu_sr);
    }

    /* enable global interrupt */
    if (dev->prefetcher_enable)
    {
    	temporary_control = 
    			IORD_ALT_MSGDMA_PREFETCHER_CONTROL(dev->prefetcher_base)
				| ALT_MSGDMA_PREFETCHER_CTRL_GLOBAL_INTR_EN_SET_MASK;
    	
    	IOWR_ALT_MSGDMA_PREFETCHER_CONTROL(dev->prefetcher_base,
    			temporary_control);
    }
    else
    {
    	temporary_control = IORD_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base) 
    			| (ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK);
    	
    	IOWR_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base, temporary_control);
    }

    return;
}
/*
 * Helper functions for constructing mm_to_st, st_to_mm, mm_to_mm standard 
 * descriptors. Unnecessary elements are set to 0 for completeness and will be 
 * ignored by the hardware.
 * Returns:
 * - status: return 0 (success)
 *           return -EINVAL (invalid argument, could be due to argument which 
 *                          has larger value than hardware setting value)
 */
static int alt_msgdma_construct_standard_descriptor(
    	alt_msgdma_dev *dev,
	alt_msgdma_standard_descriptor *descriptor,
	alt_u32 *read_address, 
	alt_u32 *write_address, 
	alt_u32 length, 
	alt_u32 control)
{
    if(dev->max_byte < length ||
       dev->enhanced_features != 0
      )
    {
        return -EINVAL;
    }
    descriptor->read_address = read_address;
    descriptor->write_address = write_address;
    descriptor->transfer_length = length;
    descriptor->control = control | ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GO_MASK;
    
    return 0;
}

/*
 * Helper functions for constructing mm_to_st, st_to_mm, mm_to_mm extended 
 * descriptors. Unnecessary elements are set to 0 for completeness and will be 
 * ignored by the hardware.
 * Returns:
 * - status: return 0 (success)
 *           return -EINVAL (invalid argument, could be due to argument which 
 *                          has larger value than hardware setting value)
 */
static int alt_msgdma_construct_extended_descriptor(
    	alt_msgdma_dev *dev,
	alt_msgdma_extended_descriptor *descriptor,
	alt_u32 *read_address, 
	alt_u32 *write_address, 
	alt_u32 length, 
	alt_u32 control,
	alt_u16 sequence_number, 
	alt_u8 read_burst_count, 
	alt_u8 write_burst_count,
	alt_u16 read_stride, 
	alt_u16 write_stride)
{
    if(dev->max_byte < length ||
       dev->max_stride < read_stride ||
       dev->max_stride < write_stride ||
       dev->enhanced_features != 1 
      )
    {
        return -EINVAL;
    }
    
    descriptor->read_address_low = read_address;
    descriptor->write_address_low = write_address;
    descriptor->transfer_length = length;
    descriptor->sequence_number = sequence_number;
    descriptor->read_burst_count = read_burst_count;
    descriptor->write_burst_count = write_burst_count;
    descriptor->read_stride = read_stride;
    descriptor->write_stride = write_stride;
    descriptor->read_address_high = NULL;
    descriptor->write_address_high = NULL;
    descriptor->control = control | ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GO_MASK;

  return 0 ;

}

/*
 * Helper functions for descriptor in async transfer.
 * Arguments:# This driver supports HAL types
 * - *dev: Pointer to msgdma device (instance) structure.
 * - *standard_desc: Pointer to single standard descriptor.
 * - *extended_desc: Pointer to single extended descriptor.
 *
 *note: Either one of both *standard_desc and *extended_desc must 
 *      be assigned with NULL, another with proper pointer value.
 *      Failing to do so can cause the function return with "-EPERM "
 *
 * If a callback routine has been previously registered with this
 * particular msgdma controller, transfer will be set up to enable interrupt 
 * generation. It is the responsibility of the application developer to check 
 * source interruption, status completion and creating suitable interrupt 
 * handling. Note: "stop on error" of CSR control register is always masking 
 * within this function. The CSR control can be set by user through calling 
 * "alt_register_callback" by passing user used defined control setting.
 *
 * Returns:
 * 0 -> success
 * -ENOSPC -> FIFO descriptor buffer is full
 * -EPERM -> operation not permitted due to descriptor type conflict
 * -ETIME -> Time out and skipping the looping after 5 msec.
 */
static int alt_msgdma_descriptor_async_transfer (
	alt_msgdma_dev *dev, 
	alt_msgdma_standard_descriptor *standard_desc, 
	alt_msgdma_extended_descriptor *extended_desc)
{
    alt_u32 control = 0;
    alt_irq_context context = 0;
    alt_u16 counter = 0;
    alt_u32 fifo_read_fill_level = (
		IORD_ALTERA_MSGDMA_CSR_DESCRIPTOR_FILL_LEVEL(dev->csr_base) & 
		ALTERA_MSGDMA_CSR_READ_FILL_LEVEL_MASK) >> 
		ALTERA_MSGDMA_CSR_READ_FILL_LEVEL_OFFSET;
    alt_u32 fifo_write_fill_level = (
		IORD_ALTERA_MSGDMA_CSR_DESCRIPTOR_FILL_LEVEL(dev->csr_base) & 
		ALTERA_MSGDMA_CSR_WRITE_FILL_LEVEL_MASK) >> 
		ALTERA_MSGDMA_CSR_WRITE_FILL_LEVEL_OFFSET;

	/* Return with error immediately if one of read/write buffer is full */
	if((dev->descriptor_fifo_depth <= fifo_write_fill_level) || 
		(dev->descriptor_fifo_depth <= fifo_read_fill_level))
	{
		/*at least one write or read FIFO descriptor buffer is full,
		returning so that this function is non-blocking*/
		return -ENOSPC;
	}
	
	/*
	* When running in a multi threaded environment, obtain the "regs_lock"
	* semaphore. This ensures that accessing registers is thread-safe.
	*/
	ALT_SEM_PEND (dev->regs_lock, 0);
	
	/* Stop the msgdma dispatcher from issuing more descriptors to the
	read or write masters  */
	/* stop issuing more descriptors */
	control = ALTERA_MSGDMA_CSR_STOP_DESCRIPTORS_MASK;
	/* making sure the read-modify-write below can't be pre-empted */
	context = alt_irq_disable_all();
	IOWR_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base, control);
	/*
	* Clear any (previous) status register information
	* that might occlude our error checking later.
	*/
	IOWR_ALTERA_MSGDMA_CSR_STATUS(
		dev->csr_base, 
		IORD_ALTERA_MSGDMA_CSR_STATUS(dev->csr_base));
	alt_irq_enable_all(context);

    if (NULL != standard_desc && NULL == extended_desc)
    {
        /*writing descriptor structure to the dispatcher, wait until descriptor 
	  write is succeed*/
        while(0 != alt_msgdma_write_standard_descriptor (
		dev->csr_base, dev->descriptor_base, standard_desc))
        {
            alt_busy_sleep(1); /* delay 1us */
            if(5000 <= counter) /* time_out if waiting longer than 5 msec */
            {
                alt_printf("time out after 5 msec while waiting" 
					" free FIFO buffer for storing standard descriptor\n");
				
				/*
				* Now that access to the registers is complete, release the 
				* registers semaphore so that other threads can access the 
				* registers.
				*/
				ALT_SEM_POST (dev->regs_lock);
				
                return -ETIME;
            }
            counter++; 
        }
    }
    else if (NULL == standard_desc && NULL != extended_desc)
    {
        counter = 0; /* reset counter */
        /*writing descriptor structure to the dispatcher, wait until descriptor 
	  write is succeed*/
        while(0 != alt_msgdma_write_extended_descriptor (
		dev->csr_base, 
		dev->descriptor_base, 
		extended_desc))
        {
            alt_busy_sleep(1); /* delay 1us */
            if(5000 <= counter) /* time_out if waiting longer than 5 msec */
            {
                alt_printf("time out after 5 msec while waiting free FIFO buffer" 
					" for storing extended descriptor\n");
				/*
				* Now that access to the registers is complete, release the 
				* registers semaphore so that other threads can access the 
				* registers.
				*/
				ALT_SEM_POST (dev->regs_lock);
				
                return -ETIME;
            }
            counter++; 
        }
    }
    else
    {
		/*
		* Now that access to the registers is complete, release the registers
		* semaphore so that other threads can access the registers.
		*/
		ALT_SEM_POST (dev->regs_lock);
		
        /* operation not permitted due to descriptor type conflict */
        return -EPERM; 
    }

    /*
    * If a callback routine has been previously registered which will be
    * called from the msgdma ISR. Set up controller to:
    *  - Run
    *  - Stop on an error with any particular descriptor
    */
    if(dev->callback)
    {

        control |= (dev->control |
                    ALTERA_MSGDMA_CSR_STOP_ON_ERROR_MASK |
                    ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK );
        control &=  (~ALTERA_MSGDMA_CSR_STOP_DESCRIPTORS_MASK);
    /* making sure the read-modify-write below can't be pre-empted */
        context = alt_irq_disable_all(); 
        IOWR_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base, control);
        alt_irq_enable_all(context);
    }
    /*
    * No callback has been registered. Set up controller to:
    *   - Run
    *   - Stop on an error with any particular descriptor
    *   - Disable interrupt generation
    */
    else
    {
        control |= (dev->control |
                    ALTERA_MSGDMA_CSR_STOP_ON_ERROR_MASK );
        control &= (~ALTERA_MSGDMA_CSR_STOP_DESCRIPTORS_MASK) &
                   (~ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK);
        /* making sure the read-modify-write below can't be pre-empted */
        context = alt_irq_disable_all();
        IOWR_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base, control);
        alt_irq_enable_all(context);
    }   

    /*
	 * Now that access to the registers is complete, release the registers
     * semaphore so that other threads can access the registers.
     */
    ALT_SEM_POST (dev->regs_lock);
    
    return 0;
}

/*
 * Helper functions for descriptor in sync transfer.
 * Arguments:
 * - *dev: Pointer to msgdma device (instance) structure.
 * - *standard_desc: Pointer to single standard descriptor.
 * - *extended_desc: Pointer to single extended descriptor.
 *
 * Note: Either one of both *standard_desc and *extended_desc must 
 *      be assigned with NULL, another with proper pointer value.
 *      Failing to do so can cause the function return with "-EPERM "
 *
 * "stop on error" of CSR control register is always being masked and interrupt 
 * is always disabled within this function.
 * The CSR control can be set by user through calling "alt_register_callback" 
 * with passing user defined control setting.
 *
 * Returns:
 * 0 -> success
 * error -> errors or conditions causing msgdma stop issuing commands to masters. 
 *          check the bit set in the error with CSR status register.
 * -EPERM -> operation not permitted due to descriptor type conflict
 * -ETIME -> Time out and skipping the looping after 5 msec.
 */
static int alt_msgdma_descriptor_sync_transfer (
	alt_msgdma_dev *dev, 
	alt_msgdma_standard_descriptor *standard_desc, 
	alt_msgdma_extended_descriptor *extended_desc)
{
    alt_u32 control=0;
    alt_irq_context context=0;
    alt_u32 csr_status = 0;
    alt_u16 counter = 0;
    alt_u32 fifo_read_fill_level = (
		IORD_ALTERA_MSGDMA_CSR_DESCRIPTOR_FILL_LEVEL(dev->csr_base) & 
		ALTERA_MSGDMA_CSR_READ_FILL_LEVEL_MASK) >> 
		ALTERA_MSGDMA_CSR_READ_FILL_LEVEL_OFFSET;
    alt_u32 fifo_write_fill_level = (
		IORD_ALTERA_MSGDMA_CSR_DESCRIPTOR_FILL_LEVEL(dev->csr_base) & 
		ALTERA_MSGDMA_CSR_WRITE_FILL_LEVEL_MASK) >> 
		ALTERA_MSGDMA_CSR_WRITE_FILL_LEVEL_OFFSET;
    alt_u32 error = ALTERA_MSGDMA_CSR_STOPPED_ON_ERROR_MASK | 
                    ALTERA_MSGDMA_CSR_STOPPED_ON_EARLY_TERMINATION_MASK |
                    ALTERA_MSGDMA_CSR_STOP_STATE_MASK |
                    ALTERA_MSGDMA_CSR_RESET_STATE_MASK;
    
    /* Wait for available FIFO buffer to store new descriptor*/
    while ((dev->descriptor_fifo_depth <= fifo_write_fill_level) || 
    	(dev->descriptor_fifo_depth <= fifo_read_fill_level))
    { 
        alt_busy_sleep(1); /* delay 1us */
        if(5000 <= counter) /* time_out if waiting longer than 5 msec */
        {
            alt_printf("time out after 5 msec while waiting free FIFO buffer"
				" for storing descriptor\n");
            return -ETIME;
        }
        counter++;  
        fifo_read_fill_level = (
			IORD_ALTERA_MSGDMA_CSR_DESCRIPTOR_FILL_LEVEL(dev->csr_base) & 
			ALTERA_MSGDMA_CSR_READ_FILL_LEVEL_MASK) >> 
			ALTERA_MSGDMA_CSR_READ_FILL_LEVEL_OFFSET;
        fifo_write_fill_level = (
			IORD_ALTERA_MSGDMA_CSR_DESCRIPTOR_FILL_LEVEL(dev->csr_base) & 
			ALTERA_MSGDMA_CSR_WRITE_FILL_LEVEL_MASK) >> 
			ALTERA_MSGDMA_CSR_WRITE_FILL_LEVEL_OFFSET;
    }

     /*
     * When running in a multi threaded environment, obtain the "regs_lock"
     * semaphore. This ensures that accessing registers is thread-safe.
     */
	ALT_SEM_PEND (dev->regs_lock, 0);
    
    /* Stop the msgdma dispatcher from issuing more descriptors to the
    read or write masters  */
    /* making sure the read-modify-write below can't be pre-empted */
    context = alt_irq_disable_all();
    IOWR_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base,
        ALTERA_MSGDMA_CSR_STOP_DESCRIPTORS_MASK);
    /*
    * Clear any (previous) status register information
    * that might occlude our error checking later.
    */
    IOWR_ALTERA_MSGDMA_CSR_STATUS(
		dev->csr_base, 
		IORD_ALTERA_MSGDMA_CSR_STATUS(dev->csr_base));

    if (NULL != standard_desc && NULL == extended_desc)
    {
        counter = 0; /* reset counter */
        /*writing descriptor structure to the dispatcher, wait until descriptor 
	  	write is succeed*/
        while(0 != alt_msgdma_write_standard_descriptor (
			dev->csr_base, dev->descriptor_base, standard_desc))
        {     
            alt_busy_sleep(1); /* delay 1us */
            if(5000 <= counter) /* time_out if waiting longer than 5 msec */
            {
                alt_printf("time out after 5 msec while writing standard" 
					" descriptor to FIFO\n");
				
				/*
				* Now that access to the registers is complete, release the 
				* registers semaphore so that other threads can access the 
				* registers.
				*/
				ALT_SEM_POST (dev->regs_lock);
				
                return -ETIME;
            }
            counter++;    
        }
    }
    else if (NULL == standard_desc && NULL != extended_desc)
    {
        counter = 0; /* reset counter */
        /*writing descriptor structure to the dispatcher, wait until descriptor 
	  	write is succeed*/
        while(0 != alt_msgdma_write_extended_descriptor (
			dev->csr_base, dev->descriptor_base, extended_desc))
        {
            alt_busy_sleep(1); /* delay 1us */
            if(5000 <= counter) /* time_out if waiting longer than 5 msec */
            {
                alt_printf("time out after 5 msec while writing extended" 
					" descriptor to FIFO\n");
				
				/*
				* Now that access to the registers is complete, release the 
				* registers semaphore so that other threads can access the 
				* registers.
				*/
				ALT_SEM_POST (dev->regs_lock);
				
                return -ETIME;
            }
            counter++;
        }
    }
    else
    {
		/*
		* Now that access to the registers is complete, release the registers
		* semaphore so that other threads can access the registers.
		*/
		ALT_SEM_POST (dev->regs_lock);
		
        /* operation not permitted due to descriptor type conflict */
        return -EPERM; 
    }
    
    /*
    * Set up msgdma controller to:
    * - Disable interrupt generation
    * - Run once a valid descriptor is written to controller
    * - Stop on an error with any particular descriptor
    */
     IOWR_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base,
		(dev->control |
		ALTERA_MSGDMA_CSR_STOP_ON_ERROR_MASK ) &
		(~ALTERA_MSGDMA_CSR_STOP_DESCRIPTORS_MASK) &
		(~ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK)) ;
	 
	alt_irq_enable_all(context);
     
    counter = 0; /* reset counter */ 
	
	csr_status = IORD_ALTERA_MSGDMA_CSR_STATUS(dev->csr_base);
	
    /* Wait for any pending transfers to complete or checking any errors or 
    conditions causing descriptor to stop dispatching */
    while (!(csr_status & error) && (csr_status & ALTERA_MSGDMA_CSR_BUSY_MASK))
    {
        alt_busy_sleep(1); /* delay 1us */
        if(5000 <= counter) /* time_out if waiting longer than 5 msec */
        {
            alt_printf("time out after 5 msec while waiting for any pending" 
				" transfer complete\n");
			
			/*
			* Now that access to the registers is complete, release the registers
			* semaphore so that other threads can access the registers.
			*/
			ALT_SEM_POST (dev->regs_lock);
			
            return -ETIME;
        }
        counter++;
        csr_status = IORD_ALTERA_MSGDMA_CSR_STATUS(dev->csr_base);
    }
    

    /*Errors or conditions causing the dispatcher stopping issuing read/write 
      commands to masters*/
    if(0 != (csr_status & error))
    {
		/*
		* Now that access to the registers is complete, release the registers
		* semaphore so that other threads can access the registers.
		*/
		ALT_SEM_POST (dev->regs_lock);
		
        return error;
    }

    /* Stop the msgdma dispatcher from issuing more descriptors to the
    read or write masters  */
    /* stop issuing more descriptors */
    control = IORD_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base) | 
	ALTERA_MSGDMA_CSR_STOP_DESCRIPTORS_MASK;
    /* making sure the read-modify-write below can't be pre-empted */
    context = alt_irq_disable_all();  
    IOWR_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base, control);
    /*
    * Clear any (previous) status register information
    * that might occlude our error checking later.
    */
    IOWR_ALTERA_MSGDMA_CSR_STATUS(
		dev->csr_base, 
		IORD_ALTERA_MSGDMA_CSR_STATUS(dev->csr_base));
    	alt_irq_enable_all(context);

	/*
	* Now that access to the registers is complete, release the registers
	* semaphore so that other threads can access the registers.
	*/
    ALT_SEM_POST (dev->regs_lock);
    
    return 0;

}

/*
 * Functions for constructing standard descriptors.  Unnecessary elements are
 * set to 0 for completeness and will be ignored by the hardware.
 * Returns:
 * - status: return 0 (success)
 *           return -EINVAL (invalid argument, could be due to argument which 
 *                          has larger value than hardware setting value)
 */
int alt_msgdma_construct_standard_st_to_mm_descriptor (
    alt_msgdma_dev *dev,
	alt_msgdma_standard_descriptor *descriptor,
	alt_u32 *write_address, alt_u32 length, alt_u32 control)
{
    return alt_msgdma_construct_standard_descriptor(dev, descriptor, NULL, 
            write_address, length, control);
}

int alt_msgdma_construct_standard_mm_to_st_descriptor (
    alt_msgdma_dev *dev,
	alt_msgdma_standard_descriptor *descriptor,
	alt_u32 *read_address,
	alt_u32 length,
	alt_u32 control)
{
    return alt_msgdma_construct_standard_descriptor(dev, descriptor, read_address, 
            NULL, length, control);

}

int alt_msgdma_construct_standard_mm_to_mm_descriptor (
    alt_msgdma_dev *dev,
	alt_msgdma_standard_descriptor *descriptor,
	alt_u32 *read_address, 
	alt_u32 *write_address, 
	alt_u32 length, 
	alt_u32 control)
{
    return alt_msgdma_construct_standard_descriptor(dev, descriptor, read_address, 
            write_address, length, control);
}

/*
 * Functions for constructing extended descriptors.  If you disable some of the
 * extended features in the hardware then you should pass in 0 for that 
 * particular descriptor element. These disabled elements will not be buffered 
 * by the dispatcher block.
 * Returns:
 * - status: return 0 (success)
 *           return -EINVAL (invalid argument, could be due to argument which 
 *                          has larger value than hardware setting value)
 */
int alt_msgdma_construct_extended_st_to_mm_descriptor (
    alt_msgdma_dev *dev,
	alt_msgdma_extended_descriptor *descriptor,
	alt_u32 *write_address, 
	alt_u32 length, 
	alt_u32 control, 
	alt_u16 sequence_number, 
	alt_u8 write_burst_count, 
	alt_u16 write_stride)
{
    return alt_msgdma_construct_extended_descriptor(dev, descriptor, 
            NULL, write_address, length, control, sequence_number, 0, 
            write_burst_count, 0, write_stride);
}

int alt_msgdma_construct_extended_mm_to_st_descriptor (
    alt_msgdma_dev *dev,
	alt_msgdma_extended_descriptor *descriptor,
	alt_u32 *read_address,
	alt_u32 length,
	alt_u32 control,
	alt_u16 sequence_number,
	alt_u8 read_burst_count, 
	alt_u16 read_stride)
{
    return alt_msgdma_construct_extended_descriptor(dev, descriptor, read_address, 
            NULL, length, control, sequence_number, read_burst_count, 0, 
            read_stride, 0);

}

int alt_msgdma_construct_extended_mm_to_mm_descriptor (
    alt_msgdma_dev *dev,
    alt_msgdma_extended_descriptor *descriptor,
    alt_u32 *read_address, 
    alt_u32 *write_address, 
    alt_u32 length, 
    alt_u32 control,
    alt_u16 sequence_number, 
    alt_u8 read_burst_count, 
    alt_u8 write_burst_count,
    alt_u16 read_stride, 
    alt_u16 write_stride)
{
    return alt_msgdma_construct_extended_descriptor(dev, descriptor, 
            read_address, write_address, length, control, sequence_number, 
            read_burst_count, write_burst_count, read_stride, write_stride);

}

/********************** MSGDMA PREFETCHER PRIVATE APIs *************************/

/*
 * Base functions for constructing mm_to_st, st_to_mm, mm_to_mm standard 
 * descriptors for the prefetcher. Unnecessary elements are set to 0 for 
 * completeness and will be ignored by the hardware.
 * The descriptor created will be suitable for park since this API will set next_ptr
 * to itself as park_mode requires. Additionally OWN_BY_HW bit left as 0 (owned by sw) 
 * until the prefetcher is started with this descriptor in the list. 
 * Returns:
 * - status: return 0 (success)
 *           return -EINVAL (invalid argument, could be due to argument which 
 *                          has larger value than hardware setting value)
 */
static int alt_msgdma_construct_prefetcher_standard_descriptor(
    alt_msgdma_dev *dev,
	alt_msgdma_prefetcher_standard_descriptor *descriptor,
	alt_u32 read_address, 
	alt_u32 write_address, 
	alt_u32 length, 
	alt_u32 control)
{
    if(dev->max_byte < length ||
       dev->enhanced_features != 0
      )
    {
        return -EINVAL;
    }
    descriptor->read_address = read_address;
    descriptor->write_address = write_address;
    descriptor->transfer_length = length;
    /* have descriptor point to itself for park_mode */
    descriptor->next_desc_ptr = (alt_u32)descriptor;  
    
    /* clear control own_by_hw bit field (SW owns this descriptor)*/
    descriptor->control = (control 
    		& ALT_MSGDMA_PREFETCHER_DESCRIPTOR_CTRL_OWN_BY_HW_CLR_MASK) 
    		| ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GO_MASK;
    
    return 0;
}

/*
 * Base functions for constructing mm_to_st, st_to_mm, mm_to_mm extended 
 * descriptors. Unnecessary elements are set to 0 for completeness and will be 
 * ignored by the hardware. The descriptor created will be suitable for park 
 * mode since this API will set next_ptr to itself as park_mode requires. 
 * Additionally OWN_BY_HW bit left as 0 (owned by sw) until the prefetcher is 
 * started with this descriptor in the list. 
 * Returns:
 * - status: return 0 (success)
 *           return -EINVAL (invalid argument, could be due to argument which 
 *                          has larger value than hardware setting value)
 */
static int alt_msgdma_construct_prefetcher_extended_descriptor(
    alt_msgdma_dev *dev,
	alt_msgdma_prefetcher_extended_descriptor *descriptor,
	alt_u32 read_address_high,
	alt_u32 read_address_low,
	alt_u32 write_address_high,
	alt_u32 write_address_low, 
	alt_u32 length, 
	alt_u32 control,
	alt_u16 sequence_number, 
	alt_u8 read_burst_count, 
	alt_u8 write_burst_count,
	alt_u16 read_stride, 
	alt_u16 write_stride)
{
	msgdma_addr64 node_addr;
	
	if(dev->max_byte < length ||
       dev->max_stride < read_stride ||
       dev->max_stride < write_stride ||
       dev->enhanced_features != 1 
      )
    {
        return -EINVAL;
    }
    
    descriptor->read_address_high = read_address_high;
    descriptor->read_address_low = read_address_low;
    descriptor->write_address_high = write_address_high;
    descriptor->write_address_low = write_address_low;
    descriptor->transfer_length = length;
    descriptor->sequence_number = sequence_number;
    descriptor->read_burst_count = read_burst_count;
    descriptor->write_burst_count = write_burst_count;
    descriptor->read_stride = read_stride;
    descriptor->write_stride = write_stride;
    /* have descriptor point to itself */
	node_addr.u64 = (uintptr_t)descriptor;
    descriptor->next_desc_ptr_low = node_addr.u32[0];  
    descriptor->next_desc_ptr_high = node_addr.u32[1];
    
    /* clear control own_by_hw bit field (SW still owns this descriptor). */
    descriptor->control = (control 
    		& ALT_MSGDMA_PREFETCHER_DESCRIPTOR_CTRL_OWN_BY_HW_CLR_MASK) 
    		| ALTERA_MSGDMA_DESCRIPTOR_CONTROL_GO_MASK;

  return 0 ;
}


/********************** MSGDMA PREFETCHER PUBLIC APIs ************************/

/*
 * Functions for constructing standard descriptors.  Unnecessary elements are
 * set to 0 for completeness and will be ignored by the hardware.
 * Returns:
 * - status: return 0 (success)
 *           return -EINVAL (invalid argument, could be due to argument which 
 *                          has larger value than hardware setting value)
 */
int alt_msgdma_construct_prefetcher_standard_mm_to_mm_descriptor (
	alt_msgdma_dev *dev,
	alt_msgdma_prefetcher_standard_descriptor *descriptor,
	alt_u32 read_address,
	alt_u32 write_address,
	alt_u32 length,
	alt_u32 control)
{
    return alt_msgdma_construct_prefetcher_standard_descriptor(dev, descriptor,
    		read_address, write_address, length, control);
}

int alt_msgdma_construct_prefetcher_standard_st_to_mm_descriptor (
	alt_msgdma_dev *dev,
	alt_msgdma_prefetcher_standard_descriptor *descriptor, 
	alt_u32 write_address, 
	alt_u32 length, 
	alt_u32 control)
{
    return alt_msgdma_construct_prefetcher_standard_descriptor(dev, descriptor,
    		0, write_address, length, control);
}
    
int alt_msgdma_construct_prefetcher_standard_mm_to_st_descriptor (
	alt_msgdma_dev *dev,
	alt_msgdma_prefetcher_standard_descriptor *descriptor, 
	alt_u32 read_address, 
	alt_u32 length, 
	alt_u32 control)
{
    return alt_msgdma_construct_prefetcher_standard_descriptor(dev, descriptor,
    		read_address, 0, length, control);
}


/*
 * Functions for constructing extended descriptors.  If you disable some of the
 * extended features in the hardware then you should pass in 0 for that 
 * particular descriptor element. These disabled elements will not be buffered 
 * by the dispatcher block.
 * Returns:
 * - status: return 0 (success)
 *           return -EINVAL (invalid argument, could be due to argument which 
 *                          has larger value than hardware setting value)
 */
int alt_msgdma_construct_prefetcher_extended_st_to_mm_descriptor (
	alt_msgdma_dev *dev,
	alt_msgdma_prefetcher_extended_descriptor *descriptor, 
	alt_u32 write_address_high,
	alt_u32 write_address_low, 
	alt_u32 length, 
	alt_u32 control, 
	alt_u16 sequence_number,
	alt_u8 write_burst_count,
	alt_u16 write_stride)
{
    return alt_msgdma_construct_prefetcher_extended_descriptor(dev, descriptor, 
            0, 0, write_address_high, write_address_low, length, control, 
			sequence_number, 0, write_burst_count, 0, write_stride);
}

int alt_msgdma_construct_prefetcher_extended_mm_to_st_descriptor (
	alt_msgdma_dev *dev,
	alt_msgdma_prefetcher_extended_descriptor *descriptor, 
	alt_u32 read_address_high,
	alt_u32 read_address_low, 
	alt_u32 length, 
	alt_u32 control, 
	alt_u16 sequence_number, 
	alt_u8 read_burst_count, 
	alt_u16 read_stride)
{
    return alt_msgdma_construct_prefetcher_extended_descriptor(dev, descriptor,
    		read_address_high, read_address_low, 0, 0, length, control, 
			sequence_number, read_burst_count, 0, read_stride, 0);
}

int alt_msgdma_construct_prefetcher_extended_mm_to_mm_descriptor (
	alt_msgdma_dev *dev,
	alt_msgdma_prefetcher_extended_descriptor *descriptor, 
	alt_u32 read_address_high,
	alt_u32 read_address_low,
	alt_u32 write_address_high, 
	alt_u32 write_address_low, 
	alt_u32 length, 
	alt_u32 control,
	alt_u16 sequence_number,
	alt_u8 read_burst_count,
	alt_u8 write_burst_count, 
	alt_u16 read_stride, 
	alt_u16 write_stride)
{
    return alt_msgdma_construct_prefetcher_extended_descriptor(dev, descriptor,
            read_address_high, read_address_low, write_address_high, 
			write_address_low, length, control, sequence_number, 
			read_burst_count, write_burst_count, read_stride, write_stride);

}

/* PREFETCHER linked list APIs */
/*
 * Function for adding standard descriptors to a standard descriptor list
 * Returns:
 * - status: return 0 (success)
 *           return -EINVAL (invalid argument, could be due to descriptor 
 *           already being in the list, descriptor pointer being NULL, or
 *           descriptor.next_ptr not pointing back to itslef)
 */
int alt_msgdma_prefetcher_add_standard_desc_to_list (
	alt_msgdma_prefetcher_standard_descriptor** list,
	alt_msgdma_prefetcher_standard_descriptor* descriptor)
{
	alt_msgdma_prefetcher_standard_descriptor *last_descr_ptr;
	
	if (descriptor == NULL)
	{
		return -EINVAL;  /* this descriptor cannot be NULL */
	}
	if (descriptor->next_desc_ptr != (alt_u32)descriptor)
	{
		return -EINVAL;  /* descriptor.next_ptr must point to itself */
	}
	if (*list == NULL)
	{
		*list = descriptor;  /* make this root-node if list is empty */
		return 0;  /* successfully added */
	}
	if (*list == descriptor)
	{
		return -EINVAL;  /* this descriptor cannot already be root-node */
	}
	
	/* get to last node in the list */
	last_descr_ptr = *list; /* start at list root-node */
	/* traverse list until you get the last node */
	while (last_descr_ptr->next_desc_ptr != (alt_u32)*list)  
	{
		if (last_descr_ptr->next_desc_ptr == (alt_u32)descriptor)
		{
			return -EINVAL;  /* descriptor cannot already be in the list */
		}
		last_descr_ptr = 
			(alt_msgdma_prefetcher_standard_descriptor*)(last_descr_ptr->next_desc_ptr);
	}
	/* add this descriptor to end of list */
	last_descr_ptr->next_desc_ptr = (alt_u32)((uintptr_t)descriptor);
	/* ensure new last pointer points the start of the list */
	descriptor->next_desc_ptr = (alt_u32)((uintptr_t)*list);  
	return 0; /* successfully added */
}

int alt_msgdma_prefetcher_add_extended_desc_to_list (
	alt_msgdma_prefetcher_extended_descriptor** list,
	alt_msgdma_prefetcher_extended_descriptor* descriptor)
{
	alt_msgdma_prefetcher_extended_descriptor *last_descr_ptr;
	msgdma_addr64 root_node_addr, next_node_addr;
	
	if (descriptor == NULL)
	{
		return -EINVAL;  /* this descriptor cannot be NULL */
	}
	
	next_node_addr.u64 = (uintptr_t)descriptor;
	if( (descriptor->next_desc_ptr_low != next_node_addr.u32[0]) &&
		(descriptor->next_desc_ptr_high != next_node_addr.u32[1]))
	{
		return -EINVAL;  /* descriptor.next_ptr must point to itself */
	}
	
	if (*list == NULL)
	{
		*list = descriptor;  /* make this the root-node if list is empty */
		return 0;
	}
	if (*list == descriptor)
	{
		return -EINVAL;  /* this descriptor cannot already be root-node */
	}
	
	/* get to last node in the list */
	last_descr_ptr = *list; /* start at list root-node */
	/* the last nodes next ptr should point to the root node*/
	root_node_addr.u64 = (uintptr_t)*list;
	
	/* traverse list until you get the last node */
	while ((last_descr_ptr->next_desc_ptr_low != root_node_addr.u32[0]) 
		&& (last_descr_ptr->next_desc_ptr_high != root_node_addr.u32[1]))
	{
		/* first check if descriptor already in the list */
		next_node_addr.u64 = (uintptr_t)descriptor;
		if ((last_descr_ptr->next_desc_ptr_low == next_node_addr.u32[0])
			&& (last_descr_ptr->next_desc_ptr_high == next_node_addr.u32[1]))
		{
			return -EINVAL;  /* descriptor cannot already be in the list */
		}
		/* go to next node in list, using 64 bit address */
		next_node_addr.u32[0] = last_descr_ptr->next_desc_ptr_low;
		next_node_addr.u32[1] = last_descr_ptr->next_desc_ptr_high;
		last_descr_ptr = 
			(alt_msgdma_prefetcher_extended_descriptor*)((uintptr_t)next_node_addr.u64);
	}
	/* add this descriptor to end of list */
	next_node_addr.u64 = (uintptr_t)descriptor;
	last_descr_ptr->next_desc_ptr_low = next_node_addr.u32[0];
	last_descr_ptr->next_desc_ptr_high = next_node_addr.u32[1];
	/* ensure new last pointer points the beginning of the list */
	descriptor->next_desc_ptr_low = root_node_addr.u32[0];
	descriptor->next_desc_ptr_high = root_node_addr.u32[1];
	return 0;
}

/*
 * Functions to set all the own-by-hw bits, need to call right before starting 
 * prefetcher since if used the create descriptor APIs the set_by_hw bits are
 * still set to SW owned. 
 */
int alt_msgdma_prefetcher_set_std_list_own_by_hw_bits (
	alt_msgdma_prefetcher_standard_descriptor *list)
{
	alt_u32 descriptor_control_field = 0;
	alt_msgdma_prefetcher_standard_descriptor *last_descr_ptr;
	if (list == NULL)
	{
		return -EINVAL;  /* this list cannot be empty */
	}

	/* update all nodes in the list */
	last_descr_ptr = list; /* start at list root-node */
	/* traverse list to update all of the nodes */
	while (last_descr_ptr->next_desc_ptr != (alt_u32)list)  
	{
		/* get current value */
		descriptor_control_field = last_descr_ptr->control;
		/* update own_by_hw bit only */
		last_descr_ptr->control = descriptor_control_field 
				| ALT_MSGDMA_PREFETCHER_DESCRIPTOR_CTRL_OWN_BY_HW_SET_MASK;
		/* go to next node in list */
		last_descr_ptr = 
			(alt_msgdma_prefetcher_standard_descriptor*)(last_descr_ptr->next_desc_ptr);
	}
	/* update the last node in the list, currently last_descr_ptr after while loop */
	descriptor_control_field = last_descr_ptr->control;    /* get current value */
	/* update own_by_hw bit only */
	last_descr_ptr->control = descriptor_control_field 
		| ALT_MSGDMA_PREFETCHER_DESCRIPTOR_CTRL_OWN_BY_HW_SET_MASK;
	
	return 0;
}

/*
 * Functions to set all the own-by-hw bits, need to call right before starting 
 * prefetcher since if used the create descriptor APIs the set_by_hw bits are
 * still set to SW owned. 
 */
int alt_msgdma_prefetcher_set_extd_list_own_by_hw_bits (
	alt_msgdma_prefetcher_extended_descriptor *list)
{
	alt_u32 descriptor_control_field = 0;
	msgdma_addr64 root_node_addr, next_node_addr;
	alt_msgdma_prefetcher_extended_descriptor *last_descr_ptr;
	
	if (list == NULL)
	{
		return -EINVAL;  /* this list cannot be empty */
	}
	
	/* update all nodes in the list */
	last_descr_ptr = list; /* start at list root-node */
	/* the last nodes next ptr should point to the root node*/
	root_node_addr.u64 = (uintptr_t)list;

	/* traverse list until you get the last node */
	while ((last_descr_ptr->next_desc_ptr_low != root_node_addr.u32[0]) 
		&& (last_descr_ptr->next_desc_ptr_high != root_node_addr.u32[1]))
	{
		/* start with current value */
		descriptor_control_field = last_descr_ptr->control;
		/* update own_by_hw bit only */
		last_descr_ptr->control = descriptor_control_field 
				| ALT_MSGDMA_PREFETCHER_DESCRIPTOR_CTRL_OWN_BY_HW_SET_MASK;
		/* go to next node in list, using 64 bit address */
		next_node_addr.u32[0] = last_descr_ptr->next_desc_ptr_low;
		next_node_addr.u32[1] = last_descr_ptr->next_desc_ptr_high;
		last_descr_ptr = 
			(alt_msgdma_prefetcher_extended_descriptor*)((uintptr_t)next_node_addr.u64);
	}
	/* update the last node in the list, currently last_descr_ptr after while loop */
	descriptor_control_field = last_descr_ptr->control;    /* start with current value */
	/* update own_by_hw bit only */
	last_descr_ptr->control = descriptor_control_field 
		| ALT_MSGDMA_PREFETCHER_DESCRIPTOR_CTRL_OWN_BY_HW_SET_MASK;
	return 0;
}


/*
 * Functions to start the prefetcher.  Will return error if prefetcher already
 * started.
 * 
 * Arguments:# This driver supports HAL types
 * - *dev: Pointer to msgdma device (instance) structure.
 * - *standard_desc: Pointer to single standard descriptor  OR  *extended_desc:
 * 	 Pointer to single extended descriptor.
 * -  park_mode_en:  setting for prefetcher park mode
 * -  poll_en: setting for poll_en (IF poll frequency still 0 this API will 
 * 	also set that to a default non-zero value)
 *
 *note: Must call API specific to descriptor type. Either 
 *	 alt_msgdma_start_prefetcher_with_std_desc_list   OR
 *	 alt_msgdma_start_prefetcher_with_extd_desc_list  
 *	 where the list paratmeter is the root-node of your linked list. Then those
 *	  APIs will call the base function accordingly.
 *
 * If a callback routine has been previously registered with this
 * particular msgdma controller, transfer will be set up to enable interrupt 
 * generation. It is the responsibility of the application developer to check 
 * source interruption, status completion and creating suitable interrupt 
 * handling.
 * Note: "stop on error" of CSR control register is always masking within this 
 *	 function. The CSR control can be set by user through calling 
 *       "alt_register_callback" by passing user used defined control setting.
 *
 * Returns:
 * 0 -> success
 * -EBUSY -> prefetcher busy processing list already, it is up to user to stop
 *  prefetcher/dispatcher correctly before calling this function.  
 *  if already busy will always return error.
 */

/*
 * Base function to start prefetcher.
 */
int alt_msgdma_start_prefetcher_with_list_addr (
	alt_msgdma_dev *dev,
	alt_u64  list_addr,
	alt_u8 park_mode_en,
	alt_u8 poll_en)
{
	alt_u32 prefetcher_ctl = 0;
	alt_u32 dispatcher_ctl = 0;
	alt_irq_context context = 0;
	
	/* use helper struct to get easy access to hi/low address */
	msgdma_addr64 root_node_addr;
	root_node_addr.u64 = list_addr;  
	
	/*
	 * When running in a multi threaded environment, obtain the "regs_lock"
	 * semaphore. This ensures that accessing registers is thread-safe.
	 */
	ALT_SEM_PEND (dev->regs_lock, 0);
	
	/* case where prefetcher already started, return busy error */ 
	prefetcher_ctl = IORD_ALT_MSGDMA_PREFETCHER_CONTROL(dev->prefetcher_base);
	if(ALT_MSGDMA_PREFETCHER_CTRL_RUN_GET(prefetcher_ctl)){
		/* release the registers semaphore */
		ALT_SEM_POST (dev->regs_lock);
		return -EBUSY;
	}
		
	/* Stop the msgdma dispatcher from issuing more descriptors to the
	   read or write masters  */
	/* stop issuing more descriptors */
	dispatcher_ctl = ALTERA_MSGDMA_CSR_STOP_DESCRIPTORS_MASK;
	
	/* making sure the read-modify-write below can't be pre-empted */
	context = alt_irq_disable_all();
	IOWR_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base, dispatcher_ctl);
	/*
	 * Clear any (previous) status register information
	 * that might occlude our error checking later.
	 */
	IOWR_ALTERA_MSGDMA_CSR_STATUS( dev->csr_base, 
			IORD_ALTERA_MSGDMA_CSR_STATUS(dev->csr_base));
	
	alt_irq_enable_all(context);
	
	/*
	 * If a callback routine has been previously registered which will be
	 * called from the msgdma ISR. Set up dispatcher to:
	 *  - Run
	 *  - Stop on an error with any particular descriptor
	 */
	if(dev->callback)
	{
		dispatcher_ctl |= (dev->control | ALTERA_MSGDMA_CSR_STOP_ON_ERROR_MASK 
				| ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK );
		dispatcher_ctl &=  (~ALTERA_MSGDMA_CSR_STOP_DESCRIPTORS_MASK);
		
		prefetcher_ctl |= ALT_MSGDMA_PREFETCHER_CTRL_GLOBAL_INTR_EN_SET_MASK;
	    /* making sure the read-modify-write below can't be pre-empted */
	    context = alt_irq_disable_all(); 
	    IOWR_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base, dispatcher_ctl);
		IOWR_ALT_MSGDMA_PREFETCHER_CONTROL(dev->prefetcher_base, prefetcher_ctl);
	    alt_irq_enable_all(context);
	 }
	 /*
	  * No callback has been registered. Set up dispatcher to:
	  *   - Run
	  *   - Stop on an error with any particular descriptor
	  *   - Disable interrupt generation
	  */
	 else
	 {
		 dispatcher_ctl |= (dev->control | ALTERA_MSGDMA_CSR_STOP_ON_ERROR_MASK);
		 dispatcher_ctl &= (~ALTERA_MSGDMA_CSR_STOP_DESCRIPTORS_MASK) 
				 & (~ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK);
		 prefetcher_ctl &= ALT_MSGDMA_PREFETCHER_CTRL_GLOBAL_INTR_EN_CLR_MASK;
	     /* making sure the read-modify-write below can't be pre-empted */
	     context = alt_irq_disable_all();
	     IOWR_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base, dispatcher_ctl);
	     IOWR_ALT_MSGDMA_PREFETCHER_CONTROL(dev->prefetcher_base, prefetcher_ctl);
	     alt_irq_enable_all(context);
	 }   
	
	 /* set next descriptor registers to point to the list root-node */
	 IOWR_ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_LOW(dev->prefetcher_base,
			 root_node_addr.u32[0]);
	 IOWR_ALT_MSGDMA_PREFETCHER_NEXT_DESCRIPTOR_PTR_HIGH(dev->prefetcher_base,
			 root_node_addr.u32[1]);
		
	 /* set park-mode */
	 if (park_mode_en){
		 prefetcher_ctl |= ALT_MSGDMA_PREFETCHER_CTRL_PARK_MODE_SET_MASK;
	 }
	 else {
		 prefetcher_ctl &= ALT_MSGDMA_PREFETCHER_CTRL_PARK_MODE_CLR_MASK;
	 }
	 
	 /* set poll-en */
	 if (poll_en){
		 prefetcher_ctl |= ALT_MSGDMA_PREFETCHER_CTRL_DESC_POLL_EN_MASK; 
		 if(IORD_ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLLING_FREQ(
				 dev->prefetcher_base) == 0){
			 /* set poll frequency to some non-zero default value */
			 IOWR_ALT_MSGDMA_PREFETCHER_DESCRIPTOR_POLLING_FREQ(
					 dev->prefetcher_base, 0xFF);
		 }
	 }
	 else {
		 prefetcher_ctl &= ALT_MSGDMA_PREFETCHER_CTRL_DESC_POLL_EN_CLR_MASK; 
	 }
	 
	 /* set the prefetcher run bit */
	 prefetcher_ctl |= ALT_MSGDMA_PREFETCHER_CTRL_RUN_SET_MASK;
	 /* start the dma since run bit is set */
	 IOWR_ALT_MSGDMA_PREFETCHER_CONTROL(dev->prefetcher_base, prefetcher_ctl);
	 
	 /*
	  * Now that access to the registers is complete, release the registers
	  * semaphore so that other threads can access the registers.
	  */
	 ALT_SEM_POST (dev->regs_lock);
	 
	 return 0;
}

/*
 * Public functions to start prefetcher.
 */
int alt_msgdma_start_prefetcher_with_std_desc_list (
	alt_msgdma_dev *dev,
	alt_msgdma_prefetcher_standard_descriptor *list,
	alt_u8 park_mode_en,
	alt_u8 poll_en)
{	
	if (alt_msgdma_prefetcher_set_std_list_own_by_hw_bits(list) != 0){
		return -EINVAL;
	}
	
	return alt_msgdma_start_prefetcher_with_list_addr (dev, (uintptr_t)list,
			park_mode_en, poll_en);
}

int alt_msgdma_start_prefetcher_with_extd_desc_list (
	alt_msgdma_dev *dev,
	alt_msgdma_prefetcher_extended_descriptor *list,
	alt_u8 park_mode_en,
	alt_u8 poll_en)
{
	if (alt_msgdma_prefetcher_set_extd_list_own_by_hw_bits(list) != 0){
		return -EINVAL;
	}
	return alt_msgdma_start_prefetcher_with_list_addr (dev, (uintptr_t)list, 
			park_mode_en, poll_en);
}


/*
 * alt_msgdma_open - Retrieve a pointer to the msgdma
 *
 * Search the list of registered msgdma for one with the supplied name.
 *
 * The return value will be NULL on failure, and non-NULL otherwise.
 *
 * Arguments:
 * - *name: Character pointer to name of msgdma peripheral as registered
 *   with the HAL. For example, an msgdma controller named "msgdma_0"
 *   in Qsys would be opened by asking for "/dev/msgdma_0_csr".
 *
 * Returns:
 * - Pointer to msgdma device instance structure, or null if the device
 *   could not be opened.
 */
alt_msgdma_dev* alt_msgdma_open (const char* name)
{
    alt_msgdma_dev* dev = NULL;

    dev = (alt_msgdma_dev*) alt_find_dev (name, &alt_msgdma_list);

    if (NULL == dev)
    {
        ALT_ERRNO = ENODEV;
    }

    return dev;
}


/*
 * alt_msgdma_init()
 *
 * Initializes the Modular Scatter-Gather DMA controller. This routine is called
 * from the ALTERA_MSGDMA_INIT macro and is called automatically
 * by alt_sys_init.c
 *
 * This routine disables interrupts, descriptor processing,
 * registers a specific instance of the device with the HAL,
 * and installs an interrupt handler for the device.
 */
void alt_msgdma_init (alt_msgdma_dev *dev, alt_u32 ic_id, alt_u32 irq)
{
    extern alt_llist alt_msgdma_list;
    alt_u32 temporary_control;
    int error;

    if (dev->prefetcher_enable)
    {
    	/* start prefetcher reset sequence */
    	IOWR_ALT_MSGDMA_PREFETCHER_CONTROL(dev->prefetcher_base, 
    			ALT_MSGDMA_PREFETCHER_CTRL_RESET_SET_MASK);
    	/* wait until hw clears the bit */
    	while(ALT_MSGDMA_PREFETCHER_CTRL_RESET_GET(
    			IORD_ALT_MSGDMA_PREFETCHER_CONTROL(dev->prefetcher_base)));
    	/*
    	 * This reset is intended to be used along with reset dispatcher in 
    	 * dispatcher core. Once the reset sequence in prefetcher core has 
    	 * completed, software is expected to reset the dispatcher core, 
    	 * and polls for dispatcher’s reset sequence to be completed.
    	 */
    }    
    
    /* Reset the registers and FIFOs of the dispatcher and master modules */
    /* set the reset bit, no need to read the control register first since 
    this write is going to clear it out */
    IOWR_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base, ALTERA_MSGDMA_CSR_RESET_MASK);
    while(0 != (IORD_ALTERA_MSGDMA_CSR_STATUS(dev->csr_base)
    		& ALTERA_MSGDMA_CSR_RESET_STATE_MASK));
    /*
    * Disable interrupts, halt descriptor processing,
    * and clear status register content
    */

    /* disable global interrupt */
    temporary_control = IORD_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base)
    		& (~ALTERA_MSGDMA_CSR_GLOBAL_INTERRUPT_MASK);
    /* stopping descriptor */
    temporary_control |= ALTERA_MSGDMA_CSR_STOP_DESCRIPTORS_MASK;
    IOWR_ALTERA_MSGDMA_CSR_CONTROL(dev->csr_base, temporary_control);

    /* clear the CSR status register */
    IOWR_ALTERA_MSGDMA_CSR_STATUS(dev->csr_base, 
    		IORD_ALTERA_MSGDMA_CSR_STATUS(dev->csr_base));
    
    if (dev->prefetcher_enable)
    {
    	/* clear all status bits that are set, since theyre W1C */
    	IOWR_ALT_MSGDMA_PREFETCHER_STATUS(dev->prefetcher_base, 
    			IORD_ALT_MSGDMA_PREFETCHER_STATUS(dev->prefetcher_base));
    }

    /* Register this instance of the msgdma controller with HAL */
    alt_dev_llist_insert((alt_dev_llist*) dev, &alt_msgdma_list);

    /*
     * Creating semaphores used to protect access to the registers 
     * when running in a multi-threaded environment.
     */
    error = ALT_SEM_CREATE (&dev->regs_lock, 1);
            
    if (!error)
    {        
        /* Install IRQ handler */
        alt_ic_isr_register(ic_id, irq, alt_msgdma_irq, dev, 0x0);
    }
    else
    {
        alt_printf("failed to create semaphores\n");
    }
    
    return;

}


/*
 * alt_msgdma_register_callback
 *
 * Associate a user-specific routine with the msgdma interrupt handler.
 * If a callback is registered, all non-blocking msgdma transfers will
 * enable interrupts that will cause the callback to be executed.
 * The callback runs as part of the interrupt service routine, and
 * great care must be taken to follow the guidelines for acceptable
 * interrupt service routine behaviour as described in the Nios II
 * Software Developer's Handbook.However, user can change some of the CSR 
 * control setting in blocking transfer by calling this function.
 *
 * Note: To disable callbacks after registering one, this routine
 * may be called passing 0x0 to the callback argument.
 *
 * Arguments:
 * - *dev: Pointer to msgdma device (instance) structure.
 * - callback: Pointer to callback routine to execute at interrupt level
 * - control: For masking the source interruption and setting configuration in 
 *            control register
 */
void alt_msgdma_register_callback(
	alt_msgdma_dev *dev,
	alt_msgdma_callback callback,
	alt_u32 control,
	void *context)
{
    dev->callback         = callback;
    dev->callback_context = context;
    dev->control          = control;

    return ;
}

/*
 * alt_msgdma_standard_descriptor_async_transfer
 *
 * Set up and commence a non-blocking transfer of one descriptors at a time.
 *
 * If the FIFO buffer for one of read/write is full at the time of this call, 
 * the routine will immediately return -ENOSPC, the application can then decide 
 * how to proceed without being blocked.
 *
 * Arguments:
 * - *dev: Pointer to msgdma device (instance) struct.
 * - *desc: Pointer to single (ready to run) descriptor.
 *
 * Returns:
 * 0 -> success
 * -ENOSPC -> FIFO descriptor buffer is full
 * -EPERM -> operation not permitted due to descriptor type conflict
 * -ETIME -> Time out and skipping the looping after 5 msec.
 */
int alt_msgdma_standard_descriptor_async_transfer(
	alt_msgdma_dev *dev,
	alt_msgdma_standard_descriptor *desc)
{
   /*
   * Error detection/handling should be performed at the application
   * or callback level as appropriate.
   */
    return alt_msgdma_descriptor_async_transfer(dev, desc, NULL);

}


/*
 * alt_msgdma_extended_descriptor_async_transfer
 *
 * Set up and commence a non-blocking transfer of one descriptors at a time.
 *
 * If the FIFO buffer for one of read/write is full at the time of this call, 
 * the routine will immediately return -ENOSPC, the application can then
 * decide how to proceed without being blocked.
 *
 * Arguments:
 * - *dev: Pointer to msgdma device (instance) struct.
 * - *desc: Pointer to single (ready to run) descriptor.
 *
 * Returns:
 * 0 -> success
 * -ENOSPC -> FIFO descriptor buffer is full
 * -EPERM -> operation not permitted due to descriptor type conflict
 * -ETIME -> Time out and skipping the looping after 5 msec.
 */
int alt_msgdma_extended_descriptor_async_transfer(
	alt_msgdma_dev *dev,
	alt_msgdma_extended_descriptor *desc)
{
    /*
    * Error detection/handling should be performed at the application
    * or callback level as appropriate.
    */
    return alt_msgdma_descriptor_async_transfer(dev, NULL, desc);
}


/*
 * alt_msgdma_standard_descriptor_sync_transfer
 *
 * This function will start commencing a blocking transfer of one standard 
 * descriptor at a time. If the FIFO buffer for one of read/write is full at the 
 * time of this call, the routine will wait until free FIFO buffer available for 
 * continue processing.
 *
 * The function will return "-1" if errors or conditions causing the dispatcher 
 * stop issuing the commands to both read and write masters before both read and 
 * write command buffers are empty.
 *
 * Additional error information is available in the status bits of
 * each descriptor that the msgdma processed; it is the responsibility
 * of the user's application to search through the descriptor 
 * to gather specific error information.
 *
 * Arguments:
 * - *dev: Pointer to msgdma device (instance) structure.
 * - *desc: Pointer to single (ready to run) descriptor.
 *
 * Returns:
 * - status: return 0 (success)
 *           return error (errors or conditions causing msgdma stop issuing 
 *           	commands to masters)
 *           Suggest suggest checking the bit set in the error with CSR status 
 *           	register.
 *           return -EPERM (operation not permitted due to descriptor type 
 *		conflict)
 *           return -ETIME (Time out and skipping the looping after 5 msec)
 */
int alt_msgdma_standard_descriptor_sync_transfer(
	alt_msgdma_dev *dev,
	alt_msgdma_standard_descriptor *desc)
{
    return alt_msgdma_descriptor_sync_transfer(dev, desc, NULL);
}

/*
 * alt_msgdma_extended_descriptor_sync_transfer
 *
 * This function will start commencing a blocking transfer of one extended 
 * descriptor at a time. If the FIFO buffer for one of read/write is full at the 
 * time of this call, the routine will wait until free FIFO buffer available for 
 * continue processing.
 *
 * The function will return "-1" if errors or conditions causing the dispatcher 
 * stop issuing the commands to both read and write masters before both read and 
 * write command buffers are empty.
 *
 * Additional error information is available in the status bits of
 * each descriptor that the msgdma processed; it is the responsibility
 * of the user's application to search through the descriptor 
 * to gather specific error information.
 *
 *
 * Arguments:
 * - *dev: Pointer to msgdma device (instance) structure.
 * - *desc: Pointer to single (ready to run) descriptor.
 *
 * Returns:
 * - status: return 0 (success)
 *           return error (errors or conditions causing msgdma stop issuing 
 *		commands to masters)
 *           Suggest suggest checking the bit set in the error with CSR status 
 *		register.
 *           return -EPERM (operation not permitted due to descriptor type 
 *		conflict)
 *           return -ETIME (Time out and skipping the looping after 5 msec)
 */
int alt_msgdma_extended_descriptor_sync_transfer(
	alt_msgdma_dev *dev,
	alt_msgdma_extended_descriptor *desc)
{
    return alt_msgdma_descriptor_sync_transfer(dev, NULL, desc);
}





