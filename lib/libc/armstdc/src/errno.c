/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief System error numbers
 *        Error codes returned by functions.
 *        Includes a list of those defined by IEEE Std 1003.1-2017.
 *
 *        This file is supposed to be used together with ARMClang and
 *        #define _AEABI_PORTABILITY_LEVEL 1
 *        or
 *        -D_AEABI_PORTABILITY_LEVEL=1
 *
 *        For details, please refer to the document:
 *        'C Library ABI for the Arm® Architecture, 2021Q1'
 *
 * @defgroup system_errno Error numbers
 * @{
 */

const int __aeabi_EDOM = 33;    /**< Argument too large */
const int __aeabi_ERANGE = 34;  /**< Result too large */
const int __aeabi_EILSEQ = 138; /**< Illegal byte sequence */
