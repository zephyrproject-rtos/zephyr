/*
 * Copyright (c) 2022 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief API for NXP SJA1000 (and compatible) CAN controller frontend drivers.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_SJA1000_H_
#define ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_SJA1000_H_

#include <zephyr/drivers/can.h>

/**
 * @name SJA1000 Output Control Register (OCR) bits
 *
 * @{
 */
#define CAN_SJA1000_OCR_OCMODE_MASK    GENMASK(1, 0)
#define CAN_SJA1000_OCR_OCPOL0	       BIT(2)
#define CAN_SJA1000_OCR_OCTN0	       BIT(3)
#define CAN_SJA1000_OCR_OCTP0	       BIT(4)
#define CAN_SJA1000_OCR_OCPOL1	       BIT(5)
#define CAN_SJA1000_OCR_OCTN1	       BIT(6)
#define CAN_SJA1000_OCR_OCTP1	       BIT(7)

#define CAN_SJA1000_OCR_OCMODE_BIPHASE FIELD_PREP(CAN_SJA1000_OCR_OCMODE_MASK, 0U)
#define CAN_SJA1000_OCR_OCMODE_TEST    FIELD_PREP(CAN_SJA1000_OCR_OCMODE_MASK, 1U)
#define CAN_SJA1000_OCR_OCMODE_NORMAL  FIELD_PREP(CAN_SJA1000_OCR_OCMODE_MASK, 2U)
#define CAN_SJA1000_OCR_OCMODE_CLOCK   FIELD_PREP(CAN_SJA1000_OCR_OCMODE_MASK, 3U)

/** @} */

/**
 * @name SJA1000 Clock Divider Register (CDR) bits
 *
 * @{
 */
#define CAN_SJA1000_CDR_CD_MASK	       GENMASK(2, 0)
#define CAN_SJA1000_CDR_CLOCK_OFF      BIT(3)
#define CAN_SJA1000_CDR_RXINTEN	       BIT(5)
#define CAN_SJA1000_CDR_CBP	       BIT(6)
#define CAN_SJA1000_CDR_CAN_MODE       BIT(7)

#define CAN_SJA1000_CDR_CD_DIV1	       FIELD_PREP(CAN_SJA1000_CDR_CD_MASK, 7U)
#define CAN_SJA1000_CDR_CD_DIV2	       FIELD_PREP(CAN_SJA1000_CDR_CD_MASK, 0U)
#define CAN_SJA1000_CDR_CD_DIV4	       FIELD_PREP(CAN_SJA1000_CDR_CD_MASK, 1U)
#define CAN_SJA1000_CDR_CD_DIV6	       FIELD_PREP(CAN_SJA1000_CDR_CD_MASK, 2U)
#define CAN_SJA1000_CDR_CD_DIV8	       FIELD_PREP(CAN_SJA1000_CDR_CD_MASK, 3U)
#define CAN_SJA1000_CDR_CD_DIV10       FIELD_PREP(CAN_SJA1000_CDR_CD_MASK, 4U)
#define CAN_SJA1000_CDR_CD_DIV12       FIELD_PREP(CAN_SJA1000_CDR_CD_MASK, 5U)
#define CAN_SJA1000_CDR_CD_DIV14       FIELD_PREP(CAN_SJA1000_CDR_CD_MASK, 6U)

/** @} */

/**
 * @brief SJA1000 specific static initializer for a minimum @p can_timing struct
 */
#define CAN_SJA1000_TIMING_MIN_INITIALIZER				\
	{								\
		.sjw = 1,						\
		.prop_seg = 0,						\
		.phase_seg1 = 1,					\
		.phase_seg2 = 1,					\
		.prescaler = 1						\
	}

/**
 * @brief SJA1000 specific static initializer for a maximum @p can_timing struct
 */
#define CAN_SJA1000_TIMING_MAX_INITIALIZER				\
	{								\
		.sjw = 4,						\
		.prop_seg = 0,						\
		.phase_seg1 = 16,					\
		.phase_seg2 = 8,					\
		.prescaler = 64						\
	}

/**
 * @brief SJA1000 driver front-end callback for writing a register value
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register offset
 * @param val Register value
 */
typedef void (*can_sja1000_write_reg_t)(const struct device *dev, uint8_t reg, uint8_t val);

/**
 * @brief SJA1000 driver front-end callback for reading a register value
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register offset
 * @retval Register value
 */
typedef uint8_t (*can_sja1000_read_reg_t)(const struct device *dev, uint8_t reg);

/**
 * @brief SJA1000 driver internal configuration structure.
 */
struct can_sja1000_config {
	const struct can_driver_config common;
	can_sja1000_read_reg_t read_reg;
	can_sja1000_write_reg_t write_reg;
	uint8_t ocr;
	uint8_t cdr;
	const void *custom;
};

/**
 * @brief Static initializer for @p can_sja1000_config struct
 *
 * @param node_id Devicetree node identifier
 * @param _custom Pointer to custom driver frontend configuration structure
 * @param _read_reg Driver frontend SJA100 register read function
 * @param _write_reg Driver frontend SJA100 register write function
 * @param _ocr Initial SJA1000 Output Control Register (OCR) value
 * @param _cdr Initial SJA1000 Clock Divider Register (CDR) value
 * @param _min_bitrate minimum bitrate supported by the CAN controller
 */
