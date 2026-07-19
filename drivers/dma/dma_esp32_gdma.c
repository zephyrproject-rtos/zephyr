/*
 * Copyright (c) 2022-2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_gdma

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_esp32_gdma, CONFIG_DMA_LOG_LEVEL);

#include <hal/gdma_hal.h>
#include <hal/gdma_hal_ahb.h>
#if defined(SOC_AXI_GDMA_SUPPORTED)
#include <hal/gdma_hal_axi.h>
#include <hal/ahb_dma_ll.h>
#include <hal/axi_dma_ll.h>
#endif
#include <hal/gdma_channel.h>
#include <hal/gdma_ll.h>
#include <hal/dma_types.h>
#include <hal/gdma_types.h>
#include <zephyr/cache.h>

#include <soc.h>
#include <esp_memory_utils.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/pm/policy.h>

#if CONFIG_ESP32_PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP && SOC_GDMA_SUPPORT_SLEEP_RETENTION
#define GDMA_SLEEP_RETENTION_ENABLED 1
#else
#define GDMA_SLEEP_RETENTION_ENABLED 0
#endif

#if GDMA_SLEEP_RETENTION_ENABLED
#include <hal/gdma_periph.h>
#include <esp_private/sleep_retention.h>

static esp_err_t gdma_create_sleep_retention_cb(void *arg)
{
	const gdma_chx_reg_ctx_link_t *ctx = (const gdma_chx_reg_ctx_link_t *)arg;

	return sleep_retention_entries_create(ctx->link_list, ctx->link_num,
					      REGDMA_LINK_PRI_GDMA, ctx->module_id);
}
#endif

#define DMA_MAX_CHANNEL GDMA_LL_PAIRS_PER_INST

/*
 * On some SoCs (ESP32-C2, ESP32-C3) RX and TX of a channel pair share a
 * single interrupt source. Detect this from DT by comparing the number of
 * interrupts against the number of channel pairs.
 */
#define DMA_ESP32_SHARED_IRQ (DT_NUM_IRQS(DT_DRV_INST(0)) == DT_INST_PROP(0, dma_channels) / 2)

#if defined(SOC_AXI_GDMA_SUPPORTED)
/*
 * On dual-bus SoCs the hal defines GDMA_LL_*_EVENT_MASK as the AHB variant,
 * but the driver also drives the AXI instance. Redefine them to pick the AXI
 * or AHB variant from the runtime is_axi flag so call sites stay bus-agnostic.
 */
#undef GDMA_LL_RX_EVENT_MASK
#undef GDMA_LL_TX_EVENT_MASK
#define GDMA_LL_RX_EVENT_MASK (data->is_axi ? AXI_DMA_LL_RX_EVENT_MASK : AHB_DMA_LL_RX_EVENT_MASK)
#define GDMA_LL_TX_EVENT_MASK (data->is_axi ? AXI_DMA_LL_TX_EVENT_MASK : AHB_DMA_LL_TX_EVENT_MASK)
#elif !defined(GDMA_LL_RX_EVENT_MASK)
/*
 * Some AHB-only SoCs (e.g. esp32c5) also dropped the generic event-mask
 * macros, exposing only the AHB variant. Fall back to it.
 */
#define GDMA_LL_RX_EVENT_MASK AHB_DMA_LL_RX_EVENT_MASK
#define GDMA_LL_TX_EVENT_MASK AHB_DMA_LL_TX_EVENT_MASK
#endif

static int get_m2m_periph_id(void)
{
	return __builtin_ctz(GDMA_LL_M2M_FREE_PERIPH_ID_MASK);
}

struct dma_esp32_data {
	gdma_hal_context_t hal;
#if defined(SOC_AXI_GDMA_SUPPORTED)
	bool is_axi;
#endif
#if CONFIG_PM
	bool pm_policy_state_on;
#endif
};

enum dma_channel_dir {
	DMA_RX,
	DMA_TX,
	DMA_UNCONFIGURED
};

struct irq_config {
	uint8_t irq_source;
	uint8_t irq_priority;
	int irq_flags;
};

/*
 * AXI DMA requires 8-byte aligned descriptors (16 bytes each); AHB-only SoCs
 * use 4-byte aligned ones (12 bytes). Use the larger type when AXI is present
 * so it works for both AHB and AXI instances.
 */
#if defined(SOC_AXI_GDMA_SUPPORTED)
typedef dma_descriptor_align8_t esp_dma_desc_t;
#else
typedef dma_descriptor_t esp_dma_desc_t;
#endif

struct dma_esp32_channel {
	uint8_t dir;
	uint8_t channel_id;
	int periph_id;
	dma_callback_t cb;
	void *user_data;
	esp_dma_desc_t desc_list[CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM];
#if CONFIG_PM
	bool m2m_transfer;
#endif
};

