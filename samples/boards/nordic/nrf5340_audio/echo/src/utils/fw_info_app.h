/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FW_INFO_APP_H_
#define _FW_INFO_APP_H_

/**
 * @brief Prints firmware info, such as Git details, compiled timestamp etc.
 *
 * @return      0 on success.
 *              Otherwise, error from underlying drivers
 */
int fw_info_app_print(void);

#endif /* _FW_INFO_APP_H_ */
