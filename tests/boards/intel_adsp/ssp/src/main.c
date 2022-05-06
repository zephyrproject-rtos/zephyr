/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/device.h>
#include <zephyr/drivers/dai.h>
#include <zephyr/drivers/dma.h>
#include <soc.h>

/* sof ssp bespoke data */
struct sof_dai_ssp_params {
	uint32_t reserved0;
	uint16_t reserved1;
	uint16_t mclk_id;

	uint32_t mclk_rate;
	uint32_t fsync_rate;
	uint32_t bclk_rate;

	uint32_t tdm_slots;
	uint32_t rx_slots;
	uint32_t tx_slots;

	uint32_t sample_valid_bits;
	uint16_t tdm_slot_width;
	uint16_t reserved2;

	uint32_t mclk_direction;

	uint16_t frame_pulse_width;
	uint16_t tdm_per_slot_padding_flag;
	uint32_t clks_control;
	uint32_t quirks;
	uint32_t bclk_delay;
} __packed;

static const struct device *dev_dai_ssp;
static const struct device *dev_dma_dw;
static struct dai_config config;
static struct sof_dai_ssp_params ssp_config;

#define BUF_SIZE 48
#define XFER_SIZE BUF_SIZE * 4
#define XFERS 2
#define DMA_DEVICE_NAME "DMA_0"
#define SSP_DEVICE_NAME "SSP_0"

K_SEM_DEFINE(xfer_sem, 0, 1);

static struct dma_config dma_cfg = {0};
static struct dma_block_config dma_block_cfgs[XFERS];
static struct dma_config dma_cfg_rx = {0};
static struct dma_block_config dma_block_cfgs_rx[XFERS];

/* 1ms frame of 48Hz sine in 48kHz, thus 48 x 32 bit samples */
static int32_t sine_buf[BUF_SIZE] = {
	0x00000000, 0x10b5150f, 0x2120fb83, 0x30fbc54d,
	0x40000000, 0x4debe4fe, 0x5a82799a, 0x658c9a2d,
	0x6ed9eba1, 0x7641af3d, 0x7ba3751d, 0x7ee7aa4c,
	0x7fffffff, 0x7ee7aa4c, 0x7ba3751d, 0x7641af3d,
	0x6ed9eba1, 0x658c9a2d, 0x5a82799a, 0x4debe4fe,
	0x40000000, 0x30fbc54d, 0x2120fb83, 0x10b5150f,
	0x00000000, 0xef4aeaf1, 0xdedf047d, 0xcf043ab3,
	0xc0000000, 0xb2141b02, 0xa57d8666, 0x9a7365d3,
	0x9126145f, 0x89be50c3, 0x845c8ae3, 0x811855b4,
	0x80000000, 0x811855b4, 0x845c8ae3, 0x89be50c3,
	0x9126145f, 0x9a7365d3, 0xa57d8666, 0xb2141b02,
	0xc0000000, 0xcf043ab3, 0xdedf047d, 0xef4aeaf1,
};

static __aligned(32) int32_t rx_data[XFERS][BUF_SIZE] = { { 0 } };

static void dma_callback(const struct device *dma_dev, void *user_data,
			 uint32_t channel, int status)
{
	if (status)
		TC_PRINT("tx callback status %d\n", status);
	else
		TC_PRINT("tx giving up\n");
}

static void dma_callback_rx(const struct device *dma_dev, void *user_data,
			    uint32_t channel, int status)
{
	if (status) {
		TC_PRINT("rx callback status %d\n", status);
	} else {
		TC_PRINT("rx giving xfer_sem\n");
		k_sem_give(&xfer_sem);
	}
}

