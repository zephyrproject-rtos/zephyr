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

#ifndef __LMEM_H__
#define __LMEM_H__

#include <stdint.h>
#include <assert.h>
#include "device_imx.h"

/*!
 * @addtogroup lmem_driver
 * @{
 */

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Processor System Cache control functions
 * @{
 */

/*!
 * @brief This function enable the System Cache.
 *
 * @param base LMEM base pointer.
 */
void LMEM_EnableSystemCache(LMEM_Type *base);

/*!
 * @brief This function disable the System Cache.
 *
 * @param base LMEM base pointer.
 */
void LMEM_DisableSystemCache(LMEM_Type *base);

/*!
 * @brief This function flush the System Cache.
 *
 * @param base LMEM base pointer.
 */
void LMEM_FlushSystemCache(LMEM_Type *base);

/*!
 * @brief This function is called to flush the System Cache by performing cache copy-backs.
 *        It must determine how many cache lines need to be copied back and then
 *        perform the copy-backs.
 *
 * @param base LMEM base pointer.
 * @param address The start address of cache line.
 * @param length The length of flush address space.
 */
void LMEM_FlushSystemCacheLines(LMEM_Type *base, void *address, uint32_t length);

/*!
 * @brief This function invalidate the System Cache.
 *
 * @param base LMEM base pointer.
 */
void LMEM_InvalidateSystemCache(LMEM_Type *base);

/*!
 * @brief This function is responsible for performing an System Cache invalidate.
 *        It must determine how many cache lines need to be invalidated and then
 *        perform the invalidation.
 *
 * @param base LMEM base pointer.
 * @param address The start address of cache line.
 * @param length The length of invalidate address space.
 */
void LMEM_InvalidateSystemCacheLines(LMEM_Type *base, void *address, uint32_t length);

/*@}*/

/*!
 * @name Processor Code Cache control functions
 * @{
 */

/*!
 * @brief This function enable the Code Cache.
 *
 * @param base LMEM base pointer.
 */
void LMEM_EnableCodeCache(LMEM_Type *base);

/*!
 * @brief This function disable the Code Cache.
 *
 * @param base LMEM base pointer.
 */
void LMEM_DisableCodeCache(LMEM_Type *base);

/*!
 * @brief This function flush the Code Cache.
 *
 * @param base LMEM base pointer.
 */
void LMEM_FlushCodeCache(LMEM_Type *base);

/*!
 * @brief This function is called to flush the Code Cache by performing cache copy-backs.
 *        It must determine how many cache lines need to be copied back and then
 *        perform the copy-backs.
 *
 * @param base LMEM base pointer.
 * @param address The start address of cache line.
 * @param length The length of flush address space.
 */
void LMEM_FlushCodeCacheLines(LMEM_Type *base, void *address, uint32_t length);

/*!
 * @brief This function invalidate the Code Cache.
 *
 * @param base LMEM base pointer.
 */
void LMEM_InvalidateCodeCache(LMEM_Type *base);

/*!
 * @brief This function is responsible for performing an Code Cache invalidate.
 *        It must determine how many cache lines need to be invalidated and then
 *        perform the invalidation.
 *
 * @param base LMEM base pointer.
 * @param address The start address of cache line.
 * @param length The length of invalidate address space.
 */
void LMEM_InvalidateCodeCacheLines(LMEM_Type *base, void *address, uint32_t length);

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* __LMEM_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/
