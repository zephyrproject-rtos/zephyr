/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file btca.c
 * @brief Interface for Best TimeTransmitter Clock Algorithm.
 *
 * References are to version 2019 of IEEE 1588, ("PTP")
 */

#ifndef ZEPHYR_INCLUDE_PTP_BTCA_H_
#define ZEPHYR_INCLUDE_PTP_BTCA_H_

#include "ds.h"
#include "port.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function comparing two datasets.
 *
 * @param[in] a Pointer to the first dataset.
 * @param[in] b Pointer to the second dataset.
 *
 * @return Negative if b is better than a, 0 if a == b, else positive.
 */
int ptp_btca_ds_cmp(const struct ptp_dataset *a, const struct ptp_dataset *b);

/**
 * @brief Function performing Best TimeTransmitter Clock state decision algorithm.
 *
 * @param[in] port Pointer to a PTP Port.
 *
 * @return Proposed PTP Port's state after execution of the state decision algorithm.
 */
enum ptp_port_state ptp_btca_state_decision(struct ptp_port *port);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PTP_BCMA_H_ */
