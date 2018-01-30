/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2015 Altera Corporation, San Jose, California, USA.           *
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


#include <errno.h>
#include <io.h>
#include <string.h>
#include <stddef.h>
#include "sys/param.h"
#include "alt_types.h"
#include "altera_generic_quad_spi_controller2_regs.h"
#include "altera_generic_quad_spi_controller2.h"
#include "priv/alt_busy_sleep.h"
#include "sys/alt_debug.h"
#include "sys/alt_cache.h"

ALT_INLINE alt_32 static alt_qspi_validate_read_write_arguments(alt_qspi_controller2_dev *flash_info,alt_u32 offset, alt_u32 length);
alt_32 static alt_qspi_poll_for_write_in_progress(alt_qspi_controller2_dev* qspi_flash_info);


/*
 *  Public API
 *
 *  Refer to “Using Flash Devices” in the
 *  Developing Programs Using the Hardware Abstraction Layer chapter
 *   of the Nios II Software Developer’s Handbook.
 */


 /**
  * alt_qspi_controller2_lock
  *
  *  Locks the range of the memory sectors, which 
  *  protected from write and erase.
  *
  * Arguments:
  * - *flash_info: Pointer to general flash device structure.
  * - sectors_to_lock: Block protection bits in EPCQ/QSPI ==> Bit4 | Bit3 | Bit2 | Bit1 | Bit0
  *                                                            TB  | BP3  | BP2  | BP1  | BP0
  * For details of setting sectors protection, please refer to EPCQ/QSPI datasheet.
  *  
  * Returns:
  * 0 -> success
  * -EINVAL -> Invalid arguments
  * -ETIME  -> Time out and skipping the looping after 0.7 sec.
  * -ENOLCK -> Sectors lock failed.
**/

int alt_qspi_controller2_lock(alt_flash_dev *flash_info, alt_u32 sectors_to_lock)

{

    alt_u32 mem_op_value = 0; /* value to write to EPCQ_MEM_OP register */

    alt_qspi_controller2_dev* qspi_flash_info = NULL;

    alt_u32 result = 0;

    alt_32 status = 0;



    /* return -EINVAL if flash_info is NULL */

    if(NULL == flash_info || 0 > sectors_to_lock)

    {

    	return -EINVAL;

    }

	

    qspi_flash_info = (alt_qspi_controller2_dev*)flash_info;



    /* sector value should occupy bits 17:8 */

    mem_op_value = sectors_to_lock << 8;



    /* sector protect commands 0b11 occupies lower 2 bits */

    mem_op_value |= ALTERA_QSPI_CONTROLLER2_MEM_OP_SECTOR_PROTECT_CMD;



    /* write sector protect command to QSPI_MEM_OP register to protect sectors */

    IOWR_ALTERA_QSPI_CONTROLLER2_MEM_OP(qspi_flash_info->csr_base, mem_op_value);

    

    /* poll write in progress to make sure no operation is in progress */

    status = alt_qspi_poll_for_write_in_progress(qspi_flash_info);

    if(status != 0)

    {

    	return status;

    }

	

	status = IORD_ALTERA_QSPI_CONTROLLER2_STATUS(qspi_flash_info->csr_base);

	result |= (status >> 2) & 0x07; /* extract out BP3 - BP0 */

	result |= (status >> 3) & 0x08; /* extract out BP4 */

    result |= (status >> 1) & 0x10; /* extract out TOP/BOTTOM bit */



	if(result != sectors_to_lock)

	{

		return -ENOLCK;

	}



    return 0;

}



/**

 * alt_qspi_controller2_get_info

 *

 * Pass the table of erase blocks to the user. This flash will return a single

 * flash_region that gives the number and size of sectors for the device used.

 *

 * Arguments:

 * - *fd: Pointer to general flash device structure.

 * - **info: Pointer to flash region

 * - *number_of_regions: Pointer to number of regions

 *

 * For details of setting sectors protection, please refer to EPCQ/QSPI datasheet.

 *  

 * Returns:

 * 0 -> success

 * -EINVAL -> Invalid arguments

 * -EIO -> Could be hardware problem.

**/

int alt_qspi_controller2_get_info

(

    alt_flash_fd *fd, /** flash device descriptor */

    flash_region **info, /** pointer to flash_region will be stored here */

    int *number_of_regions /** number of regions will be stored here */

)

