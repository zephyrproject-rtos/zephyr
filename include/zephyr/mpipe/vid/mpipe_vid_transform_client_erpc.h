/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Client-side video transform element over eRPC.
 * @ingroup mpipe_vid_transform_clients
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_TRANSFORM_CLIENT_ERPC_H_
#define ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_TRANSFORM_CLIENT_ERPC_H_

/**
 * @addtogroup mpipe_vid_transform_clients
 * @{
 */

#include <stdint.h>

#include <zephyr/mpipe/vid/mpipe_vid_transform_client.h>

/**
 * @brief Initialize a client-side video transform element bound to eRPC.
 *
 * Installs the eRPC hooks on the element, binds the video transform interface
 * to the client the eRPC backend brings up, then initializes the element as
 * mpipe_vid_transform_client_init() does.
 *
 * @param vtc Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @retval 0 Success.
 * @retval -EIO The eRPC transport could not be brought up.
 * @return Other negative errno from mpipe_vid_transform_client_init().
 */
int mpipe_vid_transform_client_erpc_init(struct mpipe_vid_transform_client *vtc, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_TRANSFORM_CLIENT_ERPC_H_ */
