/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_PRIVATE_H_
#define ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_PRIVATE_H_

#include "soc/soc_caps.h"
#include "hal/rmt_types.h"
#include "hal/rmt_hal.h"
#include "hal/dma_types.h"
#include "esp_intr_alloc.h"
#include "esp_heap_caps.h"
#include "esp_clk_tree.h"
#include "esp_pm.h"
#include "esp_attr.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/misc/espressif_rmt/rmt_common.h>
#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_ESPRESSIF_RMT_ISR_IRAM_SAFE || CONFIG_ESPRESSIF_RMT_RECV_FUNC_IN_IRAM
#define RMT_MEM_ALLOC_CAPS (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
#else
#define RMT_MEM_ALLOC_CAPS MALLOC_CAP_DEFAULT
#endif

/* RMT driver object is per-channel, the interrupt source is shared between channels */
#if CONFIG_ESPRESSIF_RMT_ISR_IRAM_SAFE
#define RMT_INTR_ALLOC_FLAG (ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_IRAM)
#else
#define RMT_INTR_ALLOC_FLAG (ESP_INTR_FLAG_SHARED)
#endif

/* Hopefully the channel offset won't change in other targets */
#define RMT_TX_CHANNEL_OFFSET_IN_GROUP (0)
#define RMT_RX_CHANNEL_OFFSET_IN_GROUP \
	(SOC_RMT_CHANNELS_PER_GROUP - SOC_RMT_TX_CANDIDATES_PER_GROUP)

#define RMT_ALLOW_INTR_PRIORITY_MASK ESP_INTR_FLAG_LOWMED

/* DMA buffer size must align to `rmt_symbol_word_t` */
#define RMT_DMA_DESC_BUF_MAX_SIZE \
	(DMA_DESCRIPTOR_BUFFER_MAX_SIZE & ~(sizeof(rmt_symbol_word_t) - 1))

/* two nodes ping-pong */
#define RMT_DMA_NODES_PING_PONG (2)

/* maximal length of PM lock name */
#define RMT_PM_LOCK_NAME_LEN_MAX (16)

/* Uninitialized priotity value */
#define RMT_GROUP_INTR_PRIORITY_UNINITALIZED (-1)

typedef struct {
	struct {
		rmt_symbol_word_t symbols[SOC_RMT_MEM_WORDS_PER_CHANNEL];
	} channels[SOC_RMT_CHANNELS_PER_GROUP];
} rmt_block_mem_t;

/* RMTMEM address is declared in <target>.peripherals.ld */
extern rmt_block_mem_t RMTMEM;

typedef enum {
	RMT_CHANNEL_DIRECTION_TX,
	RMT_CHANNEL_DIRECTION_RX,
} rmt_channel_direction_t;

typedef enum {
	RMT_FSM_INIT_WAIT,
	RMT_FSM_INIT,
	RMT_FSM_ENABLE_WAIT,
	RMT_FSM_ENABLE,
	RMT_FSM_RUN_WAIT,
	RMT_FSM_RUN,
} rmt_fsm_t;

enum {
	RMT_TX_QUEUE_READY,
	RMT_TX_QUEUE_PROGRESS,
	RMT_TX_QUEUE_COMPLETE,
	RMT_TX_QUEUE_MAX,
};

typedef struct rmt_group_t rmt_group_t;
typedef struct rmt_channel_t rmt_channel_t;
typedef struct rmt_tx_channel_t rmt_tx_channel_t;
typedef struct rmt_rx_channel_t rmt_rx_channel_t;
typedef struct rmt_sync_manager_t rmt_sync_manager_t;

struct rmt_group_t {
	/* !< Group ID, index from 0 */
	int group_id;
	/* !< To protect per-group register level concurrent access */
	struct k_spinlock spinlock;
	/* !< Hal layer for each group */
	rmt_hal_context_t hal;
	/* !< Record the group clock source, group clock is shared by all channels */
	rmt_clock_source_t clk_src;
	/* !< Resolution of group clock */
	uint32_t resolution_hz;
	/* !< A set bit in the mask indicates the channel is not available */
	uint32_t occupy_mask;
	/* !< Array of RMT TX channels */
	rmt_tx_channel_t *tx_channels[SOC_RMT_TX_CANDIDATES_PER_GROUP];
	/* !< Array of RMT RX channels */
	rmt_rx_channel_t *rx_channels[SOC_RMT_RX_CANDIDATES_PER_GROUP];
	/* !< Sync manager, this can be extended into an array
	 * if there're more sync controllers in one RMT group
	 */
	rmt_sync_manager_t *sync_manager;
	/* !< RMT interrupt priority */
	int intr_priority;
};

struct rmt_channel_t {
	/* !< Channel ID, index from 0 */
	int channel_id;
	/* !< Mask of the memory blocks that occupied by the channel */
	uint32_t channel_mask;
	/* !< Number of occupied RMT memory blocks */
	size_t mem_block_num;
	/* !< Which group the channel belongs to */
	rmt_group_t *group;
	/* !< Prevent channel resource accessing by user and interrupt concurrently */
	struct k_spinlock spinlock;
	/* !< Channel clock resolution */
	uint32_t resolution_hz;
	/* !< Allocated interrupt handle for each channel */
	intr_handle_t intr;
	/* !< Channel life cycle specific FSM */
	atomic_t fsm;
	/* !< Channel direction */
	rmt_channel_direction_t direction;
	/* !< Base address of RMT channel hardware memory */
	rmt_symbol_word_t *hw_mem_base;
	/* !< Base address of RMT channel DMA buffer */
	rmt_symbol_word_t *dma_mem_base;
	/* !< Size of RMT channel DMA buffer */
	size_t dma_mem_size;
	/* !< Channel used with DMA capability */
	bool with_dma;
	/* !< DMA instance */
	const struct device *dma_dev;
	/* !< DMA channel */
	uint8_t dma_channel;
#if CONFIG_ESPRESSIF_RMT_PM
	/* !< PM lock */
	esp_pm_lock_handle_t pm_lock;
	/* !< PM lock name */
	char pm_lock_name[RMT_PM_LOCK_NAME_LEN_MAX];
#endif
	/* RMT channel common interface */
	/* The following IO functions will have per-implementation for TX and RX channel */
	int (*del)(rmt_channel_t *channel);
	int (*set_carrier_action)(rmt_channel_t *channel, const rmt_carrier_config_t *config);
	int (*enable)(rmt_channel_t *channel);
	int (*disable)(rmt_channel_t *channel);
};

