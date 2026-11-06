/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RENESAS_RH850_U2C_SOC_H_
#define ZEPHYR_SOC_RENESAS_RH850_U2C_SOC_H_

/* Clock controller */
#define U2C_CKS_RLINC  0xff980140U
#define U2C_CKS_RLINS  0xff980148U
#define U2C_CLKKCPROT1 0xff980700U

/* Reset controller */
#define U2C_SWMRESS_RLIN3 0xff980964U
#define U2C_SWMRESS_OSTM  0xff980a84U

/* Standby controller */
#define U2C_MSR_RLIN3 0xff981060U
#define U2C_MSR_OSTM  0xff981180U
#define U2C_MSRKCPROT 0xff981710U

/* Write protection keys */
#define U2C_KCPROT_ENABLE  0xa5a5a501U
#define U2C_KCPROT_DISABLE 0xa5a5a500U

/* CKS_RLINC/CKS_RLINS: CLK_RLIN3 from CLKC_HSB */
#define U2C_CKS_RLIN_HSB 1U

#endif /* ZEPHYR_SOC_RENESAS_RH850_U2C_SOC_H_ */