{

	alt_flash_dev* flash = NULL;

	

	/* return -EINVAL if fd,info and number_of_regions are NULL */

	if(NULL == fd || NULL == info || NULL == number_of_regions)

    {

    	return -EINVAL;

    }



    flash = (alt_flash_dev*)fd;



    *number_of_regions = flash->number_of_regions;



    if (!flash->number_of_regions)

    {

      return -EIO;

    }

    else

    {

      *info = &flash->region_info[0];

    }



    return 0;

}



/**

  * alt_qspi_controller2_erase_block

  *

  * This function erases a single flash sector.

  *

  * Arguments:

  * - *flash_info: Pointer to QSPI flash device structure.

  * - block_offset: byte-addressed offset, from start of flash, of the sector to be erased

  *  

  * Returns:

  * 0 -> success

  * -EINVAL -> Invalid arguments

  * -EIO -> write failed, sector might be protected 

**/

int alt_qspi_controller2_erase_block(alt_flash_dev *flash_info, int block_offset)

{

    alt_32 ret_code = 0;

    alt_u32 mem_op_value = 0; /* value to write to EPCQ_MEM_OP register */

    alt_qspi_controller2_dev* qspi_flash_info = NULL;

    alt_u32 sector_number = 0; 



    /* return -EINVAL if flash_info is NULL */

    if(NULL == flash_info)

    {

    	return -EINVAL;

    }

	

    qspi_flash_info = (alt_qspi_controller2_dev*)flash_info;



    /* 

     * Sanity checks that block_offset is within the flash memory span and that the 

     * block offset is sector aligned.

     *

     */

    if((block_offset < 0) 

        || (block_offset >= qspi_flash_info->size_in_bytes)

        || (block_offset & (qspi_flash_info->sector_size - 1)) != 0)

    {

    	return -EINVAL;

    }



    /* calculate current sector/block number */

    sector_number = (block_offset/(qspi_flash_info->sector_size));



    /* sector value should occupy bits 23:8 */

    mem_op_value = (sector_number << 8) & ALTERA_QSPI_CONTROLLER2_MEM_OP_SECTOR_VALUE_MASK;



    /* sector erase commands 0b10 occupies lower 2 bits */

    mem_op_value |= ALTERA_QSPI_CONTROLLER2_MEM_OP_SECTOR_ERASE_CMD;



    /* write sector erase command to QSPI_MEM_OP register to erase sector "sector_number" */

    IOWR_ALTERA_QSPI_CONTROLLER2_MEM_OP(qspi_flash_info->csr_base, mem_op_value);

	

    /* check whether erase triggered a illegal erase interrupt  */

    if((IORD_ALTERA_QSPI_CONTROLLER2_ISR(qspi_flash_info->csr_base) &

            		ALTERA_QSPI_CONTROLLER2_ISR_ILLEGAL_ERASE_MASK) ==

            				ALTERA_QSPI_CONTROLLER2_ISR_ILLEGAL_ERASE_ACTIVE)

    {

	    /* clear register */

	    /* QSPI_ISR access is write one to clear (W1C) */

    	IOWR_ALTERA_QSPI_CONTROLLER2_ISR(qspi_flash_info->csr_base,

    		ALTERA_QSPI_CONTROLLER2_ISR_ILLEGAL_ERASE_MASK );

    	return -EIO; /* erase failed, sector might be protected */

    }



    return ret_code;

}



/**

 * alt_qspi_controller2_write_block

 *

 * This function writes one block/sector of data to flash. The length of the write can NOT 

 * spill into the adjacent sector.

 *

 * It assumes that someone has already erased the appropriate sector(s).

 *

 * Arguments:

 * - *flash_info: Pointer to QSPI flash device structure.

 * - block_offset: byte-addressed offset, from the start of flash, of the sector to written to

 * - data-offset: Byte offset (unaligned access) of write into flash memory. 

 *                For best performance, word(32 bits - aligned access) offset of write is recommended.

 * - *src_addr: source buffer

 * - length: size of writing

 *  

 * Returns:

 * 0 -> success

 * -EINVAL -> Invalid arguments

 * -EIO -> write failed, sector might be protected 

**/

int alt_qspi_controller2_write_block

(

    alt_flash_dev *flash_info, /** flash device info */

    int block_offset, /** sector/block offset in byte addressing */

    int data_offset, /** offset of write from base address */

    const void *data, /** data to be written */

    int length /** bytes of data to be written, >0 */

)