static int config_output_dma(const struct dai_properties *props, uint32_t *chan_id)
{
	dma_cfg.dma_slot = props->dma_hs_id;
	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.dest_handshake = 0;
	dma_cfg.source_handshake = 0;
	dma_cfg.cyclic = 1;
	dma_cfg.source_data_size = ssp_config.tdm_slot_width / 8;
	dma_cfg.dest_data_size = ssp_config.tdm_slot_width / 8;
	dma_cfg.source_burst_length = ssp_config.tdm_slots;
	dma_cfg.dest_burst_length = ssp_config.tdm_slots;
	dma_cfg.user_data = NULL;
	dma_cfg.dma_callback = dma_callback;
	dma_cfg.block_count = XFERS;
	dma_cfg.head_block = dma_block_cfgs;
	dma_cfg.complete_callback_en = false; /* per block completion */

	*chan_id = dma_request_channel(dev_dma_dw, NULL);
	if (*chan_id < 0) {
		TC_PRINT("Platform does not support dma request channel\n");
		return -1;
	}

	memset(dma_block_cfgs, 0, sizeof(dma_block_cfgs));
	for (int i = 0; i < XFERS; i++) {
		dma_block_cfgs[i].block_size = XFER_SIZE;
		dma_block_cfgs[i].source_address = (uint32_t)sine_buf;
		dma_block_cfgs[i].dest_address = props->fifo_address;
		TC_PRINT("dma block %d block_size %d, source addr %x, dest addr %x\n",
			 i, BUF_SIZE, dma_block_cfgs[i].source_address,
			 dma_block_cfgs[i].dest_address);
		if (i < XFERS - 1) {
			dma_block_cfgs[i].next_block = &dma_block_cfgs[i+1];
			TC_PRINT("set next block pointer to %p\n", dma_block_cfgs[i].next_block);
		}
	}

	return 0;
}

static int config_input_dma(const struct dai_properties *props, uint32_t *chan_id_rx)
{
	dma_cfg_rx.dma_slot = props->dma_hs_id;
	dma_cfg_rx.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg_rx.dest_handshake = 0;
	dma_cfg_rx.source_handshake = 0;
	dma_cfg_rx.cyclic = 1;
	dma_cfg_rx.source_data_size = ssp_config.tdm_slot_width / 8;
	dma_cfg_rx.dest_data_size = ssp_config.tdm_slot_width / 8;
	dma_cfg_rx.source_burst_length = ssp_config.tdm_slots;
	dma_cfg_rx.dest_burst_length = ssp_config.tdm_slots;
	dma_cfg_rx.user_data = NULL;
	dma_cfg_rx.dma_callback = dma_callback_rx;
	dma_cfg_rx.block_count = XFERS;
	dma_cfg_rx.head_block = dma_block_cfgs_rx;
	dma_cfg_rx.complete_callback_en = false; /* per block completion */

	*chan_id_rx = dma_request_channel(dev_dma_dw, NULL);
	if (*chan_id_rx < 0) {
		TC_PRINT("Platform does not support dma request channel\n");
		return -1;
	}

	memset(dma_block_cfgs_rx, 0, sizeof(dma_block_cfgs_rx));
	memset(rx_data, 0, sizeof(rx_data));
	for (int i = 0; i < XFERS; i++) {
		dma_block_cfgs_rx[i].block_size = XFER_SIZE;
		dma_block_cfgs_rx[i].source_address = props->fifo_address;
		dma_block_cfgs_rx[i].dest_address = (uint32_t)rx_data[i];
		TC_PRINT("dma block %d block_size %d, source addr %x, dest addr %x\n",
			 i, BUF_SIZE, dma_block_cfgs_rx[i].source_address,
			 dma_block_cfgs_rx[i].dest_address);
		if (i < XFERS - 1) {
			dma_block_cfgs_rx[i].next_block = &dma_block_cfgs_rx[i+1];
			TC_PRINT("set next block pointer to %p\n", dma_block_cfgs_rx[i].next_block);
		}
	}

	return 0;
}

