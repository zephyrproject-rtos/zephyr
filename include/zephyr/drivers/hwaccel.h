/*
 * Copyright (c) 2026 Ana Clara Forcelli <ana.forcelli@lsitec.org.br>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup hwaccel_interface
 * @brief Main header file for Hardware Acceleration and Coprocessor driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HWACCEL_H_
#define ZEPHYR_INCLUDE_DRIVERS_HWACCEL_H_

#include <stddef.h>
#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HW_OP_SUM = 0,
    HW_OP_SUB = 1,
    HW_OP_MULT = 2,
    HW_OP_DIV = 3,
    HW_OP_SUM_SCALAR = 4,
    HW_OP_SUB_SCALAR = 5,
    HW_OP_MULT_SCALAR = 6,
    HW_OP_DIV_SCALAR = 7,   
    HW_OP_ACCUMULATE = 8,
} accel_hw_ops_t;


typedef enum {
    FMT_UINT8 = 0,
    FMT_UINT16 = 1,
    FMT_UINT32 = 2,
    FMT_UINT64 = 3,
    FMT_UINT128 = 4,
    FMT_INT8 = 5,
    FMT_INT16 = 6,
    FMT_INT32 = 7,
    FMT_INT64 = 8,
    FMT_INT128 = 9,
    FMT_F16 = 10,
    FMT_F32 = 11,
    FMT_F64 = 12,
    FMT_F128 = 13,
    FMT_Q7 = 14,
    FMT_Q15 = 15,
    FMT_Q31 = 16,
    FMT_Q63 = 17,
} accel_buffer_fmt_t;

typedef struct {
    uint32_t op_caps;
    uint32_t fmt_caps;
    uint32_t max_input_buffers;
    uint32_t max_chan_dimension;
} accel_hw_caps_t;

// fmt is one from accel_buf_fmt
// len is length of each dimension in the buffer
// dim is the number of simensions
// buf is the actual data buffer
//
typedef struct {
    accel_buffer_fmt_t fmt;
    int* len;
    int dim;
    volatile void* buf;
} accel_buffer_t;

/* Expected to be the in every driver's config object */
struct accel_driver_config {
    const accel_hw_caps_t caps; //? if the config is const then is this const
};

/**
 * @brief Define the application callback function signature for
 * hwaccel_irq_callback_user_data_set() function.
 *
 * @param dev hwaccel device instance.
 * @param user_data Arbitrary user data.
 */
typedef void (*hwaccel_irq_callback_user_data_t)(const struct device *dev,
					      void *user_data);


/* Expected to be the in every driver's data object */
struct accel_driver_data {
    uint32_t current_config;
    hwaccel_irq_callback_user_data_t *cb;
	void *user_data;
};


__subsystem struct accel_driver_api {
    int (*query_hw_caps)(const struct device *dev, accel_hw_caps_t *caps);
//#ifdef RTIO
//    int (*iodev_submit)(const struct device *dev, *iodev_sqe);
//#endif
    int (*configure_ops)(const struct device *dev, accel_hw_ops_t* ops_series, int nops);
    int (*set_buffers)(const struct device *dev, accel_buffer_t** in_bufs, int nbufs, accel_buffer_t* out_buf);
    int (*set_callback)(const struct device *dev, hwaccel_irq_callback_user_data_t cb, void *user_data);
    int (*start)(const struct device *dev);
    int (*abort)(const struct device *dev);
};

/**
 * @brief Set Callback
 * @param dev Pointer to accelerator device
 * @param cb callback to completion of operation
 * @param user_data arbitrary pointer
 *
 * @retval 0 If Successful
 * @retval -ENOSYS if not implemented
  */
__syscall int accel_set_callback(const struct device* dev, hwaccel_irq_callback_user_data_t cb, void *user_data);

static inline int z_impl_accel_set_callback(const struct device* dev, hwaccel_irq_callback_user_data_t cb, void *user_data) 
{
    struct accel_driver_data *data = (struct accel_driver_data *)dev->data;
    const struct accel_driver_api *api = (const struct accel_driver_api *)dev->api;
    int ret;

    if(api->set_callback == NULL) {
        return -ENOSYS;
    }

    ret = api->set_callback(dev, cb, user_data);

    return ret;
}

/**
 * @brief Start
 * @param dev Pointer to accelerator device
 *
 * @retval 0 If Successful
 * @retval -ENOSYS if not implemented
  */
__syscall int accel_start(const struct device* dev);

static inline int z_impl_accel_start(const struct device* dev) 
{
    const struct accel_driver_api *api = (const struct accel_driver_api *)dev->api;
    int ret;

    if(api->start == NULL) {
        return -ENOSYS;
    }

    ret = api->start(dev);

    return ret;
}

