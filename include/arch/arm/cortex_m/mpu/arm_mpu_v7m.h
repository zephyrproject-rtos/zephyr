/*
 * Copyright (c) 2018 Linaro Limited.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Privileged No Access, Unprivileged No Access */
#define NO_ACCESS       0x0
#define NO_ACCESS_Msk   ((NO_ACCESS << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk)
/* Privileged No Access, Unprivileged No Access */
#define P_NA_U_NA       0x0
#define P_NA_U_NA_Msk   ((P_NA_U_NA << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk)
/* Privileged Read Write, Unprivileged No Access */
#define P_RW_U_NA       0x1
#define P_RW_U_NA_Msk   ((P_RW_U_NA << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk)
/* Privileged Read Write, Unprivileged Read Only */
#define P_RW_U_RO       0x2
#define P_RW_U_RO_Msk   ((P_RW_U_RO << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk)
/* Privileged Read Write, Unprivileged Read Write */
#define P_RW_U_RW       0x3
#define P_RW_U_RW_Msk   ((P_RW_U_RW << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk)
/* Privileged Read Write, Unprivileged Read Write */
#define FULL_ACCESS     0x3
#define FULL_ACCESS_Msk ((FULL_ACCESS << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk)
/* Privileged Read Only, Unprivileged No Access */
#define P_RO_U_NA       0x5
#define P_RO_U_NA_Msk   ((P_RO_U_NA << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk)
/* Privileged Read Only, Unprivileged Read Only */
#define P_RO_U_RO       0x6
#define P_RO_U_RO_Msk   ((P_RO_U_RO << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk)
/* Privileged Read Only, Unprivileged Read Only */
#define RO              0x7
#define RO_Msk          ((RO << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk)

/* Attribute flag for not-allowing execution (eXecute Never) */
#define NOT_EXEC MPU_RASR_XN_Msk

/* The following definitions are for internal use in arm_mpu.h. */
#define STRONGLY_ORDERED_SHAREABLE      MPU_RASR_S_Msk
#define DEVICE_SHAREABLE                (MPU_RASR_B_Msk | MPU_RASR_S_Msk)
#define NORMAL_OUTER_INNER_WRITE_THROUGH_SHAREABLE \
		(MPU_RASR_C_Msk | MPU_RASR_S_Msk)
#define NORMAL_OUTER_INNER_WRITE_THROUGH_NON_SHAREABLE	MPU_RASR_C_Msk
#define NORMAL_OUTER_INNER_WRITE_BACK_SHAREABLE	\
		(MPU_RASR_C_Msk | MPU_RASR_B_Msk | MPU_RASR_S_Msk)
#define NORMAL_OUTER_INNER_WRITE_BACK_NON_SHAREABLE \
		(MPU_RASR_C_Msk | MPU_RASR_B_Msk)
#define NORMAL_OUTER_INNER_NON_CACHEABLE_SHAREABLE \
		((1 << MPU_RASR_TEX_Pos) | MPU_RASR_S_Msk)
#define NORMAL_OUTER_INNER_NON_CACHEABLE_NON_SHAREABLE \
		(1 << MPU_RASR_TEX_Pos)
#define NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_SHAREABLE \
	((1 << MPU_RASR_TEX_Pos) |\
	 MPU_RASR_C_Msk | MPU_RASR_B_Msk | MPU_RASR_S_Msk)
#define NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE \
	((1 << MPU_RASR_TEX_Pos) | MPU_RASR_C_Msk | MPU_RASR_B_Msk)
#define DEVICE_NON_SHAREABLE            (2 << MPU_RASR_TEX_Pos)

