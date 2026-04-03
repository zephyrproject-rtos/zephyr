/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RA Ethernet RMAC header file
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ETH_RENESAS_RA_RMAC_H__
#define ZEPHYR_INCLUDE_DRIVERS_ETH_RENESAS_RA_RMAC_H__

#include <stdint.h>

/**
 * @defgroup renesas_ra_eth_rmac_defs Renesas RA Ethernet Definitions
 * @brief Common constants for Renesas RA Ethernet drivers.
 * @{
 */

#define RENESAS_RA_MPIC_LSC_10   0 /**< MPIC link speed: 10 Mbps. */
#define RENESAS_RA_MPIC_LSC_100  1 /**< MPIC link speed: 100 Mbps. */
#define RENESAS_RA_MPIC_LSC_1000 2 /**< MPIC link speed: 1000 Mbps. */

#define RENESAS_RA_ETHA_DISABLE_MODE   1 /**< RMAC disable mode. */
#define RENESAS_RA_ETHA_CONFIG_MODE    2 /**< RMAC configuration mode. */
#define RENESAS_RA_ETHA_OPERATION_MODE 3 /**< RMAC operation mode. */

#define RENESAS_RA_ETHA_REG_SIZE (R_ETHA1_BASE - R_ETHA0_BASE) /**< Size of ETHA register area. */

/** @} end of renesas_ra_eth_rmac_defs group */

/**
 * @brief Get the current PHY operation mode.
 *
 * @param[in] p_instance_ctrl Pointer to RMAC PHY instance control structure.
 *
 * @return Current operation mode of ETHA IP.
 */
static inline uint8_t r_rmac_phy_get_operation_mode(uint8_t channel)
{
	R_ETHA0_Type *p_etha_reg =
		(R_ETHA0_Type *)(R_ETHA0_BASE + (RENESAS_RA_ETHA_REG_SIZE * channel));

	/* Return operation mode of ETHA IP. */
	return p_etha_reg->EAMS_b.OPS;
}

/**
 * @brief Set the PHY operation mode.
 *
 * @param[in] channel RMAC channel number.
 * @param[in] mode    PHY operation mode to set.
 */
static inline void r_rmac_phy_set_operation_mode(uint8_t channel, uint8_t mode)
{
	R_ETHA0_Type *p_etha_reg =
		(R_ETHA0_Type *)(R_ETHA0_BASE + (RENESAS_RA_ETHA_REG_SIZE * channel));

	/* Mode transition */
	p_etha_reg->EAMC_b.OPC = R_ETHA0_EAMC_OPC_Msk & mode;

	WAIT_FOR((p_etha_reg->EAMS_b.OPS == mode), 1000, k_busy_wait(1));
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_ETH_RENESAS_RA_RMAC_H__ */
