/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_BOOTMODE_H_
#define ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_BOOTMODE_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @name Boot mode service error codes.
 * @{
 */

/** Invalid/unsupported boot mode transition. */
#define IRONSIDE_BOOTMODE_ERROR_UNSUPPORTED_MODE (1)
/** Failed to reboot into the boot mode due to other activity preventing a reset. */
#define IRONSIDE_BOOTMODE_ERROR_BUSY (2)
/** The boot message is too large to fit in the buffer. */
#define IRONSIDE_BOOTMODE_ERROR_MESSAGE_TOO_LARGE (3)

/**
 * @}
 */

/* IronSide call identifiers with implicit versions. */
#define IRONSIDE_CALL_ID_BOOTMODE_SERVICE_V1 5

enum {
	IRONSIDE_BOOTMODE_SERVICE_MODE_IDX,
	IRONSIDE_BOOTMODE_SERVICE_MSG_0_IDX,
	IRONSIDE_BOOTMODE_SERVICE_MSG_1_IDX,
	IRONSIDE_BOOTMODE_SERVICE_MSG_2_IDX,
	IRONSIDE_BOOTMODE_SERVICE_MSG_3_IDX,
	/* The last enum value is reserved for the number of arguments */
	IRONSIDE_BOOTMODE_SERVICE_NUM_ARGS,
};

/* Maximum size of the message parameter. */
#define IRONSIDE_BOOTMODE_SERVICE_MSG_MAX_SIZE (4 * sizeof(uint32_t))

/* Index of the return code within the service buffer. */
#define IRONSIDE_BOOTMODE_SERVICE_RETCODE_IDX (0)

/**
 * @brief Request a reboot into the secondary firmware boot mode.
 *
 * This invokes the IronSide SE boot mode service to restart the system into the secondary boot
 * mode. In this mode, the secondary configuration defined in UICR is applied instead of the
 * primary one. The system immediately reboots without a reply if the request succeeds.
 *
 * The given message data is passed to the boot report of the CPU booted in the secondary boot mode.
 *
 * @note This function does not return if the request is successful.
 * @note The device will boot into the secondary firmware instead of primary firmware.
 * @note The request does not fail if the secondary firmware is not defined.
 *
 * @param msg A message that can be placed in the cpu's boot report.
 * @param msg_size Size of the message in bytes.
 *
 * @retval 0 on success.
 * @retval -IRONSIDE_BOOTMODE_ERROR_UNSUPPORTED_MODE if the secondary boot mode is unsupported.
 * @retval -IRONSIDE_BOOTMODE_ERROR_BUSY if the reboot was blocked.
 * @retval -IRONSIDE_BOOTMODE_ERROR_MESSAGE_TOO_LARGE if msg_size is greater than
 * IRONSIDE_BOOTMODE_SERVICE_MSG_MAX_SIZE.
 * @retval Positive error status if reported by IronSide call (see error codes in @ref call.h).
 */
int ironside_bootmode_secondary_reboot(const uint8_t *msg, size_t msg_size);

#endif /* ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_BOOTMODE_H_ */
