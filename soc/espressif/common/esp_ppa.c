/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_ppa

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/slist.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>

#include <esp_intr_alloc.h>

#include "esp_err.h"
#include "esp_cache.h"
#include "driver/ppa.h"
#include "hal/ppa_ll.h"
#include "hal/ppa_types.h"
#include "hal/dma2d_ll.h"
#include "hal/dma2d_types.h"
#include "soc/dma2d_channel.h"
#include "soc/interrupts.h"

LOG_MODULE_REGISTER(esp_ppa, CONFIG_SOC_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "The pixel processing accelerator node has to be enabled");

/* Color keying, the scaled alpha mode, the fixed foreground color for
 * alpha-only inputs and the YUV selectors are left at their reset values.
 * A request depending on one of them is rejected rather than run without it.
 */

#define ESP_PPA_RX_CH    0
#define ESP_PPA_TX_BG_CH 0
#define ESP_PPA_TX_FG_CH 1

#define ESP_PPA_RX_DONE_MASK DMA2D_LL_EVENT_RX_SUC_EOF

#define ESP_PPA_MAX_PENDING CONFIG_ESP32_PPA_MAX_PENDING

/* Reset is acknowledged within a few bus cycles. Bounded because this also
 * runs from the completion interrupt and under the queue spinlock.
 */
#define ESP_PPA_RESET_SPINS 10000

/* The picture and block dimensions are 14-bit fields in the descriptor. */
#define ESP_PPA_MAX_PIC_DIM 16383

/* The scaling factor integer part is an 8-bit register field. */
#define ESP_PPA_MAX_SCALE 256.0f

enum esp_ppa_op {
	ESP_PPA_OP_FILL,
	ESP_PPA_OP_BLEND,
	ESP_PPA_OP_SRM,
};

struct esp_ppa_txn {
	sys_snode_t node;
	bool in_use;
	bool counted;
	enum esp_ppa_op op;
	struct ppa_client_t *client;
	struct k_sem *done;
	ppa_event_callback_t cb;
	void *user_data;
	uint32_t burst_bytes;
	uint32_t fill_color;
	ppa_fill_oper_config_t fill_cfg;
	ppa_blend_oper_config_t blend_cfg;
	ppa_srm_oper_config_t srm_cfg;

	dma2d_descriptor_t rx_desc __aligned(64);
	dma2d_descriptor_t bg_desc __aligned(64);
	dma2d_descriptor_t fg_desc __aligned(64);
};

struct ppa_client_t {
	ppa_operation_t oper_type;
	ppa_data_burst_length_t burst_length;
	uint32_t max_pending;
	uint32_t pending_count;
	ppa_event_callback_t on_trans_done;
};

static struct esp_ppa_state {
	bool engine_ready;
	struct k_mutex lock;
	struct k_spinlock queue_lock;
	ppa_dev_t *ppa;
	dma2d_dev_t *dma;
	intr_handle_t rx_intr;
	sys_slist_t pending;
	struct esp_ppa_txn *active;
	uint32_t err_count;
	struct esp_ppa_txn txns[ESP_PPA_MAX_PENDING];
} ppa_state = {
	.lock = Z_MUTEX_INITIALIZER(ppa_state.lock),
};

static void esp_ppa_start_txn(struct esp_ppa_txn *txn);

static uint32_t esp_ppa_burst_bytes(ppa_data_burst_length_t burst)
{
	switch (burst) {
	case PPA_DATA_BURST_LENGTH_8:
		return 8;
	case PPA_DATA_BURST_LENGTH_16:
		return 16;
	case PPA_DATA_BURST_LENGTH_32:
		return 32;
	case PPA_DATA_BURST_LENGTH_64:
		return 64;
	case PPA_DATA_BURST_LENGTH_128:
	default:
		return 128;
	}
}

static void esp_ppa_isr(void *arg)
{
	struct esp_ppa_state *st = arg;
	dma2d_dev_t *dma = st->dma;
	uint32_t status = dma2d_ll_rx_get_interrupt_status(dma, ESP_PPA_RX_CH);

	dma2d_ll_rx_clear_interrupt_status(dma, ESP_PPA_RX_CH, UINT32_MAX);

	if (!(status & ESP_PPA_RX_DONE_MASK)) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&st->queue_lock);
	struct esp_ppa_txn *done = st->active;

	st->active = NULL;

	sys_snode_t *next = sys_slist_get(&st->pending);

	if (next != NULL) {
		st->active = CONTAINER_OF(next, struct esp_ppa_txn, node);
		esp_ppa_start_txn(st->active);
	}

	k_spin_unlock(&st->queue_lock, key);

	if (done == NULL) {
		return;
	}

	if (done->done != NULL) {
		k_sem_give(done->done);
		return;
	}

	if (done->cb != NULL) {
		/* The return value reports whether a task switch is needed,
		 * which Zephyr already handles when leaving the interrupt.
		 */
		ppa_event_data_t evt;

		(void)memset(&evt, 0, sizeof(evt));
		(void)done->cb(done->client, &evt, done->user_data);
	}
	if (done->counted && done->client != NULL) {
		done->client->pending_count--;
	}
	done->in_use = false;
}

