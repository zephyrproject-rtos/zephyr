/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADI_MAX30009_MAX30009_H_
#define ZEPHYR_DRIVERS_SENSOR_ADI_MAX30009_MAX30009_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/sensor/max30009.h>

#define DT_DRV_COMPAT adi_max30009
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#define MAX30009_BUS_SPI
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#define MAX30009_BUS_I2C
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#ifdef MAX30009_BUS_SPI
#include <zephyr/drivers/spi.h>
#endif /* MAX30009_BUS_SPI */

#ifdef MAX30009_BUS_I2C
#include <zephyr/drivers/i2c.h>
#endif /* MAX30009_BUS_I2C */

/* Number of 1ms polling attempts to wait for the PLL to lock */
#define MAX30009_PLL_LOCK_RETRIES 20

/* BioZ bandgap settle time before PLL enable (datasheet: up to 6 ms) */
#define MAX30009_BG_SETTLE_MS 6

#define MAX30009_SPI_READ_CMD  0x80u
#define MAX30009_SPI_WRITE_CMD 0x00u

/*
 * MAX30009 registers definition
 */

/* Status Registers */
#define MAX30009_STATUS1 0x00u /* Status Register 1 */
#define MAX30009_STATUS2 0x01u /* Status Register 2 */

/* FIFO Registers */
#define MAX30009_FIFO_WRITE_PTR 0x08u /* FIFO Write Pointer */
#define MAX30009_FIFO_READ_PTR  0x09u /* FIFO Read Pointer */
#define MAX30009_FIFO_COUNTER1  0x0Au /* FIFO Counter 1 */
#define MAX30009_FIFO_COUNTER2  0x0Bu /* FIFO Counter 2 */
#define MAX30009_FIFO_DATA      0x0Cu /* FIFO Data Register */
#define MAX30009_FIFO_CONFIG1   0x0Du /* FIFO Configuration Register 1 */
#define MAX30009_FIFO_CONFIG2   0x0Eu /* FIFO Configuration Register 2 */

/* System Registers */
#define MAX30009_SYSTEM_SYNC        0x10u /* System Sync Register */
#define MAX30009_SYSTEM_CONFIG_1    0x11u /* System Configuration Register 1 */
#define MAX30009_PIN_FUNC_CONFIG    0x12u /* Pin Function Configuration Register */
#define MAX30009_OUTPUT_PIN_CONFIG  0x13u /* Output Pin Configuration Register */
#define MAX30009_I2C_BROADCAST_ADDR 0x14u /* I2C Broadcast Address Register */

/* PLL Configuration Registers */
#define MAX30009_PLL_CONFIG1 0x17u /* PLL Configuration Register 1 */
#define MAX30009_PLL_CONFIG2 0x18u /* PLL Configuration Register 2 */
#define MAX30009_PLL_CONFIG3 0x19u /* PLL Configuration Register 3 */
#define MAX30009_PLL_CONFIG4 0x1Au /* PLL Configuration Register 4 */

/* BioZ Setup Registers */
#define MAX30009_BIOZ_CONFIG1        0x20u /* BioZ Configuration Register 1 */
#define MAX30009_BIOZ_CONFIG2        0x21u /* BioZ Configuration Register 2 */
#define MAX30009_BIOZ_CONFIG3        0x22u /* BioZ Configuration Register 3 */
#define MAX30009_BIOZ_CONFIG4        0x23u /* BioZ Configuration Register 4 */
#define MAX30009_BIOZ_CONFIG5        0x24u /* BioZ Configuration Register 5 */
#define MAX30009_BIOZ_CONFIG6        0x25u /* BioZ Configuration Register 6 */
#define MAX30009_BIOZ_LOW_THRESHOLD  0x26u /* BioZ Low Threshold Register */
#define MAX30009_BIOZ_HIGH_THRESHOLD 0x27u /* BioZ High Threshold Register */
#define MAX30009_BIOZ_CONFIG7        0x28u /* BioZ Configuration Register 7 */

/* BioZ Calibration Registers */
#define MAX30009_BIOZ_MUX_CONFIGURATION1 0x41u /* BioZ MUX Configuration Register 1 */
#define MAX30009_BIOZ_MUX_CONFIGURATION2 0x42u /* BioZ MUX Configuration Register 2 */
#define MAX30009_BIOZ_MUX_CONFIGURATION3 0x43u /* BioZ MUX Configuration Register 3 */
#define MAX30009_BIOZ_MUX_CONFIGURATION4 0x44u /* BioZ MUX Configuration Register 4 */

