/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM_MPU_NXP_MPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_MPU_NXP_MPU_H_

#ifndef _ASMLANGUAGE

#define NXP_MPU_REGION_NUMBER 12

/* Bus Master User Mode Access */
#define UM_READ		4
#define UM_WRITE	2
#define UM_EXEC		1

#define BM0_UM_SHIFT	0
#define BM1_UM_SHIFT	6
#define BM2_UM_SHIFT	12
#define BM3_UM_SHIFT	18

/* Bus Master Supervisor Mode Access */
#define SM_RWX_ALLOW	0
#define SM_RX_ALLOW	1
#define SM_RW_ALLOW	2
#define SM_SAME_AS_UM	3

#define BM0_SM_SHIFT	3
#define BM1_SM_SHIFT	9
#define BM2_SM_SHIFT	15
#define BM3_SM_SHIFT	21

#define BM4_WE_SHIFT	24
#define BM4_RE_SHIFT	25

#if CONFIG_USB_KINETIS || CONFIG_UDC_KINETIS
#define BM4_PERMISSIONS	((1 << BM4_RE_SHIFT) | (1 << BM4_WE_SHIFT))
#else
#define BM4_PERMISSIONS	0
#endif

/* Read Attribute */
#define MPU_REGION_READ  ((UM_READ << BM0_UM_SHIFT) | \
			  (UM_READ << BM1_UM_SHIFT) | \
			  (UM_READ << BM2_UM_SHIFT) | \
			  (UM_READ << BM3_UM_SHIFT))

/* Write Attribute */
#define MPU_REGION_WRITE ((UM_WRITE << BM0_UM_SHIFT) | \
			  (UM_WRITE << BM1_UM_SHIFT) | \
			  (UM_WRITE << BM2_UM_SHIFT) | \
			  (UM_WRITE << BM3_UM_SHIFT))

/* Execute Attribute */
#define MPU_REGION_EXEC  ((UM_EXEC << BM0_UM_SHIFT) | \
			  (UM_EXEC << BM1_UM_SHIFT) | \
			  (UM_EXEC << BM2_UM_SHIFT) | \
			  (UM_EXEC << BM3_UM_SHIFT))

/* Super User Attributes */
#define MPU_REGION_SU	 ((SM_SAME_AS_UM << BM0_SM_SHIFT) | \
			  (SM_SAME_AS_UM << BM1_SM_SHIFT) | \
			  (SM_SAME_AS_UM << BM2_SM_SHIFT) | \
			  (SM_SAME_AS_UM << BM3_SM_SHIFT))

#define MPU_REGION_SU_RX ((SM_RX_ALLOW << BM0_SM_SHIFT) | \
			  (SM_RX_ALLOW << BM1_SM_SHIFT) | \
			  (SM_RX_ALLOW << BM2_SM_SHIFT) | \
			  (SM_RX_ALLOW << BM3_SM_SHIFT))

#define MPU_REGION_SU_RW ((SM_RW_ALLOW << BM0_SM_SHIFT) | \
			  (SM_RW_ALLOW << BM1_SM_SHIFT) | \
			  (SM_RW_ALLOW << BM2_SM_SHIFT) | \
			  (SM_RW_ALLOW << BM3_SM_SHIFT))

#define MPU_REGION_SU_RWX ((SM_RWX_ALLOW << BM0_SM_SHIFT) | \
			   (SM_RWX_ALLOW << BM1_SM_SHIFT) | \
			   (SM_RWX_ALLOW << BM2_SM_SHIFT) | \
			   (SM_RWX_ALLOW << BM3_SM_SHIFT))

/* The ENDADDR field has the last 5 bit reserved and set to 1 */
#define ENDADDR_ROUND(x) (x - 0x1F)

#define REGION_USER_MODE_ATTR {(MPU_REGION_READ | \
				MPU_REGION_WRITE | \
				MPU_REGION_SU)}

/* Some helper defines for common regions */
#if defined(CONFIG_MPU_ALLOW_FLASH_WRITE)
#define REGION_RAM_ATTR	  {((MPU_REGION_SU_RWX) | \
			   ((UM_READ | UM_WRITE | UM_EXEC) << BM3_UM_SHIFT) | \
			   (BM4_PERMISSIONS))}

#define REGION_FLASH_ATTR {(MPU_REGION_SU_RWX)}

#else
#define REGION_RAM_ATTR	  {((MPU_REGION_SU_RW) | \
			   ((UM_READ | UM_WRITE) << BM3_UM_SHIFT) | \
			   (BM4_PERMISSIONS))}

#define REGION_FLASH_ATTR {(MPU_REGION_READ | \
			   MPU_REGION_EXEC | \
			   MPU_REGION_SU)}
#endif

#define REGION_IO_ATTR	  {(MPU_REGION_READ | \
			   MPU_REGION_WRITE | \
			   MPU_REGION_EXEC | \
			   MPU_REGION_SU)}

#define REGION_RO_ATTR	  {(MPU_REGION_READ | MPU_REGION_SU)}

#define REGION_USER_RO_ATTR {(MPU_REGION_READ | \
			     MPU_REGION_SU)}

/* ENET (Master 3) and USB (Master 4) devices will not be able
to access RAM when the region is dynamically disabled in NXP MPU.
DEBUGGER (Master 1) can't be disabled in Region 0. */
#define REGION_DEBUGGER_AND_DEVICE_ATTR  {((MPU_REGION_SU) | \
				((UM_READ | UM_WRITE) << BM3_UM_SHIFT) | \
				(BM4_PERMISSIONS))}

