/*
 * Copyright (c) 2018-2023 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief UpdateHub Firmware Over-the-Air for Zephyr Project.
 * @defgroup updatehub UpdateHub Firmware Over-the-Air
 * @ingroup lib
 * @{
 */

#ifndef ZEPHYR_INCLUDE_MGMT_UPDATEHUB_H_
#define ZEPHYR_INCLUDE_MGMT_UPDATEHUB_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Responses messages from UpdateHub.
 *
 * @details These messages are used to inform the server and the
 * user about the process status of the UpdateHub and also
 * used to standardize the errors that may occur.
 *
 */
enum updatehub_response {
	UPDATEHUB_NETWORKING_ERROR = 0,
	UPDATEHUB_INCOMPATIBLE_HARDWARE,
	UPDATEHUB_UNCONFIRMED_IMAGE,
	UPDATEHUB_METADATA_ERROR,
	UPDATEHUB_DOWNLOAD_ERROR,
	UPDATEHUB_INSTALL_ERROR,
	UPDATEHUB_FLASH_INIT_ERROR,
	UPDATEHUB_OK,
	UPDATEHUB_HAS_UPDATE,
	UPDATEHUB_NO_UPDATE,
};

/**
 * @brief Runs UpdateHub probe and UpdateHub update automatically.
 *
 * @details The updatehub_autohandler handles the whole process
 * in pre-determined time intervals.
 */
__syscall void updatehub_autohandler(void);

/**
 * @brief The UpdateHub probe verify if there is some update to be performed.
 *
 * @return UPDATEHUB_HAS_UPDATE has an update available.
 * @return UPDATEHUB_NO_UPDATE no update available.
 * @return UPDATEHUB_NETWORKING_ERROR fail to connect to the UpdateHub server.
 * @return UPDATEHUB_INCOMPATIBLE_HARDWARE if Incompatible hardware.
 * @return UPDATEHUB_METADATA_ERROR fail to parse or to encode the metadata.
 */
__syscall enum updatehub_response updatehub_probe(void);

/**
 * @brief Apply the update package.
 *
 * @details Must be used after the UpdateHub probe, if you have updates to
 * be made, will perform the installation of the new image and the hardware
 * will rebooting.
 *
 * @return Return UPDATEHUB_OK if success
 * @return UPDATEHUB_NETWORKING_ERROR if fail to connect to the server.
 * @return UPDATEHUB_DOWNLOAD_ERROR fail while downloading the update package.
 * @return UPDATEHUB_INSTALL_ERROR fail while installing the update package.
 * @return UPDATEHUB_FLASH_INIT_ERROR fail to initialize the flash.
 */
__syscall enum updatehub_response updatehub_update(void);

/**
 * @brief Confirm that image is running as expected.
 *
 * @details Must be used before the UpdateHub probe. It should be one of first
 * actions after reboot.
 *
 * @return Return 0 if success otherwise a negative 'errno' value.
 */
__syscall int updatehub_confirm(void);

/**
 * @brief Request system to reboot.
 *
 * @return Return 0 if success otherwise a negative 'errno' value.
 */
__syscall int updatehub_reboot(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/updatehub.h>
#endif /* ZEPHYR_INCLUDE_MGMT_UPDATEHUB_H_ */
