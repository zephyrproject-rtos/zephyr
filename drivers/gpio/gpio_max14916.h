/*
 * Copyright (c) 2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_MAX14916_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_MAX14916_H_

#define MAX14906_ENABLE  1
#define MAX14906_DISABLE 0

#define MAX149x6_MAX_PKT_SIZE 3

#define MAX14916_CHANNELS 8

#define MAX14916_SETOUT_REG      0x0
#define MAX14916_SET_FLED_REG    0x1
#define MAX14916_SET_SLED_REG    0x2
#define MAX14916_INT_REG         0x3
#define MAX14916_OVR_LD_REG      0x4
#define MAX14916_CURR_LIM_REG    0x5
#define MAX14916_OW_OFF_FLT_REG  0x6
#define MAX14916_OW_ON_FLT_REG   0x7
#define MAX14916_SHT_VDD_FLT_REG 0x8
#define MAX14916_GLOB_ERR_REG    0x9
#define MAX14916_OW_OFF_EN_REG   0xA
#define MAX14916_OW_ON_EN_REG    0xB
#define MAX14916_SHT_VDD_EN_REG  0xC
#define MAX14916_CONFIG1_REG     0xD
#define MAX14916_CONFIG2_REG     0xE
#define MAX14916_CONFIG_MASK     0xF

#define MAX149x6_CHIP_ADDR_MASK GENMASK(7, 6)
#define MAX149x6_ADDR_MASK      GENMASK(4, 1)
#define MAX149x6_RW_MASK        BIT(0)

/* DoiLevel register */
#define MAX149x6_DOI_LEVEL_MASK(x) BIT(x)

/* SetOUT register */
#define MAX14906_HIGHO_MASK(x) BIT(x)

#define MAX14906_DO_MASK(x)     (GENMASK(1, 0) << (2 * (x)))
#define MAX14906_CH_DIR_MASK(x) BIT((x) + 4)
#define MAX14906_CH(x)          (x)
#define MAX14906_IEC_TYPE_MASK  BIT(7)
#define MAX14906_CL_MASK(x)     (GENMASK(1, 0) << (2 * (x)))

/* Config1 register */
#define MAX14906_SLED_MASK BIT(1)
#define MAX14906_FLED_MASK BIT(0)

#define MAX14906_CHAN_MASK_LSB(x) BIT(x)
#define MAX14906_CHAN_MASK_MSB(x) BIT((x) + 4)

enum max149x6_spi_addr {
	MAX14906_ADDR_0, /* A0=0, A1=0 */
	MAX14906_ADDR_1, /* A0=1, A1=0 */
	MAX14906_ADDR_2, /* A0=0, A1=1 */
	MAX14906_ADDR_3, /* A0=1, A1=1 */
};

enum max14916_fled_time {
	MAX14916_FLED_TIME_DISABLED,
	MAX14916_FLED_TIME_1S,
	MAX14916_FLED_TIME_2S,
	MAX14916_FLED_TIME_3S
};

enum max14916_sled_state {
	MAX14916_SLED_OFF,
	MAX14916_SLED_ON
};

enum max14916_wd {
	MAX14916_WD_DISABLED,
	MAX14916_WD_200MS,
	MAX14916_WD_600MS,
	MAX14916_WD_1200MS
};

enum max14916_ow_off_cs {
	MAX14916_OW_OFF_CS_20UA,
	MAX14916_OW_OFF_CS_100UA,
	MAX14916_OW_OFF_CS_300UA,
	MAX14916_OW_OFF_CS_600UA
};

enum max14916_sht_vdd_thr {
	MAX14916_SHT_VDD_THR_9V,
	MAX14916_SHT_VDD_THR_10V,
	MAX14916_SHT_VDD_THR_12V,
	MAX14916_SHT_VDD_THR_14V
};

union max14916_interrupt {
	uint8_t reg_raw;
	struct {
		uint8_t OVER_LD_FLT: 1; /* BIT0 */
		uint8_t CURR_LIM: 1;
		uint8_t OW_OFF_FLT: 1;
		uint8_t OW_ON_FLT: 1;
		uint8_t SHT_VDD_FLT: 1;
		uint8_t DE_MAG_FLT: 1;
		uint8_t SUPPLY_ERR: 1;
		uint8_t COM_ERR: 1; /* BIT7 */
	} reg_bits;
};

union max14916_config1 {
	uint8_t reg_raw;
	struct {
		uint8_t FLED_SET: 1; /* BIT0 */
		uint8_t SLED_SET: 1;
		uint8_t FLED_STRETCH: 2;
		uint8_t FFILTER_EN: 1;
		uint8_t FILTER_LONG: 1;
		uint8_t FLATCH_EN: 1;
		uint8_t LED_CURR_LIM: 1; /* BIT7 */
	} reg_bits;
};

union max14916_config2 {
	uint8_t reg_raw;
	struct {
		uint8_t VDD_ON_THR: 1; /* BIT0 */
		uint8_t SYNCH_WD_EN: 1;
		uint8_t SHT_VDD_THR: 2;
		uint8_t OW_OFF_CS: 2;
		uint8_t WD_TO: 2; /* BIT7 */
	} reg_bits;
};

union max14916_mask {
	uint8_t reg_raw;
	struct {
		uint8_t OVER_LD_M: 1; /* BIT0 */
		uint8_t CURR_LIM_M: 1;
		uint8_t OW_OFF_M: 1;
		uint8_t OW_ON_M: 1;
		uint8_t SHT_VDD_M: 1;
		uint8_t VDD_OK_M: 1;
		uint8_t SUPPLY_ERR_M: 1;
		uint8_t COM_ERR_M: 1; /* BIT7 */
	} reg_bits;
};

union max14916_global_err {
	uint8_t reg_raw;
	struct {
		uint8_t VINT_UV: 1; /* BIT0 */
		uint8_t VA_UVLO: 1;
		uint8_t VDD_BAD: 1;
		uint8_t VDD_WARN: 1;
		uint8_t VDD_UVLO: 1;
		uint8_t THRMSHUTD: 1;
		uint8_t SYNC_ERR: 1;
		uint8_t WDOG_ERR: 1; /* BIT7 */
	} reg_bits;
};

struct max149x6_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec fault_gpio;
	struct gpio_dt_spec ready_gpio;
	struct gpio_dt_spec sync_gpio;
	struct gpio_dt_spec en_gpio;
	bool crc_en;
	union max14916_config1 config1;
	union max14916_config2 config2;
	enum max149x6_spi_addr spi_addr;
	uint8_t pkt_size;
};

#define max14916_config max149x6_config

struct max14916_data {
	struct gpio_driver_data common;
	struct {
		uint8_t ovr_ld;
		uint8_t curr_lim;
		uint8_t ow_off;
		uint8_t ow_on;
		uint8_t sht_vdd;
	} chan;
	struct {
		uint8_t ow_off_en;
		uint8_t ow_on_en;
		uint8_t sht_vdd_en;
	} chan_en;
	struct {
		union max14916_interrupt interrupt;
		union max14916_global_err glob_err;
		union max14916_mask mask;
	} glob;
};

#endif
