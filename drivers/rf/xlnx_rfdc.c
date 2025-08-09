/* SPDX-License-Identifier: MIT */
/*
 * Copyright (c) 2020 Xilinx, Inc.
 * Copyright (c) 2025 YWL, Tron Future Tech.
 */
#define DT_DRV_COMPAT xlnx_rfdc
#include <zephyr/sys/sys_io.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/rf/rf.h>
#include "xlnx_rfdc.h"
#include "xparameters.h"
LOG_MODULE_REGISTER(xlnx_rfdc, CONFIG_RF_LOG_LEVEL);

#define XRFDC_MAX_DLY_INIT 0U
#define XRFDC_MIN_DLY_INIT 0xFFU
#define XRFDC_DLY_UNIT 2U
#define XRFDC_MAX_DISTRS 8U

static uint32_t  pll_tuning_matrix[8][4][2] = {
	{ { 0x7F8A, 0x3FFF }, { 0x7F9C, 0x3FFF }, { 0x7FE2, 0x3FFF } },
	{ { 0x7FE9, 0xFFFF }, { 0x7F8E, 0xFFFF }, { 0x7F9C, 0xFFFF } },
	{ { 0x7F95, 0xFFFF }, { 0x7F8E, 0xFFFF }, { 0x7F9A, 0xFFFF }, { 0x7F8C, 0xFFFF } },
	{ { 0x7F95, 0x3FFF }, { 0x7FEE, 0x3FFF }, { 0x7F9A, 0xFFFF }, { 0x7F9C, 0xFFFF } },
	{ { 0x7F95, 0x3FFF }, { 0x7FEE, 0x3FFF }, { 0x7F9A, 0xFFFF }, { 0x7F9C, 0xFFFF } },
	{ { 0x7F95, 0xFFFF }, { 0x7F8E, 0xFFFF }, { 0x7FEA, 0xFFFF }, { 0x7F9C, 0xFFFF } },
	{ { 0x7FE9, 0xFFFF }, { 0x7F8E, 0xFFFF }, { 0x7F9A, 0xFFFF }, { 0x7F9C, 0xFFFF } },
	{ { 0x7FEC, 0xFFFF }, { 0x7FEE, 0x3FFF }, { 0x7F9C, 0xFFFF } }
};

struct xlnx_rfdc_tile_clock_settings {
    uint32_t pll_en;
    double ref_clk_freq;
    double sample_rate;
    uint8_t source_type;
    uint8_t source_tile;
    uint8_t div_factor;
    uint8_t dist_clock;
    uint8_t delay;
};

struct threshold_settings {
    uint32_t update_threshold;
    uint32_t threshold_mode[2];
    uint32_t threashold_avg_val[2];
    uint32_t threshold_under_val[2];
    uint32_t threashold_over_val[2];
};

struct xlnx_rfdc_qmc_settings {
    uint32_t en_phase;
    uint32_t en_gain;
    double gain_correct_factor;
    double phase_correct_factor;
    int32_t offset_correct_factor;
    uint32_t event_src;
};

struct xlnx_rfdc_coarse_delay_settings {
    uint32_t coarse_delay;
    uint32_t event_src;
};

struct xlnx_rfdc_pll_settings {
    uint32_t en;
    double ref_clk_freq;
    double sample_rate;
    uint32_t ref_clk_divider;
    uint32_t feedback_divider;
    uint32_t output_divider;
    uint32_t fraction_mode;
    uint64_t fraction_data;
    uint32_t fraction_width;
};

struct xlnx_rfdc_threshold_settings {
    uint32_t update_threashold;
    uint32_t threashold_mode[2];
    uint32_t threashold_avg_val[2];
    uint32_t threashold_under_val[2];
    uint32_t threasholde_over_val[2];
};

struct xlnx_rfdc_dac_analog_datapath {
    uint32_t enable;
    uint32_t mixer_mode;
    double terminated_volt;
    double output_current;
    uint32_t inverse_sinc_filter_en;
    uint32_t decoder_mode;
    uint32_t nyquist_zone;
    uint8_t analog_path_en;
    uint8_t analog_path_available;
    struct xlnx_rfdc_qmc_settings qmc_settings;
    struct xlnx_rfdc_coarse_delay_settings coarse_delay_settings;
};

struct xlnx_rfdc_adc_analog_datapath {
    uint32_t en;
    struct xlnx_rfdc_qmc_settings qmc_settings;
    struct xlnx_rfdc_coarse_delay_settings coarse_delay_settings;
    struct xlnx_rfdc_threshold_settings threshold_settings;
    uint32_t nuquist_zone;
    uint8_t cal_mode;
    uint8_t analog_path_en;
    uint8_t analog_path_available;
};

struct xlnx_rfdc_mixer_settings {
    double freq;
    double phase_offset;
    uint32_t event_src;
    uint32_t coarse_mix_freq;
    uint32_t mixer_mode;
    uint8_t fine_mixer_scale;
    uint8_t mixer_type;
};

struct xlnx_rfdc_dac_digital_datapath {
    uint32_t mixer_input_type;
    uint32_t data_width;
    int connected_i_data;
    int connected_q_data;
    uint32_t interpolation_factor;
    uint8_t digital_path_en;
    uint8_t digital_path_available;
    struct xlnx_rfdc_mixer_settings mixer_settings;
};

struct xlnx_rfdc_adc_digital_datapath {
    uint32_t mixer_input_type;
    uint32_t data_width;
    uint32_t decimation_factor;
    int connected_i_data;
    int connected_q_data;
    uint8_t digital_path_en;
    uint8_t digital_path_available;
    struct xlnx_rfdc_mixer_settings mixer_settings;
};

struct xlnx_rfdc_dac_tile {
    uint32_t tile_baseaddr;
    uint32_t num_dac_blocks;
    struct xlnx_rfdc_pll_settings pll_settings;
    uint8_t multiband_config;
    struct xlnx_rfdc_dac_analog_datapath dac_analog_datapath[4];
    struct xlnx_rfdc_dac_digital_datapath dac_digital_datapath[4];
};

struct xlnx_rfdc_adc_tile  {
    uint32_t tile_baseaddr;
    uint32_t num_adc_block;
    struct xlnx_rfdc_pll_settings pll_settings;
    uint8_t multiband_config;
    struct xlnx_rfdc_adc_analog_datapath adc_analog_datapath[4];
    struct xlnx_rfdc_adc_digital_datapath adc_digital_datapath[4];
};

struct xlnx_rfdc_dist_info {
    uint8_t max_delay;
    uint8_t min_delay;
    uint8_t is_delay_balance;
    uint8_t source;
    uint8_t upper_bound;
    uint8_t lower_bound;
    struct xlnx_rfdc_tile_clock_settings clk_settings[2][4];
};

struct xlnx_rfdc_dac_analog_datapath_config {
    uint32_t block_available;
    uint32_t inv_sync_en;
    uint32_t mix_mode;
    uint32_t decoder_mode;
};

struct xlnx_rfdc_dac_digital_datapath_config {
    uint32_t mixer_input_data_type;
    uint32_t data_width;
    uint32_t interpolation_mode;
    uint32_t fifo_en;
    uint32_t adder_en;
    uint32_t mixer_type;
};

struct xlnx_rfdc_adc_analog_datapath_config {
    uint32_t block_available;
    uint32_t mix_mode;
};

struct xlnx_rfdc_adc_digital_datapath_config {
    uint32_t mixer_input_data_type;
    uint32_t data_width;
    uint32_t decimation_mode;
    uint32_t fifo_en;
    uint32_t mixer_type;
};

struct xlnx_rfdc_dac_tile_config {
    uint32_t en;
    uint32_t pll_en;
	double sample_rate;
	double ref_clk_freq;
	double fab_clk_freq;
	uint32_t feedback_div;
	uint32_t output_div;
	uint32_t ref_clk_div;
    uint32_t multiband_config;
    double max_sample_rate;
    uint32_t num_slices;
    uint32_t link_coupling;
    struct xlnx_rfdc_dac_analog_datapath_config dac_analog_config[4];
    struct xlnx_rfdc_dac_digital_datapath_config dac_digital_config[4];
};

struct xlnx_rfdc_adc_tile_config {
    uint32_t en;
    uint32_t pll_en;
    double sample_rate;
    double ref_clk_freq;
    double fab_clk_freq;
    uint32_t feedback_div;
    uint32_t output_div;
    uint32_t ref_clk_div;
    uint32_t multiband_config;
    double max_sample_rate;
    uint32_t num_slices;
    struct xlnx_rfdc_adc_analog_datapath_config adc_analog_config[4];
    struct xlnx_rfdc_adc_digital_datapath_config adc_digital_config[4];
};

struct xlnx_rfdc_dev_config {
    DEVICE_MMIO_ROM;
    uint32_t device_id;
    uint32_t adc_type;
    uint32_t master_adc_tile;
    uint32_t master_dac_tile;
    uint32_t adc_sys_ref_source;
    uint32_t dac_sys_ref_source;
    uint32_t ip_type;
    uint32_t si_revision;
    struct xlnx_rfdc_dac_tile_config dac_tile_config[4];
    struct xlnx_rfdc_adc_tile_config adc_tile_config[4];
    /* --------------------------------- */
    uint32_t adc4gsps;
	struct device *en_gpio;
};

struct xlnx_rfdc_dev_data {
    DEVICE_MMIO_RAM;
    uint32_t source_type;
    uint32_t source_tile_id;
    uint32_t edge_tile_ids[2];
    uint32_t edge_types[2];
    double dist_ref_clk_freq;
    uint32_t dist_clock;
    double sample_rates[2][4];
    uint32_t shutdown_mode;
    struct xlnx_rfdc_dist_info info;
    uint32_t is_ready;
    struct xlnx_rfdc_dac_tile dac_tile[4];
    struct xlnx_rfdc_adc_tile adc_tile[4];
    uint8_t update_mixer_scale;
};
struct xlnx_rfdc_dev_datas {
    struct xlnx_rfdc_dev_data distrubutions[8];
};

static uint32_t  xlnx_rfdc_set_dac_vop(const struct device *dev, uint32_t tile_id, uint32_t block_id, uint32_t ua_curr);

static uint8_t xlnx_rfdc_get_tile_layout(const struct device *dev) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    if (config->adc_tile_config[XRFDC_TILE_ID3].num_slices == 0) {
        return XRFDC_3ADC_2DAC_TILES;
    } else {
        return XRFDC_4ADC_4DAC_TILES;
    }
}

static uint32_t xlnx_rfdc_chk_tile_enable(const struct device *dev, uint32_t type, uint32_t tile_id) {
    uint32_t status, tile_mask, tile_en_reg;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    if ((type != XRFDC_ADC_TILE) && (type != XRFDC_DAC_TILE)) {
        status = XRFDC_FAILURE;
        goto return_path;
    }
    if (tile_id > XRFDC_TILE_ID_MAX) {
        status = XRFDC_FAILURE;
        goto return_path;
    }
    tile_en_reg = sys_read32(reg_base + XRFDC_TILES_ENABLED_OFFSET);
    tile_mask = XRFDC_ENABLED << tile_id;
    if (type == XRFDC_DAC_TILE) {
		tile_mask <<= XRFDC_DAC_TILES_ENABLED_SHIFT;
	}
    // printf("tile_en_reg 0x%x, tile_mask: 0x%x", tile_en_reg, tile_mask);
	if ((tile_en_reg & tile_mask) == 0U) {
		status = XRFDC_FAILURE;
	} else {
		status = XRFDC_SUCCESS;
	}
return_path:
	return status;
}

static inline void xlnx_rfdc_clrset_reg(const struct device *dev, uint32_t base_addr, uint32_t reg_addr, uint16_t mask, uint16_t data) {
    uint16_t val;
    val = sys_read16(base_addr + reg_addr);
    val = (val & ~mask) | (data & mask);
    sys_write16(val, base_addr + reg_addr);
}

static uint8_t xlnx_rfdc_type_tile2dist_tile(const struct device *dev, uint32_t type, uint32_t tile_id) {
    uint8_t dac_edge_tile, tile_layout, dist_tile;
    tile_layout = xlnx_rfdc_get_tile_layout(dev);
    dac_edge_tile = (tile_layout == XRFDC_3ADC_2DAC_TILES) ? XRFDC_CLK_DST_TILE_227:XRFDC_CLK_DST_TILE_228;
    if (type == XRFDC_ADC_TILE) {
        dist_tile = XRFDC_CLK_DST_TILE_224 - tile_id;
    } else {
        dist_tile = dac_edge_tile -tile_id;
    }
    return dist_tile;
}

static void xlnx_rfdc_dist_tile2type_tile(const struct device *dev, uint32_t dist_tile, uint32_t *type, uint32_t *tile_id) {
    uint8_t dac_edge_tile, tile_layout;
    tile_layout = xlnx_rfdc_get_tile_layout(dev);
    dac_edge_tile = (tile_layout == XRFDC_3ADC_2DAC_TILES) ? XRFDC_CLK_DST_TILE_227 : XRFDC_CLK_DST_TILE_228;
	if (dist_tile > dac_edge_tile) {
        *type = XRFDC_ADC_TILE;
        *tile_id = XRFDC_CLK_DST_TILE_224 - dist_tile;
    } else {
        *type = XRFDC_DAC_TILE;
        *tile_id = dac_edge_tile - dist_tile;
    }
}

static uint32_t xlnx_rfdc_is_high_speed_adc(const struct device *dev, uint32_t tile) {
     const struct xlnx_rfdc_dev_config *config = dev->config;
    if (tile > XRFDC_TILE_ID_MAX) {
		LOG_ERR("Invalid converter tile number in %s", __func__);
		return 0;
	}

	if (config->adc_tile_config[tile].num_slices == 0) {
		return config->adc4gsps;
	} else {
		return (config->adc_tile_config[tile].num_slices != XRFDC_NUM_SLICES_LSADC);
	}
}

static void xlnx_rfdc_set_connected_iq_data(const struct device *dev, uint32_t type, uint32_t tile_id, uint32_t block_id, int connected_i_data, int connected_q_data) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    if (type > XRFDC_DAC_TILE) {
		LOG_ERR("Invalid converter type in %s", __func__);
		return;
	}

	if (tile_id > XRFDC_TILE_ID_MAX) {
		LOG_ERR("Invalid converter tile number in %s", __func__);
		return;
	}

	if (block_id > XRFDC_BLOCK_ID_MAX) {
		LOG_ERR("Invalid converter block number in %s", __func__);
		return;
	}

	if (type == XRFDC_ADC_TILE) {
		data->adc_tile[tile_id].adc_digital_datapath[block_id].connected_i_data = connected_i_data;
		data->adc_tile[tile_id].adc_digital_datapath[block_id].connected_q_data = connected_q_data;
	} else {
		data->dac_tile[tile_id].dac_digital_datapath[block_id].connected_i_data = connected_i_data;
		data->dac_tile[tile_id].dac_digital_datapath[block_id].connected_q_data = connected_q_data;
	}
}

static void xlnx_rfdc_dac_mb_config_init(const struct device *dev, uint32_t tile_id, uint32_t block_id) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    if (config->dac_tile_config[tile_id].dac_analog_config[block_id].mix_mode ==
	    XRFDC_MIXER_MODE_C2C) {
		/* Mixer Mode is C2C */
		switch (data->dac_tile[tile_id].multiband_config) {
		case XRFDC_MB_MODE_4X:
			if (config->dac_tile_config[tile_id].num_slices == XRFDC_DUAL_TILE) {
				xlnx_rfdc_set_connected_iq_data(dev, XRFDC_DAC_TILE, tile_id, block_id, XRFDC_BLK_ID0,
							 XRFDC_BLK_ID2);
			} else {
				xlnx_rfdc_set_connected_iq_data(dev, XRFDC_DAC_TILE, tile_id, block_id, XRFDC_BLK_ID0,
							 XRFDC_BLK_ID1);
			}
			break;
		case XRFDC_MB_MODE_2X_BLK01_BLK23_ALT:
			if (block_id < XRFDC_BLK_ID2) {
				xlnx_rfdc_set_connected_iq_data(dev, XRFDC_DAC_TILE, tile_id, block_id, XRFDC_BLK_ID0,
							 XRFDC_BLK_ID2);
			} else {
				xlnx_rfdc_set_connected_iq_data(dev, XRFDC_DAC_TILE, tile_id, block_id,
							 XRFDC_BLK_ID_NONE, XRFDC_BLK_ID_NONE);
			}

			break;
		case XRFDC_MB_MODE_2X_BLK01_BLK23:
		case XRFDC_MB_MODE_2X_BLK01:
		case XRFDC_MB_MODE_2X_BLK23:
			if ((block_id == XRFDC_BLK_ID0) || (block_id == XRFDC_BLK_ID1)) {
				xlnx_rfdc_set_connected_iq_data(dev, XRFDC_DAC_TILE, tile_id, block_id, XRFDC_BLK_ID0,
							 XRFDC_BLK_ID1);
			} else {
				xlnx_rfdc_set_connected_iq_data(dev, XRFDC_DAC_TILE, tile_id, block_id, XRFDC_BLK_ID2,
							 XRFDC_BLK_ID3);
			}
			break;
		default:
			xlnx_rfdc_set_connected_iq_data(dev, XRFDC_DAC_TILE, tile_id, block_id, block_id,
						 block_id + 1U);
			break;
		}
	} else if (config->dac_tile_config[tile_id].dac_analog_config[block_id].mix_mode == 0x0) {
		/* Mixer Mode is C2R */
		switch (data->dac_tile[tile_id].multiband_config) {
		case XRFDC_MB_MODE_4X:
			xlnx_rfdc_set_connected_iq_data(dev, XRFDC_DAC_TILE, tile_id, block_id, XRFDC_BLK_ID0,
						 XRFDC_BLK_ID_NONE);
			break;
		case XRFDC_MB_MODE_2X_BLK01_BLK23:
		case XRFDC_MB_MODE_2X_BLK01:
		case XRFDC_MB_MODE_2X_BLK23:
			if ((block_id == XRFDC_BLK_ID0) || (block_id == XRFDC_BLK_ID1)) {
				xlnx_rfdc_set_connected_iq_data(dev, XRFDC_DAC_TILE, tile_id, block_id, XRFDC_BLK_ID0,
							 XRFDC_BLK_ID_NONE);
			} else {
				xlnx_rfdc_set_connected_iq_data(dev, XRFDC_DAC_TILE, tile_id, block_id, XRFDC_BLK_ID2,
							 XRFDC_BLK_ID_NONE);
			}
			break;
		default:
			xlnx_rfdc_set_connected_iq_data(dev, XRFDC_DAC_TILE, tile_id, block_id, block_id,
						 XRFDC_BLK_ID_NONE);
			break;
		}
	} else {
		/* Mixer Mode is BYPASS */
		xlnx_rfdc_set_connected_iq_data(dev, XRFDC_DAC_TILE, tile_id, block_id, block_id, XRFDC_BLK_ID_NONE);
    }
}