struct dma_esp32_config {
	struct irq_config *irq_config;
	uint8_t irq_size;
	void **irq_handlers;
	uint8_t dma_channel_max;
	uint8_t sram_alignment;
	struct dma_esp32_channel dma_channel[DMA_MAX_CHANNEL * 2];
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

/* LL wrappers: dual-bus SoCs have AHB and AXI; others use the unified API. */
#if defined(SOC_AXI_GDMA_SUPPORTED)

static inline bool dma_ll_rx_is_fsm_idle(struct dma_esp32_data *data, int ch)
{
	if (data->is_axi) {
		return axi_dma_ll_rx_is_desc_fsm_idle(data->hal.axi_dma_dev, ch);
	}
	return ahb_dma_ll_rx_is_desc_fsm_idle(data->hal.ahb_dma_dev, ch);
}

static inline bool dma_ll_tx_is_fsm_idle(struct dma_esp32_data *data, int ch)
{
	if (data->is_axi) {
		return axi_dma_ll_tx_is_desc_fsm_idle(data->hal.axi_dma_dev, ch);
	}
	return ahb_dma_ll_tx_is_desc_fsm_idle(data->hal.ahb_dma_dev, ch);
}

static inline intptr_t dma_ll_rx_get_prefetched_desc(struct dma_esp32_data *data, int ch)
{
	if (data->is_axi) {
		return axi_dma_ll_rx_get_prefetched_desc_addr(data->hal.axi_dma_dev, ch);
	}
	return ahb_dma_ll_rx_get_prefetched_desc_addr(data->hal.ahb_dma_dev, ch);
}

static inline intptr_t dma_ll_tx_get_prefetched_desc(struct dma_esp32_data *data, int ch)
{
	if (data->is_axi) {
		return axi_dma_ll_tx_get_prefetched_desc_addr(data->hal.axi_dma_dev, ch);
	}
	return ahb_dma_ll_tx_get_prefetched_desc_addr(data->hal.ahb_dma_dev, ch);
}

static inline void dma_ll_force_reg_clock(struct dma_esp32_data *data, bool en)
{
	if (data->is_axi) {
		axi_dma_ll_force_enable_reg_clock(data->hal.axi_dma_dev, en);
	} else {
		ahb_dma_ll_force_enable_reg_clock(data->hal.ahb_dma_dev, en);
	}
}

#else /* !SOC_AXI_GDMA_SUPPORTED */

static inline bool dma_ll_rx_is_fsm_idle(struct dma_esp32_data *data, int ch)
{
	return gdma_ll_rx_is_desc_fsm_idle(data->hal.dev, ch);
}

static inline bool dma_ll_tx_is_fsm_idle(struct dma_esp32_data *data, int ch)
{
	return gdma_ll_tx_is_desc_fsm_idle(data->hal.dev, ch);
}

static inline intptr_t dma_ll_rx_get_prefetched_desc(struct dma_esp32_data *data, int ch)
{
	return gdma_ll_rx_get_prefetched_desc_addr(data->hal.dev, ch);
}

static inline intptr_t dma_ll_tx_get_prefetched_desc(struct dma_esp32_data *data, int ch)
{
	return gdma_ll_tx_get_prefetched_desc_addr(data->hal.dev, ch);
}

static inline void dma_ll_force_reg_clock(struct dma_esp32_data *data, bool en)
{
	gdma_ll_force_enable_reg_clock(data->hal.dev, en);
}

#endif /* SOC_AXI_GDMA_SUPPORTED */

#if CONFIG_PM
static void IRAM_ATTR dma_esp32_pm_policy_state_lock_get(const struct device *dev)
{
	struct dma_esp32_data *data = dev->data;
	unsigned int key = irq_lock();

	if (!data->pm_policy_state_on) {
		data->pm_policy_state_on = true;
		pm_policy_state_all_lock_get();
	}

	irq_unlock(key);
}

static void IRAM_ATTR dma_esp32_pm_policy_state_lock_put(const struct device *dev)
{
	struct dma_esp32_data *data = dev->data;
	unsigned int key = irq_lock();

	if (data->pm_policy_state_on) {
		data->pm_policy_state_on = false;
		pm_policy_state_all_lock_put();
	}

	irq_unlock(key);
}
#endif

/*
 * The descriptor list is owned by the CPU except for dw0.length and dw0.owner,
 * which the GDMA writes back. Walking the list on dw0.size and next is safe
 * without invalidating the descriptors themselves.
 */
static void dma_esp32_cache_flush_data(struct dma_esp32_channel *dma_channel)
{
	esp_dma_desc_t *desc = dma_channel->desc_list;

	for (int i = 0; i < CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM && desc; ++i) {
		if (desc->buffer && desc->dw0.size) {
			sys_cache_data_flush_range(desc->buffer, desc->dw0.size);
		}
		desc = desc->next;
	}
}

/*
 * Invalidating a GDMA-written buffer also drops the rest of any cache line it
 * shares with CPU-owned data. When the buffer is not cache-line aligned or
 * sized, flush the head and tail lines first so adjacent dirty data is written
 * back to memory before the invalidate discards it.
 */
static void dma_esp32_cache_invd_data(struct dma_esp32_channel *dma_channel)
{
	const size_t line = sys_cache_data_line_size_get();
	esp_dma_desc_t *desc = dma_channel->desc_list;

	for (int i = 0; i < CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM && desc; ++i) {
		if (desc->buffer && desc->dw0.size) {
			uintptr_t start = (uintptr_t)desc->buffer;
			uintptr_t end = start + desc->dw0.size;

			if (line && (start & (line - 1))) {
				sys_cache_data_flush_range((void *)start, 1);
			}
			if (line && (end & (line - 1))) {
				sys_cache_data_flush_range((void *)(end - 1), 1);
			}
			sys_cache_data_invd_range(desc->buffer, desc->dw0.size);
		}
		desc = desc->next;
	}
}

static void IRAM_ATTR dma_esp32_isr_handle_rx(const struct device *dev,
					      struct dma_esp32_channel *rx, uint32_t intr_status)
{
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;
	int status = -EIO;
	bool pm_unlock = false;