/* DC Leads Setup Registers */
#define MAX30009_DC_LEADS_CONFIG          0x50u /* DC Leads Configuration Register */
#define MAX30009_DC_LEAD_DETECT_THRESHOLD 0x51u /* DC Lead Detect Threshold Register */

/* Lead Bias Registers */
#define MAX30009_LEAD_BIAS_CONFIG1 0x58u /* Lead Bias Configuration Register */

/* Interrupt Enable Registers */
#define MAX30009_INTERRUPT_ENABLE1 0x80u /* Interrupt Enable Register 1 */
#define MAX30009_INTERRUPT_ENABLE2 0x81u /* Interrupt Enable Register 2 */

/* Part ID Register */
#define MAX30009_PART_ID 0xFFu /* Part ID Register */

/* STATUS 1 MASKS */
#define MAX30009_STATUS1_PWR_RDY_MSK      BIT(0)
#define MAX30009_STATUS1_PHASE_LOCK_MSK   BIT(1)
#define MAX30009_STATUS1_PHASE_UNLOCK_MSK BIT(2)
#define MAX30009_STATUS1_FREQ_LOCK_MSK    BIT(3)
#define MAX30009_STATUS1_FREQ_UNLOCK_MSK  BIT(4)
#define MAX30009_FIFO_DATA_RDY_MSK        BIT(5)
#define MAX30009_STATUS1_A_FULL_MSK       BIT(7)

/* STATUS 2 MASKS */
#define MAX30009_STATUS2_DC_LOFF_NL_MSK BIT(0)
#define MAX30009_STATUS2_DC_LOFF_NH_MSK BIT(1)
#define MAX30009_STATUS2_DC_LOFF_PL_MSK BIT(2)
#define MAX30009_STATUS2_DC_LOFF_PH_MSK BIT(3)
#define MAX30009_STATUS2_DRV_OOR_MSK    BIT(4)
#define MAX30009_STATUS2_BIOZ_UNDR_MSK  BIT(5)
#define MAX30009_STATUS2_BIOZ_OVER_MSK  BIT(6)
#define MAX30009_STATUS2_LON_MSK        BIT(7)

/* FIFO COUNTER MASKS */
#define MAX30009_FIFO_COUNTER1_FIFO_DATA_COUNT_MSB_MSK BIT(7)
#define MAX30009_FIFO_COUNTER1_FIFO_OVF_COUNTER_MSK    GENMASK(6, 0)

/* FIFO CONFIGURATION 2 MASKS */
#define MAX30009_FIFO_CONFIG2_FIFO_ROLLOVER_MSK BIT(1)
#define MAX30009_FIFO_CONFIG2_A_FULL_TYPE_MSK   BIT(2)
#define MAX30009_FIFO_CONFIG2_FIFO_STAT_CLR_MSK BIT(3)
#define MAX30009_FIFO_CONFIG2_FLUSH_FIFO_MSK    BIT(4)
#define MAX30009_FIFO_CONFIG2_FIFO_MARK_MSK     BIT(5)

/* System Sync */
#define MAX30009_SYSTEM_SYNC_TIMING_SYS_RESET_MSK BIT(7)

/* SYSTEM CONFIGURATION 1 MASKS */
#define MAX30009_SYSTEM_CONFIG1_RESET_MSK       BIT(0)
#define MAX30009_SYSTEM_CONFIG1_SHDN_MSK        BIT(1)
#define MAX30009_SYSTEM_CONFIG1_DISABLE_I2C_MSK BIT(6)
#define MAX30009_SYSTEM_CONFIG1_MASTER_MSK      BIT(7)

/* Pin Functional Configuration Masks */
#define MAX30009_PIN_FUNC_CONFIG_TRIG_ICFG_MSK BIT(0)
#define MAX30009_PIN_FUNC_CONFIG_INT_FCFG_MSK  GENMASK(3, 2)

/* Output Pin Configuration Masks */
#define MAX30009_OUTPUT_PIN_CONFIG_TRIG_OCFG_MSK GENMASK(1, 0)
#define MAX30009_OUTPUT_PIN_CONFIG_INT_OCFG_MSK  GENMASK(3, 2)

