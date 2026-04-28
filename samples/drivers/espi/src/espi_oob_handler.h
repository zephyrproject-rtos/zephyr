/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ESPI_OOB_HANDLER_H__
#define __ESPI_OOB_HANDLER_H__

/* eSPI host entity address  */
#define PCH_DEST_SLV_ADDR     0x02u
#define SRC_SLV_ADDR          0x21u

#define OOB_RESPONSE_SENDER_INDEX   0x02u
#define OOB_RESPONSE_DATA_INDEX     0x04u


/* Temperature command opcode */
#define OOB_CMDCODE           0x01u
#define OOB_RESPONSE_LEN            0x05u

/* Maximum bytes for OOB transactions */
#define MAX_ESPI_BUF_LEN      80u
#define MIN_GET_TEMP_CYCLES   5u

/* 100ms */
#define MAX_OOB_TIMEOUT       100ul

void oob_rx_handler(const struct device *dev, struct espi_callback *cb,
		    struct espi_event event);

/**
 * @brief Retrieve PCH temperature over OOB channel.
 * Assumes OOB Tx and Rx as synchronous operation.
 *
 * @param dev eSPI driver handle.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP returned when OOB channel is not supported.
 * @retval -EINVAL is returned when OOB parameters are invalid.
 *
 */
int get_pch_temp_sync(const struct device *dev);


/**
 * @brief Retrieve PCH temperature over OOB channel.
 * Assumes OOB Tx and Rx as synchronous operation.
 *
 * @param dev eSPI driver handle.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP returned when OOB channel is not supported.
 * @retval -ETIMEOUT OOB operations could not be started.
 *
 */
int get_pch_temp_async(const struct device *dev);

#endif /* __ESPI_OOB_HANDLER_H__ */