static int esp_ppa_engine_init(void)
{
	unsigned int key = irq_lock();

	ppa_ll_enable_bus_clock(true);
	ppa_ll_reset_register();

	dma2d_ll_enable_bus_clock(0, true);
	dma2d_ll_reset_register(0);

	irq_unlock(key);

	ppa_state.ppa = PPA_LL_GET_HW;
	ppa_state.dma = DMA2D_LL_GET_HW(0);

	__ASSERT((uintptr_t)ppa_state.ppa == DT_INST_REG_ADDR_BY_NAME(0, ppa) &&
			 (uintptr_t)ppa_state.dma == DT_INST_REG_ADDR_BY_NAME(0, dma2d),
		 "The accelerator is not where the devicetree says it is");

	sys_slist_init(&ppa_state.pending);

	dma2d_ll_hw_enable(ppa_state.dma, true);

	int err = esp_intr_alloc_intrstatus(
		DT_INST_IRQN(0), ESP_INTR_FLAG_SHARED,
		(uint32_t)dma2d_ll_rx_get_interrupt_status_reg(ppa_state.dma, ESP_PPA_RX_CH),
		ESP_PPA_RX_DONE_MASK, esp_ppa_isr, &ppa_state, &ppa_state.rx_intr);
	if (err != 0) {
		LOG_ERR("Failed to allocate PPA interrupt (%d)", err);
		ppa_state.rx_intr = NULL;
		dma2d_ll_hw_enable(ppa_state.dma, false);
		return -EIO;
	}

	return 0;
}

static void esp_ppa_tx_reset(uint32_t ch)
{
	dma2d_dev_t *dma = ppa_state.dma;
	uint32_t spins = ESP_PPA_RESET_SPINS;

	dma2d_ll_tx_stop(dma, ch);
	while (!dma2d_ll_tx_is_reset_avail(dma, ch) && spins > 0) {
		spins--;
	}
	dma2d_ll_tx_reset_channel(dma, ch);
}

static void esp_ppa_rx_reset(uint32_t ch)
{
	dma2d_dev_t *dma = ppa_state.dma;
	uint32_t spins = ESP_PPA_RESET_SPINS;

	dma2d_ll_rx_stop(dma, ch);
	while (!dma2d_ll_rx_is_reset_avail(dma, ch) && spins > 0) {
		spins--;
	}
	dma2d_ll_rx_reset_channel(dma, ch);
}

static void esp_ppa_tx_setup(uint32_t ch, int trig_periph, int periph_id, bool dscr_port,
			     uint32_t burst_bytes)
{
	dma2d_dev_t *dma = ppa_state.dma;

	esp_ppa_tx_reset(ch);
	dma2d_ll_tx_connect_to_periph(dma, ch, trig_periph, periph_id);
	dma2d_ll_tx_enable_reorder(dma, ch, false);
	dma2d_ll_tx_enable_dscr_port(dma, ch, dscr_port);
	dma2d_ll_tx_enable_owner_check(dma, ch, false);
	dma2d_ll_tx_enable_auto_write_back(dma, ch, false);
	dma2d_ll_tx_enable_eof_mode(dma, ch, true);
	dma2d_ll_tx_enable_page_bound_wrap(dma, ch, true);
	dma2d_ll_tx_set_macro_block_size(dma, ch, DMA2D_MACRO_BLOCK_SIZE_NONE);
	dma2d_ll_tx_enable_descriptor_burst(dma, ch, true);
	dma2d_ll_tx_set_data_burst_length(dma, ch, burst_bytes);
	dma2d_ll_tx_enable_interrupt(dma, ch, UINT32_MAX, false);
	dma2d_ll_tx_clear_interrupt_status(dma, ch, UINT32_MAX);
}

