/**
 * Copyright (c) - 2024 Extreme Engineering Solutions
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_XES_ENUMERATION_MGMT_
#define H_XES_ENUMERATION_MGMT_

#define ENUMERATION_MGMT_NAME   "enumeration"
#define ENUMERATION_MGMT_VER    1

#define ENUMERATION_MGMT_LIST   0
#define ENUMERATION_MGMT_INFO   255
#define ENUMERATION_NUM_CMDS    256
#define ENUMERATION_MGMT_GP_ID  10


/**
 * @brief Registers the Enumeration Group
 */
void enumeration_mgmt_register_group(void);

/**
 * @brief Unregisters the Enumeration Group
 */
void enumeration_mgmt_unregister_group(void);

#endif /* H_XES_ENUMERATION_MGMT_ */
