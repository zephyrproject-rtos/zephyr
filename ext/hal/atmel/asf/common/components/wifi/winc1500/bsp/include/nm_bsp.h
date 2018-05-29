/**
 *
 * \file
 *
 * \brief WINC BSP API Declarations.
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
 
/** \defgroup nm_bsp BSP
 */
/**@defgroup  BSPDefine Defines
 * @ingroup nm_bsp
 * @{
 */
#ifndef _NM_BSP_H_
#define _NM_BSP_H_

#define NMI_API
/*!< 
*        Attribute used to define memory section to map Functions in host memory.
*/
#define CONST const

/*!< 
*     Used for code portability.
*/

#ifndef NULL
#define NULL ((void*)0)
#endif
/*!< 
*    Void Pointer to '0' in case of NULL is not defined. 
*/


#define BSP_MIN(x,y) ((x)>(y)?(y):(x))
/*!< 
*     Computes the minimum of \b x and \b y.
*/

 //@}

/**@defgroup  DataT  DataTypes
 * @ingroup nm_bsp
 * @{
 */
 
/*!
 * @typedef      void (*tpfNmBspIsr) (void);
 * @brief           Pointer to function.\n
 *                     Used as a data type of ISR function registered by \ref nm_bsp_register_isr
 * @return         None
 */
typedef void (*tpfNmBspIsr)(void);
  /*!
 * @ingroup DataTypes
 * @typedef      unsigned char	uint8;
 * @brief        Range of values between 0 to 255
 */
typedef unsigned char	uint8;

 /*!
 * @ingroup DataTypes
 * @typedef      unsigned short	uint16;
 * @brief        Range of values between 0 to 65535
 */
typedef unsigned short	uint16;

 /*!
 * @ingroup Data Types
 * @typedef      unsigned long	uint32;
 * @brief        Range of values between 0 to 4294967295
 */ 
typedef unsigned long	uint32;
  /*!
 * @ingroup Data Types
 * @typedef      signed char		sint8;
 * @brief        Range of values between -128 to 127
 */
typedef signed char		sint8;

 /*!
 * @ingroup DataTypes
 * @typedef      signed short	sint16;
 * @brief        Range of values between -32768 to 32767
 */
typedef signed short	sint16;

  /*!
 * @ingroup DataTypes
 * @typedef      signed long		sint32;
 * @brief        Range of values between -2147483648 to 2147483647
 */

typedef signed long		sint32;
 //@}

#ifndef CORTUS_APP