	gdma_hal_clear_intr(&data->hal, rx->channel_id, GDMA_CHANNEL_DIRECTION_RX, intr_status);

	if (intr_status & GDMA_LL_EVENT_RX_SUC_EOF) {
		dma_esp32_cache_invd_data(rx);
		status = DMA_STATUS_COMPLETE;
		pm_unlock = true;
	} else if (intr_status & GDMA_LL_EVENT_RX_DONE) {
		status = DMA_STATUS_BLOCK;
#if defined(CONFIG_SOC_SERIES_ESP32S3)
	} else if (intr_status & GDMA_LL_EVENT_RX_WATER_MARK) {
		status = DMA_STATUS_BLOCK;
#endif
	} else {
		status = -intr_status;
		pm_unlock = true;
	}

#if CONFIG_PM
	if (pm_unlock && rx->m2m_transfer) {
		rx->m2m_transfer = false;
		dma_esp32_pm_policy_state_lock_put(dev);
	}
#endif

	if (rx->cb) {
		rx->cb(dev, rx->user_data, rx->channel_id * 2, status);
	}
}

static void IRAM_ATTR dma_esp32_isr_handle_tx(const struct device *dev,
					      struct dma_esp32_channel *tx, uint32_t intr_status)
{
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;

	gdma_hal_clear_intr(&data->hal, tx->channel_id, GDMA_CHANNEL_DIRECTION_TX, intr_status);
	if (tx->cb) {
		intr_status &= ~(GDMA_LL_EVENT_TX_TOTAL_EOF | GDMA_LL_EVENT_TX_DONE |
				 GDMA_LL_EVENT_TX_EOF);

		tx->cb(dev, tx->user_data, tx->channel_id * 2 + 1, -intr_status);
	}
}

#if DMA_ESP32_SHARED_IRQ
static void IRAM_ATTR dma_esp32_isr_handle(const struct device *dev, uint8_t rx_id, uint8_t tx_id)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;
	struct dma_esp32_channel *dma_channel_rx = &config->dma_channel[rx_id];
	struct dma_esp32_channel *dma_channel_tx = &config->dma_channel[tx_id];
	uint32_t intr_status = 0;

	intr_status = gdma_hal_read_intr_status(&data->hal, dma_channel_rx->channel_id,
						GDMA_CHANNEL_DIRECTION_RX, false);
	if (intr_status) {
		dma_esp32_isr_handle_rx(dev, dma_channel_rx, intr_status);
	}

	intr_status = gdma_hal_read_intr_status(&data->hal, dma_channel_tx->channel_id,
						GDMA_CHANNEL_DIRECTION_TX, false);
	if (intr_status) {
		dma_esp32_isr_handle_tx(dev, dma_channel_tx, intr_status);
	}
}
#endif

static int dma_esp32_config_descriptor(struct dma_esp32_channel *dma_channel,
				       struct dma_block_config *block)
{
	if (!block) {
		LOG_ERR("At least one dma block is required");
		return -EINVAL;
	}

	uint32_t target_address = 0, block_size = 0;
	esp_dma_desc_t *desc_iter = dma_channel->desc_list;

