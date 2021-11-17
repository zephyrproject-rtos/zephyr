/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __DTS__

#define DT_256B		(0x07U)
#define DT_512B		(0x08U)
#define DT_1KB		(0x09U)
#define DT_2KB		(0x0aU)
#define DT_4KB		(0x0bU)
#define DT_8KB		(0x0cU)
#define DT_16KB		(0x0dU)
#define DT_32KB		(0x0eU)
#define DT_64KB		(0x0fU)
#define DT_128KB	(0x10U)
#define DT_256KB	(0x11U)
#define DT_512KB	(0x12U)
#define DT_1MB		(0x13U)
#define DT_2MB		(0x14U)
#define DT_4MB		(0x15U)
#define DT_8MB		(0x16U)
#define DT_16MB		(0x17U)
#define DT_32MB		(0x18U)
#define DT_64MB		(0x19U)
#define DT_128MB	(0x1aU)
#define DT_256MB	(0x1bU)
#define DT_512MB	(0x1cU)
#define DT_1GB		(0x1dU)
#define DT_2GB		(0x1eU)
#define DT_4GB		(0x1fU)

/*
 * NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE |
 * MPU_RASR_XN_Msk | P_RW_U_NA_Msk
 */
#define DT_SRAM_ATTR(size) (0x110b0000 | size)

/*
 * NORMAL_OUTER_INNER_NON_CACHEABLE_NON_SHAREABLE |
 * MPU_RASR_XN_Msk | P_RW_U_NA_Msk
 */
#define DT_SRAM_NOCACHE_ATTR(size) (0x11080000 | size)

#endif /* __DTS__ */
