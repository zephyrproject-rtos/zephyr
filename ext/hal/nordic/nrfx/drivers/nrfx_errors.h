/*
 * Copyright (c) 2017 - 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRFX_ERRORS_H__
#define NRFX_ERRORS_H__

#if !NRFX_CHECK(NRFX_CUSTOM_ERROR_CODES)

/**
 * @defgroup nrfx_error_codes Global Error Codes
 * @{
 * @ingroup nrfx
 *
 * @brief Global error code definitions.
 */

/** @brief Base number of error codes. */
#define NRFX_ERROR_BASE_NUM         0x0BAD0000

/** @brief Base number of driver error codes. */
#define NRFX_ERROR_DRIVERS_BASE_NUM (NRFX_ERROR_BASE_NUM + 0x10000)

/** @brief Enumerated type for error codes. */
typedef enum {
    NRFX_SUCCESS                    = (NRFX_ERROR_BASE_NUM + 0),  ///< Operation performed successfully.
    NRFX_ERROR_INTERNAL             = (NRFX_ERROR_BASE_NUM + 1),  ///< Internal error.
    NRFX_ERROR_NO_MEM               = (NRFX_ERROR_BASE_NUM + 2),  ///< No memory for operation.
    NRFX_ERROR_NOT_SUPPORTED        = (NRFX_ERROR_BASE_NUM + 3),  ///< Not supported.
    NRFX_ERROR_INVALID_PARAM        = (NRFX_ERROR_BASE_NUM + 4),  ///< Invalid parameter.
    NRFX_ERROR_INVALID_STATE        = (NRFX_ERROR_BASE_NUM + 5),  ///< Invalid state, operation disallowed in this state.
    NRFX_ERROR_INVALID_LENGTH       = (NRFX_ERROR_BASE_NUM + 6),  ///< Invalid length.
    NRFX_ERROR_TIMEOUT              = (NRFX_ERROR_BASE_NUM + 7),  ///< Operation timed out.
    NRFX_ERROR_FORBIDDEN            = (NRFX_ERROR_BASE_NUM + 8),  ///< Operation is forbidden.
    NRFX_ERROR_NULL                 = (NRFX_ERROR_BASE_NUM + 9),  ///< Null pointer.
    NRFX_ERROR_INVALID_ADDR         = (NRFX_ERROR_BASE_NUM + 10), ///< Bad memory address.
    NRFX_ERROR_BUSY                 = (NRFX_ERROR_BASE_NUM + 11), ///< Busy.
    NRFX_ERROR_ALREADY_INITIALIZED  = (NRFX_ERROR_BASE_NUM + 12), ///< Module already initialized.

    NRFX_ERROR_DRV_TWI_ERR_OVERRUN  = (NRFX_ERROR_DRIVERS_BASE_NUM + 0), ///< TWI error: Overrun.
    NRFX_ERROR_DRV_TWI_ERR_ANACK    = (NRFX_ERROR_DRIVERS_BASE_NUM + 1), ///< TWI error: Address not acknowledged.
    NRFX_ERROR_DRV_TWI_ERR_DNACK    = (NRFX_ERROR_DRIVERS_BASE_NUM + 2)  ///< TWI error: Data not acknowledged.
} nrfx_err_t;

/** @} */

#endif // !NRFX_CHECK(NRFX_CUSTOM_ERROR_CODES)

#endif // NRFX_ERRORS_H__
