/*
* Copyright 2019-2020, Synopsys, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-3-Clause license found in
* the LICENSE file in the root directory of this source tree.
*
*/

#include <string.h>

#include "idx_file.h"

// Pre-defined sizes of buffers and values
#define HEADER_ELEM_SZ 4U
#define READ_BUF_SIZE 64U

// Internal buffer for file transfers
static uint8_t buffer[READ_BUF_SIZE];

//============================================================
// Convert Big Endian byte array to values for current machine
//============================================================
static void bin2val(void *out_data_, uint8_t *in_data, uint8_t type_sz, uint32_t el_count) {
    uint32_t i = 0;
    uint32_t j = 0;

    switch(type_sz) {
    case 1:
        memcpy(out_data_, (void*)in_data, el_count);
        break;
    case 2:
        for (; i < el_count; ++i, j += type_sz)
            ((uint16_t *)out_data_)[i] = (uint16_t)(in_data[j]) << 8 | (uint16_t)(in_data[j+1]);
        break;
    case 4:
        for (; i < el_count; ++i, j += type_sz)
            ((uint32_t *)out_data_)[i] = (uint32_t)(in_data[j]) << 24 | (uint32_t)(in_data[j+1]) << 16|
                                        (uint32_t)(in_data[j+2]) << 8 | (uint32_t)(in_data[j+3]);
        break;
    case 8:
        for (; i < el_count; ++i, j += type_sz)
            ((uint64_t *)out_data_)[i] = (uint64_t)(in_data[j]) << 56  | (uint64_t)(in_data[j+1]) << 48 |
                                        (uint64_t)(in_data[j+2]) << 40 | (uint64_t)(in_data[j+3]) << 32 |
                                        (uint64_t)(in_data[j+4]) << 24 | (uint64_t)(in_data[j+5]) << 16 |
                                        (uint64_t)(in_data[j+6]) << 8 | (uint64_t)(in_data[j+7]);
        break;
    }
    return;
}

//============================================================
// Convert Values of current machine to Big Endian byte array
//============================================================
static void val2BEbin(uint8_t *out_data_, void *in_data, uint8_t type_sz, uint32_t el_count) {
    uint32_t i = 0;
    uint32_t j = 0;
    static uint16_t val16;
    static uint32_t val32;
    static uint64_t val64;

    switch(type_sz) {
    case 1:
        memcpy(out_data_, (void*)in_data, el_count);
        break;
    case 2:
        for (; i < el_count; ++i, j += type_sz) {
            val16 = ((uint16_t *)in_data)[i];
            out_data_[j+1] = (uint8_t)(val16 & 0xFF); val16 >>= 8;
            out_data_[j] = (uint8_t)(val16 & 0xFF);
        }
        break;
    case 4:
        for (; i < el_count; ++i, j += type_sz) {
            val32 = ((uint32_t *)in_data)[i];
            out_data_[j+3] = (uint8_t)(val32 & 0xFF); val32 >>= 8; out_data_[j+2] = (uint8_t)(val32 & 0xFF); val32 >>= 8;
            out_data_[j+1] = (uint8_t)(val32 & 0xFF); val32 >>= 8; out_data_[j] = (uint8_t)(val32 & 0xFF);
        }
        break;
    case 8:
        for (; i < el_count; ++i, j += type_sz) {
            val64 = ((uint64_t *)in_data)[i];
            out_data_[j+7] = (uint8_t)(val64 & 0xFF); val64 >>= 8; out_data_[j+6] = (uint8_t)(val64 & 0xFF); val64 >>= 8;
            out_data_[j+5] = (uint8_t)(val64 & 0xFF); val64 >>= 8; out_data_[j+4] = (uint8_t)(val64 & 0xFF);val64 >>= 8;
            out_data_[j+3] = (uint8_t)(val64 & 0xFF); val64 >>= 8; out_data_[j+2] = (uint8_t)(val64 & 0xFF); val64 >>= 8;
            out_data_[j+1] = (uint8_t)(val64 & 0xFF); val64 >>= 8; out_data_[j] = (uint8_t)(val64 & 0xFF);
        }
        break;
    }
    return;
}


