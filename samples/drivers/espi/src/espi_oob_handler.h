/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ESPI_OOB_HANDLER_H__
#define __ESPI_OOB_HANDLER_H__

/* eSPI host PECI over eSPI spec partial definitions for this sample code */
#define PCH_OOB_DEST_ADDR               0x02u
#define PCH_OOB_SRC_ADDR                0x21u
#define PECI_OOB_RESPONSE_SENDER_INDEX  0x02u
#define PECI_OOB_RESPONSE_DATA_INDEX    0x04u
#define PECI_OOB_GET_TEMP_CMDCODE       0x01u
#define PECI_OOB_RESPONSE_LENGTH        0x05u
#define MAX_ESPI_OOB_BUF_LEN            80u

/* Test definitions */
#define MIN_GET_TEMP_CYCLES             5u
#define MAX_GET_TEMP_TIMEOUT_MS         100ul

/**
 * @brief eSPI OOB receive handler.
 *
 * This function is called when an OOB (Out-Of-Band) event is received
 * over the eSPI interface. It processes the received OOB event and
 * performs any necessary actions based on the event data.
 *
 * @param dev Pointer to the eSPI device structure.
 * @param cb Pointer to the eSPI callback structure.
 * @param event The eSPI event data structure containing event details.
 */
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