#define CAN_SJA1000_DT_CONFIG_GET(node_id, _custom, _read_reg, _write_reg, _ocr, _cdr,             \
				  _min_bitrate)                                                    \
	{                                                                                          \
		.common = CAN_DT_DRIVER_CONFIG_GET(node_id, _min_bitrate, 1000000),                \
		.read_reg = _read_reg,                                                             \
		.write_reg = _write_reg,                                                           \
		.ocr = _ocr,                                                                       \
		.cdr = _cdr,                                                                       \
		.custom = _custom,                                                                 \
	}

/**
 * @brief Static initializer for @p can_sja1000_config struct from a DT_DRV_COMPAT instance
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param _custom Pointer to custom driver frontend configuration structure
 * @param _read_reg Driver frontend SJA100 register read function
 * @param _write_reg Driver frontend SJA100 register write function
 * @param _ocr Initial SJA1000 Output Control Register (OCR) value
 * @param _cdr Initial SJA1000 Clock Divider Register (CDR) value
 * @param _min_bitrate minimum bitrate supported by the CAN controller
 * @see CAN_SJA1000_DT_CONFIG_GET()
 */
#define CAN_SJA1000_DT_CONFIG_INST_GET(inst, _custom, _read_reg, _write_reg, _ocr, _cdr,           \
				       _min_bitrate)                                               \
	CAN_SJA1000_DT_CONFIG_GET(DT_DRV_INST(inst), _custom, _read_reg, _write_reg, _ocr, _cdr,   \
				  _min_bitrate)

/**
 * @brief SJA1000 driver internal RX filter structure.
 */
struct can_sja1000_rx_filter {
	struct can_filter filter;
	can_rx_callback_t callback;
	void *user_data;
};

/**
 * @brief SJA1000 driver internal data structure.
 */
struct can_sja1000_data {
	struct can_driver_data common;
	ATOMIC_DEFINE(rx_allocs, CONFIG_CAN_MAX_FILTER);
	struct can_sja1000_rx_filter filters[CONFIG_CAN_MAX_FILTER];
	struct k_mutex mod_lock;
	enum can_state state;
	struct k_sem tx_idle;
	can_tx_callback_t tx_callback;
	void *tx_user_data;
	void *custom;
};

/**
 * @brief Initializer for a @a can_sja1000_data struct
 * @param _custom Pointer to custom driver frontend data structure
 */
#define CAN_SJA1000_DATA_INITIALIZER(_custom)                                                      \
	{                                                                                          \
		.custom = _custom,                                                                 \
	}

/**
 * @brief SJA1000 callback API upon setting CAN bus timing
 * See @a can_set_timing() for argument description
 */
int can_sja1000_set_timing(const struct device *dev, const struct can_timing *timing);

/**
 * @brief SJA1000 callback API upon getting CAN controller capabilities
 * See @a can_get_capabilities() for argument description
 */
int can_sja1000_get_capabilities(const struct device *dev, can_mode_t *cap);

/**
 * @brief SJA1000 callback API upon starting CAN controller
 * See @a can_start() for argument description
 */
int can_sja1000_start(const struct device *dev);

/**
 * @brief SJA1000 callback API upon stopping CAN controller
 * See @a can_stop() for argument description
 */
int can_sja1000_stop(const struct device *dev);

/**
 * @brief SJA1000 callback API upon setting CAN controller mode
 * See @a can_set_mode() for argument description
 */
int can_sja1000_set_mode(const struct device *dev, can_mode_t mode);

/**
 * @brief SJA1000 callback API upon sending a CAN frame
 * See @a can_send() for argument description
 */
int can_sja1000_send(const struct device *dev, const struct can_frame *frame, k_timeout_t timeout,
		     can_tx_callback_t callback, void *user_data);

/**
 * @brief SJA1000 callback API upon adding an RX filter
 * See @a can_add_rx_callback() for argument description
 */
int can_sja1000_add_rx_filter(const struct device *dev, can_rx_callback_t callback, void *user_data,
			      const struct can_filter *filter);

/**
 * @brief SJA1000 callback API upon removing an RX filter
 * See @a can_remove_rx_filter() for argument description
 */
void can_sja1000_remove_rx_filter(const struct device *dev, int filter_id);

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
/**
 * @brief SJA1000 callback API upon recovering the CAN bus
 * See @a can_recover() for argument description
 */
int can_sja1000_recover(const struct device *dev, k_timeout_t timeout);
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

/**
 * @brief SJA1000 callback API upon getting the CAN controller state
 * See @a can_get_state() for argument description
 */
int can_sja1000_get_state(const struct device *dev, enum can_state *state,
			  struct can_bus_err_cnt *err_cnt);

/**
 * @brief SJA1000 callback API upon setting a state change callback
 * See @a can_set_state_change_callback() for argument description
 */
void can_sja1000_set_state_change_callback(const struct device *dev,
					   can_state_change_callback_t callback, void *user_data);

/**
 * @brief SJA1000 callback API upon getting the maximum number of concurrent CAN RX filters
 * See @a can_get_max_filters() for argument description
 */
int can_sja1000_get_max_filters(const struct device *dev, bool ide);

/**
 * @brief SJA1000 IRQ handler callback.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
void can_sja1000_isr(const struct device *dev);

/**
 * @brief SJA1000 driver initialization callback.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
int can_sja1000_init(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_SJA1000_H_ */