//====================================================
// Get size of element for type_class
//====================================================
uint8_t data_elem_size(tIdxDataType type_) {
    switch(type_) {
    case IDX_DT_UBYTE_1B:
    case IDX_DT_BYTE_1B:
        return 1;

    case IDX_DT_SHORT_2B:
        return 2;

    case IDX_DT_INT_4B:
    case IDX_DT_FLOAT_4B:
        return 4;

    case IDX_DT_DOUBLE_8B:
        return 8;

    default:
        return 0;
    }
    return 0;
}

//=======================================================================
// Check IDX file and Get part of it descripton for correct further reading
//=======================================================================
tIdxRetVal idx_file_check_and_get_info(tIdxDescr *descr_) {
    fpos_t position;
    uint8_t type, shapes_num;
    uint32_t elements_num = 1;
    uint32_t elements_size = 1;
    uint32_t dim;
    size_t i, file_size, rd_bytes;
    long ftell_ret;


    // Remember current position in file
    if (fgetpos(descr_->opened_file, &position) != 0)
        return IDX_ERR_FILE_ACC;

    // Obtain file size:
    rewind(descr_->opened_file);
    fseek (descr_->opened_file, 0 , SEEK_END);
    ftell_ret = ftell (descr_->opened_file);
    if (ftell_ret < 0)
        return IDX_ERR_FILE_ACC;
    file_size = (size_t)ftell_ret;
    rewind(descr_->opened_file);

    // Reading Magic Number and parse it
    if (HEADER_ELEM_SZ != fread((void*)buffer, 1, HEADER_ELEM_SZ, descr_->opened_file))
        return IDX_ERR_FILE_ACC;
    type = buffer[2];
    shapes_num = buffer[3];
    elements_size = data_elem_size((tIdxDataType)type);
    if (file_size < HEADER_ELEM_SZ * (shapes_num + 1) || elements_size == 0)
        return IDX_ERR_INCORR_HEAD;

    // Sequential demensions size reading
    rd_bytes = 0;
    for (i = 0; i < shapes_num; ++i) {
        rd_bytes += fread((void*)buffer, 1, HEADER_ELEM_SZ, descr_->opened_file);
        bin2val((void *)&dim, buffer, HEADER_ELEM_SZ, 1);
        elements_num *= dim;
    }

    // Check correctness of sizes
    if (rd_bytes != HEADER_ELEM_SZ * shapes_num)
        return IDX_ERR_INCORR_FILE;
    if (file_size != rd_bytes + HEADER_ELEM_SZ + (elements_num * elements_size))
        return IDX_ERR_INCORR_FILE;

    // Restore position in file and write back idx info
    if (fsetpos(descr_->opened_file, &position) != 0)
        return IDX_ERR_FILE_ACC;
    descr_->data_type = (tIdxDataType)type;
    descr_->num_dim = shapes_num;
    descr_->num_elements = elements_num;
    return IDX_ERR_NONE;
}

//=================================================
// Read data from IDX file according to filled descriptor
//=================================================
// REMARK: If shape pointer is not NULL then read dimensions and data from file beginig
// Else continue file reading from current position.
tIdxRetVal idx_file_read_data(tIdxDescr *descr_, void *data_, uint32_t *shape_) {
    const uint8_t elements_size = data_elem_size(descr_->data_type);
    const uint32_t max_elem_in_buf = READ_BUF_SIZE / elements_size;
    uint8_t *data_ptr = (uint8_t *)data_;
    size_t count;
    uint32_t i, elem_in_portion;

    // If shape required than we start reading file from begining and read shape
    // In other case we continue file reading from current position.
    if (shape_ != NULL) {
        fseek (descr_->opened_file, HEADER_ELEM_SZ, SEEK_SET);
        for (i = 0; i < descr_->num_dim; ++i) {
            if (fread((void*)buffer, 1, HEADER_ELEM_SZ, descr_->opened_file) != HEADER_ELEM_SZ)
                return IDX_ERR_FILE_ACC;
            bin2val((void *)&shape_[i], buffer, HEADER_ELEM_SZ, 1);
        }
    }

    // Partial reading
    i = descr_->num_elements; // total number of elemeants that will be read in this session.
    while (i > 0) {
        elem_in_portion = (i > max_elem_in_buf)? max_elem_in_buf : i;
        count = elem_in_portion * elements_size;

        if (fread((void*)buffer, 1, count, descr_->opened_file) != count)
            return IDX_ERR_FILE_ACC;
        bin2val((void *)data_ptr, buffer, elements_size, elem_in_portion);

        data_ptr += count;
        i -= elem_in_portion;
    }
    return IDX_ERR_NONE;
}