/**
 * @brief Abort
 * @param dev Pointer to accelerator device
 *
 * @retval 0 If Successful
 * @retval -ENOSYS if not implemented
  */
__syscall int accel_abort(const struct device* dev);

static inline int z_impl_accel_abort(const struct device* dev) 
{
    const struct accel_driver_api *api = (const struct accel_driver_api *)dev->api;
    int ret;

    if(api->abort == NULL) {
        return -ENOSYS;
    }

    ret = api->abort(dev);

    return ret;
}

/**
 * @brief Get Accelerator hardware capabilities.
 * 
 * @param dev Pointer to accelerator device
 * @param caps Pointer to receiving buffer to hw capabilities
 * 
 * @retval 0 If Successful
 * @retval -EINVAL if null pointers
 * @retval -ENOSYS if not implemented
 * 
 */
__syscall int accel_query_hw_caps(const struct device *dev, accel_hw_caps_t *caps);

static inline int z_impl_accel_query_hw_caps(const struct device *dev, accel_hw_caps_t *caps) {
    const struct accel_driver_api *api = (const struct accel_driver_api *)dev->api;
    int ret;

    if(api->query_hw_caps == NULL) {
        return -ENOSYS;
    }

    ret = api->query_hw_caps(dev, caps);

    return ret;
}

/**
 * @brief Configure in/out buffers.
 * 
 * @param dev Pointer to accelerator device
 * @param in_bufs pointer to array of buffer descriptors to be used as input
 * @param nbufs number of buffers are in the input buffer array
 * @param out_bufs pointer to buffer descriptor (not the buffer itself)
 * 
 * @retval 0 If Successful
 * @retval -EINVAL if null pointers and dimensions or formats exceed the controller's caps
 * @retval -ENOSYS if not implemented
 * 
 */
__syscall int accel_set_buffers(const struct device *dev, accel_buffer_t** in_bufs, int nbufs, accel_buffer_t* out_buf);

static inline int z_impl_accel_set_buffers(const struct device *dev, accel_buffer_t** in_bufs, int nbufs, accel_buffer_t* out_buf) {
    const struct accel_driver_api *api = (const struct accel_driver_api *)dev->api;
    const struct accel_driver_config *cfg = (const struct accel_driver_config *)dev->config;
    
    int ret;

    if(api->set_buffers == NULL)
        return -ENOSYS;
    
    if(in_bufs == NULL)
        return -EINVAL;
    
    if(nbufs > cfg->caps.max_input_buffers)
        return -EINVAL;

    for (int i = 0; i < nbufs; i++)
    {
        if(in_bufs[i] == NULL)
            return -EINVAL;
        
        if((in_bufs[i]->fmt & cfg->caps.fmt_caps) == 0)        
            return -EINVAL;

        if(in_bufs[i]->dim > cfg->caps.max_chan_dimension)        
            return -EINVAL;
    }
    
    if ((out_buf->fmt & cfg->caps.fmt_caps) == 0)
        return -EINVAL;

    if (out_buf->dim > cfg->caps.max_chan_dimension)
        return -EINVAL;
    
    ret = api->set_buffers(dev, in_bufs, nbufs, out_buf);

    return ret;
}

/**
 * @brief Configure sequence of operationss.
 * 
 * @param dev Pointer to accelerator device
 * @param ops pointer to array of operations... but shouldnt they be a ops mask???
 * @param nbufs number of ops
 * 
 * @retval 0 If Successful
 * @retval -EINVAL if null pointers or invalid operations
 * @retval -ENOSYS if not implemented
 * 
 */
__syscall int accel_configure_ops(const struct device *dev, accel_hw_ops_t* ops_series, int nops);

static inline int z_impl_accel_configure_ops(const struct device *dev, accel_hw_ops_t* ops_series, int nops)
{
    const struct accel_driver_api *api = (const struct accel_driver_api *)dev->api;
    const struct accel_driver_config *cfg = (const struct accel_driver_config *)dev->config;
    const struct accel_driver_data *data = (const struct accel_driver_data *)dev->data;
    int ret;

    if(api->configure_ops == NULL)
        return -ENOSYS;

    if(ops_series == NULL)
        return -EINVAL;
    
    if(nops > (cfg->caps.max_input_buffers - 1)) // can want to use same buffer? Otherwise
        return -EINVAL;

    for (int i = 0; i < nops; i++)
        if ((BIT(ops_series[i]) & cfg->caps.op_caps) == 0)        
            return -EINVAL;
    
    ret = api->configure_ops(dev, ops_series, nops);

    return ret;
}


#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/hwaccel.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_HWACCEL_H_ */