static void esp_ppa_rx_setup(uint32_t ch, int trig_periph, int periph_id, bool dscr_port,
			     uint32_t burst_bytes)
{
	dma2d_dev_t *dma = ppa_state.dma;

	esp_ppa_rx_reset(ch);
	dma2d_ll_rx_connect_to_periph(dma, ch, trig_periph, periph_id);
	dma2d_ll_rx_enable_reorder(dma, ch, false);
	dma2d_ll_rx_enable_dscr_port(dma, ch, dscr_port);
	dma2d_ll_rx_enable_owner_check(dma, ch, false);
	dma2d_ll_rx_set_auto_return_owner(dma, ch, DMA2D_DESCRIPTOR_BUFFER_OWNER_CPU);
	dma2d_ll_rx_enable_page_bound_wrap(dma, ch, true);
	dma2d_ll_rx_set_macro_block_size(dma, ch, DMA2D_MACRO_BLOCK_SIZE_NONE);
	dma2d_ll_rx_enable_descriptor_burst(dma, ch, true);
	dma2d_ll_rx_set_data_burst_length(dma, ch, burst_bytes);
	dma2d_ll_rx_clear_interrupt_status(dma, ch, UINT32_MAX);
	dma2d_ll_rx_enable_interrupt(dma, ch, ESP_PPA_RX_DONE_MASK, true);
}

/* The fill, blend and scale-rotate-mirror color modes share the same FourCC
 * values, so one lookup serves all three. UINT32_MAX marks an unsupported
 * mode.
 */
static uint32_t esp_ppa_pbyte(uint32_t color_mode)
{
	switch (color_mode) {
	case ESP_COLOR_FOURCC_BGRA32:
		return DMA2D_DESCRIPTOR_PBYTE_4B0_PER_PIXEL;
	case ESP_COLOR_FOURCC_BGR24:
		return DMA2D_DESCRIPTOR_PBYTE_3B0_PER_PIXEL;
	case ESP_COLOR_FOURCC_RGB16:
		return DMA2D_DESCRIPTOR_PBYTE_2B0_PER_PIXEL;
	case ESP_COLOR_FOURCC_ALPHA8:
	case ESP_COLOR_FOURCC_GREY:
		return DMA2D_DESCRIPTOR_PBYTE_1B0_PER_PIXEL;
	default:
		return UINT32_MAX;
	}
}

/* A block reaching outside its surface makes the DMA read or write past the
 * buffer, and oversized dimensions are truncated silently by the descriptor
 * bitfields.
 */
static bool esp_ppa_geometry_valid(uint32_t pic_w, uint32_t pic_h, uint32_t block_w,
				   uint32_t block_h, uint32_t off_x, uint32_t off_y, uint32_t pbyte)
{
	if (pbyte == UINT32_MAX) {
		return false;
	}
	if (pic_w == 0 || pic_h == 0 || block_w == 0 || block_h == 0) {
		return false;
	}
	if (pic_w > ESP_PPA_MAX_PIC_DIM || pic_h > ESP_PPA_MAX_PIC_DIM) {
		return false;
	}

	return (off_x + block_w <= pic_w) && (off_y + block_h <= pic_h);
}

static void esp_ppa_fill_desc(dma2d_descriptor_t *desc, void *buffer, uint32_t pic_w,
			      uint32_t pic_h, uint32_t block_w, uint32_t block_h, uint32_t off_x,
			      uint32_t off_y, uint32_t pbyte)
{
	memset(desc, 0, sizeof(*desc));
	desc->dma2d_en = 1;
	desc->suc_eof = 1;
	desc->owner = DMA2D_DESCRIPTOR_BUFFER_OWNER_DMA;
	desc->mode = DMA2D_DESCRIPTOR_BLOCK_RW_MODE_SINGLE;
	desc->vb_size = block_h;
	desc->hb_length = block_w;
	desc->va_size = pic_h;
	desc->ha_length = pic_w;
	desc->pbyte = pbyte;
	desc->y = off_y;
	desc->x = off_x;
	desc->buffer = buffer;
	desc->next = NULL;

	sys_cache_data_flush_range(desc, sizeof(*desc));
}