/* I2C BROADCAST ADDRESS */
#define MAX30009_I2C_BROADCAST_ADDR_MSK    GENMASK(7, 1)
#define MAX30009_I2C_BROADCAST_ADDR_EN_MSK BIT(0)

/* PLL Configuration Masks */
#define MAX30009_PLL_CONFIG1_MDIV_MSB_MSK GENMASK(7, 6)
#define MAX30009_PLL_CONFIG1_NDIV_MSK     BIT(5)
#define MAX30009_PLL_CONFIG1_KDIV_MSK     GENMASK(4, 1)
#define MAX30009_PLL_CONFIG1_PLL_EN_MSK   BIT(0)

#define MAX30009_PLL_CONFIG3_PLL_LOCK_WINDOW_MSK BIT(0)

#define MAX30009_PLL_CONFIG4_REF_CLK_SEL_MSK   BIT(6)
#define MAX30009_PLL_CONFIG4_CLK_FREQ_SEL_MSK  BIT(5)
#define MAX30009_PLL_CONFIG4_CLK_FINE_TUNE_MSK GENMASK(4, 0)

/* BioZ Configuration 1 Masks */
#define MAX30009_BIOZ_CONFIG1_BIOZ_DAC_OSR_MSK GENMASK(7, 6)
#define MAX30009_BIOZ_CONFIG1_BIOZ_ADC_OSR_MSK GENMASK(5, 3)
#define MAX30009_BIOZ_CONFIG1_BIOZ_BG_EN_MSK   BIT(2)
#define MAX30009_BIOZ_CONFIG1_BIOZ_Q_EN_MSK    BIT(1)
#define MAX30009_BIOZ_CONFIG1_BIOZ_I_EN_MSK    BIT(0)

/* BioZ Configuration 2 Masks */
#define MAX30009_BIOZ_CONFIG2_BIOZ_DHPF_MSK      GENMASK(7, 6)
#define MAX30009_BIOZ_CONFIG2_BIOZ_DLPF_MSK      GENMASK(5, 3)
#define MAX30009_BIOZ_CONFIG2_BIOZ_CMP_MSK       GENMASK(2, 1)
#define MAX30009_BIOZ_CONFIG2_EN_BIOZ_THRESH_MSK BIT(0)

/* BioZ Configuration 3 Masks */
#define MAX30009_BIOZ_CONFIG3_BIOZ_EXT_RES_MSK    BIT(7)
#define MAX30009_BIOZ_CONFIG3_BIOZ_LOFF_RAPID_MSK BIT(6)
#define MAX30009_BIOZ_CONFIG3_BIOZ_VDRV_MAG_MSK   GENMASK(5, 4)
#define MAX30009_BIOZ_CONFIG3_BIOZ_IDRV_RGE_MSK   GENMASK(3, 2)
#define MAX30009_BIOZ_CONFIG3_BIOZ_DRV_MODE_MSK   GENMASK(1, 0)

/* BioZ Configuration 4 Masks */
#define MAX30009_BIOZ_CONFIG4_BIOZ_FAST_MANUAL_MSK   BIT(1)
#define MAX30009_BIOZ_CONFIG4_BIOZ_FAST_START_EN_MSK BIT(0)

/* BioZ Configuration 5 Masks */
#define MAX30009_BIOZ_CONFIG5_BIOZ_AHPF_MSK     GENMASK(7, 4)
#define MAX30009_BIOZ_CONFIG5_BIOZ_INA_MODE_MSK BIT(3)
#define MAX30009_BIOZ_CONFIG5_BIOZ_DM_DIS_MSK   BIT(2)
#define MAX30009_BIOZ_CONFIG5_BIOZ_GAIN_MSK     GENMASK(1, 0)

/* BioZ Configuration 6 Masks */
#define MAX30009_BIOZ_CONFIG6_BIOZ_EXT_CAP_MSK    BIT(7)
#define MAX30009_BIOZ_CONFIG6_BIOZ_DC_RESTORE_MSK BIT(6)
#define MAX30009_BIOZ_CONFIG6_BIOZ_DRV_RESET_MSK  BIT(5)
#define MAX30009_BIOZ_CONFIG6_BIOZ_DAC_RESET_MSK  BIT(4)
#define MAX30009_BIOZ_CONFIG6_BIOZ_AMP_RGE_MSK    GENMASK(3, 2)
#define MAX30009_BIOZ_CONFIG6_BIOZ_AMP_BW_MSK     GENMASK(1, 0)

