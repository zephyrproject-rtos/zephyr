/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>

#include <Qspi_Ip.h>

/**
 * @brief Build a QSPI Look-up Table (LUT) sequence entry.
 *
 * @param inst instruction
 * @param pads pad information
 * @param op operand
 */
#define QSPI_LUT_OP(inst, pads, op)				\
	((Qspi_Ip_InstrOpType)((Qspi_Ip_InstrOpType)(inst)	\
		| (Qspi_Ip_InstrOpType)(pads)			\
		| (Qspi_Ip_InstrOpType)(op)))

/**
 * @brief Get the QSPI peripheral hardware instance number.
 *
 * @param dev device pointer
 */
uint8_t memc_nxp_s32_qspi_get_instance(const struct device *dev);