static int check_transmission(void)
{
	int32_t buffer[2 * BUF_SIZE];
	bool pattern_found = false;
	int32_t pattern[4];
	int start_index;
	int i, j;

	TC_PRINT("Checking transmission:\n");

	/* let's make things easier */
	for (i = 0; i < BUF_SIZE; i++) {
		buffer[i] = rx_data[0][i];
		buffer[BUF_SIZE + i] = rx_data[1][i];
	}

	for (i = 0; i < 4; i++)
		pattern[i] = sine_buf[i];

	TC_PRINT("tx_data (will be sent 2 times):\n");
	for (i = 0; i < BUF_SIZE; i += 8) {
		for (j = 0; j < 8; j++)
			TC_PRINT("0x%08x ", sine_buf[i + j]);
		TC_PRINT("\n");
	}
	TC_PRINT("\n");

	TC_PRINT("rx_data:\n");
	for (i = 0; i < BUF_SIZE * 2; i += 8) {
		for (j = 0; j < 8; j++) {
			TC_PRINT("0x%08x ", buffer[i + j]);
		}
		TC_PRINT("\n");
	}
	TC_PRINT("\n");

	/* search for pattern only on first half */
	for (i = 0; i < BUF_SIZE; i++) {
		if (buffer[i] == pattern[0] &&
		    buffer[i + 1] == pattern[1] &&
		    buffer[i + 2] == pattern[2] &&
		    buffer[i + 3] == pattern[3]) {
			pattern_found = true;
			start_index = i;
			break;
		}
	}

	if (!pattern_found) {
		TC_PRINT("pattern not found in rx buffer\n");
		return TC_FAIL;
	}

	TC_PRINT("pattern found in rx buffer an index %d value %x\n", start_index,
		 buffer[start_index]);

	for (i = 0; i < BUF_SIZE; i++) {
		TC_PRINT("tx 0x%08x rx 0x%08x\n", buffer[start_index + i], sine_buf[i]);
		if (buffer[start_index + i] != sine_buf[i])
			break;
	}

	if (i < BUF_SIZE - 1) {
		TC_PRINT("transfer differs at index %d\n", i);
		return TC_FAIL;
	}

	return TC_PASS;
}

void test_adsp_ssp_transfer(void)
{
	const struct dai_properties *props;
	static int chan_id_rx;
	static int chan_id;

	props = dai_get_properties(dev_dai_ssp, DAI_DIR_TX, 0);
	if (!props) {
		TC_PRINT("Cannot get dai tx properties\n");
		return;
	}

	if (config_output_dma(props, &chan_id)) {
		TC_PRINT("ERROR: config tx dma (%d)\n", chan_id);
		return;
	}

	TC_PRINT("Configuring the dma tx transfer on channel %d\n", chan_id);

	if (dma_config(dev_dma_dw, chan_id, &dma_cfg)) {
		TC_PRINT("ERROR: dma tx config (%d)\n", chan_id);
		return;
	}

	props = dai_get_properties(dev_dai_ssp, DAI_DIR_RX, 0);
	if (!props) {
		TC_PRINT("Cannot get dai rx properties\n");
		return;
	}

	if (config_input_dma(props, &chan_id_rx)) {
		TC_PRINT("ERROR: config rx dma (%d)\n", chan_id);
		return;
	}

	TC_PRINT("Configuring the dma rx transfer on channel %d\n", chan_id_rx);

	if (dma_config(dev_dma_dw, chan_id_rx, &dma_cfg_rx)) {
		TC_PRINT("ERROR: transfer config (%d)\n", chan_id_rx);
		return;
	}

	TC_PRINT("Starting the transfer on channels %d and %d and waiting completion\n", chan_id,
		 chan_id_rx);

	if (dai_trigger(dev_dai_ssp, DAI_DIR_RX, DAI_TRIGGER_PRE_START)) {
		TC_PRINT("ERROR: dai rx pre start\n");
		return;
	}

	if (dai_trigger(dev_dai_ssp, DAI_DIR_TX, DAI_TRIGGER_PRE_START)) {
		TC_PRINT("ERROR: dai tx pre start\n");
		return;
	}

	if (dma_start(dev_dma_dw, chan_id_rx)) {
		TC_PRINT("ERROR: dma rx transfer start (%d)\n", chan_id);
		return;
	}

	if (dma_start(dev_dma_dw, chan_id)) {
		TC_PRINT("ERROR: dma tx transfer start (%d)\n", chan_id);
		return;
	}

	if (dai_trigger(dev_dai_ssp, DAI_DIR_RX, DAI_TRIGGER_START)) {
		TC_PRINT("ERROR: rx dai start\n");
		return;
	}

	if (dai_trigger(dev_dai_ssp, DAI_DIR_TX, DAI_TRIGGER_START)) {
		TC_PRINT("ERROR: tx dai start\n");
		return;
	}


	if (k_sem_take(&xfer_sem, K_MSEC(1000)) != 0) {
		TC_PRINT("timed out waiting for xfers\n");
		return;
	}

	dma_stop(dev_dma_dw, chan_id_rx);
	dma_stop(dev_dma_dw, chan_id);
	dai_trigger(dev_dai_ssp, DAI_DIR_RX, DAI_TRIGGER_STOP);
	dai_trigger(dev_dai_ssp, DAI_DIR_TX, DAI_TRIGGER_STOP);

	check_transmission();
}

