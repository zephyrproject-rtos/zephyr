/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __FLASH_LAYOUT_H__
#define __FLASH_LAYOUT_H__

/**
 * Flash Layout for Quark SE Microcontrollers.
 *
 * @defgroup groupSEFlash Quark SE Flash Layout
 * @{
 */

typedef struct {
	QM_RW uint32_t magic;	  /**< Magic Number. */
	QM_RW uint16_t version;	/**< 0x0100. */
	QM_RW uint16_t reserved;       /**< Reserved. */
	QM_RW uint16_t osc_trim_32mhz; /**< 32MHz Oscillator trim code. */
	QM_RW uint16_t osc_trim_16mhz; /**< 16MHz Oscillator trim code. */
	QM_RW uint16_t osc_trim_8mhz;  /**< 8MHz Oscillator trim code. */
	QM_RW uint16_t osc_trim_4mhz;  /**< 4MHz Oscillator trim code. */
} qm_flash_otp_trim_t;

#if (UNIT_TEST)
extern uint8_t test_flash_page[0x800];
#define QM_FLASH_OTP_TRIM_CODE_BASE (&test_flash_page[0])
#else
#define QM_FLASH_OTP_TRIM_CODE_BASE (0xFFFFE1F0)
#endif

#define QM_FLASH_OTP_TRIM_CODE                                                 \
	((qm_flash_otp_trim_t *)QM_FLASH_OTP_TRIM_CODE_BASE)
#define QM_FLASH_OTP_SOC_DATA_VALID (0x24535021) /* $SP! */
#define QM_FLASH_OTP_TRIM_MAGIC (QM_FLASH_OTP_TRIM_CODE->magic)

typedef union {
	struct trim_fields {
		QM_RW uint16_t
		    osc_trim_32mhz; /**< 32MHz Oscillator trim code. */
		QM_RW uint16_t
		    osc_trim_16mhz; /**< 16MHz Oscillator trim code. */
		QM_RW uint16_t osc_trim_8mhz; /**< 8MHz Oscillator trim code. */
		QM_RW uint16_t osc_trim_4mhz; /**< 4MHz Oscillator trim code. */
	} fields;
	QM_RW uint32_t osc_trim_u32[2]; /**< Oscillator trim code array.*/
	QM_RW uint16_t osc_trim_u16[4]; /**< Oscillator trim code array.*/
} qm_flash_data_trim_t;

#if (UNIT_TEST)
#define QM_FLASH_DATA_TRIM_BASE (&test_flash_page[100])
#define QM_FLASH_DATA_TRIM_OFFSET (100)
#else
#define QM_FLASH_DATA_TRIM_BASE (0x4002F000)
#define QM_FLASH_DATA_TRIM_OFFSET ((uint32_t)QM_FLASH_DATA_TRIM_BASE & 0x3FFFF)
#endif

#define QM_FLASH_DATA_TRIM ((qm_flash_data_trim_t *)QM_FLASH_DATA_TRIM_BASE)
#define QM_FLASH_DATA_TRIM_CODE (&QM_FLASH_DATA_TRIM->fields)
#define QM_FLASH_DATA_TRIM_REGION QM_FLASH_REGION_SYS

#define QM_FLASH_TRIM_PRESENT_MASK (0xFC00)
#define QM_FLASH_TRIM_PRESENT (0x0000)

/*
 * Bootloader data
 */

/* The flash controller where BL-Data is stored. */
#define BL_DATA_FLASH_CONTROLLER QM_FLASH_0
/* The flash region where BL-Data is stored. */
#define BL_DATA_FLASH_REGION QM_FLASH_REGION_SYS
/* The flash address where the BL-Data Section starts. */
#define BL_DATA_FLASH_REGION_BASE QM_FLASH_REGION_SYS_0_BASE
/* The flash page where the BL-Data Section starts. */
#define BL_DATA_SECTION_BASE_PAGE (94)

/* The size (in pages) of the System_0 flash region of Quark SE. */
#define QM_FLASH_REGION_SYS_0_PAGES (96)
/* The size (in pages) of the System_1 flash region of Quark SE. */
#define QM_FLASH_REGION_SYS_1_PAGES (96)

/* The size (in pages) of the Bootloader Data section. */
#define BL_DATA_SECTION_PAGES (2)

