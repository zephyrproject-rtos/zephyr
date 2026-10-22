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

/**
 * @brief Max Resp Time of a Membership Query in milliseconds
 *
 * @param code Max Resp Code of the query
 * @param igmpv3 Decode the exponential IGMPv3 form of codes 128 and above
 *
 * @return Max Resp Time in milliseconds, 10 seconds for a code of 0.
 */
uint32_t net_ipv4_igmp_max_resp_time(uint8_t code, bool igmpv3);

#endif /* __IGMP_H */
