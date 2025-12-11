/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the Intel ADSP HDA log backend API
 * @ingroup log_backend_adsp_hda
 */

#ifndef ZEPHYR_LOG_BACKEND_ADSP_HDA_H_
#define ZEPHYR_LOG_BACKEND_ADSP_HDA_H_

/**
 * @brief Intel ADSP HDA log backend API
 * @defgroup log_backend_adsp_hda Intel ADSP HDA log backend API
 * @ingroup log_backend
 * @{
 */

#include <stdint.h>

/**
 *@brief HDA logger requires a hook for IPC messages
 *
 * When the log is flushed and written with DMA an IPC message should
 * be sent to inform the host. This hook function pointer allows for that
 */
typedef void(*adsp_hda_log_hook_t)(uint32_t written);

/**
 * @brief Initialize the Intel ADSP HDA logger
 *
 * @param hook Function is called after each HDA flush in order to
 *             inform the Host of DMA log data. This hook may be called
 *             from multiple CPUs and multiple calling contexts concurrently.
 *             It is up to the author of the hook to serialize if needed.
 *             It is guaranteed to be called once for every flush.
 * @param channel HDA stream (DMA Channel) to use for logging
 */
void adsp_hda_log_init(adsp_hda_log_hook_t hook, uint32_t channel);

/** @} */

#endif /* ZEPHYR_LOG_BACKEND_ADSP_HDA_H_ */
