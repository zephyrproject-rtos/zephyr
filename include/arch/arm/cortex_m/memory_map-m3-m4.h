/* memory_map-m3-m4.h - ARM CORTEX-M3/M4 memory map */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * DESCRIPTION
 * This module contains definitions for the memory map parts specific to the
 * CORTEX-M3/M4 series of processors. It is included by
 * nanokernel/ARM/memory_map.h
 */

#ifndef _MEMORY_MAP_M3_M4__H_
#define _MEMORY_MAP_M3_M4__H_

/* 0xe0000000 -> 0xe00fffff: private peripheral bus */

/* 0xe0000000 -> 0xe003ffff: internal [256KB] */
#define _PPB_INT_BASE_ADDR (_EDEV_END_ADDR + 1)
#define _PPB_INT_ITM _PPB_INT_BASE_ADDR
#define _PPB_INT_DWT (_PPB_INT_ITM + KB(4))
#define _PPB_INT_FPB (_PPB_INT_DWT + KB(4))
#define _PPB_INT_RSVD_1 (_PPB_INT_FPB + KB(4))
#define _PPB_INT_SCS (_PPB_INT_RSVD_1 + KB(44))
#define _PPB_INT_RSVD_2 (_PPB_INT_SCS + KB(4))
#define _PPB_INT_END_ADDR (_PPB_INT_RSVD_2 + KB(196) - 1)

/* 0xe0040000 -> 0xe00fffff: external [768K] */
#define _PPB_EXT_BASE_ADDR (_PPB_INT_END_ADDR + 1)
#define _PPB_EXT_TPIU _PPB_EXT_BASE_ADDR
#define _PPB_EXT_ETM (_PPB_EXT_TPIU + KB(4))
#define _PPB_EXT_PPB (_PPB_EXT_ETM + KB(4))
#define _PPB_EXT_ROM_TABLE (_PPB_EXT_PPB + KB(756))
#define _PPB_EXT_END_ADDR (_PPB_EXT_ROM_TABLE + KB(4) - 1)

/* 0xe0100000 -> 0xffffffff: vendor-specific [0.5GB-1MB or 511MB]  */
#define _VENDOR_BASE_ADDR (_PPB_EXT_END_ADDR + 1)
#define _VENDOR_END_ADDR 0xffffffff

#endif /* _MEMORY_MAP_M3_M4__H_ */
