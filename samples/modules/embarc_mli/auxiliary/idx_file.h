/*
 * Copyright 2019-2020, Synopsys, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-3-Clause license found in
 * the LICENSE file in the root directory of this source tree.
 *
 */

#ifndef _IDX_FILE_H
#define _IDX_FILE_H
/**
 * @file Module for IDX files input/output opeartions
 * @brief IDX file format originally was used for MNIST database. For more details see:
 *           http://yann.lecun.com/exdb/mnist/
 */

#include <stdint.h>
#include <stdio.h>
#include "../example_cifar10_caffe/src/cifar10_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @enum IDX file format data codes */
typedef enum {
	IDX_DT_UBYTE_1B = 0x08, /**< Unsigned byte (uint8_t) */
	IDX_DT_BYTE_1B = 0x09, /**< Signed byte (int8_t) */
	IDX_DT_SHORT_2B = 0x0B, /**< Signed short (int16_t) */
	IDX_DT_INT_4B = 0x0C, /**< Signed int (int32_t) */
	IDX_DT_FLOAT_4B = 0x0D, /**< 32bit float  (float) */
	IDX_DT_DOUBLE_8B = 0x0E /**< double precision float (double) */
} tIdxDataType;

/** @enum Function Return codes  */
typedef enum {
	IDX_ERR_NONE = 0x0, /**< No error occurred */
	IDX_ERR_FILE_ACC, /**< File Access Error */
	IDX_ERR_INCORR_HEAD, /**< Incorrect header of file */
	IDX_ERR_INCORR_FILE, /**< Header/file content mismatch */
	IDX_ERR_INCORR_FUNC_INPUT, /**< Function aruments error */
	IDX_ERR_NOT_ENOUGH_MEM /**< Not enough memory for reading/transform */
} tIdxRetVal;

/** @struct IDX file descriptor  */
typedef struct {
	uint32_t num_elements; /**< Number of elements (depending in operation type) */
	uint8_t num_dim; /**< Number of array dimensions */
	tIdxDataType data_type; /**< Basic element data type */

	FILE *opened_file;
/**< File descriptor. Must be opened binary for reading or writing (depends on target operation)*/
} tIdxDescr;

/*
 * Functions for complete reading/writing ,
 * including file manipulation
 */

/** @brief Read data from IDX file completely
 *
 * @detail Function opens file, read shape and all data from it to pre-allocated arrays.
 * If file is opened inside function, It it will be closed in any case (on success or on error).
 *
 * @param[in] path_ - Path to IDX file
 * @param[out] data_ - Pointer to pre-allocated array of sufficient size
 * @param[in/out] data_sz_ - Array size. Will be changed to the total number of elements was read by function.
 *                           If there is not enough data in array this value will contain required size.
 * @param[out] shape_ - IDX array shape
 * @param[in/out] shape_dims_ - Shape array size. Will be changed to the total number of dimensions was read by functions.
 *                              If there is not enough data in array this value will contain required size.
 * @param[out] el_type_ - Basic element type storedd in input IDX file
 *
 *
 * @return Operation status code (tIdxRetVal)
 */
tIdxRetVal idx_file_read_completely(const char *path_, void *data_, uint32_t *data_sz_,
				    uint32_t *shape_, uint32_t *shape_dims_,
				    tIdxDataType *el_type_);

/**
 * @brief Write data to IDX file completely
 *
 * @detail Function writes both: header of IDX file and data according to it's arguments
 * If file is opened inside function, It it will be closed in any case (on success or on error).
 *
 * @param[in] path_ - Output IDX file path
 * @param[in] data_ - Pointer to array for writing
 * @param[out] shape_ - Pointer to array with shape for writing
 * @param[in] shape_sz_ - Number of dimensions in the shape_ array
 * @param[] el_type_ - Basic element type storedd in data_ array
 *
 *
 * @return Operation status code (tIdxRetVal)
 */
tIdxRetVal idx_file_write_completely(const char *path_, void *data_, uint32_t *shape_,
				     uint32_t shape_sz_, tIdxDataType el_type);

