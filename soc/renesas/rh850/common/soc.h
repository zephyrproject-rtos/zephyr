/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RENESAS_RH850_COMMON_SOC_H_
#define ZEPHYR_SOC_RENESAS_RH850_COMMON_SOC_H_

/* System controller, at the same addresses on U2A, U2B and U2C */

/* Clock controller */
#define RH850_CKS_RLINC  0xff980140U
#define RH850_CKS_RLINS  0xff980148U
#define RH850_CLKKCPROT1 0xff980700U

/* Reset controller */
#define RH850_SWMRESS_RLIN3 0xff980964U
#define RH850_SWMRESS_OSTM  0xff980a84U

/* Standby controller */
#define RH850_MSR_RLIN3 0xff981060U
#define RH850_MSR_OSTM  0xff981180U
#define RH850_MSRKCPROT 0xff981710U

/* Write protection keys */
#define RH850_KCPROT_ENABLE  0xa5a5a501U
#define RH850_KCPROT_DISABLE 0xa5a5a500U

/* CKS_RLINC/CKS_RLINS bits 2-0: CLK_RLIN3 from CLK_HSB */
#define RH850_CKS_RLIN_HSB  1U
#define RH850_CKS_RLIN_MASK 0x7U

#endif /* ZEPHYR_SOC_RENESAS_RH850_COMMON_SOC_H_ */
