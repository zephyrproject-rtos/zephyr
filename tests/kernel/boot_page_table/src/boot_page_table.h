/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BOOT_PAGE_TABLE_H__
#define __BOOT_PAGE_TABLE_H__

#define MMU_READ                  0x00
#define MMU_WRITE                 0x01
#define MMU_READ_WRITE             (MMU_READ | MMU_WRITE)
#define MMU_PAGE_USER             0x02
#define START_ADDR_RANGE1         0x12300000
#define START_ADDR_RANGE2         0x12340000
#define START_ADDR_RANGE3         0x12400000
#define START_ADDR_RANGE4         0x12460000
#define ADDR_SIZE                 0x1000
#define STARTING_ADDR_RANGE_LMT   0x0009ff
#define START_ADR_RANGE_OVRLP_LMT 0x001000
#define REGION_PERM                (MMU_READ_WRITE | MMU_PAGE_USER)
#endif
