/*
 * Copyright (c) 2010-2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MAX22190_H
#define _MAX22190_H

#include <stdint.h>
#include <stdbool.h>

#define MAX22190_ENABLE 1
#define MAX22190_DISABLE 0

#define MAX22190_WRITE 1
#define MAX22190_READ 0
#define MAX22190_MAX_PKT_SIZE 3
#define MAX22190_CHANNELS 8
#define MAX22190_FAULT2_ENABLES 5

#define MAX22190_WB_REG 0x0
#define MAX22190_DI_REG 0x2
#define MAX22190_FAULT1_REG 0x4
#define MAX22190_FILTER_IN_REG(x) (0x6 + (2 * (x)))
#define MAX22190_CFG_REG 0x18
#define MAX22190_IN_EN_REG 0x1A
#define MAX22190_FAULT2_REG 0x1C
#define MAX22190_FAULT2_EN_REG 0x1E
#define MAX22190_GPO_REG 0x22
#define MAX22190_FAULT1_EN_REG 0x24
#define MAX22190_NOP_REG 0x26

#define MAX22190_CH_STATE_MASK(x) BIT(x)
#define MAX22190_DELAY_MASK GENMASK(2, 0)
#define MAX22190_FBP_MASK BIT(3)
#define MAX22190_WBE_MASK BIT(4)
#define MAX22190_RW_MASK BIT(7)
#define MAX22190_ADDR_MASK GENMASK(6, 0)
#define MAX22190_ALARM_MASK GENMASK(4, 3)
#define MAX22190_POR_MASK BIT(6)

#define MAX22190_FAULT_MASK(x) BIT(x)
#define MAX22190_FAULT2_WBE_MASK BIT(4)

#define MAX22190_FAULT2_EN_MASK GENMASK(5, 0)

#define MAX22190_CFG_REFDI_MASK BIT(0)
#define MAX22190_CFG_CLRF_MASK BIT(3)
#define MAX22190_CFG_24VF_MASK BIT(4)

enum max22190_ch_state {
	MAX22190_CH_OFF,
	MAX22190_CH_ON
};

enum max22190_ch_wb_state {
	MAX22190_CH_NO_WB_BREAK,
	MAX22190_CH_WB_COND_DET
};

enum max22190_mode {
	MAX22190_MODE_0,
	MAX22190_MODE_1,
	MAX22190_MODE_2,
	MAX22190_MODE_3,
};

struct max22190_config {
	struct spi_dt_spec spi;

	struct gpio_dt_spec fault_gpio;
	struct gpio_dt_spec ready_gpio;
	struct gpio_dt_spec latch_gpio;

	bool crc_en;

	enum max22190_mode mode;
	uint8_t pkt_size;
};

struct max22190_data {
	enum max22190_ch_state channels[MAX22190_CHANNELS];
	enum max22190_ch_wb_state wb[MAX22190_CHANNELS];
};

typedef struct _max22190_fault1 {
	uint8_t max22190_WBG:1; /* BIT 0 */
	uint8_t max22190_24VM:1;
	uint8_t max22190_24VL:1;
	uint8_t max22190_ALRMT1:1;
	uint8_t max22190_ALRMT2:1;
	uint8_t max22190_FAULT2:1;
	uint8_t max22190_POR:1;
	uint8_t max22190_CRC:1; /* BIT 7 */
} max22190_fault1;

typedef struct _max22190_fault1_en {
	uint8_t max22190_WBGE:1; /* BIT 0 */
	uint8_t max22190_24VME:1;
	uint8_t max22190_24VLE:1;
	uint8_t max22190_ALRMT1E:1;
	uint8_t max22190_ALRMT2E:1;
	uint8_t max22190_FAULT2E:1;
	uint8_t max22190_PORE:1;
	uint8_t max22190_CRCE:1; /* BIT 7 */
} max22190_fault1_en;

typedef struct _max22190_fault2 {
	uint8_t max22190_RFWBS:1; /* BIT 0 */
	uint8_t max22190_RFWBO:1;
	uint8_t max22190_RFDIS:1;
	uint8_t max22190_RFDIO:1;
	uint8_t max22190_OTSHDN:1;
	uint8_t max22190_FAULT8CK:1;
	uint8_t max22190_DUMMY:2; /* BIT 7 */
} max22190_fault2;

typedef struct _max22190_fault2_en {
	uint8_t max22190_RFWBSE:1; /* BIT 0 */
	uint8_t max22190_RFWBOE:1;
	uint8_t max22190_RFDISE:1;
	uint8_t max22190_RFDIOE:1;
	uint8_t max22190_OTSHDNE:1;
	uint8_t max22190_FAULT8CKE:1;
	uint8_t max22190_DUMMY:2; /* BIT 7 */
} max22190_fault2_en;

typedef struct _max22190_cfg {
	uint8_t max22190_DUMMY1:3; /* BIT 7 */
	uint8_t max22190_24VF:1;
	uint8_t max22190_CLRF:1;
	uint8_t max22190_DUMMY2:2;
	uint8_t max22190_REFDI_SH_EN:1; /* BIT 0 */
} max22190_cfg;

typedef struct _max22190_flt {
	uint8_t max22190_DELAY:3; /* BIT 0 */
	uint8_t max22190_FBP:1;
	uint8_t max22190_WBE:1;
	uint8_t max22190_DUMMY:2; /* BIT 7 */
} max22190_flt;

enum max22190_delay {
	MAX22190_DELAY_50US,
	MAX22190_DELAY_100US,
	MAX22190_DELAY_400US,
	MAX22190_DELAY_800US,
	MAX22190_DELAY_1800US,
	MAX22190_DELAY_3200US,
	MAX22190_DELAY_12800US,
	MAX22190_DELAY_20MS
};

#endif
