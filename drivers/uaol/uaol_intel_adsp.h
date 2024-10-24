/*
 * Copyright (c) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_UAOL_UAOL_INTEL_ADSP_H_
#define ZEPHYR_DRIVERS_UAOL_UAOL_INTEL_ADSP_H_

#include <stdint.h>

/* UAOL HD Audio Multiple Links (HDAML) Registers */
#define UAOLCAP_OFFSET				0x00
#define UAOLCTL_OFFSET				0x04
#define UAOLOSIDV_OFFSET			0x08
#define UAOLSDIID_OFFSET			0x0C
#define UAOLEPTR_OFFSET				0x20

/* UAOL Shim x Registers */
#define UAOLxPCMSCAP_OFFSET			0x10
#define UAOLxPCMSyCHC_OFFSET(y)			(0x14 + 0x4 * (y))
#define UAOLxPCMSyCM_OFFSET(y)			(0x16 + 0x4 * (y))

/* UAOL IP x Registers */
#define UAOLxTBDF_OFFSET			0x00
#define UAOLxSUV_OFFSET				0x02
#define UAOLxOPC_OFFSET				0x04
#define UAOLxIPC_OFFSET				0x06
#define UAOLxFC_OFFSET				0x10
#define UAOLxFA_OFFSET				0x14
#define UAOLxIC_OFFSET				0x18
#define UAOLxIR_OFFSET				0x1C
#define UAOLxICPy_OFFSET(y)			(0x20 + 0x04 * (y))
#define UAOLxIRPy_OFFSET(y)			(0x30 + 0x04 * (y))
#define UAOLxPCMSyCTL_OFFSET(y)			(0x40 + 0x20 * (y))
#define UAOLxPCMSySTS_OFFSET(y)			(0x48 + 0x20 * (y))
#define UAOLxPCMSyRA_OFFSET(y)			(0x4C + 0x20 * (y))
#define UAOLxPCMSyFSA_OFFSET(y)			(0x50 + 0x20 * (y))

union UAOLCTL {
	uint32_t full;
	struct {
		uint32_t scf    : 4;
		uint32_t oflen  : 1;
		uint32_t rsvd15 : 11;
		uint32_t spa    : 1;
		uint32_t rsvd22 : 6;
		uint32_t cpa    : 1;
		uint32_t rsvd31 : 8;
	} part;
};

union UAOLxPCMSCAP {
	uint16_t full;
	struct {
		uint16_t iss    : 4;
		uint16_t oss    : 4;
		uint16_t bss    : 5;
		uint16_t rsvd15 : 3;
	} part;
};

union UAOLxPCMSyCM {
	uint16_t full;
	struct {
		uint16_t lchan  : 4;
		uint16_t hchan  : 4;
		uint16_t strm   : 6;
		uint16_t rsvd15 : 2;
	} part;
};

union UAOLxTBDF {
	uint16_t full;
	struct {
		uint16_t fncn : 3;
		uint16_t devn : 5;
		uint16_t busn : 8;
	} part;
};

union UAOLxOPC {
	uint16_t full;
	struct {
		uint16_t opc : 16;
	} part;
};

union UAOLxIPC {
	uint16_t full;
	struct {
		uint16_t ipc : 16;
	} part;
};

union UAOLxFC {
	uint32_t full;
	struct {
		uint32_t cimfrm : 14;
		uint32_t rsvd15 : 2;
		uint32_t mfrm   : 3;
		uint32_t frm    : 11;
		uint32_t rsvd31 : 2;
	} part;
};

union UAOLxFA {
	uint32_t full;
	struct {
		uint32_t acimfcnt : 14;
		uint32_t rsvd15   : 2;
		uint32_t amfcnt   : 3;
		uint32_t afcnt    : 11;
		uint32_t adir     : 1;
		uint32_t adj      : 1;
	} part;
};

union UAOLxIC {
	uint32_t full;
	struct {
		uint32_t rsvd23 : 24;
		uint32_t icb    : 1;
		uint32_t icmp   : 1;
		uint32_t rsvd31 : 6;
	} part;
};

union UAOLxIR {
	uint32_t full;
	struct {
		uint32_t rsvd23 : 24;
		uint32_t irvi   : 1;
		uint32_t irmp   : 1;
		uint32_t irvie  : 1;
		uint32_t rsvd31 : 5;
	} part;
};

union UAOLxICPy {
	uint32_t full;
	struct {
		uint32_t dw : 32;
	} part;
};

union UAOLxIRPy {
	uint32_t full;
	struct {
		uint32_t dw : 32;
	} part;
};

union UAOLxPCMSyCTL {
	uint64_t full;
	struct {
		uint64_t aps    : 14;
		uint64_t pm     : 3;
		uint64_t mps    : 11;
		uint64_t si     : 4;
		uint64_t sen    : 1;
		uint64_t asbs   : 7;
		uint64_t srst   : 1;
		uint64_t ass    : 2;
		uint64_t eie    : 1;
		uint64_t feie   : 1;
		uint64_t sme    : 1;
		uint64_t sso    : 4;
		uint64_t rsvd63 : 14;
	} part;
};

union UAOLxPCMSySTS {
	uint32_t full;
	struct {
		uint32_t evcx  : 29;
		uint32_t fifoe : 1;
		uint32_t sbusy : 1;
		uint32_t ofia  : 1;
	} part;
};

union UAOLxPCMSyRA {
	uint32_t full;
	struct {
		uint32_t fcadivn : 9;
		uint32_t rsvd15  : 7;
		uint32_t fcadivm : 7;
		uint32_t rsvd23  : 1;
		uint32_t fbadj   : 1;
		uint32_t fbadir  : 1;
		uint32_t rsvd31  : 6;
	} part;
};

union UAOLxPCMSyFSA {
	uint16_t full;
	struct {
		uint16_t fsao : 16;
	} part;
};

#endif /* ZEPHYR_DRIVERS_UAOL_UAOL_INTEL_ADSP_H_ */
