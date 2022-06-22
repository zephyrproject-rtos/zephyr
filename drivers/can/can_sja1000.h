/*
 * Copyright (c) 2022 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CAN_SJA1000_H_
#define ZEPHYR_DRIVERS_CAN_SJA1000_H_

#include <zephyr/drivers/can.h>

/* Output Control Register (OCR) bits */
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

/* Clock Divider Register (CDR) bits */
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

#define CAN_SJA1000_TIMING_MIN_INITIALIZER				\
	{								\
		.sjw = 1,						\
		.prop_seg = 0,						\
		.phase_seg1 = 1,					\
		.phase_seg2 = 1,					\
		.prescaler = 1						\
	}

#define CAN_SJA1000_TIMING_MAX_INITIALIZER				\
	{								\
		.sjw = 4,						\
		.prop_seg = 0,						\
		.phase_seg1 = 16,					\
		.phase_seg2 = 8,					\
		.prescaler = 64						\
	}

typedef void (*can_sja1000_write_reg_t)(const struct device *dev, uint8_t reg, uint8_t val);

typedef uint8_t (*can_sja1000_read_reg_t)(const struct device *dev, uint8_t reg);

struct can_sja1000_config {
	can_sja1000_read_reg_t read_reg;
	can_sja1000_write_reg_t write_reg;
	uint32_t bitrate;
	uint32_t sample_point;
	uint32_t sjw;
	uint32_t phase_seg1;
	uint32_t phase_seg2;
	const struct device *phy;
	uint32_t max_bitrate;
	uint8_t ocr;
	uint8_t cdr;
	const void *custom;
};

#define CAN_SJA1000_DT_CONFIG_GET(node_id, _custom, _read_reg, _write_reg, _ocr, _cdr)             \
	{                                                                                          \
		.read_reg = _read_reg, .write_reg = _write_reg,                                    \
		.bitrate = DT_PROP(node_id, bus_speed), .sjw = DT_PROP(node_id, sjw),              \
		.phase_seg1 = DT_PROP_OR(node_id, phase_seg1, 0),                                  \
		.phase_seg2 = DT_PROP_OR(node_id, phase_seg2, 0),                                  \
		.sample_point = DT_PROP_OR(node_id, sample_point, 0),                              \
		.max_bitrate = DT_CAN_TRANSCEIVER_MAX_BITRATE(node_id, 1000000), .ocr = _ocr,      \
		.cdr = _cdr, .custom = _custom,                                                    \
	}

#define CAN_SJA1000_DT_CONFIG_INST_GET(inst, _custom, _read_reg, _write_reg, _ocr, _cdr)           \
	CAN_SJA1000_DT_CONFIG_GET(DT_DRV_INST(inst), _custom, _read_reg, _write_reg, _ocr, _cdr)

struct can_sja1000_rx_filter {
	struct zcan_filter filter;
	can_rx_callback_t callback;
	void *user_data;
};

struct can_sja1000_data {
	ATOMIC_DEFINE(rx_allocs, CONFIG_CAN_MAX_FILTER);
	struct can_sja1000_rx_filter filters[CONFIG_CAN_MAX_FILTER];
	struct k_mutex mod_lock;
	can_mode_t mode;
	enum can_state state;
	can_state_change_callback_t state_change_cb;
	void *state_change_cb_data;
	struct k_sem tx_idle;
	struct k_sem tx_done;
	can_tx_callback_t tx_callback;
	void *tx_user_data;
	int tx_status;
	uint32_t sjw;
	void *custom;
};

#define CAN_SJA1000_DATA_INITIALIZER(_custom)                                                      \
	{                                                                                          \
		.custom = _custom,                                                                 \
	}

int can_sja1000_set_timing(const struct device *dev, const struct can_timing *timing);

int can_sja1000_get_capabilities(const struct device *dev, can_mode_t *cap);

int can_sja1000_set_mode(const struct device *dev, can_mode_t mode);

int can_sja1000_send(const struct device *dev, const struct zcan_frame *frame, k_timeout_t timeout,
		     can_tx_callback_t callback, void *user_data);

int can_sja1000_add_rx_filter(const struct device *dev, can_rx_callback_t callback, void *user_data,
			      const struct zcan_filter *filter);

void can_sja1000_remove_rx_filter(const struct device *dev, int filter_id);

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
int can_sja1000_recover(const struct device *dev, k_timeout_t timeout);
#endif /* !CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

int can_sja1000_get_state(const struct device *dev, enum can_state *state,
			  struct can_bus_err_cnt *err_cnt);

void can_sja1000_set_state_change_callback(const struct device *dev,
					   can_state_change_callback_t callback, void *user_data);

int can_sja1000_get_max_filters(const struct device *dev, enum can_ide id_type);

int can_sja1000_get_max_bitrate(const struct device *dev, uint32_t *max_bitrate);

void can_sja1000_isr(const struct device *dev);

int can_sja1000_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_CAN_SJA1000_H_ */
