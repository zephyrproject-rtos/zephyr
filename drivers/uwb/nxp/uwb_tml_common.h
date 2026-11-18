/*
 * Copyright 2019,2020,2022 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UWB_TML_COMMON_H__
#define __UWB_TML_COMMON_H__

#include "uwb_tml_status.h"

#define BACKOFF_TIMEOUT_VALUE 30

#define SHIFT_AND_OVERRIDE_HEADER(buff, hdr_len)                                                   \
	buff = buff + UCI_CMD_INDEX;                                                               \
	for (int i = hdr_len; i > 0; i--) {                                                        \
		(buff)[i] = (buff)[i - 1];                                                         \
	}                                                                                          \
	buff = buff + 1;

extern void uwb_nxp_tml_backoff_delay_reset(uint16_t *stepDelay);

/* Returns 1 if timed out beyond reasonable limit
 *
 * Increments stepDelay by 1.
 */
extern int uwb_nxp_tml_backoff_delay(uint16_t *stepDelay);

#define ENSURE_BACKOFF_TIMEOUT_OR_RET(COUNT, RETVAL)                                               \
	timeout = uwb_nxp_tml_backoff_delay(&COUNT);                                               \
	if (timeout == 1) {                                                                        \
		LOG_D("SPI Backoff timeout. Line: %d", __LINE__);                                  \
		return RETVAL;                                                                     \
	}

#define ENSURE_BACKOFF_TIMEOUT_OR_CLEANUP(COUNT)                                                   \
	timeout = uwb_nxp_tml_backoff_delay(&COUNT);                                               \
	if (timeout == 1) {                                                                        \
		LOG_D("SPI Backoff timeout. Line: %d", __LINE__);                                  \
		goto cleanup;                                                                      \
	}

#define ENSURE_BACKOFF_TIMEOUT_OR_UNLOCKMUTEX(COUNT)                                               \
	timeout = uwb_nxp_tml_backoff_delay(&COUNT);                                               \
	if (timeout == 1) {                                                                        \
		LOG_D("SPI Backoff timeout. Line: %d", __LINE__);                                  \
		goto unlockmutex;                                                                  \
	}

#endif /* __UWB_TML_COMMON_H__ */