{

    alt_u32 buffer_offset = 0; /** offset into data buffer to get write data */

    alt_u32 remaining_length = length; /** length left to write */

    alt_u32 write_offset = data_offset; /** offset into flash to write too */



    alt_qspi_controller2_dev *qspi_flash_info = (alt_qspi_controller2_dev*)flash_info;

	

    /* 

     * Sanity checks that data offset is not larger then a sector, that block offset is 

     * sector aligned and within the valid flash memory range and a write doesn't spill into 

     * the adjacent flash sector.

     */

    if(block_offset < 0

        || data_offset < 0

        || NULL == flash_info

        || NULL == data

        || data_offset >= qspi_flash_info->size_in_bytes

        || block_offset >= qspi_flash_info->size_in_bytes

        || length > (qspi_flash_info->sector_size - (data_offset - block_offset))

        || length < 0

        || (block_offset & (qspi_flash_info->sector_size - 1)) != 0) 

    {

    	return -EINVAL;

    }



    /*

     * Do writes one 32-bit word at a time.

     * We need to make sure that we pad the first few bytes so they're word aligned if they are

     * not already.

     */

    while (remaining_length > 0)

    {

    	alt_u32 word_to_write = 0xFFFFFFFF; /** initialize word to write to blank word */

    	alt_u32 padding = 0; /** bytes to pad the next word that is written */

    	alt_u32 bytes_to_copy = sizeof(alt_u32); /** number of bytes from source to copy */



        /*

         * we need to make sure the write is word aligned

    	 * this should only be true at most 1 time

    	 */

        if (0 != (write_offset & (sizeof(alt_u32) - 1)))

        {

        	/*

        	 * data is not word aligned

        	 * calculate padding bytes need to add before start of a data offset

        	 */

            padding = write_offset & (sizeof(alt_u32) - 1);



            /* update variables to account for padding being added */

            bytes_to_copy -= padding;



            if(bytes_to_copy > remaining_length)

            {

            	bytes_to_copy = remaining_length;

            }



            write_offset = write_offset - padding;

            if(0 != (write_offset & (sizeof(alt_u32) - 1)))

            {

            	return -EINVAL;

            }

        }

        else

        {

            if(bytes_to_copy > remaining_length)

            {

            	bytes_to_copy = remaining_length;

            }

        }



        /* prepare the word to be written */

        memcpy((((void*)&word_to_write)) + padding, ((void*)data) + buffer_offset, bytes_to_copy);



        /* update offset and length variables */

        buffer_offset += bytes_to_copy;

        remaining_length -= bytes_to_copy;



        /* write to flash 32 bits at a time */

        IOWR_32DIRECT(qspi_flash_info->data_base, write_offset, word_to_write);



        /* check whether write triggered a illegal write interrupt */

        if((IORD_ALTERA_QSPI_CONTROLLER2_ISR(qspi_flash_info->csr_base) &

        		ALTERA_QSPI_CONTROLLER2_ISR_ILLEGAL_WRITE_MASK) ==

        				ALTERA_QSPI_CONTROLLER2_ISR_ILLEGAL_WRITE_ACTIVE)

        {

		    /* clear register */

        	IOWR_ALTERA_QSPI_CONTROLLER2_ISR(qspi_flash_info->csr_base,

			ALTERA_QSPI_CONTROLLER2_ISR_ILLEGAL_WRITE_MASK );

        	return -EIO; /** write failed, sector might be protected */

        }



        /* update current offset */

        write_offset = write_offset + sizeof(alt_u32);

    }



    return 0;

}



/**

 * alt_qspi_controller2_write

 *

 * Program the data into the flash at the selected address.

 *

 * The different between this function and alt_qspi_controller2_write_block function

 * is that this function (alt_qspi_controller2_write) will automatically erase a block as needed

 * Arguments:

 * - *flash_info: Pointer to QSPI flash device structure.

 * - offset: Byte offset (unaligned access) of write to flash memory. For best performance, 

 *           word(32 bits - aligned access) offset of write is recommended.

 * - *src_addr: source buffer

 * - length: size of writing

 *  

 * Returns:

 * 0 -> success

 * -EINVAL -> Invalid arguments

 * -EIO -> write failed, sector might be protected 

 *

**/