static void xlnx_rfdc_adc_mb_config_init(const struct device *dev, uint32_t tile_id, uint32_t block_id) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    if (config->adc_tile_config[tile_id].adc_analog_config[block_id].mix_mode ==
	    XRFDC_MIXER_MODE_C2C) {
		/* Mixer mode is C2C */
		switch (data->adc_tile[tile_id].multiband_config) {
		case XRFDC_MB_MODE_4X:
			xlnx_rfdc_set_connected_iq_data(dev, XRFDC_ADC_TILE, tile_id, block_id, XRFDC_BLK_ID0,
						 XRFDC_BLK_ID1);
			break;
		case XRFDC_MB_MODE_2X_BLK01_BLK23:
		case XRFDC_MB_MODE_2X_BLK01:
		case XRFDC_MB_MODE_2X_BLK23:
			if ((block_id == XRFDC_BLK_ID0) || (block_id == XRFDC_BLK_ID1)) {
				xlnx_rfdc_set_connected_iq_data(dev, XRFDC_ADC_TILE, tile_id, block_id, XRFDC_BLK_ID0,
							 XRFDC_BLK_ID1);
			} else {
				xlnx_rfdc_set_connected_iq_data(dev, XRFDC_ADC_TILE, tile_id, block_id, XRFDC_BLK_ID2,
							 XRFDC_BLK_ID3);
			}
			break;
		default:
			xlnx_rfdc_set_connected_iq_data(dev, XRFDC_ADC_TILE, tile_id, block_id, block_id,
						 block_id + 1U);
			break;
		}
	} else if (config->adc_tile_config[tile_id].adc_analog_config[block_id].mix_mode == 0x0) {
		/* Mixer mode is R2C */
		switch (data->adc_tile[tile_id].multiband_config) {
		case XRFDC_MB_MODE_4X:
			xlnx_rfdc_set_connected_iq_data(dev, XRFDC_ADC_TILE, tile_id, block_id, XRFDC_BLK_ID0,
						 XRFDC_BLK_ID_NONE);
			break;
		case XRFDC_MB_MODE_2X_BLK01_BLK23:
		case XRFDC_MB_MODE_2X_BLK01:
		case XRFDC_MB_MODE_2X_BLK23:
			if ((block_id == XRFDC_BLK_ID0) || (block_id == XRFDC_BLK_ID1)) {
				xlnx_rfdc_set_connected_iq_data(dev, XRFDC_ADC_TILE, tile_id, block_id, XRFDC_BLK_ID0,
							 XRFDC_BLK_ID_NONE);
			} else {
				xlnx_rfdc_set_connected_iq_data(dev, XRFDC_ADC_TILE, tile_id, block_id, XRFDC_BLK_ID2,
							 XRFDC_BLK_ID_NONE);
			}
			break;
		default:
			xlnx_rfdc_set_connected_iq_data(dev, XRFDC_ADC_TILE, tile_id, block_id, block_id,
						 XRFDC_BLK_ID_NONE);
			break;
		}
	} else {
		/* Mixer mode is BYPASS */
		xlnx_rfdc_set_connected_iq_data(dev, XRFDC_ADC_TILE, tile_id, block_id, block_id, XRFDC_BLK_ID_NONE);
	}
}

static uint32_t xlnx_rfdc_is_dac_digital_path_en(const struct device *dev, uint32_t tile_id, uint32_t block_id) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    uint32_t is_digi_path_avail, digi_path_shift, digi_path_en_reg;
    digi_path_shift = block_id + XRFDC_DIGITAL_PATH_ENABLED_SHIFT + (XRFDC_PATH_ENABLED_TILE_SHIFT * tile_id);
	digi_path_en_reg = sys_read32(reg_base + XRFDC_IP_BASE + XRFDC_DAC_PATHS_ENABLED_OFFSET);
	digi_path_en_reg &= (XRFDC_ENABLED << digi_path_shift);
	is_digi_path_avail = digi_path_en_reg >> digi_path_shift;
	return is_digi_path_avail;
}

static uint32_t xlnx_rfdc_is_adc_digital_path_en(const struct device *dev, uint32_t tile_id, uint32_t block_id) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    uint32_t is_digi_path_avail, digi_path_shift, digi_path_en_reg;
    if (xlnx_rfdc_is_high_speed_adc(dev, tile_id) == XRFDC_ENABLED) {
		if ((block_id == 2U) || (block_id == 3U)) {
			is_digi_path_avail = 0;
			goto return_path;
		}
		if (block_id == 1U) {
			block_id = 2U;
		}
	}

	digi_path_shift = block_id + XRFDC_DIGITAL_PATH_ENABLED_SHIFT + (XRFDC_PATH_ENABLED_TILE_SHIFT * tile_id);
	digi_path_en_reg = sys_read32(reg_base + XRFDC_IP_BASE + XRFDC_ADC_PATHS_ENABLED_OFFSET);
	digi_path_en_reg &= (XRFDC_ENABLED << digi_path_shift);
	is_digi_path_avail = digi_path_en_reg >> digi_path_shift;

return_path:
	return is_digi_path_avail;
}

static uint32_t xlnx_rfdc_is_dac_block_en(const struct device *dev, uint32_t tile_id, uint32_t block_id) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    uint32_t is_block_avalible, block_shift, block_en_reg;
    if (tile_id > XRFDC_TILE_ID_MAX) {
		LOG_ERR("Invalid converter tile number in %s", __func__);
		return 0U;
	}

	if (block_id > XRFDC_BLOCK_ID_MAX) {
		LOG_ERR("Invalid converter block number in %s", __func__);
		return 0U;
	}

	block_shift = block_id + (XRFDC_PATH_ENABLED_TILE_SHIFT * tile_id);
	block_en_reg = sys_read32(reg_base + XRFDC_IP_BASE + XRFDC_DAC_PATHS_ENABLED_OFFSET);
	block_en_reg &= (XRFDC_ENABLED << block_shift);
	is_block_avalible = block_en_reg >> block_shift;
	return is_block_avalible;
}

static uint32_t xlnx_rfdc_is_adc_block_en(const struct device *dev, uint32_t tile_id, uint32_t block_id) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    uint32_t is_block_avalible, block_shift, block_en_reg;
    if (tile_id > XRFDC_TILE_ID_MAX) {
		LOG_ERR("Invalid converter tile number in %s", __func__);
		return 0U;
	}

	if (block_id > XRFDC_BLOCK_ID_MAX) {
		LOG_ERR("Invalid converter block number in %s", __func__);
		return 0U;
	}

	if (xlnx_rfdc_is_high_speed_adc(dev, tile_id) == XRFDC_ENABLED) {
		if ((block_id == 2U) || (block_id == 3U)) {
			is_block_avalible = 0;
			goto return_path;
		}
		if (block_id == 1U) {
			block_id = 2U;
		}
	}

	block_shift = block_id + (XRFDC_PATH_ENABLED_TILE_SHIFT * tile_id);
	block_en_reg = sys_read32(reg_base + XRFDC_IP_BASE + XRFDC_ADC_PATHS_ENABLED_OFFSET);
	block_en_reg &= (XRFDC_ENABLED << block_shift);
	is_block_avalible = block_en_reg >> block_shift;

return_path:
	return is_block_avalible;
}

static void xlnx_rfdc_update_pll_struct(const struct device *dev, uint32_t type, uint32_t tile_id) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    if (type == XRFDC_ADC_TILE) {
		data->adc_tile[tile_id].pll_settings.sample_rate =
			config->adc_tile_config[tile_id].sample_rate;
		data->adc_tile[tile_id].pll_settings.ref_clk_freq =
			config->adc_tile_config[tile_id].ref_clk_freq;
		data->adc_tile[tile_id].pll_settings.en =
			config->adc_tile_config[tile_id].pll_en;
		data->adc_tile[tile_id].pll_settings.feedback_divider =
			config->adc_tile_config[tile_id].feedback_div;
		data->adc_tile[tile_id].pll_settings.output_divider =
			config->adc_tile_config[tile_id].output_div;
		data->adc_tile[tile_id].pll_settings.ref_clk_divider =
			config->adc_tile_config[tile_id].ref_clk_div;
	} else {
		data->dac_tile[tile_id].pll_settings.sample_rate =
			config->dac_tile_config[tile_id].sample_rate;
		data->dac_tile[tile_id].pll_settings.ref_clk_freq =
			config->dac_tile_config[tile_id].ref_clk_freq;
		data->dac_tile[tile_id].pll_settings.en =
			config->dac_tile_config[tile_id].pll_en;
		data->dac_tile[tile_id].pll_settings.feedback_divider =
			config->dac_tile_config[tile_id].feedback_div;
		data->dac_tile[tile_id].pll_settings.output_divider =
			config->dac_tile_config[tile_id].output_div;
		data->dac_tile[tile_id].pll_settings.ref_clk_divider =
			config->dac_tile_config[tile_id].ref_clk_div;
	}
}

static void xlnx_rfdc_dac_init(const struct device *dev) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uint32_t tile_id, block_id;
    uint8_t mixer_type;
    for (tile_id = XRFDC_TILE_ID0; tile_id < XRFDC_TILE_ID4; tile_id++) {
		data->dac_tile[tile_id].num_dac_blocks = 0U;
		for (block_id = XRFDC_BLK_ID0; block_id < XRFDC_BLK_ID4; block_id++) {
			if (xlnx_rfdc_is_dac_block_en(dev, tile_id, block_id) != 0U) {
				data->dac_tile[tile_id].num_dac_blocks += 1U;
				data->dac_tile[tile_id].dac_analog_datapath[block_id].analog_path_en =
					XRFDC_ANALOGPATH_ENABLE;
			}
			/* Initialize Data type */
			if (config->dac_tile_config[tile_id].dac_analog_config[block_id].mix_mode ==
			    XRFDC_MIXER_MODE_BYPASS) {
				data->dac_tile[tile_id].dac_digital_datapath[block_id].mixer_input_type =
					config->dac_tile_config[tile_id]
						.dac_digital_config[block_id]
						.mixer_input_data_type;
			} else {
				data->dac_tile[tile_id].dac_digital_datapath[block_id].mixer_input_type =
					XRFDC_DATA_TYPE_IQ;
			}
			/* Initialize mixer_type */
			mixer_type = config->dac_tile_config[tile_id]
					    .dac_digital_config[block_id]
					    .mixer_type;
			data->dac_tile[tile_id].dac_digital_datapath[block_id].mixer_settings.mixer_type =
				mixer_type;

			data->dac_tile[tile_id].dac_digital_datapath[block_id].connected_i_data =
				XRFDC_BLK_ID_NONE;
			data->dac_tile[tile_id].dac_digital_datapath[block_id].connected_q_data =
				XRFDC_BLK_ID_NONE;
			data->dac_tile[tile_id].multiband_config =
				config->dac_tile_config[tile_id].multiband_config;
			if (xlnx_rfdc_is_dac_digital_path_en(dev, tile_id, block_id) != 0U) {
				data->dac_tile[tile_id].dac_digital_datapath[block_id].digital_path_available =
					XRFDC_DIGITALPATH_ENABLE;
				data->dac_tile[tile_id].dac_digital_datapath[block_id].digital_path_en =
					XRFDC_DIGITALPATH_ENABLE;
				/* Initialize ConnectedI/QData, MB Config */
				xlnx_rfdc_dac_mb_config_init(dev, tile_id, block_id);
			}
            // xlnx_rfdc_set_dac_vop(dev, tile_id, block_id, 32000U);
		}
		/* Initialize PLL Structure */
		xlnx_rfdc_update_pll_struct(dev, XRFDC_DAC_TILE, tile_id);
	}
}

static void xlnx_rfdc_adc_init(const struct device *dev) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uint32_t tile_id, block_id;
    uint8_t mixer_type;
    for (tile_id = XRFDC_TILE_ID0; tile_id < XRFDC_TILE_ID4; tile_id++) {
		data->adc_tile[tile_id].num_adc_block = 0U;
		for (block_id = XRFDC_BLK_ID0; block_id < XRFDC_BLK_ID4; block_id++) {
			if (xlnx_rfdc_is_adc_block_en(dev, tile_id, block_id) != 0U) {
				data->adc_tile[tile_id].num_adc_block += 1U;
				data->adc_tile[tile_id].adc_analog_datapath[block_id].analog_path_en =
					XRFDC_ANALOGPATH_ENABLE;
			}
			/* Initialize Data type */
			if (config->adc_tile_config[tile_id].adc_analog_config[block_id].mix_mode ==
			    XRFDC_MIXER_MODE_BYPASS) {
				data->adc_tile[tile_id].adc_digital_datapath[block_id].mixer_input_type =
					config->adc_tile_config[tile_id]
						.adc_digital_config[block_id]
						.mixer_input_data_type;
			} else {
				data->adc_tile[tile_id].adc_digital_datapath[block_id].mixer_input_type =
					config->adc_tile_config[tile_id]
						.adc_analog_config[block_id]
						.mix_mode;
			}
			/* Initialize mixer_type */
			mixer_type = config->adc_tile_config[tile_id]
					    .adc_digital_config[block_id]
					    .mixer_type;
			data->adc_tile[tile_id].adc_digital_datapath[block_id].mixer_settings.mixer_type =
				mixer_type;

			data->adc_tile[tile_id].adc_digital_datapath[block_id].connected_i_data =
				XRFDC_BLK_ID_NONE;
			data->adc_tile[tile_id].adc_digital_datapath[block_id].connected_q_data =
				XRFDC_BLK_ID_NONE;
			data->adc_tile[tile_id].multiband_config =
				config->adc_tile_config[tile_id].multiband_config;
			if (xlnx_rfdc_is_adc_digital_path_en(dev, tile_id, block_id) != 0U) {
				data->adc_tile[tile_id].adc_digital_datapath[block_id].digital_path_available =
					XRFDC_DIGITALPATH_ENABLE;
				data->adc_tile[tile_id].adc_digital_datapath[block_id].digital_path_en =
					XRFDC_DIGITALPATH_ENABLE;
				/* Initialize ConnectedI/QData, MB Config */
				xlnx_rfdc_adc_mb_config_init(dev, tile_id, block_id);
			}
		}

		/* Initialize PLL Structure */
		xlnx_rfdc_update_pll_struct(dev, XRFDC_ADC_TILE, tile_id);
	}
}

static uint32_t xlnx_rfdc_get_clock_source(const struct device *dev, uint32_t type, uint32_t tile_id, uint32_t *clock_source) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    uint32_t base_addr, status, pll_en_reg;
    status = xlnx_rfdc_chk_tile_enable(dev, type, tile_id);
	if (status != XRFDC_SUCCESS) {
		LOG_ERR("Requested tile (%s %u) not available in %s",
			  (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
		goto return_path;
	}

	base_addr = XRFDC_DRP_BASE(type, tile_id) + XRFDC_HSCOM_ADDR;

	if (config->ip_type < XRFDC_GEN3) {
        *clock_source = sys_read16(reg_base + base_addr + XRFDC_CLK_NETWORK_CTRL1) & XRFDC_CLK_NETWORK_CTRL1_USE_PLL_MASK;
	} else {
		pll_en_reg = sys_read16(reg_base + base_addr + XRFDC_PLL_DIVIDER0);
		if ((pll_en_reg & (XRFDC_PLL_DIVIDER0_BYP_OPDIV_MASK | XRFDC_PLL_DIVIDER0_MODE_MASK)) == XRFDC_DISABLED) {
			*clock_source = XRFDC_EXTERNAL_CLK;
		} else if ((pll_en_reg & XRFDC_PLL_DIVIDER0_BYP_PLL_MASK) != 0) {
			*clock_source = XRFDC_EXTERNAL_CLK;
		} else {
			*clock_source = XRFDC_INTERNAL_PLL_CLK;
		}
	}

	status = XRFDC_SUCCESS;
return_path:
	return status;
}

static uint32_t xlnx_rfdc_get_max_sample_rate(const struct device *dev, uint32_t type, uint32_t tile_id, double *max_sample_rate) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uint32_t status;
    if ((type != XRFDC_ADC_TILE) && (type != XRFDC_DAC_TILE)) {
		status = XRFDC_FAILURE;
		goto return_path;
	}
	if (tile_id > XRFDC_TILE_ID_MAX) {
		status = XRFDC_FAILURE;
		goto return_path;
	}
	if (type == XRFDC_ADC_TILE) {
		*max_sample_rate = config->adc_tile_config[tile_id].max_sample_rate * 1000;
		if (*max_sample_rate == 0) {
			*max_sample_rate = xlnx_rfdc_is_high_speed_adc(dev, tile_id) ? XRFDC_ADC_4G_SAMPLING_MAX :
											 XRFDC_ADC_2G_SAMPLING_MAX;
		}
	} else {
		*max_sample_rate = config->dac_tile_config[tile_id].max_sample_rate * 1000;
		if (*max_sample_rate == 0) {
			*max_sample_rate = XRFDC_DAC_SAMPLING_MAX;
		}
	}
	status = XRFDC_SUCCESS;
return_path:
	return status;
}

static uint32_t xlnx_rfdc_get_min_sample_rate(const struct device *dev, uint32_t type, uint32_t tile_id, double *min_sample_rate) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uint32_t status;
    if ((type != XRFDC_ADC_TILE) && (type != XRFDC_DAC_TILE)) {
		status = XRFDC_FAILURE;
		goto return_path;
	}
	if (tile_id > XRFDC_TILE_ID_MAX) {
		status = XRFDC_FAILURE;
		goto return_path;
	}
	if (type == XRFDC_ADC_TILE) {
		*min_sample_rate = xlnx_rfdc_is_high_speed_adc(dev, tile_id) ? XRFDC_ADC_4G_SAMPLING_MIN :
										 XRFDC_ADC_2G_SAMPLING_MIN;
	} else {
		*min_sample_rate = XRFDC_DAC_SAMPLING_MIN;
	}
	status = XRFDC_SUCCESS;