/* -------------------------------------------------------------------------- */
/*                    Functions for manual reading/writing                    */
/* -------------------------------------------------------------------------- */

/**
 * @brief Define basic element size of type_
 *
 * @param[in] type_ - IDX file data type
 *
 * @return Size of basic element in bytes. Returns 0 if type is unknown.
 */
uint8_t data_elem_size(tIdxDataType type_);

/**
 * @brief Check consistency of opened IDX file and fill some descriptor fields
 *
 * @detail Function analyses opened IDX file and fills next fields of descriptor: num_dim, data_type, num_elements
 *
 * @param[in] descr_ - Descriptor of IDX file with correctly opened file (user responsibility)
 *
 * @return Operation status code (tIdxRetVal)
 */
tIdxRetVal idx_file_check_and_get_info(tIdxDescr *descr_);

/**
 * @brief Check consistency of compiled array file and fill some descriptor fields
 *
 * @detail Function analyses opened array file and fills next fields of descriptor:
 *         num_dim, data_type, num_elements
 *
 * @param[in] descr_ - Descriptor of array file with correctly opened file (user responsibility)
 *
 * @param[in] target - Pointer to array
 *
 * @return No return value
 */
void array_file_check_and_get_info(tIdxDescr *descr_, tIdxArrayFlag *target);

/**
 * @brief Partial data reading from opened IDX file
 *
 * @detail Function reads descr_->num_elements values from file sequentially. Position inside file will be changed accordingly.
 *         If shape_ pointer is not NULL then read dimensions and data from the beginig of the file .
 *         Else continue file reading from current position.
 *
 * @param[in] descr_ - Descriptor of IDX file with correctly opened file (user responsibility)
 * @param[in] data_ - Pointer to pre-allocated array of sufficient size (for storing descr_->num_elements values)
 * @param[in] shape_ - Pointer to array with shape. If not NULL - will be filled according to IDX file header
 *
 * @return Operation status code (tIdxRetVal)
 */
tIdxRetVal idx_file_read_data(tIdxDescr *descr_, void *data_, uint32_t *shape_);

/**
 * @brief Partial data reading from opened array file
 *
 * @detail Function reads descr_->num_elements values from file sequentially.
 *         Position inside file will be changed accordingly.
 *         If shape_ pointer is not NULL
 *         then read dimensions and data from the beginig of the file .
 *         Else continue file reading from current position.
 *
 * @param[in] descr_ - Descriptor of array file (user responsibility)
 * @param[in] data_ - Pointer to pre-allocated array of sufficient size
 * @param[in] shape_ - Pointer to array with shape.
 * @param[in] target - Pointer to array.
 *
 * @return No return value
 */
void array_file_read_data(tIdxDescr *descr_, void *data_, uint32_t *shape_, tIdxArrayFlag *target);

/**
 * @brief Write IDX file from input array
 *
 * @detail Writes data to file.
 *          In comparison with idx_file_write_completely it writes only data and fill placeholder for header which
 *          must be filled separately by idx_file_write_header function.
 *          Function writes descr_->num_elements values to file sequentially. Position inside file will be changed accordingly.
 *
 * @param[in] descr_ - Descriptor of IDX file with correctly opened file (user responsibility)
 * @param[in] data_ - Pointer to array for writing
 *
 * @return Operation status code (tIdxRetVal)
 */
tIdxRetVal idx_file_write_data(tIdxDescr *descr_, const void *data_);

/**
 * @brief Fill IDX file header according to provided parameters
 *
 * @detail Function move position of file to the beginning and writes header according to provided data.
 *         Position will be set to the current end of file afterward.
 *
 * @param[in] descr_ - Descriptor of IDX file with correctly opened file (user responsibility)
 * @param[in] shape_ - Pointer to array with shape.
 *
 * @return Operation status code (tIdxRetVal)
 */
tIdxRetVal idx_file_write_header(const tIdxDescr *descr_, const uint32_t *shape_);

#ifdef __cplusplus
}
#endif

#endif /* _IDX_FILE_H */
