/*
 * Copyright (c) 2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_MAX14906_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_MAX14906_H_

#define MAX14906_FAULT2_ENABLES 5
#define MAX14906_CHANNELS       4
#define MAX14916_CHANNELS       8
#define MAX149x6_MAX_PKT_SIZE   3

#define MAX14906_SETOUT_REG      0x0
#define MAX14906_SETLED_REG      0x1
#define MAX14906_DOILEVEL_REG    0x2
#define MAX14906_INT_REG         0x3
#define MAX14906_OVR_LD_REG      0x4
#define MAX14906_OPN_WIR_FLT_REG 0x5
#define MAX14906_SHT_VDD_FLT_REG 0x6
#define MAX14906_GLOB_ERR_REG    0x7
#define MAX14906_OPN_WR_EN_REG   0x8
#define MAX14906_SHT_VDD_EN_REG  0x9
#define MAX14906_CONFIG1_REG     0xA
#define MAX14906_CONFIG2_REG     0xB
#define MAX14906_CONFIG_DI_REG   0xC
#define MAX14906_CONFIG_DO_REG   0xD
#define MAX14906_CONFIG_CURR_LIM 0xE
#define MAX14906_CONFIG_MASK     0xF

#define MAX149x6_CHIP_ADDR_MASK GENMASK(7, 6)
#define MAX149x6_ADDR_MASK      GENMASK(4, 1)
#define MAX149x6_RW_MASK        BIT(0)

/* DoiLevel register */
#define MAX14906_DOI_LEVEL_MASK(x) BIT(x)

/* SetOUT register */
#define MAX14906_HIGHO_MASK(x) BIT(x)

#define MAX14906_DO_MASK(x)     (GENMASK(1, 0) << (2 * (x)))
#define MAX14906_CH_DIR_MASK(x) BIT((x) + 4)
#define MAX14906_CH(x)          (x)
#define MAX14906_IEC_TYPE_MASK  BIT(7)
#define MAX14906_CL_MASK(x)     (GENMASK(1, 0) << (2 * (x)))

/**
 * @brief Hardwired device address
 */
enum max149x6_spi_addr {
	MAX14906_ADDR_0, /* A0=0, A1=0 */
	MAX14906_ADDR_1, /* A0=1, A1=0 */
	MAX14906_ADDR_2, /* A0=0, A1=1 */
	MAX14906_ADDR_3, /* A0=1, A1=1 */
};

enum max14906_iec_type {
	MAX14906_TYPE_1_3,
	MAX14906_TYPE_2,
};

/**
 * @brief Channel configuration options.
 */
enum max14906_function {
	MAX14906_OUT,
	MAX14906_IN,
	MAX14906_HIGH_Z
};

/**
 * @brief Configuration options for the output driver (on each channel).
 */
enum max14906_do_mode {
	MAX14906_HIGH_SIDE,
	MAX14906_HIGH_SIDE_INRUSH,
	MAX14906_PUSH_PULL_CLAMP,
	MAX14906_PUSH_PULL
};

/**
 * @brief Current limit options for output channels.
 */
enum max14906_cl {
	MAX14906_CL_600,
	MAX14906_CL_130,
	MAX14906_CL_300,
	MAX14906_CL_1200,
};

union max14906_doi_level {
	uint8_t reg_raw;
	struct {
		uint8_t VDDOK_FAULT1: 1; /* BIT0 */
		uint8_t VDDOK_FAULT2: 1;
		uint8_t VDDOK_FAULT3: 1;
		uint8_t VDDOK_FAULT4: 1;
		uint8_t SAFE_DAMAGE_F1: 1;
		uint8_t SAFE_DAMAGE_F2: 1;
		uint8_t SAFE_DAMAGE_F3: 1;
		uint8_t SAFE_DAMAGE_F4: 1; /* BIT7 */
	} reg_bits;
};

union max14906_interrupt {
	uint8_t reg_raw;
	struct {
		uint8_t OVER_LD_FAULT: 1; /* BIT0 */
		uint8_t CURR_LIM: 1;
		uint8_t OW_OFF_FAULT: 1;
		uint8_t ABOVE_VDD_FAULT: 1;
		uint8_t SHT_VDD_FAULT: 1;
		uint8_t DE_MAG_FAULT: 1;
		uint8_t SUPPLY_ERR: 1;
		uint8_t COM_ERR: 1; /* BIT7 */
	} reg_bits;
};

union max14906_ovr_ld_chf {
	uint8_t reg_raw;
	struct {
		uint8_t OVL1: 1; /* BIT0 */
		uint8_t OVL2: 1;
		uint8_t OVL3: 1;
		uint8_t OVL4: 1;
		uint8_t CL1: 1;
		uint8_t CL2: 1;
		uint8_t CL3: 1;
		uint8_t CL4: 1; /* BIT7 */
	} reg_bits;
};

union max14906_opn_wir_chf {
	uint8_t reg_raw;
	struct {
		uint8_t OW_OFF1: 1; /* BIT0 */
		uint8_t OW_OFF2: 1;
		uint8_t OW_OFF3: 1;
		uint8_t OW_OFF4: 1;
		uint8_t ABOVE_VDD1: 1;
		uint8_t ABOVE_VDD2: 1;
		uint8_t ABOVE_VDD3: 1;
		uint8_t ABOVE_VDD4: 1; /* BIT7 */
	} reg_bits;
};