int alt_qspi_controller2_write(

    alt_flash_dev *flash_info, /** device info */

    int offset, /** offset of write from base address */

    const void *src_addr, /** source buffer */

    int length /** size of writing */

)

{

    alt_32 ret_code = 0;



    alt_qspi_controller2_dev *qspi_flash_info = NULL;



    alt_u32 write_offset = offset; /** address of next byte to write */

    alt_u32 remaining_length = length; /** length of write data left to be written */

    alt_u32 buffer_offset = 0; /** offset into source buffer to get write data */

    alt_u32 i = 0;



    /* return -EINVAL if flash_info and src_addr are NULL */

	if(NULL == flash_info || NULL == src_addr)

    {

    	return -EINVAL;

    }

	

	qspi_flash_info = (alt_qspi_controller2_dev*)flash_info;

	

    /* make sure the write parameters are within the bounds of the flash */

    ret_code = alt_qspi_validate_read_write_arguments(qspi_flash_info, offset, length);



	if(0 != ret_code)

	{

		return ret_code;

	}



    /*

     * This loop erases and writes data one sector at a time. We check for write completion 

     * before starting the next sector.

     */

    for(i = offset/qspi_flash_info->sector_size ; i < qspi_flash_info->number_of_sectors; i++)

    {

        alt_u32 block_offset = 0; /** block offset in byte addressing */

    	alt_u32 offset_within_current_sector = 0; /** offset into current sector to write */

        alt_u32 length_to_write = 0; /** length to write to current sector */



    	if(0 >= remaining_length)

    	{

    		break; /* out of data to write */

    	}



        /* calculate current sector/block offset in byte addressing */

        block_offset = write_offset & ~(qspi_flash_info->sector_size - 1);

           

        /* calculate offset into sector/block if there is one */

        if(block_offset != write_offset)

        {

            offset_within_current_sector = write_offset - block_offset;

        }



        /* erase sector */

        ret_code = alt_qspi_controller2_erase_block(flash_info, block_offset);



        if(0 != ret_code)

        {

            return ret_code;

        }



        /* calculate the byte size of data to be written in a sector */

        length_to_write = MIN(qspi_flash_info->sector_size - offset_within_current_sector, 

                remaining_length);



        /* write data to erased block */

        ret_code = alt_qspi_controller2_write_block(flash_info, block_offset, write_offset,

            src_addr + buffer_offset, length_to_write);





        if(0 != ret_code)

        {

            return ret_code;

        }



        /* update remaining length and buffer_offset pointer */

        remaining_length -= length_to_write;

        buffer_offset += length_to_write;

        write_offset += length_to_write; 

    }



    return ret_code;

}



/**

 * alt_qspi_controller2_read

 *

 * There's no real need to use this function as opposed to using memcpy directly. It does

 * do some sanity checks on the bounds of the read.

 *

 * Arguments:

 * - *flash_info: Pointer to general flash device structure.

 * - offset: offset read from flash memory.

 * - *dest_addr: destination buffer

 * - length: size of reading

 *

 * Returns:

 * 0 -> success

 * -EINVAL -> Invalid arguments

**/

int alt_qspi_controller2_read

(

    alt_flash_dev *flash_info, /** device info */

    int offset, /** offset of read from base address */

    void *dest_addr, /** destination buffer */

    int length /** size of read */

)

{

    alt_32 ret_code = 0;

	alt_qspi_controller2_dev *qspi_flash_info = NULL;

	

	/* return -EINVAL if flash_info and dest_addr are NULL */

	if(NULL == flash_info || NULL == dest_addr)

    {

    	return -EINVAL;

    }

	

    qspi_flash_info = (alt_qspi_controller2_dev*)flash_info;



	/* validate arguments */

	ret_code = alt_qspi_validate_read_write_arguments(qspi_flash_info, offset, length);



	/* copy data from flash to destination address */

	if(0 == ret_code)

	{

		memcpy(dest_addr, (alt_u8*)qspi_flash_info->data_base + offset, length);

	}



    return ret_code;

}