//============================================================
// Write IDX file from source (full file in 1 operation in contrast with reading)
//============================================================
tIdxRetVal idx_file_write(tIdxDescr *descr_, const void *data_, uint32_t *shape_) {
    uint32_t i, elem_in_portion;
    const uint8_t elements_size = data_elem_size(descr_->data_type);
    uint32_t max_elem_in_buf;
    size_t count;
    uint8_t *data_ptr = (uint8_t *)data_;

    // Write formatted magic number in header
    buffer[0] = buffer[1] = 0;
    buffer[2] = descr_->data_type;
    buffer[3] = descr_->num_dim;
    if (fwrite((void*)buffer, 1, HEADER_ELEM_SZ, descr_->opened_file) != HEADER_ELEM_SZ)
        return IDX_ERR_FILE_ACC;

    // Write dimensions in header
    i = descr_->num_dim;
    max_elem_in_buf = READ_BUF_SIZE / HEADER_ELEM_SZ;
    while (i > 0) {
        elem_in_portion = (i > max_elem_in_buf)? max_elem_in_buf : i;
        val2BEbin(buffer, (void*)shape_, HEADER_ELEM_SZ, elem_in_portion);

        count = elem_in_portion * HEADER_ELEM_SZ;
        if (fwrite((void*)buffer, 1, count, descr_->opened_file) != count)
            return IDX_ERR_FILE_ACC;
        shape_ += elem_in_portion;
        i -= elem_in_portion;
    }

    // Write data
    i = descr_->num_elements;
    max_elem_in_buf = READ_BUF_SIZE / elements_size;
    while (i > 0) {
        elem_in_portion = (i > max_elem_in_buf)? max_elem_in_buf : i;
        val2BEbin(buffer, (void*)data_ptr, elements_size, elem_in_portion);

        count = elem_in_portion * elements_size;
        if (fwrite((void*)buffer, 1, count, descr_->opened_file) != count)
            return IDX_ERR_FILE_ACC;

        data_ptr += count;
        i -= elem_in_portion;
    }
    return IDX_ERR_NONE;
}

//============================================================
// Write header to IDX file (without header)
//============================================================
tIdxRetVal idx_file_write_header(const tIdxDescr *descr_, const uint32_t *shape_) {
    uint32_t i, elem_in_portion;
    uint32_t max_elem_in_buf;
    size_t count;

    // Set position to the beginning of the file(position of header start)
    rewind(descr_->opened_file);

    // Write formatted magic number in header
    buffer[0] = buffer[1] = 0;
    buffer[2] = descr_->data_type;
    buffer[3] = descr_->num_dim;
    if (fwrite((void*)buffer, 1, HEADER_ELEM_SZ, descr_->opened_file) != HEADER_ELEM_SZ)
        return IDX_ERR_FILE_ACC;

    // Write dimensions in header
    i = descr_->num_dim;
    max_elem_in_buf = READ_BUF_SIZE / HEADER_ELEM_SZ;
    while (i > 0) {
        elem_in_portion = (i > max_elem_in_buf)? max_elem_in_buf : i;
        val2BEbin(buffer, (void*)shape_, HEADER_ELEM_SZ, elem_in_portion);

        count = elem_in_portion * HEADER_ELEM_SZ;
        if (fwrite((void*)buffer, 1, count, descr_->opened_file) != count)
            return IDX_ERR_FILE_ACC;
        shape_ += elem_in_portion;
        i -= elem_in_portion;
    }

    // Set position to the end of file
    if (fseek (descr_->opened_file, 0 , SEEK_END) != 0)
        return IDX_ERR_FILE_ACC;

    return IDX_ERR_NONE;
}