#ifdef __cplusplus
extern "C"{
#endif

/** \defgroup BSPAPI Function
 *   @ingroup nm_bsp
 */


/** @defgroup NmBspInitFn nm_bsp_init
 *  @ingroup BSPAPI
 *
 *  Initialization for BSP (<strong>B</strong>oard <strong>S</strong>upport <strong>P</strong>ackage) such as Reset and Chip Enable Pins for WINC, delays,
 *  register ISR, enable/disable IRQ for WINC, ...etc. You must use this function in the head of your application to 
 *  enable WINC and Host Driver to communicate with each other. 
 */
 /**@{*/
/*!
 * @fn           sint8 nm_bsp_init(void);
 * @brief		 This function is used to initialize the <strong>B</strong>oard <strong>S</strong>upport <strong>P</strong>ackage <strong>(BSP)</strong> in order to prepare the WINC
 *				 before it start working.
 *
 *				 The nm_bsp_init function is the first function that should be called at the beginning of
 *				 every application to initialize the BSP and the WINC board. Otherwise, the rest of the BSP function
 *				 calls will return with failure. This function should also be called after the WINC has been switched off with 
 *				 a successful call to "nm_bsp_deinit" in order to reinitialize the BSP before the user can use any of the WINC API
 *				 functions again. After the function initialize the WINC. Hard reset must be applied to start the WINC board.
 * @note         Implementation of this function is host dependent.
 * @warning      inappropriate use of this function will lead to unavailability of host-chip communication.\n
 *  
 * @see			 nm_bsp_deinit, nm_bsp_reset
 * @return       The function returns @ref M2M_SUCCESS for successful operations and a negative value otherwise.

 */
sint8 nm_bsp_init(void);
 /**@}*/

 
 /** @defgroup NmBspDeinitFn nm_bsp_deinit
 *    @ingroup BSPAPI
 *   	 De-initialization for BSP ((<strong>B</strong>oard <strong>S</strong>upport <strong>P</strong>ackage)). This function should be called only after
 *		 a successful call to nm_bsp_init. 
 */
 /**@{*/
/*!
 * @fn           sint8 nm_bsp_deinit(void);
 * @pre          The BSP should be initialized through \ref nm_bsp_init first.
 * @brief		 This function is used to de-initialize the BSP and turn off the WINC board.
 *				 
 *				 The nm_bsp_deinit is the last function that should be called after the application has finished and before the WINC is switched 
 *				 off. The function call turns off the WINC board by setting CHIP_EN and RESET_N signals low.Every function call of "nm_bsp_init" should
 *				 be matched with a call to nm_bsp_deinit. Failure to do so may result in the WINC consuming higher power than expected. 
 * @note         Implementation of this function is host dependent.
 * @warning      misuse may lead to unknown behavior in case of soft reset.\n
 * @see          nm_bsp_init               
 * @return      The function returns @ref M2M_SUCCESS for successful operations and a negative value otherwise.

 */
sint8 nm_bsp_deinit(void);
 /**@}*/

 
/** @defgroup NmBspResetFn  nm_bsp_reset
*     @ingroup BSPAPI
*      Resetting WINC1500 SoC by setting CHIP_EN and RESET_N signals low, then after specific delay the function will put CHIP_EN high then RESET_N high,
*      for the timing between signals please review the WINC data-sheet
*/
/**@{*/
 /*!
 * @fn           void nm_bsp_reset(void);    
 * @param [in]   None
 * @brief		 Applies a hardware reset to the WINC board.
 *				 The "nm_bsp_reset" is used to apply a hard reset to the WINC board by setting CHIP_EN and RESET_N signals low, then after specific delay
 *				 the function will put CHIP_EN high then RESET_N high, for the detailed timing between signals please review the WINC data-sheet. After a
 *				 successful call, the WINC board firmware will kick off to load and kick off the WINC firmware. This function should be called to reset the 
 *				 WINC firmware after the BSP is initialized and before the start of any communication with WINC board. Calling this function at any other time
 *				 will result in losing the state and connections saved in the WINC board and starting again from the initial state. The host driver will need 
 * 				 to be de-initialized before calling nm_bsp_reset and initialized again after it using the " m2m_wifi_(de)init". 
 * @pre          Initialize \ref nm_bsp_init first
 * @note         Implementation of this function is host dependent and called by HIF layer.
 * @warning		 Calling this function will drop any connection and internal state saved on the WINC firmware.
 * @see          nm_bsp_init, m2m_wifi_init,  m2m_wifi_deinit
 * @return       None

 */
void nm_bsp_reset(void);
 /**@}*/

 
/** @defgroup NmBspSleepFn nm_bsp_sleep
*     @ingroup BSPAPI
*     Sleep in units of milliseconds.\n
*    This function used by HIF Layer according to different situations. 
*/
/**@{*/
/*!
 * @fn           void nm_bsp_sleep(uint32);
 * @brief   	 Used to put the host to sleep for the specified duration.
 *				 Forcing the host to sleep for extended period may lead to host not being able to respond to WINC board events.It's important to
 *				 be considerate while choosing the sleep period. 
 * @param [in]   u32TimeMsec
 *               Time unit in milliseconds
 * @pre          Initialize \ref nm_bsp_init first
 * @warning      Maximum value must nor exceed 4294967295 milliseconds which is equal to 4294967.295 seconds.\n
 * @note         Implementation of this function is host dependent.
 * @see           nm_bsp_init               
 * @return       None
 */
void nm_bsp_sleep(uint32 u32TimeMsec);
/**@}*/

  
/** @defgroup NmBspRegisterFn nm_bsp_register_isr
*     @ingroup BSPAPI
*   Register ISR (Interrupt Service Routine) in the initialization of HIF (Host Interface) Layer. 
*   When the interrupt trigger the BSP layer should call the pfisr function once inside the interrupt.
*/
/**@{*/
/*!
 * @fn           void nm_bsp_register_isr(tpfNmBspIsr);
 * @param [in]   tpfNmBspIsr  pfIsr
 *               Pointer to ISR handler in HIF
 * @brief		 Register the host interface interrupt service routine.
 *				 WINC board utilize SPI interface to communicate with the host. This function register the SPI interrupt the notify
 *				 the host whenever there is an outstanding message from the WINC board. The function should be called during the initialization
 *				 of the host interface. It an internal driver function and shouldn't be called by the application. 
 * @warning      Make sure that ISR for IRQ pin for WINC is disabled by default in your implementation.
 * @note         Implementation of this function is host dependent and called by HIF layer.
 * @see          tpfNmBspIsr
 * @return       None

 */
void nm_bsp_register_isr(tpfNmBspIsr pfIsr);
/**@}*/

  
/** @defgroup NmBspInterruptCtrl nm_bsp_interrupt_ctrl
*     @ingroup BSPAPI
*    Synchronous enable/disable interrupts function
*/
/**@{*/
/*!
 * @fn           void nm_bsp_interrupt_ctrl(uint8);
 * @pre			 The interrupt must be registered using nm_bsp_register_isr first.
 * @brief        Enable/Disable interrupts
 *				 This function can be used to enable/disable the WINC to host interrupt as the depending on how the driver is implemented.
 *               It an internal driver function and shouldn't be called by the application.
 * @param [in]   u8Enable
 *               '0' disable interrupts. '1' enable interrupts 
 * @see          tpfNmBspIsr, nm_bsp_register_isr     
 * @note         Implementation of this function is host dependent and called by HIF layer.
 * @return       None

 */
void nm_bsp_interrupt_ctrl(uint8 u8Enable);
  /**@}*/

#ifdef __cplusplus
}
#endif

#endif

#ifdef _NM_BSP_BIG_END
#define NM_BSP_B_L_32(x) \
((((x) & 0x000000FF) << 24) + \
(((x) & 0x0000FF00) << 8)  + \
(((x) & 0x00FF0000) >> 8)   + \
(((x) & 0xFF000000) >> 24))
#define NM_BSP_B_L_16(x) \
((((x) & 0x00FF) << 8) + \
(((x)  & 0xFF00) >> 8))
#else
#define NM_BSP_B_L_32(x)  (x)
#define NM_BSP_B_L_16(x)  (x)
#endif


#endif	/*_NM_BSP_H_*/