/* Bit-masks to disable sub-regions. */
#define SUB_REGION_0_DISABLED	((0x01 << MPU_RASR_SRD_Pos) & MPU_RASR_SRD_Msk)
#define SUB_REGION_1_DISABLED	((0x02 << MPU_RASR_SRD_Pos) & MPU_RASR_SRD_Msk)
#define SUB_REGION_2_DISABLED	((0x04 << MPU_RASR_SRD_Pos) & MPU_RASR_SRD_Msk)
#define SUB_REGION_3_DISABLED	((0x08 << MPU_RASR_SRD_Pos) & MPU_RASR_SRD_Msk)
#define SUB_REGION_4_DISABLED	((0x10 << MPU_RASR_SRD_Pos) & MPU_RASR_SRD_Msk)
#define SUB_REGION_5_DISABLED	((0x20 << MPU_RASR_SRD_Pos) & MPU_RASR_SRD_Msk)
#define SUB_REGION_6_DISABLED	((0x40 << MPU_RASR_SRD_Pos) & MPU_RASR_SRD_Msk)
#define SUB_REGION_7_DISABLED	((0x80 << MPU_RASR_SRD_Pos) & MPU_RASR_SRD_Msk)


#define REGION_SIZE(size) ((ARM_MPU_REGION_SIZE_ ## size \
	<< MPU_RASR_SIZE_Pos) & MPU_RASR_SIZE_Msk)

#define REGION_32B      REGION_SIZE(32B)
#define REGION_64B      REGION_SIZE(64B)
#define REGION_128B     REGION_SIZE(128B)
#define REGION_256B     REGION_SIZE(256B)
#define REGION_512B     REGION_SIZE(512B)
#define REGION_1K       REGION_SIZE(1KB)
#define REGION_2K       REGION_SIZE(2KB)
#define REGION_4K       REGION_SIZE(4KB)
#define REGION_8K       REGION_SIZE(8KB)
#define REGION_16K      REGION_SIZE(16KB)
#define REGION_32K      REGION_SIZE(32KB)
#define REGION_64K      REGION_SIZE(64KB)
#define REGION_128K     REGION_SIZE(128KB)
#define REGION_256K     REGION_SIZE(256KB)
#define REGION_512K     REGION_SIZE(512KB)
#define REGION_1M       REGION_SIZE(1MB)
#define REGION_2M       REGION_SIZE(2MB)
#define REGION_4M       REGION_SIZE(4MB)
#define REGION_8M       REGION_SIZE(8MB)
#define REGION_16M      REGION_SIZE(16MB)
#define REGION_32M      REGION_SIZE(32MB)
#define REGION_64M      REGION_SIZE(64MB)
#define REGION_128M     REGION_SIZE(128MB)
#define REGION_256M     REGION_SIZE(256MB)
#define REGION_512M     REGION_SIZE(512MB)
#define REGION_1G       REGION_SIZE(1GB)
#define REGION_2G       REGION_SIZE(2GB)
#define REGION_4G       REGION_SIZE(4GB)

/* Some helper defines for common regions */
#define REGION_RAM_ATTR(size) \
{ \
	(NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE | \
	 MPU_RASR_XN_Msk | size | P_RW_U_NA_Msk) \
}
#if defined(CONFIG_MPU_ALLOW_FLASH_WRITE)
#define REGION_FLASH_ATTR(size) \
{ \
	(NORMAL_OUTER_INNER_WRITE_THROUGH_NON_SHAREABLE | size | \
		P_RW_U_RO_Msk) \
}
#else
#define REGION_FLASH_ATTR(size) \
{ \
	(NORMAL_OUTER_INNER_WRITE_THROUGH_NON_SHAREABLE | size | RO_Msk) \
}
#endif
#define REGION_PPB_ATTR(size) { (STRONGLY_ORDERED_SHAREABLE | size | \
		P_RW_U_NA_Msk) }
#define REGION_IO_ATTR(size) { (DEVICE_NON_SHAREABLE | size | P_RW_U_NA_Msk) }

struct arm_mpu_region_attr {
	/* Attributes belonging to RASR (including the encoded region size) */
	u32_t rasr;
};

typedef struct arm_mpu_region_attr arm_mpu_region_attr_t;