/**

 * altera_qspi_controller2_init

 *

 * alt_sys_init.c will call this function automatically through macro

 *

 * Information in system.h is checked against expected values that are determined by the silicon_id.

 * If the information doesn't match then this system is configured incorrectly. Most likely the wrong

 * type of EPCS or EPCQ/QSPI device was selected when instantiating the soft IP.

 *

 * Arguments:

 * - *flash: Pointer to QSPI flash device structure.

 *

 * Returns:

 * 0 -> success

 * -EINVAL -> Invalid arguments.

 * -ENODEV -> System is configured incorrectly.

**/

alt_32 altera_qspi_controller2_init(alt_qspi_controller2_dev *flash)

{

	alt_u32 silicon_id = 0;

	alt_u32 size_in_bytes = 0;

	alt_u32 number_of_sectors = 0;



    /* return -EINVAL if flash is NULL */

	if(NULL == flash)

    {

    	return -EINVAL;

    }

	

	/* return -ENODEV if CSR slave is not attached */

	if(NULL == (void *)flash->csr_base)

	{

		return -ENODEV;

	}





	/*

	 * If flash is an EPCQ/QSPI device, we read the QSPI_RD_RDID register for the ID

	 * If flash is an EPCS device, we read the QSPI_RD_SID register for the ID

	 *

	 * Whether or not the flash is a EPCQ, QSPI or EPCS is indicated in the system.h. The system.h gets

	 * this value from the hw.tcl of the IP. If this value is set incorrectly, then things will go

	 * badly.

	 *

	 * In both cases, we can determine the number of sectors, which we can use

	 * to calculate a size. We compare that size to the system.h value to make sure

	 * the QSPI soft IP was configured correctly.

	 */

	if(0 == flash->is_epcs)

	{

		/* If we're an EPCQ or QSPI, we read QSPI_RD_RDID for the silicon ID */

		silicon_id = IORD_ALTERA_QSPI_CONTROLLER2_RDID(flash->csr_base);

		silicon_id &= ALTERA_QSPI_CONTROLLER2_RDID_MASK;



		/* Determine which EPCQ/QSPI device so we can figure out the number of sectors */

		/*EPCQ and QSPI share the same ID for the same capacity*/

		switch(silicon_id)

		{

			case ALTERA_QSPI_CONTROLLER2_RDID_QSPI16:

			{

				number_of_sectors = 32;

				break;

			}

			case ALTERA_QSPI_CONTROLLER2_RDID_QSPI32:

			{

				number_of_sectors = 64;

				break;

			}

			case ALTERA_QSPI_CONTROLLER2_RDID_QSPI64:

			{

				number_of_sectors = 128;

				break;

			}

			case ALTERA_QSPI_CONTROLLER2_RDID_QSPI128:

			{

				number_of_sectors = 256;

				break;

			}

			case ALTERA_QSPI_CONTROLLER2_RDID_QSPI256:

			{

				number_of_sectors = 512;

				break;

			}

			case ALTERA_QSPI_CONTROLLER2_RDID_QSPI512:

			{

				number_of_sectors = 1024;

				break;

			}

			case ALTERA_QSPI_CONTROLLER2_RDID_QSPI1024:

			{

				number_of_sectors = 2048;

				break;

			}

			default:

			{

				return -ENODEV;

			}

		}

	}

	else {

		/* If we're an EPCS, we read QSPI_RD_SID for the silicon ID */

		silicon_id = IORD_ALTERA_QSPI_CONTROLLER2_SID(flash->csr_base);

		silicon_id &= ALTERA_QSPI_CONTROLLER2_SID_MASK;



		/* Determine which EPCS device so we can figure out various properties */

		switch(silicon_id)

		{

			case ALTERA_QSPI_CONTROLLER2_SID_EPCS16:

			{

				number_of_sectors = 32;

				break;

			}

			case ALTERA_QSPI_CONTROLLER2_SID_EPCS64:

			{

				number_of_sectors = 128;

				break;

			}

			case ALTERA_QSPI_CONTROLLER2_SID_EPCS128:

			{

				number_of_sectors = 256;

				break;

			}

			default:

			{

				return -ENODEV;

			}

		}

	}



	/* Calculate size of flash based on number of sectors */

	size_in_bytes = number_of_sectors * flash->sector_size;



	/*

	 * Make sure calculated size is the same size given in system.h

	 * Also check number of sectors is the same number given in system.h

	 * Otherwise the QSPI IP was not configured correctly

	 */

	if(	size_in_bytes != flash->size_in_bytes ||

			number_of_sectors != flash->number_of_sectors)

	{

		flash->dev.number_of_regions = 0;

		return -ENODEV;

	}

	else

	{

		flash->silicon_id = silicon_id;

		flash->number_of_sectors = number_of_sectors;



		/*

		 * populate fields of region_info required to conform to HAL API

		 * create 1 region that composed of "number_of_sectors" blocks

		 */

		flash->dev.number_of_regions = 1;

		flash->dev.region_info[0].offset = 0;

		flash->dev.region_info[0].region_size = size_in_bytes;

		flash->dev.region_info[0].number_of_blocks = number_of_sectors;

		flash->dev.region_info[0].block_size = flash->sector_size;

	}





    /*

     * Register this device as a valid flash device type

     *

     * Only register the device if it's configured correctly.

     */

		alt_flash_device_register(&(flash->dev));





    return 0;

}