	for (int i = 0; i < CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM; ++i) {
		if (block_size == 0) {
			if (dma_channel->dir == DMA_TX) {
				target_address = block->source_address;
			} else {
				target_address = block->dest_address;
			}

			bool dma_capable = esp_ptr_dma_capable((uint32_t *)target_address);

#if defined(CONFIG_ESP_SPIRAM)
			dma_capable =
				dma_capable || esp_ptr_dma_ext_capable((uint32_t *)target_address);
#endif
#if defined(SOC_DMA_CAN_ACCESS_FLASH)
			/*
			 * On SoCs whose DMA can read flash-mapped memory, a TX
			 * source buffer may live in DROM (e.g. a const buffer).
			 * Flash is read-only, so only allow it for the TX path.
			 */
			if (dma_channel->dir == DMA_TX) {
				dma_capable =
					dma_capable || esp_ptr_in_drom((uint32_t *)target_address);
			}
#endif
			if (!dma_capable) {
				if (dma_channel->dir == DMA_TX) {
					LOG_ERR("Tx buffer not in DMA capable memory: %p",
						(uint32_t *)target_address);
				} else {
					LOG_ERR("Rx buffer not in DMA capable memory: %p",
						(uint32_t *)target_address);
				}

				return -EINVAL;
			}

			block_size = block->block_size;
		}

		uint32_t buffer_size;

		if (block_size > DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED) {
			buffer_size = DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED;
		} else {
			buffer_size = block_size;
		}

		memset(desc_iter, 0, sizeof(esp_dma_desc_t));
		desc_iter->buffer = (void *)target_address;
		desc_iter->dw0.size = buffer_size;
		if (dma_channel->dir == DMA_TX) {
			desc_iter->dw0.length = buffer_size;
		}
		desc_iter->dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;

		target_address += buffer_size;
		block_size -= buffer_size;

		if (!block_size) {
			if (block->next_block) {
				block = block->next_block;
			} else {
				desc_iter->next = NULL;
				if (dma_channel->dir == DMA_TX) {
					desc_iter->dw0.suc_eof = 1;
				}
				break;
			}
		}

		desc_iter->next = desc_iter + 1;
		desc_iter += 1;
	}

	if (desc_iter->next) {
		memset(dma_channel->desc_list, 0, sizeof(dma_channel->desc_list));
		LOG_ERR("Run out of DMA descriptors. Increase CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM");
		return -EINVAL;
	}

	/* Write back cache so the GDMA hardware sees fresh descriptor values.
	 * No-op when CACHE_MANAGEMENT is disabled (SoCs without a data cache).
	 */
	sys_cache_data_flush_range(dma_channel->desc_list, sizeof(dma_channel->desc_list));

	if (dma_channel->dir == DMA_TX) {
		dma_esp32_cache_flush_data(dma_channel);
	} else {
		dma_esp32_cache_invd_data(dma_channel);
	}

	return 0;
}

static int dma_esp32_config_rx(const struct device *dev, struct dma_esp32_channel *dma_channel,
			       struct dma_config *config_dma)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;

	dma_channel->dir = DMA_RX;

	gdma_hal_reset(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_RX);

	if (dma_channel->periph_id == SOC_GDMA_TRIG_PERIPH_M2M0) {
		gdma_hal_connect_mem(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_RX,
				     get_m2m_periph_id());
	} else {
		gdma_hal_connect_peri(&data->hal, dma_channel->channel_id,
				      GDMA_CHANNEL_DIRECTION_RX, dma_channel->periph_id);
	}

	if (config_dma->dest_burst_length) {
		gdma_hal_enable_burst(&data->hal, dma_channel->channel_id,
				      GDMA_CHANNEL_DIRECTION_RX, config->sram_alignment >= 4,
				      config->sram_alignment >= 4);
	}

	gdma_hal_set_strategy(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_RX, true,
			      false, false);

	dma_channel->cb = config_dma->dma_callback;
	dma_channel->user_data = config_dma->user_data;

	gdma_hal_enable_intr(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_RX,
			     GDMA_LL_RX_EVENT_MASK, false);

	gdma_hal_clear_intr(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_RX,
			    GDMA_LL_RX_EVENT_MASK);

	return dma_esp32_config_descriptor(dma_channel, config_dma->head_block);
}

static int dma_esp32_config_tx(const struct device *dev, struct dma_esp32_channel *dma_channel,
			       struct dma_config *config_dma)
{
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;

	dma_channel->dir = DMA_TX;

	gdma_hal_reset(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_TX);

	if (dma_channel->periph_id == SOC_GDMA_TRIG_PERIPH_M2M0) {
		gdma_hal_connect_mem(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_TX,
				     get_m2m_periph_id());
	} else {
		gdma_hal_connect_peri(&data->hal, dma_channel->channel_id,
				      GDMA_CHANNEL_DIRECTION_TX, dma_channel->periph_id);
	}

	if (config_dma->source_burst_length) {
		gdma_hal_enable_burst(&data->hal, dma_channel->channel_id,
				      GDMA_CHANNEL_DIRECTION_TX, true, true);
	}

	dma_channel->cb = config_dma->dma_callback;
	dma_channel->user_data = config_dma->user_data;

	gdma_hal_enable_intr(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_TX,
			     GDMA_LL_TX_EVENT_MASK, false);

	gdma_hal_clear_intr(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_TX,
			    GDMA_LL_TX_EVENT_MASK);