/* BioZ Configuration 7 Masks */
#define MAX30009_BIOZ_CONFIG7_BIOZ_STBY_ON_MSK     BIT(4)
#define MAX30009_BIOZ_CONFIG7_BIOZ_Q_CLK_PHASE_MSK BIT(3)
#define MAX30009_BIOZ_CONFIG7_BIOZ_I_CLK_PHASE_MSK BIT(2)
#define MAX30009_BIOZ_CONFIG7_BIOZ_INA_CHOP_EN_MSK BIT(1)
#define MAX30009_BIOZ_CONFIG7_BIOZ_CH_F_SEL_MSK    BIT(0)

/* BioZ Mux Configuration 1 Masks */
#define MAX30009_BIOZ_MUX_CONFIGURATION1_BMUX_RSEL_MSK        GENMASK(7, 6)
#define MAX30009_BIOZ_MUX_CONFIGURATION1_BMUX_BIST_MSK        BIT(5)
#define MAX30009_BIOZ_MUX_CONFIGURATION1_CONNECT_CAL_ONLY_MSK BIT(2)
#define MAX30009_BIOZ_MUX_CONFIGURATION1_MUX_EN_MSK           BIT(1)
#define MAX30009_BIOZ_MUX_CONFIGURATION1_CAL_EN_MSK           BIT(0)

/* BioZ Mux Configuration 2 Masks */
#define MAX30009_BIOZ_MUX_CONFIG2_BMUX_GSR_RSEL_MSK  GENMASK(7, 6)
#define MAX30009_BIOZ_MUX_CONFIG2_GSR_LOAD_EN_MSK    BIT(5)
#define MAX30009_BIOZ_MUX_CONFIG2_EN_EXT_IN_LOAD_MSK BIT(1)
#define MAX30009_BIOZ_MUX_CONFIG2_EN_INT_IN_LOAD_MSK BIT(0)

/* BioZ Mux Configuration 3 Masks */
#define MAX30009_BIOZ_MUX_CONFIG3_BIP_ASSIGN_MSK  GENMASK(7, 6)
#define MAX30009_BIOZ_MUX_CONFIG3_BIN_ASSIGN_MSK  GENMASK(5, 4)
#define MAX30009_BIOZ_MUX_CONFIG3_DRVP_ASSIGN_MSK GENMASK(3, 2)
#define MAX30009_BIOZ_MUX_CONFIG3_DRVN_ASSIGN_MSK GENMASK(1, 0)

/* BioZ DC Leads Configuration Masks */
#define MAX30009_DC_LEADS_CONFIG_EN_LON_DET_MSK  BIT(7)
#define MAX30009_DC_LEADS_CONFIG_EN_LOFF_DET_MSK BIT(6)
#define MAX30009_DC_LEADS_CONFIG_EN_EXT_LOFF_MSK BIT(5)
#define MAX30009_DC_LEADS_CONFIG_EN_DRV_OOR_MSK  BIT(4)
#define MAX30009_DC_LEADS_CONFIG_LOFF_IPOL_MSK   BIT(3)
#define MAX30009_DC_LEADS_CONFIG_LOFF_IMAG_MSK   GENMASK(2, 0)

/* Lead Detect Threshold Masks */
#define MAX30009_DC_LEAD_DETECT_THRESH_LOFF_THRESH_MSK GENMASK(3, 0)

/* Lead Bias Configuration 1 Masks */
#define MAX30009_LEAD_BIAS_CONFIG1_RBIAS_VALUE_MSK  GENMASK(3, 2)
#define MAX30009_LEAD_BIAS_CONFIG1_EN_RBIAS_BIP_MSK BIT(1)
#define MAX30009_LEAD_BIAS_CONFIG1_EN_RBIAS_BIN_MSK BIT(0)

/* Interrupt Enable 1 Masks */
#define MAX30009_INTERRUPT_ENABLE1_A_FULL_EN_MSK        BIT(7)
#define MAX30009_INTERRUPT_ENABLE1_FIFO_DATA_RDY_EN_MSK BIT(5)
#define MAX30009_INTERRUPT_ENABLE1_FREQ_UNLOCK_EN_MSK   BIT(4)
#define MAX30009_INTERRUPT_ENABLE1_FREQ_LOCK_EN_MSK     BIT(3)
#define MAX30009_INTERRUPT_ENABLE1_PHASE_UNLOCK_EN_MSK  BIT(2)
#define MAX30009_INTERRUPT_ENABLE1_PHASE_LOCK_EN_MSK    BIT(1)