/*

 *	Private API

 *

 *	Helper functions used by Public API functions.

 *  

 * Arguments:

 * - *flash_info: Pointer to QSPI flash device structure.

 * - offset: Offset of read/write from base address.

 * - length: Length of read/write in bytes.

 *

 * Returns:

 * 0 -> success

 * -EINVAL -> Invalid arguments

 */

/**

 * Used to check that arguments to a read or write are valid

 */

ALT_INLINE alt_32 static alt_qspi_validate_read_write_arguments

(

		alt_qspi_controller2_dev *flash_info, /** device info */

		alt_u32 offset, /** offset of read/write */

		alt_u32 length /** length of read/write */

)

{

    alt_qspi_controller2_dev *qspi_flash_info = NULL;

    alt_u32 start_address = 0;

    alt_32 end_address = 0;

	

  /* return -EINVAL if flash_info is NULL */

   if(NULL == flash_info)

   {

    	return -EINVAL;

   }

	

  qspi_flash_info = (alt_qspi_controller2_dev*)flash_info;



  start_address = qspi_flash_info->data_base + offset; /** first address of read or write */

  end_address = start_address + length; /** last address of read or write (not inclusive) */



  /* make sure start and end address is less then the end address of the flash */

  if(

		  start_address >= qspi_flash_info->data_end ||

		  end_address >= qspi_flash_info->data_end ||

		  offset < 0 ||

		  length < 0

  )

  {

	  return -EINVAL;

  }



  return 0;

}



/*

 * Private function that polls write in progress bit QSPI_RD_STATUS.

 *

 * Write in progress will be set if any of the following operations are in progress:

 * 	-WRITE STATUS REGISTER

 * 	-WRITE NONVOLATILE CONFIGURATION REGISTER

 * 	-PROGRAM

 * 	-ERASE

 *

 * Assumes QSPI was configured correctly.

 *

 * If ALTERA_QSPI_CONTROLLER2_1US_TIMEOUT_VALUE is set, the function will time out after

 * a period of time determined by that value.

 *

 * Arguments:

 * - *qspi_flash_info: Pointer to QSPI flash device structure.

 *  

 * Returns:

 * 0 -> success

 * -EINVAL -> Invalid arguments

 * -ETIME  -> Time out and skipping the looping after 0.7 sec.

 */

alt_32 static alt_qspi_poll_for_write_in_progress(alt_qspi_controller2_dev* qspi_flash_info)

{  

    /* we'll want to implement timeout if a timeout value is specified */

#if ALTERA_QSPI_CONTROLLER2_1US_TIMEOUT_VALUE > 0

	alt_u32 timeout = ALTERA_QSPI_CONTROLLER2_1US_TIMEOUT_VALUE;

	alt_u16 counter = 0;

#endif



    /* return -EINVAL if qspi_flash_info is NULL */

	if(NULL == qspi_flash_info)

    {

    	return -EINVAL;

    }



	/* while Write in Progress bit is set, we wait */

	while((IORD_ALTERA_QSPI_CONTROLLER2_STATUS(qspi_flash_info->csr_base) &

			ALTERA_QSPI_CONTROLLER2_STATUS_WIP_MASK) ==

			ALTERA_QSPI_CONTROLLER2_STATUS_WIP_BUSY)

	{

        alt_busy_sleep(1); /* delay 1us */

#if ALTERA_QSPI_CONTROLLER2_1US_TIMEOUT_VALUE > 0

		if(timeout <= counter )

		{

			return -ETIME;

		}

		

		counter++;

#endif



	}



	return 0;

}





