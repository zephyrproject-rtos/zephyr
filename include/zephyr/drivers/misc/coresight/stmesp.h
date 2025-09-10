/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_CORESIGHT_STMESP_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_CORESIGHT_STMESP_H_

#include <zephyr/devicetree.h>

/**
 * @brief Interfaces for Coresight STMESP (STM Extended Stimulus Port)
 * @defgroup stmsp_interface Coresight STMESP
 * @ingroup misc_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN
 * @brief STMESP register structure.
 */
typedef struct {
	volatile uint32_t G_DMTS[2];
	volatile uint32_t G_DM[2];
	volatile uint32_t G_DTS[2];
	volatile uint32_t G_D[2];
	volatile uint32_t RESERVED0[16];
	volatile uint32_t G_FLAGTS[2];
	volatile uint32_t G_FLAG[2];
	volatile uint32_t G_TRIGTS[2];
	volatile uint32_t G_TRIG[2];
	volatile uint32_t I_DMTS[2];
	volatile uint32_t I_DM[2];
	volatile uint32_t I_DTS[2];
	volatile uint32_t I_D[2];
	volatile uint32_t RESERVED1[16];
	volatile uint32_t I_FLAGTS[2];
	volatile uint32_t I_FLAG[2];
	volatile uint32_t I_TRIGTS[2];
	volatile uint32_t I_TRIG[2];
} STMESP_Type;

/** @brief Helper function for getting target register.
 *
 * @param reg STMESP register set.
 * @param ts Use timestamp.
 * @param marked Use marked register.
 * @param guaranteed True to use guaranteed access.
 *
 * @return Address of the register.
 */
static inline volatile void *_stmesp_get_data_reg(STMESP_Type *reg, bool ts,
						  bool marked, bool guaranteed)
{
	if (ts) {
		if (guaranteed) {
			if (marked) {
				return &reg->G_DMTS[0];
			} else {
				return &reg->G_DTS[0];
			}
		} else {
			if (marked) {
				return &reg->I_DMTS[0];
			} else {
				return &reg->I_DTS[0];
			}
		}
	} else {
		if (guaranteed) {
			if (marked) {
				return &reg->G_DM[0];
			} else {
				return &reg->G_D[0];
			}
		} else {
			if (marked) {
				return &reg->I_DM[0];
			} else {
				return &reg->I_D[0];
			}
		}
	}
}

/** @endcond */

/** @brief Write flag to STMESP
 *
 * @param reg STMESP register set.
 * @param data Data written to the flag register.
 * @param ts If true add timestamp.
 * @param guaranteed If true guaranteed write and invariant if false.
 */
static inline void stmesp_flag(STMESP_Type *reg, uint32_t data, bool ts, bool guaranteed)
{
	if (ts) {
		if (guaranteed) {
			reg->G_FLAGTS[0] = data;
		} else {
			reg->I_FLAGTS[0] = data;
		}
	} else {
		if (guaranteed) {
			reg->G_FLAG[0] = data;
		} else {
			reg->I_FLAG[0] = data;
		}
	}
}

/** @brief Write 8 bit data to STMESP
 *
 * @param reg STMESP register set.
 * @param data Byte to write.
 * @param ts If true add timestamp.
 * @param marked If true marked write.
 * @param guaranteed If true guaranteed write and invariant if false.
 */
static inline void stmesp_data8(STMESP_Type *reg, uint8_t data, bool ts,
				bool marked, bool guaranteed)
{
	*(volatile uint8_t *)_stmesp_get_data_reg(reg, ts, marked, guaranteed) = data;
}

/** @brief Write 16 bit data to STMESP
 *
 * @param reg STMESP register set.
 * @param data Half word to write.
 * @param ts If true add timestamp.
 * @param marked If true marked write.
 * @param guaranteed If true guaranteed write and invariant if false.
 */
static inline void stmesp_data16(STMESP_Type *reg, uint16_t data, bool ts,
				 bool marked, bool guaranteed)
{
	*(volatile uint16_t *)_stmesp_get_data_reg(reg, ts, marked, guaranteed) = data;
}

/** @brief Write 32 bit data to STMESP
 *
 * @param reg STMESP register set.
 * @param data Word to write.
 * @param ts If true add timestamp.
 * @param marked If true marked write.
 * @param guaranteed If true guaranteed write and invariant if false.
 */
static inline void stmesp_data32(STMESP_Type *reg, uint32_t data, bool ts,
				 bool marked, bool guaranteed)
{
	*(volatile uint32_t *)_stmesp_get_data_reg(reg, ts, marked, guaranteed) = data;
}

/**
 * @brief Return address of a STM extended stimulus port.
 *
 * Function return a port from the local STMESP instance.
 *
 * @param[in]	idx	Index of the requested stimulus port.
 * @param[out]	port	Location where pointer to the port is written.
 *
 * @retval	-EINVAL if @p idx or @p port is invalid.
 * @retval	0 on success.
 */
static inline int stmesp_get_port(uint32_t idx, STMESP_Type **port)

{
	/* Check if index is within STM ports */
	if ((port == NULL) ||
	    (idx >= (DT_REG_SIZE(DT_NODELABEL(stmesp)) / sizeof(STMESP_Type)))) {
		return -EINVAL;
	}

	STMESP_Type *const base = (STMESP_Type *const)DT_REG_ADDR(DT_NODELABEL(stmesp));

	*port = &base[idx];

	return 0;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_CORESIGHT_STMESP_H_ */