static void esp_ppa_start_txn(struct esp_ppa_txn *txn)
{
	dma2d_dev_t *dma = ppa_state.dma;
	ppa_dev_t *ppa = ppa_state.ppa;
	uint32_t burst = txn->burst_bytes;

	switch (txn->op) {
	case ESP_PPA_OP_FILL: {
		ppa_fill_oper_config_t *cfg = &txn->fill_cfg;

		ppa_ll_blend_reset(ppa);
		esp_ppa_rx_setup(ESP_PPA_RX_CH, DMA2D_TRIG_PERIPH_PPA_BLEND,
				 SOC_DMA2D_TRIG_PERIPH_PPA_BLEND_RX, false, burst);
		dma2d_ll_rx_set_desc_addr(dma, ESP_PPA_RX_CH, (uint32_t)&txn->rx_desc);
		dma2d_ll_rx_start(dma, ESP_PPA_RX_CH);

		ppa_ll_blend_configure_filling_block(ppa, cfg->out.fill_cm, &txn->fill_color,
						     cfg->fill_block_w, cfg->fill_block_h);
		ppa_ll_blend_set_tx_color_mode(ppa, cfg->out.fill_cm);
		ppa_ll_blend_start(ppa, PPA_LL_BLEND_TRANS_MODE_FILL);
		break;
	}
	case ESP_PPA_OP_BLEND: {
		ppa_blend_oper_config_t *cfg = &txn->blend_cfg;
		color_pixel_rgb888_data_t ck_low = {.r = 0xFF, .g = 0xFF, .b = 0xFF};
		color_pixel_rgb888_data_t ck_high = {.r = 0x00, .g = 0x00, .b = 0x00};
		color_pixel_rgb888_data_t ck_zero = {.r = 0x00, .g = 0x00, .b = 0x00};

		ppa_ll_blend_reset(ppa);
		esp_ppa_tx_setup(ESP_PPA_TX_BG_CH, DMA2D_TRIG_PERIPH_PPA_BLEND,
				 SOC_DMA2D_TRIG_PERIPH_PPA_BLEND_BG_TX, false, burst);
		esp_ppa_tx_setup(ESP_PPA_TX_FG_CH, DMA2D_TRIG_PERIPH_PPA_BLEND,
				 SOC_DMA2D_TRIG_PERIPH_PPA_BLEND_FG_TX, false, burst);
		esp_ppa_rx_setup(ESP_PPA_RX_CH, DMA2D_TRIG_PERIPH_PPA_BLEND,
				 SOC_DMA2D_TRIG_PERIPH_PPA_BLEND_RX, false, burst);

		dma2d_ll_tx_set_desc_addr(dma, ESP_PPA_TX_BG_CH, (uint32_t)&txn->bg_desc);
		dma2d_ll_tx_set_desc_addr(dma, ESP_PPA_TX_FG_CH, (uint32_t)&txn->fg_desc);
		dma2d_ll_rx_set_desc_addr(dma, ESP_PPA_RX_CH, (uint32_t)&txn->rx_desc);

		ppa_ll_blend_set_rx_bg_color_mode(ppa, cfg->in_bg.blend_cm);
		ppa_ll_blend_enable_rx_bg_byte_swap(ppa, cfg->bg_byte_swap);
		ppa_ll_blend_enable_rx_bg_rgb_swap(ppa, cfg->bg_rgb_swap);
		ppa_ll_blend_configure_rx_bg_alpha(ppa, cfg->bg_alpha_update_mode,
						   cfg->bg_alpha_fix_val);

		ppa_ll_blend_set_rx_fg_color_mode(ppa, cfg->in_fg.blend_cm);
		ppa_ll_blend_enable_rx_fg_byte_swap(ppa, cfg->fg_byte_swap);
		ppa_ll_blend_enable_rx_fg_rgb_swap(ppa, cfg->fg_rgb_swap);
		ppa_ll_blend_configure_rx_fg_alpha(ppa, cfg->fg_alpha_update_mode,
						   cfg->fg_alpha_fix_val);

		ppa_ll_blend_set_tx_color_mode(ppa, cfg->out.blend_cm);

		ppa_ll_blend_configure_rx_bg_ck_range(ppa, &ck_low, &ck_high);
		ppa_ll_blend_configure_rx_fg_ck_range(ppa, &ck_low, &ck_high);
		ppa_ll_blend_set_ck_default_rgb(ppa, &ck_zero);
		ppa_ll_blend_enable_ck_fg_bg_reverse(ppa, false);

		dma2d_ll_tx_start(dma, ESP_PPA_TX_BG_CH);
		dma2d_ll_tx_start(dma, ESP_PPA_TX_FG_CH);
		dma2d_ll_rx_start(dma, ESP_PPA_RX_CH);

		ppa_ll_blend_start(ppa, PPA_LL_BLEND_TRANS_MODE_BLEND);
		break;
	}
	case ESP_PPA_OP_SRM: {
		ppa_srm_oper_config_t *cfg = &txn->srm_cfg;
		ppa_srm_color_mode_t in_cm = cfg->in.srm_cm;
		ppa_srm_color_mode_t out_cm = cfg->out.srm_cm;
		uint32_t sx_int = (uint32_t)cfg->scale_x;
		uint32_t sx_frac = (uint32_t)(cfg->scale_x * PPA_LL_SRM_SCALING_FRAG_MAX) &
				   (PPA_LL_SRM_SCALING_FRAG_MAX - 1);
		uint32_t sy_int = (uint32_t)cfg->scale_y;
		uint32_t sy_frac = (uint32_t)(cfg->scale_y * PPA_LL_SRM_SCALING_FRAG_MAX) &
				   (PPA_LL_SRM_SCALING_FRAG_MAX - 1);
		ppa_ll_srm_mb_size_t mb_size = ppa_ll_srm_get_mb_size(ppa);
		uint32_t port_bh, port_bv;

		ppa_ll_srm_get_dma_dscr_port_mode_block_size(ppa, in_cm, mb_size, &port_bh,
							     &port_bv);

		ppa_ll_srm_reset(ppa);

		esp_ppa_tx_setup(ESP_PPA_TX_BG_CH, DMA2D_TRIG_PERIPH_PPA_SRM,
				 SOC_DMA2D_TRIG_PERIPH_PPA_SRM_TX, true, burst);
		dma2d_ll_tx_set_dscr_port_block_size(dma, ESP_PPA_TX_BG_CH, port_bh, port_bv);
		esp_ppa_rx_setup(ESP_PPA_RX_CH, DMA2D_TRIG_PERIPH_PPA_SRM,
				 SOC_DMA2D_TRIG_PERIPH_PPA_SRM_RX, true, burst);

		dma2d_ll_tx_set_desc_addr(dma, ESP_PPA_TX_BG_CH, (uint32_t)&txn->bg_desc);
		dma2d_ll_rx_set_desc_addr(dma, ESP_PPA_RX_CH, (uint32_t)&txn->rx_desc);

		ppa_ll_srm_set_rx_color_mode(ppa, in_cm);
		ppa_ll_srm_enable_rx_byte_swap(ppa, cfg->byte_swap);
		ppa_ll_srm_enable_rx_rgb_swap(ppa, cfg->rgb_swap);
		ppa_ll_srm_configure_rx_alpha(ppa, cfg->alpha_update_mode, cfg->alpha_fix_val);
		ppa_ll_srm_set_tx_color_mode(ppa, out_cm);

		ppa_ll_srm_set_rotation_angle(ppa, cfg->rotation_angle);
		ppa_ll_srm_set_scaling_x(ppa, sx_int, sx_frac);
		ppa_ll_srm_set_scaling_y(ppa, sy_int, sy_frac);
		ppa_ll_srm_enable_mirror_x(ppa, cfg->mirror_x);
		ppa_ll_srm_enable_mirror_y(ppa, cfg->mirror_y);

		uint32_t out_depth = (out_cm == PPA_SRM_COLOR_MODE_RGB888) ? 24 : 16;
		uint32_t w_div = (out_cm == PPA_SRM_COLOR_MODE_RGB888) ? 32 : 64;
		uint32_t w_out = sx_int * cfg->in.block_w +
				 sx_frac * cfg->in.block_w / PPA_LL_SRM_SCALING_FRAG_MAX;
		uint32_t w_left = w_out % w_div;
		uint32_t h_mb = (mb_size == PPA_LL_SRM_MB_SIZE_16_16) ? 16 : 32;
		uint32_t h_in_left = cfg->in.block_h % h_mb;
		uint32_t h_left;
		bool bypass;

		w_left = (w_left == 0) ? w_div : w_left;
		h_in_left = (h_in_left == 0) ? h_mb : h_in_left;
		h_left = sy_int * h_in_left + sy_frac * h_in_left / PPA_LL_SRM_SCALING_FRAG_MAX;
		bypass = ((w_out > w_div) || (cfg->in.block_h > h_mb)) &&
			 ((w_left * h_left * out_depth) < (12 * 128));
		ppa_ll_srm_bypass_mb_order(ppa, bypass);

		dma2d_ll_tx_start(dma, ESP_PPA_TX_BG_CH);
		dma2d_ll_rx_start(dma, ESP_PPA_RX_CH);

		ppa_ll_srm_start(ppa);
		break;
	}
	}
}

