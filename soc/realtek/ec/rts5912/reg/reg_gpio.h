/*
 * Copyright (c) 2023 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
  * @brief GPIO Controller (GPIO)
  */

typedef struct {

	union {
		volatile uint32_t GCR[132];

		struct {
			volatile uint32_t DIR			: 1;
			volatile uint32_t INDETEN		: 1;
			volatile uint32_t INVOLMD		: 1;
			volatile uint32_t PINSTS		: 1;
			uint32_t				: 4;
			volatile uint32_t MFCTRL		: 3;
			volatile uint32_t OUTDRV		: 1;
			volatile uint32_t SLEWRATE		: 1;
			volatile uint32_t PULLDWEN		: 1;
			volatile uint32_t PULLUPEN		: 1;
			volatile uint32_t SCHEN			: 1;
			volatile uint32_t OUTMD			: 1;
			volatile uint32_t OUTCTRL		: 1;
			uint32_t				: 6;
			volatile uint32_t INTCTRL    		: 3;
			uint32_t				: 1;
			volatile uint32_t INTEN			: 1;
			uint32_t				: 2;
			volatile uint32_t INTSTS		: 1;
		} GCR_b[132];
	} ;
} GPIO_Type;

/* GCR */
#define GPIO_GCR_DIR_Pos					(0UL)
#define GPIO_GCR_DIR_Msk					(0x1UL)
#define GPIO_GCR_INDETEN_Pos					(1UL)
#define GPIO_GCR_INDETEN_Msk					(0x2UL)
#define GPIO_GCR_INVOLMD_Pos					(2UL)
#define GPIO_GCR_INVOLMD_Msk					(0x4UL)
#define GPIO_GCR_PINSTS_Pos					(3UL)
#define GPIO_GCR_PINSTS_Msk					(0x8UL)
#define GPIO_GCR_MFCTRL_Pos					(8UL)
#define GPIO_GCR_MFCTRL_Msk					(0x700UL)
#define GPIO_GCR_OUTDRV_Pos					(11UL)
#define GPIO_GCR_OUTDRV_Msk					(0x800UL)
#define GPIO_GCR_SLEWRATE_Pos					(12UL)
#define GPIO_GCR_SLEWRATE_Msk					(0x1000UL)
#define GPIO_GCR_PULLDWEN_Pos					(13UL)
#define GPIO_GCR_PULLDWEN_Msk					(0x2000UL)
#define GPIO_GCR_PULLUPEN_Pos					(14UL)
#define GPIO_GCR_PULLUPEN_Msk					(0x4000UL)
#define GPIO_GCR_SCHEN_Pos					(15UL)
#define GPIO_GCR_SCHEN_Msk					(0x8000UL)
#define GPIO_GCR_OUTMD_Pos					(16UL)
#define GPIO_GCR_OUTMD_Msk					(0x10000UL)
#define GPIO_GCR_OUTCTRL_Pos					(17UL)
#define GPIO_GCR_OUTCTRL_Msk					(0x20000UL)
#define GPIO_GCR_INTCTRL_Pos					(24UL)
#define GPIO_GCR_INTCTRL_Msk					(0x7000000UL)
#define GPIO_GCR_INTEN_Pos					(28UL)
#define GPIO_GCR_INTEN_Msk					(0x10000000UL)
#define GPIO_GCR_INTSTS_Pos					(31UL)
#define GPIO_GCR_INTSTS_Msk					(0x80000000UL)
