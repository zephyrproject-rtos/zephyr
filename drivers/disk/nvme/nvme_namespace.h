/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISK_NVME_NVME_NAMESPACE_H_
#define ZEPHYR_DRIVERS_DISK_NVME_NVME_NAMESPACE_H_

struct nvme_namespace_data {
	/** namespace size */
	uint64_t		nsze;

	/** namespace capacity */
	uint64_t		ncap;

	/** namespace utilization */
	uint64_t		nuse;

	/** namespace features */
	uint8_t			nsfeat;

	/** number of lba formats */
	uint8_t			nlbaf;

	/** formatted lba size */
	uint8_t			flbas;

	/** metadata capabilities */
	uint8_t			mc;

	/** end-to-end data protection capabilities */
	uint8_t			dpc;

	/** end-to-end data protection type settings */
	uint8_t			dps;

	/** Namespace Multi-path I/O and Namespace Sharing Capabilities */
	uint8_t			nmic;

	/** Reservation Capabilities */
	uint8_t			rescap;

	/** Format Progress Indicator */
	uint8_t			fpi;

	/** Deallocate Logical Block Features */
	uint8_t			dlfeat;

	/** Namespace Atomic Write Unit Normal  */
	uint16_t		nawun;

	/** Namespace Atomic Write Unit Power Fail */
	uint16_t		nawupf;

	/** Namespace Atomic Compare & Write Unit */
	uint16_t		nacwu;

	/** Namespace Atomic Boundary Size Normal */
	uint16_t		nabsn;

	/** Namespace Atomic Boundary Offset */
	uint16_t		nabo;

	/** Namespace Atomic Boundary Size Power Fail */
	uint16_t		nabspf;

	/** Namespace Optimal IO Boundary */
	uint16_t		noiob;

	/** NVM Capacity */
	uint8_t			nvmcap[16];

	/** Namespace Preferred Write Granularity  */
	uint16_t		npwg;

	/** Namespace Preferred Write Alignment */
	uint16_t		npwa;

	/** Namespace Preferred Deallocate Granularity */
	uint16_t		npdg;

	/** Namespace Preferred Deallocate Alignment */
	uint16_t		npda;

	/** Namespace Optimal Write Size */
	uint16_t		nows;

	/* bytes 74-91: Reserved */
	uint8_t			reserved5[18];

	/** ANA Group Identifier */
	uint32_t		anagrpid;

	/* bytes 96-98: Reserved */
	uint8_t			reserved6[3];

	/** Namespace Attributes */
	uint8_t			nsattr;

	/** NVM Set Identifier */
	uint16_t		nvmsetid;

	/** Endurance Group Identifier */
	uint16_t		endgid;

	/** Namespace Globally Unique Identifier */
	uint8_t			nguid[16];

	/** IEEE Extended Unique Identifier */
	uint8_t			eui64[8];

	/** lba format support */
	uint32_t		lbaf[16];

	uint8_t			reserved7[192];

	uint8_t			vendor_specific[3712];
} __packed __aligned(4);

static inline
void nvme_namespace_data_swapbytes(struct nvme_namespace_data *s)
{
#if _BYTE_ORDER != _LITTLE_ENDIAN
	int i;

	s->nsze = sys_le64_to_cpu(s->nsze);
	s->ncap = sys_le64_to_cpu(s->ncap);
	s->nuse = sys_le64_to_cpu(s->nuse);
	s->nawun = sys_le16_to_cpu(s->nawun);
	s->nawupf = sys_le16_to_cpu(s->nawupf);
	s->nacwu = sys_le16_to_cpu(s->nacwu);
	s->nabsn = sys_le16_to_cpu(s->nabsn);
	s->nabo = sys_le16_to_cpu(s->nabo);
	s->nabspf = sys_le16_to_cpu(s->nabspf);
	s->noiob = sys_le16_to_cpu(s->noiob);
	s->npwg = sys_le16_to_cpu(s->npwg);
	s->npwa = sys_le16_to_cpu(s->npwa);
	s->npdg = sys_le16_to_cpu(s->npdg);
	s->npda = sys_le16_to_cpu(s->npda);
	s->nows = sys_le16_to_cpu(s->nows);
	s->anagrpid = sys_le32_to_cpu(s->anagrpid);
	s->nvmsetid = sys_le16_to_cpu(s->nvmsetid);
	s->endgid = sys_le16_to_cpu(s->endgid);
	for (i = 0; i < 16; i++) {
		s->lbaf[i] = sys_le32_to_cpu(s->lbaf[i]);
	}
#else
	ARG_UNUSED(s);
#endif
}

/* Readable identifier: nvme%%n%%\0 */
#define NVME_NAMESPACE_NAME_MAX_LENGTH 10

struct nvme_namespace {
	struct nvme_controller *ctrlr;
	struct nvme_namespace_data data;
	uint32_t id;
	uint32_t flags;
	uint32_t boundary;
	char name[NVME_NAMESPACE_NAME_MAX_LENGTH];
};

enum nvme_namespace_flags {
	NVME_NS_DEALLOCATE_SUPPORTED	= 0x1,
	NVME_NS_FLUSH_SUPPORTED		= 0x2,
};

uint32_t nvme_namespace_get_sector_size(struct nvme_namespace *ns);

uint64_t nvme_namespace_get_num_sectors(struct nvme_namespace *ns);

uint64_t nvme_namespace_get_size(struct nvme_namespace *ns);

uint32_t nvme_namespace_get_flags(struct nvme_namespace *ns);

const char *nvme_namespace_get_serial_number(struct nvme_namespace *ns);

const char *nvme_namespace_get_model_number(struct nvme_namespace *ns);

const struct nvme_namespace_data *
nvme_namespace_get_data(struct nvme_namespace *ns);

uint32_t nvme_namespace_get_stripesize(struct nvme_namespace *ns);

int nvme_namespace_construct(struct nvme_namespace *ns,
			     uint32_t id,
			     struct nvme_controller *ctrlr);

#endif /* ZEPHYR_DRIVERS_DISK_NVME_NVME_NAMESPACE_H_ */