static struct esp_ppa_txn *esp_ppa_alloc_txn(void)
{
	for (int i = 0; i < ESP_PPA_MAX_PENDING; i++) {
		if (!ppa_state.txns[i].in_use) {
			struct esp_ppa_txn *txn = &ppa_state.txns[i];

			memset(txn, 0, offsetof(struct esp_ppa_txn, rx_desc));
			txn->in_use = true;
			return txn;
		}
	}

	return NULL;
}

static int esp_ppa_submit(struct esp_ppa_txn *txn, bool blocking)
{
	k_spinlock_key_t key = k_spin_lock(&ppa_state.queue_lock);

	if (ppa_state.active == NULL) {
		ppa_state.active = txn;
		esp_ppa_start_txn(txn);
	} else {
		sys_slist_append(&ppa_state.pending, &txn->node);
	}

	if (!blocking) {
		txn->client->pending_count++;
		txn->counted = true;
	}

	k_spin_unlock(&ppa_state.queue_lock, key);

	return 0;
}

esp_err_t ppa_register_client(const ppa_client_config_t *config, ppa_client_handle_t *ret_client)
{
	if (config == NULL || ret_client == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	if (config->oper_type >= PPA_OPERATION_INVALID) {
		return ESP_ERR_INVALID_ARG;
	}

	struct ppa_client_t *client = k_calloc(1, sizeof(*client));

	if (client == NULL) {
		return ESP_ERR_NO_MEM;
	}

	client->oper_type = config->oper_type;
	client->burst_length = config->data_burst_length;
	client->max_pending = config->max_pending_trans_num ? config->max_pending_trans_num : 1;

	k_mutex_lock(&ppa_state.lock, K_FOREVER);
	if (!ppa_state.engine_ready) {
		if (esp_ppa_engine_init() != 0) {
			k_mutex_unlock(&ppa_state.lock);
			k_free(client);
			return ESP_FAIL;
		}
		ppa_state.engine_ready = true;
	}
	k_mutex_unlock(&ppa_state.lock);

	*ret_client = client;

	return ESP_OK;
}

esp_err_t ppa_unregister_client(ppa_client_handle_t ppa_client)
{
	if (ppa_client == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	if (ppa_client->pending_count != 0) {
		return ESP_ERR_INVALID_STATE;
	}

	k_free(ppa_client);

	return ESP_OK;
}

esp_err_t ppa_client_register_event_callbacks(ppa_client_handle_t ppa_client,
					      const ppa_event_callbacks_t *cbs)
{
	if (ppa_client == NULL || cbs == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	ppa_client->on_trans_done = cbs->on_trans_done;

	return ESP_OK;
}

static esp_err_t esp_ppa_run(struct esp_ppa_txn *txn, ppa_trans_mode_t mode)
{
	if (mode == PPA_TRANS_MODE_NON_BLOCKING) {
		txn->cb = txn->client->on_trans_done;
		esp_ppa_submit(txn, false);
		return ESP_OK;
	}

	struct k_sem done;

	k_sem_init(&done, 0, 1);
	txn->done = &done;
	esp_ppa_submit(txn, true);

	int ret = k_sem_take(&done, K_USEC(CONFIG_ESP32_PPA_WAIT_TIMEOUT_US));

	if (ret != 0) {
		k_spinlock_key_t key = k_spin_lock(&ppa_state.queue_lock);

		txn->done = NULL;

		if (ppa_state.active == txn) {
			/* Hand the engine to the next transaction, otherwise
			 * every later one waits on this abandoned transfer.
			 */
			esp_ppa_rx_reset(ESP_PPA_RX_CH);
			esp_ppa_tx_reset(ESP_PPA_TX_BG_CH);
			esp_ppa_tx_reset(ESP_PPA_TX_FG_CH);
			ppa_state.active = NULL;

			sys_snode_t *next = sys_slist_get(&ppa_state.pending);

			if (next != NULL) {
				ppa_state.active = CONTAINER_OF(next, struct esp_ppa_txn, node);
				esp_ppa_start_txn(ppa_state.active);
			}
		} else {
			sys_slist_find_and_remove(&ppa_state.pending, &txn->node);
		}

		txn->in_use = false;

		k_spin_unlock(&ppa_state.queue_lock, key);
		ppa_state.err_count++;
		return ESP_ERR_TIMEOUT;
	}

	txn->in_use = false;

	return ESP_OK;
}

esp_err_t ppa_do_fill(ppa_client_handle_t ppa_client, const ppa_fill_oper_config_t *config)
{
	if (ppa_client == NULL || config == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	if (ppa_client->oper_type != PPA_OPERATION_FILL) {
		return ESP_ERR_INVALID_ARG;
	}

	uint32_t out_pbyte = esp_ppa_pbyte(config->out.fill_cm);

	if (config->out.buffer == NULL ||
	    !esp_ppa_geometry_valid(config->out.pic_w, config->out.pic_h, config->fill_block_w,
				    config->fill_block_h, config->out.block_offset_x,
				    config->out.block_offset_y, out_pbyte)) {
		return ESP_ERR_INVALID_ARG;
	}

	k_mutex_lock(&ppa_state.lock, K_FOREVER);

	k_spinlock_key_t key = k_spin_lock(&ppa_state.queue_lock);
	struct esp_ppa_txn *txn = esp_ppa_alloc_txn();

	if (txn == NULL || (config->mode == PPA_TRANS_MODE_NON_BLOCKING &&
			    ppa_client->pending_count >= ppa_client->max_pending)) {
		if (txn != NULL) {
			txn->in_use = false;
		}
		k_spin_unlock(&ppa_state.queue_lock, key);
		k_mutex_unlock(&ppa_state.lock);
		return ESP_FAIL;
	}
	k_spin_unlock(&ppa_state.queue_lock, key);

	txn->op = ESP_PPA_OP_FILL;
	txn->client = ppa_client;
	txn->burst_bytes = esp_ppa_burst_bytes(ppa_client->burst_length);
	txn->fill_cfg = *config;
	txn->fill_color = config->fill_color_val;
	txn->user_data = config->user_data;

	esp_ppa_fill_desc(&txn->rx_desc, config->out.buffer, config->out.pic_w, config->out.pic_h,
			  config->fill_block_w, config->fill_block_h, config->out.block_offset_x,
			  config->out.block_offset_y, out_pbyte);

	esp_err_t ret = esp_ppa_run(txn, config->mode);

	k_mutex_unlock(&ppa_state.lock);

	return ret;
}

esp_err_t ppa_do_blend(ppa_client_handle_t ppa_client, const ppa_blend_oper_config_t *config)
{
	if (ppa_client == NULL || config == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	if (ppa_client->oper_type != PPA_OPERATION_BLEND) {
		return ESP_ERR_INVALID_ARG;
	}
	if (config->in_bg.block_w != config->in_fg.block_w ||
	    config->in_bg.block_h != config->in_fg.block_h) {
		return ESP_ERR_INVALID_ARG;
	}

	uint32_t bg_pbyte = esp_ppa_pbyte(config->in_bg.blend_cm);
	uint32_t fg_pbyte = esp_ppa_pbyte(config->in_fg.blend_cm);
	uint32_t out_pbyte = esp_ppa_pbyte(config->out.blend_cm);

	if (config->in_bg.buffer == NULL || config->in_fg.buffer == NULL ||
	    config->out.buffer == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	if (!esp_ppa_geometry_valid(config->in_bg.pic_w, config->in_bg.pic_h, config->in_bg.block_w,
				    config->in_bg.block_h, config->in_bg.block_offset_x,
				    config->in_bg.block_offset_y, bg_pbyte) ||
	    !esp_ppa_geometry_valid(config->in_fg.pic_w, config->in_fg.pic_h, config->in_fg.block_w,
				    config->in_fg.block_h, config->in_fg.block_offset_x,
				    config->in_fg.block_offset_y, fg_pbyte) ||
	    !esp_ppa_geometry_valid(config->out.pic_w, config->out.pic_h, config->in_fg.block_w,
				    config->in_fg.block_h, config->out.block_offset_x,
				    config->out.block_offset_y, out_pbyte)) {
		return ESP_ERR_INVALID_ARG;
	}

	if (config->bg_ck_en || config->fg_ck_en || config->ck_reverse_bg2fg) {
		return ESP_ERR_NOT_SUPPORTED;
	}
	if (config->bg_alpha_update_mode == PPA_ALPHA_SCALE ||
	    config->fg_alpha_update_mode == PPA_ALPHA_SCALE) {
		return ESP_ERR_NOT_SUPPORTED;
	}

	k_mutex_lock(&ppa_state.lock, K_FOREVER);

	k_spinlock_key_t key = k_spin_lock(&ppa_state.queue_lock);
	struct esp_ppa_txn *txn = esp_ppa_alloc_txn();

	if (txn == NULL || (config->mode == PPA_TRANS_MODE_NON_BLOCKING &&
			    ppa_client->pending_count >= ppa_client->max_pending)) {
		if (txn != NULL) {
			txn->in_use = false;
		}
		k_spin_unlock(&ppa_state.queue_lock, key);
		k_mutex_unlock(&ppa_state.lock);
		return ESP_FAIL;
	}
	k_spin_unlock(&ppa_state.queue_lock, key);

	txn->op = ESP_PPA_OP_BLEND;
	txn->client = ppa_client;
	txn->burst_bytes = esp_ppa_burst_bytes(ppa_client->burst_length);
	txn->blend_cfg = *config;
	txn->user_data = config->user_data;

	esp_ppa_fill_desc(&txn->bg_desc, (void *)config->in_bg.buffer, config->in_bg.pic_w,
			  config->in_bg.pic_h, config->in_bg.block_w, config->in_bg.block_h,
			  config->in_bg.block_offset_x, config->in_bg.block_offset_y, bg_pbyte);
	esp_ppa_fill_desc(&txn->fg_desc, (void *)config->in_fg.buffer, config->in_fg.pic_w,
			  config->in_fg.pic_h, config->in_fg.block_w, config->in_fg.block_h,
			  config->in_fg.block_offset_x, config->in_fg.block_offset_y, fg_pbyte);
	esp_ppa_fill_desc(&txn->rx_desc, config->out.buffer, config->out.pic_w, config->out.pic_h,
			  config->in_fg.block_w, config->in_fg.block_h, config->out.block_offset_x,
			  config->out.block_offset_y, out_pbyte);

	esp_err_t ret = esp_ppa_run(txn, config->mode);

	k_mutex_unlock(&ppa_state.lock);

	return ret;
}

esp_err_t ppa_do_scale_rotate_mirror(ppa_client_handle_t ppa_client,
				     const ppa_srm_oper_config_t *config)
{
	if (ppa_client == NULL || config == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	if (ppa_client->oper_type != PPA_OPERATION_SRM) {
		return ESP_ERR_INVALID_ARG;
	}

	if (!(config->scale_x > 0.0f) || !(config->scale_y > 0.0f) ||
	    config->scale_x >= ESP_PPA_MAX_SCALE || config->scale_y >= ESP_PPA_MAX_SCALE) {
		return ESP_ERR_INVALID_ARG;
	}

	uint32_t in_pbyte = esp_ppa_pbyte(config->in.srm_cm);
	uint32_t out_pbyte = esp_ppa_pbyte(config->out.srm_cm);
	uint32_t out_block_w = (uint32_t)(config->in.block_w * config->scale_x);
	uint32_t out_block_h = (uint32_t)(config->in.block_h * config->scale_y);

	if (config->in.buffer == NULL || config->out.buffer == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	if (!esp_ppa_geometry_valid(config->in.pic_w, config->in.pic_h, config->in.block_w,
				    config->in.block_h, config->in.block_offset_x,
				    config->in.block_offset_y, in_pbyte) ||
	    !esp_ppa_geometry_valid(config->out.pic_w, config->out.pic_h, out_block_w, out_block_h,
				    config->out.block_offset_x, config->out.block_offset_y,
				    out_pbyte)) {
		return ESP_ERR_INVALID_ARG;
	}

	k_mutex_lock(&ppa_state.lock, K_FOREVER);

	k_spinlock_key_t key = k_spin_lock(&ppa_state.queue_lock);
	struct esp_ppa_txn *txn = esp_ppa_alloc_txn();

	if (txn == NULL || (config->mode == PPA_TRANS_MODE_NON_BLOCKING &&
			    ppa_client->pending_count >= ppa_client->max_pending)) {
		if (txn != NULL) {
			txn->in_use = false;
		}
		k_spin_unlock(&ppa_state.queue_lock, key);
		k_mutex_unlock(&ppa_state.lock);
		return ESP_FAIL;
	}
	k_spin_unlock(&ppa_state.queue_lock, key);

	txn->op = ESP_PPA_OP_SRM;
	txn->client = ppa_client;
	txn->burst_bytes = esp_ppa_burst_bytes(ppa_client->burst_length);
	txn->srm_cfg = *config;
	txn->user_data = config->user_data;

	esp_ppa_fill_desc(&txn->bg_desc, (void *)config->in.buffer, config->in.pic_w,
			  config->in.pic_h, config->in.block_w, config->in.block_h,
			  config->in.block_offset_x, config->in.block_offset_y, in_pbyte);
	esp_ppa_fill_desc(&txn->rx_desc, config->out.buffer, config->out.pic_w, config->out.pic_h,
			  out_block_w, out_block_h, config->out.block_offset_x,
			  config->out.block_offset_y, out_pbyte);

	esp_err_t ret = esp_ppa_run(txn, config->mode);

	k_mutex_unlock(&ppa_state.lock);

	return ret;
}

esp_err_t ppa_set_rgb2gray_formula(uint8_t r_weight, uint8_t g_weight, uint8_t b_weight)
{
	ARG_UNUSED(r_weight);
	ARG_UNUSED(g_weight);
	ARG_UNUSED(b_weight);

	return ESP_ERR_NOT_SUPPORTED;
}
