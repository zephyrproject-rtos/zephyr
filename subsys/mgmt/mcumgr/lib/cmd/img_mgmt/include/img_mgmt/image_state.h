/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_IMG_MGMT_IMAGE_STATE__
#define H_IMG_MGMT_IMAGE_STATE_

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Image state flags
 */
#define IMG_MGMT_STATE_F_PENDING	0x01
#define IMG_MGMT_STATE_F_CONFIRMED	0x02
#define IMG_MGMT_STATE_F_ACTIVE		0x04
#define IMG_MGMT_STATE_F_PERMANENT	0x08

/*
 * Swap Types for image management state machine
 */
#define IMG_MGMT_SWAP_TYPE_NONE		0
#define IMG_MGMT_SWAP_TYPE_TEST		1
#define IMG_MGMT_SWAP_TYPE_PERM		2
#define IMG_MGMT_SWAP_TYPE_REVERT	3
#define IMG_MGMT_SWAP_TYPE_UNKNOWN	255

/**
 * @brief Check if the image slot is in use
 *
 * @param slot Slot to check if its in use
 *
 * @return 0 on success, non-zero on failure
 */
int img_mgmt_slot_in_use(int slot);

/**
 * @brief Check if the DFU status is pending
 *
 * @return 1 if there's pending DFU otherwise 0.
 */
int img_mgmt_state_any_pending(void);

/**
 * @brief Collects information about the specified image slot
 *
 * @param query_slot Slot to read state flags from
 *
 * @return return the state flags
 */
uint8_t img_mgmt_state_flags(int query_slot);

/**
 * @brief Sets the pending flag for the specified image slot.  That is, the system
 * will swap to the specified image on the next reboot.  If the permanent
 * argument is specified, the system doesn't require a confirm after the swap
 * occurs.
 *
 * @param slot	   Image slot to set pending
 * @param permanent  If set no confirm is required after image swap
 *
 * @return 0 on success, non-zero on failure
 */
int img_mgmt_state_set_pending(int slot, int permanent);

/**
 * Confirms the current image state.  Prevents a fallback from occurring on the
 * next reboot if the active image is currently being tested.
 *
 * @return 0 on success, non -zero on failure
 */
int img_mgmt_state_confirm(void);

#ifdef __cplusplus
}
#endif

#endif /* H_IMG_MGMT_IMAGE_STATE_ */