	return dma_esp32_config_descriptor(dma_channel, config_dma->head_block);
}

static int dma_esp32_config(const struct device *dev, uint32_t channel,
			    struct dma_config *config_dma)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_channel *dma_channel = &config->dma_channel[channel];
	int ret = 0;

	if (channel >= config->dma_channel_max) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (!config_dma) {
		return -EINVAL;
	}

	if (config_dma->source_burst_length != config_dma->dest_burst_length) {
		LOG_ERR("Source and destination burst lengths must be equal");
		return -EINVAL;
	}

	dma_channel->periph_id = config_dma->channel_direction == MEMORY_TO_MEMORY
					 ? SOC_GDMA_TRIG_PERIPH_M2M0
					 : config_dma->dma_slot;

	dma_channel->channel_id = channel / 2;

	switch (config_dma->channel_direction) {
	case MEMORY_TO_MEMORY:
		struct dma_esp32_channel *dma_channel_rx =
			&config->dma_channel[dma_channel->channel_id * 2];
		struct dma_esp32_channel *dma_channel_tx =
			&config->dma_channel[(dma_channel->channel_id * 2) + 1];

		dma_channel_rx->channel_id = dma_channel->channel_id;
		dma_channel_tx->channel_id = dma_channel->channel_id;

		dma_channel_rx->periph_id = dma_channel->periph_id;
		dma_channel_tx->periph_id = dma_channel->periph_id;

		ret = dma_esp32_config_rx(dev, dma_channel_rx, config_dma);
		if (ret < 0) {
			break;
		}
		ret = dma_esp32_config_tx(dev, dma_channel_tx, config_dma);
		break;
	case PERIPHERAL_TO_MEMORY:
		ret = dma_esp32_config_rx(dev, dma_channel, config_dma);
		break;
	case MEMORY_TO_PERIPHERAL:
		ret = dma_esp32_config_tx(dev, dma_channel, config_dma);
		break;
	default:
		LOG_ERR("Invalid Channel direction");
		return -EINVAL;
	}

	return ret;
}

static int dma_esp32_start(const struct device *dev, uint32_t channel)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;
	struct dma_esp32_channel *dma_channel = &config->dma_channel[channel];

	if (channel >= config->dma_channel_max) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (dma_channel->periph_id == SOC_GDMA_TRIG_PERIPH_M2M0) {
		struct dma_esp32_channel *dma_channel_rx =
			&config->dma_channel[dma_channel->channel_id * 2];
		struct dma_esp32_channel *dma_channel_tx =
			&config->dma_channel[(dma_channel->channel_id * 2) + 1];

#if CONFIG_PM
		dma_channel_rx->m2m_transfer = true;
		dma_esp32_pm_policy_state_lock_get(dev);
#endif
		gdma_hal_enable_intr(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_RX,
				     GDMA_LL_EVENT_RX_SUC_EOF | GDMA_LL_EVENT_RX_DONE, true);
		gdma_hal_enable_intr(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_TX,
				     GDMA_LL_EVENT_TX_EOF, true);

		gdma_hal_start_with_desc(&data->hal, dma_channel->channel_id,
					 GDMA_CHANNEL_DIRECTION_RX,
					 (intptr_t)dma_channel_rx->desc_list);
		gdma_hal_start_with_desc(&data->hal, dma_channel->channel_id,
					 GDMA_CHANNEL_DIRECTION_TX,
					 (intptr_t)dma_channel_tx->desc_list);
	} else {
		if (dma_channel->dir == DMA_RX) {
			gdma_hal_enable_intr(
				&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_RX,
				GDMA_LL_EVENT_RX_SUC_EOF | GDMA_LL_EVENT_RX_DONE, true);
			gdma_hal_start_with_desc(&data->hal, dma_channel->channel_id,
						 GDMA_CHANNEL_DIRECTION_RX,
						 (intptr_t)dma_channel->desc_list);
		} else if (dma_channel->dir == DMA_TX) {
			gdma_hal_enable_intr(&data->hal, dma_channel->channel_id,
					     GDMA_CHANNEL_DIRECTION_TX, GDMA_LL_EVENT_TX_EOF, true);
			gdma_hal_start_with_desc(&data->hal, dma_channel->channel_id,
						 GDMA_CHANNEL_DIRECTION_TX,
						 (intptr_t)dma_channel->desc_list);
		} else {
			LOG_ERR("Channel %d is not configured", channel);
			return -EINVAL;
		}
	}

	return 0;
}

