/**
 *
 * \file
 *
 * \brief WINC1500 SPI Flash.
 *
 * Copyright (c) 2016-2017 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

/** \defgroup SPIFLASH Spi Flash
 * @file           spi_flash.h
 * @brief          This file describe SPI flash APIs, how to use it and limitations with each one. 
 * @section      Example
 *                 This example illustrates a complete guide of how to use these APIs.
 * @code{.c}
					#include "spi_flash.h"

					#define DATA_TO_REPLACE	"THIS IS A NEW SECTOR IN FLASH"

					int main()
					{
						uint8	au8FlashContent[FLASH_SECTOR_SZ] = {0};
						uint32	u32FlashTotalSize = 0;
						uint32	u32FlashOffset = 0;
	
						ret = m2m_wifi_download_mode();
						if(M2M_SUCCESS != ret)
						{
							printf("Unable to enter download mode\r\n");
						}
						else
						{
							u32FlashTotalSize = spi_flash_get_size();
						}

						while((u32FlashTotalSize > u32FlashOffset) && (M2M_SUCCESS == ret))
						{
							ret = spi_flash_read(au8FlashContent, u32FlashOffset, FLASH_SECTOR_SZ);
							if(M2M_SUCCESS != ret)
							{
								printf("Unable to read SPI sector\r\n");
								break;
							}
							memcpy(au8FlashContent, DATA_TO_REPLACE, strlen(DATA_TO_REPLACE));
		
							ret = spi_flash_erase(u32FlashOffset, FLASH_SECTOR_SZ);
							if(M2M_SUCCESS != ret)
							{
								printf("Unable to erase SPI sector\r\n");
								break;
							}
		
							ret = spi_flash_write(au8FlashContent, u32FlashOffset, FLASH_SECTOR_SZ);
							if(M2M_SUCCESS != ret)
							{
								printf("Unable to write SPI sector\r\n");
								break;
							}
							u32FlashOffset += FLASH_SECTOR_SZ;
						}
	
						if(M2M_SUCCESS == ret)
						{
							printf("Successful operations\r\n");
						}
						else
						{
							printf("Failed operations\r\n");
						}
	
						while(1);
						return M2M_SUCCESS;
					}
 * @endcode  
 */

#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__
#include "common/include/nm_common.h"
#include "bus_wrapper/include/nm_bus_wrapper.h"
#include "driver/source/nmbus.h"
#include "driver/source/nmasic.h"

/**
 *	@fn		spi_flash_enable
 *	@brief	Enable spi flash operations
 *	@version	1.0
 */
sint8 spi_flash_enable(uint8 enable);
/** \defgroup SPIFLASHAPI Function
 *   @ingroup SPIFLASH
 */

 /** @defgroup SPiFlashGetFn spi_flash_get_size
 *  @ingroup SPIFLASHAPI
 */
  /**@{*/
/*!
 * @fn             uint32 spi_flash_get_size(void);
 * @brief         Returns with \ref uint32 value which is total flash size\n
 * @note         Returned value in Mb (Mega Bit).
 * @return      SPI flash size in case of success and a ZERO value in case of failure.
 */
uint32 spi_flash_get_size(void);
 /**@}*/

  /** @defgroup SPiFlashRead spi_flash_read
 *  @ingroup SPIFLASHAPI
 */
  /**@{*/
/*!
 * @fn             sint8 spi_flash_read(uint8 *, uint32, uint32);
 * @brief          Read a specified portion of data from SPI Flash.\n
 * @param [out]    pu8Buf
 *                 Pointer to data buffer which will fill in with data in case of successful operation.
 * @param [in]     u32Addr
 *                 Address (Offset) to read from at the SPI flash.
 * @param [in]     u32Sz
 *                 Total size of data to be read in bytes
 * @warning	       
 *                 - Address (offset) plus size of data must not exceed flash size.\n
 *                 - No firmware is required for reading from SPI flash.\n
 *                 - In case of there is a running firmware, it is required to pause your firmware first 
 *                   before any trial to access SPI flash to avoid any racing between host and running firmware on bus using 
 *                   @ref m2m_wifi_download_mode
 * @note           
 *                 - It is blocking function\n
 * @sa             m2m_wifi_download_mode, spi_flash_get_size
 * @return        The function returns @ref M2M_SUCCESS for successful operations  and a negative value otherwise.
 */
sint8 spi_flash_read(uint8 *pu8Buf, uint32 u32Addr, uint32 u32Sz);
 /**@}*/

  /** @defgroup SPiFlashWrite spi_flash_write
 *  @ingroup SPIFLASHAPI
 */
  /**@{*/
/*!
 * @fn             sint8 spi_flash_write(uint8 *, uint32, uint32);
 * @brief          Write a specified portion of data to SPI Flash.\n
 * @param [in]     pu8Buf
 *                 Pointer to data buffer which contains the required to be written.
 * @param [in]     u32Offset
 *                 Address (Offset) to write at the SPI flash.
 * @param [in]     u32Sz
 *                 Total number of size of data bytes
 * @note           
 *                 - It is blocking function\n
 *                 - It is user's responsibility to verify that data has been written successfully 
 *                   by reading data again and compare it with the original.
 * @warning	       
 *                 - Address (offset) plus size of data must not exceed flash size.\n
 *                 - No firmware is required for writing to SPI flash.\n
 *                 - In case of there is a running firmware, it is required to pause your firmware first 
 *                   before any trial to access SPI flash to avoid any racing between host and running firmware on bus using 
 *                   @ref m2m_wifi_download_mode.
 *                 - Before writing to any section, it is required to erase it first.
 * @sa             m2m_wifi_download_mode, spi_flash_get_size, spi_flash_erase
 * @return       The function returns @ref M2M_SUCCESS for successful operations  and a negative value otherwise.
 
 */
sint8 spi_flash_write(uint8* pu8Buf, uint32 u32Offset, uint32 u32Sz);
 /**@}*/

  /** @defgroup SPiFlashErase spi_flash_erase
 *  @ingroup SPIFLASHAPI
 */
  /**@{*/
/*!
 * @fn             sint8 spi_flash_erase(uint32, uint32);
 * @brief          Erase a specified portion of SPI Flash.\n
 * @param [in]     u32Offset
 *                 Address (Offset) to erase from the SPI flash.
 * @param [in]     u32Sz
 *                 Size of SPI flash required to be erased.
 * @note         It is blocking function \n  
* @warning	       
*                 - Address (offset) plus size of data must not exceed flash size.\n
*                 - No firmware is required for writing to SPI flash.\n
 *                 - In case of there is a running firmware, it is required to pause your firmware first 
 *                   before any trial to access SPI flash to avoid any racing between host and running firmware on bus using 
 *                   @ref m2m_wifi_download_mode
 *                 - It is blocking function\n
 * @sa             m2m_wifi_download_mode, spi_flash_get_size
 * @return       The function returns @ref M2M_SUCCESS for successful operations  and a negative value otherwise.

 */
sint8 spi_flash_erase(uint32 u32Offset, uint32 u32Sz);
 /**@}*/
#endif	//__SPI_FLASH_H__
