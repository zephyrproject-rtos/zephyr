/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PULSE_IO_ESPRESSIF_RMT_PULSE_IO_ESP32_RMT_H_
#define ZEPHYR_DRIVERS_PULSE_IO_ESPRESSIF_RMT_PULSE_IO_ESP32_RMT_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pulse_io.h>

#include <soc/soc_caps.h>
#include <hal/rmt_types.h>
#include <hal/rmt_hal.h>
#include <hal/rmt_ll.h>
#include <hal/rmt_periph.h>
#include <esp_intr_alloc.h>
#include <esp_clk_tree.h>

#define RMT_NUM_CHANNELS      RMT_LL_GET(CHANS_PER_INST)
#define RMT_NUM_TX_CHANNELS   RMT_LL_GET(TX_CANDIDATES_PER_INST)
#define RMT_NUM_RX_CHANNELS   RMT_LL_GET(RX_CANDIDATES_PER_INST)
#define RMT_TX_CHANNEL_OFFSET 0
#define RMT_RX_CHANNEL_OFFSET (RMT_NUM_CHANNELS - RMT_NUM_TX_CHANNELS)

/* Nonzero when every channel has its own clock divider chain instead
 * of one shared group clock (ESP32 and ESP32-S2).
 */
#define RMT_CHANNEL_CLK_INDEPENDENT RMT_LL_GET(CHANNEL_CLK_INDEPENDENT)

#define RMT_MEM_WORDS       SOC_RMT_MEM_WORDS_PER_CHANNEL
#define RMT_PING_PONG_WORDS (RMT_MEM_WORDS / 2)

/* Maximum duration of one hardware half-entry, in channel ticks */
#define RMT_DURATION_MAX 32767U

/* ESP32 and ESP32-S2 do not define the prescale limit; the per-channel
 * clock divider is an 8-bit field where 0 means 256.
 */
#ifndef RMT_LL_CHANNEL_CLOCK_MAX_PRESCALE
#define RMT_LL_CHANNEL_CLOCK_MAX_PRESCALE 256
#endif

#define RMT_INTR_ALLOC_FLAGS (ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_LOWMED)

#ifdef CONFIG_PULSE_IO_ESP32_RMT_DMA
#define RMT_DMA_SUPPORTED 1
#else
#define RMT_DMA_SUPPORTED 0
#endif

#if RMT_DMA_SUPPORTED
#define RMT_DMA_CHANNEL_UNDEFINED 0xFF
#define RMT_DMA_WORDS             CONFIG_PULSE_IO_ESP32_RMT_DMA_WORDS
#endif

typedef struct {
	struct {
		rmt_symbol_word_t symbols[RMT_MEM_WORDS];
	} channels[RMT_NUM_CHANNELS];
} rmt_block_mem_t;

/* RMTMEM address is declared in <target>.peripherals.ld */
extern rmt_block_mem_t RMTMEM;

enum rmt_channel_state {
	RMT_CH_FREE,
	RMT_CH_OPEN,
	RMT_CH_READY,
	RMT_CH_ACTIVE,
};

struct rmt_tx_iter {
	const struct pulse_symbol *syms;
	const struct pulse_cell *cells;
	uint32_t cell_period;
	size_t count;
	size_t pos;
	uint32_t rem;
	uint8_t level;
	uint8_t cell_phase;
};

struct rmt_channel {
	const struct device *dev;
	uint8_t index;
	uint8_t state;
	struct pulse_io_config cfg;
	uint32_t resolution_hz;
	intr_handle_t intr;
	struct k_sem done;
	struct k_spinlock lock;
	rmt_symbol_word_t *hw_mem;
	volatile int result;
	bool with_dma;

	struct rmt_tx_iter iter;
	uint8_t refill_region;
	bool eof_written;
	uint32_t remain_loops;

	struct pulse_symbol *rx_buf;
	size_t rx_cap;
	size_t rx_count;
	size_t rx_hw_off;
	size_t rx_dma_blocks;
	bool rx_overflow;
};

struct rmt_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
#if RMT_DMA_SUPPORTED
	const struct device *dma_dev;
	uint8_t tx_dma_channel;
	uint8_t rx_dma_channel;
#endif
};

struct rmt_data {
	rmt_hal_context_t hal;
	struct k_spinlock glock;
	uint32_t group_resolution_hz;
	uint32_t src_clk_hz;
	uint32_t filter_clk_hz;
	struct rmt_channel channels[RMT_NUM_CHANNELS];
};

int rmt_select_channel_clock(const struct device *dev, struct rmt_channel *ch,
			     uint32_t resolution_hz);

int rmt_tx_configure(const struct device *dev, struct rmt_channel *ch);
int rmt_rx_configure(const struct device *dev, struct rmt_channel *ch);
int rmt_tx_start(const struct device *dev, struct rmt_channel *ch,
		 const struct pulse_io_tx_req *req);
int rmt_rx_start(const struct device *dev, struct rmt_channel *ch,
		 const struct pulse_io_rx_req *req);
void rmt_tx_halt(const struct device *dev, struct rmt_channel *ch);
void rmt_rx_halt(const struct device *dev, struct rmt_channel *ch);

#endif /* ZEPHYR_DRIVERS_PULSE_IO_ESPRESSIF_RMT_PULSE_IO_ESP32_RMT_H_ */