/* Interrupt Enable 2 Masks */
#define MAX30009_INTERRUPT_ENABLE2_LON_EN_MSK        BIT(7)
#define MAX30009_INTERRUPT_ENABLE2_BIOZ_OVER_EN_MSK  BIT(6)
#define MAX30009_INTERRUPT_ENABLE2_BIOZ_UNDR_EN_MSK  BIT(5)
#define MAX30009_INTERRUPT_ENABLE2_DRV_OOR_EN_MSK    BIT(4)
#define MAX30009_INTERRUPT_ENABLE2_DC_LOFF_PH_EN_MSK BIT(3)
#define MAX30009_INTERRUPT_ENABLE2_DC_LOFF_PL_EN_MSK BIT(2)
#define MAX30009_INTERRUPT_ENABLE2_DC_LOFF_NH_EN_MSK BIT(1)
#define MAX30009_INTERRUPT_ENABLE2_DC_LOFF_NL_EN_MSK BIT(0)

/* Part ID */
#define MAX30009_PART_ID_VAL 0x42u

#define MAX30009_MDIV_MSB_MASK GENMASK(9, 8)
#define MAX30009_MDIV_LSB_MASK GENMASK(7, 0)

/* Reference clock options (Hz) selectable via clk-freq-sel */
#define MAX30009_REF_CLK_32K768_HZ 32768
#define MAX30009_REF_CLK_32K_HZ    32000

/* Valid PLL feedback multiplier (M) range per reference clock (datasheet MDIV) */
#define MAX30009_PLL_MULT_MIN_32K768 427
#define MAX30009_PLL_MULT_MAX_32K768 854
#define MAX30009_PLL_MULT_MIN_32K    438
#define MAX30009_PLL_MULT_MAX_32K    875

/* BioZ ADC clock: divider base (512 << NDIV) and valid output range (Hz) */
#define MAX30009_BIOZ_ADC_CLK_DIV_BASE 512
#define MAX30009_BIOZ_ADC_CLK_MIN_HZ   16000
#define MAX30009_BIOZ_ADC_CLK_MAX_HZ   36375

/* BioZ synthesis clock: valid output range (Hz) */
#define MAX30009_BIOZ_SYNTH_CLK_MIN_HZ 4096
#define MAX30009_BIOZ_SYNTH_CLK_MAX_HZ 28000000

/* BioZ oversampling ratios: natural value = base << register code */
#define MAX30009_BIOZ_ADC_OSR_BASE     8
#define MAX30009_BIOZ_ADC_OSR_MAX_CODE 7
#define MAX30009_BIOZ_DAC_OSR_BASE     32
#define MAX30009_BIOZ_DAC_OSR_MAX_CODE 3

#define MAX30009_MAX_FIFO_WATERMARK 256

#define MAX30009_FIFO_BYTES_PER_SAMPLE 3

union max30009_bus {
#if defined(MAX30009_BUS_I2C)
	struct i2c_dt_spec i2c;
#endif /* MAX30009_BUS_I2C */
#if defined(MAX30009_BUS_SPI)
	struct spi_dt_spec spi;
#endif /* MAX30009_BUS_SPI */
	uint8_t dummy;
};

/**
 * @brief Bus type a given MAX30009 instance is attached to
 *
 * Used to select the correct register-access framing at runtime so that a
 * build containing both SPI and I2C instances handles each one correctly.
 */
enum max30009_bus_type {
	MAX30009_BUS_TYPE_I2C,
	MAX30009_BUS_TYPE_SPI,
};

/**
 * @brief Function pointer to check if bus is ready
 *
 */
typedef int (*max30009_bus_is_ready_fn)(const union max30009_bus *bus);
/**
 * @brief Function pointer for single-byte register access
 *
 */
typedef int (*max30009_reg_access_fn)(const struct device *dev, bool read, uint8_t reg_addr,
				      uint8_t *data);

struct max30009_clk_config {
	uint8_t ref_clk_source;
	uint8_t clk_freq_sel;
	uint8_t clk_fine_tune;
	uint16_t pll_multiplier;
	uint8_t bioz_adc_clk_div: 1;
	uint8_t bioz_synth_clk_div: 4;
};

