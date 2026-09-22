/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Common utility functions for SD subsystem
 */

#ifndef ZEPHYR_SUBSYS_SD_UTILS_H_
#define ZEPHYR_SUBSYS_SD_UTILS_H_

#include <zephyr/kernel.h>
#include <zephyr/sd/sd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Custom SD return codes. Used internally to indicate conditions that may
 * not be errors, but are abnormal return conditions
 */
enum sd_return_codes {
	SD_RETRY = 1,
	SD_NOT_SDIO = 2,
	SD_RESTART = 3,
};

/* Checks SD status return codes */
static inline int sd_check_response(struct sdhc_command *cmd)
{
	if (cmd->response_type == SD_RSP_TYPE_R1) {
		return (cmd->response[0U] & SD_R1_ERR_FLAGS);
	}
	return 0;
}

/* Delay function for SD subsystem */
static inline void sd_delay(unsigned int millis)
{
	k_msleep(millis);
}

/*
 * Helper function to retry sending command to SD card
 * Will retry command if return code equals SD_RETRY
 */
static inline int sd_retry(int(*cmd)(struct sd_card *card),
	struct sd_card *card,
	int retries)
{
	int ret = -ETIMEDOUT;

	while (retries-- >= 0) {
		/* Try cmd */
		ret = cmd(card);
		/**
		 * Functions have 3 possible responses:
		 * 0: success
		 * SD_RETRY: retry command
		 * other: does not retry
		 */
		if (ret != SD_RETRY) {
			break;
		}
	}
	return ret == SD_RETRY ? -ETIMEDOUT : ret;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_SD_UTILS_H_ */