//============================================================
// Write data to IDX file (without header)
//============================================================
tIdxRetVal idx_file_write_data(tIdxDescr *descr_, const void *data_) {
    uint32_t i, elem_in_portion;
    const uint8_t elements_size = data_elem_size(descr_->data_type);
    uint32_t max_elem_in_buf;
    size_t count;
    long ftell_ret;
    uint8_t *data_ptr = (uint8_t *)data_;

    // Check that header has already been written, or write a placeholder for it.
    const uint32_t header_size = (1 + descr_->num_dim) * HEADER_ELEM_SZ;
    ftell_ret = ftell(descr_->opened_file);
    if (ftell_ret < 0)
        return IDX_ERR_FILE_ACC;
    else if (ftell_ret < header_size) {
        if (descr_->num_dim > (READ_BUF_SIZE / HEADER_ELEM_SZ))
            return IDX_ERR_INCORR_FUNC_INPUT;
        memset(buffer, 0, READ_BUF_SIZE);
        idx_file_write_header(descr_, (uint32_t *)buffer);
    }

    // Write data itself
    i = descr_->num_elements;
    max_elem_in_buf = READ_BUF_SIZE / elements_size;
    while (i > 0) {
        elem_in_portion = (i > max_elem_in_buf)? max_elem_in_buf : i;
        val2BEbin(buffer, (void*)data_ptr, elements_size, elem_in_portion);

        count = elem_in_portion * elements_size;
        if (fwrite((void*)buffer, 1, count, descr_->opened_file) != count)
            return IDX_ERR_FILE_ACC;

        data_ptr += count;
        i -= elem_in_portion;
    }
    return IDX_ERR_NONE;
}

//============================================================
// Read IDX file completely
//============================================================
tIdxRetVal idx_file_read_completely(
        const char *path_,
        void *data_,
        uint32_t *data_sz_,
        uint32_t *shape_,
        uint32_t *shape_dims_,
        tIdxDataType *el_type_) {
    tIdxRetVal ret = IDX_ERR_NONE;
    FILE* file = NULL;
    tIdxDescr descr;
    uint32_t el_size = 0;

    // Check function input
    if (path_ == NULL || data_sz_ == NULL || shape_dims_ == NULL) {
        ret = IDX_ERR_INCORR_FUNC_INPUT;
        goto ret_err;
    }

    // Open file
    if ((file = fopen(path_, "rb")) == NULL) {
        ret =  IDX_ERR_FILE_ACC;
        goto ret_err;
    }

    // Check file header and memory requirements
    descr.opened_file = file;
    ret = idx_file_check_and_get_info(&descr);
    if (ret != IDX_ERR_NONE)
        goto ret_err;
    el_size = data_elem_size(descr.data_type);

    // Check data sizes if they are pre-allocated
    if ((data_sz_[0] < descr.num_elements * el_size) ||
            (shape_dims_[0] < descr.num_dim)) {
        data_sz_[0] = descr.num_elements * el_size;
        shape_dims_[0] = descr.num_dim;
        ret = IDX_ERR_NOT_ENOUGH_MEM;
        goto ret_err;
    }

    // Check data
    ret = idx_file_read_data(&descr, data_, shape_);
    data_sz_[0] = descr.num_elements * el_size;
    shape_dims_[0] = descr.num_dim;

    if (el_type_ != NULL)
        *el_type_ = descr.data_type;
ret_err:
    if (file != NULL)
        fclose(file);
    return ret;
}

//============================================================
// Write data to IDX file completely
//============================================================
tIdxRetVal idx_file_write_completely(
        const char *path_,
        void *data_,
        uint32_t *shape_,
        uint32_t shape_sz_,
        tIdxDataType el_type) {
    FILE* file;
    tIdxRetVal ret = IDX_ERR_NONE;
    tIdxDescr descr;

    if (shape_sz_ == 0 || shape_   == NULL || data_    == NULL)
        ret = IDX_ERR_INCORR_FUNC_INPUT;
    else if ((file= fopen(path_, "wb")) != NULL) {
        descr.opened_file = file;
        descr.num_dim = shape_sz_;
        descr.data_type = el_type;
        descr.num_elements = 1;
        for (uint32_t i = 0; i < shape_sz_; i++)
            descr.num_elements *= shape_[i];

        ret = idx_file_write(&descr, data_, shape_);
        fclose(file);
    } else {
        ret =  IDX_ERR_FILE_ACC;
    }

    return ret;
}