struct max30009_fifo_config {
	uint16_t fifo_watermark: 9;
	bool fifo_a_full_type;
	bool fifo_rollover;
};

struct max30009_bioz_config_1 {
	uint8_t bioz_dac_osr: 2;
	uint8_t bioz_adc_osr: 3;
	bool bioz_bg_en;
	bool bioz_q_en;
	bool bioz_i_en;
};

struct max30009_bioz_config_2 {
	uint8_t bioz_dhpf: 2;
	uint8_t bioz_dlpf: 3;
	uint8_t bioz_cmp: 2;
	bool en_bioz_thresh;
};

struct max30009_bioz_config_3 {
	bool bioz_ext_res;
	bool loff_rapid;
	uint8_t bioz_vdrv_mag: 2;
	uint8_t bioz_idrv_rge: 2;
	uint8_t bioz_drv_mode: 2;
};

struct max30009_bioz_config_4 {
	bool bioz_fast_manual;
	bool bioz_fast_start_en;
};

struct max30009_bioz_config_5 {
	uint8_t bioz_ahpf: 4;
	bool bioz_ina_mode;
	bool bioz_dm_dis;
	uint8_t bioz_gain: 2;
};

struct max30009_bioz_config_6 {
	bool bioz_ext_cap;
	bool bioz_dc_restore;
	uint8_t bioz_amp_rge: 2;
	uint8_t bioz_amp_bw: 2;
};

struct max30009_bioz_config_7 {
	bool bioz_stby_on;
	bool bioz_q_clk_phase;
	bool bioz_i_clk_phase;
	bool bioz_ina_chop_en;
	bool bioz_ch_f_sel;
};

struct max30009_bioz_mux_config_1 {
	uint8_t bmux_rsel: 2;
	bool bmux_bist_en;
	bool connect_cal_only;
	bool mux_en;
	bool cal_en;
};

struct max30009_bioz_mux_config_2 {
	uint8_t bmux_gsr_rsel: 2;
	bool gsr_load_en;
	bool en_ext_in_load;
	bool en_int_in_load;
};

struct max30009_bioz_mux_config_3 {
	uint8_t bip_assign: 2;
	uint8_t bin_assign: 2;
	uint8_t drvp_assign: 2;
	uint8_t drvn_assign: 2;
};

struct max30009_bioz_config {
	struct max30009_bioz_config_1 cfg_1;
	struct max30009_bioz_config_2 cfg_2;
	struct max30009_bioz_config_3 cfg_3;
	struct max30009_bioz_config_4 cfg_4;
	struct max30009_bioz_config_5 cfg_5;
	struct max30009_bioz_config_6 cfg_6;
	struct max30009_bioz_config_7 cfg_7;
};

struct max30009_bioz_mux_config {
	struct max30009_bioz_mux_config_1 mux_cfg_1;
	struct max30009_bioz_mux_config_2 mux_cfg_2;
	struct max30009_bioz_mux_config_3 mux_cfg_3;
};

struct max30009_dc_leads_config {
	bool en_lon_det;
	bool en_loff_det;
	bool en_ext_loff;
	bool en_drv_oor;
	uint8_t loff_ipol: 1;
	uint8_t loff_imag: 3;
	uint8_t loff_thresh: 4;
};

struct max30009_lead_bias_config_1 {
	uint8_t rbias_value: 2;
	bool en_rbias_bip;
	bool en_rbias_bin;
};

/**
 * @brief MAX30009 runtime data structure
 *
 */
#ifdef CONFIG_MAX30009_TRIGGER
/**
 * @brief MAX30009 per-event trigger handlers
 *
 * Groups the trigger handler/trigger pairs so they do not inflate the
 * top-level max30009_data structure.
 */
struct max30009_trigger_data {
	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trigger;
	sensor_trigger_handler_t dc_loff_nl_handler;
	const struct sensor_trigger *dc_loff_nl_trigger;
	sensor_trigger_handler_t dc_loff_nh_handler;
	const struct sensor_trigger *dc_loff_nh_trigger;
	sensor_trigger_handler_t dc_loff_pl_handler;
	const struct sensor_trigger *dc_loff_pl_trigger;
	sensor_trigger_handler_t dc_loff_ph_handler;
	const struct sensor_trigger *dc_loff_ph_trigger;
	sensor_trigger_handler_t drv_oor_handler;
	const struct sensor_trigger *drv_oor_trigger;
	sensor_trigger_handler_t bioz_undr_handler;
	const struct sensor_trigger *bioz_undr_trigger;
	sensor_trigger_handler_t bioz_over_handler;
	const struct sensor_trigger *bioz_over_trigger;
	sensor_trigger_handler_t lon_handler;
	const struct sensor_trigger *lon_trigger;
};
#endif /* CONFIG_MAX30009_TRIGGER */

