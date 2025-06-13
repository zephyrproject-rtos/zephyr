/*
 * Copyright (c) 2025 T
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_PHY_COMMON_H_
#define ZEPHYR_DRIVERS_ETHERNET_PHY_COMMON_H_

#include <zephyr/net/phy.h>

#define PHY_INST_GENERATE_DEFAULT_SPEEDS(n)							\
((DT_INST_ENUM_HAS_VALUE(n, default_speeds, 10base_half_duplex) ? LINK_HALF_10BASE : 0) |	\
(DT_INST_ENUM_HAS_VALUE(n, default_speeds, 10base_full_duplex) ? LINK_FULL_10BASE : 0) |	\
(DT_INST_ENUM_HAS_VALUE(n, default_speeds, 100base_half_duplex) ? LINK_HALF_100BASE : 0) |	\
(DT_INST_ENUM_HAS_VALUE(n, default_speeds, 100base_full_duplex) ? LINK_FULL_100BASE : 0) |	\
(DT_INST_ENUM_HAS_VALUE(n, default_speeds, 1000base_half_duplex) ? LINK_HALF_1000BASE : 0) |	\
(DT_INST_ENUM_HAS_VALUE(n, default_speeds, 1000base_full_duplex) ? LINK_FULL_1000BASE : 0))


#endif /* ZEPHYR_DRIVERS_ETHERNET_PHY_COMMON_H_ */
