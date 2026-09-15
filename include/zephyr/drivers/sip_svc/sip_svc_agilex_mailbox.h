/*
 * Copyright (c) 2022-2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SIP_SVC_AGILEX_MB_H_
#define ZEPHYR_INCLUDE_SIP_SVC_AGILEX_MB_H_

/**
 * @file
 * @brief Intel SoC FPGA Agilex customized SDM Mailbox communication
 *        protocol handler. SDM Mailbox protocol will be embedded in
 *        Arm SiP Services SMC protocol and sent to/from SDM via Arm
 *        SiP Services.
 */

#define SIP_SVP_MB_MAX_WORD_SIZE		1024
#define SIP_SVP_MB_HEADER_TRANS_ID_OFFSET	24
#define SIP_SVP_MB_HEADER_TRANS_ID_MASK		0xFF
#define SIP_SVP_MB_HEADER_LENGTH_OFFSET		12
#define SIP_SVP_MB_HEADER_LENGTH_MASK		0x7FF

/** @brief Bit offset of the SDM mailbox header job ID field */
#define SIP_SVP_MB_HEADER_JOB_ID_OFFSET		24

/** @brief Bit mask of the SDM mailbox header job ID field */
#define SIP_SVP_MB_HEADER_JOB_ID_MASK		0xF

/** @brief Bit offset of the SDM mailbox header client ID field */
#define SIP_SVP_MB_HEADER_CLIENT_ID_OFFSET	28

/** @brief Bit mask of the SDM mailbox header client ID field */
#define SIP_SVP_MB_HEADER_CLIENT_ID_MASK	0xF

/**
 * @brief TF-A mailbox client ID used for SiP V2 async poll matching
 *
 * TF-A routes async mailbox responses for the SiP V2 path through the V3
 * service using MBOX_ATF_CLIENT_ID. The mailbox header client/job fields
 * (and matching SMC a1 byte) must use this value, not the sip_svc client index.
 */
#define SIP_SVC_MBOX_ATF_CLIENT_ID		0x1U

#define SIP_SVC_MB_HEADER_GET_CLIENT_ID(header) \
	((header) >> SIP_SVP_MB_HEADER_CLIENT_ID_OFFSET & \
		SIP_SVP_MB_HEADER_CLIENT_ID_MASK)

#define SIP_SVC_MB_HEADER_GET_TRANS_ID(header) \
	((header) >> SIP_SVP_MB_HEADER_TRANS_ID_OFFSET & \
		SIP_SVP_MB_HEADER_TRANS_ID_MASK)

#define SIP_SVC_MB_HEADER_SET_TRANS_ID(header, id) \
	(header) &= ~(SIP_SVP_MB_HEADER_TRANS_ID_MASK << \
		SIP_SVP_MB_HEADER_TRANS_ID_OFFSET); \
	(header) |= (((id) & SIP_SVP_MB_HEADER_TRANS_ID_MASK) << \
		SIP_SVP_MB_HEADER_TRANS_ID_OFFSET);

#define SIP_SVC_MB_HEADER_GET_LENGTH(header) \
	((header) >> SIP_SVP_MB_HEADER_LENGTH_OFFSET & \
		SIP_SVP_MB_HEADER_LENGTH_MASK)

#endif /* ZEPHYR_INCLUDE_SIP_SVC_AGILEX_MB_H_ */
