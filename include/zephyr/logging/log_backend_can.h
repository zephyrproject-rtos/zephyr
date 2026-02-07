/*
 * Copyright (c) 2025 Paul Wedeck <paulwedeck@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the CAN logging backend API
 * @ingroup log_backend_can
 */

#ifndef ZEPHYR_LOG_BACKEND_CAN_H_
#define ZEPHYR_LOG_BACKEND_CAN_H_

/**
 * @brief CAN log backend API
 * @defgroup log_backend_can CAN log backend API
 * @ingroup log_backend
 * @{
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Determines details about the can frame bein sent.
 *
 * @details This function can be used to determine which can id and options to use when sending the
 * frame. Valid options are defined in <zephyr/can.h> and consist of:
 *          - CAN_FRAME_IDE: interpret id as extended (29 bit) CAN id),
 *          - CAN_FRAME_FDF: enables CAN-FD transport, including 64 byte frames
 *          - CAN_FRAME_BRS: enables CAN-FD baud-rate switching (only valid if CAN_FRAME_FDF is set)
 *
 *          CAN_FRAME_FDF can only be enabled if the CAN controller is in FD mode.
 *
 * @param id The can id to be used.
 * @param flags The flags to be used. Also determines max frame size.
 *
 * @return True if options are valid, false otherwise.
 */
bool log_backend_can_set_frameopts(uint32_t id, uint8_t flags);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_LOG_BACKEND_CAN_H_ */