void test_adsp_ssp_config_set(void)
{
	int ret;

	/* generic config */
	config.type = DAI_INTEL_SSP;
	config.dai_index = 0;
	config.channels = 2;
	config.rate = 48000;
	/*
	 * 1st byte = "ssp mode" = 1 = SOF_DAI_FMT_I2S = I2S mode
	 * 3rd byte = "frame mode" = 0 = SOF_DAI_FMT_NB_NF = normal bit clock + frame
	 * 4th byte = "clocks mode" = 4 = SOF_DAI_FMT_CBC_CFC =
	 * codec bclk consumer & frame consumer
	 */
	config.format = 0x00004001;
	config.options = 0;
	config.word_size = 0;
	config.block_size = 0;

	/* bespoke config */
	ssp_config.mclk_id = 0;
	ssp_config.mclk_rate = 24576000;
	ssp_config.fsync_rate = 48000;
	ssp_config.bclk_rate = 3072000;
	ssp_config.tdm_slots = 2;
	ssp_config.rx_slots = 3;
	ssp_config.tx_slots = 3;
	ssp_config.sample_valid_bits = 32;
	ssp_config.tdm_slot_width = 32;
	ssp_config.mclk_direction = 0;
	ssp_config.frame_pulse_width = 0;
	ssp_config.tdm_per_slot_padding_flag = 0;
	ssp_config.clks_control = 0;
	ssp_config.quirks = 1 << 6; /* loopback bit on */
	ssp_config.bclk_delay = 0;

	ret = dai_config_set(dev_dai_ssp, &config, &ssp_config);

	zassert_equal(ret, TC_PASS, NULL);
}

void test_adsp_ssp_probe(void)
{
	int ret;

	ret = dai_probe(dev_dai_ssp);

	zassert_equal(ret, TC_PASS, NULL);
}

void test_main(void)
{
	dev_dai_ssp = device_get_binding(SSP_DEVICE_NAME);

	if (dev_dai_ssp != NULL) {
		k_object_access_grant(dev_dai_ssp, k_current_get());
	}

	zassert_not_null(dev_dai_ssp, "device SSP_0 not found");

	dev_dma_dw = device_get_binding(DMA_DEVICE_NAME);

	zassert_not_null(dev_dma_dw, "device DMA_0 not found");

	ztest_test_suite(adsp_ssp,
			 ztest_unit_test(test_adsp_ssp_probe),
			 ztest_unit_test(test_adsp_ssp_config_set),
			 ztest_unit_test(test_adsp_ssp_transfer));

	ztest_run_test_suite(adsp_ssp);
}
