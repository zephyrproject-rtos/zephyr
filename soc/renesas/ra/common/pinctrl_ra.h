/*
 * Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_RENESAS_RA_COMMON_RA_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_RENESAS_RA_COMMON_RA_PINCTRL_SOC_H_

enum {
	PmnPFS_PODR_POS,
	PmnPFS_PIDR_POS,
	PmnPFS_PDR_POS,
	PmnPFS_RSV3_POS,
	PmnPFS_PCR_POS,
	PmnPFS_RSV5_POS,
	PmnPFS_NCODR_POS,
	PmnPFS_RSV7_POS,
	PmnPFS_RSV8_POS,
	PmnPFS_RSV9_POS,
	PmnPFS_DSCR_POS,
	PmnPFS_DSCR1_POS,
	PmnPFS_EOR_POS,
	PmnPFS_EOF_POS,
	PmnPFS_ISEL_POS,
	PmnPFS_ASEL_POS,
	PmnPFS_PMR_POS,
};

struct pinctrl_ra_pin {
	union {
		uint32_t config;
		struct {
			uint8_t PODR: 1;
			uint8_t PIDR: 1;
			uint8_t PDR: 1;
			uint8_t RESERVED3: 1;
			uint8_t PCR: 1;
			uint8_t RESERVED5: 1;
			uint8_t NCODR: 1;
			uint8_t RESERVED7: 1;
			uint8_t RESERVED8: 1;
			uint8_t RESERVED9: 1;
			uint8_t DSCR: 2;
			uint8_t EOFR: 2;
			uint8_t ISEL: 1;
			uint8_t ASEL: 1;
			uint8_t PMR: 1;
			uint8_t RESERVED17: 7;
			uint8_t PSEL: 5;
			uint8_t RESERVED29: 3;
		};
		/* Using RESERVED fields for store pin and port info. */
		struct {
			uint32_t UNUSED0: 17;
			uint8_t pin: 4;
			uint8_t port: 3;
			uint32_t UNUSED24: 5;
			uint8_t port4: 1;
			uint32_t UNUSED30: 2;
		};
	};
};

typedef struct pinctrl_ra_pin pinctrl_soc_pin_t;

extern int pinctrl_ra_query_config(uint32_t port, uint32_t pin,
				   struct pinctrl_ra_pin *const pincfg);

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.config = DT_PROP_BY_IDX(node_id, prop, idx),                                      \
	},

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,    \
				       Z_PINCTRL_STATE_PIN_INIT)                                   \
	}

#endif /* ZEPHYR_SOC_ARM_RENESAS_RA_COMMON_RA_PINCTRL_SOC_H_ */
