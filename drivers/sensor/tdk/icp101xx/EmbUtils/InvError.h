/*
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @defgroup InvError Error code
 *	@brief    Common error code
 *
 *	@ingroup EmbUtils
 *	@{
 */

#ifndef _INV_ERROR_H_
#define _INV_ERROR_H_

/** @brief Common error code definition
 */
enum inv_error {
	INV_ERROR_SUCCESS = 0,         /**< no error */
	INV_ERROR = -1,                /**< unspecified error */
	INV_ERROR_NIMPL = -2,          /**< function not implemented for given arguments */
	INV_ERROR_TRANSPORT = -3,      /**< error occurred at transport level */
	INV_ERROR_TIMEOUT = -4,        /**< action did not complete in the expected time window */
	INV_ERROR_SIZE = -5,           /**< Arg length is too big */
	INV_ERROR_OS = -6,             /**< error related to OS */
	INV_ERROR_IO = -7,             /**< error related to IO operation */
	INV_ERROR_MEM = -9,            /**< not enough memory to complete requested action */
	INV_ERROR_HW = -10,            /**< error at HW level */
	INV_ERROR_BAD_ARG = -11,       /**< Wrong argument */
	INV_ERROR_UNEXPECTED = -12,    /**< something unexpected happened */
	INV_ERROR_FILE = -13,          /**< cannot access file or unexpected format */
	INV_ERROR_PATH = -14,          /**< invalid file path */
	INV_ERROR_IMAGE_TYPE = -15,    /**< error when image type is not managed */
	INV_ERROR_WATCHDOG = -16,      /**< error when device doesn't respond to ping */
	INV_ERROR_FIFO_OVERFLOW = -17, /**< FIFO overflow detected */
};

#endif /* _INV_ERROR_H_ */

/** @} */
