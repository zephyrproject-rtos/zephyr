/**
 * Copyright (c) 2017 IpTronix
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 *  Initialization for BSP such as Reset and Chip Enable Pins for WINC, delays, register ISR, enable/disable IRQ for WINC, ...etc. You must use this function in the head of your application to
 *  enable WINC and Host Driver communicate each other.
 */

#ifndef _NM_BSP_H_
#define _NM_BSP_H_

#include <zephyr/types.h>

#define NMI_API

/*!
 * @fn           sint8 nm_bsp_init(void);
 * @note         Implementation of this function is host dependent.
 * @warning      Missing use will lead to unavailability of host communication.\n
 *
 * @return       The function returns @ref M2M_SUCCESS for successful operations and a negative value otherwise.
 */
s8_t nm_bsp_init(void);

/*!
 *       De-initialization for BSP (Board Support Package)
 * @fn           sint8 nm_bsp_deinit(void);
 * @pre          Initialize \ref nm_bsp_init first
 * @note         Implementation of this function is host dependent.
 * @warning      Missing use may lead to unknown behavior in case of soft reset.\n
 * @see          nm_bsp_init
 * @return      The function returns 0 for successful operations and
 *              a negative value otherwise.
 */
s8_t nm_bsp_deinit(void);

/*!
 * Resetting NMC1500 SoC by setting CHIP_EN and RESET_N signals low,
 * then after specific delay the function will put CHIP_EN high then
 * RESET_N high, for the timing between signals please review the
 * WINC data-sheet
 *
 * @fn           void nm_bsp_reset(void);
 * @param [in]   None
 * @pre          Initialize \ref nm_bsp_init first
 * @note         Implementation of this function is host dependent and called by HIF layer.
 * @see          nm_bsp_init
 * @return       None
 */
void nm_bsp_reset(void);

/*!
 * Sleep in units of milliseconds.\n
 * This function used by HIF Layer according to different situations.
 * @fn           void nm_bsp_sleep(uint32);
 * @brief
 * @param [in]   u32TimeMsec
 *               Time unit in milliseconds
 * @pre          Initialize \ref nm_bsp_init first
 * @warning      Maximum value must nor exceed 4294967295 milliseconds
 *                 which is equal to 4294967.295 seconds.\n
 * @note         Implementation of this function is host dependent.
 * @see           nm_bsp_init
 * @return       None
 */
void nm_bsp_sleep(u32_t time_msec);

/*!
 * Register ISR (Interrupt Service Routine) in the initialization
 * of HIF (Host Interface) Layer.
 * When the interrupt trigger the BSP layer should call the isr_fun
 * function once inside the interrupt.
 * @fn           void nm_bsp_register_isr(tpfNmBspIsr);
 * @param [in]   tpfNmBspIsr  pfIsr
 *               Pointer to ISR handler in HIF
 * @warning      Make sure that ISR for IRQ pin for WINC is disabled by
 *               default in your implementation.
 * @note         Implementation of this function is host dependent and
 *               called by HIF layer.
 * @see          tpfNmBspIsr
 * @return       None
 */
void nm_bsp_register_isr(void (*isr_fun)(void));

/*!
 * Synchronous enable/disable interrupts function
 * @fn           void nm_bsp_interrupt_ctrl(uint8);
 * @brief        Enable/Disable interrupts
 * @param [in]   u8Enable
 *               '0' disable interrupts. '1' enable interrupts
 * @see          tpfNmBspIsr
 * @note         Implementation of this function is host dependent and
 *               called by HIF layer.
 * @return       None
 */
void nm_bsp_interrupt_ctrl(u8_t enable);

#endif  /*_NM_BSP_H_*/
