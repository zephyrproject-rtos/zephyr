/**
 * @file
 * @brief IGMP definitions
 * This is not to be included by the application.
 */

/*
 * Copyright (c) 2024 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IGMP_H
#define __IGMP_H

#define IGMPV3_MODE_IS_INCLUDE        0x01
#define IGMPV3_MODE_IS_EXCLUDE        0x02
#define IGMPV3_CHANGE_TO_INCLUDE_MODE 0x03
#define IGMPV3_CHANGE_TO_EXCLUDE_MODE 0x04
#define IGMPV3_ALLOW_NEW_SOURCES      0x05
#define IGMPV3_BLOCK_OLD_SOURCES      0x06

#endif /* __IGMP_H */