#define REGION_DEBUG_ATTR  {MPU_REGION_SU}

#define REGION_BACKGROUND_ATTR	{MPU_REGION_SU_RW}

struct nxp_mpu_region_attr {
	/* NXP MPU region access permission attributes */
	uint32_t attr;
};

typedef struct nxp_mpu_region_attr nxp_mpu_region_attr_t;

/* Typedef for the k_mem_partition attribute*/
typedef struct {
	uint32_t ap_attr;
} k_mem_partition_attr_t;

/* Kernel macros for memory attribution
 * (access permissions and cache-ability).
 *
 * The macros are to be stored in k_mem_partition_attr_t
 * objects.
 */

/* Read-Write access permission attributes */
#define K_MEM_PARTITION_P_NA_U_NA	((k_mem_partition_attr_t) \
	{(MPU_REGION_SU)})
#define K_MEM_PARTITION_P_RW_U_RW	((k_mem_partition_attr_t) \
	{(MPU_REGION_READ | MPU_REGION_WRITE | MPU_REGION_SU)})
#define K_MEM_PARTITION_P_RW_U_RO	((k_mem_partition_attr_t) \
	{(MPU_REGION_READ | MPU_REGION_SU_RW)})
#define K_MEM_PARTITION_P_RW_U_NA	((k_mem_partition_attr_t) \
	{(MPU_REGION_SU_RW)})
#define K_MEM_PARTITION_P_RO_U_RO	((k_mem_partition_attr_t) \
	{(MPU_REGION_READ | MPU_REGION_SU)})
#define K_MEM_PARTITION_P_RO_U_NA	((k_mem_partition_attr_t) \
	{(MPU_REGION_SU_RX)})

/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RWX_U_RWX	((k_mem_partition_attr_t) \
	{(MPU_REGION_READ | MPU_REGION_WRITE | \
	  MPU_REGION_EXEC | MPU_REGION_SU)})
#define K_MEM_PARTITION_P_RWX_U_RX	((k_mem_partition_attr_t) \
	{(MPU_REGION_READ | MPU_REGION_EXEC | MPU_REGION_SU_RWX)})
#define K_MEM_PARTITION_P_RX_U_RX	((k_mem_partition_attr_t) \
	{(MPU_REGION_READ | MPU_REGION_EXEC | MPU_REGION_SU)})

/*
 * @brief Evaluate Write-ability
 *
 * Evaluate whether the access permissions include write-ability.
 *
 * @param attr The k_mem_partition_attr_t object holding the
 *             MPU attributes to be checked against write-ability.
 */
#define K_MEM_PARTITION_IS_WRITABLE(attr) \
	({ \
		int __is_writable__; \
		switch (attr.ap_attr) { \
		case MPU_REGION_WRITE: \
		case MPU_REGION_SU_RW: \
			__is_writable__ = 1; \
			break; \
		default: \
			__is_writable__ = 0; \
		} \
		__is_writable__; \
	})

/*
 * @brief Evaluate Execution allowance
 *
 * Evaluate whether the access permissions include execution.
 *
 * @param attr The k_mem_partition_attr_t object holding the
 *             MPU attributes to be checked against execution
 *             allowance.
 */
#define K_MEM_PARTITION_IS_EXECUTABLE(attr) \
	({ \
		int __is_executable__; \
		switch (attr.ap_attr) { \
		case MPU_REGION_SU_RX: \
		case MPU_REGION_EXEC: \
			__is_executable__ = 1; \
			break; \
		default: \
			__is_executable__ = 0; \
		} \
		__is_executable__; \
	})


/* Region definition data structure */
struct nxp_mpu_region {
	/* Region Base Address */
	uint32_t base;
	/* Region End Address */
	uint32_t end;
	/* Region Name */
	const char *name;
	/* Region Attributes */
	nxp_mpu_region_attr_t attr;
};

#define MPU_REGION_ENTRY(_name, _base, _end, _attr) \
	{\
		.name = _name, \
		.base = _base, \
		.end = _end, \
		.attr = _attr, \
	}

/* MPU configuration data structure */
struct nxp_mpu_config {
	/* Number of regions */
	uint32_t num_regions;
	/* Regions */
	const struct nxp_mpu_region *mpu_regions;
	/* SRAM Region */
	uint32_t sram_region;
};

/* Reference to the MPU configuration.
 *
 * This struct is defined and populated for each SoC (in the SoC definition),
 * and holds the build-time configuration information for the fixed MPU
 * regions enabled during kernel initialization. Dynamic MPU regions (e.g.
 * for Thread Stack, Stack Guards, etc.) are programmed during runtime, thus,
 * not kept here.
 */
extern const struct nxp_mpu_config mpu_config;

#endif /* _ASMLANGUAGE */

#define _ARCH_MEM_PARTITION_ALIGN_CHECK(start, size) \
	BUILD_ASSERT((size) % \
		CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE == 0 && \
		(size) >= CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE && \
		(uint32_t)(start) % CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE == 0, \
		"the size of the partition must align with minimum MPU \
		 region size" \
		" and greater than or equal to minimum MPU region size." \
		"start address of the partition must align with minimum MPU \
		 region size.")

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_MPU_NXP_MPU_H_ */
