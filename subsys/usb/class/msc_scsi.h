/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MSC_SCSI_H_
#define ZEPHYR_MSC_SCSI_H_

/*
 * Mandatory command set for USB MSC Bulk-Only Transport
 */

/* Generic CDB for 6 byte commands */
struct cdb6 {
	u8_t code;
	u8_t info[4];
	u8_t control;
} __packed;

/* Get allocation length from generic 6 byte cdb */
static inline u8_t cdb6_get_length(const struct cdb6 *cmd)
{
	return cmd->info[3];
}

/* Generic CDB for 10 byte commands */
struct cdb10 {
	u8_t code;
	u8_t info[8];
	u8_t control;
} __packed;

/* Get allocation length from generic 10 byte cdb */
static inline u16_t cdb10_get_length(const struct cdb10 *cmd)
{
	return sys_get_be16(&cmd->info[6]);
}

/* Generic CDB for 12 byte commands */
struct cdb12 {
	u8_t code;
	u8_t info[10];
	u8_t control;
} __packed;

/* Get allocation length from generic 12 byte cdb */
static inline u16_t cdb12_get_length(const struct cdb12 *cmd)
{
	return sys_get_be32(&cmd->info[5]);
}

/* Sense data response codes */
#define SDRC_CURRENT_ERRORS		0x70

/* Sense keys */
#define SK_ILLEGAL_REQUEST		0x5

/* ASC and ASCQ codes */
#define ASCQ_CANNOT_RM_UNKNOWN_FORMAT	0x3001
#define ASCQ_INVALID_FIELD_IN_CDB	0x2400
#define ASCQ_INVALID_CMD_OPCODE		0x2000

/* The fixed format sense data (response codes 0x70 and 0x71) */
struct fixed_format_sense_data {
	u8_t code		: 7;
	u8_t valid		: 1;
	u8_t obsolete;
	u8_t sense_key		: 4;
	u8_t reserved		: 1;
	u8_t ili		: 1;
	u8_t eom		: 1;
	u8_t filemark		: 1;
	u8_t info[4];
	u8_t as_length;
	struct additional_sense_data {
		u8_t cmd_specific_info[4];
		u8_t asc_ascq[2];
		u8_t fru_code;
		u8_t sks[3];
	} asd;
} __packed;

/* INQUIRY command */
struct cdb_inquiry {
	u8_t code;
	u8_t evpd		: 1;
	u8_t reserved		: 7;
	u8_t page_code;
	u8_t length[2];
	u8_t control;
} __packed;

struct dabc_inquiry_data {
	u64_t type		: 5;
	u64_t qualifier		: 3;
	u64_t reserved		: 7;
	u64_t rmb		: 1;
	u64_t version		: 8;
	u64_t rdf		: 4;
	u64_t hisup		: 1;
	u64_t normaca		: 1;
	u64_t obsolete1		: 2;
	u64_t length		: 8;
	u64_t protect		: 1;
	u64_t obsolete2		: 2;
	u64_t three_pc		: 1;
	u64_t tpgs		: 2;
	u64_t acc		: 1;
	u64_t sccs		: 1;
	u64_t obsolete3		: 4;
	u64_t multip		: 1;
	u64_t vs1		: 1;
	u64_t encserv		: 1;
	u64_t obsolete4		: 1;
	u64_t vs2		: 1;
	u64_t cmdque		: 1;
	u64_t obsolete5		: 6;
	u8_t t10_vid[8];
	u8_t product_id[16];
	u8_t product_rev[4];
} __packed;

#define DIRECT_ACCESS_BLOCK_DEVICE	0x00

struct mode_parameter_header6 {
	u8_t data_length;
	u8_t medium_type;
	u8_t reserved1		: 4;
	u8_t dpofua		: 1;
	u8_t reserved2		: 2;
	u8_t wp			: 1;
	u8_t bd_length;
} __packed;

struct mode_parameter6 {
	struct mode_parameter_header6 hdr;
} __packed;

struct capacity_descriptor {
	/* Capacity List Header */
	struct capacity_list_header {
		u8_t reserved[3];
		u8_t length;
	} clh;
	/* Current Capacity Descriptor */
	struct current_capacity_descriptor {
		u8_t numof_blocks[4];
		u8_t type	: 2;
		u8_t reserved	: 6;
		u8_t block_length[3];
	} ccd;
} __packed;

#define DESCRIPTOR_TYPE_FORMATTED_MEDIA		0x02

#endif /* ZEPHYR_MSC_SCSI_H_ */
