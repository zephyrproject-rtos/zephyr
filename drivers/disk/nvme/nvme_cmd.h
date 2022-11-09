/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2022 Intel Corp.
 */

#ifndef ZEPHYR_DRIVERS_DISK_NVME_NVME_COMMAND_H_
#define ZEPHYR_DRIVERS_DISK_NVME_NVME_COMMAND_H_

#include <zephyr/sys/slist.h>

struct nvme_command {
	/* dword 0 */
	struct _cdw0 {
		uint8_t opc;		/* opcode */
		uint8_t fuse : 2;	/* fused operation */
		uint8_t rsvd : 4;	/* reserved */
		uint8_t psdt : 2;       /* PRP or SGL for Data Transfer */
		uint16_t cid;		/* command identifier */
	} cdw0;

	/* dword 1 */
	uint32_t nsid;		/* namespace identifier */

	/* dword 2-3 */
	uint32_t cdw2;
	uint32_t cdw3;

	/* dword 4-5 */
	uint64_t mptr;		/* metadata pointer */

	/* dword 6-7 and 8-9 */
	struct _dptr {
		uint64_t prp1;	/* prp entry 1 */
		uint64_t prp2;	/* prp entry 2 */
	} dptr;			/* data pointer */

	/* dword 10 */
	union {
		uint32_t cdw10;	/* command-specific */
		uint32_t ndt;	/* Number of Dwords in Data transfer */
	};

	/* dword 11 */
	union {
		uint32_t cdw11;	/* command-specific */
		uint32_t ndm;	/* Number of Dwords in Metadata transfer */
	};

	/* dword 12-15 */
	uint32_t cdw12;		/* command-specific */
	uint32_t cdw13;		/* command-specific */
	uint32_t cdw14;		/* command-specific */
	uint32_t cdw15;		/* command-specific */
};

struct nvme_completion {
	/* dword 0 */
	uint32_t	cdw0;	/* command-specific */

	/* dword 1 */
	uint32_t	rsvd;

	/* dword 2 */
	uint16_t	sqhd;	/* submission queue head pointer */
	uint16_t	sqid;	/* submission queue identifier */

	/* dword 3 */
	uint16_t	cid;	/* command identifier */
	uint16_t	p      : 1; /* phase tag */
	uint16_t	status : 15;
} __aligned(8);

#endif /* ZEPHYR_DRIVERS_DISK_NVME_NVME_COMMAND_H_ */