typedef struct {
	/* !< Encode user payload into RMT symbols */
	rmt_encoder_handle_t encoder;
	/* !< Encoder payload */
	const void *payload;
	/* !< Payload size */
	size_t payload_bytes;
	/* !< Transaction can be continued in a loop for specific times */
	int loop_count;
	/* !< User required loop count may exceed hardware limitation,
	 * the driver will transfer them in batches
	 */
	int remain_loop_count;
	/* !< Track the number of transmitted symbols */
	size_t transmitted_symbol_num;
	struct {
		/* !< Set the output level for the "End Of Transmission" */
		uint32_t eot_level : 1;
		/* !< Indicate whether the encoding has finished
		 * (not the encoding of transmission)
		 */
		uint32_t encoding_done: 1;
	} flags;
} rmt_tx_trans_desc_t;

struct rmt_tx_channel_t {
	/* !< Channel base class */
	rmt_channel_t base;
	/* !< Runtime argument, indicating the next writing position in the RMT hardware memory */
	size_t mem_off;
	/* !< Runtime argument, indicating the end of current writing region */
	size_t mem_end;
	/* !< Ping-pong size (half of the RMT channel memory) */
	size_t ping_pong_symbols;
	/* !< Size of transaction queue */
	size_t queue_size;
	/* !< Indicates the number of transactions that are undergoing but not recycled to
	 * ready_queue
	 */
	size_t num_trans_inflight;
	/* !< Transaction queues */
	struct k_msgq trans_queues[RMT_TX_QUEUE_MAX];
	/* !< Memory to store the static structure for trans_queues */
	struct msg *trans_queue_structs[RMT_TX_QUEUE_MAX];
	/* !< Points to current transaction */
	rmt_tx_trans_desc_t *cur_trans;
	/* !< User context */
	void *user_data;
	/* !< Callback, invoked on trans done */
	rmt_tx_done_callback_t on_trans_done;
	/* !< Transfer descriptor pool */
	rmt_tx_trans_desc_t trans_desc_pool[];
};

typedef struct {
	/* !< Buffer for saving the received symbols */
	void *buffer;
	/* !< Size of the buffer, in bytes */
	size_t buffer_size;
	/* !< Track the number of received symbols */
	size_t received_symbol_num;
	/* !< Tracking offset in the copy destination */
	size_t copy_dest_off;
} rmt_rx_trans_desc_t;

struct rmt_rx_channel_t {
	/* !< Channel base class */
	rmt_channel_t base;
	/* !< Filter clock resolution, in Hz */
	uint32_t filter_clock_resolution_hz;
	/* !< Starting offset to fetch the symbols in RMT-MEM */
	size_t mem_off;
	/* !< Ping-pong size (half of the RMT channel memory) */
	size_t ping_pong_symbols;
	/* !< Callback, invoked on receive done */
	rmt_rx_done_callback_t on_recv_done;
	/* !< User context */
	void *user_data;
	/* !< Transaction description */
	rmt_rx_trans_desc_t trans_desc;
	/* !< Number of DMA nodes, determined by how big the memory block that user configures */
	size_t num_dma_nodes;
};

/**
 * @brief Acquire RMT group handle.
 *
 * @param group_id Group ID.
 * @retval RMT group handle.
 */
rmt_group_t *rmt_acquire_group_handle(int group_id);

/**
 * @brief Release RMT group handle.
 *
 * @param group RMT group handle, returned from `rmt_acquire_group_handle`.
 */
void rmt_release_group_handle(rmt_group_t *group);

/**
 * @brief Set clock source for RMT peripheral
 *
 * @param channel RMT channel handle.
 * @param clk_src Clock source.
 * @retval 0 If successful.
 * @retval -EINVAL if setting clock source failed because the clk_src is not supported.
 * @retval -ENODEV if setting clock source failed because the clk_src is different from other RMT
 * channel.
 * @retval -ENODEV if setting clock source failed because of other error.
 */
int rmt_select_periph_clock(rmt_channel_handle_t channel, rmt_clock_source_t clk_src);

/**
 * @brief Set interrupt priority to RMT group.
 *
 * @param group RMT group to set interrupt priority to.
 * @param intr_priority User-specified interrupt priority (in num, not bitmask).
 * @retval false if interrupt priority set successfully.
 * @retval true if interrupt priority conflict with previous specified.
 */
bool rmt_set_intr_priority_to_group(rmt_group_t *group, int intr_priority);

/**
 * @brief Get isr_flags to be passed to `esp_intr_alloc_intrstatus()` according to `intr_priority`
 * set in RMT group.
 *
 * @param group RMT group.
 * @retval ISR flags value.
 */
int rmt_get_isr_flags(rmt_group_t *group);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_PRIVATE_H_ */