#if (BL_CONFIG_DUAL_BANK)
/* ARC Partition size, in pages */
#define BL_PARTITION_SIZE_ARC                                                  \
	((QM_FLASH_REGION_SYS_0_PAGES - BL_DATA_SECTION_PAGES) / 2)
#define BL_PARTITION_SIZE_LMT (QM_FLASH_REGION_SYS_1_PAGES / 2)
#else /* !BL_CONFIG_DUAL_BANK */
#define BL_PARTITION_SIZE_ARC                                                  \
	((QM_FLASH_REGION_SYS_0_PAGES - BL_DATA_SECTION_PAGES))
#define BL_PARTITION_SIZE_LMT (QM_FLASH_REGION_SYS_1_PAGES)
#endif /* BL_CONFIG_DUAL_BANK */

/* Number of boot targets. */
#define BL_BOOT_TARGETS_NUM (2)

#define BL_TARGET_IDX_LMT (0)
#define BL_TARGET_IDX_ARC (1)

#define BL_PARTITION_IDX_LMT0 (0)
#define BL_PARTITION_IDX_ARC0 (1)
#define BL_PARTITION_IDX_LMT1 (2)
#define BL_PARTITION_IDX_ARC1 (3)

#define BL_TARGET_0_LMT                                                        \
	{                                                                      \
		.active_partition_idx = BL_PARTITION_IDX_LMT0, .svn = 0        \
	}

#define BL_TARGET_1_ARC                                                        \
	{                                                                      \
		.active_partition_idx = BL_PARTITION_IDX_ARC0, .svn = 0        \
	}

/*
 * Macro for defining an application flash partition.
 *
 * @param[in] target The index of the target associated with the partition.
 * @param[in] ctrl   The flash controller on which the partition is located.
 * @param[in] region_addr The base address of the region where the partition is
 * 			  located.
 * @param[in] size The size in pages of the partition.
 * @param[in] idx  The index of the partition within the flash region (0 for
 * 		   the first partition in the region, 1 for the second one).
 */
#define DEFINE_PARTITION(target, ctrl, region_addr, size, idx)                 \
	{                                                                      \
		.target_idx = target, .controller = ctrl,                      \
		.first_page = (idx * size), .num_pages = size,                 \
		.start_addr = ((uint32_t *)region_addr) +                      \
			      (idx * size * QM_FLASH_PAGE_SIZE_DWORDS),        \
		.is_consistent = true                                          \
	}

/* PARTITION 0: LMT-0 */
#define BL_PARTITION_0                                                         \
	DEFINE_PARTITION(BL_TARGET_IDX_LMT, QM_FLASH_1,                        \
			 QM_FLASH_REGION_SYS_1_BASE, BL_PARTITION_SIZE_LMT, 0)

/* PARTITION 1: ARC-0 */
#define BL_PARTITION_1                                                         \
	DEFINE_PARTITION(BL_TARGET_IDX_ARC, QM_FLASH_0,                        \
			 QM_FLASH_REGION_SYS_0_BASE, BL_PARTITION_SIZE_ARC, 0)

/* PARTITION 2: LMT-1 */
#define BL_PARTITION_2                                                         \
	DEFINE_PARTITION(BL_TARGET_IDX_LMT, QM_FLASH_1,                        \
			 QM_FLASH_REGION_SYS_1_BASE, BL_PARTITION_SIZE_LMT, 1)

/* PARTITION 3: ARC-1 */
#define BL_PARTITION_3                                                         \
	DEFINE_PARTITION(BL_TARGET_IDX_ARC, QM_FLASH_0,                        \
			 QM_FLASH_REGION_SYS_0_BASE, BL_PARTITION_SIZE_ARC, 1)

#define BL_TARGET_LIST                                                         \
	{                                                                      \
		BL_TARGET_0_LMT, BL_TARGET_1_ARC                               \
	}

#if BL_CONFIG_DUAL_BANK
#define BL_PARTITION_LIST                                                      \
	{                                                                      \
		BL_PARTITION_0, BL_PARTITION_1, BL_PARTITION_2, BL_PARTITION_3 \
	}
#else /* !BL_CONFIG_DUAL_BANK */
#define BL_PARTITION_LIST                                                      \
	{                                                                      \
		BL_PARTITION_0, BL_PARTITION_1                                 \
	}

#endif /* BL_CONFIG_DUAL_BANK */

/**
 * @}
 */

#endif /* __FLASH_LAYOUT_H__ */