return_path:
	return status;
}

static uint32_t xlnx_rfdc_get_plllock_status(const struct device *dev, uint32_t type, uint32_t tile_id, uint32_t *lock_status) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    uint32_t base_addr, read_reg, clk_src = 0, status;
    /*
	 * Get tile clock source information
	 */
	if (xlnx_rfdc_get_clock_source(dev, type, tile_id, &clk_src) != XRFDC_SUCCESS) {
		LOG_INF(" Get clock source request %s %u failed in %s",
			  (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
		status = XRFDC_FAILURE;
		goto return_path;
	}

	if (clk_src == XRFDC_EXTERNAL_CLK) {
		LOG_INF("%s %u uses external clock source in %s",
			  (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
		*lock_status = XRFDC_PLL_LOCKED;
	} else {
		if (type == XRFDC_ADC_TILE) {
			base_addr = XRFDC_ADC_TILE_CTRL_STATS_ADDR(tile_id);
		} else {
			base_addr = XRFDC_DAC_TILE_CTRL_STATS_ADDR(tile_id);
		}

		read_reg = sys_read16(reg_base + base_addr + XRFDC_STATUS_OFFSET) & XRFDC_PLL_LOCKED_MASK;
		if (read_reg != 0U) {
			*lock_status = XRFDC_PLL_LOCKED;
		} else {
			*lock_status = XRFDC_PLL_UNLOCKED;
		}
	}

	status = XRFDC_SUCCESS;
return_path:
	return status;
}


static uint32_t xlnx_rfdc_wait_for_state(const struct device *dev, uint32_t type, 
                uint32_t tile_id, uint32_t state) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    uint32_t status, delay_count, tile_state;
    status = xlnx_rfdc_chk_tile_enable(dev, type, tile_id);
    if (status != XRFDC_SUCCESS) {
		LOG_ERR("Requested tile (%s %u) not available in %s",
			  (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
		goto return_path;
	}
    tile_state = sys_read16(reg_base + XRFDC_CTRL_STS_BASE(type, tile_id) + XRFDC_CURRENT_STATE_OFFSET) & XRFDC_CURRENT_STATE_MASK;
	delay_count = 0U;
	while (tile_state < state) {
		if (delay_count == XRFDC_WAIT_ATTEMPTS_CNT) {
			status = XRFDC_FAILURE;
			LOG_ERR("timeout error in %s[%u] going to state %u in %s",
				  (type ? "DAC" : "ADC"), tile_id, state, __func__);
			goto return_path;
		} else {
			/* Wait for 0.1 msec */
			k_usleep(XRFDC_STATE_WAIT);
			delay_count++;
            tile_state = sys_read16(reg_base + XRFDC_CTRL_STS_BASE(type, tile_id) +
						XRFDC_CURRENT_STATE_OFFSET) & XRFDC_CURRENT_STATE_MASK;
		}
	}

	status = XRFDC_SUCCESS;
return_path:
	return status;
}

static uint32_t xlnx_rfdc_wait_for_restart_clr(const struct device *dev, uint32_t type, uint32_t tile_id, uint32_t base_addr,
                uint32_t end) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    uint32_t clk_src = 0, delay_count, lock_status = 0, status;
    /*
	 * Get tile clock source information
	 */
	if (xlnx_rfdc_get_clock_source(dev, type, tile_id, &clk_src) != XRFDC_SUCCESS) {
		status = XRFDC_FAILURE;
		goto return_path;
	}

	if ((clk_src == XRFDC_INTERNAL_PLL_CLK) && (end > XRFDC_STATE_CLK_DET)) {
		/*
		 * Wait for internal PLL to lock
		 */
		if (xlnx_rfdc_get_plllock_status(dev, type, tile_id, &lock_status) != XRFDC_SUCCESS) {
			status = XRFDC_FAILURE;
			goto return_path;
		}
		delay_count = 0U;
		while (lock_status != XRFDC_PLL_LOCKED) {
			if (delay_count == XRFDC_PLL_LOCK_DLY_CNT) {
				LOG_ERR("&s %u timed out at state %u in %s",
					  (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id,
					  sys_read16(reg_base + base_addr + XRFDC_CURRENT_STATE_OFFSET), __func__);
				status = XRFDC_FAILURE;
				goto return_path;
			} else {
				/* Wait for 1 msec */
				k_usleep(XRFDC_PLL_LOCK_WAIT);
				delay_count++;
				(void)xlnx_rfdc_get_plllock_status(dev, type, tile_id, &lock_status);
			}
		}
	}

	if (end == XRFDC_STATE_FULL) {
		/* Wait till restart bit clear */
		delay_count = 0U;
		while (sys_read16(reg_base + base_addr + XRFDC_RESTART_OFFSET) != 0U) {
			if (delay_count == XRFDC_RESTART_CLR_DLY_CNT) {
				LOG_ERR("%s %u timed out at state %u in %s",
					  (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id,
					  sys_read16(reg_base + base_addr + XRFDC_CURRENT_STATE_OFFSET), __func__);
				status = XRFDC_FAILURE;
				goto return_path;
			} else {
				/* Wait for 1 msec */
				k_usleep(XRFDC_RESTART_CLR_WAIT);
				delay_count++;
			}
		}
	} else {
		status = xlnx_rfdc_wait_for_state(dev, type, tile_id, end);
		if (status == XRFDC_FAILURE) {
			goto return_path;
		}
	}

	status = XRFDC_SUCCESS;
return_path:
	return status;
}

static uint32_t xlnx_rfdc_chk_clk_dist_valid(const struct device *dev) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    uint32_t status, type, tile, pkg_tile_id, pkg_adc_edge_tile;
    uint8_t is_pll, tile_layout;
    uint16_t efuse;
    if (data->dist_clock > XRFDC_DIST_OUT_OUTDIV) {
        LOG_ERR("Invalid parameter valut for distribution out (%u) for %s %u in %s",
        (data->dist_clock, data->source_type == XRFDC_ADC_TILE)? "ADC" : "DAC",
        data->source_tile_id, __func__);
        status = XRFDC_FAILURE;
        goto return_path;
    }
    if ((data->info.source < data->info.lower_bound) ||
	    (data->info.source > data->info.upper_bound)) {
		LOG_ERR( "%s %u does not reside between %s %u and %s %u in %s\r\n",
			  ((data->source_type == XRFDC_ADC_TILE) ? "ADC" : "DAC"),
			  data->source_tile_id,
			  ((data->edge_types[0] == XRFDC_ADC_TILE) ? "ADC" : "DAC"),
			  data->edge_tile_ids[0],
			  ((data->edge_types[1] == XRFDC_ADC_TILE) ? "ADC" : "DAC"),
			  data->edge_tile_ids[1], __func__);
		status = XRFDC_FAILURE;
		goto return_path;
	}
    if (data->info.upper_bound == data->info.lower_bound) {
		if (data->dist_clock != XRFDC_DIST_OUT_NONE) {
			LOG_ERR("Invalid Parameter Value for Distribution Out (%u) for Single tile Distribution in %s",
				data->dist_clock, __func__);
			status = XRFDC_FAILURE;
			goto return_path;
		}
	} else {
		if (data->dist_clock == XRFDC_DIST_OUT_NONE) {
			LOG_ERR("Invalid Parameter Value for Distribution Out (%u) for Multi tile Distribution in %s",
				data->dist_clock, __func__);
			status = XRFDC_FAILURE;
			goto return_path;
		}
	}
    for (pkg_tile_id = data->info.lower_bound; pkg_tile_id < data->info.upper_bound; pkg_tile_id++) {
        xlnx_rfdc_dist_tile2type_tile(dev, pkg_tile_id, &type, &tile);
        status = xlnx_rfdc_chk_tile_enable(dev, type, tile);
        if (status != XRFDC_SUCCESS) {
            LOG_ERR(" %s %u not enabled in %s",((type == XRFDC_ADC_TILE) ? "ADC" : "DAC"), tile, __func__);
            goto return_path;
        }
    }
    tile_layout = xlnx_rfdc_get_tile_layout(dev);
    if (tile_layout == XRFDC_4ADC_4DAC_TILES) {
		is_pll = (data->sample_rates[data->source_type]
							     [data->source_tile_id] >
			 data->dist_ref_clk_freq);
		if ((data->source_tile_id == XRFDC_TILE_ID0) ||
		    (data->source_tile_id == XRFDC_TILE_ID3)) {
			if ((data->dist_clock == XRFDC_DIST_OUT_OUTDIV) ||
			    ((data->dist_clock == XRFDC_DIST_OUT_RX) &&
			     (is_pll == XRFDC_DISABLED))) {
				status = XRFDC_FAILURE;
				LOG_ERR("Distribution of full rate clock from edge tiles not supported in %s",
					  __func__);
				goto return_path;
			}
		}

		if ((data->source_type == XRFDC_DAC_TILE) &&
		    (data->info.upper_bound > XRFDC_CLK_DST_TILE_226)) {
			if ((data->dist_clock == XRFDC_DIST_OUT_OUTDIV) ||
			    (is_pll == XRFDC_DISABLED)) {
				efuse = sys_read16(reg_base + XRFDC_DRP_BASE(XRFDC_ADC_TILE, XRFDC_BLK_ID1) + XRFDC_HSCOM_ADDR + XRFDC_HSCOM_EFUSE_2_OFFSET);
				if (efuse & XRFDC_PREMIUMCTRL_CLKDIST) {
					status = XRFDC_FAILURE;
					LOG_ERR("Invalid Configuration in %s", __func__);
					goto return_path;
				}
			}
		}

		pkg_adc_edge_tile = XRFDC_CLK_DST_TILE_227;
	} else {
		pkg_adc_edge_tile = XRFDC_CLK_DST_TILE_226;
	}

	if ((data->source_type == XRFDC_ADC_TILE) &&
	    (data->info.lower_bound < pkg_adc_edge_tile)) {
		LOG_ERR("DAC Cannot source from ADC in %s", __func__);
		status = XRFDC_FAILURE;
		goto return_path;
	}

	status = XRFDC_SUCCESS;
return_path:
	return status;
}

static uint32_t xlnx_rfdc_restart_ipsm(const struct device *dev, uint32_t type, int tile_id, uint32_t start, uint32_t end) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    uint32_t status, base_addr, tile_layout;
    uint16_t no_of_tile, index;
    /* An input tile if of -1 selects all tiles */
	if (tile_id == XRFDC_SELECT_ALL_TILES) {
		tile_layout = xlnx_rfdc_get_tile_layout(dev);
		if (tile_layout == XRFDC_3ADC_2DAC_TILES) {
			no_of_tile = (type == XRFDC_ADC_TILE) ? XRFDC_TILE_ID3 : XRFDC_TILE_ID2;
		} else {
			no_of_tile = XRFDC_NUM_OF_TILES4;
		}
		index = XRFDC_TILE_ID0;
	} else {
		no_of_tile = tile_id + 1;
		index = tile_id;
	}

	for (; index < no_of_tile; index++) {
		base_addr = XRFDC_CTRL_STS_BASE(type, index);
		status = xlnx_rfdc_chk_tile_enable(dev, type, index);

		if ((status != XRFDC_SUCCESS) && (tile_id != XRFDC_SELECT_ALL_TILES)) {
			LOG_ERR("Requested tile (%s %u) not available in %s",
				  (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", index, __func__);
			goto return_path;
		} else if (status != XRFDC_SUCCESS) {
			LOG_ERR("%s %u not available in %s",
				  (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", index, __func__);
			continue;
		} else {
			/*power saving for Gen 3 Quad ADCs*/
			if ((config->ip_type >= XRFDC_GEN3) &&
			    (xlnx_rfdc_is_high_speed_adc(dev, index) == 0U) && (type != XRFDC_DAC_TILE) &&
			    (end == XRFDC_SM_STATE1)) {
				end = XRFDC_SM_STATE3;
			}
			/* Write start and end states */
			xlnx_rfdc_clrset_reg(dev, reg_base + base_addr, XRFDC_RESTART_STATE_OFFSET, XRFDC_PWR_STATE_MASK,
					(start << XRFDC_RSR_START_SHIFT) | end);

			/* Trigger restart */
			sys_write32(XRFDC_RESTART_MASK, reg_base + base_addr + XRFDC_RESTART_OFFSET);

			/* Wait for restart bit clear */
			status = xlnx_rfdc_wait_for_restart_clr(dev, type, index, base_addr, end);
			if (status != XRFDC_SUCCESS) {
				goto return_path;
			}
		}
	}

	status = XRFDC_SUCCESS;
return_path:
	return status;
}

static uint32_t xlnx_rfdc_startup(const struct device *dev, uint32_t type, uint32_t tile_id) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uint32_t status;
    status = xlnx_rfdc_restart_ipsm(dev, type, tile_id,  XRFDC_SM_STATE1, XRFDC_SM_STATE15);
	return status;
}

static uint32_t xlnx_rfdc_startup_dist(const struct device *dev)
{
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    uint32_t status, type, tile;
    uint8_t pkg_tile_id;

	status = XRFDC_SUCCESS;

	for (pkg_tile_id = data->info.lower_bound;
	     pkg_tile_id <= data->info.upper_bound; pkg_tile_id++) {
		xlnx_rfdc_dist_tile2type_tile(dev, pkg_tile_id, &type, &tile);
		xlnx_rfdc_clrset_reg(dev, reg_base + XRFDC_CTRL_STS_BASE(type, tile), XRFDC_RESTART_STATE_OFFSET,
				XRFDC_PWR_STATE_MASK, (XRFDC_SM_STATE1 << XRFDC_RSR_START_SHIFT) | XRFDC_SM_STATE15);
		/* Trigger restart */
		sys_write32(XRFDC_RESTART_MASK,reg_base + XRFDC_CTRL_STS_BASE(type, tile) + XRFDC_RESTART_OFFSET);
	}
	/*Ensure source tile reaches state where it is fit to distribute*/
	status |= xlnx_rfdc_wait_for_state(dev, data->source_type,
				     data->source_tile_id, XRFDC_SM_STATE7);

	for (pkg_tile_id = data->info.lower_bound;
	     pkg_tile_id <= data->info.upper_bound; pkg_tile_id++) {
		xlnx_rfdc_dist_tile2type_tile(dev, pkg_tile_id, &type, &tile);
		status |= xlnx_rfdc_wait_for_state(dev, type, tile, XRFDC_SM_STATE15);
	}
	return status;
}

static uint32_t xlnx_rfdc_get_pll_config(const struct device *dev, uint32_t type, uint32_t tile_id, struct xlnx_rfdc_pll_settings *pll_settings) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    uint32_t status, base_addr, fb_div, ref_clk_div, en = XRFDC_DISABLED, pll_freq, pll_fs;
    uint8_t output_div, divide_mode;
    uint16_t read_reg;
    double ref_clk_freq, sample_rate;
    status = xlnx_rfdc_chk_tile_enable(dev, type, tile_id);
	if (status != XRFDC_SUCCESS) {
		LOG_ERR("Requested tile (%s %u) not available in %s",
			  (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
		goto return_path;
	}

	base_addr = XRFDC_CTRL_STS_BASE(type, tile_id);
	pll_freq = sys_read32(reg_base + base_addr + XRFDC_PLL_FREQ);

	ref_clk_freq = ((double)pll_freq) / XRFDC_MILLI;
	pll_fs = sys_read32(reg_base + base_addr + XRFDC_PLL_FS);
	sample_rate = ((double)pll_fs) / XRFDC_MICRO;
	if (pll_fs == 0) {
		/*This code is here to support the old IPs.*/
		if (type == XRFDC_ADC_TILE) {
			pll_settings->en = data->adc_tile[tile_id].pll_settings.en;
			pll_settings->feedback_divider = data->adc_tile[tile_id].pll_settings.feedback_divider;
			pll_settings->output_divider = data->adc_tile[tile_id].pll_settings.output_divider;
			pll_settings->ref_clk_divider = data->adc_tile[tile_id].pll_settings.ref_clk_divider;
			pll_settings->ref_clk_freq = data->adc_tile[tile_id].pll_settings.ref_clk_freq;
			pll_settings->sample_rate = data->adc_tile[tile_id].pll_settings.sample_rate;
			status = XRFDC_SUCCESS;
			goto return_path;
		} else {
			pll_settings->en = data->dac_tile[tile_id].pll_settings.en;
			pll_settings->feedback_divider = data->dac_tile[tile_id].pll_settings.feedback_divider;
			pll_settings->output_divider = data->dac_tile[tile_id].pll_settings.output_divider;
			pll_settings->ref_clk_divider = data->dac_tile[tile_id].pll_settings.ref_clk_divider;
			pll_settings->ref_clk_freq = data->dac_tile[tile_id].pll_settings.ref_clk_freq;
			pll_settings->sample_rate = data->dac_tile[tile_id].pll_settings.sample_rate;
			status = XRFDC_SUCCESS;
			goto return_path;
		}
	} else {
		if (type == XRFDC_ADC_TILE) {
			base_addr = XRFDC_ADC_TILE_DRP_ADDR(tile_id);
		} else {
			base_addr = XRFDC_DAC_TILE_DRP_ADDR(tile_id);
		}

		base_addr += XRFDC_HSCOM_ADDR;

		fb_div = (sys_read16(reg_base + base_addr + XRFDC_PLL_FPDIV) & 0x00FF) + 2;

		read_reg = sys_read16(reg_base + base_addr + XRFDC_PLL_REFDIV);
		if (read_reg & XRFDC_REFCLK_DIV_1_MASK) {
			ref_clk_div = XRFDC_REF_CLK_DIV_1;
		} else {
			switch (read_reg & XRFDC_REFCLK_DIV_MASK) {
			case XRFDC_REFCLK_DIV_2_MASK:
				ref_clk_div = XRFDC_REF_CLK_DIV_2;
				break;
			case XRFDC_REFCLK_DIV_3_MASK:
				ref_clk_div = XRFDC_REF_CLK_DIV_3;
				break;
			case XRFDC_REFCLK_DIV_4_MASK:
				ref_clk_div = XRFDC_REF_CLK_DIV_4;
				break;
			default:
				/*
					 * IP currently supporting 1 to 4 divider values. This
					 * error condition might change in future based on IP update.
					 */
				LOG_ERR("Unsupported Reference clock Divider value (%u) for %s %u in %s",
					  (read_reg & XRFDC_REFCLK_DIV_MASK), (type == XRFDC_ADC_TILE) ? "ADC" : "DAC",
					  tile_id, __func__);
				status = XRFDC_FAILURE;
				goto return_path;
			}
		}
		if (xlnx_rfdc_get_clock_source(dev, type, tile_id, &en) != XRFDC_SUCCESS) {
			status = XRFDC_FAILURE;
			goto return_path;
		}

		read_reg = sys_read16(reg_base + base_addr + XRFDC_PLL_DIVIDER0);
		divide_mode = (read_reg & XRFDC_PLL_DIVIDER0_MODE_MASK) >> XRFDC_PLL_DIVIDER0_SHIFT;

		switch (divide_mode) {
		case XRFDC_PLL_OUTDIV_MODE_1:
			output_div = 1;
			break;
		case XRFDC_PLL_OUTDIV_MODE_2:
			output_div = 2;
			break;
		case XRFDC_PLL_OUTDIV_MODE_3:
			output_div = 3;
			break;
		case XRFDC_PLL_OUTDIV_MODE_N:
			output_div = ((read_reg & XRFDC_PLL_DIVIDER0_VALUE_MASK) + 2) << 1;
			break;
		default:
			LOG_ERR("Unsupported Output clock Divider value (%u) for %s %u in %s",
				  divide_mode, (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
			status = XRFDC_FAILURE;
			goto return_path;
			break;
		}
		pll_settings->en = en;
		pll_settings->feedback_divider = fb_div;
		pll_settings->output_divider = output_div;
		pll_settings->ref_clk_divider = ref_clk_div;
		pll_settings->ref_clk_freq = ref_clk_freq;
		pll_settings->sample_rate = sample_rate;
	}

	status = XRFDC_SUCCESS;
return_path:

	return status;
}

static uint32_t xlnx_rfdc_set_pll_config(const struct device *dev, uint32_t type, uint32_t tile_id, 
                double ref_clk_freq, double sample_rate) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    uint32_t base_addr, status, fb_div, output_div, best_fb_div = 0, best_output_div = 0x02, div_mode = 0,
                div_val = 0, pll_freq_index = 0, fb_div_index = 0, ref_clk_div = 0x01, vco_min, vco_max;
    uint16_t read_reg;
    double cal_sample_rate, pll_freq, sample_err, best_err = 0xFFFFFFFF;
    if (type == XRFDC_ADC_TILE) {
		base_addr = XRFDC_ADC_TILE_DRP_ADDR(tile_id);
	} else {
		base_addr = XRFDC_DAC_TILE_DRP_ADDR(tile_id);
	}

	base_addr += XRFDC_HSCOM_ADDR;

	read_reg = sys_read16(reg_base + base_addr + XRFDC_PLL_REFDIV);
	if (read_reg & XRFDC_REFCLK_DIV_1_MASK) {
		ref_clk_div = XRFDC_REF_CLK_DIV_1;
	} else {
		switch (read_reg & XRFDC_REFCLK_DIV_MASK) {
		case XRFDC_REFCLK_DIV_2_MASK:
			ref_clk_div = XRFDC_REF_CLK_DIV_2;
			break;
		case XRFDC_REFCLK_DIV_3_MASK:
			ref_clk_div = XRFDC_REF_CLK_DIV_3;
			break;
		case XRFDC_REFCLK_DIV_4_MASK:
			ref_clk_div = XRFDC_REF_CLK_DIV_4;
			break;
		default:
			/*
				 * IP currently supporting 1 to 4 divider values. This
				 * error condition might change in future based on IP update.
				 */
			LOG_ERR("Unsupported Reference clock Divider value (%u) for %s %u in %s",
				  (read_reg & XRFDC_REFCLK_DIV_MASK), (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id,
				  __func__);
			return XRFDC_FAILURE;
		}
	}

	ref_clk_freq /= ref_clk_div;

	/*
	 * Sweep valid integer values of fb_div(N) and record a list
	 * of values that fall in the valid VCO range 8.5GHz - 12.8GHz
	 */

	if (config->ip_type < XRFDC_GEN3) {
		vco_min = VCO_RANGE_MIN;
		vco_max = VCO_RANGE_MAX;
	} else {
		if (type == XRFDC_ADC_TILE) {
			vco_min = VCO_RANGE_ADC_MIN;
			vco_max = VCO_RANGE_ADC_MAX;
		} else {
			vco_min = VCO_RANGE_DAC_MIN;
			vco_max = VCO_RANGE_DAC_MAX;
		}
	}

	for (fb_div = PLL_FPDIV_MIN; fb_div <= PLL_FPDIV_MAX; fb_div++) {
		pll_freq = fb_div * ref_clk_freq;

		if ((pll_freq >= vco_min) && (pll_freq <= vco_max)) {
			/*
			 * Sweep values of output_div(M) to find the output frequency
			 * that best matches the user requested value
			 */
			if (config->ip_type >= XRFDC_GEN3) {
				output_div = PLL_DIVIDER_MIN_GEN3;
				cal_sample_rate = (pll_freq / output_div);

				if (sample_rate > cal_sample_rate) {
					sample_err = sample_rate - cal_sample_rate;
				} else {
					sample_err = cal_sample_rate - sample_rate;
				}

				if (best_err > sample_err) {
					best_fb_div = fb_div;
					best_output_div = output_div;
					best_err = sample_err;
				}
			}
			for (output_div = PLL_DIVIDER_MIN; output_div <= PLL_DIVIDER_MAX; output_div += 2U) {
				cal_sample_rate = (pll_freq / output_div);

				if (sample_rate > cal_sample_rate) {
					sample_err = sample_rate - cal_sample_rate;
				} else {
					sample_err = cal_sample_rate - sample_rate;
				}

				if (best_err > sample_err) {
					best_fb_div = fb_div;
					best_output_div = output_div;
					best_err = sample_err;
				}
			}

			output_div = 3U;
			cal_sample_rate = (pll_freq / output_div);

			if (sample_rate > cal_sample_rate) {
				sample_err = sample_rate - cal_sample_rate;
			} else {
				sample_err = cal_sample_rate - sample_rate;
			}

			if (best_err > sample_err) {
				best_fb_div = fb_div;
				best_output_div = output_div;
				best_err = sample_err;
			}
		}

		/*
		 * PLL Static configuration
		 */
		sys_write16(0x80U, reg_base + base_addr + XRFDC_PLL_SDM_CFG0);
		sys_write16(0x111U, reg_base + base_addr + XRFDC_PLL_SDM_SEED0);
		sys_write16(0x11U, reg_base + base_addr + XRFDC_PLL_SDM_SEED1);
		sys_write16(0x08U, reg_base + base_addr + XRFDC_PLL_VCO1);
		if (config->ip_type < XRFDC_GEN3) {
			sys_write16(0x45U, reg_base + base_addr + XRFDC_PLL_VREG);
			sys_write16(0x5800U, reg_base + base_addr + XRFDC_PLL_VCO0);

		} else {
			sys_write16(0x2DU, reg_base + base_addr + XRFDC_PLL_VREG);
			sys_write16(0x5F03U, reg_base + base_addr + XRFDC_PLL_VCO0);
		}
		/*
		 * Set Feedback divisor value
		 */
		sys_write16( best_fb_div - 2U, reg_base + base_addr + XRFDC_PLL_FPDIV);

		/*
		 * Set Output divisor value
		 */
		if (best_output_div == 1U) {
			div_mode = 0x0U;
			/*if divisor is 1 bypass toatally*/
			div_val = XRFDC_PLL_DIVIDER0_BYP_OPDIV_MASK;
		} else if (best_output_div == 2U) {
			div_mode = 0x1U;
		} else if (best_output_div == 3U) {
			div_mode = 0x2U;
			div_val = 0x1U;
		} else if (best_output_div >= 4U) {
			div_mode = 0x3U;
			div_val = ((best_output_div - 4U) / 2U);
		}

		xlnx_rfdc_clrset_reg(dev, reg_base + base_addr, XRFDC_PLL_DIVIDER0, XRFDC_PLL_DIVIDER0_MASK,
				((div_mode << XRFDC_PLL_DIVIDER0_SHIFT) | div_val));

		if (config->ip_type >= XRFDC_GEN3) {
			if (best_output_div > PLL_DIVIDER_MIN_GEN3) {
				xlnx_rfdc_clrset_reg(dev, reg_base + base_addr, XRFDC_PLL_DIVIDER0, XRFDC_PLL_DIVIDER0_ALT_MASK,
						XRFDC_DISABLED);
			} else {
				xlnx_rfdc_clrset_reg(dev, reg_base + base_addr, XRFDC_PLL_DIVIDER0, XRFDC_PLL_DIVIDER0_ALT_MASK,
						XRFDC_PLL_DIVIDER0_BYPDIV_MASK);
			}
		}
		/*
		 * Enable fine sweep
		 */
		sys_write16(XRFDC_PLL_CRS2_VAL, reg_base + base_addr + XRFDC_PLL_CRS2);

		/*
		 * Set default PLL spare inputs LSB
		 */
		if (config->ip_type < XRFDC_GEN3) {
			sys_write16(0x507U, reg_base + base_addr + XRFDC_PLL_SPARE0);
		} else {
			sys_write16(0x0D37U, reg_base + base_addr + XRFDC_PLL_SPARE0);
		}
		/*
		 * Set PLL spare inputs MSB
		 */
		if (config->ip_type < XRFDC_GEN3) {
			sys_write16(0x0U, reg_base + base_addr + XRFDC_PLL_SPARE1);
		} else {
			sys_write16(0x80U, reg_base + base_addr + XRFDC_PLL_SPARE1);
		}
		pll_freq = ref_clk_freq * best_fb_div;

		if (pll_freq < 9400U) {
			pll_freq_index = 0U;
			fb_div_index = 2U;
			if (best_fb_div < 21U) {
				fb_div_index = 0U;
			} else if (best_fb_div < 30U) {
				fb_div_index = 1U;
			}
		} else if (pll_freq < 10070U) {
			pll_freq_index = 1U;
			fb_div_index = 2U;
			if (best_fb_div < 18U) {
				fb_div_index = 0U;
			} else if (best_fb_div < 30U) {
				fb_div_index = 1U;
			}
		} else if (pll_freq < 10690U) {
			pll_freq_index = 2U;
			fb_div_index = 3U;
			if (best_fb_div < 18U) {
				fb_div_index = 0U;
			} else if (best_fb_div < 25U) {
				fb_div_index = 1U;
			} else if (best_fb_div < 35U) {
				fb_div_index = 2U;
			}
		} else if (pll_freq < 10990U) {
			pll_freq_index = 3U;
			fb_div_index = 3U;
			if (best_fb_div < 19U) {
				fb_div_index = 0U;
			} else if (best_fb_div < 27U) {
				fb_div_index = 1U;
			} else if (best_fb_div < 38U) {
				fb_div_index = 2U;
			}
		} else if (pll_freq < 11430U) {
			pll_freq_index = 4U;
			fb_div_index = 3U;
			if (best_fb_div < 19U) {
				fb_div_index = 0U;
			} else if (best_fb_div < 27U) {
				fb_div_index = 1U;
			} else if (best_fb_div < 38U) {
				fb_div_index = 2U;
			}
		} else if (pll_freq < 12040U) {
			pll_freq_index = 5U;
			fb_div_index = 3U;
			if (best_fb_div < 20U) {
				fb_div_index = 0U;
			} else if (best_fb_div < 28U) {
				fb_div_index = 1U;
			} else if (best_fb_div < 40U) {
				fb_div_index = 2U;
			}
		} else if (pll_freq < 12530U) {
			pll_freq_index = 6U;
			fb_div_index = 3U;
			if (best_fb_div < 23U) {
				fb_div_index = 0U;
			} else if (best_fb_div < 30U) {
				fb_div_index = 1U;
			} else if (best_fb_div < 42U) {
				fb_div_index = 2U;
			}
		} else if (pll_freq < 20000U) {
			pll_freq_index = 7U;
			fb_div_index = 2U;
			if (best_fb_div < 20U) {
				fb_div_index = 0U;
				/*
				 * Set PLL spare inputs LSB
				 */
				if (config->ip_type < XRFDC_GEN3) {
					sys_write16(0x577, reg_base + base_addr + XRFDC_PLL_SPARE0);
				} else {
					sys_write16(0x0D37U, reg_base + base_addr + XRFDC_PLL_SPARE0);
				}
			} else if (best_fb_div < 39U) {
				fb_div_index = 1U;
			}
		}

		/*
		 * Enable automatic selection of the VCO, this will work with the
		 * IP version 2.0.1 and above and using older version of IP is
		 * not likely to work.
		 */

		xlnx_rfdc_clrset_reg(dev, reg_base + base_addr, XRFDC_PLL_CRS1, XRFDC_PLL_VCO_SEL_AUTO_MASK,
				XRFDC_PLL_VCO_SEL_AUTO_MASK);

		/*
		 * PLL bits for loop filters LSB
		 */
		sys_write16(pll_tuning_matrix[pll_freq_index][fb_div_index][0], reg_base + base_addr + XRFDC_PLL_LPF0);

		/*
		 * PLL bits for loop filters MSB
		 */
		sys_write16(XRFDC_PLL_LPF1_VAL, reg_base + base_addr + XRFDC_PLL_LPF1);

		/*
		 * Set PLL bits for charge pumps
		 */
		sys_write16(pll_tuning_matrix[pll_freq_index][fb_div_index][1], reg_base + base_addr + XRFDC_PLL_CHARGEPUMP);
	}

	cal_sample_rate = (best_fb_div * ref_clk_freq) / best_output_div;
	/* Store Sampling Frequency in kHz */
	sys_write32((cal_sample_rate * XRFDC_MILLI), reg_base + XRFDC_CTRL_STS_BASE(type, tile_id) + XRFDC_PLL_FS);
	/* Convert to GHz */
	cal_sample_rate /= XRFDC_MILLI;

	if (type == XRFDC_ADC_TILE) {
		data->adc_tile[tile_id].pll_settings.sample_rate = cal_sample_rate;
		data->adc_tile[tile_id].pll_settings.ref_clk_divider = ref_clk_div;
		data->adc_tile[tile_id].pll_settings.feedback_divider = best_fb_div;
		data->adc_tile[tile_id].pll_settings.output_divider = best_output_div;
	} else {
		data->dac_tile[tile_id].pll_settings.sample_rate = cal_sample_rate;
		data->dac_tile[tile_id].pll_settings.ref_clk_divider = ref_clk_div;
		data->dac_tile[tile_id].pll_settings.feedback_divider = best_fb_div;
		data->dac_tile[tile_id].pll_settings.output_divider = best_output_div;
	}

	status= XRFDC_SUCCESS;

	return status;
}

static uint32_t xlnx_rfdc_dynamic_pll_config(const struct device *dev, uint32_t type, uint32_t tile_id, uint8_t source,
                double ref_clk_freq, double sampling_rate) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    uint32_t clk_src = 0, status, base_addr, pll_en = 0, init_powerup_state, op_div, pll_freq,
    pll_fs, divide_mode, primary_div_val, secondary_div_val = 0, pll_bypass_val, net_ctrl_reg = 0,
    north_clk = 0, fg_delay = 0, adc_edge_tile_id, tile_layout;
    double max_sample_rate = 0.0, min_sample_rate = 0.0;
    if ((source != XRFDC_INTERNAL_PLL_CLK) && (source != XRFDC_EXTERNAL_CLK)) {
		LOG_ERR("Invalid source value (%u) for %s %u in %s", source,
			  (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
		status = XRFDC_FAILURE;
		goto return_path;
	}

	status = xlnx_rfdc_chk_tile_enable(dev, type, tile_id);
	if (status != XRFDC_SUCCESS) {
		LOG_ERR("Requested tile (%s %u) not available in %s",
			  (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
		goto return_path;
	}

	/*
	 * Get tile clock source information
	 */
	if (xlnx_rfdc_get_clock_source(dev, type, tile_id, &clk_src) != XRFDC_SUCCESS) {
		status = XRFDC_FAILURE;
		goto return_path;
	}

	if (xlnx_rfdc_get_max_sample_rate(dev, type, tile_id, &max_sample_rate) != XRFDC_SUCCESS) {
		status = XRFDC_FAILURE;
		goto return_path;
	}
	if (xlnx_rfdc_get_min_sample_rate(dev, type, tile_id, &min_sample_rate) != XRFDC_SUCCESS) {
		status = XRFDC_FAILURE;
		goto return_path;
	}
	if ((sampling_rate < min_sample_rate) || (sampling_rate > max_sample_rate)) {
		LOG_ERR("Invalid sampling rate value (%lf) for %s %u in %s", sampling_rate,
			  (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
		status = XRFDC_FAILURE;
		goto return_path;
	}

	pll_freq = ((ref_clk_freq + 0.0005) * XRFDC_MILLI);
	pll_fs = ((sampling_rate + 0.0005) * XRFDC_MILLI);
	op_div = ((ref_clk_freq / sampling_rate) + 0.5);
	if ((source == XRFDC_EXTERNAL_CLK) && (pll_freq != pll_fs)) {
		if (config->ip_type < XRFDC_GEN3) {
			LOG_ERR("Sampling rate value (%lf) must match the reference frequency (%lf) for %s %u in %s",
				sampling_rate, ref_clk_freq, (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
			status = XRFDC_FAILURE;
			goto return_path;
		} else if ((pll_freq % pll_fs) != 0U) {
			LOG_ERR("The reference frequency (%lf) must be an integer multiple of the Sampling rate (%lf) for %s %u in %",
				ref_clk_freq, sampling_rate, (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
			status = XRFDC_FAILURE;
			goto return_path;
		}
	}

	if (source == XRFDC_INTERNAL_PLL_CLK) {
		if ((ref_clk_freq < XRFDC_REFFREQ_MIN) || (ref_clk_freq > XRFDC_REFFREQ_MAX)) {
			LOG_ERR("Input reference clock frequency (%lf MHz) does not respect the specifications for internal PLL usage. Please use a different frequency (%lf - %lf MHz) or bypass the internal PLL for %s %u in %s",
				ref_clk_freq, XRFDC_REFFREQ_MIN, XRFDC_REFFREQ_MAX,
				(type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
			status = XRFDC_FAILURE;
			goto return_path;
		}
	}

	if (config->ip_type < XRFDC_GEN3) {
		if ((source != XRFDC_INTERNAL_PLL_CLK) && (clk_src != XRFDC_INTERNAL_PLL_CLK)) {
			LOG_ERR("Requested tile (%s %u) uses external clock source in %s",
				  (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
			if (type == XRFDC_ADC_TILE) {
				data->adc_tile[tile_id].pll_settings.sample_rate =
					(double)(sampling_rate / XRFDC_MILLI);
				data->adc_tile[tile_id].pll_settings.ref_clk_freq = ref_clk_freq;
			} else {
				data->dac_tile[tile_id].pll_settings.sample_rate =
					(double)(sampling_rate / XRFDC_MILLI);
				data->dac_tile[tile_id].pll_settings.ref_clk_freq = ref_clk_freq;
			}
			status = XRFDC_SUCCESS;
			goto return_path;
		}
	} else {
		base_addr = XRFDC_DRP_BASE(type, tile_id) + XRFDC_HSCOM_ADDR;
		net_ctrl_reg = sys_read16(reg_base + base_addr + XRFDC_CLK_NETWORK_CTRL1);
	}

	if (type == XRFDC_ADC_TILE) {
		base_addr = XRFDC_ADC_TILE_CTRL_STATS_ADDR(tile_id);
        init_powerup_state = (sys_read16(reg_base + base_addr + XRFDC_STATUS_OFFSET) & XRFDC_PWR_UP_STAT_MASK) >> 
				      XRFDC_PWR_UP_STAT_SHIFT;
		base_addr = XRFDC_ADC_TILE_DRP_ADDR(tile_id) + XRFDC_HSCOM_ADDR;
	} else {
		base_addr = XRFDC_DAC_TILE_CTRL_STATS_ADDR(tile_id);
		init_powerup_state = (sys_read16(reg_base + base_addr + XRFDC_STATUS_OFFSET) & XRFDC_PWR_UP_STAT_MASK) >>
				      XRFDC_PWR_UP_STAT_SHIFT;
		base_addr = XRFDC_DAC_TILE_DRP_ADDR(tile_id) + XRFDC_HSCOM_ADDR;
	}

	/*
	 * Stop the ADC or DAC tile by putting tile in reset state if not stopped already
	 */
	base_addr = XRFDC_CTRL_STS_BASE(type, tile_id);
	init_powerup_state = (sys_read16(reg_base + base_addr + XRFDC_STATUS_OFFSET) & XRFDC_PWR_UP_STAT_MASK) >>
			      XRFDC_PWR_UP_STAT_SHIFT;
	base_addr = XRFDC_DRP_BASE(type, tile_id) + XRFDC_HSCOM_ADDR;

	if (source == XRFDC_INTERNAL_PLL_CLK) {
		pll_en = 0x1;
		/*
		 * Configure the PLL
		 */
		if (xlnx_rfdc_set_pll_config(dev, type, tile_id, ref_clk_freq, sampling_rate) != XRFDC_SUCCESS) {
			status = XRFDC_FAILURE;
			goto return_path;
		}
		if (config->ip_type >= XRFDC_GEN3) {
			xlnx_rfdc_clrset_reg(dev, reg_base + base_addr, XRFDC_PLL_DIVIDER0, XRFDC_PLL_DIVIDER0_BYP_PLL_MASK,
					XRFDC_DISABLED);
			if ((net_ctrl_reg & XRFDC_CLK_NETWORK_CTRL1_REGS_MASK) != XRFDC_DISABLED) {
				sys_write16(XRFDC_HSCOM_PWR_STATS_RX_PLL, reg_base + base_addr + XRFDC_HSCOM_PWR_STATE_OFFSET);
			} else {
				sys_write16(XRFDC_HSCOM_PWR_STATS_DIST_PLL, reg_base + base_addr + XRFDC_HSCOM_PWR_STATE_OFFSET);
			}
		} else {
			xlnx_rfdc_clrset_reg(dev, reg_base + base_addr, XRFDC_CLK_NETWORK_CTRL1,
					XRFDC_CLK_NETWORK_CTRL1_USE_PLL_MASK, XRFDC_CLK_NETWORK_CTRL1_USE_PLL_MASK);
			sys_write16(XRFDC_HSCOM_PWR_STATS_PLL, reg_base + base_addr + XRFDC_HSCOM_PWR_STATE_OFFSET);
		}
		if ((type == XRFDC_ADC_TILE) && (xlnx_rfdc_is_high_speed_adc(dev, tile_id) == XRFDC_DISABLED)) {
			xlnx_rfdc_clrset_reg(dev, reg_base + XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, tile_id, XRFDC_BLK_ID0),
					XRFDC_ADC_DAC_MC_CFG0_OFFSET,
					(XRFDC_RX_PR_MC_CFG0_IDIV_MASK | XRFDC_RX_PR_MC_CFG0_PSNK_MASK),
					XRFDC_RX_PR_MC_CFG0_PSNK_MASK);
			xlnx_rfdc_clrset_reg(dev, reg_base + XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, tile_id, XRFDC_BLK_ID1),
					XRFDC_ADC_DAC_MC_CFG0_OFFSET,
					(XRFDC_RX_PR_MC_CFG0_IDIV_MASK | XRFDC_RX_PR_MC_CFG0_PSNK_MASK),
					XRFDC_RX_PR_MC_CFG0_PSNK_MASK);
			xlnx_rfdc_clrset_reg(dev, reg_base + XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, tile_id, XRFDC_BLK_ID2),
					XRFDC_ADC_DAC_MC_CFG0_OFFSET,
					(XRFDC_RX_PR_MC_CFG0_IDIV_MASK | XRFDC_RX_PR_MC_CFG0_PSNK_MASK),
					XRFDC_RX_PR_MC_CFG0_PSNK_MASK);
			xlnx_rfdc_clrset_reg(dev, reg_base + XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, tile_id, XRFDC_BLK_ID3),
					XRFDC_ADC_DAC_MC_CFG0_OFFSET,
					(XRFDC_RX_PR_MC_CFG0_IDIV_MASK | XRFDC_RX_PR_MC_CFG0_PSNK_MASK),
					XRFDC_RX_PR_MC_CFG0_PSNK_MASK);
		}
	} else {
		if (config->ip_type >= XRFDC_GEN3) {
			tile_layout = xlnx_rfdc_get_tile_layout(dev);
			adc_edge_tile_id = (tile_layout == XRFDC_3ADC_2DAC_TILES) ? XRFDC_TILE_ID2 : XRFDC_TILE_ID3;
			switch (op_div) {
			case 1U:
				/*This is a special case where we want to totally bypass the entire block.*/
				pll_bypass_val = XRFDC_DISABLED;
				divide_mode = XRFDC_PLL_OUTDIV_MODE_1;
				primary_div_val = XRFDC_DISABLED;
				secondary_div_val = XRFDC_RX_PR_MC_CFG0_PSNK_MASK;
				break;
			case 2U:
				/*dividers used depend on configuration*/
				if ((type == XRFDC_ADC_TILE) && (tile_id < adc_edge_tile_id) &&
				    (xlnx_rfdc_is_high_speed_adc(dev, tile_id) == XRFDC_DISABLED)) {
					north_clk = sys_read16(reg_base + XRFDC_ADC_TILE_DRP_ADDR((tile_id + 1U)) + XRFDC_HSCOM_ADDR + XRFDC_HSCOM_CLK_DSTR_OFFSET) &
						(XRFDC_CLK_DISTR_MUX6_SRC_INT | XRFDC_CLK_DISTR_MUX6_SRC_NTH);

					secondary_div_val = sys_read16(reg_base + XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, (tile_id + 1U), XRFDC_BLK_ID0) + XRFDC_ADC_DAC_MC_CFG0_OFFSET) & 
						(XRFDC_RX_PR_MC_CFG0_IDIV_MASK | XRFDC_RX_PR_MC_CFG0_PSNK_MASK);
				}
				if ((north_clk != XRFDC_CLK_DISTR_MUX6_SRC_OFF) &&
				    (secondary_div_val == XRFDC_RX_PR_MC_CFG0_IDIV_MASK)) {
					pll_bypass_val = XRFDC_DISABLED;
					divide_mode = XRFDC_PLL_OUTDIV_MODE_1;
					primary_div_val = XRFDC_DISABLED;
				} else {
					pll_bypass_val = XRFDC_PLL_DIVIDER0_BYP_PLL_MASK;
					divide_mode = XRFDC_PLL_OUTDIV_MODE_2;
					primary_div_val = XRFDC_DISABLED;
					secondary_div_val = XRFDC_RX_PR_MC_CFG0_PSNK_MASK;
				}
				break;
			case 4U:
				pll_bypass_val = XRFDC_PLL_DIVIDER0_BYP_PLL_MASK;
				if ((type == XRFDC_ADC_TILE) && (tile_id == adc_edge_tile_id) &&
				    (xlnx_rfdc_is_high_speed_adc(dev, tile_id) == XRFDC_DISABLED)) {
					divide_mode = XRFDC_PLL_OUTDIV_MODE_2;
					primary_div_val = XRFDC_DISABLED;
					secondary_div_val = XRFDC_RX_PR_MC_CFG0_IDIV_MASK;
				} else {
					LOG_ERR("Invalid divider value (%u) for %s %u in %s",
						  op_div, (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
					status = XRFDC_FAILURE;
					goto return_path;
				}

				break;
			default:
				LOG_ERR("Invalid divider value (%u) for %s %u in %s", op_div,
					  (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
				status = XRFDC_FAILURE;
				goto return_path;
			}

			if (op_div == 1) {
				if ((net_ctrl_reg & XRFDC_CLK_NETWORK_CTRL1_REGS_MASK) != XRFDC_DISABLED) {
					sys_write16(XRFDC_HSCOM_PWR_STATS_DIST_EXT_SRC, reg_base + base_addr + XRFDC_HSCOM_PWR_STATE_OFFSET);
				} else {
					sys_write16(XRFDC_HSCOM_PWR_STATS_DIST_EXT, reg_base + base_addr + XRFDC_HSCOM_PWR_STATE_OFFSET);
				}

			} else {
				sys_write16(XRFDC_HSCOM_PWR_STATS_DIST_EXT, reg_base + base_addr + XRFDC_HSCOM_PWR_STATE_OFFSET);
				if ((net_ctrl_reg & XRFDC_CLK_NETWORK_CTRL1_REGS_MASK) != XRFDC_DISABLED) {
					sys_write16(XRFDC_HSCOM_PWR_STATS_DIST_EXT_DIV_SRC, reg_base + base_addr + XRFDC_HSCOM_PWR_STATE_OFFSET);
				} else {
					sys_write16(XRFDC_HSCOM_PWR_STATS_DIST_EXT_DIV, reg_base + base_addr + XRFDC_HSCOM_PWR_STATE_OFFSET);
				}
			}
			if ((type == XRFDC_ADC_TILE) &&
			    (xlnx_rfdc_is_high_speed_adc(dev, tile_id) == XRFDC_DISABLED)) {
				xlnx_rfdc_clrset_reg(dev, reg_base + XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, tile_id, XRFDC_BLK_ID0),
						XRFDC_ADC_DAC_MC_CFG0_OFFSET,
						(XRFDC_RX_PR_MC_CFG0_IDIV_MASK | XRFDC_RX_PR_MC_CFG0_PSNK_MASK),
						secondary_div_val);
				xlnx_rfdc_clrset_reg(dev, reg_base + XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, tile_id, XRFDC_BLK_ID1),
						XRFDC_ADC_DAC_MC_CFG0_OFFSET,
						(XRFDC_RX_PR_MC_CFG0_IDIV_MASK | XRFDC_RX_PR_MC_CFG0_PSNK_MASK),
						secondary_div_val);
				xlnx_rfdc_clrset_reg(dev, reg_base + XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, tile_id, XRFDC_BLK_ID2),
						XRFDC_ADC_DAC_MC_CFG0_OFFSET,
						(XRFDC_RX_PR_MC_CFG0_IDIV_MASK | XRFDC_RX_PR_MC_CFG0_PSNK_MASK),
						secondary_div_val);
				xlnx_rfdc_clrset_reg(dev, reg_base + XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, tile_id, XRFDC_BLK_ID3),
						XRFDC_ADC_DAC_MC_CFG0_OFFSET,
						(XRFDC_RX_PR_MC_CFG0_IDIV_MASK | XRFDC_RX_PR_MC_CFG0_PSNK_MASK),
						secondary_div_val);
			}
			xlnx_rfdc_clrset_reg(dev, reg_base + XRFDC_DRP_BASE(type, tile_id) + XRFDC_HSCOM_ADDR,
					XRFDC_PLL_DIVIDER0, XRFDC_PLL_DIVIDER0_MASK,
					((divide_mode << XRFDC_PLL_DIVIDER0_SHIFT) | primary_div_val | pll_bypass_val));
		} else {
			op_div = 0; /*keep backwards compatibility */
			sys_write16(XRFDC_HSCOM_PWR_STATS_EXTERNAL, reg_base + base_addr + XRFDC_HSCOM_PWR_STATE_OFFSET);
		}

		xlnx_rfdc_clrset_reg(dev, base_addr, reg_base + XRFDC_CLK_NETWORK_CTRL1, XRFDC_CLK_NETWORK_CTRL1_USE_PLL_MASK,
				XRFDC_DISABLED);
		sampling_rate /= XRFDC_MILLI;

		if (type == XRFDC_ADC_TILE) {
			data->adc_tile[tile_id].pll_settings.sample_rate = sampling_rate;
			data->adc_tile[tile_id].pll_settings.ref_clk_divider = 0x0U;
			data->adc_tile[tile_id].pll_settings.feedback_divider = 0x0U;
			data->adc_tile[tile_id].pll_settings.output_divider = op_div;
		} else {
			data->dac_tile[tile_id].pll_settings.sample_rate = sampling_rate;
			data->dac_tile[tile_id].pll_settings.ref_clk_divider = 0x0U;
			data->dac_tile[tile_id].pll_settings.feedback_divider = 0x0U;
			data->dac_tile[tile_id].pll_settings.output_divider = op_div;
		}
		sys_write32(pll_fs, reg_base + XRFDC_CTRL_STS_BASE(type, tile_id) + XRFDC_PLL_FS);
	}
	sys_write32(pll_freq, reg_base + XRFDC_CTRL_STS_BASE(type, tile_id) + XRFDC_PLL_FREQ);

	if ((config->ip_type >= XRFDC_GEN3) && (type == XRFDC_ADC_TILE)) {
		base_addr = XRFDC_ADC_TILE_CTRL_STATS_ADDR(tile_id);
		if (data->adc_tile[tile_id].pll_settings.sample_rate >
		    XRFDC_CAL_DIV_CUTOFF_FREQ(xlnx_rfdc_is_high_speed_adc(dev, tile_id))) {
			fg_delay = (sys_read32(reg_base + base_addr + XRFDC_CAL_TMR_MULT_OFFSET) * XRFDC_CAL_AXICLK_MULT);
			xlnx_rfdc_clrset_reg(dev, reg_base + base_addr, XRFDC_CAL_DIV_BYP_OFFSET, XRFDC_CAL_DIV_BYP_MASK,
					XRFDC_DISABLED);
			sys_write32(fg_delay, reg_base + base_addr + XRFDC_CAL_DLY_OFFSET);
		} else {
			xlnx_rfdc_clrset_reg(dev, reg_base + base_addr, XRFDC_CAL_DIV_BYP_OFFSET, XRFDC_CAL_DIV_BYP_MASK,
					XRFDC_CAL_DIV_BYP_MASK);
			sys_write32(0U, base_addr + XRFDC_CAL_DLY_OFFSET);
		}
	}

	/*
	 * Re-start the ADC or DAC tile if tile was shut down in this function
	 */
	if (init_powerup_state != XRFDC_DISABLED) {
		status = xlnx_rfdc_startup(dev, type, tile_id);
		if (status != XRFDC_SUCCESS) {
			status = XRFDC_FAILURE;
			goto return_path;
		}
	}

	if (type == XRFDC_ADC_TILE) {
		data->adc_tile[tile_id].pll_settings.ref_clk_freq = ref_clk_freq;
		data->adc_tile[tile_id].pll_settings.en = pll_en;
	} else {
		data->dac_tile[tile_id].pll_settings.ref_clk_freq = ref_clk_freq;
		data->dac_tile[tile_id].pll_settings.en = pll_en;
	}

	status = XRFDC_SUCCESS;
return_path:
	return status;
}

static uint32_t xlnx_rfdc_check_block_enabled(struct device *dev, uint32_t type, uint32_t tile_id, uint32_t block_id) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uint32_t is_block_avail, status;
    if ((type != XRFDC_ADC_TILE) && (type != XRFDC_DAC_TILE)) {
		status = XRFDC_FAILURE;
		goto return_path;
	}
	if ((tile_id > XRFDC_TILE_ID_MAX) || (block_id > XRFDC_BLOCK_ID_MAX)) {
		status = XRFDC_FAILURE;
		goto return_path;
	}
	if (type == XRFDC_ADC_TILE) {
		is_block_avail = xlnx_rfdc_is_adc_block_en(dev, tile_id, block_id);
	} else {
		is_block_avail = xlnx_rfdc_is_dac_block_en(dev, tile_id, block_id);
	}
	if (is_block_avail == 0U) {
		status = XRFDC_FAILURE;
	} else {
		status = XRFDC_SUCCESS;
	}
return_path:
	return status;
}


static uint32_t xlnx_rfdc_set_dac_vop(const struct device *dev, uint32_t tile_id, uint32_t block_id, uint32_t ua_curr)
{
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    uint32_t status, base_addr, gen1_compatibility_mode, opt_idx, code, link_coupling;
    float ua_curr_int, ua_curr_next;
	uint32_t baseaddr;

	/* Tuned optimization values*/
	uint32_t BldrOPCBias[64] = { 22542, 26637, 27661, 27661, 28686, 28686, 29710, 29711, 30735, 30735, 31760,
				31760, 32784, 32785, 33809, 33809, 34833, 34833, 35857, 36881, 37906, 38930,
				38930, 39954, 40978, 42003, 43027, 43027, 44051, 45075, 46100, 47124, 48148,
				49172, 50196, 51220, 52245, 53269, 53269, 54293, 55317, 56342, 57366, 58390,
				58390, 58390, 59415, 59415, 59415, 59415, 60439, 60439, 60439, 60439, 60439,
				60440, 62489, 62489, 63514, 63514, 63514, 64539, 64539, 64539 };
	uint32_t CSCBldr[64] = { 49152, 49152, 49152, 49152, 49152, 49152, 49152, 49152, 49152, 49152, 49152, 49152, 49152,
			    49152, 49152, 49152, 40960, 40960, 40960, 40960, 40960, 40960, 40960, 40960, 40960, 40960,
			    40960, 40960, 40960, 40960, 40960, 40960, 32768, 32768, 32768, 32768, 32768, 32768, 32768,
			    32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 24576, 24576, 24576, 24576,
			    24576, 24576, 24576, 24576, 24576, 24576, 24576, 24576, 24576, 24576, 24576, 24576 };
	uint32_t CSCBiasProd[64] = { 0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,
				5,  5,  5,  5,  5,  5,  6,  7,  8,  9,  10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16,
				16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 31, 31, 31, 31 };
	uint32_t CSCBiasES1[32] = { 5, 5, 5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  8,  8,  8,  9,
			       9, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 18, 19, 19, 20, 20 };



	if (config->ip_type < XRFDC_GEN3) {
		status = XRFDC_FAILURE;
		LOG_ERR("Requested functionality not available for this IP in %s", __func__);
		goto return_path;
	}

	status = xlnx_rfdc_check_block_enabled(dev, XRFDC_DAC_TILE, tile_id, block_id);
	if (status != XRFDC_SUCCESS) {
		LOG_ERR("DAC %u block %u not available in %s", tile_id, block_id, __func__);
		goto return_path;
	}

	if (ua_curr > XRFDC_MAX_I_UA(config->si_revision)) {
		LOG_ERR("Invalid current selection (too high - %u) for DAC %u block %u in %s",
			  ua_curr, tile_id, block_id, __func__);
		status = XRFDC_FAILURE;
		goto return_path;
	}
	if (ua_curr < XRFDC_MIN_I_UA(config->si_revision)) {
		LOG_ERR("Invalid current selection (too low - %u) for DAC %u block %u in %s",
			  ua_curr, tile_id, block_id, __func__);
		status = XRFDC_FAILURE;
		goto return_path;
	}

	baseaddr = XRFDC_CTRL_STS_BASE(XRFDC_DAC_TILE, tile_id);
	link_coupling = sys_read32(reg_base + XRFDC_CPL_TYPE_OFFSET + baseaddr);
	if (link_coupling != XRFDC_DAC_LINK_COUPLING_AC) {
		status = XRFDC_FAILURE;
		LOG_ERR("Requested functionality not available DC coupled configuration in %s", __func__);
		goto return_path;
	}

	ua_curr_int = (float)ua_curr;

	baseaddr = XRFDC_BLOCK_BASE(XRFDC_DAC_TILE, tile_id, block_id);

	gen1_compatibility_mode = sys_read16(reg_base + baseaddr + XRFDC_ADC_DAC_MC_CFG2_OFFSET) & XRFDC_DAC_MC_CFG2_GEN1_COMP_MASK;
	if (gen1_compatibility_mode == XRFDC_DAC_MC_CFG2_GEN1_COMP_MASK) {
		LOG_ERR("Invalid compatibility mode is set for DAC %u block %u in %s",
			  tile_id, block_id, __func__);
		status = XRFDC_FAILURE;
		goto return_path;
	}

	xlnx_rfdc_clrset_reg(dev, baseaddr + reg_base, XRFDC_DAC_VOP_CTRL_OFFSET,
			(XRFDC_DAC_VOP_CTRL_REG_UPDT_MASK | XRFDC_DAC_VOP_CTRL_TST_BLD_MASK), XRFDC_DISABLED);

	if (config->si_revision == XRFDC_ES1_SI) {
		xlnx_rfdc_clrset_reg(dev, baseaddr + reg_base, XRFDC_ADC_DAC_MC_CFG0_OFFSET, XRFDC_DAC_MC_CFG0_CAS_BLDR_MASK,
				XRFDC_CSCAS_BLDR);
		xlnx_rfdc_clrset_reg(dev, baseaddr + reg_base, XRFDC_ADC_DAC_MC_CFG2_OFFSET,
				(XRFDC_DAC_MC_CFG2_BLDGAIN_MASK | XRFDC_DAC_MC_CFG2_CAS_BIAS_MASK),
				(XRFDC_BLDR_GAIN | XRFDC_OPCAS_BIAS));
	}

	ua_curr_next = 
		((float)((sys_read16(reg_base + baseaddr + XRFDC_DAC_MC_CFG3_OFFSET) & XRFDC_DAC_MC_CFG3_CSGAIN_MASK) >>
			 XRFDC_DAC_MC_CFG3_CSGAIN_SHIFT) *
		 XRFDC_STEP_I_UA(config->si_revision)) +
		(float)XRFDC_MIN_I_UA_INT(config->si_revision);

	while (ua_curr_int != ua_curr_next) {
		if (ua_curr_next < ua_curr_int) {
			ua_curr_next += ua_curr_next / 10;
			if (ua_curr_next > ua_curr_int)
				ua_curr_next = ua_curr_int;
		} else {
			ua_curr_next -= ua_curr_next / 10;
			if (ua_curr_next < ua_curr_int)
				ua_curr_next = ua_curr_int;
		}
		code = (uint32_t)((ua_curr_next - XRFDC_MIN_I_UA_INT(config->si_revision)) /
			     XRFDC_STEP_I_UA(config->si_revision));

		opt_idx = (code & XRFDC_DAC_MC_CFG3_OPT_LUT_MASK(config->si_revision)) >>
			 XRFDC_DAC_MC_CFG3_OPT_LUT_SHIFT(config->si_revision);
		if (config->si_revision == XRFDC_ES1_SI) {
			xlnx_rfdc_clrset_reg(dev, baseaddr + reg_base, XRFDC_DAC_MC_CFG3_OFFSET,
					(XRFDC_DAC_MC_CFG3_CSGAIN_MASK | XRFDC_DAC_MC_CFG3_OPT_MASK),
					((code << XRFDC_DAC_MC_CFG3_CSGAIN_SHIFT) | CSCBiasES1[opt_idx]));
		} else {
			xlnx_rfdc_clrset_reg(dev, baseaddr + reg_base, XRFDC_ADC_DAC_MC_CFG0_OFFSET,
					XRFDC_DAC_MC_CFG0_CAS_BLDR_MASK, CSCBldr[opt_idx]);
			xlnx_rfdc_clrset_reg(dev, baseaddr + reg_base, XRFDC_ADC_DAC_MC_CFG2_OFFSET,
					(XRFDC_DAC_MC_CFG2_BLDGAIN_MASK | XRFDC_DAC_MC_CFG2_CAS_BIAS_MASK),
					(BldrOPCBias[opt_idx] | ((code & XRFDC_DAC_VOP_BLDR_LOW_BITS_MASK)
								<< XRFDC_DAC_MC_CFG3_CSGAIN_SHIFT)));
			xlnx_rfdc_clrset_reg(dev, baseaddr + reg_base, XRFDC_DAC_MC_CFG3_OFFSET,
					(XRFDC_DAC_MC_CFG3_CSGAIN_MASK | XRFDC_DAC_MC_CFG3_OPT_MASK),
					((code << XRFDC_DAC_MC_CFG3_CSGAIN_SHIFT) | CSCBiasProd[opt_idx]));
		}

		xlnx_rfdc_clrset_reg(dev, baseaddr + reg_base, XRFDC_DAC_MC_CFG3_OFFSET, XRFDC_DAC_MC_CFG3_UPDATE_MASK,
				XRFDC_DAC_MC_CFG3_UPDATE_MASK);

		k_usleep(1);
    }

	status = XRFDC_SUCCESS;
return_path:
	return status;
}

static uint32_t xlnx_rfdc_set_tileclk_settings(const struct device *dev, uint32_t type, uint32_t tile_id, 
                    struct xlnx_rfdc_tile_clock_settings *settings) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uint32_t status, base_addr, power_state_mask_reg;
    uint16_t pll_source, network_ctrl_reg, dist_ctrl_reg, pll_ref_div_reg;
    struct xlnx_rfdc_pll_settings pll_settings;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    pll_source = (settings->pll_en == XRFDC_ENABLED) ? XRFDC_INTERNAL_PLL_CLK : XRFDC_EXTERNAL_CLK;
	status = xlnx_rfdc_dynamic_pll_config(dev, type, tile_id, pll_source, settings->ref_clk_freq,
					settings->sample_rate);
	if (status != XRFDC_SUCCESS) {
		LOG_ERR("Could not set up PLL settings for %s %u %s",
			  (type == XRFDC_ADC_TILE) ? "ADC" : "DAC", tile_id, __func__);
		goto return_path;
	}

	if (pll_source == XRFDC_EXTERNAL_CLK) {
		(void)xlnx_rfdc_get_pll_config(dev, type, tile_id, &pll_settings);
		settings->div_factor = pll_settings.output_divider;
	} else {
		settings->div_factor = 1U;
	}

	/* in cases where pll output divder is totally bypassed distribute the RX clock instead */
	if ((pll_source == XRFDC_EXTERNAL_CLK) && (settings->div_factor == 1U) &&
	    (settings->dist_clock == XRFDC_DIST_OUT_OUTDIV)) {
		settings->dist_clock = XRFDC_DIST_OUT_RX;
	}

	dist_ctrl_reg = 0;
	pll_ref_div_reg = 0;
	network_ctrl_reg = 0;
	if ((settings->source_tile == tile_id) && (settings->source_type == type)) {
		if (settings->dist_clock == XRFDC_DIST_OUT_NONE) {
			if (settings->pll_en == XRFDC_DISABLED) {
				pll_ref_div_reg |= XRFDC_PLLREFDIV_INPUT_OFF;
				network_ctrl_reg |= XRFDC_NET_CTRL_CLK_REC_DIST_T1;
				if (settings->div_factor < 2) {
					/*
					T1 from Self
					No PLL
					Do Not Use PLL Output Divider
					Do Not Distribute
					*/
					network_ctrl_reg |= XRFDC_NET_CTRL_CLK_T1_SRC_LOCAL;
					dist_ctrl_reg |= XRFDC_DIST_CTRL_CLK_T1_SRC_LOCAL;
					power_state_mask_reg = XRFDC_HSCOM_PWR_STATS_DIST_EXT_SRC;
				} else {
					/*
					T1 from Self
					No PLL
					Use PLL Output Divider
					Do Not Distribute
					*/
					power_state_mask_reg = XRFDC_HSCOM_PWR_STATS_DIST_EXT_DIV_SRC;
				}
			} else {
				/*
				T1 from Self
				PLL
				Use PLL Output Divider
				Do Not Distribute
				*/
				network_ctrl_reg |= XRFDC_NET_CTRL_CLK_REC_PLL;
				power_state_mask_reg = XRFDC_HSCOM_PWR_STATS_RX_PLL;
			}
		} else {
			if (settings->pll_en == XRFDC_DISABLED) {
				network_ctrl_reg |= XRFDC_NET_CTRL_CLK_REC_DIST_T1;
				pll_ref_div_reg |= XRFDC_PLLREFDIV_INPUT_OFF;
				if (settings->div_factor < 2) {
					/*
					T1 From Distribution (RX back)
					No PLL
					Do Not Use PLL Output Divider
					Send to Distribution
					*/
					pll_ref_div_reg |= XRFDC_PLLREFDIV_INPUT_OFF;
					network_ctrl_reg |= XRFDC_NET_CTRL_CLK_T1_SRC_DIST;
					dist_ctrl_reg |= XRFDC_DIST_CTRL_TO_T1;
					dist_ctrl_reg |= XRFDC_DIST_CTRL_DIST_SRC_LOCAL;
					power_state_mask_reg = XRFDC_HSCOM_PWR_STATS_DIST_EXT_SRC;
				} else if (settings->dist_clock == XRFDC_DIST_OUT_RX) {
					/*
					RX Back From Distribution
					No PLL
					Use PLL Output Divider
					Send to Distribution
					*/
					network_ctrl_reg |= XRFDC_NET_CTRL_CLK_INPUT_DIST;
					dist_ctrl_reg |= XRFDC_DIST_CTRL_TO_PLL_DIV;
					dist_ctrl_reg |= XRFDC_DIST_CTRL_DIST_SRC_LOCAL;
					power_state_mask_reg = XRFDC_HSCOM_PWR_STATS_DIST_EXT_DIV_SRC;
				} else {
					/*
					PLL Output Divider Back From Distribution
					No PLL
					Use PLL Output Divider
					Send to Distribution
					*/
					network_ctrl_reg |= XRFDC_NET_CTRL_CLK_REC_DIST_T1;
					network_ctrl_reg |= XRFDC_NET_CTRL_CLK_T1_SRC_DIST;
					network_ctrl_reg |= XRFDC_NET_CTRL_CLK_INPUT_DIST;
					dist_ctrl_reg |= XRFDC_DIST_CTRL_TO_PLL_DIV;
					dist_ctrl_reg |= XRFDC_DIST_CTRL_DIST_SRC_PLL;
					power_state_mask_reg = XRFDC_HSCOM_PWR_STATS_DIST_EXT_DIV_SRC;
				}

			} else {
				/*
				RX Back From Distribution
				PLL
				Use PLL Output Divider
				Send to Distribution
				*/
				if (settings->dist_clock == XRFDC_DIST_OUT_RX) {
					network_ctrl_reg |= XRFDC_NET_CTRL_CLK_REC_DIST_T1;
					pll_ref_div_reg |= XRFDC_PLLREFDIV_INPUT_DIST;
					dist_ctrl_reg |= XRFDC_DIST_CTRL_TO_PLL_DIV;
					dist_ctrl_reg |= XRFDC_DIST_CTRL_DIST_SRC_LOCAL;
					power_state_mask_reg = XRFDC_HSCOM_PWR_STATS_RX_PLL;
				} else {
					network_ctrl_reg |= XRFDC_NET_CTRL_CLK_T1_SRC_DIST;
					network_ctrl_reg |= XRFDC_NET_CTRL_CLK_REC_PLL;
					pll_ref_div_reg |= XRFDC_PLLOPDIV_INPUT_DIST_LOCAL;
					dist_ctrl_reg |= XRFDC_DIST_CTRL_TO_T1;
					dist_ctrl_reg |= XRFDC_DIST_CTRL_DIST_SRC_PLL;
					power_state_mask_reg = XRFDC_HSCOM_PWR_STATS_RX_PLL;
				}
			}
		}
	} else {
		if (type == XRFDC_ADC_TILE) {
			/* This is needed if distributing a full rate clock from ADC 0/1 to ADC 2/3 */
			if ((tile_id > XRFDC_TILE_ID1) && (settings->source_tile < XRFDC_TILE_ID2) &&
			    (settings->source_type == XRFDC_ADC_TILE)) {
				dist_ctrl_reg |= XRFDC_CLK_DISTR_MUX5A_SRC_RX;
			}
		}
		if (settings->pll_en == XRFDC_DISABLED) {
			pll_ref_div_reg |= XRFDC_PLLREFDIV_INPUT_OFF;
			if (settings->div_factor > 1) {
				/*
				source From Distribution
				No PLL
				Use PLL Output Divider
				Do Not Distribute
				*/
				network_ctrl_reg |= XRFDC_NET_CTRL_CLK_INPUT_DIST;
				dist_ctrl_reg |= XRFDC_DIST_CTRL_TO_PLL_DIV;
				power_state_mask_reg = XRFDC_HSCOM_PWR_STATS_DIST_EXT_DIV;
				if (settings->dist_clock == XRFDC_DIST_OUT_OUTDIV) {
					dist_ctrl_reg |= XRFDC_DIST_CTRL_DIST_SRC_PLL;
				}
			} else {
				/*
				source From Distribution
				No PLL
				Do Not Use PLL Output Divider
				Do Not Distribute
				*/
				network_ctrl_reg |= XRFDC_NET_CTRL_CLK_T1_SRC_DIST;
				dist_ctrl_reg |= XRFDC_DIST_CTRL_TO_T1;
				power_state_mask_reg = ((type == XRFDC_ADC_TILE) ? XRFDC_HSCOM_PWR_STATS_DIST_EXT_DIV :
										XRFDC_HSCOM_PWR_STATS_DIST_EXT);
			}
		} else {
			/*
			source From Distribution
			PLL
			Use PLL Output Divider
			Do Not Distribute
			*/
			pll_ref_div_reg |= XRFDC_PLLREFDIV_INPUT_DIST;
			dist_ctrl_reg |= XRFDC_DIST_CTRL_TO_PLL_DIV;
			power_state_mask_reg = XRFDC_HSCOM_PWR_STATS_DIST_PLL;
		}
	}

	/*Write to Registers*/
	if (type == XRFDC_ADC_TILE) {
		base_addr = XRFDC_ADC_TILE_DRP_ADDR(tile_id);
	} else {
		base_addr = XRFDC_DAC_TILE_DRP_ADDR(tile_id);
	}
	base_addr += XRFDC_HSCOM_ADDR;
	xlnx_rfdc_clrset_reg(dev, reg_base + base_addr, XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK_ALT, dist_ctrl_reg);
	xlnx_rfdc_clrset_reg(dev, reg_base + base_addr, XRFDC_CLK_NETWORK_CTRL1, XRFDC_HSCOM_NETWORK_CTRL1_MASK, network_ctrl_reg);
	xlnx_rfdc_clrset_reg(dev, reg_base + base_addr, XRFDC_PLL_REFDIV, XRFDC_PLL_REFDIV_MASK, pll_ref_div_reg);
    sys_write16(power_state_mask_reg, reg_base + base_addr + XRFDC_HSCOM_PWR_STATE_OFFSET);
	status = XRFDC_SUCCESS;
return_path:
	return status;
}

static int xlnx_rfdc_set_clock_dist(const struct device *dev) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    uint8_t delay_left, delay_right, delay, delay_out_src_left, delay_out_src_right,
    fb_input_right = 0, fb_input_left = 0, dac_edge_tile, tile_layout, pkg_tile_id, first_tile;
    int8_t clk_det_itr;
    uint32_t tile, type, status, status_non_blocking;
    uint16_t reg, srcreg, clk_detect_mask_old, clk_detect_reg;
    struct xlnx_rfdc_tile_clock_settings settings;
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    memset(&data->info, 0, sizeof(data->info));
    data->info.source = xlnx_rfdc_type_tile2dist_tile(dev, data->source_type, data->source_tile_id);
    data->info.upper_bound = xlnx_rfdc_type_tile2dist_tile(dev, data->edge_types[0], data->edge_tile_ids[0]);
    data->info.lower_bound = xlnx_rfdc_type_tile2dist_tile(dev, data->edge_types[1], data->edge_tile_ids[1]);

    if ( data-> info.upper_bound < data-> info.lower_bound) {
		data-> info.lower_bound ^= data-> info.upper_bound;
		data-> info.upper_bound ^= data-> info.lower_bound;
		data-> info.lower_bound ^= data-> info.upper_bound;
	}
    status = xlnx_rfdc_chk_clk_dist_valid(dev);
    if (status != XRFDC_SUCCESS) {
        LOG_ERR("Invalid Distribution in %s", __func__);
		goto return_path;
    }
    tile_layout = xlnx_rfdc_get_tile_layout(dev);
    clk_detect_mask_old = sys_read16(reg_base + XRFDC_CTRL_STS_BASE(data->source_type,
							   data->source_tile_id) + XRFDC_CLOCK_DETECT_OFFSET) & XRFDC_CLOCK_DETECT_SRC_MASK;

	clk_detect_reg = (XRFDC_CLOCK_DETECT_CLK << ((XRFDC_CLK_DST_TILE_224 - data->info.source)
						   << XRFDC_CLOCK_DETECT_DST_SHIFT));

	first_tile = (tile_layout == XRFDC_3ADC_2DAC_TILES) ? XRFDC_CLK_DST_TILE_228 : XRFDC_CLK_DST_TILE_231;
	for (pkg_tile_id = first_tile; pkg_tile_id < data->info.lower_bound; pkg_tile_id++) {
        reg = sys_read16(reg_base + XRFDC_CTRL_STS_BASE(data->source_type, data->source_tile_id) + XRFDC_CLOCK_DETECT_OFFSET);
        reg = (reg & ~(XRFDC_DISABLED)) | ((clk_detect_mask_old | clk_detect_reg) & XRFDC_DISABLED);
        sys_write16(reg, reg_base + XRFDC_CTRL_STS_BASE(data->source_type, data->source_tile_id) + XRFDC_CLOCK_DETECT_OFFSET);
    }
	for (pkg_tile_id = data->info.upper_bound; pkg_tile_id <= XRFDC_CLK_DST_TILE_224; pkg_tile_id++) {
        reg = sys_read16(reg_base + XRFDC_CTRL_STS_BASE(data->source_type, data->source_tile_id) + XRFDC_CLOCK_DETECT_OFFSET);
        reg = (reg & ~(XRFDC_PWR_STATE_MASK)) | ((clk_detect_mask_old | clk_detect_reg) & XRFDC_PWR_STATE_MASK);
        sys_write16(reg, reg_base + XRFDC_CTRL_STS_BASE(data->source_type, data->source_tile_id) + XRFDC_CLOCK_DETECT_OFFSET);
	}

	for (pkg_tile_id = data->info.lower_bound;
	     pkg_tile_id <= data->info.upper_bound; pkg_tile_id++) {
		xlnx_rfdc_dist_tile2type_tile(dev, pkg_tile_id, &type, &tile);
        reg = sys_read16(reg_base + XRFDC_CTRL_STS_BASE(type, tile) + XRFDC_RESTART_STATE_OFFSET);
        reg = (reg & ~(XRFDC_DISABLED)) | ((XRFDC_SM_STATE1 << XRFDC_RSR_START_SHIFT) | XRFDC_SM_STATE1);
        sys_write16(reg, reg_base + XRFDC_CTRL_STS_BASE(type, tile) + XRFDC_RESTART_STATE_OFFSET);
		/* Trigger restart */
		sys_write32(XRFDC_RESTART_MASK, reg_base + XRFDC_CTRL_STS_BASE(type, tile) + XRFDC_RESTART_OFFSET);
		status |= xlnx_rfdc_wait_for_state(dev, type, tile, XRFDC_SM_STATE1);
		if (status != XRFDC_SUCCESS) {
			status = XRFDC_FAILURE;
			goto return_path;
		}
	}

	dac_edge_tile = (tile_layout == XRFDC_3ADC_2DAC_TILES) ? XRFDC_CLK_DST_TILE_227 : XRFDC_CLK_DST_TILE_228;
	status_non_blocking = XRFDC_SUCCESS;
	delay_left = (-data->info.lower_bound + data->info.source);
	delay_right = (data->info.upper_bound - data->info.source);
	delay_out_src_left = 0U;
	delay_out_src_right = 0U;
	data->info.max_delay = XRFDC_MAX_DLY_INIT;
	data->info.min_delay = XRFDC_MIN_DLY_INIT;
	data->info.is_delay_balance = 0U;
	if ((delay_left == 0U) && (delay_right == 0U)) { /*self contained*/
		srcreg = XRFDC_CLK_DISTR_OFF;
	} else {
		srcreg = XRFDC_CLK_DISTR_MUX9_SRC_INT;
		if (delay_left == 0U) {
			srcreg |= XRFDC_CLK_DISTR_MUX8_SRC_NTH;
		} else {
			srcreg |= XRFDC_CLK_DISTR_MUX8_SRC_INT;
		}
		if (((data->info.source == dac_edge_tile) ||
		     (data->info.source == XRFDC_CLK_DST_TILE_224)) &&
		    ((delay_left > 1U) || (delay_right > 0U))) {
			srcreg |= XRFDC_CLK_DISTR_MUX4A_SRC_INT | XRFDC_CLK_DISTR_MUX6_SRC_NTH |
				  XRFDC_CLK_DISTR_MUX7_SRC_INT;
			fb_input_right = 1U;
			fb_input_left = 0U;
			delay_out_src_right = XRFDC_DLY_UNIT;
		} else {
			if ((delay_left > 1U) || ((delay_left == 1U) && (delay_right == 1U))) {
				srcreg |= XRFDC_CLK_DISTR_MUX4A_SRC_STH | XRFDC_CLK_DISTR_MUX6_SRC_NTH |
					  XRFDC_CLK_DISTR_MUX7_SRC_INT;
				delay_out_src_right = XRFDC_DLY_UNIT;
				fb_input_right = 0U;
				fb_input_left = 1U;
			} else {
				fb_input_right = (delay_left == 0U) ? 0U : 1U;
				fb_input_left = 0U;
				if ((delay_right > 1U) &&
				    (data->info.source != XRFDC_CLK_DST_TILE_229)) {
					fb_input_right = (delay_left == 0U) ? 0U : 1U;
					srcreg |= XRFDC_CLK_DISTR_MUX4A_SRC_INT;
					srcreg |= XRFDC_CLK_DISTR_MUX7_SRC_STH;
					delay_out_src_left = XRFDC_DLY_UNIT;
				} else {
					fb_input_right = 1U;
					if (delay_left == 0U) {
						srcreg |= XRFDC_CLK_DISTR_MUX4A_SRC_STH;
						srcreg |= XRFDC_CLK_DISTR_MUX7_SRC_OFF;
					} else {
						srcreg |= XRFDC_CLK_DISTR_MUX4A_SRC_INT;
						srcreg |= XRFDC_CLK_DISTR_MUX7_SRC_INT;
					}
				}
				if (delay_right == 0U) {
					srcreg |= XRFDC_CLK_DISTR_MUX6_SRC_OFF;
				} else {
					srcreg |= XRFDC_CLK_DISTR_MUX6_SRC_INT;
				}
			}
		}
	}

	if (srcreg == XRFDC_CLK_DISTR_OFF) {
		settings.delay = 0U;
	} else if (fb_input_left == 0U) {
		settings.delay = delay_out_src_left + XRFDC_DLY_UNIT;
	} else {
		settings.delay = delay_out_src_right + XRFDC_DLY_UNIT;
	}

	data->info.max_delay = XRFDC_MAX(data->info.max_delay, (settings.delay));
	data->info.min_delay = XRFDC_MIN(data->info.min_delay, (settings.delay));

	xlnx_rfdc_clrset_reg(dev, reg_base +
			(XRFDC_DRP_BASE(data->source_type, data->source_tile_id) +
			 XRFDC_HSCOM_ADDR),
			XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK, srcreg);
	xlnx_rfdc_clrset_reg(dev, reg_base +
			XRFDC_CTRL_STS_BASE(data->source_type, data->source_tile_id),
			XRFDC_CLOCK_DETECT_OFFSET, XRFDC_CLOCK_DETECT_MASK, clk_detect_reg);
	settings.source_type = data->source_type;
	settings.source_tile = data->source_tile_id;
	settings.dist_clock = data->dist_clock;
	settings.ref_clk_freq = data->dist_ref_clk_freq;
	settings.sample_rate =
		(data
			 ->sample_rates[data->source_type][data->source_tile_id]);
	settings.pll_en = (settings.sample_rate > data->dist_ref_clk_freq);
	status_non_blocking |= xlnx_rfdc_set_tileclk_settings(dev, data->source_type,
						      data->source_tile_id, &settings);
	data->info
		.clk_settings[data->source_type][data->source_tile_id] = settings;
	if (data->dist_clock == XRFDC_DIST_OUT_OUTDIV) {
		settings.ref_clk_freq = settings.sample_rate;
		settings.pll_en = XRFDC_DISABLED;
	} else {
		settings.ref_clk_freq = data->dist_ref_clk_freq;
	}
	settings.dist_clock = XRFDC_DIST_OUT_NONE;
	/*Leftmost tile*/
	if (delay_left) {
		settings.delay = delay_out_src_left + (delay_left << 1U);
		data->info.max_delay =
			XRFDC_MAX(data->info.max_delay, (settings.delay));
		data->info.min_delay =
			XRFDC_MIN(data->info.min_delay, (settings.delay));
		reg = XRFDC_CLK_DISTR_MUX6_SRC_OFF | XRFDC_CLK_DISTR_MUX8_SRC_INT | XRFDC_CLK_DISTR_MUX9_SRC_INT;

		if ((data->info.source != dac_edge_tile) && (delay_left == 1U) && (delay_right == 1U)) {
			reg |= XRFDC_CLK_DISTR_MUX4A_SRC_INT | XRFDC_CLK_DISTR_MUX7_SRC_STH;

		} else {
			reg |= XRFDC_CLK_DISTR_MUX4A_SRC_STH | XRFDC_CLK_DISTR_MUX7_SRC_OFF;
		}
		/* setup clk detect register */
		clk_detect_reg =
			(XRFDC_CLOCK_DETECT_BOTH << ((XRFDC_CLK_DST_TILE_224 - data->info.source)
						     << XRFDC_CLOCK_DETECT_DST_SHIFT));
		for (clk_det_itr = delay_left - 1; clk_det_itr > 0; clk_det_itr--) {
			clk_detect_reg |=
				(XRFDC_CLOCK_DETECT_DIST
				 << ((XRFDC_CLK_DST_TILE_224 - (data->info.source - clk_det_itr))
				     << XRFDC_CLOCK_DETECT_DST_SHIFT));
		}

		xlnx_rfdc_dist_tile2type_tile(dev, (data->info.source - delay_left), &type, &tile);
		xlnx_rfdc_clrset_reg(dev, reg_base + (XRFDC_DRP_BASE(type, tile) + XRFDC_HSCOM_ADDR),
				XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK, reg);
		xlnx_rfdc_clrset_reg(dev, reg_base + XRFDC_CTRL_STS_BASE(type, tile), XRFDC_CLOCK_DETECT_OFFSET,
				XRFDC_CLOCK_DETECT_MASK, clk_detect_reg);
		settings.sample_rate = (data->sample_rates[type][tile]);
		status_non_blocking |= xlnx_rfdc_set_tileclk_settings(dev, type, tile, &settings);
		data->info.clk_settings[type][tile] = settings;
	}
	/*Rest of tiles left of data->info.source*/
	for (delay = 1U; delay < delay_left; delay++) {
		reg = XRFDC_CLK_DISTR_MUX6_SRC_OFF | XRFDC_CLK_DISTR_MUX7_SRC_STH | XRFDC_CLK_DISTR_MUX8_SRC_INT |
		      XRFDC_CLK_DISTR_MUX9_SRC_INT;
		if (fb_input_left == 0U) {
			reg |= XRFDC_CLK_DISTR_MUX4A_SRC_STH;
		} else {
			reg |= XRFDC_CLK_DISTR_MUX4A_SRC_INT;
		}
		settings.delay = delay_out_src_left + ((delay + fb_input_left) << 1U);
		data->info.max_delay =
			XRFDC_MAX(data->info.max_delay, (settings.delay));
		data->info.min_delay =
			XRFDC_MIN(data->info.min_delay, (settings.delay));
		fb_input_left = !fb_input_left;

		/* setup clk detect register */
		clk_detect_reg =
			(XRFDC_CLOCK_DETECT_BOTH << ((XRFDC_CLK_DST_TILE_224 - data->info.source)
						     << XRFDC_CLOCK_DETECT_DST_SHIFT));
		for (clk_det_itr = delay - 1; clk_det_itr > 0; clk_det_itr--) {
			clk_detect_reg |=
				(XRFDC_CLOCK_DETECT_DIST
				 << ((XRFDC_CLK_DST_TILE_224 - (data->info.source - clk_det_itr))
				     << XRFDC_CLOCK_DETECT_DST_SHIFT));
		}
        xlnx_rfdc_dist_tile2type_tile(dev, (data->info.source - delay), &type, &tile);
		xlnx_rfdc_clrset_reg(dev, reg_base + (XRFDC_DRP_BASE(type, tile) + XRFDC_HSCOM_ADDR),
				XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK, reg);
		xlnx_rfdc_clrset_reg(dev, reg_base + XRFDC_CTRL_STS_BASE(type, tile), XRFDC_CLOCK_DETECT_OFFSET,
				XRFDC_CLOCK_DETECT_MASK, clk_detect_reg);
		settings.sample_rate = (data->sample_rates[type][tile]);
		status_non_blocking |= xlnx_rfdc_set_tileclk_settings(dev, type, tile, &settings);
		data->info.clk_settings[type][tile] = settings;
	}

	/*tiles to right*/
	reg = srcreg;
	for (delay = 1U; delay < delay_right; delay++) {
		xlnx_rfdc_dist_tile2type_tile(dev, (data->info.source + delay), &type, &tile);
		settings.sample_rate = (data->sample_rates[type][tile]);
		if (type == XRFDC_ADC_TILE) {
			if ((tile == ((tile_layout == XRFDC_3ADC_2DAC_TILES) ? XRFDC_TILE_ID2 : XRFDC_TILE_ID3)) &&
			    (settings.pll_en != XRFDC_ENABLED) && (settings.ref_clk_freq != settings.sample_rate)) {
				settings.dist_clock = XRFDC_DIST_OUT_OUTDIV;
			}
		}
		if (((reg & XRFDC_CLK_DISTR_MUX4A_SRC_INT) != XRFDC_CLK_DISTR_MUX4A_SRC_INT) ||
		    ((reg & XRFDC_CLK_DISTR_MUX7_SRC_STH) == XRFDC_CLK_DISTR_MUX7_SRC_STH)) {
			reg = 0U;
		} else {
			reg = XRFDC_CLK_DISTR_MUX8_SRC_INT;
		}
		if (settings.dist_clock == XRFDC_DIST_OUT_OUTDIV) {
			reg |= XRFDC_CLK_DISTR_MUX4A_SRC_INT | XRFDC_CLK_DISTR_MUX6_SRC_INT |
			       XRFDC_CLK_DISTR_MUX8_SRC_INT;
			fb_input_right = 1U;
			settings.delay = delay_out_src_right + (delay << 1U);
		} else if (((delay + data->info.source) == dac_edge_tile) ||
			   (fb_input_right == 0U)) {
			fb_input_right = 0U;
			reg |= XRFDC_CLK_DISTR_MUX4A_SRC_INT | XRFDC_CLK_DISTR_MUX6_SRC_NTH;
			settings.delay = delay_out_src_right + (delay << 1U);
		} else {
			reg |= XRFDC_CLK_DISTR_MUX4A_SRC_STH | XRFDC_CLK_DISTR_MUX6_SRC_NTH;
			settings.delay = delay_out_src_right + ((delay + 1U) << 1U);
		}
		data->info.max_delay =
			XRFDC_MAX(data->info.max_delay, (settings.delay));
		data->info.min_delay =
			XRFDC_MIN(data->info.min_delay, (settings.delay));
		reg |= XRFDC_CLK_DISTR_MUX7_SRC_OFF | XRFDC_CLK_DISTR_MUX8_SRC_NTH | XRFDC_CLK_DISTR_MUX9_SRC_NTH;

		fb_input_right = !fb_input_right;
		/* setup clk detect register */
		clk_detect_reg =
			(XRFDC_CLOCK_DETECT_BOTH << ((XRFDC_CLK_DST_TILE_224 - data->info.source)
						     << XRFDC_CLOCK_DETECT_DST_SHIFT));
		for (clk_det_itr = delay - 1; clk_det_itr > 0; clk_det_itr--) {
			clk_detect_reg |=
				(XRFDC_CLOCK_DETECT_DIST
				 << ((XRFDC_CLK_DST_TILE_224 - (data->info.source + clk_det_itr))
				     << XRFDC_CLOCK_DETECT_DST_SHIFT));
		}
		xlnx_rfdc_clrset_reg(dev, reg_base + (XRFDC_DRP_BASE(type, tile) + XRFDC_HSCOM_ADDR),
				XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK, reg);
		xlnx_rfdc_clrset_reg(dev, reg_base + XRFDC_CTRL_STS_BASE(type, tile), XRFDC_CLOCK_DETECT_OFFSET,
				XRFDC_CLOCK_DETECT_MASK, clk_detect_reg);
		status_non_blocking |= xlnx_rfdc_set_tileclk_settings(dev, type, tile, &settings);
		data->info.clk_settings[type][tile] = settings;
		if (settings.dist_clock == XRFDC_DIST_OUT_OUTDIV) {
			settings.ref_clk_freq /= settings.div_factor;
			settings.dist_clock = XRFDC_DIST_OUT_NONE;
		}
	}

	/*Rightmost tile*/
	if (delay_right) {
		if ((reg & XRFDC_CLK_DISTR_MUX4A_SRC_INT) != XRFDC_CLK_DISTR_MUX4A_SRC_INT) {
			reg = 0U;
		} else {
			reg = XRFDC_CLK_DISTR_MUX8_SRC_INT;
		}
		reg |= XRFDC_CLK_DISTR_MUX4A_SRC_INT | XRFDC_CLK_DISTR_MUX6_SRC_OFF | XRFDC_CLK_DISTR_MUX7_SRC_OFF |
		       XRFDC_CLK_DISTR_MUX9_SRC_NTH;
		settings.delay = delay_out_src_right + (delay_right << 1U);
		data->info.max_delay =
			XRFDC_MAX(data->info.max_delay, (settings.delay));
		data->info.min_delay =
			XRFDC_MIN(data->info.min_delay, (settings.delay));

		/* setup clk detect register */
		clk_detect_reg =
			(XRFDC_CLOCK_DETECT_BOTH << ((XRFDC_CLK_DST_TILE_224 - data->info.source)
						     << XRFDC_CLOCK_DETECT_DST_SHIFT));
		for (clk_det_itr = delay_right - 1; clk_det_itr > 0; clk_det_itr--) {
			clk_detect_reg |=
				(XRFDC_CLOCK_DETECT_DIST
				 << ((XRFDC_CLK_DST_TILE_224 - (data->info.source + clk_det_itr))
				     << XRFDC_CLOCK_DETECT_DST_SHIFT));
		}

		xlnx_rfdc_dist_tile2type_tile(dev, (data->info.source + delay_right), &type, &tile);
		xlnx_rfdc_clrset_reg(dev, reg_base + (XRFDC_DRP_BASE(type, tile) + XRFDC_HSCOM_ADDR),
				XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK, reg);
		xlnx_rfdc_clrset_reg(dev, reg_base + XRFDC_CTRL_STS_BASE(type, tile), XRFDC_CLOCK_DETECT_OFFSET,
				XRFDC_CLOCK_DETECT_MASK, clk_detect_reg);
		settings.sample_rate = (data->sample_rates[type][tile]);
		status_non_blocking |= xlnx_rfdc_set_tileclk_settings(dev, type, tile, &settings);
		data->info.clk_settings[type][tile] = settings;
	}
	data->info.is_delay_balance =
		(data->info.max_delay == data->info.min_delay) ? 1U : 0U;

	/*start tiles*/
	if (data->shutdown_mode == XRFDC_DISABLED) {
		status = xlnx_rfdc_startup_dist(dev);
	}
	status |= status_non_blocking;
return_path:
	return status;
}

static int xlnx_rfdc_shutdown(const struct device *dev) {
	const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
	uint8_t tile_layout, pkg_tile_id, first_tile;
	uint32_t tile, type, status;
	data->info.source = xlnx_rfdc_type_tile2dist_tile(dev, data->source_type, data->source_tile_id);
    data->info.upper_bound = xlnx_rfdc_type_tile2dist_tile(dev, data->edge_types[0], data->edge_tile_ids[0]);
    data->info.lower_bound = xlnx_rfdc_type_tile2dist_tile(dev, data->edge_types[1], data->edge_tile_ids[1]);
	tile_layout = xlnx_rfdc_get_tile_layout(dev);
	first_tile = (tile_layout == XRFDC_3ADC_2DAC_TILES) ? XRFDC_CLK_DST_TILE_228 : XRFDC_CLK_DST_TILE_231;
	for (pkg_tile_id = data->info.lower_bound;
	     pkg_tile_id <= data->info.upper_bound; pkg_tile_id++) {
		xlnx_rfdc_dist_tile2type_tile(dev, pkg_tile_id, &type, &tile);
        xlnx_rfdc_restart_ipsm(dev, type, tile, XRFDC_SM_STATE1, XRFDC_SM_STATE1);
		if (status != XRFDC_SUCCESS) {
			status = XRFDC_FAILURE;
			goto return_path;
		}
	}
return_path:
	return status;
}


static int xlnx_rfdc_dac_power_off(const struct device *dev) {
	const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
	uint32_t status = 0;
	LOG_INF("dac power off");
	data->source_tile_id = XRFDC_TILE_ID2;
	data->source_type = XRFDC_DAC_TILE;
	data->edge_tile_ids[0] = XRFDC_TILE_ID0;
	data->edge_tile_ids[1] = XRFDC_TILE_ID3;
	data->edge_types[0] = XRFDC_DAC_TILE;
	data->edge_types[1] = XRFDC_DAC_TILE;
    xlnx_rfdc_shutdown(dev);
	return status;
}

static int xlnx_rfdc_adc_power_off(const struct device *dev) {
	const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
	uint32_t status = 0;
	LOG_INF("adc power off");
	data->source_tile_id = XRFDC_TILE_ID1;
	data->source_type = XRFDC_ADC_TILE;
	data->edge_tile_ids[0] = XRFDC_TILE_ID0;
	data->edge_tile_ids[1] = XRFDC_TILE_ID3;
	data->edge_types[0] = XRFDC_ADC_TILE;
	data->edge_types[1] = XRFDC_ADC_TILE;
    xlnx_rfdc_shutdown(dev);
	return status;
}

static int xlnx_rfdc_dac_power_on(const struct device *dev) {
	const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
	int status = 0;
	LOG_INF("dac power on");

	data->source_tile_id = XRFDC_TILE_ID2;
	data->source_type = XRFDC_DAC_TILE;
	data->edge_tile_ids[0] = XRFDC_TILE_ID0;
	data->edge_tile_ids[1] = XRFDC_TILE_ID3;
	data->edge_types[0] = XRFDC_DAC_TILE;
	data->edge_types[1] = XRFDC_DAC_TILE;
	data->dist_ref_clk_freq = 7776.00;
	data->dist_clock = XRFDC_DIST_OUT_RX;
	data->sample_rates[1][0] = 7776.00;
	data->sample_rates[1][1] = 7776.00;
	data->sample_rates[1][2] = 7776.00;
    data->sample_rates[1][3] = 7776.00;
    return xlnx_rfdc_set_clock_dist(dev);
}

static int xlnx_rfdc_adc_power_on(const struct device *dev) {
	const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
	int status = 0;
	LOG_INF("adc power on");

	data->source_tile_id = XRFDC_TILE_ID1;
	data->source_type = XRFDC_ADC_TILE;
	data->edge_tile_ids[0] = XRFDC_TILE_ID0;
	data->edge_tile_ids[1] = XRFDC_TILE_ID3;
	data->edge_types[0] = XRFDC_ADC_TILE;
	data->edge_types[1] = XRFDC_ADC_TILE;
	data->dist_ref_clk_freq = 3888.00;
	data->dist_clock = XRFDC_DIST_OUT_RX;
	data->sample_rates[0][0] = 3888.00;
	data->sample_rates[0][1] = 3888.00;
	data->sample_rates[0][2] = 3888.00;
    data->sample_rates[0][3] = 3888.00;
    return xlnx_rfdc_set_clock_dist(dev);
}

static void xlnx_rfdc_init(const struct device *dev) {
    const struct xlnx_rfdc_dev_config *config = dev->config;
    struct xlnx_rfdc_dev_data *data = dev->data;
    DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
    uintptr_t reg_base = DEVICE_MMIO_GET(dev);
    // const struct device *en_gpio = DEVICE_DT_GET(GPIO_NODE);
	// if (!device_is_ready(en_gpio)) {
	// 	LOG_ERR("No gpio device found");
	// }
	// int ret = gpio_pin_configure(en_gpio, GPIO_PIN, GPIO_OUTPUT_ACTIVE);
	// if (ret != 0) {
	// 	LOG_ERR("GPIO config error");
	// }
	// ret = gpio_pin_set(en_gpio, GPIO_PIN, 1);
	// if (ret != 0) {
	// 	LOG_ERR("GPIO set error");
	// }
    // k_msleep(100);
    // sys_write16(1, reg_base + 0x04);
    // xlnx_rfdc_dac_init(dev);
    // xlnx_rfdc_adc_init(dev);
    // LOG_INF("Xilinx RF Data Converter init");
    // data->source_tile_id = XRFDC_TILE_ID2;
	// data->source_type = XRFDC_DAC_TILE;
	// data->edge_tile_ids[0] = XRFDC_TILE_ID0;
	// data->edge_tile_ids[1] = XRFDC_TILE_ID3;
	// data->edge_types[0] = XRFDC_DAC_TILE;
	// data->edge_types[1] = XRFDC_DAC_TILE;
	// data->dist_ref_clk_freq = 7776.00;
	// data->dist_clock = XRFDC_DIST_OUT_RX;
	// data->sample_rates[1][0] = 7776.00;
	// data->sample_rates[1][1] = 7776.00;
	// data->sample_rates[1][2] = 7776.00;
    // data->sample_rates[1][3] = 7776.00;
    // xlnx_rfdc_set_clock_dist(dev);
    // data->source_tile_id = XRFDC_TILE_ID1;
	// data->source_type = XRFDC_ADC_TILE;
	// data->edge_tile_ids[0] = XRFDC_TILE_ID0;
	// data->edge_tile_ids[1] = XRFDC_TILE_ID3;
	// data->edge_types[0] = XRFDC_ADC_TILE;
	// data->edge_types[1] = XRFDC_ADC_TILE;
	// data->dist_ref_clk_freq = 3888.00;
	// data->dist_clock = XRFDC_DIST_OUT_RX;
	// data->sample_rates[0][0] = 3888.00;
	// data->sample_rates[0][1] = 3888.00;
	// data->sample_rates[0][2] = 3888.00;
    // data->sample_rates[0][3] = 3888.00;
    // xlnx_rfdc_set_clock_dist(dev);
}

static const struct rfdc_driver_api xlnx_rfdc_driver_api = {
    .dac_power_on = xlnx_rfdc_dac_power_on,
	.adc_power_on = xlnx_rfdc_adc_power_on,
    .dac_power_off = xlnx_rfdc_dac_power_off,
	.adc_power_off = xlnx_rfdc_adc_power_off,
    .set_vop = xlnx_rfdc_set_dac_vop,
};


#define XLNX_RFDC_DEV_INIT(port) \
    static struct xlnx_rfdc_dev_data xlnx_rfdc_dev_data_##port; \
    static struct xlnx_rfdc_dev_config xlnx_rfdc_dev_cfg_##port = { \
        DEVICE_MMIO_ROM_INIT(DT_DRV_INST(port)), \
        XPAR_USP_RF_DATA_CONVERTER_0_DEVICE_ID,\
        XPAR_USP_RF_DATA_CONVERTER_0_HIGH_SPEED_ADC,\
        XPAR_USP_RF_DATA_CONVERTER_0_SYSREF_MASTER,\
        XPAR_USP_RF_DATA_CONVERTER_0_SYSREF_MASTER,\
        XPAR_USP_RF_DATA_CONVERTER_0_SYSREF_SOURCE,\
        XPAR_USP_RF_DATA_CONVERTER_0_SYSREF_SOURCE,\
        XPAR_USP_RF_DATA_CONVERTER_0_IP_TYPE,\
        XPAR_USP_RF_DATA_CONVERTER_0_SILICON_REVISION,\
        {\
               {\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC0_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC0_PLL_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC0_SAMPLING_RATE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC0_REFCLK_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC0_FABRIC_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC0_FBDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC0_OUTDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC0_REFCLK_DIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC0_BAND,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC0_FS_MAX,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC0_SLICES,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC0_LINK_COUPLING,\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE00_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL00,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE00,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE00,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE01_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL01,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE01,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE01,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE02_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL02,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE02,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE02,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE03_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL03,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE03,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE03,\
                       },\
                   },\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE00,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH00,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE00,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO00_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER00_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE00,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE01,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH01,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE01,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO01_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER01_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE01,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE02,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH02,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE02,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO02_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER02_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE02,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE03,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH03,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE03,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO03_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER03_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE03,\
                       },\
                   },\
               },\
               {\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC1_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC1_PLL_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC1_SAMPLING_RATE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC1_REFCLK_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC1_FABRIC_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC1_FBDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC1_OUTDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC1_REFCLK_DIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC1_BAND,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC1_FS_MAX,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC1_SLICES,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC1_LINK_COUPLING,\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE10_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL10,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE10,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE10,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE11_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL11,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE11,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE11,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE12_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL12,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE12,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE12,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE13_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL13,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE13,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE13,\
                       },\
                   },\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE10,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH10,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE10,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO10_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER10_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE10,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE11,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH11,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE11,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO11_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER11_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE11,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE12,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH12,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE12,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO12_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER12_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE12,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE13,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH13,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE13,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO13_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER13_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE13,\
                       },\
                   },\
               },\
               {\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC2_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC2_PLL_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC2_SAMPLING_RATE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC2_REFCLK_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC2_FABRIC_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC2_FBDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC2_OUTDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC2_REFCLK_DIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC2_BAND,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC2_FS_MAX,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC2_SLICES,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC2_LINK_COUPLING,\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE20_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL20,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE20,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE20,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE21_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL21,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE21,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE21,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE22_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL22,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE22,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE22,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE23_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL23,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE23,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE23,\
                       },\
                   },\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE20,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH20,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE20,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO20_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER20_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE20,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE21,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH21,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE21,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO21_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER21_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE21,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE22,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH22,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE22,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO22_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER22_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE22,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE23,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH23,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE23,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO23_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER23_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE23,\
                       },\
                   },\
               },\
               {\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC3_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC3_PLL_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC3_SAMPLING_RATE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC3_REFCLK_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC3_FABRIC_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC3_FBDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC3_OUTDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC3_REFCLK_DIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC3_BAND,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC3_FS_MAX,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC3_SLICES,\
                   XPAR_USP_RF_DATA_CONVERTER_0_DAC3_LINK_COUPLING,\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE30_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL30,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE30,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE30,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE31_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL31,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE31,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE31,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE32_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL32,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE32,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE32,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_SLICE33_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INVSINC_CTRL33,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_MODE33,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DECODER_MODE33,\
                       },\
                   },\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE30,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH30,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE30,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO30_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER30_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE30,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE31,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH31,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE31,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO31_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER31_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE31,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE32,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH32,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE32,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO32_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER32_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE32,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_TYPE33,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_DATA_WIDTH33,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_INTERPOLATION_MODE33,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_FIFO33_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_ADDER33_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_DAC_MIXER_TYPE33,\
                       },\
                   },\
               },\
        },\
        {\
               {\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC0_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC0_PLL_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC0_SAMPLING_RATE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC0_REFCLK_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC0_FABRIC_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC0_FBDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC0_OUTDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC0_REFCLK_DIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC0_BAND,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC0_FS_MAX,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC0_SLICES,\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE00_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE00,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE01_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE01,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE02_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE02,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE03_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE03,\
                       },\
                   },\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE00,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH00,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE00,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO00_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE00,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE01,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH01,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE01,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO01_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE01,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE02,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH02,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE02,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO02_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE02,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE03,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH03,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE03,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO03_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE03,\
                       },\
                   },\
               },\
               {\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC1_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC1_PLL_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC1_SAMPLING_RATE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC1_REFCLK_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC1_FABRIC_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC1_FBDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC1_OUTDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC1_REFCLK_DIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC1_BAND,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC1_FS_MAX,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC1_SLICES,\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE10_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE10,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE11_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE11,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE12_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE12,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE13_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE13,\
                       },\
                   },\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE10,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH10,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE10,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO10_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE10,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE11,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH11,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE11,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO11_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE11,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE12,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH12,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE12,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO12_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE12,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE13,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH13,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE13,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO13_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE13,\
                       },\
                   },\
               },\
               {\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC2_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC2_PLL_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC2_SAMPLING_RATE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC2_REFCLK_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC2_FABRIC_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC2_FBDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC2_OUTDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC2_REFCLK_DIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC2_BAND,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC2_FS_MAX,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC2_SLICES,\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE20_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE20,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE21_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE21,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE22_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE22,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE23_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE23,\
                       },\
                   },\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE20,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH20,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE20,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO20_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE20,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE21,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH21,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE21,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO21_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE21,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE22,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH22,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE22,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO22_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE22,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE23,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH23,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE23,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO23_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE23,\
                       },\
                   },\
               },\
               {\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC3_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC3_PLL_ENABLE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC3_SAMPLING_RATE,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC3_REFCLK_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC3_FABRIC_FREQ,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC3_FBDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC3_OUTDIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC3_REFCLK_DIV,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC3_BAND,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC3_FS_MAX,\
                   XPAR_USP_RF_DATA_CONVERTER_0_ADC3_SLICES,\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE30_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE30,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE31_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE31,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE32_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE32,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_SLICE33_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_MODE33,\
                       },\
                   },\
                   {\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE30,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH30,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE30,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO30_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE30,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE31,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH31,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE31,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO31_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE31,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE32,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH32,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE32,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO32_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE32,\
                       },\
                       {\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_TYPE33,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DATA_WIDTH33,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_DECIMATION_MODE33,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_FIFO33_ENABLE,\
                           XPAR_USP_RF_DATA_CONVERTER_0_ADC_MIXER_TYPE33,\
                       },\
                   },\
               },\
        } \
    }; \
    DEVICE_DT_INST_DEFINE(port, \
	xlnx_rfdc_init, \
	NULL, \
	&xlnx_rfdc_dev_data_##port, \
	&xlnx_rfdc_dev_cfg_##port, \
	POST_KERNEL, CONFIG_RFDC_INIT_PRIORITY, \
	(void*)&xlnx_rfdc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XLNX_RFDC_DEV_INIT)