static int dma_esp32_stop(const struct device *dev, uint32_t channel)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;
	struct dma_esp32_channel *dma_channel = &config->dma_channel[channel];

	if (channel >= config->dma_channel_max) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (dma_channel->periph_id == SOC_GDMA_TRIG_PERIPH_M2M0) {
		gdma_hal_enable_intr(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_RX,
				     GDMA_LL_RX_EVENT_MASK, false);
		gdma_hal_enable_intr(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_TX,
				     GDMA_LL_TX_EVENT_MASK, false);
		gdma_hal_stop(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_RX);
		gdma_hal_stop(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_TX);
#if CONFIG_PM
		struct dma_esp32_channel *dma_channel_rx =
			&config->dma_channel[dma_channel->channel_id * 2];

		if (dma_channel_rx->m2m_transfer) {
			dma_channel_rx->m2m_transfer = false;
			dma_esp32_pm_policy_state_lock_put(dev);
		}
#endif
	}

	if (dma_channel->dir == DMA_RX) {
		gdma_hal_enable_intr(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_RX,
				     GDMA_LL_RX_EVENT_MASK, false);
		gdma_hal_stop(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_RX);
	} else if (dma_channel->dir == DMA_TX) {
		gdma_hal_enable_intr(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_TX,
				     GDMA_LL_TX_EVENT_MASK, false);
		gdma_hal_stop(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_TX);
	}

	return 0;
}

static int dma_esp32_get_status(const struct device *dev, uint32_t channel,
				struct dma_status *status)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;
	struct dma_esp32_channel *dma_channel = &config->dma_channel[channel];
	esp_dma_desc_t *desc;

	if (channel >= config->dma_channel_max) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (!status) {
		return -EINVAL;
	}

	memset(status, 0, sizeof(struct dma_status));

	if (dma_channel->dir == DMA_RX) {
		status->busy = !dma_ll_rx_is_fsm_idle(data, dma_channel->channel_id);
		status->dir = PERIPHERAL_TO_MEMORY;
		desc = (esp_dma_desc_t *)dma_ll_rx_get_prefetched_desc(data,
								       dma_channel->channel_id);
		if (desc >= dma_channel->desc_list) {
			/*
			 * The GDMA writes the received length back into the
			 * descriptor in memory. On SoCs with a data cache the CPU
			 * copy is stale, so invalidate just the prefetched
			 * descriptor before reading dw0.length.
			 */
			sys_cache_data_invd_range(desc, sizeof(*desc));
			status->read_position = desc - dma_channel->desc_list;
			status->total_copied = desc->dw0.length
						+ dma_channel->desc_list[0].dw0.size
						* status->read_position;
		}
	} else if (dma_channel->dir == DMA_TX) {
		status->busy = !dma_ll_tx_is_fsm_idle(data, dma_channel->channel_id);
		status->dir = MEMORY_TO_PERIPHERAL;
		desc = (esp_dma_desc_t *)dma_ll_tx_get_prefetched_desc(data,
								       dma_channel->channel_id);
		if (desc >= dma_channel->desc_list) {
			status->write_position = desc - dma_channel->desc_list;
		}
	}

	return 0;
}

static int dma_esp32_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			    size_t size)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;
	struct dma_esp32_channel *dma_channel = &config->dma_channel[channel];
	esp_dma_desc_t *desc_iter = dma_channel->desc_list;
	uint32_t buf;

	if (channel >= config->dma_channel_max) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (dma_channel->dir == DMA_RX) {
		gdma_hal_reset(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_RX);
		buf = dst;
	} else if (dma_channel->dir == DMA_TX) {
		gdma_hal_reset(&data->hal, dma_channel->channel_id, GDMA_CHANNEL_DIRECTION_TX);
		buf = src;
	} else {
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(dma_channel->desc_list); ++i) {
		memset(desc_iter, 0, sizeof(esp_dma_desc_t));
		desc_iter->buffer = (void *)(buf + DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED * i);
		desc_iter->dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
		if (size <= DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED) {
			desc_iter->dw0.size = size;
			if (dma_channel->dir == DMA_TX) {
				desc_iter->dw0.length = size;
				desc_iter->dw0.suc_eof = 1;
			}
			desc_iter->next = NULL;
			break;
		}
		desc_iter->dw0.size = DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED;
		if (dma_channel->dir == DMA_TX) {
			desc_iter->dw0.length = DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED;
		}
		size -= DMA_DESCRIPTOR_BUFFER_MAX_SIZE_4B_ALIGNED;
		desc_iter->next = desc_iter + 1;
		desc_iter += 1;
	}

	if (desc_iter->next) {
		memset(desc_iter, 0, sizeof(esp_dma_desc_t));
		LOG_ERR("Not enough DMA descriptors. Increase CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM");
		return -EINVAL;
	}

	sys_cache_data_flush_range(dma_channel->desc_list, sizeof(dma_channel->desc_list));

	if (dma_channel->dir == DMA_TX) {
		dma_esp32_cache_flush_data(dma_channel);
	} else {
		dma_esp32_cache_invd_data(dma_channel);
	}

	return 0;
}