struct max30009_data {
	int pll_clk;
	int bioz_adc_clk;
	int bioz_synth_clk;
#ifdef CONFIG_MAX30009_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct max30009_trigger_data trig;

#ifdef CONFIG_MAX30009_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_MAX30009_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_MAX30009_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif /* CONFIG_MAX30009_TRIGGER_OWN_THREAD || CONFIG_MAX30009_TRIGGER_GLOBAL_THREAD */
#endif /* CONFIG_MAX30009_TRIGGER */
#ifdef CONFIG_MAX30009_STREAM
	struct rtio_iodev_sqe *sqe;
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
	uint8_t status1;
	uint8_t fifo_counter[2];
	uint64_t timestamp;
	struct rtio *r_cb;
	uint8_t fifo_watermark_irq;
	uint8_t fifo_samples;
	uint16_t fifo_total_bytes;
#endif /* CONFIG_MAX30009_STREAM */
};

/**
 * @brief MAX30009 device configuration structure
 *
 */
struct max30009_dev_config {
	const union max30009_bus bus;
	enum max30009_bus_type bus_type;
	max30009_bus_is_ready_fn bus_is_ready;
	max30009_reg_access_fn reg_access;
	struct max30009_clk_config clk_cfg;
	struct max30009_fifo_config fifo_cfg;
	struct max30009_bioz_config bioz_cfg;
	struct max30009_bioz_mux_config bioz_mux_cfg;
	struct max30009_dc_leads_config dc_leads_cfg;
	struct max30009_lead_bias_config_1 lead_bias_cfg_1;
	uint8_t bioz_low_threshold;
	uint8_t bioz_high_threshold;
#ifdef CONFIG_MAX30009_TRIGGER
	struct gpio_dt_spec interrupt_gpio;
#endif /* CONFIG_MAX30009_TRIGGER */
};

struct max30009_fifo_data {
	uint8_t is_fifo: 1;
	uint64_t timestamp;
	uint8_t status1;
	uint16_t fifo_samples;
	uint16_t fifo_byte_count;
	uint8_t sample_set_size;
	uint32_t sample_period_ns;
};

/**
 * @brief MAX30009 single-byte register read function
 *
 * @param dev Device pointer
 * @param reg_addr Register address
 * @param data Pointer to the byte to read into
 * @return int 0 on success, negative error code on failure
 */
int max30009_reg_read(const struct device *dev, uint8_t reg_addr, uint8_t *data);

/**
 * @brief MAX30009 single-byte register write function
 *
 * @param dev Device pointer
 * @param reg_addr Register address
 * @param data Byte value to write
 * @return int 0 on success, negative error code on failure
 */
int max30009_reg_write(const struct device *dev, uint8_t reg_addr, uint8_t data);

/**
 * @brief MAX30009 register update function
 *
 * @param dev Device pointer
 * @param reg_addr Register address
 * @param mask Bit mask
 * @param value Value to set
 * @return
 */
int max30009_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t value);

#ifdef CONFIG_MAX30009_TRIGGER
/**
 * @brief MAX30009 interrupt initialization function
 *
 * @param dev Device pointer
 * @return int 0 on success, negative error code on failure
 */
int max30009_init_interrupt(const struct device *dev);

/**
 * @brief MAX30009 trigger set function
 *
 * @param dev Device pointer
 * @param trig Trigger configuration
 * @param handler Trigger handler function
 * @return int 0 on success, negative error code on failure
 */
int max30009_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);
#endif /* CONFIG_MAX30009_TRIGGER */

#ifdef CONFIG_MAX30009_STREAM
void max30009_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
void max30009_stream_irq_handler(const struct device *dev);
void max30009_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
int max30009_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
#endif /* CONFIG_MAX30009_STREAM */

#endif /* ZEPHYR_DRIVERS_SENSOR_ADI_MAX30009_MAX30009_H_ */
