/*
 * Copyright (c) 2025, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum memc_mspi_aps_z8_rlc {
	MEMC_MSPI_APS_Z8_RLC_4,
	MEMC_MSPI_APS_Z8_RLC_5,
	MEMC_MSPI_APS_Z8_RLC_6,
	MEMC_MSPI_APS_Z8_RLC_7,
	MEMC_MSPI_APS_Z8_RLC_8,
	MEMC_MSPI_APS_Z8_RLC_9,
};

enum memc_mspi_aps_z8_wlc {
	MEMC_MSPI_APS_Z8_WLC_3, /* reserved for APS51216BA */
	MEMC_MSPI_APS_Z8_WLC_7,
	MEMC_MSPI_APS_Z8_WLC_5,
	MEMC_MSPI_APS_Z8_WLC_9, /* reserved for APS25616N */
	MEMC_MSPI_APS_Z8_WLC_4, /* reserved for APS51216BA */
	MEMC_MSPI_APS_Z8_WLC_8, /* reserved for APS25616N */
	MEMC_MSPI_APS_Z8_WLC_6,
	MEMC_MSPI_APS_Z8_WLC_10, /* reserved for APS25616N */
};

/* default for APS51216BA */
#define MEMC_MSPI_APS_Z8_RX_DUMMY_DEFAULT 6
#define MEMC_MSPI_APS_Z8_TX_DUMMY_DEFAULT 6

#define MEMC_MSPI_APS_Z8_CMD_LENGTH_DEFAULT  1
#define MEMC_MSPI_APS_Z8_ADDR_LENGTH_DEFAULT 4

enum memc_mspi_aps_z8_ds {
	MEMC_MSPI_APS_Z8_DRIVE_STRENGTH_FULL,
	MEMC_MSPI_APS_Z8_DRIVE_STRENGTH_HALF,
	MEMC_MSPI_APS_Z8_DRIVE_STRENGTH_QUARTER,
	MEMC_MSPI_APS_Z8_DRIVE_STRENGTH_OCTUPLE,
};

struct memc_mspi_aps_z8_reg {
	union {
		uint8_t MR0;                    /* Mode register 0                      */

		struct {
			uint8_t DS:         2;  /* [0..1] drive strength                */
			uint8_t RLC:        3;  /* [2..4] Read latency code.            */
			uint8_t LT:         1;  /* [5..5] Latency type                  */
			uint8_t:            1;
			uint8_t TSO:        1;  /* [7..7] Temperature Sensor Override   */
		} MR0_b;
	};

	union {
		uint8_t MR1;                    /* Mode register 1                      */

		struct {
			uint8_t VID:        5;  /* [0..4] Vendor ID                     */
			uint8_t:            2;
			uint8_t ULP:        1;  /* [7..7] Half sleep.                   */
		} MR1_b;
	};

	union {
		uint8_t MR2;                    /* Mode register 2                      */

		struct {
			uint8_t DENSITY:    3;  /* [0..2] Density                       */
			uint8_t GENERATION: 2;  /* [3..4] Generation                    */
			uint8_t GB:         3;  /* [5..7] Good or bad                   */
		} MR2_b;
	};

	union {
		uint8_t MR3;                    /* Mode register 3                      */

		struct {
			uint8_t:            4;
			uint8_t SRF:        2;  /* [4..5] Self refresh flag             */
			uint8_t:            1;
			uint8_t RBXen:      1;  /* [7..7] Row Boundary Crossing Enable  */
		} MR3_b;
	};

	union {
		uint8_t MR4;                    /* Mode register 4                      */

		struct {
			uint8_t PASR:       3;  /* [0..2] control refresh address space */
			uint8_t RFS:        2;  /* [3..4] Refresh Frequency setting     */
			uint8_t WLC:        3;  /* [5..7] Write latency code.           */
		} MR4_b;
	};

	union {
		uint8_t MR6;                    /* Mode register 6                      */

		struct {
			uint8_t ULPM:       8;  /* [0..7] ULP modes                     */
		} MR6_b;
	};

	union {
		uint8_t MR8;                    /* Mode register 8                      */

		struct {
			uint8_t BL:         2;  /* [0..1] Burst Length                  */
			uint8_t BT:         1;  /* [2..2] Burst type                    */
			uint8_t RBX:        1;  /* [3..3] Row Boundary Crossing Read EN */
			uint8_t:            2;
			uint8_t IOM:        1;  /* [6..6] IO mode                       */
			uint8_t:            1;
		} MR8_b;
	};
};