static int dma_esp32_configure_irq(const struct device *dev)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *)dev->data;
	struct irq_config *irq_cfg = (struct irq_config *)config->irq_config;
	volatile uint32_t *status_reg;
	uint32_t status_mask;

	for (uint8_t i = 0; i < config->irq_size; i++) {
#if DMA_ESP32_SHARED_IRQ
		status_reg = gdma_ll_rx_get_interrupt_status_reg(data->hal.dev, i);
		status_mask = (GDMA_LL_RX_EVENT_MASK | GDMA_LL_TX_EVENT_MASK);
#else
		/* We assume device tree to feature RX/TX pairs */
		if (i % 2) {
			status_reg = (volatile uint32_t *)gdma_hal_get_intr_status_reg(
				&data->hal, i / 2, GDMA_CHANNEL_DIRECTION_TX);
			status_mask = GDMA_LL_TX_EVENT_MASK;
		} else {
			status_reg = (volatile uint32_t *)gdma_hal_get_intr_status_reg(
				&data->hal, i / 2, GDMA_CHANNEL_DIRECTION_RX);
			status_mask = GDMA_LL_RX_EVENT_MASK;
		}
#endif

		int ret = esp_intr_alloc_intrstatus(irq_cfg[i].irq_source,
			ESP_PRIO_TO_FLAGS(irq_cfg[i].irq_priority) |
				ESP_INT_FLAGS_CHECK(irq_cfg[i].irq_flags) | ESP_INTR_FLAG_IRAM,
			(uint32_t)status_reg,
			status_mask,
			(intr_handler_t)config->irq_handlers[i],
			(void *)dev,
			NULL);

		if (ret != 0) {
			LOG_ERR("Could not allocate interrupt handler");
			return ret;
		}
	}

	return 0;
}

static int dma_esp32_init(const struct device *dev)
{
	struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;
	struct dma_esp32_data *data = (struct dma_esp32_data *)dev->data;
	struct dma_esp32_channel *dma_channel;
	int ret = 0;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Could not initialize clock (%d)", ret);
		return ret;
	}

	for (uint8_t i = 0; i < DMA_MAX_CHANNEL * 2; i++) {
		dma_channel = &config->dma_channel[i];
		dma_channel->cb = NULL;
		dma_channel->dir = DMA_UNCONFIGURED;
		dma_channel->periph_id = ESP_GDMA_TRIG_PERIPH_INVALID;
		memset(dma_channel->desc_list, 0, sizeof(dma_channel->desc_list));
	}

#if defined(SOC_AXI_GDMA_SUPPORTED)
	/*
	 * Dual-bus SoCs have two gdma instances sharing the same compatible, told
	 * apart by base address: the AXI instance lives at DR_REG_AXI_DMA_BASE,
	 * the AHB instance does not. The hal init picks the matching register base.
	 */
	if ((uint32_t)data->hal.dev == DR_REG_AXI_DMA_BASE) {
		gdma_ll_enable_bus_clock(1, true);
		gdma_ll_reset_register(1);
		gdma_hal_config_t hal_config = {
			.group_id = GDMA_LL_AXI_GROUP_START_ID,
		};
		gdma_axi_hal_init(&data->hal, &hal_config);
		data->is_axi = true;
	} else {
		gdma_hal_config_t hal_config = {
			.group_id = GDMA_LL_AHB_GROUP_START_ID,
		};
		gdma_ahb_hal_init(&data->hal, &hal_config);
		data->is_axi = false;
	}
#else
	gdma_hal_config_t hal_config = {
		.group_id = GDMA_LL_AHB_GROUP_START_ID,
	};
	gdma_ahb_hal_init(&data->hal, &hal_config);
#endif
	dma_ll_force_reg_clock(data, true);

	ret = dma_esp32_configure_irq(dev);
	if (ret < 0) {
		LOG_ERR("Could not configure IRQ (%d)", ret);
		return ret;
	}

#if GDMA_SLEEP_RETENTION_ENABLED
	for (uint8_t pair = 0; pair < GDMA_LL_PAIRS_PER_INST; pair++) {
		const gdma_chx_reg_ctx_link_t *ctx =
			&gdma_chx_regs_retention[hal_config.group_id][pair];
		sleep_retention_module_init_param_t init_param = {
			.cbs = {.create = {.handle = gdma_create_sleep_retention_cb,
					   .arg = (void *)ctx}},
		};
		esp_err_t err = sleep_retention_module_init(ctx->module_id, &init_param);

		if (err == ESP_OK) {
			err = sleep_retention_module_allocate(ctx->module_id);
		}
		if (err != ESP_OK) {
			LOG_WRN("Failed to init GDMA sleep retention for pair %d", pair);
		}
	}
#endif

	return 0;
}

static DEVICE_API(dma, dma_esp32_api) = {
	.config = dma_esp32_config,
	.start = dma_esp32_start,
	.stop = dma_esp32_stop,
	.get_status = dma_esp32_get_status,
	.reload = dma_esp32_reload,
};

#if DMA_ESP32_SHARED_IRQ

