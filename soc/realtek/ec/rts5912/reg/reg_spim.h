/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_SPIM_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_SPIM_H

/**
 * @brief SPIM Controller (SPIM)
 */

struct spim_reg {
	union {
		volatile uint32_t MSCMDL;
		struct {
			volatile uint32_t CMD0: 8;
			volatile uint32_t CMD1: 8;
			volatile uint32_t CMD2: 8;
			volatile uint32_t CMD3: 8;
		} MSCMDL_b;
	};

	union {
		volatile uint32_t CMDH;
		struct {
			volatile uint32_t CMD4: 8;
			uint32_t: 24;
		} CMDH_b;
	};

	union {
		volatile uint32_t MSCMDN;
		struct {
			volatile uint32_t NUM: 6;
			uint32_t: 26;
		} MSCMDN_b;
	};

	const uint32_t RESERVED;

	union {
		volatile uint32_t ADDR;
		struct {
			volatile uint32_t ADDR0: 8;
			volatile uint32_t ADDR1: 8;
			volatile uint32_t ADDR2: 8;
			uint32_t: 8;
		} ADDR_b;
	};

	union {
		volatile uint32_t MSADDRN;
		struct {
			volatile uint32_t NUM: 5;
			uint32_t: 27;
		} MSADDRN_b;
	};

	union {
		volatile uint32_t MSLEN;
		struct {
			volatile uint32_t LEN: 9;
			uint32_t: 23;
		} MSLEN_b;
	};

	union {
		volatile uint32_t MSTRSF;
		struct {
			volatile uint32_t MODE: 4;
			const uint32_t TXOVF: 1;
			const uint32_t RXOVF: 1;
			const uint32_t END: 1;
			volatile uint32_t START: 1;
			uint32_t: 24;
		} MSTRSF_b;
	};

	union {
		volatile uint32_t CTRL;
		struct {
			volatile uint32_t RST: 1;
			volatile uint32_t TRANSEL: 1;
			volatile uint32_t MODE: 2;
			uint32_t: 1;
			volatile uint32_t LSBFST: 1;
			volatile uint32_t CSPOR: 1;
			volatile uint32_t SEL: 1;
			uint32_t: 24;
		} CTRL_b;
	};

	union {
		volatile uint32_t MSCFG;
		struct {
			volatile uint32_t EDO: 2;
			volatile uint32_t TCS: 2;
			uint32_t: 1;
			volatile uint32_t DUMMYBIT: 3;
			uint32_t: 24;
		} MSCFG_b;
	};

	union {
		volatile uint32_t MSCKDV;
		struct {
			volatile uint32_t DIV0: 8;
			volatile uint32_t DIV1: 8;
			uint32_t: 16;
		} MSCKDV_b;
	};

	union {
		const uint32_t MSSTS;
		struct {
			const uint32_t RXFULL: 1;
			const uint32_t TXFULL: 1;
			uint32_t: 30;
		} MSSTS_b;
	};

	union {
		volatile uint32_t MSSIG;
		struct {
			const uint32_t MISO: 1;
			volatile uint32_t MOSI: 1;
			volatile uint32_t CLK: 1;
			volatile uint32_t CS: 1;
			uint32_t: 28;
		} MSSIG_b;
	};

	union {
		volatile uint32_t MSTX;
		struct {
			volatile uint32_t DATA: 8;
			uint32_t: 24;
		} MSTX_b;
	};

	union {
		const uint32_t MSRX;
		struct {
			const uint32_t DATA: 8;
			uint32_t: 24;
		} MSRX_b;
	};
};

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_SPIM_H */