union max14906_sht_vdd_chf {
	uint8_t reg_raw;
	struct {
		uint8_t SHVDD1: 1; /* BIT0 */
		uint8_t SHVDD2: 1;
		uint8_t SHVDD3: 1;
		uint8_t SHVDD4: 1;
		uint8_t VDDOV1: 1;
		uint8_t VDDOV2: 1;
		uint8_t VDDOV3: 1;
		uint8_t VDDOV4: 1; /* BIT7 */
	} reg_bits;
};

union max14906_global_err {
	uint8_t reg_raw;
	struct {
		uint8_t VINT_UV: 1; /* BIT0 */
		uint8_t V5_UVLO: 1;
		uint8_t VDD_LOW: 1;
		uint8_t VDD_WARN: 1;
		uint8_t VDD_UVLO: 1;
		uint8_t THRMSHUTD: 1;
		uint8_t LOSSGND: 1;
		uint8_t WDOG_ERR: 1; /* BIT7 */
	} reg_bits;
};

union max14906_opn_wr_en {
	uint8_t reg_raw;
	struct {
		uint8_t OW_OFF_EN1: 1; /* BIT0 */
		uint8_t OW_OFF_EN2: 1;
		uint8_t OW_OFF_EN3: 1;
		uint8_t OW_OFF_EN4: 1;
		uint8_t GDRV_EN1: 1;
		uint8_t GDRV_EN2: 1;
		uint8_t GDRV_EN3: 1;
		uint8_t GDRV_EN4: 1; /* BIT7 */
	} reg_bits;
};

union max14906_sht_vdd_en {
	uint8_t reg_raw;
	struct {
		uint8_t SH_VDD_EN1: 1; /* BIT0 */
		uint8_t SH_VDD_EN2: 1;
		uint8_t SH_VDD_EN3: 1;
		uint8_t SH_VDD_EN4: 1;
		uint8_t VDD_OV_EN1: 1;
		uint8_t VDD_OV_EN2: 1;
		uint8_t VDD_OV_EN3: 1;
		uint8_t VDD_OV_EN4: 1; /* BIT7 */
	} reg_bits;
};

union max14906_config_di {
	uint8_t reg_raw;
	struct {
		uint8_t OVL_BLANK: 2; /* BIT0 */
		uint8_t OVL_STRETCH_EN: 1;
		uint8_t ABOVE_VDD_PROT_EN: 1;
		uint8_t VDD_FAULT_SEL: 1;
		uint8_t VDD_FAULT_DIS: 1;
		uint8_t RESERVED: 1;
		uint8_t TYP_2_DI: 1; /* BIT7 */
	} reg_bits;
};

union max14906_config_do {
	uint8_t reg_raw;
	struct {
		uint8_t DO_MODE1: 2; /* BIT0 */
		uint8_t DO_MODE2: 2;
		uint8_t DO_MODE3: 2;
		uint8_t DO_MODE4: 2; /* BIT7 */
	} reg_bits;
};

union max14906_config_curr_lim {
	uint8_t reg_raw;
	struct {
		uint8_t CL1: 2; /* BIT0 */
		uint8_t CL2: 2;
		uint8_t CL3: 2;
		uint8_t CL4: 2; /* BIT7 */
	} reg_bits;
};

union max14906_mask {
	uint8_t reg_raw;
	struct {
		uint8_t OVER_LD_M: 1; /* BIT0 */
		uint8_t CURR_LIM_M: 1;
		uint8_t OW_OFF_M: 1;
		uint8_t ABOVE_VDD_M: 1;
		uint8_t SHT_VDD_M: 1;
		uint8_t VDD_OK_M: 1;
		uint8_t SUPPLY_ERR_M: 1;
		uint8_t COM_ERR_M: 1; /* BIT7 */
	} reg_bits;
};

union max14906_config1 {
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

union max14906_config2 {
	uint8_t reg_raw;
	struct {
		uint8_t VDD_ON_THR: 1; /* BIT0 */
		uint8_t SYNCH_WD_EN: 1;
		uint8_t SHT_VDD_THR: 2;
		uint8_t OW_OFF_CS: 2;
		uint8_t WD_TO: 2; /* BIT7 */
	} reg_bits;
};

/* Config1 register Enable/Disable SLED */
#define MAX149x6_SLED_MASK BIT(1)
/* Config1 register Enable/Disable FLED */
#define MAX149x6_FLED_MASK BIT(0)

#define MAX149x6_ENABLE  1
#define MAX149x6_DISABLE 0

struct max149x6_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec fault_gpio;
	struct gpio_dt_spec ready_gpio;
	struct gpio_dt_spec sync_gpio;
	struct gpio_dt_spec en_gpio;
	bool crc_en;
	union max14906_config1 config1;
	union max14906_config2 config2;
	union max14906_config_curr_lim curr_lim;
	union max14906_config_di config_do;
	union max14906_config_do config_di;
	enum max149x6_spi_addr spi_addr;
	uint8_t pkt_size;
};

#define max14906_config max149x6_config

struct max14906_data {
	struct gpio_driver_data common;
	struct {
		union max14906_doi_level doi_level;
		union max14906_ovr_ld_chf ovr_ld;
		union max14906_opn_wir_chf opn_wir;
		union max14906_sht_vdd_chf sht_vdd;
	} chan;
	struct {
		union max14906_opn_wr_en opn_wr_en;
		union max14906_sht_vdd_en sht_vdd_en;
	} chan_en;
	struct {
		union max14906_interrupt interrupt;
		union max14906_global_err glob_err;
		union max14906_mask mask;
	} glob;
};

#endif