#define DMA_ESP32_DEFINE_IRQ_HANDLER(channel)                                                      \
	__attribute__((unused)) static void IRAM_ATTR dma_esp32_isr_##channel(                     \
		const struct device *dev)                                                          \
	{                                                                                          \
		dma_esp32_isr_handle(dev, channel * 2, channel * 2 + 1);                           \
	}

#else

#define DMA_ESP32_DEFINE_IRQ_HANDLER(channel)                                                      \
	__attribute__((unused)) static void IRAM_ATTR dma_esp32_isr_##channel##_rx(                \
		const struct device *dev)                                                          \
	{                                                                                          \
		struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;          \
		struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;           \
		uint32_t intr_status = gdma_hal_read_intr_status(                                  \
			&data->hal, channel, GDMA_CHANNEL_DIRECTION_RX, false);                    \
		if (intr_status) {                                                                 \
			dma_esp32_isr_handle_rx(dev, &config->dma_channel[channel * 2],            \
						intr_status);                                      \
		}                                                                                  \
	}                                                                                          \
                                                                                                   \
	__attribute__((unused)) static void IRAM_ATTR dma_esp32_isr_##channel##_tx(                \
		const struct device *dev)                                                          \
	{                                                                                          \
		struct dma_esp32_config *config = (struct dma_esp32_config *)dev->config;          \
		struct dma_esp32_data *data = (struct dma_esp32_data *const)(dev)->data;           \
		uint32_t intr_status = gdma_hal_read_intr_status(                                  \
			&data->hal, channel, GDMA_CHANNEL_DIRECTION_TX, false);                    \
		if (intr_status) {                                                                 \
			dma_esp32_isr_handle_tx(dev, &config->dma_channel[channel * 2 + 1],        \
						intr_status);                                      \
		}                                                                                  \
	}

#endif

#if DMA_ESP32_SHARED_IRQ
#define ESP32_DMA_HANDLER(channel) dma_esp32_isr_##channel
#else
#define ESP32_DMA_HANDLER(channel) dma_esp32_isr_##channel##_rx, dma_esp32_isr_##channel##_tx
#endif

DMA_ESP32_DEFINE_IRQ_HANDLER(0)
#if DMA_MAX_CHANNEL >= 2
DMA_ESP32_DEFINE_IRQ_HANDLER(1)
#endif
#if DMA_MAX_CHANNEL >= 3
DMA_ESP32_DEFINE_IRQ_HANDLER(2)
#endif
#if DMA_MAX_CHANNEL >= 4
DMA_ESP32_DEFINE_IRQ_HANDLER(3)
#endif
#if DMA_MAX_CHANNEL >= 5
DMA_ESP32_DEFINE_IRQ_HANDLER(4)
#endif

static void *irq_handlers[] = {
	ESP32_DMA_HANDLER(0),
#if DMA_MAX_CHANNEL >= 2
	ESP32_DMA_HANDLER(1),
#endif
#if DMA_MAX_CHANNEL >= 3
	ESP32_DMA_HANDLER(2),
#endif
#if DMA_MAX_CHANNEL >= 4
	ESP32_DMA_HANDLER(3),
#endif
#if DMA_MAX_CHANNEL >= 5
	ESP32_DMA_HANDLER(4),
#endif
};

#define IRQ_NUM(idx) DT_NUM_IRQS(DT_DRV_INST(idx))
#define IRQ_ENTRY(n, idx)                                                                          \
	{DT_INST_IRQ_BY_IDX(idx, n, irq), DT_INST_IRQ_BY_IDX(idx, n, priority),                    \
	 DT_INST_IRQ_BY_IDX(idx, n, flags)},

/* clang-format off */
#define DMA_ESP32_INIT(idx)                                                  \
	static struct irq_config irq_config_##idx[] = {                      \
		LISTIFY(IRQ_NUM(idx), IRQ_ENTRY, (), idx)                    \
	};                                                                   \
	static struct dma_esp32_config dma_config_##idx = {                  \
		.irq_config = irq_config_##idx,                              \
		.irq_size = IRQ_NUM(idx),                                    \
		.irq_handlers = irq_handlers,                                \
		.dma_channel_max = DT_INST_PROP(idx, dma_channels),          \
		.sram_alignment =                                            \
			DT_INST_PROP(idx, dma_buf_addr_alignment),           \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),       \
		.clock_subsys =                                              \
			(void *)DT_INST_CLOCKS_CELL(idx, offset),            \
	};                                                                   \
	static struct dma_esp32_data dma_data_##idx = {                      \
		.hal = {                                                     \
			.dev = (void *)DT_INST_REG_ADDR(idx),                \
		},                                                           \
	};                                                                   \
	DEVICE_DT_INST_DEFINE(idx, dma_esp32_init, NULL,                     \
			      &dma_data_##idx, &dma_config_##idx,            \
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,        \
			      &dma_esp32_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(DMA_ESP32_INIT)
